#!/usr/bin/env python3
"""Archives d'un mod natif pour sa release : une par machine, avec SHA256SUMS (ce que le launcher telecharge et controle).
SPDX-License-Identifier: MIT

  python3 sdk/package_mod.py [--out dist]      depuis le dossier du mod (celui de nds-mod.json)

Chaque archive <id>-<version>-<machine>.zip contient le dossier du mod tel que le launcher l'installe : nds-mod.json, les fichiers
de jeu (fichiers/, remplacements/, dossiers par code de jeu) et la seule bibliotheque de cette machine. Une machine dont la
bibliotheque n'a pas ete compilee est sautee, et dite.
"""
import argparse
import hashlib
import json
import sys
import zipfile
from pathlib import Path

DATA = ("fichiers", "remplacements")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--mod", default=".", help="dossier du mod (nds-mod.json)")
    ap.add_argument("--out", default="dist")
    a = ap.parse_args()
    root = Path(a.mod).resolve()
    manifest = json.loads((root / "nds-mod.json").read_text(encoding="utf8"))
    mod_id, version = manifest.get("id"), manifest.get("version")
    if not mod_id or not version:
        sys.exit("nds-mod.json : \"id\" et \"version\" sont obligatoires")
    if manifest.get("format") != 2 or not manifest.get("plugin"):
        sys.exit("nds-mod.json : un mod natif a \"format\": 2 et \"plugin\" (machine -> bibliotheque)")
    games = manifest.get("games") or {}
    game_dirs = [root / code for code in (games if isinstance(games, (list, dict)) else []) if (root / code).is_dir()]
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    sums = []
    for platform, lib in sorted(manifest["plugin"].items()):
        library = root / lib
        if not library.is_file():
            print("saute %s : %s n'est pas compile" % (platform, lib))
            continue
        name = "%s-%s-%s.zip" % (mod_id, version, platform)
        with zipfile.ZipFile(out / name, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
            z.write(root / "nds-mod.json", "%s/nds-mod.json" % mod_id)
            z.write(library, "%s/%s" % (mod_id, Path(lib).as_posix()))
            for folder in [root / d for d in DATA] + game_dirs:
                for f in sorted(folder.rglob("*")) if folder.is_dir() else []:
                    if f.is_file():
                        z.write(f, "%s/%s" % (mod_id, f.relative_to(root).as_posix()))
        digest = hashlib.sha256((out / name).read_bytes()).hexdigest()
        sums.append("%s  %s" % (digest, name))
        print(name, (out / name).stat().st_size // 1024, "Ko")
    if not sums:
        sys.exit("aucune bibliotheque compilee : lancer build.sh d'abord")
    (out / "SHA256SUMS").write_text("\n".join(sums) + "\n", encoding="ascii", newline="\n")


if __name__ == "__main__":
    main()
