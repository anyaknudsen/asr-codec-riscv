# codecs/codec_b vendored third-party sources

`codecs/codec_b/encoder.c` is a thin adapter (see the module comment
at the top of the file) that calls into a vendored third-party library
copied verbatim into this directory. Nothing here is written by this
project; it is pulled in unmodified (aside from removing files this
project doesn't need) so `codecs/codec_b/` is self-contained and the
shared build scripts (`scripts/build_native.sh`,
`scripts/build_riscv.sh`) can compile it like any other codec (see
"Build integration" in `../codec.h`).

This project only implements/measures the encoder side of codec_b -
decoding is assumed to happen off the edge device this project
profiles (see the top-level README.md) - so no MP3 decoder is vendored
here.

## shine/ - MP3 encoder (used by `encoder.c`)

- Upstream: <https://github.com/savonet/shine> (fixed-point MP3 encoder).
- Vendored from: `src/lib/*.c`, `src/lib/*.h` at commit
  `098b9aaa6770a41bdf3f9c049307c1398f04d2d3` (2022-12-30). Only the
  library itself is vendored - `src/bin/` (the `shineenc` CLI, which
  depends on libsndfile-style WAVE file reading) is not needed since
  `encoder.c` feeds PCM frames to the library directly.
- License: LGPL v2 (see `shine/COPYING`).

## Why vendor instead of building against `/agent/repos/shine` directly?

`codecs/<name>/` folders are meant to be self-contained (see
`README.md` "Repository conventions") and the RISC-V cross-build in
particular should not depend on paths outside this repository. Copying
the (small, lib-only) pieces actually needed keeps
`scripts/build_native.sh`/`scripts/build_riscv.sh` codec-layout-agnostic
(see the "glob every `.c` file under `codecs/<name>/`" comment in those
scripts) and keeps codec_b buildable even if the sibling `shine`
checkout isn't present.
