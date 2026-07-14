#!/bin/bash
# Builds one native (host-architecture) binary per codec.
#
# Native binaries are the ground truth: they run directly on the build
# machine (no RISC-V simulator involved) and their output is what Spike
# and gem5 output gets compared against later.
#
# Usage:
#   bash scripts/build_native.sh
#
# Produces:
#   build/codec_a_native
#   build/codec_b_native
#   build/codec_c_native
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

CC="${NATIVE_CC:-cc}"
OPT="${OPT:--O2}"
CODECS="${CODECS:-codec_a codec_b codec_c}"

mkdir -p build

for codec in $CODECS; do
  echo "==> building build/${codec}_native"
  "$CC" $OPT -Wall -Wextra -std=c11 -ffp-contract=off \
    -Icommon -Icodecs/"$codec" \
    app/main.c common/io.c common/pcm.c \
    codecs/"$codec"/encoder.c codecs/"$codec"/decoder.c \
    -lm \
    -o build/"${codec}"_native
done

echo "Native build complete."
