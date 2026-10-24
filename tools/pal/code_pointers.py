"""List code pointers stored in data that the recompiler has no function for.

Usage: python -I tools/pal/code_pointers.py <default.xbe> <x86_func_mapping.cpp> <text.asm listing> [more .asm]

A dword in .rdata or .data is taken when it points at an instruction start of the listing that follows
padding or a return (a function boundary), and no recompiled function starts there.
"""
import re
import struct
import sys

data = open(sys.argv[1], "rb").read()
base = struct.unpack_from("<I", data, 0x104)[0]
nsec, saddr = struct.unpack_from("<II", data, 0x11C)
secs = {}
for i in range(nsec):
    flags, va, vsize, raw, rsize, name_addr = struct.unpack_from("<6I", data, saddr - base + i * 56)
    name = data[name_addr - base:data.index(b"\0", name_addr - base)].decode()
    secs[name] = (va, raw, rsize)
have = {int(x, 16) for x in re.findall(r"\{0x([0-9A-Fa-f]+)u,", open(sys.argv[2]).read())}
insns = set()
for path in sys.argv[3:]:
    for m in re.finditer(r"^  0x([0-9A-F]{8})  ", open(path).read(), re.M):
        insns.add(int(m.group(1), 16))


def byte(va):
    for v, raw, rsize in secs.values():
        if v <= va < v + rsize:
            return data[raw + va - v]
    return None


found = {}
for sec in (".rdata", ".data"):
    va, raw, rsize = secs[sec]
    for o in range(0, rsize - 3, 4):
        p = struct.unpack_from("<I", data, raw + o)[0]
        if p in have or p not in insns:
            continue
        prev = byte(p - 1)
        if prev in (0xCC, 0x90, 0xC3) or byte(p - 3) == 0xC2:
            found.setdefault(p, va + o)
for p, where in sorted(found.items()):
    print(f"0x{p:X} = \"\"  # pointer at 0x{where:X}")
print(f"# {len(found)} code pointers without a function", file=sys.stderr)
