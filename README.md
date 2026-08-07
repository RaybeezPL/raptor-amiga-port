# Raptor: Call of the Shadows - Amiga 68060 RTG Port

This repository contains an AmigaOS 3.x port of **Raptor: Call of the Shadows**, based on the open-source reverse-engineered codebase by [skynettx/raptor](https://github.com/skynettx/raptor).

The current focus is a native build for **68060-based Amiga systems with RTG graphics**, with primary testing and optimization aimed at:

- **Amiga 2000 / Amiga 3000 / Amiga 4000** with RTG cards such as **CyberVision 64/3D**, Picasso IV, or similar
- **Amiga 1200 with PiStorm / Emu68**
- **AmigaOS 3.2**
- **Picasso96 RTG**
- **AHI audio**

This fork is focused on making the game run natively on classic Amiga hardware without unnecessary abstraction layers. The rendering path is being adapted for a fixed **320x200, 8-bit paletted RTG mode**, with Amiga-specific SDL replacement stubs and an RTG-friendly chunky output path.

## Project goals

The goals of this port are:

- Bring **Raptor: Call of the Shadows** to classic Amiga systems with **68060 + RTG**
- Replace SDL-dependent parts of the engine with **native AmigaOS implementations**
- Keep the rendering path efficient for Amiga RTG hardware, avoiding unnecessary format conversion and slow per-pixel drawing paths
- Make the codebase practical for testing on both **WinUAE** and **real hardware**
- Document the porting process in a clean and reproducible way

## Current status

Current version: **BETA 0.8.1 NOSOUND**

Working:

- Native **m68k AmigaOS cross-build** (68060 + FPU)
- Full gameplay on real hardware (A2000, A1200 + PiStorm/Emu68) and WinUAE
- Keyboard, mouse and joystick/CD32 pad input working simultaneously
- RTG video path for **320x200x8-bit** output on a dedicated screen;
  tries P96 (Picasso96) first, falls back to CGX (CyberGraphX), then
  falls back to 320x240x8 with letterbox on RTG cards that lack 320x200
- **`GFX=AUTO|RTG|AGA`** parameter — controls the graphics driver path
  (CLI: `-gfx=RTG`; icon ToolType: `GFX=RTG`)
- Workbench icon ToolTypes (NOSOUND/NOMUSIC/NOJOY/GFX) via the
  official WBStartup + icon.library mechanism
- Clean startup banner and parameter output on Shell/CLI; on Workbench
  launches no console window is opened at all (nothing is left behind
  when the game exits)
- Phantom middle-mouse-button filtering on RTG screens (fixes intro/demo
  skipping and erratic steering on some machines)
- English requester with troubleshooting info when RTG mode is required
  but unavailable (no silent fallback to AGA)


Work still in progress:

- **AHI audio playback** - the game currently must be started with
  `-nosound` (see README_AMIGA.md)

For detailed requirements, controls, parameters and troubleshooting see
**README_AMIGA.md** - the main port documentation.


## Target configuration

Recommended baseline target:

- **CPU:** Motorola 68060 or PiStorm/Emu68 equivalent
- **Graphics:** RTG board with Picasso96 (P96) support; also works
  with CyberGraphX (CGX / cybergraphics.library)
- **Display mode:** 320x200, 8-bit paletted
- **RAM:** 4 MB Fast RAM minimum (8 MB recommended) + 2 MB Chip RAM
  (the game itself uses ~3 MB of Fast RAM; on RTG the screen bitmap
  lives in graphics card memory, so Chip RAM is only needed by the OS)

- **OS:** AmigaOS 3.2
- **Audio:** AHI

Current development and testing is mainly aimed at systems such as:

- **CyberVision 64/3D**
- **Picasso IV**
- **PiStorm-based Amiga systems**

## Build environment

Primary development environment:

- **Host OS:** Windows 11
- **Build environment:** Ubuntu under WSL2
- **Compiler:** `m68k-amigaos-gcc`
- **Main branch:** `main`

The current porting workflow focuses on practical iteration speed, reproducible cross-builds, and fast emulator-to-real-hardware testing.

## Assets and legal note

This repository does **not** include the original game data files.

To use this port, you must provide your own legal copy of the original **Raptor: Call of the Shadows** data files (shareware or full version, compatible data set required).

## Upstream base

This Amiga port is based on the reverse-engineered open-source project:

- [skynettx/raptor](https://github.com/skynettx/raptor)

That upstream project reconstructs the original game engine in C/C++ and made this Amiga port possible.

## Scope of this fork

This repository is **not** a generic multi-platform fork. Its main purpose is to develop and maintain the **Amiga 68060 RTG port**.

Platform-specific notes for Windows, Linux, macOS, and Android from the original upstream project are not the focus of this fork and may differ from the current upstream README.

## Upstream project reference

This repository is a downstream Amiga-focused fork of:

- [skynettx/raptor](https://github.com/skynettx/raptor)

For the original multi-platform project documentation, installation notes, and upstream release information, refer to the upstream repository.

## Credits

Special thanks to:

- **nukeykt** and contributors involved in the reverse-engineered Raptor codebase
- **skynettx** for the open-source C/C++ recreation used as the base for this port
- The Amiga community, emulator authors, and RTG/AHI toolchain developers
