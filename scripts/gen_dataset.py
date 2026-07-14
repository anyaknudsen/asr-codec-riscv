#!/usr/bin/env python3
"""Generates the synthetic PCM test dataset used across native, Spike,
and gem5 runs.

We don't ship real speech recordings in this repo. This script
generates small, deterministic, headerless mono 16 kHz signed 16-bit
little-endian PCM files covering the input categories called out in
README.md (silence, tone/speech-like, noise, mixed, high/low energy),
plus a set of fixed-size files (tiny/small/medium/long) meant for
progressively heavier correctness and performance runs.

If you have real speech recordings instead, convert them with ffmpeg
and drop the results into dataset/pcm/ directly - see README.md
"Dataset" section - you do not need this script for real recordings.

Usage:
  python3 scripts/gen_dataset.py
  python3 scripts/gen_dataset.py --out-dir dataset/pcm --sample-rate 16000
"""
import argparse
import math
import os
import random
import struct

SAMPLE_RATE = 16000
FRAME_MS = 20
FRAME_SAMPLES = SAMPLE_RATE * FRAME_MS // 1000  # 320


def write_pcm(path, samples):
    with open(path, "wb") as f:
        f.write(struct.pack("<%dh" % len(samples), *samples))


def clip16(x):
    return max(-32768, min(32767, int(round(x))))


def silence(num_samples):
    return [0] * num_samples


def tone(num_samples, freq_hz=220.0, amplitude=8000, sample_rate=SAMPLE_RATE):
    return [
        clip16(amplitude * math.sin(2 * math.pi * freq_hz * i / sample_rate))
        for i in range(num_samples)
    ]


def speech_like(num_samples, sample_rate=SAMPLE_RATE, seed=0, amplitude=6000):
    """A crude "speech-like" signal: a slowly varying fundamental with a
    couple of harmonics and a slow amplitude envelope, roughly mimicking
    voiced speech's quasi-periodic structure without being real speech."""
    rng = random.Random(seed)
    f0 = 140.0  # a plausible voiced-speech pitch, Hz
    jitter = rng.uniform(-5, 5)
    out = []
    for i in range(num_samples):
        t = i / sample_rate
        envelope = 0.5 + 0.5 * math.sin(2 * math.pi * 2.0 * t)
        f = f0 + jitter * math.sin(2 * math.pi * 4.0 * t)
        sample = (
            0.6 * math.sin(2 * math.pi * f * t)
            + 0.3 * math.sin(2 * math.pi * 2 * f * t)
            + 0.1 * math.sin(2 * math.pi * 3 * f * t)
        )
        out.append(clip16(amplitude * envelope * sample))
    return out


def noise(num_samples, amplitude=4000, seed=0):
    rng = random.Random(seed)
    return [clip16(rng.uniform(-amplitude, amplitude)) for _ in range(num_samples)]


def mixed(num_samples, sample_rate=SAMPLE_RATE, seed=0):
    speech = speech_like(num_samples, sample_rate=sample_rate, seed=seed, amplitude=5000)
    noisy = noise(num_samples, amplitude=1200, seed=seed + 1)
    return [clip16(a + b) for a, b in zip(speech, noisy)]


def high_energy(num_samples, sample_rate=SAMPLE_RATE, seed=0):
    return speech_like(num_samples, sample_rate=sample_rate, seed=seed, amplitude=16000)


def low_energy(num_samples, sample_rate=SAMPLE_RATE, seed=0):
    return speech_like(num_samples, sample_rate=sample_rate, seed=seed, amplitude=600)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out-dir", default="dataset/pcm")
    parser.add_argument("--sample-rate", type=int, default=SAMPLE_RATE)
    args = parser.parse_args()

    os.makedirs(args.out_dir, exist_ok=True)
    sr = args.sample_rate

    def frames_to_samples(n):
        return n * (sr * FRAME_MS // 1000)

    def seconds_to_samples(s):
        return int(sr * s)

    sized_files = {
        "tiny_1frame.pcm": frames_to_samples(1),
        "small_10frames.pcm": frames_to_samples(10),
        "medium_1sec.pcm": seconds_to_samples(1),
        "long_10sec.pcm": seconds_to_samples(10),
    }
    for name, count in sized_files.items():
        write_pcm(os.path.join(args.out_dir, name), speech_like(count, sample_rate=sr, seed=1))

    category_files = {
        "category_silence.pcm": silence(seconds_to_samples(1)),
        "category_single_speaker.pcm": speech_like(seconds_to_samples(1), sample_rate=sr, seed=2),
        "category_multi_speaker.pcm": mixed(seconds_to_samples(1), sample_rate=sr, seed=3),
        "category_high_energy.pcm": high_energy(seconds_to_samples(1), sample_rate=sr, seed=4),
        "category_low_energy.pcm": low_energy(seconds_to_samples(1), sample_rate=sr, seed=5),
        "category_noisy.pcm": noise(seconds_to_samples(1), amplitude=9000, seed=6),
    }
    for name, samples in category_files.items():
        write_pcm(os.path.join(args.out_dir, name), samples)

    sample_files = {
        "sample_001_16k_s16le.pcm": speech_like(seconds_to_samples(2), sample_rate=sr, seed=7),
        "sample_002_16k_s16le.pcm": mixed(seconds_to_samples(2), sample_rate=sr, seed=8),
    }
    for name, samples in sample_files.items():
        write_pcm(os.path.join(args.out_dir, name), samples)

    all_files = {**sized_files, **{k: len(v) for k, v in category_files.items()},
                 **{k: len(v) for k, v in sample_files.items()}}
    frame_samples = sr * FRAME_MS // 1000
    print(f"Wrote {len(all_files)} PCM files to {args.out_dir}/ at {sr} Hz, mono, s16le:")
    for name in sorted(all_files):
        count = all_files[name]
        print(f"  {name:32s} {count:8d} samples  ({count / sr * 1000:.0f} ms, {count / frame_samples:.1f} frames @ {FRAME_MS} ms/frame)")


if __name__ == "__main__":
    main()
