"""Recettes des jeux natifs : ce qu'il faut pour traduire un jeu, sans une seule instruction du jeu.

Une recette dit ou sont les fonctions et quels mots du binaire sont des pointeurs de fonctions (adresses et noms,
fruit de la decompilation). Le code lui-meme est lu dans la rom du joueur, verifiee par son SHA-1.
"""
import gzip
import hashlib
import json
import re
import struct


def rom_sha1(path):
    h = hashlib.sha1()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def blz_uncompress(image, base, end_addr):
    """Decompression du module principal (MIi_UncompressBackward du SDK)."""
    end = end_addr - base
    packed_sizes, extra = struct.unpack_from("<II", image, end - 8)
    start = end - (packed_sizes >> 24)
    stop = end - (packed_sizes & 0xFFFFFF)
    out = bytearray(image[:end]) + bytearray(extra)
    src, dst = start, end + extra
    while src > stop:
        src -= 1
        flags = out[src]
        for _ in range(8):
            if src <= stop:
                break
            if flags & 0x80:
                b = (out[src - 1] << 8) | out[src - 2]
                src -= 2
                count = (b >> 12) + 3
                offset = (b & 0xFFF) + 3
                for _ in range(count):
                    dst -= 1
                    out[dst] = out[dst + offset]
            else:
                src -= 1
                dst -= 1
                out[dst] = out[src]
            flags = (flags << 1) & 0xFF
    return bytes(out)


def rom_images(rom_path, build_info_offset):
    """Le code du jeu d'apres la rom : module principal decompresse, puis ITCM/DTCM a leurs adresses."""
    with open(rom_path, "rb") as f:
        rom = f.read()
    offset, entry, ram, size = struct.unpack_from("<IIII", rom, 0x20)
    arm9 = rom[offset : offset + size]
    info = struct.unpack_from("<6I", arm9, build_info_offset)
    if info[5]:
        arm9 = blz_uncompress(arm9, ram, info[5])
    images = {"arm9": (ram, arm9)}
    src = info[2]
    # SDK des jeux DSi : entrees de 16 octets au lieu de 12
    step = 12 if (info[1] - info[0]) % 12 == 0 else 16
    for at in range(info[0], info[1], step):
        dest, length, _bss = struct.unpack_from("<III", arm9, at - ram)
        images["auto_%08x" % dest] = (dest, arm9[src - ram : src - ram + length])
        src += length
    return images, entry


def rom_overlays(rom_path):
    """Les overlays de l'ARM9 d'apres la rom, decompresses : numero -> (adresse, octets)."""
    with open(rom_path, "rb") as f:
        rom = f.read()
    fat, table, size = struct.unpack_from("<I", rom, 0x48)[0], *struct.unpack_from("<II", rom, 0x50)
    out = {}
    for at in range(table, table + size, 32):
        num, ram, _size, _bss, _ctor, _ctor_end, file_id, packed = struct.unpack_from("<8I", rom, at)
        start, end = struct.unpack_from("<II", rom, fat + file_id * 8)
        raw = rom[start:end]
        if packed >> 24 & 1:
            raw = blz_uncompress(raw, ram, ram + (packed & 0xFFFFFF))
        out[num] = (ram, raw)
    return out


def read_memory(game, addr, n):
    """Le code d'un overlay passe avant les modules : ses adresses peuvent etre aussi celles des donnees d'un autre."""
    for num, start, end in game.overlays:
        if start <= addr < end:
            base, raw = game.overlay_images[num]
            return raw[addr - base : addr - base + n]
    for base, raw in game.image.values():
        if base <= addr < base + len(raw):
            return raw[addr - base : addr - base + n]
    return None


def default_name(addr):
    return "func_%08x" % addr


def export(game, code, sha1, path):
    """Ecrit la recette d'un jeu depuis son projet."""
    functions = []
    for addr in sorted(game.functions):
        name, size, thumb = game.functions[addr]
        functions.append([addr, size, 1 if thumb else 0, None if name == default_name(addr) else name])
    known = set(game.functions)
    loads = sorted([frm, to & ~1] for frm, (kind, to) in game.relocs.items() if kind == "load" and (to & ~1) in known)
    data = {n: a for n, a in game.data.items() if not re.match(r"(data|lbl|func|sub|unk)_", n)}
    recipe = {"code": code, "sha1": sha1, "build_info": game.build_info, "functions": functions, "loads": loads, "data": data, "overlays": game.overlays}
    if getattr(game, "multi", False):
        # overlays qui se chevauchent ("overlays": "multi") : le module principal seul dans functions, chaque overlay a part
        recipe["functions"] = [f for f in functions if f[0] in game.main_addrs]
        recipe["ov_functions"] = {str(num): [[addr, size, 1 if thumb else 0, None if name == default_name(addr) else name]
                                             for addr, (name, size, thumb) in sorted(funcs.items())]
                                  for num, funcs in sorted(game.ov_functions.items())}
        recipe["ov_loads"] = {str(num): sorted([frm, to & ~1] for frm, (kind, to) in relocs.items() if kind == "load")
                              for num, relocs in sorted(game.ov_relocs.items())}
    # sans date dans le fichier : une recette identique ne change pas dans git
    with open(path, "wb") as raw, gzip.GzipFile(filename="", fileobj=raw, mode="wb", compresslevel=9, mtime=0) as z:
        z.write(json.dumps(recipe, separators=(",", ":")).encode("utf8"))


class RecipeGame:
    """Meme interface que Game (native_build), mais d'apres une recette et la rom du joueur."""

    def __init__(self, recipe_path, rom_path):
        with gzip.open(recipe_path, "rt", encoding="utf8") as f:
            r = json.load(f)
        self.code = r["code"]
        found = rom_sha1(rom_path)
        if found != r["sha1"]:
            raise SystemExit("rom differente de celle prevue pour %s : %s au lieu de %s" % (self.code, found, r["sha1"]))
        self.functions = {a: (n or default_name(a), s, bool(t)) for a, s, t, n in r["functions"]}
        self.relocs = {frm: ("load", to) for frm, to in r["loads"]}
        self.data = r["data"]
        self.units = []
        self.build_info = r["build_info"]
        self.image, self.entry = rom_images(rom_path, self.build_info)
        self.overlays = [tuple(o) for o in r.get("overlays", [])]
        self.overlay_images = rom_overlays(rom_path) if self.overlays else {}
        self.main_addrs = set(self.functions)
        self.ov_functions = {int(num): {a: (n or default_name(a), s, bool(t)) for a, s, t, n in funcs} for num, funcs in r.get("ov_functions", {}).items()}
        self.ov_relocs = {int(num): {frm: ("load", to) for frm, to in loads} for num, loads in r.get("ov_loads", {}).items()}

    def memory(self, addr, n):
        return read_memory(self, addr, n)
