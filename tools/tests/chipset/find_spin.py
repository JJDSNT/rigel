#!/usr/bin/env python3
"""Print the address of the first branch-to-itself in a --disasm dump.

A test program's final `bra *` is its "I finished" marker, and that is where
the breakpoint belongs. Locating it in the disassembly keeps the test working
when an instruction is added above it, which a hardcoded address does not.
"""
import re
import sys

PATTERN = re.compile(r"\[DIS \]\s+([0-9a-f]{8})\s+bra\S*\s+\$([0-9a-f]+)")


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: find_spin.py <disasm-output>", file=sys.stderr)
        return 2

    with open(sys.argv[1], encoding="utf-8", errors="replace") as fh:
        for line in fh:
            m = PATTERN.match(line)
            if m and int(m.group(1), 16) == int(m.group(2), 16):
                print(m.group(1))
                return 0

    print("no self-branch found", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
