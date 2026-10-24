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
- Xbox / XInput controllers through SDL3; keyboard and mouse as a fallback (keys rebindable in `settings.toml`).
- Music, sound effects and voices.
- Not yet checked: saves, every stage. Expect bugs.

## Features

- **Hor+ widescreen**: the game's 4:3 projection, focal length and frustum are widened to the output's aspect
  (16:9, 21:9…), while the 2D interface keeps its 4:3 proportions. Option `ecran_large`.
- Any resolution, internal resolution up to 200 % (supersampling), SMAA, adaptive sharpening, anisotropic filtering.
- 60 fps, vsync, frame cap; English, French, German, Spanish or Italian text and voices from the PAL disc
  (`langue` in `settings.toml`).
- In-game options panel on **F1**, saved to `settings.toml`.

## Supported version

Jet Set Radio Future, **Europe** (En, Fr, De, Es, It), title ID `49470018`, XDK 4134.
`default.xbe` SHA-1: `edd8547185025834df914724069aa1e63109cbc1`.

The addresses in `config/game.toml` belong to this executable.

## XDK 4134

The SDK supported XDK 4831 and 5849. JSRF was built with XDK **4134**, and this repository adds what that needed
(in `sdk/`, proposed upstream in [TGP482/XboxXBERecomp#14](https://github.com/TGP482/XboxXBERecomp/pull/14)):

- `sdk/xdk/4134/layout.toml`: the D3D device, XAPI and DSP image layouts, each value tied to the code that proves it.
- The 4134 render state numbering (146 states, no unused slots), translated to the 4831/5849 one the renderer keeps.
- Contiguous memory for the title kept under 64 MB: the 4134 Direct3D masks physical addresses with `0x3FFFFFF`.
- Entry points older titles call: `IDirectSound_AddRef/GetCaps/SetAllParameters`, the `IDirectSoundBuffer` cone,
  mode and factor setters, `XInputPoll`, the kernel's `NtUserIoApcDispatcher`; fixed function immediate mode.

## Playing (release zip)

1. Download `JSRF.zip` from the [releases](https://github.com/yacineolivier-maker/JSRF-PAL-Recomp/releases) and extract it.
2. Put your **PAL** disc image (`.iso`) in the `assets` folder, or `default.xbe` with its `Media` folder.
3. Run `JSRF.exe`. The first start translates and compiles the game's code from **your** copy (a few minutes,
   with a progress window): no game code is distributed. Later starts are immediate.
4. **F1** opens the options. Saves go to `hdd\` beside the exe.

## Building

1. Visual Studio 2022 or later with **Desktop development with C++**, **C++ Clang tools for Windows** and
   **C++ CMake tools for Windows**.
2. Put your game files in `assets`: the PAL `.iso`, or `default.xbe` with its `Media` folder (extract with
   [extract-xiso](https://github.com/XboxDev/extract-xiso) or `python tools/pal/xiso_extract.py image.iso assets`).
3. Run `build.bat` (`release.bat` makes the zip, without game code).

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

État : démarrage, logos, avertissement, écran titre, démo, nouvelle partie, scènes, dialogues et premiers pas en jeu
à 60 fps en 1080p, musique et voix. Écran large Hor+ (interface gardée en 4:3), SMAA, netteté, panneau d'options F1,
choix de la langue du disque PAL, clavier-souris en secours.
Jouer : extraire `JSRF.zip` (page Releases), mettre l'ISO PAL dans `assets`, lancer `JSRF.exe`. Le premier démarrage
compile le code du jeu à partir de votre copie (quelques minutes) ; aucun code du jeu n'est distribué. F1 : options.
