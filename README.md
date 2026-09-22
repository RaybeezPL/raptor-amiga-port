# Raptor: Call of the Shadows — Amiga Port

This repository contains an AmigaOS 3.x port of **Raptor: Call of the Shadows**, based on the open-source reverse-engineered codebase by [skynettx/raptor](https://github.com/skynettx/raptor).

The port ships four dedicated m68k binaries for 68030 and 68060 systems, with and without an FPU. Graphics are rendered either through **RTG (Picasso96 or CyberGraphX)** or through a separate **native AGA** path. The game always renders a fixed logical **320x200, 8-bit paletted** image on its own screen.

| Binary | Target |
| --- | --- |
| `raptor` | 68060 with FPU |
| `raptor_nofpu` | 68060 without FPU (68EC060/68LC060 or a broken FPU), soft-float |
| `raptor_030_fpu` | 68030 with an external 68881/68882 FPU |
| `raptor_030` | 68030 without an FPU, soft-float |

## Download

Download the ready-to-run Amiga port:

- **[raptor.lha on Aminet](https://aminet.net/game/shoot/raptor.lha)**
- [GitHub Releases](https://github.com/RaybeezPL/raptor-amiga-port/releases)

The Amiga archive does not include the copyrighted original game data. To run the port, you must separately provide `FILE0000.GLB` through `FILE0004.GLB` from the full PC/DOS version 1.2, as explained in the next section.

## Required original game data

This Amiga port requires the original PC/DOS game data from the full version of **Raptor: Call of the Shadows, version 1.2**. The required game data is not included with this port, and the port will not run without it.

Copy `FILE0000.GLB` through `FILE0004.GLB` from your legally owned copy of the game. Detailed installation instructions are available in [README_AMIGA.md](README_AMIGA.md).

The original game is still available to purchase on Steam as [Raptor: Call of the Shadows (1994 Classic Edition)](https://store.steampowered.com/app/358360/Raptor_Call_of_the_Shadows_1994_Classic_Edition/).

The Steam purchase provides the original PC game, not the Amiga port.

## Project goals

The goals of this port are:

- Bring **Raptor: Call of the Shadows** to classic Amiga systems with a **68030 or 68060 CPU** (RTG or AGA graphics)
- Replace SDL-dependent parts of the engine with **native AmigaOS implementations**
- Keep the rendering path efficient for Amiga RTG hardware, avoiding unnecessary format conversion and slow per-pixel drawing paths
- Make the codebase practical for testing on both **WinUAE** and **real hardware**
- Document the porting process in a clean and reproducible way

## Current status

Current version: **0.9.9-rc.4** — release candidate.

Working:

- Four native **m68k AmigaOS cross-builds**: `raptor` (68060 + FPU),
  `raptor_nofpu` (soft-float, for FPU-less 68EC060/68LC060 or a broken
  FPU), `raptor_030_fpu` (68030 + external 68881/68882 FPU) and
  `raptor_030` (soft-float 68030)
- Full gameplay on real hardware — tested on A2000 with CyberVision 64/3D, A1200 + PiStorm/Emu68 (display output verified via both RTG and AGA), and WinUAE; expected to work on other Amiga models with PiStorm/Emu68 + RTG
- Keyboard, mouse and joystick/CD32 pad input working simultaneously
- RTG video path for **320x200x8-bit** output on a dedicated screen;
  tries P96 (Picasso96) first, falls back to CGX (CyberGraphX), then
  falls back to a 320x240x8 screen with a black band at the bottom on
  RTG cards that lack 320x200
- **`GFX=AUTO|RTG|AGA`** parameter — controls the graphics driver path
  (CLI: `-gfx=RTG`; icon ToolType: `GFX=RTG`)
- Accelerated frame presentation on every display path: Picasso96 uses
  the driver's own `p96WritePixelArray`, CyberGraphX uses CGX
  `WritePixelArray`, and the `GFX=AGA` chipset screen uses a custom
  chunky-to-planar converter — native AGA rendering has been optimized
  by caching the converted AGA bitmap used for C2P blits, reducing
  unnecessary conversion work when the game frame does not change — all
  replacing the generic `WriteChunkyPixels` OS conversion (kept as
  fallback)
- **`MUSIC=OFF|ADLIB|CAMD|MHI|WAVE`** parameter — selects the music
  backend: built-in AdLib/OPL3 emulation, General MIDI via CAMD, MP3
  files from the `MP3/` drawer in the game directory via an MHI hardware
  decoder, pre-decoded WAV files from the `WAVE/` drawer mixed into the
  AHI stream, or no music. No music backend is enabled by default:
  without a `MUSIC=` option Raptor uses `MUSIC=OFF` and initializes no
  music backend (CLI: `-music=mhi`; icon ToolType: `MUSIC=MHI`). For
  `MUSIC=MHI`, optional `MHIDRIVER=<driver>` selects a specific decoder.
  `MP3PRELOAD=OFF` is the default and uses the original MHI streaming
  path, streaming the MP3 during playback. `MP3PRELOAD=ON` loads the
  complete MP3 into memory before playback; playback then uses resident
  MP3 data with no MP3 disk I/O during playback, independently of the
  selected MHI driver (CLI: `-music=mhi -mp3preload=on`; Workbench
  ToolTypes: `MUSIC=MHI` and `MP3PRELOAD=ON`). MHI strips ID3v2/ID3v1
  metadata before decoding and recognizes the Prisma MegaMix,
  Amiblaster, Prelude/MPEGit, MAS Player, ArmedWarp, mpeg.device and
  MNT ZZ9000 driver families. For WARP MHI cards, `MP3PRELOAD=ON` is
  recommended: normal streaming produced audible clicks during track
  changes on the tested Amiga IDE + WARP setup, while preload eliminated
  those clicks in that setup. This observation does not imply identical
  behavior on every WARP configuration. Prisma MegaMix, Armed WARP, and
  MNT ZZ9000 / ZZ9000AX are all real-hardware verified configurations.
  MNT ZZ9000 / ZZ9000AX is recognized via
  `LIBS:MHI/mhizz9000.library` and reported as "MNT ZZ9000". MHI
  initialization and MP3 playback were confirmed on a real Amiga 4000
  with ZZ9000 + ZZ9000AX.
  `MHIDRIVER=` and driver auto-detection are case-insensitive for
  drivers in `LIBS:MHI/`: the requested path is tried first, then the
  `LIBS:MHI/#?.library` scan matches basenames case-insensitively and
  opens the driver under its exact filename (e.g.
  `MHIDRIVER=mhiArmedWarp.library` can resolve to
  `LIBS:MHI/mhiArmedWARP.library`)
- **`AHIUNIT=0|1|2|3`** parameter — optional ahi.device unit selection
  for the game's AHI sound output. It selects which ahi.device unit
  Raptor uses for game audio / sound effects (default: unit 0). Valid
  units: 0, 1, 2, 3. It does not select an MHI decoder and does not
  change `MUSIC=MHI` (CLI: `-ahiunit=1`; icon ToolType: `AHIUNIT=1`)
- **`MOUSE=ON|OFF`** / **`NOMOUSE`** and **`JOYSTICK=ON|OFF`** / **`NOJOY`**
  parameters — enable/disable the mouse and joystick input devices (CLI:
  `-mouse=off`, `-nomouse`, `-joystick=off`, `-nojoy`; icon ToolTypes:
  `MOUSE=OFF`, `NOMOUSE`, `JOYSTICK=OFF`, `NOJOY`). Both default to ON.
  An explicit `KEYWORD=value` wins over the legacy bare flag. Disabling an
  input device is a performance/troubleshooting option: with the mouse off
  the window registers no mouse events at all, and with the joystick off
  the game port is never polled.
- Workbench icon ToolTypes (NOSOUND/NOMUSIC/NOJOY/NOMOUSE/GFX/VIDEO/MUSIC/
  AHIUNIT/MHIDRIVER/MP3PRELOAD/JOYSTICK/MOUSE) via the official WBStartup + icon.library
  mechanism
- Clean startup banner and parameter output on Shell/CLI; on Workbench
  launches no console window is opened at all (nothing is left behind
  when the game exits)
- Phantom-input filtering hardened for PiStorm/Emu68 machines: the
  middle-mouse button is dropped on every display path (RTG and AGA) and
  the gameport is polled no more than approximately 50 times per second with a clear-read unmask — fixes
  intro/demo skipping and erratic steering
- English requester with troubleshooting info when RTG mode is required
  but unavailable (no silent fallback to AGA)
- **Sound effects through AHI** (ahi.device): 11025 Hz 16-bit stereo -
  the native rate of the game's samples - streamed by a dedicated audio
  task using the canonical double-buffered CMD_WRITE scheme. The AHI
  callback buffer is 1024 frames (~93 ms at 11025 Hz) with
  `MUSIC=ADLIB` or `MUSIC=MHI` for additional scheduling/underrun
  headroom, and 512 frames (~46 ms) with `MUSIC=CAMD`, `MUSIC=WAVE`,
  `MUSIC=OFF` or `-nomusic`. This changes callback/buffer granularity
  only, not the number of audio frames processed per second
- **Persistent audio volumes** via `amiga.cfg` in the game directory
  (created on first run): separate startup volumes for AdLib/OPL3
  music, MHI/MP3 music, WAVE music and sound effects; the in-game
  Options sliders write their values back to it
- **Automatic 64 KB main-process stack** on Amiga: Raptor requests a
  minimum 65536-byte main stack through the libnix `__stack` /
  swapstack startup mechanism before `main()`, so CLI users no longer
  need to execute `Stack 65536` manually; if Shell/Workbench already
  provides a larger stack, it is preserved

Work still in progress / roadmap:

- Fine-tuning and performance polish on real 68k hardware

For detailed requirements, controls, parameters and troubleshooting see
**README_AMIGA.md** - the main port documentation.

## Demo / Gameplay

Gameplay footage recorded on Amiga hardware:

[![Raptor Amiga Port - gameplay](https://img.youtube.com/vi/C9Q2ygClWMI/hqdefault.jpg)](https://youtu.be/C9Q2ygClWMI)

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
  separate full rendering path for AGA-capable hardware, primarily
  Amiga 1200 and Amiga 4000; it can also be selected in WinUAE and
  compatible PiStorm/Emu68 setups when AGA is available. On WinUAE and
  PiStorm/Emu68, choose either RTG or AGA according to the system
  configuration.
- **Display mode:** the game renders a fixed logical 320x200, 8-bit
  paletted image. On RTG the physical screen is 320x200 when the driver
  offers it, otherwise 320x240 with the 320x200 image shown 1:1 from the
  top-left corner and a black band at the bottom. On the native AGA path
  in PAL mode, a black band may be visible at the bottom of the screen
  because the game uses a 320x200 display area; use `VIDEO=NTSC` with
  `GFX=AGA` to open the screen directly in NTSC mode (see Native AGA
  display notes below).
- **RAM:** 4 MB Fast RAM minimum (8 MB recommended) + 2 MB Chip RAM
  (the game itself uses roughly 3 MB of Fast RAM; on RTG the screen
  bitmap lives in graphics card memory, so Chip RAM is only needed by
  the OS)
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
- **Amiga with PiStorm/Emu68 + RTG** (A500, A600, A1200, A2000, A3000, A4000 — tested or expected to work)

### Tested configurations

- Raspberry Pi 400 running PiMIGA, tested in both AGA and RTG modes.
- Amiga 1200 with PiStorm running CaffeineOS, tested in AGA PAL, AGA NTSC and RTG modes.
- Amiga 1200 with TerribleFire 1260 (68060LC 50MHz no FPU, AGA PAL/NTSC, 128MB Fast RAM, Kickstart 3.2.3), tested with MUSIC=WAVE.
- Amiga 1200 with Mediator, Blizzard 1260, Voodoo3 and Prelude audio on the clock port, including MHI playback.
- Amiga 2000 with a TekMagic 68060 at 50 MHz, CyberVision 64/3D and Prisma MegaMix.
- Amiga 4000 with a 68060 at 50 MHz, Picasso IV and AGA graphics; WAVE music, MIDI/CAMD and MHI were tested.
- Amiga 4000 with ZZ9000 + ZZ9000AX; MHI MP3 playback verified using the MHI driver for the ZZ9000AX card.
- Amiga 1200 with Blizzard 1260 at 56 MHz, Mediator, Voodoo3 and AmigaOS 3.2.3; MUSIC=WAVE verified.
- WinUAE with 68030 and 68060 configurations, both with and without FPU, tested in AGA and RTG modes.
- Tested on AmigaOS 3.1.4, 3.2, 3.2.2, 3.2.3 and 3.9.

### Native AGA display notes (PAL/NTSC)

Native AGA mode is selected with `GFX=AGA`.

The game always renders a fixed logical **320×200** image from the top‑left corner of the screen. On a PAL‑configured Amiga, the remaining lower part of the native PAL display area may appear black. This is normal and intentional.

`VIDEO=AUTO` (default) uses the system‑selected native AGA display mode.

`VIDEO=NTSC` explicitly requests the native NTSC low‑resolution display mode used by the game. With `GFX=AGA` the game opens its **320×200×8** AGA screen directly in NTSC timing, using `SA_DisplayID=NTSC_MONITOR_ID|LORES_KEY` (native NTSC low‑res), so the 320×200 image fills the entire screen vertically. No reboot is required, nothing needs to be changed in `DEVS:Monitors`, and no Early Startup Display Options are needed.

`VIDEO=PAL` is accepted but currently does not force a PAL ModeID; the default native AGA mode (AUTO) is used instead.

`VIDEO=` has no effect on the RTG display path: with `GFX=RTG` (or `GFX=AUTO`) the option is ignored and the RTG mode is used. `VIDEO=NTSC` affects only the native AGA path.

As Workbench ToolTypes, set them on separate lines:

```text
GFX=AGA
VIDEO=NTSC
```

## Build environment

Primary development environment:

- **Host OS:** Windows 11
- **Build environment:** Ubuntu under WSL2
- **Toolchain:** `m68k-amigaos-gcc` / `m68k-amigaos-g++`
- **Main branch:** `main`

The current porting workflow focuses on practical iteration speed, reproducible cross-builds, and fast emulator-to-real-hardware testing.

## Assets and legal note

This repository and the Amiga release archive do **not** include the original game data files. You must provide your own legal copy of the original **Raptor: Call of the Shadows** data files; see [Required original game data](#required-original-game-data) above.

## License

Raptor Amiga Port is distributed under the GNU General Public License,
Version 2, June 1991. The full license text is available in
[LICENSE](LICENSE).

This port is based on the open-source reverse-engineered codebase from
[skynettx/raptor](https://github.com/skynettx/raptor). Applicable
copyright and license notices from upstream and bundled components are
preserved.

## Upstream project

This Amiga port is based on the reverse-engineered open-source project:

- [skynettx/raptor](https://github.com/skynettx/raptor)

That upstream project reconstructs the original game engine in C/C++ and made this Amiga port possible. For the original multi-platform project documentation, installation notes, and upstream release information, refer to the upstream repository.

## Scope of this fork

This repository is **not** a generic multi-platform fork. Its main purpose is to develop and maintain the **Amiga 68030/68060 port (RTG and AGA)**.

Platform-specific notes for Windows, Linux, macOS, and Android from the original upstream project are not the focus of this fork and may differ from the current upstream README.

## Credits

- **[nukeykt](https://github.com/nukeykt)** and contributors involved in the reverse-engineered Raptor codebase
- **[skynettx](https://github.com/skynettx)** for the open-source C/C++ recreation used as the base for this port
- The Amiga community, emulator authors, and RTG/AHI toolchain developers
- The testers from PPA.PL for their invaluable feedback and support during development
- Special thanks to PPA user [Jacques](https://www.ppa.pl/uzytkownicy/792) for his patience and extensive MHI testing on WARP hardware. His testing made MP3PRELOAD possible.
- Special thanks also to PPA users [vojo](https://www.ppa.pl/uzytkownicy/5586), [BULI](https://www.ppa.pl/uzytkownicy/1916), [Mokry](https://www.ppa.pl/uzytkownicy/124) and [AD99](https://www.ppa.pl/uzytkownicy/7236) for their testing, feedback and support during development.
