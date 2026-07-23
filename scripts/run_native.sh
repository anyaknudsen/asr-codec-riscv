#!/bin/bash
# Runs the native encoder for each codec on one dataset input file,
# producing the reference output that Spike and gem5 encode outputs
# get compared against.
#
# This project only exercises/measures the encoder: decoding is
# assumed to happen off the edge device this project profiles (see
# README.md), so there is no decode step here.
#
# Usage:
#   bash scripts/run_native.sh
#   INPUT=dataset/pcm/tiny_1frame.pcm bash scripts/run_native.sh
#
# Produces (for INPUT=dataset/pcm/<name>.pcm):
#   results/native/<codec>_<name>.bit   (encode output)
#   results/native/<codec>_<name>.log   (encoder stdout, incl. frames=<n>)
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

INPUT="${INPUT:-dataset/pcm/small_10frames.pcm}"
CODECS="${CODECS:-codec_a codec_b codec_c}"
NAME="$(basename "$INPUT")"
NAME="${NAME%.pcm}"

mkdir -p results/native

for codec in $CODECS; do
  BIN="build/${codec}_native"
  if [ ! -x "$BIN" ]; then
    echo "error: $BIN not found. Run scripts/build_native.sh first." >&2
    exit 1
  fi

  BIT="results/native/${codec}_${NAME}.bit"

  echo "==> ${codec}: native encode"
  "$BIN" "$INPUT" "$BIT" | tee "results/native/${codec}_${NAME}.log"
done

echo "Native run complete for INPUT=$INPUT"
