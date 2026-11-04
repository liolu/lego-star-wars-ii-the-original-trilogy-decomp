#!/usr/bin/env python3
"""Archive du SDK des mods : nds-mod-sdk-<version>.zip, publiee sur la release sdk-v<version> du studio (.github/workflows/sdk.yml).
SPDX-License-Identifier: MIT

  python native/sdk/make_sdk.py [--out dossier]

Le modele (template/) recoit une copie des en-tetes et de package_mod.py dans template/sdk/ : copie tel quel, il compile.
La version vient de native/sdk/VERSION.
"""
import argparse
import zipfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
TOP = ["nds_mod.h", "nds_util.h", "package_mod.py", "LICENSE", "README.md", "VERSION"]
FOR_TEMPLATE = ["nds_mod.h", "nds_util.h", "package_mod.py", "LICENSE"]


def add(z, source, name):
    info = zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
    info.compress_type = zipfile.ZIP_DEFLATED
    executable = name.endswith((".sh", ".py"))
    info.external_attr = (0o755 if executable else 0o644) << 16
    z.writestr(info, source.read_bytes())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=".")
    a = ap.parse_args()
    version = (HERE / "VERSION").read_text().strip()
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    archive = out / ("nds-mod-sdk-%s.zip" % version)
    root = "nds-mod-sdk-%s/" % version
    with zipfile.ZipFile(archive, "w") as z:
        for name in TOP:
            add(z, HERE / name, root + name)
        for folder in ("docs", "template", "template-script"):
            for f in sorted((HERE / folder).rglob("*")):
                if f.is_file():
                    add(z, f, root + f.relative_to(HERE).as_posix())
        for name in FOR_TEMPLATE:
            add(z, HERE / name, root + "template/sdk/" + name)
    print(archive, archive.stat().st_size // 1024, "Ko")


if __name__ == "__main__":
    main()
