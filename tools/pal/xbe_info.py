"""Dump the header, certificate, sections and XDK libraries of an XBE.

Usage: python -I tools/xbe_info.py <default.xbe> [--json out.json]
"""
import hashlib
import json
import struct
import sys

RETAIL_EP_KEY = 0xA8FC57AB
RETAIL_KT_KEY = 0x5B6D40B6
DEBUG_EP_KEY = 0x94859D4B
DEBUG_KT_KEY = 0xEFB1F152

REGIONS = {1: "NA", 2: "JP", 4: "RestOfWorld(PAL)", 0x80000000: "Manufacturing"}


def cstr(data, off):
    end = data.index(b"\0", off)
    return data[off:end].decode("latin-1")


def main():
    path = sys.argv[1]
    data = open(path, "rb").read()
    assert data[:4] == b"XBEH", "not an XBE"
    base = struct.unpack_from("<I", data, 0x104)[0]
    va = lambda a: a - base  # header region is file-mapped 1:1

    hdr = dict(zip(
        ["base", "size_headers", "size_image", "size_image_header", "timestamp",
         "cert_addr", "num_sections", "section_addr", "init_flags", "entry",
         "tls_addr", "stack", "heap_reserve", "heap_commit", "pe_base", "pe_size",
         "pe_checksum", "pe_timestamp", "debug_path", "debug_file", "debug_ufile",
         "kernel_thunk", "nonkernel_import", "num_libs", "lib_addr",
         "kernel_lib", "xapi_lib"],
        struct.unpack_from("<27I", data, 0x104)))

    entry, thunk = hdr["entry"], hdr["kernel_thunk"]
    if (entry ^ RETAIL_EP_KEY) < 0x80000000 and (entry ^ RETAIL_EP_KEY) > base:
        kind, entry, thunk = "retail", entry ^ RETAIL_EP_KEY, thunk ^ RETAIL_KT_KEY
    else:
        kind, entry, thunk = "debug", entry ^ DEBUG_EP_KEY, thunk ^ DEBUG_KT_KEY

    c = va(hdr["cert_addr"])
    title_id = struct.unpack_from("<I", data, c + 8)[0]
    title = data[c + 12:c + 12 + 80].decode("utf-16-le").rstrip("\0")
    region, ratings, disc, version = struct.unpack_from("<IIII", data, c + 0xA0)

    sections = []
    s = va(hdr["section_addr"])
    for i in range(hdr["num_sections"]):
        (flags, vaddr, vsize, raddr, rsize, name_addr, _refc, _h, _t,
         ) = struct.unpack_from("<IIIIIIIII", data, s + i * 56)
        digest = data[s + i * 56 + 36:s + i * 56 + 56].hex()
        raw = data[raddr:raddr + rsize]
        sections.append({
            "name": cstr(data, va(name_addr)), "vaddr": hex(vaddr), "vsize": vsize,
            "raw": hex(raddr), "rsize": rsize, "flags": hex(flags),
            "sha1": hashlib.sha1(raw).hexdigest(), "header_digest": digest})

    libs = []
    l = va(hdr["lib_addr"])
    for i in range(hdr["num_libs"]):
        name = data[l + i * 16:l + i * 16 + 8].rstrip(b"\0").decode()
        major, minor, build, flags = struct.unpack_from("<HHHH", data, l + i * 16 + 8)
        libs.append(f"{name} {major}.{minor}.{build} (qfe {flags & 0x1FFF})")

    # kernel imports: thunk table lives inside a section; map VA -> file
    def va_to_file(a):
        for sec in sections:
            v = int(sec["vaddr"], 16)
            if v <= a < v + sec["rsize"]:
                return int(sec["raw"], 16) + (a - v)
        return None

    imports = []
    t = va_to_file(thunk)
    if t is not None:
        while True:
            ordinal = struct.unpack_from("<I", data, t)[0]
            if ordinal == 0:
                break
            imports.append(ordinal & 0x7FFFFFFF)
            t += 4

    info = {
        "file_sha1": hashlib.sha1(data).hexdigest(), "file_size": len(data),
        "kind": kind, "base": hex(base), "entry": hex(entry), "kernel_thunk": hex(thunk),
        "timestamp": hdr["timestamp"], "title_id": f"{title_id:08X}", "title": title,
        "region": [n for b, n in REGIONS.items() if region & b], "version": version,
        "disc": disc, "debug_path": cstr(data, va(hdr["debug_path"])),
        "tls_addr": hex(hdr["tls_addr"]), "stack": hdr["stack"],
        "sections": sections, "libraries": libs, "kernel_imports": imports,
    }
    if "--json" in sys.argv:
        json.dump(info, open(sys.argv[sys.argv.index("--json") + 1], "w"), indent=1)
    for k, v in info.items():
        if k == "sections":
            print("sections:")
            for sec in v:
                print(f"  {sec['name']:<10} va={sec['vaddr']:>10} vsize={sec['vsize']:>8} "
                      f"raw={sec['raw']:>8} rsize={sec['rsize']:>8} sha1={sec['sha1'][:12]}")
        elif k == "libraries":
            print("libraries:", ", ".join(v))
        elif k == "kernel_imports":
            print(f"kernel_imports ({len(v)}):", v)
        else:
            print(f"{k}: {v}")


if __name__ == "__main__":
    main()
