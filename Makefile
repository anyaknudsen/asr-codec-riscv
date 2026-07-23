# Thin wrapper around scripts/*.sh and scripts/*.py. Scripts remain the
# source of truth (they're easier to read end-to-end and to run
# directly, e.g. from CI); these targets just save typing.
#
# Common overrides (see each script for the full list):
#   INPUT=dataset/pcm/tiny_1frame.pcm
#   CODECS="codec_a codec_b"
#   RISCV_CC=riscv64-linux-gnu-gcc RISCV_ISA=rv64gc RISCV_ABI=lp64d
#   CPU_TYPE=TimingSimpleCPU GEM5_ROOT=/path/to/gem5
#
# Example:
#   make dataset build run-native run-spike compare

SHELL := /bin/bash

.PHONY: all dataset build build-native build-riscv \
        run run-native run-spike run-gem5 \
        inspect compare report clean help

all: build run compare

help:
	@echo "Targets:"
	@echo "  dataset       generate the synthetic PCM dataset (dataset/pcm/)"
	@echo "  build         build native + RISC-V binaries for all codecs"
	@echo "  build-native  build build/<codec>_native"
	@echo "  build-riscv   build build/<codec>_rv64.elf"
	@echo "  run-native    run native encoder, write results/native/"
	@echo "  run-spike     run Spike encoder, write results/spike/"
	@echo "  run-gem5      run gem5 SE-mode encoder, write results/gem5/"
	@echo "  run           run-native + run-spike (NOT run-gem5, it's slow; use 'make run-gem5' explicitly)"
	@echo "  inspect       objdump disassembly + Spike instruction trace (tiny input)"
	@echo "  compare       cmp native vs Spike vs gem5 outputs"
	@echo "  report        print the cycles/frame, instr/frame, IPC, real-time table"
	@echo "  clean         remove build/ and results/ contents (keeps dataset/)"

dataset:
	python3 scripts/gen_dataset.py

build: build-native build-riscv

build-native:
	bash scripts/build_native.sh

build-riscv:
	bash scripts/build_riscv.sh

run: run-native run-spike

run-native:
	bash scripts/run_native.sh

run-spike:
	bash scripts/run_spike.sh

run-gem5:
	bash scripts/run_gem5.sh

inspect:
	bash scripts/inspect_riscv.sh

compare:
	bash scripts/compare_outputs.sh

report:
	python3 scripts/report.py

clean:
	rm -rf build results/native results/spike results/gem5
	mkdir -p build results/native results/spike results/gem5
	touch build/.gitkeep results/native/.gitkeep results/spike/.gitkeep results/gem5/.gitkeep
