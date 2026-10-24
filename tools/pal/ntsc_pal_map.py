"""Build an NTSC -> PAL code address map for Jet Set Radio Future.

The PAL build (D:\\Jet2\\Src_Pal) is the NTSC game code relinked with small
changes, so functions appear in the same order with drifting offsets. We
align the NTSC function list (Miggas/JSRF-Recomp analysis) with the PAL
function list from tools.disasm on the PAL default.xbe, per section, on
function size, and keep the matched pairs as anchors.

Usage:
  python -I tools/ntsc_pal_map.py <ntsc functions_named.json> <pal functions.json> <out map.json>

Output: {"pairs": [[ntsc, pal, size], ...], "stats": {...}}
Use map_addr() (importable) to translate an arbitrary NTSC code address.
"""
import bisect
import difflib
import json
import sys


def load(path):
    funcs = json.load(open(path))
    by_sec = {}
    for f in funcs:
        by_sec.setdefault(f.get("section", ".text"), []).append(
            (int(f["start"], 16), int(f["size"])))
    for v in by_sec.values():
        v.sort()
    return by_sec


def align(ntsc, pal):
    a = [s for _, s in ntsc]
    b = [s for _, s in pal]
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    pairs = []
    for blk in sm.get_matching_blocks():
        for k in range(blk.size):
            n, p = ntsc[blk.a + k], pal[blk.b + k]
            pairs.append((n[0], p[0], n[1]))
    return pairs


class AddrMap:
    def __init__(self, pairs):
        self.pairs = sorted(pairs)
        self.keys = [p[0] for p in self.pairs]

    def map_addr(self, a):
        """Return (pal_addr, exact) for an NTSC code address, or (None, False)."""
        i = bisect.bisect_right(self.keys, a) - 1
        if i < 0:
            return None, False
        n, p, size = self.pairs[i]
        if a < n + size:
            return p + (a - n), True
        # between anchors: assume the drift of the previous anchor
        return p + (a - n), False


def main():
    ntsc, pal = load(sys.argv[1]), load(sys.argv[2])
    pairs, stats = [], {}
    for sec in sorted(set(ntsc) | set(pal)):
        got = align(ntsc.get(sec, []), pal.get(sec, []))
        # drop anchors whose drift is wildly off its neighbours (chance size equality)
        clean = []
        for i, (n, p, s) in enumerate(got):
            d = p - n
            near = [q - m for m, q, _ in got[max(0, i - 3):i + 4] if (m, q) != (n, p)]
            if near and min(abs(d - x) for x in near) > 0x400:
                continue
            clean.append((n, p, s))
        pairs += clean
        stats[sec] = {"ntsc": len(ntsc.get(sec, [])), "pal": len(pal.get(sec, [])),
                      "matched": len(clean),
                      "matched_bytes": sum(s for _, _, s in clean),
                      "ntsc_bytes": sum(s for _, s in ntsc.get(sec, []))}
    json.dump({"pairs": [[f"0x{n:08X}", f"0x{p:08X}", s] for n, p, s in sorted(pairs)],
               "stats": stats}, open(sys.argv[3], "w"), indent=0)
    for sec, s in stats.items():
        print(f"{sec:<8} ntsc={s['ntsc']:>5} pal={s['pal']:>5} matched={s['matched']:>5} "
              f"bytes {100 * s['matched_bytes'] / max(1, s['ntsc_bytes']):5.1f}%")


def load_map(path):
    d = json.load(open(path))
    return AddrMap([(int(n, 16), int(p, 16), s) for n, p, s in d["pairs"]])


if __name__ == "__main__":
    main()
