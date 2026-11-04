#!/usr/bin/env python3
"""Archive du runtime d'un jeu (ROADMAP-MODS.md, etape 3 bis) : ce que le launcher telecharge depuis la release runtime-v<n>
du depot du jeu, avec SHA256SUMS. Sans code du jeu : traducteur, runtime nitro-pc, SDK des mods et recette (adresses).

  python native/tools/runtime_package.py <code> [--version n] [--out dossier]

Lance depuis le dossier native/ d'un depot de jeu (rempli par runtime_sync.py), ou depuis celui du studio pour un essai.
La version vient de native/VERSION si elle n'est pas donnee. Sortie : native-runtime-<n>.zip et SHA256SUMS.
"""
import argparse
import hashlib
import json
import sys
import zipfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
NATIVE = HERE.parent
TOOLS = ["arm2c.py", "native_build.py", "recipe.py"]


def files(code):
    """(chemin dans l'archive, contenu) : rien de plus que ce que la generation lit."""
    out = [("native/tools/" + name, (HERE / name).read_bytes()) for name in TOOLS]
    out += [("native/nitro-pc/" + f.name, f.read_bytes()) for f in sorted((NATIVE / "nitro-pc").iterdir()) if f.is_file()]
    out += [("native/sdk/" + f.name, f.read_bytes()) for f in sorted((NATIVE / "sdk").iterdir()) if f.is_file()]
    recipe = NATIVE / "games" / (code + ".json.gz")
    if not recipe.is_file():
        sys.exit("pas de recette %s" % recipe)
    out.append(("native/games/" + recipe.name, recipe.read_bytes()))
    patches = json.loads((NATIVE / "games" / "patches.json").read_text(encoding="utf8"))
    # un seul jeu : les reglages des autres n'ont rien a faire ici
    out.append(("native/games/patches.json", (json.dumps({code: patches[code]} if code in patches else {}, indent=1, sort_keys=True) + "\n").encode()))
    for name in ("LICENSE", "VERSION"):
        if (NATIVE / name).is_file():
            out.append(("native/" + name, (NATIVE / name).read_bytes()))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("code")
    ap.add_argument("--version", type=int, default=None)
    ap.add_argument("--out", default=".")
    a = ap.parse_args()
    version = a.version
    if version is None:
        version = int((NATIVE / "VERSION").read_text().strip())
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    archive = out / ("native-runtime-%d.zip" % version)
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name, data in files(a.code):
            info = zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))  # meme archive pour les memes fichiers
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            z.writestr(info, data)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    (out / "SHA256SUMS").write_text("%s  %s\n" % (digest, archive.name), encoding="ascii", newline="\n")
    print(archive, archive.stat().st_size // 1024, "Ko", digest)


if __name__ == "__main__":
    main()
