# Jet Set Radio Future (PAL) — XBE Recomp

An unofficial native Windows port of **Jet Set Radio Future** (original Xbox, 2002), **European release**, made by
static recompilation of the game's `default.xbe` with [Xbox XBE Recomp](https://github.com/TGP482/XboxXBERecomp).
No emulator: the game's x86 code is translated to C++ and compiled for x64; the Xbox kernel, Direct3D 8,
DirectSound and XAPI are reimplemented on Windows (Direct3D 12, XAudio2, SDL3).

*Version française plus bas.*

> [!IMPORTANT]
> This repository contains **no game data, no XBE and no recompiled code**. You need your own legally obtained copy
> of the game. Only the **PAL (Europe) release** is supported: the other community ports
> ([Miggas/JSRF-Recomp](https://github.com/Miggas/JSRF-Recomp),
> [JetSetRadioFuture-Unleashed](https://github.com/JuMatt/JetSetRadioFuture-Unleashed),
> [jsrf-recomp](https://github.com/andeecollard/jsrf-recomp)) need the USA disc. This one is for the European disc and
> its five languages (English, French, German, Spanish, Italian).

## Status

Early work in progress:

- Boots through the logos, the PAL graffiti warning, the title screen and the attract demo, then starts a new game:
  story scenes and dialogue text work, at 60 fps in 1080p.
- Xbox / XInput controllers through SDL3.
- Not yet checked: audio by ear, saves, every stage. Expect bugs.

## Supported version

Jet Set Radio Future, **Europe** (En, Fr, De, Es, It), title ID `49470018`, XDK 4134.
`default.xbe` SHA-1: `edd8547185025834df914724069aa1e63109cbc1`.

The addresses in `config/game.toml` belong to this executable.

## XDK 4134

The SDK supported XDK 4831 and 5849. JSRF was built with XDK **4134**, and this repository adds what that needed
(in `sdk/`, meant to go upstream):

- `sdk/xdk/4134/layout.toml`: the D3D device, XAPI and DSP image layouts, each value tied to the code that proves it.
- The 4134 render state numbering (146 states, no unused slots), translated to the 4831/5849 one the renderer keeps.
- Contiguous memory for the title kept under 64 MB: the 4134 Direct3D masks physical addresses with `0x3FFFFFF`.
- Entry points older titles call: `IDirectSound_AddRef/GetCaps/SetAllParameters`, the `IDirectSoundBuffer` cone,
  mode and factor setters, `XInputPoll`, the kernel's `NtUserIoApcDispatcher`; fixed function immediate mode.

## Building

1. Visual Studio 2022 or later with **Desktop development with C++**, **C++ Clang tools for Windows** and
   **C++ CMake tools for Windows**.
2. Put your game files in `assets`: `default.xbe` with the `Media` folder beside it (extract your disc image, e.g.
   with [extract-xiso](https://github.com/XboxDev/extract-xiso), or `python tools/pal/xiso_extract.py image.iso assets`).
3. Run `build.bat`. The first build recompiles the game; the exe is `out\build\release\JSRF.exe`.

## Tools

`tools/pal/` holds the scripts used to bring the PAL executable up: XISO extraction, XBE header dump, the NTSC→PAL
address map built from the NTSC analysis, and scans for code pointers the recompiler missed. They read only your own
copy and output addresses.

## Contributing

Issues and pull requests are welcome. This port is developed with the help of an AI assistant (Claude); everything
is tested on the real game before being committed.

## License

MIT, see `LICENSE`. Third party software: `THIRD_PARTY.md`. Jet Set Radio Future is © SEGA / Smilebit; this project
is not affiliated with SEGA.

---

# Version française

Portage Windows natif non officiel de **Jet Set Radio Future** (Xbox, 2002), **version européenne**, par
recompilation statique du `default.xbe` avec Xbox XBE Recomp. Aucune donnée du jeu n'est fournie : il faut votre
propre copie. Seule la version **PAL** est prise en charge (les autres portages communautaires demandent le disque
américain) ; ses cinq langues sont disponibles.

État : démarrage, logos, avertissement, écran titre, démo, nouvelle partie, scènes et dialogues à 60 fps en 1080p.
Compilation : Visual Studio avec Clang et CMake, jeu extrait dans `assets`, puis `build.bat`.
