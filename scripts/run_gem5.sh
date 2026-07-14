#!/bin/bash
# Runs each codec's RISC-V binary under gem5's syscall-emulation (SE)
# mode with a configurable CPU/cache/memory model, producing both the
# codec's own output file and gem5 architecture-level performance
# statistics (results/gem5/<run>/stats.txt).
#
# gem5 comes AFTER Spike: it does not replace Spike's correctness check,
# it adds simulated cycles, instructions, IPC, and cache behavior on top
# of a binary Spike has already validated. Use a tiny input here -
# gem5 simulation is much slower than Spike.
#
# Usage:
#   bash scripts/run_gem5.sh
#   INPUT=dataset/pcm/small_10frames.pcm CPU_TYPE=TimingSimpleCPU bash scripts/run_gem5.sh
#
# Produces (for INPUT=dataset/pcm/<name>.pcm):
#   results/gem5/<codec>_<name>_encode/<codec>_<name>.bit
#   results/gem5/<codec>_<name>_encode/stats.txt   (+ config.ini, simout, simerr, ...)
#   results/gem5/<codec>_<name>_decode/<codec>_<name>_decoded.pcm
#   results/gem5/<codec>_<name>_decode/stats.txt
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

  for mode in encode decode; do
    RUN_DIR="results/gem5/${codec}_${NAME}_${mode}"
    mkdir -p "$RUN_DIR"
    ABS_RUN_DIR="$ROOT_DIR/$RUN_DIR"

    if [ "$mode" = "encode" ]; then
      IN_PATH="$ROOT_DIR/$INPUT"
      OUT_PATH="$ABS_RUN_DIR/${codec}_${NAME}.bit"
    else
      IN_PATH="$ABS_RUN_DIR/../${codec}_${NAME}_encode/${codec}_${NAME}.bit"
      OUT_PATH="$ABS_RUN_DIR/${codec}_${NAME}_decoded.pcm"
      if [ ! -f "$IN_PATH" ]; then
        echo "error: expected encode output '$IN_PATH' - run encode first (this script does encode then decode per codec)." >&2
        exit 1
      fi
    fi

    echo "==> ${codec}: gem5 ${mode} (outdir=$RUN_DIR)"
    "$GEM5" --outdir="$ABS_RUN_DIR" \
      "$SE_PY" \
      --cmd="$ELF" \
      --options="$mode $IN_PATH $OUT_PATH" \
      --cpu-type="$CPU_TYPE" \
      "${CACHE_FLAGS[@]}" \
      --mem-size="$MEM_SIZE"
  done
done

echo "gem5 run complete for INPUT=$INPUT"
echo "Stats: results/gem5/<codec>_${NAME}_<mode>/stats.txt"
