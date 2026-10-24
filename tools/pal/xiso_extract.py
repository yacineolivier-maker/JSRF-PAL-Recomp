"""Minimal XDVDFS (Xbox ISO) extractor.

Usage: python -I tools/xiso_extract.py <image.iso> <out_dir>

Handles both plain XISO images (game partition at offset 0) and full
redump images (game partition at 0x18300000).
"""
import os
import struct
import sys

SECTOR = 2048
MAGIC = b"MICROSOFT*XBOX*MEDIA"
PARTITION_OFFSETS = (0, 0x18300000, 0x0FD90000, 0x89D80000, 0x2080000)


def find_base(f):
    for base in PARTITION_OFFSETS:
        f.seek(base + 32 * SECTOR)
        if f.read(20) == MAGIC:
            return base
    raise SystemExit("XDVDFS volume descriptor not found")


def walk(f, base, sector, size, out_dir, listing, rel=""):
    if size == 0:
        return
    f.seek(base + sector * SECTOR)
    table = f.read(size)
    stack = [0]
    seen = set()
    while stack:
        off = stack.pop()
        if off in seen or off + 14 > len(table):
            continue
        seen.add(off)
        left, right, start, length, attr, nlen = struct.unpack_from("<HHIIBB", table, off)
        if left == 0xFFFF and right == 0xFFFF:
            continue  # padding
        name = table[off + 14:off + 14 + nlen].decode("latin-1")
        if left:
            stack.append(left * 4)
        if right:
            stack.append(right * 4)
        path = os.path.join(rel, name)
        dst = os.path.join(out_dir, path)
        if attr & 0x10:
            os.makedirs(dst, exist_ok=True)
            listing.append((path + "/", 0))
            walk(f, base, start, length, out_dir, listing, path)
        else:
            listing.append((path, length))
            f.seek(base + start * SECTOR)
            remaining = length
            with open(dst, "wb") as o:
                while remaining:
                    chunk = f.read(min(remaining, 1 << 20))
                    if not chunk:
                        raise SystemExit(f"truncated image while reading {path}")
                    o.write(chunk)
                    remaining -= len(chunk)


def main():
    iso, out_dir = sys.argv[1], sys.argv[2]
    os.makedirs(out_dir, exist_ok=True)
    listing = []
    with open(iso, "rb") as f:
        base = find_base(f)
        f.seek(base + 32 * SECTOR + 20)
        root_sector, root_size = struct.unpack("<II", f.read(8))
        walk(f, base, root_sector, root_size, out_dir, listing)
    listing.sort()
    for path, length in listing:
        print(f"{length:>12}  {path}")
    print(f"{len(listing)} entries")


if __name__ == "__main__":
    main()
