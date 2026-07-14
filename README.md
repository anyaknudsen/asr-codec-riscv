# asr-codec-riscv

A single repository for evaluating three speech codecs on RISC-V:
first for **correctness** (native C vs. the RISC-V binary running under
the Spike ISA simulator), then for **performance** (the same RISC-V
binary running under the gem5 architectural simulator).

One repo, three codec folders, one shared dataset, one shared build
system, one Spike workflow, one gem5 workflow, one comparison/reporting
format. All three codecs are treated identically by every script below.

```
speech-codec-riscv/            (this repo, package name asr-codec-riscv)
  app/
    main.c                     CLI entry point (encode/decode dispatch only)

  common/
    io.c / io.h                generic file I/O helpers
    pcm.c / pcm.h               PCM (s16le) helpers, frame handling
    codec_api.h                 shared encode_file()/decode_file() interface

  codecs/
    codec_a/  encoder.c decoder.c codec.h   (not yet implemented, see below)
    codec_b/  encoder.c decoder.c codec.h   (not yet implemented, see below)
    codec_c/  encoder.c decoder.c codec.h   (not yet implemented, see below)

  dataset/
    raw/                        original recordings (.wav), if you add any
    pcm/                        converted/synthetic raw s16le PCM inputs
    expected/                    optional reference outputs
    README.md                   dataset documentation

  build/                        generated binaries (gitignored contents)
  results/
    native/                     native run outputs + logs
    spike/                      Spike run outputs + logs + asm/trace
    gem5/                       gem5 run outputs + stats.txt per run

  scripts/
    build_native.sh              build build/<codec>_native
    build_riscv.sh                build build/<codec>_rv64.elf
    run_native.sh                  run native encode+decode
    run_spike.sh                    run Spike encode+decode (via riscv-pk)
    run_gem5.sh                      run gem5 SE-mode encode+decode + stats
    inspect_riscv.sh                 objdump + Spike instruction trace (tiny input)
    compare_outputs.sh                native vs Spike vs gem5 diff
    gen_dataset.py                    generate the synthetic PCM dataset
    report.py                         cycles/frame, instr/frame, IPC, real-time table

  Makefile                       thin wrapper around scripts/
```

## Status: codec algorithms are not implemented yet

`codecs/codec_a`, `codecs/codec_b`, and `codecs/codec_c` currently
contain **outline/skeleton files only** - `codec.h`, `encoder.c`, and
`decoder.c` with the shared interface wired up and `TODO(codec_x)`
comments describing what goes where, but `encode_file()`/`decode_file()`
just print "not implemented yet" and return failure. This is
intentional: the codec algorithms themselves are implemented separately
(see the `TODO` comments in each file for the expected structure).

Everything else in this repo - the shared I/O/PCM helpers, the CLI, the
build scripts, the Spike workflow, the gem5 workflow, the dataset, and
the comparison/reporting tooling - **is implemented and has been tested
end-to-end** against the stub codecs: native and RISC-V builds succeed
for all three codecs; `scripts/run_native.sh`, `scripts/run_spike.sh`,
and `scripts/run_gem5.sh` all correctly run the RISC-V binary (under
Spike+`pk` and under gem5 SE mode respectively) and surface the same
"not implemented yet" failure the native binary reports, proving the
full toolchain plumbing works, including real dataset file I/O (not
just console output) and gem5 stats collection (`scripts/report.py`
correctly extracts `simInsts`/`numCycles`/`ipc`/cache-miss lines from a
real `stats.txt`). Once `encode_file()`/`decode_file()` are filled in
for a codec, the entire pipeline below should work for it without any
script changes.

## The three codecs

`codecs/codec_a/`, `codecs/codec_b/`, and `codecs/codec_c/` are meant to
hold three different speech codec implementations (e.g. different
algorithms, or fixed-point vs. floating-point variants). They share:

- the same dataset (`dataset/pcm/*.pcm`)
- the same build scripts and compiler flags
- the same Spike workflow
- the same gem5 workflow
- the same results/reporting format

which is precisely what makes a one-repo, three-folder layout better
here than three separate repos: the codecs are part of the same
performance comparison, not unrelated projects.

Each codec implements exactly two functions, declared in
`common/codec_api.h`:

```c
int encode_file(const char *input_path, const char *output_path);
int decode_file(const char *input_path, const char *output_path);
```

`app/main.c` is linked directly against exactly one codec's
`encoder.c`/`decoder.c` per binary (see `scripts/build_native.sh` /
`scripts/build_riscv.sh`), so `main.c` never needs to know which codec
it was built with - it only parses `<encode|decode> <in> <out>`, calls
the matching function, prints `frames=<n>`, and returns success/failure.
`codecs/<name>/codec.h` is where that codec's own constants (sample
rate, frame size, bitstream magic, quantizer tables, etc.) belong. Fill
in the `TODO(codec_x)` markers in `codec.h`/`encoder.c`/`decoder.c` to
implement each codec; do not change the `encode_file()`/`decode_file()`
signatures, since the whole build/run pipeline depends on that contract
staying stable.

## Dataset

See `dataset/README.md` for full details. In short: codecs read/write
raw, headerless, mono, 16 kHz, signed 16-bit little-endian PCM
(`s16le`). `scripts/gen_dataset.py` generates a small synthetic PCM
dataset (tiny/small/medium/long sizes, plus silence/speech/noise/mixed
category files) so the whole pipeline is exercisable without needing
real recordings:

```bash
python3 scripts/gen_dataset.py
```

To use real recordings instead, drop `.wav` files into `dataset/raw/`
and convert them with `ffmpeg`:

```bash
ffmpeg -i dataset/raw/speaker1_sentence1.wav \
  -ac 1 -ar 16000 -f s16le -acodec pcm_s16le \
  dataset/pcm/speaker1_sentence1_16k_s16le.pcm
```

Use the **same** `dataset/pcm/*.pcm` file across the native, Spike, and
gem5 runs of a given comparison - that's what makes their outputs
directly comparable.

## The workflow

```
1. Native C reference run
2. RISC-V cross-compilation
3. Spike correctness run
4. Spike/objdump instruction-level inspection
5. gem5 performance simulation
6. Output comparison
7. Performance metric extraction
8. Cycles/frame and real-time feasibility analysis
9. Repeat for all three codecs, both encode and decode
```

Native run:

```
C source code -> normal compiler -> native executable -> dataset -> reference output
```

Spike run:

```
C source code -> RISC-V compiler -> RISC-V ELF -> Spike -> same dataset -> RISC-V output
```

gem5 run:

```
C source code -> RISC-V compiler -> RISC-V ELF -> gem5 CPU/memory model -> same dataset -> output + performance stats
```

**Spike** is the golden functional RISC-V ISA simulator: it checks
whether the RISC-V binary *behaves correctly*, but does not model
caches, pipelines, branch prediction, memory stalls, or energy, so it
is not a performance tool. **gem5** comes after Spike, once
correctness is established, and adds simulated cycles, instructions,
IPC, and cache/memory behavior on top of a binary Spike has already
validated.

### 1-2. Build

```bash
bash scripts/build_native.sh   # -> build/codec_{a,b,c}_native
bash scripts/build_riscv.sh    # -> build/codec_{a,b,c}_rv64.elf
```

or via the Makefile: `make build` (or `make build-native` /
`make build-riscv` individually).

### 3. Native + Spike correctness runs

```bash
bash scripts/run_native.sh     # -> results/native/*
bash scripts/run_spike.sh      # -> results/spike/*
```

Both default to `INPUT=dataset/pcm/small_10frames.pcm`; override with
`INPUT=dataset/pcm/<name>.pcm bash scripts/run_native.sh`, etc. See
"RISC-V toolchain setup" below for what Spike run needs installed
(`spike`, `pk`).

### 4. Instruction-level inspection (Spike/objdump)

```bash
bash scripts/inspect_riscv.sh   # -> results/spike/<codec>_rv64.asm, *.trace
```

Only run this on tiny inputs (default `dataset/pcm/tiny_1frame.pcm`) -
Spike instruction traces get huge fast. Use it to sanity-check what the
compiler generated: which functions dominate, whether
multiply/divide/floating-point/compressed instructions appear, and
whether the inner loops look reasonable. This step is *not* a
performance measurement.

### 5. gem5 performance simulation

```bash
GEM5_ROOT=/path/to/gem5 bash scripts/run_gem5.sh   # -> results/gem5/<run>/{*.bit,*.pcm,stats.txt,...}
```

Defaults to `INPUT=dataset/pcm/tiny_1frame.pcm` (gem5 simulation is
much slower than Spike - start tiny). See "gem5 setup" below.

### 6. Output comparison

```bash
bash scripts/compare_outputs.sh
```

Checks `native == Spike == gem5` for both the encoded `.bit` and the
decoded `.pcm`, for every codec. Prints `All outputs match` if so. Only
trust performance numbers (step 7-8) once this passes. Pass
`WITH_GEM5=0` to compare native vs. Spike only (e.g. before you've run
gem5 yet).

### 7-8. Performance metrics + real-time feasibility

```bash
python3 scripts/report.py --frame-ms 20 --clock-mhz 100
```

Reads each codec's own `frames=<n>` summary line together with gem5's
`stats.txt` (`simInsts`, cycle count, IPC) and computes
instructions/frame, cycles/frame, and (given `--frame-ms`, the codec's
frame duration, and `--clock-mhz`, a hypothetical target clock)
whether the codec fits in real time:

```
available_cycles_per_frame = clock_hz * frame_duration_seconds
cpu_usage = measured_cycles_per_frame / available_cycles_per_frame
```

`cpu_usage <= 1` means it fits in real time at that clock, ignoring OS
and I/O overhead. The exact gem5 stat names vary by CPU model/version;
`scripts/report.py` tries a few common names and also supports
`--show-cache-lines` to print any `icache`/`dcache`/`l2`/`l3`
miss-count lines it finds verbatim, matching:

```bash
grep -E "simInsts|numCycles|ipc|cpi|icache|dcache" results/gem5/<run>/stats.txt
```

### 9. Repeat

Repeat steps 3-8 for `codec_a`, `codec_b`, `codec_c`, for both `encode`
and `decode`, and for each dataset input you care about (see
`dataset/README.md` - don't judge codecs on a single input file, since
codec workload is signal-dependent). Also repeat across build variants
you want to compare: `-O2` vs `-O3`, `rv64gc` vs `rv64imac`, different
gem5 cache sizes, `TimingSimpleCPU` vs another gem5 CPU model, etc. -
every script here reads its build/ISA/CPU knobs from environment
variables for exactly this reason (see each script's header comment).

## Target comparison table

Once codecs are implemented and the above has been run for each
input/codec/mode, the report this project is meant to produce looks
like:

```
Codec     Mode     Output correct?   Frames   Cycles/frame   Instr/frame   IPC   Real-time at 100 MHz?
codec_a   encode   yes               50       500000         800000        1.6   yes
codec_a   decode   yes               50       ...            ...           ...   ...
codec_b   encode   yes               50       900000         1300000       1.4   yes
codec_b   decode   yes               50       ...            ...           ...   ...
codec_c   encode   yes               50       2400000        3000000       1.25  no
codec_c   decode   yes               50       ...            ...           ...   ...
```

or, per input, per ISA/opt-level variant:

```
Input              Frames   Codec     Mode     ISA      Opt   Instr/frame   Cycles/frame   IPC   D$ misses/frame   Real-time at 100 MHz?
tiny_1frame         1       codec_a   encode   rv64gc   -O2   ...           ...            ...   ...               yes/no
small_10frames      10      codec_a   encode   rv64gc   -O2   ...           ...            ...   ...               yes/no
medium_1sec          50      codec_a   encode   rv64gc   -O2   ...           ...            ...   ...               yes/no
```

## RISC-V toolchain setup

These scripts expect the following to be on `PATH` (not installed by
default on a plain dev machine - see below for how each was built and
verified for this project):

| Tool | Purpose | Used by |
| --- | --- | --- |
| `riscv64-linux-gnu-gcc`, `riscv64-linux-gnu-objdump` | RISC-V cross-compiler/objdump (glibc target) | `scripts/build_riscv.sh`, `scripts/inspect_riscv.sh` |
| `spike` | RISC-V ISA simulator | `scripts/run_spike.sh`, `scripts/inspect_riscv.sh` |
| `pk` (riscv-pk) | proxy kernel providing file I/O syscalls to Spike | `scripts/run_spike.sh`, `scripts/inspect_riscv.sh` |
| `gem5.opt` | gem5 simulator binary | `scripts/run_gem5.sh` |

### Why `riscv64-linux-gnu-gcc` (glibc) instead of a bare-metal `riscv64-unknown-elf-gcc` toolchain

Both were tried. `riscv-pk` (`pk`) implements the standard **Linux
RV64 syscall ABI** over `ecall` (its `syscall.h` uses the same syscall
numbers as the Linux kernel: `SYS_write=64`, `SYS_openat=56`,
`SYS_exit=93`, ...). A statically-linked `riscv64-linux-gnu-gcc`
(glibc) binary issues exactly that syscall ABI, so `spike pk
./binary` "just works" for real file I/O (`fopen`/`fread`/`fwrite` on
actual dataset files, not just console output) - this was verified
directly in this environment. The distro's `riscv64-unknown-elf-gcc`
package, by contrast, ships **picolibc**, whose default (and
"hosted"/"semihost") C library variants on this distro do not wire up
`stdout`/file syscalls in a way compatible with `pk` out of the box
(there is no ready-made picolibc backend here that speaks `pk`'s
syscall ABI for real file I/O - only a `libdummyhost.a` with `_exit()`
and no-op console stubs). Getting real file I/O through that toolchain
would require either building `riscv-newlib`/`libgloss` from source (it
does implement `pk`-compatible syscalls) or writing a custom syscall
shim - out of scope here given `riscv64-linux-gnu-gcc` already works
end-to-end.

This also constrains the ISA/ABI sweep described in "Compiler/ISA
variants to compare" below: this distro's `riscv64-linux-gnu-gcc` ships
only the `lp64d` glibc multilib (no integer-only `lp64` ABI), so an
`rv64imac`/`lp64` (integer-only) build needs either a
`riscv64-linux-gnu-gcc` with additional multilibs installed, or the
`riscv64-unknown-elf-gcc` + picolibc route above with its file-I/O
caveat.

### Install the RISC-V cross-compiler (Ubuntu/Debian)

```bash
sudo apt-get install -y gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

### Build Spike (riscv-isa-sim) from source

Not packaged on Ubuntu/Debian; build it:

```bash
sudo apt-get install -y device-tree-compiler libboost-all-dev autoconf automake libtool

git clone --depth 1 https://github.com/riscv-software-src/riscv-isa-sim.git
cd riscv-isa-sim && mkdir build && cd build
../configure --prefix=/opt/riscv-tools/install
make -j"$(nproc)" && make install
```

### Build riscv-pk from source

`riscv-pk` is a small, freestanding (bare-metal) program built with the
RISC-V *ELF* cross-compiler (`riscv64-unknown-elf-gcc`, not the glibc
one above - `pk` needs its own minimal freestanding toolchain
regardless of which toolchain later compiles your actual codec
binaries) - it does not link against any target C library, it
implements its own:

```bash
sudo apt-get install -y gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf picolibc-riscv64-unknown-elf

git clone --depth 1 https://github.com/riscv-software-src/riscv-pk.git
cd riscv-pk && mkdir build && cd build
CC="riscv64-unknown-elf-gcc -I/usr/lib/picolibc/riscv64-unknown-elf/include" \
CFLAGS="-nostdlib -nostartfiles" \
  ../configure --prefix=/opt/riscv-tools/install --host=riscv64-unknown-elf \
  --with-arch=rv64imafdc_zicsr_zifencei --with-abi=lp64d
make -j"$(nproc)" && make install
```

(The extra include path and `--with-arch`/`--with-abi` are needed
because this distro's `riscv64-unknown-elf-gcc` package only ships a
reduced multilib set and needs `<string.h>`/`<sys/stat.h>` declarations
from picolibc purely for `pk`'s own header includes - `pk` still links
`-nostdlib`, it does not use picolibc's compiled library.)

Add both install locations to `PATH`:

```bash
export PATH=/opt/riscv-tools/install/bin:/opt/riscv-tools/install/riscv64-unknown-elf/bin:$PATH
```

### Verify Spike + pk

```bash
spike pk build/codec_a_rv64.elf encode dataset/pcm/tiny_1frame.pcm /tmp/out.bit
```

(Will currently print `codec_a: encode_file() is not implemented yet`
and exit 1 - that's the stub codec correctly running on the RISC-V ISA
simulator, not a toolchain problem. See "Status" above.)

## gem5 setup

Not packaged; build the RISC-V target from source (this is the biggest
of the three toolchain builds - budget real time and disk space for it):

```bash
sudo apt-get install -y scons python3-dev libboost-dev protobuf-compiler \
    libprotobuf-dev libgoogle-perftools-dev libpng-dev

git clone --depth 1 --branch v24.1.0.3 https://github.com/gem5/gem5.git
cd gem5
scons build/RISCV/gem5.opt -j"$(nproc)"
```

Then point `scripts/run_gem5.sh` at it:

```bash
export PATH=/path/to/gem5/build/RISCV:$PATH   # for gem5.opt, or use GEM5=/path/to/gem5.opt
export GEM5_ROOT=/path/to/gem5                # for configs/deprecated/example/se.py
bash scripts/run_gem5.sh
```

`scripts/run_gem5.sh` uses `configs/deprecated/example/se.py`
(syscall-emulation mode) with `--cpu-type`, `--caches`, `--l1i_size`,
`--l1d_size`, and `--mem-size`, matching the exact options this
project's target gem5 version (`v24.1.0.3`) still exposes. See the
script header for all overridable environment variables
(`CPU_TYPE`, `MEM_SIZE`, `L1I_SIZE`, `L1D_SIZE`, `CACHES=0` to disable
caches for e.g. `AtomicSimpleCPU`).

## Compiler/ISA variants to compare

Every build script knob below is an environment variable, so sweeps
are just repeated invocations with different env vars (see each
script's header comment for the full list):

```bash
OPT=-O3 bash scripts/build_native.sh
OPT=-O3 RISCV_ISA=rv64gc RISCV_ABI=lp64d bash scripts/build_riscv.sh
RISCV_ISA=rv64imac RISCV_ABI=lp64 bash scripts/build_riscv.sh   # integer-only ABI, see toolchain caveat above
CPU_TYPE=AtomicSimpleCPU CACHES=0 bash scripts/run_gem5.sh
L1D_SIZE=64kB L1I_SIZE=64kB bash scripts/run_gem5.sh
```

Compare, at minimum: `-O0`/`-O2`/`-O3`, `rv64gc` vs `rv64imac`, `rv32*`
vs `rv64*`, floating-point vs. fixed-point codecs, with/without gem5
caches, different L1 sizes, different gem5 CPU models, encoder vs.
decoder, and `codec_a` vs. `codec_b` vs. `codec_c`. Always keep
`-ffp-contract=off` (already the default in both build scripts) when
comparing floating-point codecs across native and RISC-V - see the
portability note below.

## Common C portability issues (native -> RISC-V)

Watch for these when implementing/porting a codec, since native and
RISC-V output silently diverging is the failure mode
`scripts/compare_outputs.sh` exists to catch:

- **Signed integer overflow** (undefined behavior; may optimize
  differently per target).
- **`char` signedness** assumptions (plain `char` is signed on x86-64,
  and signed by default on this project's RISC-V targets too, but
  don't rely on it - use `unsigned char`/`int8_t` explicitly for byte
  buffers, as `common/io.c`/`common/pcm.c` do).
- **Unaligned pointer casts** (e.g. reinterpreting a `uint8_t*` buffer
  as `int16_t*`) - `common/pcm.c` deliberately decodes/encodes s16le
  samples byte-by-byte instead of casting, to avoid both alignment and
  endianness assumptions.
- **Endianness** - both x86-64 and this project's RISC-V targets are
  little-endian, but the dataset format is defined as little-endian
  explicitly (not "native-endian") so the code stays correct even if
  that changes.
- **Floating-point differences**, specifically fused multiply-add
  contraction: RISC-V's `D`/`F` extensions have a native `fmadd`
  instruction; x86-64 without `-mfma` does not. If a compiler is free
  to fuse `a*b+c` into one rounding step on one target and two rounding
  steps on the other, native and RISC-V floating-point codecs can
  produce bit-different (but equally "correct") output. Both build
  scripts pass `-ffp-contract=off` to prevent this - keep it if you
  add floating-point code.
- **Non-standard/host-only libraries.**
- **File I/O assumptions** (path separators, text vs. binary mode -
  always open PCM/bitstream files in binary mode, as
  `common/io.c` does).
- **Word-size / pointer-size assumptions** (avoid casting pointers to
  `int`; this project's RISC-V target is 64-bit like the typical build
  host, but don't assume that in general).

## Repository conventions

- `app/main.c` never contains codec algorithm code - only argument
  parsing, calling the codec's `encode_file()`/`decode_file()`, and
  printing the `frames=<n>` summary line other tooling
  (`scripts/report.py`) parses back out.
- `common/` never contains codec-specific logic - only generic file
  I/O (`io.c`/`io.h`) and generic PCM handling (`pcm.c`/`pcm.h`).
- Each `codecs/<name>/` folder owns its own bitstream format, internal
  helpers, and constants; nothing outside that folder should need to
  know about them beyond the shared `encode_file()`/`decode_file()`
  contract in `common/codec_api.h`.
