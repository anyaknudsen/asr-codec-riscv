#!/bin/bash
# Cross-compiles one static RISC-V ELF binary per codec.
#
# This does NOT run anything - it only produces build/<codec>_rv64.elf
# binaries meant to be executed under Spike (scripts/run_spike.sh) or
# gem5 (scripts/run_gem5.sh). The RISC-V ELF cannot run directly on the
# build machine.
#
# Toolchain notes (see README.md "RISC-V toolchain setup" for the full
# rationale):
#   - Default is riscv64-linux-gnu-gcc (glibc), producing a static,
#     Linux-syscall-ABI binary. This is what lets the SAME binary run
#     under `spike pk ...` (riscv-pk implements the Linux RV64 syscall
#     table over ecall) and under gem5's syscall-emulation (SE) mode.
#   - -march/-mabi select the ISA/ABI. Override RISCV_ISA/RISCV_ABI (see
#     below) to compare variants, e.g.:
#       RISCV_ISA=rv64imac RISCV_ABI=lp64 bash scripts/build_riscv.sh
#     Note: an integer-only ABI like lp64 requires a matching glibc
#     multilib (not installed by default on most distros) or switching
#     RISCV_CC to a bare-metal newlib/picolibc toolchain
#     (riscv64-unknown-elf-gcc) with its own syscall layer. The default
#     rv64gc/lp64d combination is the one this project's scripts are
#     tested against.
#   - -ffp-contract=off prevents the compiler from fusing
#     multiply+add into a single fused-multiply-add instruction on
#     targets that have one (RISC-V's fmadd.d) when the host compiler
#     would not have fused the same expression (x86-64 without -mfma).
#     Without this flag, floating-point codecs could produce
#     bit-different (though equally "correct") output on native vs.
#     RISC-V builds purely from rounding differences, breaking the
#     cmp-based output comparison in scripts/compare_outputs.sh.
#
# Usage:
#   bash scripts/build_riscv.sh
#
# Produces:
#   build/codec_a_rv64.elf
#   build/codec_b_rv64.elf
#   build/codec_c_rv64.elf
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

RISCV_CC="${RISCV_CC:-riscv64-linux-gnu-gcc}"
RISCV_ISA="${RISCV_ISA:-rv64gc}"
RISCV_ABI="${RISCV_ABI:-lp64d}"
OPT="${OPT:--O2}"
CODECS="${CODECS:-codec_a codec_b codec_c}"

if ! command -v "$RISCV_CC" >/dev/null 2>&1; then
  echo "error: RISC-V cross-compiler '$RISCV_CC' not found on PATH." >&2
  echo "       See README.md 'RISC-V toolchain setup' for install instructions." >&2
  exit 1
fi

mkdir -p build

for codec in $CODECS; do
  echo "==> building build/${codec}_rv64.elf ($RISCV_ISA/$RISCV_ABI via $RISCV_CC)"
  "$RISCV_CC" $OPT -static -march="$RISCV_ISA" -mabi="$RISCV_ABI" \
    -Wall -Wextra -std=c11 -ffp-contract=off \
    -Icommon -Icodecs/"$codec" \
    app/main.c common/io.c common/pcm.c \
    codecs/"$codec"/encoder.c codecs/"$codec"/decoder.c \
    -lm \
    -o build/"${codec}"_rv64.elf
done

echo "RISC-V build complete."
