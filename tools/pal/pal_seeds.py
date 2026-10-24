"""Translate NTSC function starts and seeds to PAL seeds.

Usage:
  python -I tools/pal_seeds.py <map.json> <ntsc functions_named.json> <ntsc seeds.json>
                               <pal default.xbe> <out pal_seeds.json> [--report out.txt]

An NTSC address is kept when it lands in a matched function (exact), or when
the anchors on both sides agree on the drift and the PAL bytes before it are
padding or a return (a plausible function boundary).
"""
import bisect
import json
import struct
import sys

sys.path.insert(0, __file__.rsplit("\\", 1)[0].rsplit("/", 1)[0])
from ntsc_pal_map import load_map  # noqa: E402


def load_sections(xbe):
    data = open(xbe, "rb").read()
    base = struct.unpack_from("<I", data, 0x104)[0]
    nsec, saddr = struct.unpack_from("<II", data, 0x11C)
    secs = []
    for i in range(nsec):
        flags, va, vsize, raw, rsize = struct.unpack_from("<5I", data, saddr - base + i * 56)
        secs.append((va, raw, rsize))
    return data, secs


def byte_at(data, secs, va):
    for v, raw, rsize in secs:
        if v <= va < v + rsize:
            return data[raw + va - v]
    return None


def boundary_ok(data, secs, va):
    prev = byte_at(data, secs, va - 1)
    if prev is None:
        return False
    if prev in (0xCC, 0x90, 0xC3):
        return True
    # ret imm16
    return byte_at(data, secs, va - 3) == 0xC2


def main():
    amap = load_map(sys.argv[1])
    ntsc_funcs = json.load(open(sys.argv[2]))
    ntsc_seeds = json.load(open(sys.argv[3]))
    data, secs = load_sections(sys.argv[4])
    starts = {int(f["start"], 16) for f in ntsc_funcs} | {int(s["start"], 16) for s in ntsc_seeds}
    seed_set = {int(s["start"], 16) for s in ntsc_seeds}
    keys = amap.keys
    out, report = {}, []
    for a in sorted(starts):
        p, exact = amap.map_addr(a)
        if p is None:
            report.append(f"0x{a:08X} -> none (before first anchor)")
            continue
        if not exact:
            i = bisect.bisect_right(keys, a)
            if i < len(keys):
                n2, p2, _ = amap.pairs[i]
                if (p2 - n2) != (p - a):
                    report.append(f"0x{a:08X} -> ambiguous (drift changes before next anchor)")
                    continue
            if not boundary_ok(data, secs, p) and a not in seed_set:
                report.append(f"0x{a:08X} -> 0x{p:08X} rejected (not a boundary)")
                continue
        out[a] = p
    pal = sorted(set(out.values()))
    json.dump([{"start": f"0x{p:08X}"} for p in pal], open(sys.argv[5], "w"), indent=0)
    json.dump({f"0x{a:08X}": f"0x{p:08X}" for a, p in out.items()},
              open(sys.argv[5].replace(".json", "_ntsc2pal.json"), "w"), indent=0)
    seeds_mapped = sum(1 for a in seed_set if a in out)
    print(f"{len(starts)} NTSC starts, {len(out)} mapped, {len(starts) - len(out)} dropped; "
          f"seeds {seeds_mapped}/{len(seed_set)}")
    if "--report" in sys.argv:
        open(sys.argv[sys.argv.index("--report") + 1], "w").write("\n".join(report) + "\n")


if __name__ == "__main__":
    main()
