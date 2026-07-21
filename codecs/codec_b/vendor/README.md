# codec_b vendored third-party sources

`codecs/codec_b/encoder.c` and `codecs/codec_b/decoder.c` are thin
adapters (see the module comment at the top of each file) that call
into two vendored third-party libraries copied verbatim into this
directory. Nothing here is written by this project; it is pulled in
unmodified (aside from removing files this project doesn't need) so
`codecs/codec_b/` is self-contained and the shared build scripts
(`scripts/build_native.sh`, `scripts/build_riscv.sh`) can compile it
like any other codec (see "Build integration" in `../codec.h`).

## shine/ - MP3 encoder (used by `encoder.c`)

- Upstream: <https://github.com/savonet/shine> (fixed-point MP3 encoder).
- Vendored from: `src/lib/*.c`, `src/lib/*.h` at commit
  `098b9aaa6770a41bdf3f9c049307c1398f04d2d3` (2022-12-30). Only the
  library itself is vendored - `src/bin/` (the `shineenc` CLI, which
  depends on libsndfile-style WAVE file reading) is not needed since
  `encoder.c` feeds PCM frames to the library directly.
- License: LGPL v2 (see `shine/COPYING`).

## minimp3/ - MP3 decoder (used by `decoder.c`)

- Upstream: <https://github.com/lieff/minimp3> (single-header MP3
  decoder).
- Vendored from: `minimp3.h` at commit
  `ca7c706001331a5a8e3182ce3b3ce3b243589154` (2021-07-31). This is the
  low-level, dependency-free decoding API only (`mp3dec_init()` /
  `mp3dec_decode_frame()`); `minimp3_ex.h` (the higher-level
  file/seek/mmap helpers) is not needed here since `decoder.c` already
  has the whole compressed input in memory via `common/io.h` and drives
  the frame loop itself.
- `minimp3.h` is a header-only library: declarations are always
  visible, and the implementation is compiled in exactly once from
  `minimp3_impl.c` (`#define MINIMP3_IMPLEMENTATION` before including
  it) so it isn't duplicated across translation units.
- License: CC0 1.0 / public domain (see `minimp3/LICENSE`).

## Why vendor instead of building against `/agent/repos/shine` and
`/agent/repos/minimp3` directly?

`codecs/<name>/` folders are meant to be self-contained (see
`README.md` "Repository conventions") and the RISC-V cross-build in
particular should not depend on paths outside this repository. Copying
the (small, header/lib-only) pieces actually needed keeps
`scripts/build_native.sh`/`scripts/build_riscv.sh` codec-layout-agnostic
(see the "glob every `.c` file under `codecs/<name>/`" comment in those
scripts) and keeps codec_b buildable even if the sibling `shine`/
`minimp3` checkouts aren't present.
