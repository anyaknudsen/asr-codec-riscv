# Dataset

This project's codecs are exercised with **raw, headerless, mono, 16 kHz,
signed 16-bit little-endian PCM** (`s16le`). That format is what
`app/main.c` and `common/pcm.c` read/write, and what gets passed as the
`<input_path>`/`<output_path>` command-line arguments to every codec
binary.

```
dataset/
  raw/        original audio (.wav, if/when you add real recordings)
  pcm/        converted raw s16le PCM - the actual files passed to the
              native, Spike, and gem5 runs
  expected/   optional reference outputs (e.g. sample_001.ref.bit),
              populated once a codec is implemented and you want a
              "known good" bitstream to diff future changes against
```

## Converting real recordings (dataset/raw -> dataset/pcm)

If you add real speech recordings to `dataset/raw/` (e.g. `.wav`
files), convert them to the dataset format with `ffmpeg`:

```bash
ffmpeg -i dataset/raw/speaker1_sentence1.wav \
  -ac 1 \
  -ar 16000 \
  -f s16le \
  -acodec pcm_s16le \
  dataset/pcm/speaker1_sentence1_16k_s16le.pcm
```

- `-ac 1` mono
- `-ar 16000` 16 kHz sampling rate
- `-f s16le` signed 16-bit little-endian raw PCM (no WAV header)

Use the **same** `dataset/pcm/*.pcm` file for the native, Spike, and
gem5 runs of a given comparison so outputs are directly comparable
(see `scripts/compare_outputs.sh`).

## Synthetic dataset (dataset/pcm, generated)

Real speech recordings aren't checked into this repo. Instead,
`scripts/gen_dataset.py` generates a small, deterministic, synthetic
PCM dataset that exercises the same input categories a real evaluation
would use. Regenerate it any time with:

```bash
python3 scripts/gen_dataset.py
```

| File | Duration | Frames (20 ms) | Content |
| --- | --- | --- | --- |
| `tiny_1frame.pcm` | 20 ms | 1 | speech-like tone, for Spike trace / gem5 smoke runs |
| `small_10frames.pcm` | 200 ms | 10 | speech-like tone, default for native/Spike correctness runs |
| `medium_1sec.pcm` | 1 s | 50 | speech-like tone |
| `long_10sec.pcm` | 10 s | 500 | speech-like tone, native/Spike only - too slow for routine gem5 runs |
| `category_silence.pcm` | 1 s | 50 | all-zero samples |
| `category_single_speaker.pcm` | 1 s | 50 | one speech-like voice |
| `category_multi_speaker.pcm` | 1 s | 50 | speech-like voice + low-level noise |
| `category_high_energy.pcm` | 1 s | 50 | speech-like voice, large amplitude |
| `category_low_energy.pcm` | 1 s | 50 | speech-like voice, small amplitude |
| `category_noisy.pcm` | 1 s | 50 | uniform random noise, no tonal structure |
| `sample_001_16k_s16le.pcm` | 2 s | 100 | speech-like voice |
| `sample_002_16k_s16le.pcm` | 2 s | 100 | speech-like voice + low-level noise |

All files: mono, 16 kHz, signed 16-bit little-endian PCM, generated
with fixed random seeds (fully deterministic/reproducible).

"Speech-like" means a synthesized quasi-periodic signal (a fundamental
plus a couple of harmonics under a slow amplitude envelope) that
exercises pitch-periodic and time-varying-energy codec behavior
without being a real recording - it is **not** real speech. Replace it
with real recordings under `dataset/raw/` + the `ffmpeg` conversion
above whenever you want representative quality/compression numbers;
keep the synthetic set for cheap, reproducible correctness/performance
smoke-testing (especially under gem5, where simulation is slow).

## Why more than one input file

Speech codec workload can vary a lot depending on pitch search, VAD,
LPC analysis, entropy coding, mode decisions, and filters, all of which
are signal-dependent. A single input file is not a fair performance
comparison. At minimum, run each codec across the `category_*.pcm`
files above (silence, single-speaker, multi-speaker/noisy-mixed,
high-energy, low-energy, noisy) in addition to the sized files, and
report results per input (see the reporting table in the top-level
`README.md`).
