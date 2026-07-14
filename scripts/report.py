#!/usr/bin/env python3
"""Performance metric extraction and real-time feasibility report.

Implements workflow steps 7-8:
  7. Performance metric extraction (from gem5 stats.txt)
  8. Cycles/frame and real-time feasibility analysis

Reads:
  - "frames=<n>" from the codec binary's own stdout, captured by
    scripts/run_native.sh / scripts/run_spike.sh into
    results/<runner>/<codec>_<name>_<mode>.log, or by gem5 itself into
    results/gem5/<codec>_<name>_<mode>/simout.
  - simInsts / numCycles / ipc / cache-miss stats from
    results/gem5/<codec>_<name>_<mode>/stats.txt.

Only trust these numbers once scripts/compare_outputs.sh reports that
native, Spike, and gem5 outputs all match (see README.md).

Usage:
  python3 scripts/report.py
  python3 scripts/report.py --input dataset/pcm/tiny_1frame.pcm --clock-mhz 100
  python3 scripts/report.py --codecs codec_a codec_b --modes encode
"""
import argparse
import os
import re
import sys

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

FRAMES_RE = re.compile(r"frames=(\d+)")
SIM_INSTS_RE = re.compile(r"^\s*simInsts\s+(\d+)", re.MULTILINE)
# gem5's exact stat name for retired cycles has varied across CPU models
# and gem5 versions (numCycles, system.cpu.numCycles, ...cpu0.numCycles,
# board.processor.cores0.core.numCycles, etc.) - try a few likely
# candidates before giving up.
NUM_CYCLES_RES = [
    re.compile(r"^\s*system\.cpu\.numCycles\s+(\d+)", re.MULTILINE),
    re.compile(r"^\s*system\.cpu0\.numCycles\s+(\d+)", re.MULTILINE),
    re.compile(r"(?:^|\.)numCycles\s+(\d+)", re.MULTILINE),
]
IPC_RES = [
    re.compile(r"^\s*system\.cpu\.ipc\s+([\d.]+)", re.MULTILINE),
    re.compile(r"(?:^|\.)ipc\s+([\d.]+)", re.MULTILINE),
]
CACHE_LINE_RE = re.compile(r"(icache|dcache|l2|l3).*overallMisses.*?(\d+)", re.IGNORECASE)


def find_first(patterns, text):
    for pat in patterns:
        m = pat.search(text)
        if m:
            return m.group(1)
    return None


def read_text(path):
    try:
        with open(path, "r", errors="replace") as f:
            return f.read()
    except OSError:
        return None


def find_frames(codec, name, mode, runner):
    """runner is 'native', 'spike', or 'gem5'."""
    if runner == "gem5":
        candidates = [
            os.path.join(ROOT_DIR, "results", "gem5", f"{codec}_{name}_{mode}", "simout"),
        ]
    else:
        candidates = [
            os.path.join(ROOT_DIR, "results", runner, f"{codec}_{name}_{mode}.log"),
        ]
    for path in candidates:
        text = read_text(path)
        if text is None:
            continue
        m = FRAMES_RE.search(text)
        if m:
            return int(m.group(1))
    return None


def parse_gem5_stats(codec, name, mode):
    stats_path = os.path.join(ROOT_DIR, "results", "gem5", f"{codec}_{name}_{mode}", "stats.txt")
    text = read_text(stats_path)
    if text is None:
        return None

    sim_insts = None
    m = SIM_INSTS_RE.search(text)
    if m:
        sim_insts = int(m.group(1))

    num_cycles_str = find_first(NUM_CYCLES_RES, text)
    num_cycles = int(num_cycles_str) if num_cycles_str else None

    ipc_str = find_first(IPC_RES, text)
    ipc = float(ipc_str) if ipc_str else None

    cache_lines = []
    for line in text.splitlines():
        if CACHE_LINE_RE.search(line):
            cache_lines.append(line.strip())

    return {
        "sim_insts": sim_insts,
        "num_cycles": num_cycles,
        "ipc": ipc,
        "cache_lines": cache_lines,
        "stats_path": stats_path,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--input", default="dataset/pcm/tiny_1frame.pcm",
                         help="dataset PCM file the runs were performed on")
    parser.add_argument("--codecs", nargs="+", default=["codec_a", "codec_b", "codec_c"])
    parser.add_argument("--modes", nargs="+", default=["encode", "decode"])
    parser.add_argument("--clock-mhz", type=float, default=100.0,
                         help="target clock frequency in MHz for the real-time feasibility check")
    parser.add_argument("--frame-ms", type=float, default=None,
                         help="frame duration in ms (if omitted, only cycles/instrs per frame are reported, "
                              "not real-time feasibility, since frame duration is codec-specific)")
    parser.add_argument("--show-cache-lines", action="store_true",
                         help="also print raw cache-miss stat lines found in stats.txt")
    args = parser.parse_args()

    name = os.path.basename(args.input)
    if name.endswith(".pcm"):
        name = name[: -len(".pcm")]

    rows = []
    for codec in args.codecs:
        for mode in args.modes:
            frames = (
                find_frames(codec, name, mode, "gem5")
                or find_frames(codec, name, mode, "spike")
                or find_frames(codec, name, mode, "native")
            )
            gem5_stats = parse_gem5_stats(codec, name, mode)

            row = {
                "codec": codec,
                "mode": mode,
                "frames": frames,
                "sim_insts": None,
                "num_cycles": None,
                "ipc": None,
                "instr_per_frame": None,
                "cycles_per_frame": None,
                "realtime_ok": None,
                "cache_lines": [],
            }

            if gem5_stats:
                row["sim_insts"] = gem5_stats["sim_insts"]
                row["num_cycles"] = gem5_stats["num_cycles"]
                row["ipc"] = gem5_stats["ipc"]
                row["cache_lines"] = gem5_stats["cache_lines"]

                if frames and gem5_stats["sim_insts"]:
                    row["instr_per_frame"] = gem5_stats["sim_insts"] / frames
                if frames and gem5_stats["num_cycles"]:
                    row["cycles_per_frame"] = gem5_stats["num_cycles"] / frames

                if row["cycles_per_frame"] and args.frame_ms:
                    available_cycles_per_frame = args.clock_mhz * 1e6 * (args.frame_ms / 1000.0)
                    usage = row["cycles_per_frame"] / available_cycles_per_frame
                    row["realtime_ok"] = usage <= 1.0
                    row["cpu_usage"] = usage

            rows.append(row)

    header = ["codec", "mode", "frames", "instr/frame", "cycles/frame", "ipc",
               f"cpu%@{args.clock_mhz:g}MHz", "real-time?"]
    col_widths = [10, 8, 8, 14, 14, 8, 14, 11]

    def fmt_row(cells):
        return "  ".join(str(c).ljust(w) for c, w in zip(cells, col_widths))

    print(fmt_row(header))
    print(fmt_row(["-" * w for w in col_widths]))
    for row in rows:
        instr_pf = f"{row['instr_per_frame']:.0f}" if row["instr_per_frame"] else "n/a"
        cyc_pf = f"{row['cycles_per_frame']:.0f}" if row["cycles_per_frame"] else "n/a"
        ipc = f"{row['ipc']:.2f}" if row["ipc"] else "n/a"
        cpu_pct = f"{row['cpu_usage'] * 100:.1f}%" if row.get("cpu_usage") is not None else "n/a"
        rt = ("yes" if row["realtime_ok"] else "no") if row["realtime_ok"] is not None else "n/a"
        print(fmt_row([row["codec"], row["mode"], row["frames"] if row["frames"] is not None else "n/a",
                        instr_pf, cyc_pf, ipc, cpu_pct, rt]))

        if args.show_cache_lines and row["cache_lines"]:
            for line in row["cache_lines"]:
                print(f"    {line}")

    if not args.frame_ms:
        print()
        print("Note: pass --frame-ms <ms> (this codec's frame duration) to also compute")
        print("      real-time feasibility at --clock-mhz (default 100 MHz).")


if __name__ == "__main__":
    sys.exit(main())
