# Raptor: Call of the Shadows — Amiga Port (68030/68060 & EC/LC, RTG/AGA, AHI/MHI/CAMD/WAVE)

This repository contains an AmigaOS 3.x port of **Raptor: Call of the Shadows**, based on the open-source reverse-engineered codebase by [skynettx/raptor](https://github.com/skynettx/raptor).

The port targets **68060-class Amiga systems** — both full **68060 with FPU** and FPU-less **68EC060 / 68LC060** (dedicated soft-float binary) — as well as **68030 systems**, covered by two separate dedicated binaries: **68030 with an external 68881/68882 FPU** (`raptor_030_fpu`) and **68030 without an FPU** (`raptor_030`, soft-float) — plus faster 68k hardware, with **RTG graphics (Picasso96 / CyberGraphX)** or the **native AGA chipset** (`GFX=AGA`), with primary testing and optimization aimed at:

- **Any Amiga system with a compatible RTG graphics card** — for example **CyberVision 64/3D**, **Picasso IV**, or similar — using **Picasso96** or **CyberGraphX**
- **Amiga A500 / A600 / A1200 / A2000 / A3000 / A4000 with PiStorm / Emu68** — RTG or native AGA, according to the configured environment
- **Amiga A1200 / A4000** — native AGA rendering (`GFX=AGA`); in **WinUAE** and compatible PiStorm/Emu68 setups either RTG or AGA can be selected when AGA is available
- **AmigaOS 3.2**
- **AHI audio for sound effects**
- **MHI audio for MP3 music**
- **CAMD MIDI music**
- **WAVE audio for WAV music**

AmigaOS 3.1.4, 3.2, 3.2.2, 3.2.3 and 3.9 are tested and known to work.
Other AmigaOS versions may also work, but have not been fully verified.

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

Current version: **0.9.9-rc.1** — the final pre-release build before the planned release.

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
- **`MUSIC=ADLIB|CAMD|MHI|WAVE|OFF`** parameter — selects the music backend:
  built-in AdLib/OPL3 emulation, General MIDI via CAMD, MP3 files from
  the `MP3/` drawer via an MHI hardware decoder, pre-decoded WAV files
  from the `WAVE/` drawer mixed into the AHI stream, or no music. No
  music backend is enabled by default on Amiga: without a `MUSIC=`
  option Raptor uses `MUSIC=OFF` and initializes no AdLib/OPL3, CAMD,
  MHI or WAVE music backend — enable music explicitly with
  `MUSIC=ADLIB`, `MUSIC=MHI`, `MUSIC=CAMD` or `MUSIC=WAVE` (CLI:
  `-music=CAMD`; icon ToolType: `MUSIC=CAMD`)
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
- **Music: no backend enabled by default** — without a `MUSIC=` option
  Raptor uses `MUSIC=OFF` and initializes no music backend; enable
  music explicitly with `MUSIC=ADLIB` (lightweight DOSBox dbopl OPL3
  emulation core, only a few percent of a 68060, mixed into the AHI
  stream), **General MIDI via CAMD** (camd.library) with `MUSIC=CAMD`
  for external synths / CAMD software synths (cluster "out.0"; needs a
  configured MIDI driver or synth), **MP3 music via MHI**
  (`MUSIC=MHI`) for a Prisma Megamix / MAS / Delfina hardware decoder,
  with files in the game's `MP3/` drawer, or **pre-decoded WAVE music**
  (`MUSIC=WAVE`) with WAV files in the game's `WAVE/` drawer mixed
  into the AHI stream
- **Persistent audio volumes** via `amiga.cfg` in the game directory
  (created on first run): separate startup volumes for AdLib/OPL3
  music, MHI/MP3 music, WAVE music and sound effects; the in-game
  Options sliders write their values back to it


Work still in progress / roadmap:

- Fine-tuning and performance polish on real 68k hardware
- Version **0.9.9-rc.1** is the final pre-release build before the
  planned release

For detailed requirements, controls, parameters and troubleshooting see
**README_AMIGA.md** - the main port documentation.

## Demo / Gameplay

Gameplay footage recorded on Amiga hardware:

<iframe width="560" height="315"
  src="https://www.youtube.com/embed/C9Q2ygClWMI"
  frameborder="0"
  allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture"
  allowfullscreen>
</iframe>

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

- **OS:** AmigaOS 3.2 (AmigaOS 3.1.4, 3.2, 3.2.2, 3.2.3 and 3.9 are
  tested and known to work; other versions may also work but have not
  been fully verified)
- **Audio:** AHI (ahi.device) for sound effects; music requires an
  explicit backend selection with `MUSIC=ADLIB`, `MUSIC=MHI`,
  `MUSIC=CAMD` or `MUSIC=WAVE` (without a `MUSIC=` option Raptor uses
  `MUSIC=OFF`); camd.library (CAMD) optional for MIDI music output
  (MUSIC=CAMD)

Current development and testing is mainly aimed at systems such as:

- **CyberVision 64/3D**
- **Picasso IV**
- **Any Amiga with PiStorm/Emu68 + RTG** (A500, A600, A1200, A2000, A3000, A4000 — all tested or expected to work)

### Tested configurations

- Raspberry Pi 400 running PiMIGA, tested in both AGA and RTG modes.
- Amiga 1200 with PiStorm running VaffeineOS, tested in AGA PAL, AGA NTSC and RTG modes.
- Amiga 1200 with Mediator, Blizzard 1260, Voodoo3 and Prelude audio on the clock port, including MHI playback.
- Amiga 2000 with a TekMagic 68060 at 50 MHz, CyberVision 64/3D and Prisma MegaMix.
- Amiga 4000 with a 68060 at 50 MHz, Picasso IV and AGA graphics; WAVE music, MIDI/CAMD and MHI were tested.
- WinUAE with 68030 and 68060 configurations, both with and without FPU, tested in AGA and RTG modes.
- Tested on AmigaOS 3.1.4, 3.2, 3.2.2, 3.2.3 and 3.9.

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

## License

Raptor Amiga Port is distributed under the GNU General Public License,
Version 2, June 1991. The full license text is available in
[LICENSE](LICENSE).

This port is based on the open-source reverse-engineered codebase from
[skynettx/raptor](https://github.com/skynettx/raptor). Applicable
copyright and license notices from upstream and bundled components are
preserved.

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
