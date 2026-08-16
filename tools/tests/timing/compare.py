#!/usr/bin/env python3
"""Compare timing-test rows from the harness against the reference.

Reads the 8-digit hex values the test streams over the serial port, prints a
row-by-row comparison, and — when a baseline exists — fails if any row has
moved away from it.

The reference is what a row *should* be. The baseline is what Rigel currently
produces, so a run that matches it passes even where Rigel is still wrong:
this is a ratchet against regression, not a demand that everything be right.
"""
import argparse
import json
import re
import sys

HEX_ROW = re.compile(r"^\[SERIAL\]\s+([0-9A-Fa-f]{8})\s*$")


def read_rows(path, count):
    """Pull the first `count` hex values out of a harness log."""
    rows = []
    with open(path, encoding="utf-8", errors="replace") as fh:
        for line in fh:
            m = HEX_ROW.match(line.strip())
            if m:
                rows.append(int(m.group(1), 16))
                if len(rows) == count:
                    break
    return rows


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("log", help="harness output containing the [SERIAL] rows")
    ap.add_argument("reference", help="reference.json")
    ap.add_argument("--baseline", help="baseline.json to gate on")
    ap.add_argument("--write-baseline", help="record this run as the baseline")
    ap.add_argument("--tolerance", type=float, default=0.02,
                    help="fractional drift from the baseline that still passes")
    args = ap.parse_args()

    with open(args.reference, encoding="utf-8") as fh:
        ref = json.load(fh)

    spec = ref["rows"]
    raw = set(ref.get("raw_rows", []))
    rows = read_rows(args.log, len(spec))

    if len(rows) < len(spec):
        print(f"FAIL: got {len(rows)} rows, expected {len(spec)}")
        print("      the test streams them over serial; did it boot?")
        return 1

    baseline = None
    if args.baseline:
        try:
            with open(args.baseline, encoding="utf-8") as fh:
                baseline = json.load(fh)["rows"]
        except FileNotFoundError:
            print(f"note: no baseline at {args.baseline}; reporting only")

    print(f"{'row':>3} {'what':14} {'rigel':>8} {'ref':>8} {'ratio':>6}  {'vs base':>9}")
    regressed = []
    for i, entry in enumerate(spec):
        got = rows[i]
        want = entry["fsuae"]
        name = entry["name"]

        if i in raw:
            print(f"{i:>3} {name:14} {got:>8X} {want:>8X} {'raw':>6}  {'':>9}")
            continue

        ratio = got / want if want else 0.0
        drift = ""
        if baseline is not None:
            was = baseline[i]
            if was:
                d = (got - was) / was
                drift = f"{d:+7.1%}"
                if abs(d) > args.tolerance:
                    regressed.append((name, i, was, got))
            else:
                drift = "   n/a"

        print(f"{i:>3} {name:14} {got:>8} {want:>8} {ratio:>6.2f}  {drift:>9}")

    # Worst offenders against the reference, so the report leads with them.
    off = [(abs(rows[i] / e["fsuae"] - 1.0), i, e["name"])
           for i, e in enumerate(spec) if i not in raw and e["fsuae"]]
    off.sort(reverse=True)
    print("\nfurthest from reference: " +
          ", ".join(f"{n} {d:.0%}" for d, _, n in off[:6]))

    if args.write_baseline:
        with open(args.write_baseline, "w", encoding="utf-8") as fh:
            json.dump({"rows": rows}, fh, indent=2)
            fh.write("\n")
        print(f"\nbaseline written to {args.write_baseline}")
        return 0

    if regressed:
        print("\nFAIL: moved away from the baseline")
        for name, i, was, got in regressed:
            print(f"  row {i} {name}: {was} -> {got}")
        return 1

    if baseline is not None:
        print("\nPASS: every row matches the baseline")
    return 0


if __name__ == "__main__":
    sys.exit(main())
