# Raptor: Call of the Shadows - Amiga 68060 & EC/LC RTG/AGA Port

This repository contains an AmigaOS 3.x port of **Raptor: Call of the Shadows**, based on the open-source reverse-engineered codebase by [skynettx/raptor](https://github.com/skynettx/raptor).

The port targets **68060-class Amiga systems** — both full **68060 with FPU** and FPU-less **68EC060 / 68LC060** (dedicated soft-float binary) — with **RTG graphics (Picasso96 / CyberGraphX)** or the **native AGA chipset** (`GFX=AGA`), with primary testing and optimization aimed at:

- **Amiga 2000 / Amiga 3000 / Amiga 4000** with RTG cards such as **CyberVision 64/3D**, Picasso IV, or similar
- **Amiga 1200 with PiStorm / Emu68** (RTG or AGA)
- **AmigaOS 3.2**
- **Picasso96 RTG** (CyberGraphX supported, AGA as fallback)
- **AHI audio**

This fork is focused on making the game run natively on classic Amiga hardware without unnecessary abstraction layers. The game renders in a fixed **320x200, 8-bit paletted mode** on a dedicated screen, with Amiga-specific SDL replacement stubs and driver-native frame presentation on every display path.

## Download

Ready-to-run binaries are on the [Releases page](https://github.com/RaybeezPL/raptor-amiga-port/releases):

- **`raptor`** — 68060 **with FPU** (recommended)
- **`raptor_nofpu`** — soft-float build for **68060 without FPU** (68EC060/68LC060 or a broken FPU)

You also need the original game data files (`FILE0000.GLB` ... `FILE0004.GLB`, full version 1.2) — see **README_AMIGA.md** for installation.

## Project goals

The goals of this port are:

- Bring **Raptor: Call of the Shadows** to classic Amiga systems with a **68060-class CPU** (RTG or AGA graphics)
- Replace SDL-dependent parts of the engine with **native AmigaOS implementations**
- Keep the rendering path efficient for Amiga RTG hardware, avoiding unnecessary format conversion and slow per-pixel drawing paths
- Make the codebase practical for testing on both **WinUAE** and **real hardware**
- Document the porting process in a clean and reproducible way

## Current status

Current version: **0.9.5 AHI**

Working:

- Two native **m68k AmigaOS cross-builds**: `raptor` (68060 + FPU) and
  `raptor_nofpu` (soft-float, for FPU-less 68EC060/68LC060 or a broken
  FPU) — `build_amiga_nofpu.sh` / `make -f Makefile.amiga NOFPU=1`
- Full gameplay on real hardware (A2000, A1200 + PiStorm/Emu68) and WinUAE
- Keyboard, mouse and joystick/CD32 pad input working simultaneously
- RTG video path for **320x200x8-bit** output on a dedicated screen;
  tries P96 (Picasso96) first, falls back to CGX (CyberGraphX), then
  falls back to 320x240x8 with letterbox on RTG cards that lack 320x200
- **`GFX=AUTO|RTG|AGA`** parameter — controls the graphics driver path
  (CLI: `-gfx=RTG`; icon ToolType: `GFX=RTG`)
- Accelerated frame presentation on every display path: Picasso96 uses
  the driver's own `p96WritePixelArray`, CyberGraphX uses CGX
  `WritePixelArray`, and the `GFX=AGA` chipset screen uses a custom
  68060 chunky-to-planar converter — all replacing the generic
  `WriteChunkyPixels` OS conversion (kept as fallback)
- **`MUSIC=ADLIB|CAMD|MHI|OFF`** parameter — selects the music backend:
  built-in AdLib/OPL3 emulation (default), General MIDI via CAMD, MP3
  files from the `MP3/` drawer via an MHI hardware decoder, or no music
  (CLI: `-music=CAMD`; icon ToolType: `MUSIC=CAMD`)
- Workbench icon ToolTypes (NOSOUND/NOMUSIC/NOJOY/GFX/MUSIC) via the
  official WBStartup + icon.library mechanism
- Clean startup banner and parameter output on Shell/CLI; on Workbench
  launches no console window is opened at all (nothing is left behind
  when the game exits)
- Phantom-input filtering hardened for PiStorm/Emu68 machines: the
  middle-mouse button is dropped on every display path (RTG and AGA) and
  the gameport is polled at max 50 Hz with a clear-read unmask — fixes
  intro/demo skipping and erratic steering
- English requester with troubleshooting info when RTG mode is required
  but unavailable (no silent fallback to AGA)
- **Sound effects through AHI** (ahi.device): 11025 Hz 16-bit stereo -
  the native rate of the game's samples - streamed by a dedicated audio
  task using the canonical double-buffered CMD_WRITE scheme
- **Music: AdLib/OPL3 emulation by default** (lightweight DOSBox dbopl
  core, only a few percent of a 68060) mixed into the AHI stream -
  always audible; optional **General MIDI via CAMD** (camd.library)
  with `MUSIC=CAMD` for external synths / CAMD software synths
  (cluster "out.0"; needs a configured MIDI driver or synth), or
  **MP3 music via MHI** (`MUSIC=MHI`) for a Prisma Megamix / MAS /
  Delfina hardware decoder, with files in the game's `MP3/` drawer
- **Persistent audio volumes** via `amiga.cfg` in the game directory
  (created on first run): separate startup volumes for AdLib/OPL3
  music, MHI/MP3 music and sound effects; the in-game Options sliders
  write their values back to it


Work still in progress:

- Fine-tuning and performance polish on real 68060 hardware

For detailed requirements, controls, parameters and troubleshooting see
**README_AMIGA.md** - the main port documentation.


## Target configuration

Recommended baseline target:

- **CPU:** Motorola 68060 with FPU (`raptor`) or FPU-less 68EC060/68LC060
  (`raptor_nofpu` soft-float build); PiStorm/Emu68 equivalent
- **Graphics:** RTG board with Picasso96 (P96) support; also works
  with CyberGraphX (CGX / cybergraphics.library) or a native AGA
  chipset screen (`GFX=AGA`, no RTG card required)
- **Display mode:** 320x200, 8-bit paletted
- **RAM:** 4 MB Fast RAM minimum (8 MB recommended) + 2 MB Chip RAM
  (the game itself uses ~3 MB of Fast RAM; on RTG the screen bitmap
  lives in graphics card memory, so Chip RAM is only needed by the OS)

- **OS:** AmigaOS 3.2
- **Audio:** AHI (ahi.device) for sound effects and the default
  AdLib/OPL3 music; camd.library (CAMD) optional for MIDI music
  output (MUSIC=CAMD)

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

This repository is **not** a generic multi-platform fork. Its main purpose is to develop and maintain the **Amiga 68060 port (RTG and AGA)**.

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
