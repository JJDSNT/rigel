#!/usr/bin/env python3
"""Compare the FPU results the test ROM left in Chip RAM against expected.json.

The ROM writes one big-endian int32 per opmode, then a $5A5A5A5A sentinel. The
sentinel is checked too: without it, a run that died early would leave zeros
that happen to match the several opmodes whose expected result is 0.
"""
import json
import struct
import sys

SENTINEL = 0x5A5A5A5A


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: check.py <results.bin> <expected.json>", file=sys.stderr)
        return 2

    data = open(sys.argv[1], "rb").read()
    with open(sys.argv[2], encoding="utf-8") as fh:
        expected = json.load(fh)

    names, exp = expected["names"], expected["expected"]
    n = len(names)

    if len(data) < (n + 1) * 4:
        print(f"FAIL: dump is {len(data)} bytes, need {(n + 1) * 4}")
        return 1

    values = struct.unpack(f">{n}i", data[: n * 4])
    (sentinel,) = struct.unpack(">I", data[n * 4 : n * 4 + 4])

    if sentinel != SENTINEL:
        print(f"FAIL: sentinel is {sentinel:08x}, expected {SENTINEL:08x} — "
              "the ROM did not run to completion, so the results are stale")
        return 1

    ok = True
    for name, want, got in zip(names, exp, values):
        status = "OK" if got == want else "MISMATCH"
        if got != want:
            ok = False
        print(f"  {name:10s} expected={want:6d} got={got:6d}  {status}")

    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
