#!/bin/bash
# Compares native, Spike, and gem5 encode outputs for the same dataset
# input. This is the core correctness check of the whole workflow:
#
#   native .bit == Spike .bit == gem5 .bit
#
# This project only exercises/measures the encoder: decoding is
# assumed to happen off the edge device this project profiles (see
# README.md), so there is nothing to compare on the decode side.
#
# Only trust gem5 performance stats (scripts/run_gem5.sh, results/gem5/*/stats.txt)
# after this script reports that all outputs match.
#
# Usage:
#   bash scripts/compare_outputs.sh
#   INPUT=dataset/pcm/tiny_1frame.pcm bash scripts/compare_outputs.sh
#   WITH_GEM5=0 bash scripts/compare_outputs.sh   # native vs Spike only
set -uo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

INPUT="${INPUT:-dataset/pcm/small_10frames.pcm}"
CODECS="${CODECS:-codec_a codec_b codec_c}"
WITH_GEM5="${WITH_GEM5:-1}"
NAME="$(basename "$INPUT")"
NAME="${NAME%.pcm}"

fail=0

compare() {
  local label="$1" a="$2" b="$3"
  if [ ! -f "$a" ]; then
    echo "SKIP  $label: missing $a"
    return
  fi
  if [ ! -f "$b" ]; then
    echo "SKIP  $label: missing $b"
    return
  fi
  if cmp -s "$a" "$b"; then
    echo "OK    $label"
  else
    echo "FAIL  $label: $a != $b"
    fail=1
  fi
}

for codec in $CODECS; do
  NATIVE_BIT="results/native/${codec}_${NAME}.bit"
  SPIKE_BIT="results/spike/${codec}_${NAME}.bit"
  GEM5_BIT="results/gem5/${codec}_${NAME}/${codec}_${NAME}.bit"

  compare "${codec} native vs spike (encode .bit)" "$NATIVE_BIT" "$SPIKE_BIT"

  if [ "$WITH_GEM5" = "1" ]; then
    compare "${codec} native vs gem5 (encode .bit)" "$NATIVE_BIT" "$GEM5_BIT"
  fi
done

if [ "$fail" -eq 0 ]; then
  echo "All outputs match"
else
  echo "One or more comparisons FAILED (see above)"
fi

exit "$fail"
