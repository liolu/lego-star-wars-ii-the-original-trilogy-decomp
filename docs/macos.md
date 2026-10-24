# Building on macOS

The build uses native macOS releases of dsd and objdiff, and Wibo 1.2.0 to run
the original Windows compiler and linker. Apple Silicon needs Rosetta for Wibo.
Install Python 3.11 or newer and Ninja, then use the normal build commands:

```sh
python3 -m venv venv
source venv/bin/activate
pip install -r tools/requirements.txt ninja
python tools/configure.py usa
ninja
python tools/match.py verify
```

Supply your own original ROM as described in [the extraction guide](../extract/README.md).
Existing compiler and tool installations can be selected with `--compiler`,
`--dsd`, `--objdiff`, `--sjiswrap`, and `-w` (the Wibo path).

On macOS, `tools/macos_compiler.py` validates ASCII source and headers, then
forwards the original flags to mwccarm through Wibo. ASCII bytes are identical
in UTF-8 and Shift-JIS. The Windows sjiswrap loader requires an API unavailable
in macOS Wibo; non-ASCII input is rejected instead of changing its encoding.
Use separate `-i DIR` include arguments. Response files are unsupported by
this wrapper. Linux and Windows keep the existing sjiswrap compilation path.

The compiler flags, per-source C/C++ language selector, linker flags, strict
verifier and expected ROM hash remain unchanged. A successful build must print
`VERIFY PASSED` with ROM SHA1 `7c1afa070797945ba5a2577c7ddfded5cf2fc443`.
