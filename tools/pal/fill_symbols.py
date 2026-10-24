"""Fill the zero entries of a config written by xbe_symbols from an XbSymbolDatabase listing.

Usage: python -I tools/pal/fill_symbols.py <game.toml> <XbSymbolDatabaseCLI output> [--write]

XbSymbolDatabase (Cxbx-Reloaded) knows XDK 4134, which the SDK has no signatures for yet. Entries
the scan already found are kept; a disagreement is reported, never overwritten.
"""
import re
import sys

# SDK data names -> XbSymbolDatabase names
DATA = {
    "D3D_pDevice": "D3D_g_pDevice",
    "D3D_RenderState": "D3D_g_RenderState",
    "D3D_DeferredTextureState": "D3D_g_DeferredTextureState",
    "XPP_GamepadType": "g_DeviceType_Gamepad",
    "XPP_MemoryUnitType": "g_DeviceType_MU",
}


def main():
    cfg_path, db_path = sys.argv[1], sys.argv[2]
    db = {}
    for line in open(db_path):
        m = re.match(r"(\w+?)__(\w+) = (0x[0-9a-fA-F]+)", line)
        if m:
            db.setdefault(m.group(2), int(m.group(3), 16))
    text = open(cfg_path, encoding="utf-8").read()
    filled, conflicts = [], []

    def sub(m):
        name, val, tail = m.group(1), int(m.group(3), 16) if m.group(3) != "0" else 0, m.group(4)
        key = DATA.get(name, name)
        if key not in db:
            return m.group(0)
        if val == 0:
            filled.append(name)
            v = f"0x{db[key]:X}"
            return f"{name}{m.group(2)}{v}{tail.rstrip() or ''}  # XbSymbolDatabase"
        if val != db[key]:
            conflicts.append(f"{name}: scan 0x{val:X}, XbSymbolDatabase 0x{db[key]:X}")
        return m.group(0)

    text = re.sub(r"^(\w+)(\s*=\s*)(0x[0-9A-Fa-f]+|0)\b([^\n]*)$", sub, text, flags=re.M)
    print(f"filled {len(filled)}: {' '.join(filled)}")
    for c in conflicts:
        print("CONFLICT", c)
    left = re.findall(r"^(\w+)\s*=\s*0\s*$", text, re.M)
    print(f"still 0 ({len(left)}): {' '.join(left)}")
    if "--write" in sys.argv:
        open(cfg_path, "w", encoding="utf-8", newline="\n").write(text)


if __name__ == "__main__":
    main()
