#!/bin/bash
# Runs each codec's RISC-V binary under Spike, the golden functional
# RISC-V ISA simulator, using riscv-pk (the proxy kernel) to provide
# file I/O and other syscalls over Spike's HTIF mechanism.
#
# Spike checks that the RISC-V binary behaves correctly. It is
# deliberately NOT used for performance numbers here: Spike does not
# model caches, pipelines, branch prediction, or memory stalls. See
# scripts/run_gem5.sh for the architecture-level performance simulator.
#
# Usage:
#   bash scripts/run_spike.sh
#   INPUT=dataset/pcm/tiny_1frame.pcm bash scripts/run_spike.sh
#
# Produces (for INPUT=dataset/pcm/<name>.pcm):
#   results/spike/<codec>_<name>.bit
#   results/spike/<codec>_<name>_decoded.pcm
#
# After this, compare against native output:
#   bash scripts/compare_outputs.sh
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

INPUT="${INPUT:-dataset/pcm/small_10frames.pcm}"
CODECS="${CODECS:-codec_a codec_b codec_c}"
SPIKE="${SPIKE:-spike}"
PK="${PK:-pk}"
NAME="$(basename "$INPUT")"
NAME="${NAME%.pcm}"

if ! command -v "$SPIKE" >/dev/null 2>&1; then
  echo "error: Spike ('$SPIKE') not found on PATH." >&2
  echo "       See README.md 'Spike setup' for build instructions." >&2
  exit 1
fi

if ! command -v "$PK" >/dev/null 2>&1; then
  echo "error: riscv-pk ('$PK') not found on PATH." >&2
  echo "       See README.md 'Spike setup' for build instructions." >&2
  exit 1
fi

mkdir -p results/spike

for codec in $CODECS; do
  ELF="build/${codec}_rv64.elf"
  if [ ! -f "$ELF" ]; then
    echo "error: $ELF not found. Run scripts/build_riscv.sh first." >&2
    exit 1
  fi

  BIT="results/spike/${codec}_${NAME}.bit"
  DECODED="results/spike/${codec}_${NAME}_decoded.pcm"

  echo "==> ${codec}: spike encode"
  "$SPIKE" "$PK" "$ELF" encode "$INPUT" "$BIT" | tee "results/spike/${codec}_${NAME}_encode.log"

  echo "==> ${codec}: spike decode"
  "$SPIKE" "$PK" "$ELF" decode "$BIT" "$DECODED" | tee "results/spike/${codec}_${NAME}_decode.log"
done

echo "Spike run complete for INPUT=$INPUT"
