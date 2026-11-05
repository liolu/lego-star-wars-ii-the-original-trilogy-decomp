#!/usr/bin/env python3
"""Compile un jeu DS decompile en programme natif lie au runtime nitro-pc : Windows 32 bits ou macOS (arm64, x86_64).

  python native_build.py <dossier du projet de jeu> [--version eu] [--out dossier] [--lift f1,f2] [-j 4]
  python native_build.py --recipe native/games/<code>.json.gz --rom <rom> [--out dossier]   (ce que fait le launcher)

C du jeu (src/ sauf src/data) -> gcc -m32. Le reste (assembleur, fonctions non matchees, fichiers que gcc refuse)
-> arm2c.py. Les donnees ne sont pas compilees : elles viennent de la rom de l'utilisateur, chargee aux adresses DS,
et chaque symbole de donnees devient un symbole absolu. Rien de la rom n'entre dans l'exe.
macOS (native/MACOS.md 3.6) : clang, code traduit seulement (pas de --with-c), memoire DS dans une fenetre (DS()) ;
le runtime trouve les donnees qu'il nomme dans nds_addr_<nom> au lieu de symboles absolus.
"""
import argparse
import hashlib
import json
import os
import platform
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import arm2c
import recipe

HERE = Path(__file__).resolve().parent
RUNTIME = HERE.parent / "nitro-pc"
WIN32 = sys.platform == "win32"
DARWIN = sys.platform == "darwin"
WINLIBS = r"C:\Users\thomr\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin"
# La boite a outils du launcher donne son compilateur (NDSGS_COMPILER_BIN : la boite commune, quand le runtime vient du depot
# du jeu) ; sinon celui installe sur ce PC
TOOLCHAIN_BIN = HERE.parent.parent / "mingw64" / "bin"
for bin_dir in (os.environ.get("NDSGS_COMPILER_BIN", ""), str(TOOLCHAIN_BIN), WINLIBS) if WIN32 else ():
    if bin_dir and os.path.isdir(bin_dir):
        os.environ["PATH"] = bin_dir + os.pathsep + os.environ["PATH"]
        break
# Couche systeme du runtime (nitro-pc/os.h) : les sources propres a chaque plateforme, les autres sont communes
OS_SOURCES = {"win32": ["os_win32.c"], "darwin": ["os_posix.c", "audio_coreaudio.c"]}
PLATFORM_SOURCES = {name for names in OS_SOURCES.values() for name in names}


def runtime_sources():
    own = OS_SOURCES.get(sys.platform)
    if not own:
        sys.exit("pas de couche systeme nitro-pc pour la plateforme %s" % sys.platform)
    return sorted(p for p in RUNTIME.glob("*.c") if p.name not in PLATFORM_SOURCES or p.name in own)


IDLE = 0x4000 | 0x08000000  # priorite basse, le PC reste utilisable ; pas de console quand le launcher nous lance

GAME_FLAGS = ["-m32", "-std=gnu89", "-O1", "-w", "-fwrapv", "-fno-strict-aliasing", "-fno-delete-null-pointer-checks",
              "-fno-common", "-fno-builtin", "-fno-stack-protector", "-mno-stack-arg-probe", "-mno-ms-bitfields",
              "-msse2", "-mfpmath=sse", "-mno-fp-ret-in-387", "-fno-omit-frame-pointer", "-fno-ident", "-g1"]
RUNTIME_FLAGS = ["-m32", "-std=gnu11", "-O2", "-Wall", "-Wno-unused", "-fno-builtin", "-fno-stack-protector",
                 "-mno-stack-arg-probe", "-msse2", "-mfpmath=sse", "-mno-fp-ret-in-387", "-fno-strict-aliasing",
                 "-fno-ident", "-g1"]
KR_DECL = re.compile(r"^[A-Za-z_][\w \t*]*\b\w+\s*\(\s*\)\s*;", re.M)
LIFT_FLAGS = ["-m32", "-std=gnu89", "-O1", "-w", "-fwrapv", "-fno-strict-aliasing", "-fno-builtin",
              "-fno-stack-protector", "-mno-stack-arg-probe", "-msse2", "-g1"]
SYM = re.compile(r"^(\S+) kind:(\w+)(?:\(([^)]*)\))? addr:0x([0-9a-f]+)")
CC = ["gcc", "-m32"]  # tables.c, missing.c
HOST_PREFIX = "ds_"  # fonctions du jeu hors Windows 32 bits, GAME_FN de nitro.h
MISSING = re.compile(r"undefined reference to `([^']+)'")  # GNU ld
EXE = ".exe"
UNDERSCORE = 1  # les symboles C commencent par _ (Windows 32 bits, Mach-O), pas en Windows 64 bits
IDENTITY = WIN32  # Windows 32 bits : adresse DS = adresse de l'hote ; --arch x86_64 : fenetre, comme sur Mac

if DARWIN:
    # clang d'Apple (Xcode Command Line Tools) ; -arch choisi dans main(). -ffp-contract=off : sans lui clang fusionne
    # a*b+c en FMA sur arm64, arrondi autrement que sur la DS. -std fige : CALLF passe 14 u32 par u64 (*)(), valable
    # avec l'ABI arm64 d'Apple tant que les fonctions ne sont pas variadiques et que le dialecte reste avant C23.
    LIFT_FLAGS = ["-std=gnu89", "-O1", "-w", "-fwrapv", "-fno-strict-aliasing", "-ffp-contract=off",
                  "-fno-stack-protector", "-fno-omit-frame-pointer"]
    GAME_FLAGS = list(LIFT_FLAGS)  # seulement avec --with-c, refuse sur cette plateforme
    # les -Werror montrent toute supposition de pointeur 32 bits qui resterait dans le runtime
    RUNTIME_FLAGS = ["-std=gnu11", "-O2", "-Wall", "-Wno-unused", "-fno-strict-aliasing", "-ffp-contract=off",
                     "-fno-stack-protector", "-Werror=pointer-to-int-cast", "-Werror=int-to-pointer-cast"]
    MISSING = re.compile(r'^\s*"_([^"]+)", referenced from:', re.M)  # ld64
    EXE = ""


def run(cmd, **kw):
    if WIN32:
        kw["creationflags"] = IDLE
    return subprocess.run([str(c) for c in cmd], capture_output=True, text=True, errors="replace", **kw)


def low_priority():
    """Le Mac reste utilisable pendant la compilation : nice, et qualite de service utility pour tous les fils."""
    if DARWIN:
        os.nice(10)
        # -p n'accepte pas -c : on relance le compilateur sous taskpolicy (P-cores encore permis, contrairement a -b)
        if subprocess.run(["taskpolicy", "-c", "utility", "true"], capture_output=True).returncode == 0:
            CC[:0] = ["taskpolicy", "-c", "utility"]


class Game:
    def __init__(self, root, version):
        self.root = Path(root)
        self.version = version
        cfg = self.root / "config" / version / "arm9"
        self.functions = {}  # addr -> (nom, taille, thumb)
        self.data = {}  # nom -> addr
        self.relocs = {}
        self.units = []  # (fichier source, [(start, end)])
        self.overlays = []  # (numero, debut du code, fin du code)
        # "overlays": "multi" (patches.json) : chaque overlay garde ses fonctions et ses relocs, meme a la meme adresse qu'un autre
        self.main_addrs = set()
        self.ov_functions = {}  # numero -> {adresse: (nom, taille, thumb)}
        self.ov_relocs = {}  # numero -> {depuis: (sorte, vers)}
        modules = [cfg] + [p for p in (cfg / "itcm", cfg / "dtcm") if p.exists()]
        modules += sorted(p for p in (cfg / "overlays").glob("*") if p.is_dir()) if (cfg / "overlays").exists() else []
        for m in modules:
            ov = int(m.name[2:]) if m.parent.name == "overlays" else None
            for line in (m / "symbols.txt").read_text(errors="replace").splitlines():
                s = SYM.match(line)
                if not s:
                    continue
                name, kind, detail, addr = s.group(1), s.group(2), s.group(3) or "", int(s.group(4), 16)
                if kind == "function":
                    size = re.search(r"size=0x([0-9a-f]+)", detail)
                    entry = (name, int(size.group(1), 16) if size else 0, "thumb" in detail)
                    if addr not in self.functions:
                        self.functions[addr] = entry
                    if ov is None:
                        self.main_addrs.add(addr)
                    else:
                        self.ov_functions.setdefault(ov, {})[addr] = entry
                elif kind in ("data", "bss"):
                    self.data.setdefault(name, addr)
            for line in (m / "relocs.txt").read_text(errors="replace").splitlines():
                r = re.match(r"from:0x([0-9a-f]+) kind:(\w+) to:0x([0-9a-f]+)", line)
                if r:
                    self.relocs[int(r.group(1), 16)] = (r.group(2), int(r.group(3), 16))
                    if ov is not None:
                        self.ov_relocs.setdefault(ov, {})[int(r.group(1), 16)] = (r.group(2), int(r.group(3), 16))
            self.read_delinks(m / "delinks.txt")
            if m.parent.name == "overlays":
                self.overlays += [(int(m.name[2:]), s, e) for s, e in overlay_code(m / "delinks.txt")]
            lcf = m / "lcf_symbols.txt"
            if lcf.exists():
                for name, value in re.findall(r"^(\w+) = (0x[0-9a-fA-F]+);", lcf.read_text(), re.M):
                    self.data[name] = int(value, 16)
        self.image = {}
        ext = self.root / "extract" / version / "arm9"
        for name in ("arm9", "itcm", "dtcm"):
            meta = (ext / (name + ".yaml")).read_text()
            base = int(re.search(r"base_address: (\d+)", meta).group(1))
            self.image[name] = (base, (ext / (name + ".bin")).read_bytes())
        meta = (ext / "arm9.yaml").read_text()
        self.entry = int(re.search(r"entry_function: (\d+)", meta).group(1))
        self.build_info = int(re.search(r"build_info: (\d+)", meta).group(1))
        table = self.root / "extract" / version / "arm9_overlays"
        self.overlay_images = {}
        if self.overlays:
            for num, base in re.findall(r"- id: (\d+)\s+base_address: (\d+)", (table / "overlays.yaml").read_text()):
                self.overlay_images[int(num)] = (int(base), (table / ("ov%03d.bin" % int(num))).read_bytes())

    def read_delinks(self, path):
        current = None
        for line in path.read_text(errors="replace").splitlines():
            if line.endswith(":") and not line.startswith(" "):
                current = [line[:-1], []]
                self.units.append(current)
            elif current and re.match(r"\s+\.(text|init|itcm)\b", line):
                m = re.search(r"start:0x([0-9a-f]+) end:0x([0-9a-f]+)", line)
                if m:
                    current[1].append((int(m.group(1), 16), int(m.group(2), 16)))
            elif not current:
                m = re.match(r"\s+\.ctor\s+start:0x([0-9a-f]+) end:0x([0-9a-f]+)", line)
                if m:
                    self.data["ARM9_CTOR_START"] = int(m.group(1), 16)
                    self.data["ARM9_CTOR_END"] = int(m.group(2), 16)

    def memory(self, addr, n):
        return recipe.read_memory(self, addr, n)


def overlay_code(path):
    """Plages de code d'un overlay, d'apres l'en-tete de son delinks.txt."""
    ranges = []
    for line in path.read_text(errors="replace").splitlines():
        if line.endswith(":") and not line.startswith(" "):
            break
        m = re.match(r"\s+\.\w+\s+start:0x([0-9a-f]+) end:0x([0-9a-f]+) kind:code", line)
        if m and m.group(1) != m.group(2):
            ranges.append((int(m.group(1), 16), int(m.group(2), 16)))
    return ranges


def game_patches(code):
    """Ce que le depot sait d'un jeu en plus de sa recette : noms de fonctions du SDK, accroches du runtime, adresses."""
    path = HERE.parent / "games" / "patches.json"
    return json.loads(path.read_text(encoding="utf8")).get(code, {}) if path.exists() else {}


# Toujours definies : les autres jeux gardent 0, et le runtime sait que la fonction n'existe pas pour eux
PATCH_VALUES = ["nds_net_screens", "nds_net_menu_screen", "nds_old_sdk", "nds_stack_margin", "nds_stack_reuse", "nds_long_args"]


def has_asm(text):
    return re.search(r"^\s*(asm|__asm)\b|\basm\s+\w", text, re.M) is not None


def obj_for(out, src):
    return out / "obj" / (hashlib.sha1(str(src).encode()).hexdigest()[:10] + "_" + src.stem + ".o")


def compile_one(args):
    src, obj, flags = args
    if obj.exists() and obj.stat().st_mtime >= src.stat().st_mtime:
        return src, True, ""
    p = run([*CC, *flags, "-c", src, "-o", obj])
    return src, p.returncode == 0, p.stderr


def nm_defined(objs, jobs):
    def one(o):
        res = run(["nm", "--defined-only", "-g", o]).stdout
        return o, [(l.split()[1], l.split()[2]) for l in res.splitlines() if len(l.split()) == 3]
    with ThreadPoolExecutor(jobs) as e:
        return dict(e.map(one, objs))


# ------------------------------------------------------------------ overlays qui se chevauchent ("overlays": "multi")
# Chaque overlay est traduit avec ses propres noms (ovNNN_...). Une adresse ou plusieurs overlays ont une fonction recoit un
# aiguillage (ovsel_<adresse>) : il regarde quels octets sont charges a cette adresse et appelle la traduction de cet overlay.

def overlay_plan(game):
    covers = {}
    for num, start, end in game.overlays:
        covers.setdefault(num, []).append((start, end))
    owners = {}
    for num in sorted(game.ov_functions):
        if num not in game.overlay_images:
            continue
        for addr in game.ov_functions[num]:
            owners.setdefault(addr, []).append(num)
    ov_name = {(num, addr): "ov%03d_%s" % (num, f[0]) for num, funcs in game.ov_functions.items() for addr, f in funcs.items()}
    glob = {addr: ov_name[(nums[0], addr)] if len(nums) == 1 else "ovsel_%08x" % addr for addr, nums in owners.items()}
    return covers, owners, ov_name, glob


def main_memory(game):
    def mem(addr, n):
        for base, raw in game.image.values():
            if base <= addr < base + len(raw):
                return raw[addr - base : addr - base + n]
        return None
    return mem


def overlay_memory(num, base, raw, main_mem):
    def mem(addr, n):
        if base <= addr < base + len(raw):
            return raw[addr - base : addr - base + n]
        return main_mem(addr, n)
    return mem


def lift_multi(lifter, game, names, to_lift, plan):
    """Traduit le module principal puis chaque overlay, chacun avec ses noms, sa memoire et ses sous-routines."""
    covers, owners, ov_name, glob = plan
    main_mem = main_memory(game)
    main_syms = dict(lifter.symbols)
    for addr, g in glob.items():
        main_syms.setdefault(addr, g)
    ctxs = {None: (main_syms, main_mem, dict(lifter.sizes), {}, "",
                   {frm for frm, (kind, to) in game.relocs.items() if kind == "load" and (to & ~1) in main_syms})}
    for num, funcs in sorted(game.ov_functions.items()):
        if num not in game.overlay_images:
            continue
        ranges = covers.get(num, [])
        syms = {a: n for a, n in main_syms.items() if not any(s <= a < e for s, e in ranges)}
        syms.update({a: ov_name[(num, a)] for a in funcs})
        cp = {frm for frm, (kind, to) in game.ov_relocs.get(num, {}).items() if kind == "load" and (to & ~1) in syms}
        base, raw = game.overlay_images[num]
        ctxs[num] = (syms, overlay_memory(num, base, raw, main_mem), {a: f[1] for a, f in funcs.items()}, {}, "ov%03d_" % num, cp)

    def enter(ctx):
        lifter.symbols, lifter.memory, lifter.sizes, lifter.new_subs, lifter.sub_prefix, lifter.code_ptr = ctxs[ctx]

    for _ in range(4):
        before = len(lifter.flag_funcs) + len(lifter.gosub_funcs)
        lifter.unhandled = []
        done = set()
        queue = [(None, f, names[f], game.functions[f][2]) for f in to_lift]
        queue += [(num, a, ov_name[(num, a)], f[2]) for num, funcs in sorted(game.ov_functions.items()) if num in ctxs for a, f in funcs.items()]
        bodies = []
        cur = -1
        while queue:
            ctx, addr, name, thumb = queue.pop()
            if (ctx, addr) in done:
                continue
            done.add((ctx, addr))
            if ctx != cur:
                enter(ctx)
                cur = ctx
            bodies.append(lifter.lift(name, addr, thumb))
            for sub_addr, sub_name in list(lifter.new_subs.items()):
                if (ctx, sub_addr) not in done:
                    queue.append((ctx, sub_addr, sub_name, thumb))
        # un appelant d'aiguillage qui lit les drapeaux : chaque candidat doit les rendre
        for addr, nums in owners.items():
            if len(nums) > 1 and glob[addr] in lifter.flag_funcs:
                lifter.flag_funcs.update(ov_name[(n, addr)] for n in nums)
        if len(lifter.flag_funcs) + len(lifter.gosub_funcs) == before:
            break
    enter(None)
    return bodies, done


OV_PICK = """
/* aiguillage des overlays (native_build.ovsel_code) : k = {n, debut de chaque candidat, puis par candidat : m, (decalage, mot) * m} */
static int ov_match_(u32 a, const u32 *p) { u32 m = p[0], j; for (j = 0; j < m; j++) if (*(volatile u32 *)DS(a + p[1 + 2 * j]) != p[2 + 2 * j]) return 0; return 1; }
static void *ov_pick_(u32 a, const u32 *k, void *const *f, u32 *last) {
    u32 n = k[0], i = *last;
    if (ov_match_(a, k + k[1 + i])) return f[i];
    for (i = 0; i < n; i++) if (ov_match_(a, k + k[1 + i])) { *last = i; return f[i]; }
    nds_unhandled("aucun overlay charge ici", a);
    return f[0];
}
"""


def ovsel_code(game, plan):
    covers, owners, ov_name, glob = plan
    args = ", ".join(["u32 r0", "u32 r1", "u32 r2", "u32 r3"] + ["u32 a%d" % i for i in range(arm2c.STACK_ARGS)])
    fwd = ", ".join(["r0", "r1", "r2", "r3"] + ["a%d" % i for i in range(arm2c.STACK_ARGS)])
    out = [OV_PICK]
    for addr in sorted(a for a, nums in owners.items() if len(nums) > 1):
        nums = owners[addr]
        words = {}
        for num in nums:
            base, raw = game.overlay_images[num]
            size = max(game.ov_functions[num][addr][1], 4)
            b = raw[addr - base : addr - base + size]
            words[num] = [int.from_bytes(b[i : i + 4], "little") for i in range(0, len(b) - 3, 4)] or [0]
        idx, body = [], []
        for num in nums:
            offs = {0}
            for other in nums:
                if other != num:
                    for i, (x, y) in enumerate(zip(words[num], words[other])):
                        if x != y:
                            offs.add(i)
                            break
            idx.append(1 + len(nums) + len(body))
            body.append(len(offs))
            for i in sorted(offs):
                body += [i * 4, words[num][i]]
        k = [len(nums)] + idx + body
        out += ["extern u64 %s();" % ov_name[(n, addr)] for n in nums]
        out.append("static const u32 ovk_%08x[] = {%s};" % (addr, ",".join("0x%xu" % v for v in k)))
        out.append("static void *const ovf_%08x[] = {%s};" % (addr, ", ".join("(void *)%s" % ov_name[(n, addr)] for n in nums)))
        out.append("u64 ovsel_%08x(%s) { static u32 last_; return ((u64 (*)())ov_pick_(0x%08xu, ovk_%08x, ovf_%08x, &last_))(%s); }" % (addr, args, addr, addr, addr, fwd))
    return "\n".join(out) + "\n"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("project", nargs="?", help="dossier du projet de jeu ; sans projet : --recipe et --rom")
    ap.add_argument("--recipe", help="recette du jeu (native/games/<code>.json.gz)")
    ap.add_argument("--rom", help="rom du joueur, avec --recipe")
    ap.add_argument("--version", default=None)
    ap.add_argument("--out", default=None)
    ap.add_argument("--lift", default="", help="fonctions a traduire depuis l'ARM meme si elles ont du C (pour trouver un bug)")
    ap.add_argument("-j", type=int, default=4)
    ap.add_argument("--with-c", action="store_true", help="compiler aussi le C decompile du jeu (registres par exceptions, plus lent)")
    ap.add_argument("--instrument", action="store_true", help="trace les entrees de fonctions (dernier appels au plantage)")
    ap.add_argument("--arch", default=None, help="macOS : arm64 ou x86_64 (par defaut celle de ce Mac) ; Windows : x86 (defaut) ou x86_64")
    ap.add_argument("--no-hooks", action="store_true", help="sans le test d'accroche a l'entree des fonctions traduites (mesures)")
    ap.add_argument("--insn-hooks", default="", help="instructions accrochables (\"instruction_hooks\" des mods actifs) : 0x...,0x...")
    a = ap.parse_args()
    if not WIN32 and a.with_c:
        # le C matche utilise de vrais pointeurs, 64 bits ici : structures et pointeurs en memoire DS casses (MACOS.md 3.5)
        sys.exit("--with-c : Windows 32 bits seulement")
    if DARWIN:
        arch = a.arch or platform.machine()
        if arch not in ("arm64", "x86_64"):
            sys.exit("--arch : arm64 ou x86_64, pas %s" % arch)
        CC[:] = ["clang", "-arch", arch]
        low_priority()
    elif not WIN32:
        sys.exit("pas de profil de compilation pour la plateforme %s" % sys.platform)
    elif a.arch == "x86_64":
        # Windows 64 bits : memes options, la memoire DS passe dans une fenetre (DS()) comme sur Mac
        global IDENTITY, UNDERSCORE
        IDENTITY = False
        UNDERSCORE = 0
        if a.with_c:
            sys.exit("--with-c : Windows 32 bits seulement")
        for flags in (GAME_FLAGS, RUNTIME_FLAGS, LIFT_FLAGS, CC):
            flags[:] = ["-m64" if f == "-m32" else f for f in flags]
        RUNTIME_FLAGS.extend(["-Werror=pointer-to-int-cast", "-Werror=int-to-pointer-cast"])
    elif a.arch not in (None, "x86"):
        sys.exit("--arch : x86 ou x86_64, pas %s" % a.arch)
    if a.recipe:
        game = recipe.RecipeGame(a.recipe, a.rom)
        root, code = Path("."), game.code
    else:
        root = Path(a.project).resolve()
        version = a.version or next(p.name for p in (root / "config").iterdir() if (p / "arm9").exists())
        code = re.search(r"gamecode: (\w+)", (root / "extract" / version / "header.yaml").read_text()).group(1)
    out = Path(a.out) if a.out else HERE.parent / "build" / (code + ("-c" if a.with_c else "") + ("-debug" if a.instrument else ""))
    if a.instrument:
        skip = "-finstrument-functions-exclude-function-list=LDRW,ROR,LSL,LSR,ASR,CLZ,SAT32"
        if not WIN32:  # clang n'a pas cette liste : les petites fonctions du PRELUDE sont inlinees avant l'instrumentation
            skip = "-finstrument-functions-after-inlining"
        GAME_FLAGS.extend(["-finstrument-functions", skip])
        LIFT_FLAGS.extend(["-finstrument-functions", skip, "-DNDS_INSTRUMENT"])
        RUNTIME_FLAGS.append("-DNDS_INSTRUMENT")
    (out / "obj").mkdir(parents=True, exist_ok=True)
    # les objets ne sont refaits que si leur source change : un autre compilateur ou une autre -arch les refait tous
    stamp = out / "obj" / "cc.txt"
    if not stamp.exists() or stamp.read_text() != " ".join(CC):
        for o in (out / "obj").glob("*.o"):
            o.unlink()
        stamp.write_text(" ".join(CC))
    (out / "lift").mkdir(exist_ok=True)
    print("jeu", code, "->", out, flush=True)
    if not a.recipe:
        game = Game(root, version)
    force = set(filter(None, a.lift.split(",")))
    patches = game_patches(code)
    if "stack_args" in patches:
        arm2c.set_stack_args(int(patches["stack_args"]))
    for addr, name in patches.get("names", {}).items():
        addr = int(addr, 16)
        game.functions[addr] = (name, *game.functions[addr][1:])
    game.data.update({name: int(addr, 16) for name, addr in patches.get("data", {}).items()})
    if not patches.get("overlays"):
        game.overlays = []
    multi = patches.get("overlays") == "multi"
    if multi:
        # overlays qui se chevauchent : le module principal ne garde que ses fonctions, chaque overlay est traduit a part
        game.functions = {addr: game.functions[addr] for addr in game.main_addrs}
    # "shared_regs" : les cibles des tables de sauts de l'assembleur ecrit a la main deviennent des points d'entree
    for lo, hi in patches.get("shared_regs", []):
        lo, hi = int(lo, 16), int(hi, 16)
        for at in range(lo, hi, 4):
            raw = game.memory(at, 4)
            v = int.from_bytes(raw, "little") if raw and len(raw) == 4 else 0
            if lo <= v < hi and not v & 3 and v not in game.functions:
                game.functions[v] = ("func_%08x" % v, 0, False)

    # 1. runtime : ce qu'il definit remplace le jeu
    rt_srcs = runtime_sources()
    rt_flags = RUNTIME_FLAGS + ["-I", RUNTIME]
    with ThreadPoolExecutor(a.j) as e:
        res = list(e.map(compile_one, [(s, obj_for(out, s), rt_flags) for s in rt_srcs]))
    fails = [r for r in res if not r[1]]
    for r in fails:
        print(r[2][:3000])
    if fails:
        sys.exit("runtime : erreur de compilation")
    rt_objs = [obj_for(out, s) for s in rt_srcs]
    rt_syms = nm_defined(rt_objs, a.j)
    # la couche systeme (os_*.c) ne remplace rien du jeu : son main (macOS) n'est pas le main du jeu, que crt0 appelle
    os_objs = {obj_for(out, s) for s in rt_srcs if s.name in PLATFORM_SOURCES}
    rt_defs = {s[UNDERSCORE:] for o, syms in rt_syms.items() if o not in os_objs for t, s in syms if t in "TDBR"}

    # 2. C du jeu
    by_addr = sorted(game.functions)
    names = {addr: game.functions[addr][0] for addr in by_addr}
    if not IDENTITY:
        # nom de l'hote : le jeu a ses memcpy, exit, sprintf... qui prennent des adresses DS ; sous leur vrai nom ils
        # remplaceraient ceux de libSystem dans le runtime et dans les appels que clang ajoute. GAME_FN de nitro.h.
        # Ce que le runtime remplace garde son nom.
        names = {addr: n if n in rt_defs else HOST_PREFIX + n for addr, n in names.items()}
    srcs = []
    file_funcs = {}
    kr_files = 0
    for unit, ranges in game.units:
        path = root / unit
        if not unit.startswith("src/") or unit.startswith("src/data/") or not path.exists():
            continue
        funcs = [f for f in by_addr if any(s <= f < e for s, e in ranges)]
        file_funcs[path] = funcs
        text = path.read_text(errors="replace")
        if not a.with_c or has_asm(text):
            continue
        # declaration sans prototype : le C matche compte sur les registres r0-r3 deja en place (f() sans arguments),
        # ce que le x86 ne garde pas ; la traduction de l'ARM reste exacte
        if KR_DECL.search(text):
            kr_files += 1
            continue
        if any(names[f] in rt_defs or names[f] in force for f in funcs):
            continue
        srcs.append(path)
    game_flags = GAME_FLAGS + ["-I", root / "include", "-I", root / "include" / "nitro"]
    with ThreadPoolExecutor(a.j) as e:
        res = list(e.map(compile_one, [(s, obj_for(out, s), game_flags) for s in srcs]))
    good = [r[0] for r in res if r[1]]
    bad = [r for r in res if not r[1]]
    # un fichier qui garde ses propres variables : les vraies sont dans la rom, aux adresses DS, et d'autres fonctions
    # ou l'ARM7 peuvent y acceder par leur adresse. On le traduit pour qu'il utilise celles de la rom.
    def own_data(src):
        res = run(["nm", "--defined-only", obj_for(out, src)]).stdout
        for line in res.splitlines():
            f = line.split()
            if len(f) == 3 and f[1] in "dDbBC" and not f[2].startswith("."):
                return True
        return False
    with ThreadPoolExecutor(a.j) as e:
        flags = list(e.map(own_data, good))
    data_files = sum(flags)
    good = [g for g, o in zip(good, flags) if not o]
    print("C compile :", len(good), " refuse :", len(bad), " sans prototypes (traduits) :", kr_files, " avec variables (traduits) :", data_files)

    # 3. fonctions a traduire : tout ce que le C ne couvre pas
    covered = {f for s in good for f in file_funcs[s]}
    to_lift = [f for f in by_addr if f not in covered and names[f] not in rt_defs]
    used = {}
    symbols = {}
    for f in by_addr:
        n = names[f]
        symbols[f] = n
    code_ptr = {frm for frm, (kind, to) in game.relocs.items() if kind == "load" and (to & ~1) in symbols}
    lifter = arm2c.Lifter(game.memory, symbols, None, code_ptr)
    lifter.debug_pc = a.instrument
    lifter.hook_all = not a.no_hooks
    lifter.insn_hooks = {addr: k for k, addr in enumerate(sorted({int(x, 16) & ~1 for x in a.insn_hooks.split(",") if x.strip()}))}
    lifter.sizes = {addr: game.functions[addr][1] for addr in by_addr}
    lifter.hooks = {int(addr, 16): fn for addr, fn in patches.get("hooks", {}).items()}
    lifter.exit_hooks = {int(addr, 16): fn for addr, fn in patches.get("exit_hooks", {}).items()}
    lifter.gates = {int(addr, 16): fn for addr, fn in patches.get("gates", {}).items()}
    if "eh_end_catch" in patches:
        lifter.eh_end_catch = int(patches["eh_end_catch"], 16)
    # assembleur ecrit a la main qui se passe des registres d'une fonction a l'autre : [[debut, fin], ...]
    lifter.shared_ranges = [(int(lo, 16), int(hi, 16)) for lo, hi in patches.get("shared_regs", [])]
    lifter.arm_stack = bool(patches.get("arm_stack")) and lifter.eh_end_catch is None
    plan = overlay_plan(game) if multi else None
    protos = set()
    if multi:
        bodies, done = lift_multi(lifter, game, names, to_lift, plan)
    else:
      # plusieurs passes : les fonctions qui renvoient des drapeaux se decouvrent chez leurs appelants
      for _ in range(4):
        before = len(lifter.flag_funcs) + len(lifter.gosub_funcs)
        lifter.unhandled = []
        done = set()
        queue = [(f, names[f], game.functions[f][2]) for f in to_lift]
        bodies = []
        while queue:
            addr, name, thumb = queue.pop()
            if addr in done:
                continue
            done.add(addr)
            bodies.append(lifter.lift(name, addr, thumb))
            for sub_addr, sub_name in list(lifter.new_subs.items()):
                if sub_addr not in done:
                    queue.append((sub_addr, sub_name, thumb))
        if len(lifter.flag_funcs) + len(lifter.gosub_funcs) == before:
            break
    print("fonctions a retour par drapeaux :", " ".join(sorted(lifter.flag_funcs))[:600])
    # les pointeurs de code sont des adresses DS (nds_call_target) : seuls les appels directs nomment une fonction
    for n in set(re.findall(r"CALL[FX]\((\w+)\)", "\n".join(bodies))):
        if not n.startswith("nds_"):
            protos.add(n)
    lift_c = ("#define NDS_EH 1\n" if lifter.eh_end_catch is not None else "") + arm2c.PRELUDE + "\n".join("extern u64 %s();" % p for p in sorted(protos)) + "\n\n" + "\n".join(bodies) + "\n" + lifter.hook_table()
    if lifter.arm_stack:
        lift_c = lift_c.replace(arm2c.PRELUDE, arm2c.PRELUDE + arm2c.ARM_STACK_PRELUDE, 1)
    if lifter.shared_ranges:
        lift_c = lift_c.replace(arm2c.PRELUDE, arm2c.PRELUDE + arm2c.xr_shared_macro(lifter.shared_ranges) + arm2c.XR_PRELUDE, 1)
    if multi:
        lift_c += ovsel_code(game, plan)
    parts = int(patches.get("lift_parts", 1))
    if parts > 1:
        # "lift_parts" (patches.json) : un gros jeu en plusieurs fichiers, compiles en parallele avec bien moins de memoire
        # (Heroes of Mana : 27 Mo de C, plus de 15 Go pour gcc 64 bits d'un seul bloc). Ce qui est partage entre les
        # fonctions (drapeaux F_*, nds_xr) est defini dans le premier morceau et declare extern dans les autres.
        head_len = len(lift_c) - len("\n".join(bodies) + "\n" + lifter.hook_table()) - (len(ovsel_code(game, plan)) if multi else 0)
        head, tail = lift_c[:head_len], lift_c[head_len + len("\n".join(bodies)):]
        flags_def = "static u32 F_N, F_Z, F_C, F_V;"
        size = sum(len(b) for b in bodies) // parts + 1
        chunks, cur, cur_len = [], [], 0
        for b in bodies:
            cur.append(b)
            cur_len += len(b)
            if cur_len >= size and len(chunks) < parts - 1:
                chunks.append(cur)
                cur, cur_len = [], 0
        chunks.append(cur)
        texts = []
        for k, chunk in enumerate(chunks):
            h = head.replace(flags_def, "u32 F_N, F_Z, F_C, F_V;" if k == 0 else "extern u32 F_N, F_Z, F_C, F_V;", 1)
            if k:
                h = h.replace("u32 nds_xr[14], nds_xr_in;", "extern u32 nds_xr[14], nds_xr_in;", 1)
            texts.append(h + "\n".join(chunk) + (tail if k == len(chunks) - 1 else "\n"))
        lift_paths = [out / "lift" / ("lifted_%d.c" % k) for k in range(len(texts))]
    else:
        texts, lift_paths = [lift_c], [out / "lift" / "lifted.c"]
    for path, text in zip(lift_paths, texts):
        if not path.exists() or path.read_text() != text:
            path.write_text(text)
    print("fonctions traduites depuis l'ARM :", len(done), " instructions non gerees :", len(lifter.unhandled))
    (out / "unhandled.txt").write_text("\n".join(lifter.unhandled))
    # un seul gros fichier serait lent et fragile : gcc -O1 tient bien 2 Mo de C simple
    lobjs = [out / "obj" / (path.stem + ".o") for path in lift_paths]
    todo = [(path, obj) for path, obj in zip(lift_paths, lobjs) if not obj.exists() or obj.stat().st_mtime < path.stat().st_mtime]
    # morceaux ("lift_parts") : un par coeur, le launcher ne passe pas -j
    jobs = max(a.j, min(os.cpu_count() or 4, len(todo))) if len(lift_paths) > 1 else a.j
    with ThreadPoolExecutor(max(1, min(jobs, len(todo) or 1))) as e:
        results = list(e.map(lambda po: (po[0], run([*CC, *LIFT_FLAGS, "-c", po[0], "-o", po[1]])), todo))
    for path, p in results:
        if p.returncode:
            print(p.stderr[:5000])
            sys.exit("%s : erreur" % path.name)

    # 4. tables : adresse ARM -> fonction de l'hote (nds_call_target), entree, build_info.
    # La memoire DS garde les pointeurs de fonctions du jeu tels quels (adresses ARM), rien n'y est corrige.
    game_objs = [obj_for(out, s) for s in good]
    defs = nm_defined(game_objs + lobjs, a.j)
    defined = {s[UNDERSCORE:]: t for syms in defs.values() for t, s in syms}
    defined.update({s: "T" for s in rt_defs})
    fn_names = {n for n in names.values() if n in defined}
    ov_entries = list(plan[3].items()) if multi else []  # adresse d'overlay -> sa fonction, ou son aiguillage (ovsel_)
    table = sorted((addr, n) for addr, n in list(names.items()) + ov_entries + [(k, v) for k, v in lifter.new_subs.items()] if n in defined or n in lifter.new_subs.values())
    gen = ["typedef unsigned int u32;"]
    for _, n in table:
        gen.append("extern char %s[];" % n)
    gen.append("const struct { u32 arm; void *x86; } nds_fn_table[] = {")
    gen += ["  {0x%08x, %s}," % (addr, n) for addr, n in table]
    gen.append("}; const unsigned nds_fn_count = %d;" % len(table))
    gen.append("extern char %s[]; void *const nds_entry_fn = %s;" % (names[game.entry], names[game.entry]))
    gen.append("const u32 nds_build_info_offset = 0x%x;" % game.build_info)
    gen.append("const int nds_has_c = %d;" % (1 if good else 0))
    gen += ["const u32 %s = %s;" % (name, patches.get("values", {}).get(name, "0")) for name in PATCH_VALUES]
    # des fonctions du jeu que le runtime appelle : leur vrai nom seulement pour le jeu qui les decrit
    open_screen = patches.get("functions", {}).get("nds_open_screen")
    if open_screen:
        open_screen = open_screen if IDENTITY else HOST_PREFIX + open_screen
        gen.append("extern char %s[]; void *const nds_open_screen = %s;" % (open_screen, open_screen))
    else:
        gen.append("void *const nds_open_screen = 0;")
    # api->runtime(nom) des mods : fonctions et variables globales du runtime, sans promesse de stabilite
    rt_names = sorted({s[UNDERSCORE:] for syms in rt_syms.values() for t, s in syms if t in "TDBR"} - {"main"})
    rt_names = [n for n in rt_names if re.fullmatch(r"[A-Za-z_]\w*", n)]
    gen += ["extern char %s[];" % n for n in rt_names]
    gen.append("const struct { const char *name; void *p; } nds_runtime_syms[] = {")
    gen += ['  {"%s", %s},' % (n, n) for n in rt_names]
    gen.append("}; const unsigned nds_runtime_sym_count = %d;" % len(rt_names))
    # version du runtime pour le multi (net.c compare les deux joueurs) : empreinte des sources du runtime et du traducteur
    version = hashlib.sha1()
    for src in sorted(RUNTIME.glob("*.[ch]")) + [HERE / "arm2c.py", HERE / "native_build.py"]:
        version.update(src.name.encode() + src.read_bytes().replace(b"\r\n", b"\n"))
    gen.append('const char nds_runtime_version[] = "%s";' % version.hexdigest()[:12])
    gen_path = out / "lift" / "tables.c"
    gen_path.write_text("\n".join(gen) + "\n")
    gobj = out / "obj" / "tables.o"
    p = run([*CC, "-c", "-w", "-std=gnu89", gen_path, "-o", gobj])
    if p.returncode:
        print(p.stderr[:3000])
        sys.exit("tables.c : erreur")

    # 5. donnees : symboles absolus ; une copie definie par le C devient faible
    data_defs = {}
    for o, syms in defs.items():
        for t, s in syms:
            if t in "DBRC" and s[UNDERSCORE:] in game.data:
                data_defs.setdefault(o, []).append(s)
    if data_defs and not IDENTITY:
        sys.exit("donnees definies hors du runtime : " + " ".join(s for v in data_defs.values() for s in v)[:600])
    for o, syms in data_defs.items():
        run(["objcopy", *sum((["-W", s] for s in syms), []), o])
    print("donnees definies par le C et rendues faibles :", sum(len(v) for v in data_defs.values()))
    data = [(n, addr) for n, addr in sorted(game.data.items()) if n not in rt_defs]
    if IDENTITY:
        abs_lines = []
        for n, addr in data:
            abs_lines += [".globl _%s" % n, ".set _%s, 0x%08x" % (n, addr)]
        abs_path = out / "lift" / "dsabs.s"
        abs_path.write_text("\n".join(abs_lines) + "\n")
        aobj = out / "obj" / "dsabs.o"
        p = run(["as", "--32", abs_path, "-o", aobj])
    else:
        # ld64 n'a pas de symboles absolus utilisables : le code traduit a les adresses en nombres, et le runtime lit
        # celles qu'il nomme (GAME_SYM/GAME_ADDR de nitro.h) dans nds_addr_<nom>
        abs_lines = ["const unsigned nds_addr_%s = 0x%08xu;" % (n, addr) for n, addr in data if re.fullmatch(r"[A-Za-z_]\w*", n)]
        abs_path = out / "lift" / "dsaddr.c"
        abs_path.write_text("\n".join(abs_lines) + "\n")
        aobj = out / "obj" / "dsaddr.o"
        p = run([*CC, "-c", "-w", "-std=gnu89", abs_path, "-o", aobj])
    if p.returncode:
        print(p.stderr[:3000])
        sys.exit("adresses des donnees : erreur")

    # 6. edition de liens, avec bouchons pour les symboles introuvables
    if WIN32:
        libs = []
        for dll in ("kernel32", "winmm", "ws2_32"):
            lib = out / ("lib%s.a" % dll)
            if IDENTITY:
                run(["dlltool", "-m", "i386", "--as-flags=--32", "-k", "-d", RUNTIME / (dll + ".def"), "-l", lib])
            else:
                # x64 : pas de decoration @n des noms
                plain = out / (dll + ".def")
                plain.write_text(re.sub(r"@\d+$", "", (RUNTIME / (dll + ".def")).read_text(), flags=re.M))
                run(["dlltool", "-m", "i386:x86-64", "-d", plain, "-l", lib])
            libs.append(lib)
        k32, w32, ws2 = libs
    exe = out / (code + EXE)
    objs = rt_objs + game_objs + lobjs + [gobj, aobj]
    rsp = out / "link.rsp"
    def link(extra):
        rsp.write_text("\n".join('"%s"' % str(o).replace("\\", "/") for o in objs + extra))
        if WIN32 and not IDENTITY:
            return run(["ld", "-m", "i386pep", "-e", "nds_start", "--stack", "0x40000",
                        "--subsystem", "console", "--disable-dynamicbase", "--disable-reloc-section",
                        "@" + str(rsp), k32, w32, ws2, "-o", exe])
        if WIN32:
            return run(["ld", "-m", "i386pe", "-e", "_nds_start", "--stack", "0x40000","--image-base", "0x10000000",
                        "--subsystem", "console", "--disable-dynamicbase", "--disable-reloc-section",
                        "@" + str(rsp), k32, w32, ws2, "-o", exe])
        # clang + ld64 : libSystem, AudioToolbox pour le son ; dyld entre dans main (os_posix.c), ld64 signe ad hoc la sortie arm64
        return run([*CC, "@" + str(rsp), "-framework", "AudioToolbox", "-o", exe])
    p = link([])
    missing = sorted(set(MISSING.findall(p.stderr)))
    if missing:
        stub = out / "lift" / "missing.c"
        lines = ["void nds_missing(const char *);"]
        for m in missing:
            if m.endswith(("@4", "@8", "@12", "@16", "@20", "@24", "@28")):
                continue
            lines.append('unsigned long long %s(void) { nds_missing("%s"); return 0; }' % (m, m))
        stub.write_text("\n".join(lines) + "\n")
        sobj = out / "obj" / "missing.o"
        run([*CC, "-w", "-std=gnu89", "-O1", "-fno-builtin", "-c", stub, "-o", sobj])
        print("symboles sans definition (bouchons) :", len(missing), " ".join(missing[:60]))
        p = link([sobj])
    if p.returncode:
        print(p.stderr[:6000])
        sys.exit("edition de liens : erreur")
    print("OK", exe, exe.stat().st_size // 1024, "Ko")


if __name__ == "__main__":
    main()
