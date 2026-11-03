# LEGO Star Wars II decompilation (USA)

<!-- bytematch:start -->
[![Code](https://bytematch.dev/liolu/lego-star-wars-ii-the-original-trilogy-decomp.svg?label=Code&measure=matched_code_percent)](https://bytematch.dev/liolu/lego-star-wars-ii-the-original-trilogy-decomp) [![Data](https://bytematch.dev/liolu/lego-star-wars-ii-the-original-trilogy-decomp.svg?label=Data&measure=matched_data_percent)](https://bytematch.dev/liolu/lego-star-wars-ii-the-original-trilogy-decomp) [![Functions](https://bytematch.dev/liolu/lego-star-wars-ii-the-original-trilogy-decomp.svg?label=Functions&measure=matched_functions)](https://bytematch.dev/liolu/lego-star-wars-ii-the-original-trilogy-decomp)
<!-- bytematch:end -->

Nintendo DS, game code `AL7E`. Matching decompilation with Metrowerks mwccarm (compiler version still to be identified: `python tools/probe_compiler.py`).

No game assets in this repository: see `extract/README.md`.

## C++ translation units

The build supports evidenced natural C++ alongside C,
with per-source language selection and the unchanged full-ROM matching gate.
See [the C++ build and source guide](docs/cpp.md) before adding a `.cpp` file.
See [matching exceptions](docs/exceptions.md) before using assembly, `volatile` or other non-natural constructs.

## Progress

<!-- progress:start -->
`██████████████░░░░░░` **71.58%** of the code matched · data 100.00% · 4412/4764 functions

| Part of the code | Files | Functions | Share of all code |
|---|---|---|---|
| Auto-matched (raw m2c output, to clean up) | 4393 | 4392/4393 | 70.41% |
| Reused from other decompilations, not merged into files yet | 20 | 20/20 | 1.17% |
| Game code: `src/data/` | 1462 | 0/0 | 0.00% |

*Updated 2026-11-03 by `python tools/match.py progress` ([DS Decomp Studio](https://github.com/liolu/ds-decomp-studio)). A function counts only when it compiles to the exact same bytes and the ROM still rebuilds byte for byte.*
<!-- progress:end -->

```shell
python tools/configure.py usa
ninja
python tools/match.py status
```

Created from the rayman-rr template. See `AGENTS.md` for the rules.
