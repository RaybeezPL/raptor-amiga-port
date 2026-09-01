# Raptor: Call of the Shadows - Amiga Port (68030/68060 & EC/LC, RTG/AGA, AHI/MHI/CAMD)

This repository contains an AmigaOS 3.x port of **Raptor: Call of the Shadows**, based on the open-source reverse-engineered codebase by [skynettx/raptor](https://github.com/skynettx/raptor).

The port targets **68060-class Amiga systems** — both full **68060 with FPU** and FPU-less **68EC060 / 68LC060** (dedicated soft-float binary) — as well as **68030 systems**, covered by two separate dedicated binaries: **68030 with an external 68881/68882 FPU** (`raptor_030_fpu`) and **68030 without an FPU** (`raptor_030`, soft-float) — plus faster 68k hardware, with **RTG graphics (Picasso96 / CyberGraphX)** or the **native AGA chipset** (`GFX=AGA`), with primary testing and optimization aimed at:

- **Any Amiga system with a compatible RTG graphics card** — for example **CyberVision 64/3D**, **Picasso IV**, or similar — using **Picasso96** or **CyberGraphX**
- **Amiga A500 / A600 / A1200 / A2000 / A3000 / A4000 with PiStorm / Emu68** — RTG or native AGA, according to the configured environment
- **Amiga A1200 / A4000** — native AGA rendering (`GFX=AGA`); in **WinUAE** and compatible PiStorm/Emu68 setups either RTG or AGA can be selected when AGA is available
- **AmigaOS 3.2**
- **AHI audio**

This fork is focused on making the game run natively on classic Amiga hardware without unnecessary abstraction layers. The game renders in a fixed **320x200, 8-bit paletted mode** on a dedicated screen, with Amiga-specific SDL replacement stubs and driver-native frame presentation on every display path.

## Download

Ready-to-run binaries are on the [Releases page](https://github.com/RaybeezPL/raptor-amiga-port/releases):

- **`raptor`** — 68060 **with FPU** (recommended)
- **`raptor_nofpu`** — soft-float build for **68060 without FPU** (68EC060/68LC060 or a broken FPU)
- **`raptor_030_fpu`** — **68030 with an external 68881/68882 FPU**
- **`raptor_030`** — soft-float build for **68030 without an FPU**

You also need the original game data files (`FILE0000.GLB` ... `FILE0004.GLB`, full version 1.2) — see **README_AMIGA.md** for installation.

## Project goals

The goals of this port are:

- Bring **Raptor: Call of the Shadows** to classic Amiga systems with a **68030/68060-class CPU** (RTG or AGA graphics)
- Replace SDL-dependent parts of the engine with **native AmigaOS implementations**
- Keep the rendering path efficient for Amiga RTG hardware, avoiding unnecessary format conversion and slow per-pixel drawing paths
- Make the codebase practical for testing on both **WinUAE** and **real hardware**
- Document the porting process in a clean and reproducible way

## Current status

Current version: **0.9.7**

Working:

- Four native **m68k AmigaOS cross-builds**: `raptor` (68060 + FPU),
  `raptor_nofpu` (soft-float, for FPU-less 68EC060/68LC060 or a broken
  FPU), `raptor_030_fpu` (68030 + external 68881/68882 FPU) and
  `raptor_030` (soft-float 68030)
- Full gameplay on real hardware — tested on A2000 with CyberVision 64/3D, A1200 + PiStorm/Emu68 (display output verified via both RTG and AGA), and WinUAE; expected to work on any Amiga model (A500/A600/A1200/A2000/A3000/A4000) with PiStorm/Emu68 + RTG
- Keyboard, mouse and joystick/CD32 pad input working simultaneously
- RTG video path for **320x200x8-bit** output on a dedicated screen;
  tries P96 (Picasso96) first, falls back to CGX (CyberGraphX), then
  falls back to 320x240x8 with letterbox on RTG cards that lack 320x200
- **`GFX=AUTO|RTG|AGA`** parameter — controls the graphics driver path
  (CLI: `-gfx=RTG`; icon ToolType: `GFX=RTG`)
- Accelerated frame presentation on every display path: Picasso96 uses
  the driver's own `p96WritePixelArray`, CyberGraphX uses CGX
  `WritePixelArray`, and the `GFX=AGA` chipset screen uses a custom
  68060 chunky-to-planar converter — native AGA rendering has been
  optimized by caching the converted AGA bitmap used for C2P blits,
  reducing unnecessary conversion work when the game frame does not
  change — all replacing the generic `WriteChunkyPixels` OS conversion
  (kept as fallback)
- **`MUSIC=ADLIB|CAMD|MHI|OFF`** parameter — selects the music backend:
  built-in AdLib/OPL3 emulation (default), General MIDI via CAMD, MP3
  files from the `MP3/` drawer via an MHI hardware decoder, or no music
  (CLI: `-music=CAMD`; icon ToolType: `MUSIC=CAMD`)
- **`MOUSE=ON|OFF`** / **`NOMOUSE`** and **`JOYSTICK=ON|OFF`** / **`NOJOY`**
  parameters — enable/disable the mouse and joystick input devices (CLI:
  `-mouse=off`, `-nomouse`, `-joystick=off`, `-nojoy`; icon ToolTypes:
  `MOUSE=OFF`, `NOMOUSE`, `JOYSTICK=OFF`, `NOJOY`). Both default to ON.
  An explicit `KEYWORD=value` wins over the legacy bare flag. Disabling an
  input device is a performance/troubleshooting option: with the mouse off
  the window registers no mouse events at all, and with the joystick off
  the game port is never polled.
- Workbench icon ToolTypes (NOSOUND/NOMUSIC/NOJOY/NOMOUSE/GFX/MUSIC/
  JOYSTICK/MOUSE) via the official WBStartup + icon.library mechanism
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


Work still in progress / roadmap:

- Fine-tuning and performance polish on real 68k hardware
- **Pre-decoded WAVE music** (planned): the game will read soundtrack
  files from a directory named `WAVE` in the game installation root,
  alongside the executable. Each filename is identical to the existing
  MP3 song mapping filename with only the extension changed from
  `.mp3` to `.wav` (spaces and exact base names preserved, e.g.
  `WAVE/Main Menu.wav`, `WAVE/Wave Music 1.wav`, `WAVE/Boss 1.wav`).
  Required format: RIFF/WAVE container, uncompressed PCM, signed
  16-bit little-endian samples, stereo (2 channels), 11025 Hz —
  matching the existing 11025 Hz SFX/AHI audio pipeline (no runtime
  MP3 decoding and no runtime resampling). Missing WAVE files are
  non-fatal: affected songs stay silent while the game and sound
  effects continue normally.
- Once WAVE music playback is completed and tested, version
  **0.99 pre-release** is planned as the next public milestone

For detailed requirements, controls, parameters and troubleshooting see
**README_AMIGA.md** - the main port documentation.


## Target configuration

Recommended baseline target:

- **CPU:** Motorola 68060 with FPU (`raptor`), FPU-less 68EC060/68LC060
  (`raptor_nofpu` soft-float build), 68030 with an external 68881/68882
  FPU (`raptor_030_fpu`), or 68030 without an FPU (`raptor_030`
  soft-float build); PiStorm/Emu68 equivalent
- **Graphics:** RTG rendering on systems with a compatible RTG
  graphics card (for example CyberVision 64/3D, Picasso IV, or
  similar) through Picasso96 (P96) or CyberGraphX (CGX /
  cybergraphics.library). Native AGA rendering (`GFX=AGA`) is a
  separate full rendering path for Amiga 1200 and Amiga 4000; it can
  also be selected in WinUAE and compatible PiStorm/Emu68 setups when
  AGA is available. On WinUAE and PiStorm/Emu68, choose either RTG or
  AGA according to the system configuration.
- **Display mode:** 320x200, 8-bit paletted. In PAL mode, a black band may
  be visible at the bottom of the screen because the game uses a 320x200
  display area. To fill the screen vertically, select NTSC in Amiga Early
  Startup before booting; the game will then open full-screen at 320x200.
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
- **Any Amiga with PiStorm/Emu68 + RTG** (A500, A600, A1200, A2000, A3000, A4000 — all tested or expected to work)

### Native AGA display notes (PAL/NTSC)

- Native AGA mode is selected with `GFX=AGA`.
- The game always renders a fixed logical 320x200 image from the top-left.
- On a PAL-configured Amiga, the remaining lower native PAL display area may
  be black. This is normal and intentional.
- To use native NTSC timing so the 320x200 image fills the native NTSC screen
  vertically, the Amiga itself must be configured/booted in NTSC before
  starting the game.
- On AmigaOS 3.x: reset/reboot, hold both mouse buttons to open Early Startup
  Control, select NTSC in Display Options, boot AmigaOS, then start Raptor
  with `GFX=AGA`.
- `VIDEO=AUTO` preserves the system-selected native AGA mode.
- `VIDEO=NTSC` alone does **not** change a PAL-configured Amiga to NTSC; the
  Amiga itself must be booted in NTSC as described above.
- The game does not install, copy, delete, or modify anything under
  `DEVS:Monitors`.
- As Workbench ToolTypes, set them on separate lines:
  ```
  GFX=AGA
  VIDEO=AUTO
  ```

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

This repository is **not** a generic multi-platform fork. Its main purpose is to develop and maintain the **Amiga 68030/68060 port (RTG and AGA)**.

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
