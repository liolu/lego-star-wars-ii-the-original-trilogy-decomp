#!/usr/bin/env python3
"""Compile tout le runtime nitro-pc sans rom (native/MACOS.md 3.8), avec les options et la liste de sources de native_build.py.

  python3 native/tests/runtime_check.py [--arch arm64|x86_64]    # macOS : -Werror, les -Werror de pointeurs en plus
  python  native/tests/runtime_check.py [--arch x86_64]          # Windows : gcc -m32 (defaut) ou -m64, comme native_build.py

Ne lie rien (les symboles du jeu manquent) : les fichiers sont seulement compiles. Code de sortie 1 au premier echec.
"""
import argparse
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import native_build as nb


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--arch", default=None, help="macOS : arm64 ou x86_64 (par defaut celle de cette machine) ; Windows : x86 (defaut) ou x86_64")
    a = ap.parse_args()
    if nb.DARWIN:
        arch = a.arch or ("arm64" if __import__("platform").machine() == "arm64" else "x86_64")
        # Les appels par pointeur sans prototype (Fn) sont voulus : -std=gnu11 est fige, CALLF en depend (MACOS.md 3.6).
        # clang les signale comme obsoletes en C23 ; ici ils restent des avertissements, tout le reste est une erreur.
        cc = ["clang", "-arch", arch, "-Werror", "-Wno-error=deprecated-non-prototype"]
    elif a.arch == "x86_64":
        # les modules Windows des joueurs : memes options que native_build.py --arch x86_64
        cc = ["gcc", "-m64"]
    elif a.arch in (None, "x86"):
        cc = ["gcc"] + nb.CC[1:]  # gcc -m32
    else:
        sys.exit("--arch : x86 ou x86_64, pas %s" % a.arch)
    sources = nb.runtime_sources()
    runtime_flags = nb.RUNTIME_FLAGS
    if not nb.DARWIN and a.arch == "x86_64":
        runtime_flags = ["-m64" if f == "-m32" else f for f in runtime_flags]
        runtime_flags += ["-Werror=pointer-to-int-cast", "-Werror=int-to-pointer-cast"]
    flags = runtime_flags + ["-I", str(nb.RUNTIME), "-c", "-o"]
    null = "NUL" if sys.platform == "win32" else "/dev/null"

    def compile_one(src):
        r = subprocess.run(cc + flags + [null, str(src)], capture_output=True, text=True)
        return src, r.returncode, (r.stdout + r.stderr).strip()

    failed = 0
    with ThreadPoolExecutor() as pool:
        for src, code, output in pool.map(compile_one, sources):
            print(("ok     " if code == 0 else "ECHEC  ") + src.name)
            if output:
                print(output)
            failed += code != 0
    print("%d fichiers, %d en echec (%s)" % (len(sources), failed, " ".join(cc)))
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
