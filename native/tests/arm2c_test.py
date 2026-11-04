#!/usr/bin/env python3
"""Test sans rom du traducteur (tools/arm2c.py) et des appels du runtime (nitro-pc/calltab.c), native/MACOS.md 4.1.

  python3 native/tests/arm2c_test.py           # traduit arm2c_cases.s, compile le harnais, le lance
  python3 native/tests/arm2c_test.py --regen   # apres un changement de arm2c_cases.s (clang avec la cible ARM)
  python3 native/tests/arm2c_test.py --arch x86_64   # macOS : version Intel, lancee par Rosetta sur Apple Silicon

Couvre : RD/WR et la rotation de LDRW, les macros de drapeaux, CALLF a 10 arguments (fonction traduite et fonction C) et 14 (t_args14),
les accroches des mods (HOOK_ENTRY du PRELUDE, nds_hook_install de nitro-pc/calltab.c, chaine de deux accroches, NDS_INSN),
les sous-routines internes (GOSUB_TAG, gs_), nds_call_target sur des adresses ARM et Thumb, et les fils verts de
nitro-pc/context.c (OS_SaveContext/OS_LoadContext, nds_run_on_stack : deux fils sur des piles de la fenetre se
passent la main un million de fois chacun).
Chaque cas est compile deux fois : normal et avec NDS_INSTRUMENT (HOST2DS verifie alors que la pile est dans la fenetre).

macOS, Linux : cc (ou $CC). Fenetre de 4 Go n'importe ou (nds_mem), le code traduit tourne sur un fil dont la pile
est dans la fenetre, comme les piles de stack_alloc.
Windows : gcc -m32 de WinLibs ou de la boite a outils (chemins et options de native_build.py), memoire DS aux adresses DS.
Il faut capstone : pip install capstone==5.0.7 (ou le python de la boite a outils, qui l'a deja).
"""
import argparse
import os
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
RUNTIME = HERE.parent / "nitro-pc"
sys.path.insert(0, str(HERE.parent / "tools"))

BASE = 0x02000000  # arm2c_cases.s y est charge
# fonctions sans code ARM ici (fonctions C du harnais, comme les remplacements du runtime) : adresse DS, nom, Thumb
HOST = [(0x02000F00, "h_pack10", True), (0x01FF8000, "h_itcm", False), (0x027E0100, "h_dtcm", False)]

# --- CASES : ecrit par --regen (nom, decalage depuis BASE, Thumb, octets) ---
CASES = [
    ("t_ldr", 0x000, False, "000090e51eff2fe1"),
    ("t_store", 0x020, False, "001080e5b610c0e10910c0e51eff2fe1"),
    ("t_cmpflags", 0x040, False, "0020a0e3010050e1082082430420820302208223012082630200a0e11eff2fe1"),
    ("t_addflags", 0x080, False, "0020a0e3013090e0082082430420820302208223012082630200a0e11eff2fe1"),
    ("t_lsrs", 0x0c0, False, "a010b0e10000a0e302008023040080031eff2fe1"),
    ("t_smax", 0x0e0, False, "010050e10100a0b11eff2fe1"),
    ("t_pack10", 0x100, False, "010280e1020480e1030680e100c09de50c0880e104c09de50c0a80e108c09de50c0c80e10cc09de50c0e80e110109de514c09de50c1281e11eff2fe1"),
    ("t_call10", 0x180, False, "10402de918d04de20540a0e300408de50640a0e304408de50740a0e308408de50840a0e30c408de50940a0e310408de50a40a0e314408de50100a0e30210a0e30320a0e30430a0e3ccffffeb18d08de21080bde8"),
    ("t_callptr", 0x200, False, "10402de918d04de20540a0e300408de50640a0e304408de50740a0e308408de50840a0e30c408de50940a0e310408de50a40a0e314408de50100a0e30210a0e30320a0e30430a0e308c09fe53cff2fe118d08de21080bde8010f0002"),
    ("t_callreg", 0x280, False, "10402de900c0a0e10100a0e13cff2fe11080bde8"),
    ("t_tail", 0x2c0, False, "00c0a0e10100a0e11cff2fe1"),
    ("t_gosub", 0x300, False, "10402de9020000eb030000eb040000eb1080bde88000a0e11eff2fe1010080e20ef0a0e104e02de5f9ffffeb090080e204f09de4"),
    ("t_thumb", 0x380, True, "00b500f003f800f001f800bd400003307047"),
    ("t_args14", 0x400, False, "24009de518109de51eff2fe1"),
]
# --- fin des CASES ---


def write(path, text):
    with open(path, "w", newline="\n") as f:  # fins de ligne Unix, aussi sous Windows
        f.write(text)


# ------------------------------------------------------------------ --regen : assembler arm2c_cases.s
def elf_sections(d):
    shoff, = struct.unpack_from("<I", d, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", d, 0x2E)
    hdrs = [struct.unpack_from("<10I", d, shoff + i * shentsize) for i in range(shnum)]
    names = hdrs[shstrndx]
    name = lambda off: d[names[4] + off:d.index(b"\0", names[4] + off)].decode()
    return {name(h[0]): h for h in hdrs}, hdrs


def regen():
    cc = os.environ.get("CC", "clang")
    with tempfile.TemporaryDirectory() as tmp:
        obj = Path(tmp) / "cases.o"
        p = subprocess.run([cc, "-target", "armv5te-none-eabi", "-c", str(HERE / "arm2c_cases.s"), "-o", str(obj)],
                           capture_output=True, text=True)
        if p.returncode:
            sys.exit(p.stderr or "assembleur ARM introuvable")
        d = obj.read_bytes()
    secs, hdrs = elf_sections(d)
    text_h = secs[".text"]
    text = bytearray(d[text_h[4]:text_h[4] + text_h[5]])
    symtab = secs[".symtab"]
    strtab = hdrs[symtab[6]]
    syms = []
    for i in range(symtab[5] // 16):
        st_name, value, size, info, _, shndx = struct.unpack_from("<IIIBBH", d, symtab[4] + i * 16)
        syms.append((d[strtab[4] + st_name:d.index(b"\0", strtab[4] + st_name)].decode(), value, size, info))
    # l'assembleur laisse les bl en relogements (R_ARM_CALL), meme vers un label local : on les resout ici
    rel = secs.get(".rel.text")
    for i in range(rel[5] // 8 if rel else 0):
        off, info = struct.unpack_from("<II", d, rel[4] + i * 8)
        kind, target = info & 0xFF, syms[info >> 8]
        if kind not in (28, 29) or target[1] & 1:
            sys.exit("relogement non gere : type %d vers %s" % (kind, target[0]))
        insn, = struct.unpack_from("<I", text, off)
        addend = ((insn & 0xFFFFFF) ^ 0x800000) - 0x800000 << 2
        insn = insn & 0xFF000000 | ((target[1] + addend - off) >> 2) & 0xFFFFFF
        struct.pack_into("<I", text, off, insn)
    funcs = sorted((v & ~1, n, bool(v & 1), s) for n, v, s, info in syms if info >> 4 == 1)  # globaux
    lines = ["CASES = ["]
    for off, name, thumb, size in funcs:
        lines.append('    ("%s", 0x%03x, %s, "%s"),' % (name, off, thumb, text[off:off + size].hex()))
    lines.append("]")
    me = Path(__file__)
    src = me.read_text()
    head, rest = src.split("# --- CASES", 1)
    _, tail = rest.split("# --- fin des CASES ---", 1)
    marker = rest.split("\n", 1)[0]
    write(me, head + "# --- CASES" + marker + "\n" + "\n".join(lines) + "\n# --- fin des CASES ---" + tail)
    print("CASES :", len(funcs), "fonctions")


# ------------------------------------------------------------------ traduction
def lift():
    import arm2c
    image = {BASE + off: bytes.fromhex(h) for _, off, _, h in CASES}

    def memory(addr, n):
        for start, raw in image.items():
            if start <= addr < start + len(raw):
                return raw[addr - start:addr - start + n]
        return None

    funcs = {BASE + off: (name, len(bytes.fromhex(h)), thumb) for name, off, thumb, h in CASES}
    symbols = {a: f[0] for a, f in funcs.items()}
    symbols.update({a: n for a, n, _ in HOST})
    targets = {a | t for a, (_, _, t) in funcs.items()} | {a | t for a, _, t in HOST}
    # pool litteral : un mot qui vaut l'adresse d'une fonction porte un pointeur de code (les relocs du vrai build)
    code_ptr = {a + i for a, raw in image.items() for i in range(0, len(raw) - 3, 4)
                if int.from_bytes(raw[i:i + 4], "little") in targets}
    lifter = arm2c.Lifter(memory, symbols, None, code_ptr)
    lifter.sizes = {a: f[1] for a, f in funcs.items()}
    lifter.hook_all = True  # comme native_build.py : chaque fonction passe par HOOK_ENTRY
    lifter.insn_hooks = {BASE + 0x0e8: 0}  # t_smax, avant bx lr : accroche a une instruction (NDS_INSN)
    for _ in range(4):  # comme native_build.py : drapeaux et sous-routines se decouvrent au fil des passes
        before = len(lifter.flag_funcs) + len(lifter.gosub_funcs)
        lifter.unhandled = []
        bodies = {f[0]: lifter.lift(f[0], a, f[2]) for a, f in sorted(funcs.items())}
        if len(lifter.flag_funcs) + len(lifter.gosub_funcs) == before:
            break
    if lifter.unhandled or lifter.new_subs:
        sys.exit("traduction incomplete : %s %s" % (lifter.unhandled, lifter.new_subs))
    bodies[None] = lifter.hook_table()
    return funcs, bodies


def check_text(bodies):
    """Ce que le traducteur doit ecrire (verifie sur le C, avant de le compiler)."""
    all_c = "\n".join(b for n, b in bodies.items() if n)
    rules = [
        ("pas de pointeur de l'hote sur 32 bits", "(u32)&" not in all_c),
        ("pas de goto *(void *)", "goto *(void *)" not in all_c),
        ("litteral de code = adresse ARM avec le bit Thumb", "0x2000f01u /* &h_pack10 */" in bodies["t_callptr"]),
        ("appel indirect par nds_call_target", "CALLF(nds_call_target(r12))" in bodies["t_callptr"]),
        ("t_gosub : 4 retours dans gs_", bodies["t_gosub"].count("&&L") == 4),
        ("t_gosub : lr = GOSUB_TAG + k", "lr = 0xfff00000u;" in bodies["t_gosub"] and "lr = 0xfff00003u;" in bodies["t_gosub"]),
        ("t_gosub : retours par GOSUB_RET", bodies["t_gosub"].count("GOSUB_RET(lr)") == 2 and "GOSUB_RET(pc_val)" in bodies["t_gosub"]),
        ("t_thumb : 2 retours dans gs_", bodies["t_thumb"].count("&&L") == 2),
        ("sans sous-routine, pas de gs_", "gs_" not in bodies["t_call10"]),
        ("accroche a une instruction", "NDS_INSN(0, 0x20000e8u);" in bodies["t_smax"]),
        ("accroche a l'entree de chaque fonction", all("HOOK_ENTRY(%s, " % n in b for n, b in bodies.items() if n)),
    ]
    bad = [r for r, ok in rules if not ok]
    for r in bad:
        print("ECHEC (C genere) :", r)
    return len(rules), len(bad)


def write_sources(out, funcs, bodies):
    import arm2c
    out.mkdir(parents=True, exist_ok=True)
    write(out / "prelude.h", arm2c.PRELUDE)
    args = ", ".join(["u32"] * 10)
    host = [(a, n, t) for a, n, t in HOST]
    every = sorted([(a, f[0], f[2]) for a, f in funcs.items()] + host)
    lines = [arm2c.PRELUDE]
    # host, et les entrees HOOK_ENTRY (assembleur) des fonctions traduites
    lines += ["extern u64 %s();" % n for _, n, _ in every]
    lines += [bodies[n] for _, n, _ in every if n in bodies]
    lines.append(bodies[None])
    lines.append("const struct { u32 arm; void *x86; } nds_fn_table[] = {")
    lines += ["  {0x%08xu, (void *)%s}," % (a, n) for a, n, _ in every]
    lines.append("}; const unsigned nds_fn_count = %d;" % len(every))
    write(out / "lifted.c", "\n".join(lines) + "\n")
    h = ["/* Genere par arm2c_test.py */"]
    for a, n, t in every:
        h.append("u64 %s(%s);" % (n, args))
        h.append("#define A_%s 0x%08xu" % (n, a | t))
    write(out / "cases.h", "\n".join(h) + "\n")


# ------------------------------------------------------------------ compilation
def toolchain():
    if sys.platform == "win32":
        import native_build  # met WinLibs ou la boite a outils sur le PATH
        harness = ["-m32", "-std=gnu11", "-O1", "-Wall", "-Wno-unused", "-msse2", "-mfpmath=sse", "-fno-strict-aliasing"]
        return ["gcc"], native_build.LIFT_FLAGS, native_build.RUNTIME_FLAGS, harness, ["-m32"], ".exe"
    harness = ["-std=gnu11", "-O1", "-Wall", "-Wno-unused", "-fno-strict-aliasing", "-ffp-contract=off"]
    if sys.platform == "darwin":
        import native_build  # options du profil darwin (MACOS.md 3.6)
        return [os.environ.get("CC", "clang")], native_build.LIFT_FLAGS, native_build.RUNTIME_FLAGS, harness + ["-Wno-deprecated-non-prototype"], [], ""
    # Linux : pas encore de profil dans native_build.py, memes options que darwin
    lift_flags = ["-std=gnu89", "-O1", "-w", "-fwrapv", "-fno-strict-aliasing", "-ffp-contract=off",
                  "-fno-stack-protector", "-fno-omit-frame-pointer"]
    rt = ["-std=gnu11", "-O2", "-Wall", "-Wno-unused", "-fno-strict-aliasing", "-ffp-contract=off",
          "-Werror=pointer-to-int-cast", "-Werror=int-to-pointer-cast"]
    cc = os.environ.get("CC", "cc")
    if "clang" in subprocess.run([cc, "--version"], capture_output=True, text=True).stdout.lower():
        harness.append("-Wno-deprecated-non-prototype")  # CALLF : u64 (*)(), voulu (MACOS.md 3.6)
    return [cc], lift_flags, rt, harness, ["-lpthread"], ""


def link_win32(objs, exe, out):
    """WinLibs n'a pas de CRT 32 bits (-lmingw32, -lgcc) : comme les modules des jeux, ld et des bibliotheques d'import."""
    defs = out / "msvcrt.def"
    write(defs, "LIBRARY msvcrt.dll\nEXPORTS\nexit\nmemset\nprintf\nputchar\nputs\nvprintf\n_setjmp3\nlongjmp\n")
    libs = []
    for path in (defs, RUNTIME / "kernel32.def"):
        lib = out / ("lib%s.a" % path.stem)
        subprocess.run(["dlltool", "-m", "i386", "--as-flags=--32", "-k", "-d", str(path), "-l", str(lib)], check=True)
        libs.append(str(lib))
    return subprocess.run(["ld", "-m", "i386pe", "-e", "_harness_start", "--image-base", "0x10000000", "--disable-dynamicbase", "--subsystem", "console", *objs, *libs, "-o", str(exe)],
                          capture_output=True, text=True)


def build_and_run(out, variant, arch=None):
    cc, lift_flags, rt_flags, harness_flags, link_flags, exe_suffix = toolchain()
    if arch:
        cc = cc + ["-arch", arch]
        variant = "%s-%s" % (variant, arch)
    extra = ["-DNDS_INSTRUMENT"] if variant.startswith("instrument") else []
    objs = []
    rt = rt_flags + ["-I", str(RUNTIME)]
    for src, flags in ((out / "lifted.c", lift_flags), (RUNTIME / "calltab.c", rt), (RUNTIME / "context.c", rt),
                       (HERE / "harness.c", harness_flags + ["-I", str(out)])):
        obj = out / ("%s-%s.o" % (src.stem, variant))
        p = subprocess.run(cc + list(flags) + extra + ["-c", str(src), "-o", str(obj)], capture_output=True, text=True)
        if p.returncode or p.stderr.strip():
            print(p.stderr[:4000])
        if p.returncode:
            sys.exit("%s : erreur de compilation (%s)" % (src.name, variant))
        objs.append(str(obj))
    exe = out / ("harness-%s%s" % (variant, exe_suffix))
    if sys.platform == "win32":
        p = link_win32(objs, exe, out)
    else:
        p = subprocess.run(cc + [*objs, "-o", str(exe)] + link_flags, capture_output=True, text=True)
    if p.returncode:
        sys.exit(p.stderr[:4000] + "\nedition de liens : erreur (%s)" % variant)
    p = subprocess.run([str(exe)], capture_output=True, text=True)
    sys.stdout.write(p.stdout + p.stderr)
    return p.returncode == 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--regen", action="store_true", help="reassembler arm2c_cases.s et recopier les octets ici")
    ap.add_argument("--out", default=str(HERE.parent / "build" / "tests"), help="dossier des fichiers generes")
    ap.add_argument("--arch", help="macOS : architecture du harnais (arm64, x86_64 par Rosetta)")
    a = ap.parse_args()
    if a.arch and sys.platform != "darwin":
        sys.exit("--arch : macOS seulement")
    if a.regen:
        regen()
        return
    try:
        import capstone  # noqa: F401
    except ImportError:
        sys.exit("capstone manque : pip install capstone==5.0.7")
    if not CASES:
        sys.exit("CASES vide : lancer --regen")
    funcs, bodies = lift()
    n, bad = check_text(bodies)
    print("C genere : %d regles, %d echecs" % (n, bad))
    out = Path(a.out)
    write_sources(out, funcs, bodies)
    ok = [build_and_run(out, v, a.arch) for v in ("normal", "instrument")]
    sys.exit(0 if all(ok) and not bad else 1)


if __name__ == "__main__":
    main()
