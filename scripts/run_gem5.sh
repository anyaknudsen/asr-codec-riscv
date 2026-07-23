#!/bin/bash
# Runs each codec's RISC-V encoder binary under gem5's syscall-emulation
# (SE) mode with a configurable CPU/cache/memory model, producing both
# the encoded output file and gem5 architecture-level performance
# statistics (results/gem5/<run>/stats.txt).
#
# gem5 comes AFTER Spike: it does not replace Spike's correctness check,
# it adds simulated cycles, instructions, IPC, and cache behavior on top
# of a binary Spike has already validated. Use a tiny input here -
# gem5 simulation is much slower than Spike.
#
# This project only exercises/measures the encoder: decoding is
# assumed to happen off the edge device this project profiles (see
# README.md), so there is no decode step here.
#
# Usage:
#   bash scripts/run_gem5.sh
#   INPUT=dataset/pcm/small_10frames.pcm CPU_TYPE=TimingSimpleCPU bash scripts/run_gem5.sh
#
# Produces (for INPUT=dataset/pcm/<name>.pcm):
#   results/gem5/<codec>_<name>/<codec>_<name>.bit
#   results/gem5/<codec>_<name>/stats.txt   (+ config.ini, simout, simerr, ...)
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

INPUT="${INPUT:-dataset/pcm/tiny_1frame.pcm}"
CODECS="${CODECS:-codec_a codec_b codec_c}"
GEM5="${GEM5:-gem5.opt}"
CPU_TYPE="${CPU_TYPE:-TimingSimpleCPU}"
MEM_SIZE="${MEM_SIZE:-512MB}"
L1I_SIZE="${L1I_SIZE:-32kB}"
L1D_SIZE="${L1D_SIZE:-32kB}"
CACHES="${CACHES:-1}" # set to 0 to disable --caches (e.g. for AtomicSimpleCPU)

if [ -z "${GEM5_ROOT:-}" ]; then
  echo "error: set GEM5_ROOT to the gem5 checkout (contains configs/deprecated/example/se.py)." >&2
  exit 1
fi
SE_PY="${SE_PY:-$GEM5_ROOT/configs/deprecated/example/se.py}"

if ! command -v "$GEM5" >/dev/null 2>&1; then
  echo "error: gem5 binary ('$GEM5') not found on PATH." >&2
  echo "       See README.md 'gem5 setup' for build instructions." >&2
  exit 1
fi
if [ ! -f "$SE_PY" ]; then
  echo "error: se.py not found at '$SE_PY' (check GEM5_ROOT/SE_PY)." >&2
  exit 1
fi

NAME="$(basename "$INPUT")"
NAME="${NAME%.pcm}"

CACHE_FLAGS=()
if [ "$CACHES" = "1" ]; then
  CACHE_FLAGS=(--caches --l1i_size="$L1I_SIZE" --l1d_size="$L1D_SIZE")
fi

mkdir -p results/gem5

for codec in $CODECS; do
  ELF="$ROOT_DIR/build/${codec}_rv64.elf"
  if [ ! -f "$ELF" ]; then
    echo "error: $ELF not found. Run scripts/build_riscv.sh first." >&2
    exit 1
  fi

  RUN_DIR="results/gem5/${codec}_${NAME}"
  mkdir -p "$RUN_DIR"
  ABS_RUN_DIR="$ROOT_DIR/$RUN_DIR"

  IN_PATH="$ROOT_DIR/$INPUT"
  OUT_PATH="$ABS_RUN_DIR/${codec}_${NAME}.bit"

  echo "==> ${codec}: gem5 encode (outdir=$RUN_DIR)"
  "$GEM5" --outdir="$ABS_RUN_DIR" \
    "$SE_PY" \
    --cmd="$ELF" \
    --options="$IN_PATH $OUT_PATH" \
    --output="$ABS_RUN_DIR/simout" \
    --errout="$ABS_RUN_DIR/simerr" \
    --cpu-type="$CPU_TYPE" \
    "${CACHE_FLAGS[@]}" \
    --mem-size="$MEM_SIZE"
done

echo "gem5 run complete for INPUT=$INPUT"
echo "Stats: results/gem5/<codec>_${NAME}/stats.txt"
