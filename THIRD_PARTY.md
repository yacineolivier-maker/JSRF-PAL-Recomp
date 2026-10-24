# Third party software

- **Xbox XBE Recomp** (TGP482, MIT): `sdk/`, license in `sdk/LICENSE`. The libraries linked into the exe
  (toml++, Dear ImGui, SDL3) and their licenses are listed in `sdk/THIRD_PARTY.txt`. The recompiler uses Zydis
  (MIT), downloaded at configure time. Local changes to the SDK (XDK 4134 support) are listed in `sdk/UPSTREAM_COMMIT`
  and in the git history.
- The XDK 4134 render state numbering in `sdk/runtime/xdk/d3d8.cpp` was derived from
  [Cxbx-Reloaded](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded)'s `DxbxRenderStateInfo` table (GPL-2.0; only the
  ordering facts are used, no code). XDK symbol addresses were cross-checked with
  [XbSymbolDatabase](https://github.com/Cxbx-Reloaded/XbSymbolDatabase) (MIT), run on the user's own copy.
- The NTSC function list of [Miggas/JSRF-Recomp](https://github.com/Miggas/JSRF-Recomp) (MIT) was mapped to the PAL
  executable by `tools/pal/ntsc_pal_map.py` (addresses only).
