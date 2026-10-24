"""List code addresses the title's code loads as immediates (push/mov) with no recompiled function.

Usage: python -I tools/pal/imm_pointers.py <default.xbe> <x86_func_mapping.cpp> <asm listing>...

Kept when the immediate is an instruction start of the listing that follows padding or a return.
"""
import re
import struct
import sys

data = open(sys.argv[1], "rb").read()
base = struct.unpack_from("<I", data, 0x104)[0]
nsec, saddr = struct.unpack_from("<II", data, 0x11C)
secs = []
for i in range(nsec):
    flags, va, vsize, raw, rsize = struct.unpack_from("<5I", data, saddr - base + i * 56)
    secs.append((va, raw, rsize))


def byte(va):
    for v, raw, rsize in secs:
        if v <= va < v + rsize:
            return data[raw + va - v]
    return None


have = {int(x, 16) for x in re.findall(r"\{0x([0-9A-Fa-f]+)u,", open(sys.argv[2]).read())}
insns, refs = set(), {}
for path in sys.argv[3:]:
    for m in re.finditer(r"^  0x([0-9A-F]{8})  [0-9a-f]+\s+(\w+)\s+(.*?)\s*(;.*)?$", open(path).read(), re.M):
        a = int(m.group(1), 16)
        insns.add(a)
        if m.group(2) in ("push", "mov", "lea", "cmp"):
            for imm in re.findall(r"(?<![\[+\-\w])0x([0-9a-f]{5,6})\b(?!\])", m.group(3)):
                refs.setdefault(int(imm, 16), a)
found = {}
for p, where in refs.items():
    if p in have or p not in insns:
        continue
    prev = byte(p - 1)
    if prev in (0xCC, 0x90, 0xC3) or byte(p - 3) == 0xC2:
        found[p] = where
for p, where in sorted(found.items()):
    print(f"0x{p:X} = \"\"  # immediate at 0x{where:X}")
print(f"# {len(found)} immediates without a function", file=sys.stderr)
