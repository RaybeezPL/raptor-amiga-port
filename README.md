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

Current milestone tag: **`v0.7.0-amiga-preview1`**

Implemented or already working:

- Native **m68k AmigaOS cross-build**
- Successful Amiga executable linking
- SDL replacement layer for the Amiga port
- RTG video path for **320x200x8-bit** output
- Visible graphics output on Amiga-side test builds
- Intro playback visible on screen
- Basic Amiga-specific backend integration groundwork
- Repository cleaned up with **`main`** as the primary development branch

Work still in progress:

- Stable **keyboard and mouse input**
- Full **AHI audio playback**
- Further optimization of the RTG rendering path
- Extended testing on real Amiga hardware
- Gameplay validation beyond boot / intro / early menu flow

## Target configuration

Recommended baseline target:

- **CPU:** Motorola 68060 or PiStorm/Emu68 equivalent
- **Graphics:** RTG board with Picasso96 support
- **Display mode:** 320x200, 8-bit paletted
- **RAM:** 32 MB Fast RAM minimum
- **OS:** AmigaOS 3.2
- **Audio:** AHI

Current development and testing is mainly aimed at systems such as:

- **CyberVision 64/3D**
- **Picasso IV**
- **PiStorm-based Amiga systems**

## Build environment

Primary development environment:

- **Host OS:** Windows 11 + WSL2
- **Build environment:** Ubuntu under WSL
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
