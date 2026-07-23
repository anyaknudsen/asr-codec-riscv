#!/bin/bash
# Instruction-level inspection helper for step 4 of the workflow:
# Spike/objdump instruction-level inspection.
#
# This is NOT a performance tool (see scripts/run_gem5.sh for that). It
# only helps you sanity-check what code the RISC-V compiler generated
# (e.g. whether floating-point, multiply/divide, or compressed
# instructions appear, and whether the hot loops look reasonable) and
# produces a full retired-instruction trace from Spike on a tiny input.
#
# Traces get large fast, so this intentionally only runs on
# dataset/pcm/tiny_1frame.pcm-sized inputs by default.
#
# Usage:
#   bash scripts/inspect_riscv.sh
#   INPUT=dataset/pcm/tiny_1frame.pcm CODECS=codec_a bash scripts/inspect_riscv.sh
#
# Produces:
#   results/spike/<codec>_rv64.asm          (objdump disassembly)
#   results/spike/<codec>_<name>.trace       (spike -l instruction trace)
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

INPUT="${INPUT:-dataset/pcm/tiny_1frame.pcm}"
CODECS="${CODECS:-codec_a codec_b codec_c}"
SPIKE="${SPIKE:-spike}"
PK="${PK:-pk}"
OBJDUMP="${OBJDUMP:-riscv64-linux-gnu-objdump}"
NAME="$(basename "$INPUT")"
NAME="${NAME%.pcm}"

mkdir -p results/spike

for codec in $CODECS; do
  ELF="build/${codec}_rv64.elf"
  if [ ! -f "$ELF" ]; then
    echo "error: $ELF not found. Run scripts/build_riscv.sh first." >&2
    exit 1
  fi

  echo "==> ${codec}: objdump disassembly"
  "$OBJDUMP" -d "$ELF" > "results/spike/${codec}_rv64.asm"

  echo "==> ${codec}: spike instruction trace (encode, tiny input)"
  BIT="results/spike/${codec}_${NAME}.bit"
  "$SPIKE" -l "$PK" "$ELF" "$INPUT" "$BIT" \
    > /dev/null 2> "results/spike/${codec}_${NAME}.trace"
done

echo "Inspection artifacts written under results/spike/."
echo "Look for: hot functions, mul/div instructions, float instructions,"
echo "compressed instructions, and whether inner loops look reasonable."
