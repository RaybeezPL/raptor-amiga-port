# Raptor — AmigaOS 3.x Port: Architecture, Build and Compatibility Notes

> **Status notice.** This document originated in July 2026 as a pre-porting
> analysis for an AmigaOS 3.x / m68k port of Raptor. The Amiga build path is
> now **implemented** (port version 0.9.6_MHI at the time of writing). The
> text has been revised to separate current facts from remaining work and
> from historical design notes. For actual build behavior, the following are
> authoritative and take precedence over anything written here:
>
> - `CMakeLists.txt` (CMake Amiga build, `RAPTOR_AMIGA`)
> - `Makefile.amiga` (primary make-based Amiga build)
> - `build_amiga.sh`, `build_amiga_nofpu.sh`, `build_amiga_030.sh`,
>   `build_amiga_030_fpu.sh` (build profiles)
> - the current source tree, especially `src/amiga/`
>
> User-facing documentation lives in `README_AMIGA.md` (English) and
> `README_AMIGA_PL.md` (Polish); the change history is in `CHANGELOG.md`.

---

## 1. Current Port Status

The facts below were verified against the repository source. Where a feature
exists in code but has not been confirmed on real Amiga hardware, it is
marked **requires validation**.

### 1.1 Build system

- The Amiga build is selected in `CMakeLists.txt` with `-DRAPTOR_AMIGA=ON`,
  or detected automatically when `CMAKE_SYSTEM_NAME` matches `Amiga` or
  `CMAKE_SYSTEM_PROCESSOR` matches `m68k` (a cross toolchain file sets these).
- The Amiga build **must not use external SDL2**. It compiles with the
  in-tree SDL2 compatibility/stub layer in `src/amiga/`
  (`SDL.h` redirects to `amiga_sdl_stubs.h`, enabled with
  `-DUSE_SDL_STUBS`). `find_package(SDL2)` and `SDL2::SDL2` are host-only.
- The Amiga source list and the host source list are intentionally
  different. The Amiga list includes the CAMD MIDI backend (`mpucamd.cpp`),
  the MHI MP3 backend (`mpumhi.cpp`), the DOSBox dbopl OPL core
  (`dbopl.cpp` + `opl3dbopl.cpp` shim) and the Amiga-specific
  `amiga_cfg.cpp` / `amiga_stubs_impl.cpp`. It excludes the Nuked OPL3 core
  (`opl3.cpp`) and the Windows/Linux/macOS MIDI backends
  (`mpuwinmm.cpp`, `mpualsa.cpp`, `mpucorea.cpp`, `mpucorem.cpp`).
- The SDL2/textscreen-based `raptorsetup` target is **not built** on Amiga.
  The `src/setup/` directory and `include/textscreen/` are not part of this
  repository tree; the host build warns when textscreen is absent.
- `Makefile.amiga` is the primary build path; the CMake Amiga branch mirrors
  its source list and definitions. Note the optimization levels differ:
  `Makefile.amiga` uses `-O2`, while `CMakeLists.txt` sets `-O3` globally
  (see section 5).

### 1.2 Build profiles

Four build profiles exist, defined by `Makefile.amiga` and driven by the
`build_amiga*.sh` scripts:

| Target           | Invocation                                   | CPU / FPU                          | Compile flags                  | Link flags             |
|------------------|----------------------------------------------|------------------------------------|--------------------------------|------------------------|
| `raptor`         | `make -f Makefile.amiga` / `build_amiga.sh`  | 68060 + on-die FPU                 | `-m68060 -m68881`              | `-m68060 -m68881`      |
| `raptor_nofpu`   | `NOFPU=1` / `build_amiga_nofpu.sh`           | 68060 without FPU (68EC060/68LC060), soft-float | `-m68060 -msoft-float` | `-m68000 -msoft-float` |
| `raptor_030_fpu` | `CPU=030 NOFPU=0` / `build_amiga_030_fpu.sh` | 68030 + external 68881/68882       | `-m68030 -m68881`              | `-m68030 -m68881`      |
| `raptor_030`     | `CPU=030 NOFPU=1` / `build_amiga_030.sh`     | 68030 without FPU, soft-float      | `-m68030 -msoft-float`         | `-m68000 -msoft-float` |

- The soft-float variants link with `-m68000 -msoft-float` because the bebbo
  gcc 6.5.0b toolchain has no soft-float multilib; linking as m68000 selects
  the base libnix `libm.a` whose float-math thunks go through
  `mathieeedoubbas.library` instead of returning through the FPU register
  `fp0` (see the rationale comment in `Makefile.amiga`).
- `build_amiga_nofpu.sh` and `build_amiga_030.sh` verify with
  `m68k-amigaos-objdump` that the binary contains **zero FPU instructions**;
  the 030 scripts additionally reject 68040/68060-only instructions
  (`byterev`/`bitrev`). `build_amiga_030_fpu.sh` expects FPU instructions
  but still rejects 68040/68060-only CPU instructions.
- The scripts clean all generated `*.o`/`*.d` files before each build and
  currently `cd` to a hardcoded path (`/mnt/c/amiga-raptor/raptor`).
- In CMake, the CPU/link flags are cache variables
  (`RAPTOR_AMIGA_CPU_FLAGS`, `RAPTOR_AMIGA_LINK_FLAGS`, default
  `-m68060 -m68881`) so a no-FPU build can override them.

### 1.3 Video

Implemented in the SDL stub layer (`src/amiga/amiga_sdl_stubs.h`) behind the
game's normal `i_video.cpp` flow:

- Fixed logical resolution 320x200x8; the game opens its own screen.
- `GFX=AUTO|RTG|AGA` selection (CLI and Workbench ToolTypes):
  - AUTO/RTG require RTG: Picasso96 first, then CyberGraphX (CGX) fallback;
    320x200x8 first, then 320x240x8 with a 40-row letterbox. On failure an
    English requester is shown and the game does not start (no silent
    fallback to AGA).
  - AGA forces a native chipset 320x200x8 custom screen.
- Four frame-blit paths, selected once at screen open and logged
  (`[VIDEO] blit path: ...`): `p96WritePixelArray` (P96), CGX
  `WritePixelArray` (RECTFMT_LUT8), a custom 68060 chunky-to-planar (C2P)
  converter for AGA (verified bit-exact against a brute-force reference in a
  host-side test), and `WriteChunkyPixels` as the generic fallback.
- Palette via `LoadRGB32`; the system pointer is hidden during play.
- `i_video.cpp` skips `CreateUpscaledTexture()` and no-op renderer calls
  under `__AMIGA__`; presentation goes through `SDL_LowerBlit()` +
  `SDL_RenderPresent()` in the stubs.
- Tested on real hardware with Picasso96 RTG (Amiga 2000, 68060, AmigaOS
  3.2.1, per `CHANGELOG.md` 0.9.0) and exercised on PiStorm/Emu68.
  **CGX and AGA paths on real hardware: requires validation.**

### 1.4 Input

- Keyboard and mouse via a real IDCMP event pump in the stubs
  (`IDCMP_RAWKEY`, `IDCMP_MOUSEMOVE`, `IDCMP_MOUSEBUTTONS`), with a full SDL
  scancode table and raw-key mapping; keyboard, mouse and joystick work
  simultaneously.
- Joystick/CD32 pad via `lowlevel.library` `ReadJoyPort()` (port 1), polled
  in `SDL_PumpEvents` at most at ~50 Hz; the polled state is shared with
  `joyapi.cpp`/`input.cpp` through the single-instance globals owned by
  `amiga_stubs_impl.cpp`.
- `MOUSE=ON|OFF` / `NOMOUSE` and `JOYSTICK=ON|OFF` / `NOJOY` parameters
  disable per-device processing (performance/troubleshooting aids); an
  explicit `KEYWORD=value` wins over the legacy bare flag.
- The middle mouse button is ignored by design (phantom-button filtering on
  some machines, e.g. PiStorm/Emu68); special-weapon cycling is on SPACE.
- Keyboard and mouse are exercised on real hardware and emulator; joystick
  behavior beyond the standard DB9/CD32 polling code: **requires
  validation** on the specific device set used by players.

### 1.5 Audio and music

- Sound effects: AHI (`ahi.device` v4+), 11025 Hz 16-bit stereo — the
  native rate of the game's samples — streamed by a dedicated
  high-priority audio task using the canonical double-buffered `CMD_WRITE`
  scheme. The task owns the entire device side (port, IO requests,
  OpenDevice/CloseDevice). The official vendored `devices/ahi.h` header is
  enforced with compile-time layout guards.
- Music, default: built-in AdLib/OPL3 emulation mixed into the AHI stream,
  using the lightweight DOSBox **dbopl** core (`dbopl.cpp` +
  `opl3dbopl.cpp` shim) instead of the Nuked OPL3 core, which stalled a real
  68060.
- Music, opt-in `MUSIC=CAMD`: General MIDI event stream through
  `camd.library` to the fixed cluster `out.0` (`mpucamd.cpp`); silent unless
  a MIDI driver/synth is attached; automatic fallback to AdLib/OPL3.
- Music, opt-in `MUSIC=MHI`: MP3 files from the `MP3/` drawer through an MHI
  decoder driver (`mpumhi.cpp`; Prisma Megamix, MAS Player, Prelude MPEGit,
  Delfina/mpeg.device, or any driver in `LIBS:MHI/`); `MHIDRIVER=` overrides
  driver auto-detection; falls back to AdLib/OPL3.
- `NOSOUND` / `NOMUSIC` parameters; persistent volumes (music_adlib,
  music_mhi, sfx) stored in `amiga.cfg` (`src/amiga/amiga_cfg.cpp/h`).
- AHI sound effects and AdLib music were tested on real hardware (0.9.0);
  CAMD and MHI paths are implemented and documented but depend on external
  MIDI/MHI hardware: **requires validation** on those setups.

### 1.6 Timer

- `SDL_GetTicks()` is implemented with AmigaOS `DateStamp()` (minute/tick
  resolution, ~20 ms granularity); `SDL_Delay()` uses DOS `Delay()`
  (1/50 s ticks). This is coarser than `timer.device` microsecond timing but
  has been sufficient for the game loop so far; finer timing
  **requires validation** if future features need it.

### 1.7 Filesystem and configuration

- The Amiga build has **no SETUP.INI support at all**: `src/prefapi.cpp/h`
  was removed (changelog 0.9.2) and all preferences use built-in defaults
  plus command-line parameters / icon ToolTypes.
- `loadsave.cpp` (`RAP_InitLoadSave`) uses the current directory on Amiga
  (the `SDL_GetPrefPath()` path is compiled only for
  `_WIN32`/`__linux__`/`__APPLE__`); saved games and `amiga.cfg` live in the
  game directory, which must be writable.
- `SDL_filesystem.h` in `src/amiga/` is a redirect header; no filesystem
  stub functions are needed on the Amiga path.
- The game requires the five GLB data files (`FILE0000.GLB` ..
  `FILE0004.GLB`) from the full version 1.2; `GLB_InitSystem()` no longer
  looks for the obsolete `FILE0005.GLB`.

### 1.8 Validation status summary

| Area | Code status | Real-hardware status |
|------|-------------|----------------------|
| Build (68060 + FPU, Makefile) | Implemented | Tested (A2000, 68060, OS 3.2.1, P96) |
| Build (soft-float / 030 profiles) | Implemented, instruction checks in scripts | WinUAE-tested; real 030/EC060 iron: requires validation |
| CMake Amiga build | Implemented | Requires validation (Makefile is the primary path) |
| Video P96 RTG | Implemented | Tested |
| Video CGX fallback | Implemented | Requires validation |
| Video AGA + C2P | Implemented (C2P bit-exact verified host-side) | Requires validation |
| Keyboard / mouse | Implemented | Tested |
| Joystick / CD32 pad | Implemented | Requires validation across devices |
| AHI SFX + AdLib/dbopl music | Implemented | Tested |
| CAMD MIDI | Implemented | Requires validation (needs MIDI driver/synth) |
| MHI MP3 | Implemented | Requires validation (needs MHI hardware decoder) |
| Save/load, `amiga.cfg` | Implemented | Tested (basic flow) |

---

## 2. Repository Structure — Overview

Raptor is a classic top-down shoot'em'up, originally written for DOS and
reconstructed for modern systems using SDL2 (upstream skynettx/raptor). The
code is C/C++ (C++11). The Amiga port builds a subset of this tree with the
in-tree SDL stub layer.

### 2.1 Main source modules (`src/`)

| Module | Files | Amiga notes |
|--------|-------|-------------|
| Graphics engine | `gfxapi.cpp/h`, `gfxapi_a.cpp` | 320x200x8 software renderer (chunky pixels), unchanged |
| Video layer | `i_video.cpp/h` | `__AMIGA__` guards; upscale path skipped; blit via stubs |
| GLB file system | `glbapi.cpp/h` | Custom asset format (.GLB), encryption, cache |
| Virtual memory | `vmemapi.cpp/h` | Custom memory manager with LRU eviction |
| Sound | `fx.cpp/h`, `dspapi.cpp/h` | Software mixer; on Amiga 11025 Hz into AHI |
| Music | `musapi.cpp/h`, `i_oplmusic.cpp` | MUS format; Amiga uses dbopl OPL3, CAMD or MHI |
| OPL core (Amiga) | `dbopl.cpp/h`, `opl3dbopl.cpp/h` | DOSBox dbopl behind an opl3.h-compatible shim |
| OPL core (host) | `opl3.cpp/h`, `opl3dbopl.cpp` | Nuked OPL3 — not built on Amiga |
| MIDI backends | `mpucamd.cpp` (Amiga), `mpumhi.cpp` (Amiga), `mpuwinmm/mpualsa/mpucorea/mpucorem.cpp` (host) | Only CAMD and MHI are built on Amiga |
| TinySoundFont | `mputsf.cpp`, `include/TinySoundFont/tsf.h` | Compiled on Amiga but deliberately unused (see `Makefile.amiga` comments) |
| Input | `kbdapi.cpp`, `joyapi.cpp`, `ptrapi.cpp`, `ptrapi_a.cpp`, `input.cpp` | SDL event API backed by the Amiga stubs |
| Window/GUI system | `swdapi.cpp`, `windows.cpp` | In-game UI with data from GLB |
| Game logic | `rap.cpp`, `enemy.cpp`, `shots.cpp`, `tile.cpp`, `bonus.cpp`, etc. | Pure logic, minimal platform dependencies |
| Save/load | `loadsave.cpp/h` | Explicit byte-order serialization; Amiga uses the game directory |
| Demo | `demo.cpp/h` | Gameplay recording/playback (`REC`/`PLAY`) |
| Intro animations | `intro.cpp`, `movie.cpp/h`, `movie_a.cpp` | Animated cutscene sequences |
| Amiga port layer | `src/amiga/` (see section 3) | SDL stubs, config, vendored headers |
| Settings (INI) | — | `prefapi.cpp/h` removed; no SETUP.INI on Amiga |
| Textscreen (setup) | — | Not present in this tree; `raptorsetup` not built on Amiga |

---

## 3. `src/amiga/` — Actual Port Layer

```
src/amiga/
├── amiga_sdl_stubs.h      # Minimal SDL2 stub layer (video, input, audio,
│                          # timer, byte order); active with USE_SDL_STUBS
├── amiga_stubs_impl.cpp   # Single TU that owns all shared stub globals
│                          # (AMIGA_STUBS_OWNER pattern)
├── amiga_cfg.cpp/.h       # Persistent audio volumes in amiga.cfg
├── SDL.h                  # Redirect -> amiga_sdl_stubs.h
├── SDL_endian.h           # Redirect -> amiga_sdl_stubs.h
├── SDL_filesystem.h       # Redirect header (no filesystem stubs needed)
├── SDL_opengl.h           # Empty redirect (OpenGL not used on Amiga)
├── devices/ahi.h          # Vendored official AHI SDK header
├── midi/camd.h, midi/mididefs.h   # Vendored CAMD developer-kit headers
├── clib/camd_protos.h, clib/mhi_protos.h
├── inline/camd.h, inline/mhi.h    # Hand-written GCC inline stubs (LVOs
│                                  # verified against the official .fd files)
├── proto/camd.h, proto/mhi.h      # Hand-written NDK-style prototypes
├── libraries/mhi.h        # Vendored MHI developer-kit v1.2 header
├── CAMD-HEADERS.txt       # Provenance notes for the CAMD headers
└── MHI-HEADERS.txt        # Provenance notes for the MHI headers
```

Key implementation points in `amiga_sdl_stubs.h`:

- **Video:** `Amiga_OpenGameScreen()` implements the GFX=AUTO/RTG/AGA
  selection, exact P96/CGX mode matching from the live mode lists (never a
  native chipset ModeID on the RTG paths), the 320x240x8 letterbox fallback,
  and the blit-path selection (`AmigaBlitMode`).
- **Input:** `SDL_PumpEvents()` drains the IDCMP port and translates
  IntuiMessages into SDL events; joystick polling uses `ReadJoyPort(1)` with
  phantom-input debouncing (50 Hz cap, masked-line re-enable delay).
- **Audio:** `SDL_OpenAudio()`/`SDL_OpenAudioDevice()` spawn the
  "Raptor Audio Task", which owns ahi.device and streams double-buffered
  `CMD_WRITE` requests; `SDL_CloseAudio()` is idempotent and waits for the
  task to stop.
- **Byte order:** `SDL_BYTEORDER` is `SDL_BIG_ENDIAN`; `SDL_SwapLE16/32`
  perform real byte swaps, `SDL_SwapBE*` are no-ops.
- **Timer:** `SDL_GetTicks()` via `DateStamp()`, `SDL_Delay()` via `Delay()`.

---

## 4. Endianness (Little Endian -> Big Endian)

### 4.1 Mechanism (verified current)

`src/entypes.h` maps the game's little-endian accessors onto SDL swap
macros:

```c
#include "SDL_endian.h"
#define LE_USHORT(x) SDL_SwapLE16(x)
#define LE_SHORT(x)  (signed short) SDL_SwapLE16(x)
#define LE_ULONG(x)  SDL_SwapLE32(x)
#define LE_LONG(x)   (signed int) SDL_SwapLE32(x)
```

On Amiga, `SDL_endian.h` redirects to `amiga_sdl_stubs.h`, which defines
`SDL_BYTEORDER = SDL_BIG_ENDIAN` and implements `SDL_SwapLE16/32` as real
byte swaps (no-ops would only occur on little-endian hosts). The mechanism
described in the original analysis is therefore intact and active.

### 4.2 Where the LE_* macros are used

The `LE_LONG`, `LE_SHORT`, etc. macros are used throughout the code that
reads binary data from `.GLB` files (game assets in DOS little-endian
format):

- **GLB file system** (`glbapi.cpp`): `KEYFILE` headers — `LE_ULONG(key.offset)`, `LE_ULONG(key.filesize)`
- **Graphics structures** (`gfxapi.cpp`, `gfxapi_a.cpp`): `GFX_PIC.width/height`, `GFX_SPRITE.offset/length`
- **Enemy sprites** (`enemy.cpp`): the entire `SPRITE` structure (30+ int/short fields)
- **Map data** (`tile.cpp`): `MAZELEVEL`, `MAZEDATA` fields
- **Audio DSP** (`dspapi.cpp`): `dsp_t.format/freq/length`
- **Music** (`musapi.cpp`): `mushead_t.len/offset/channels`
- **OPL/GenMIDI** (`i_oplmusic.cpp`): instrument flags
- **Window system** (`swdapi.cpp`): dozens of fields
- **Demo** (`demo.cpp`): `RECORD.px/py/playerpic`
- **Fonts** (`gfxapi.cpp`): `FONT.charofs[]`
- **GSS patches** (`gssapi.cpp`): audio format fields

### 4.3 Game save/load system

`loadsave.cpp` contains explicit byte-by-byte serialization (e.g.
`SaveRead32()` assembles 32-bit values from four `SaveRead8()` calls). This
is fully portable and endian-safe. The Amiga branch of `RAP_InitLoadSave()`
stores saved games in the current directory.

### 4.4 The ITEM_ID union in glbapi.cpp

Big-endian handling for the `ITEM_ID` union is present:

```c
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
typedef struct { uint16_t filenum; uint16_t itemnum; } ITEM_ID;
#else
typedef struct { uint16_t itemnum; uint16_t filenum; } ITEM_ID;
#endif
```

### 4.5 Remaining endianness considerations

1. **Per-read swapping overhead.** Data in `SPRITE`, `CSPRITE`,
   `MAZELEVEL`, etc. structures is not converted in place; LE values are
   swapped on every read. On 68060 the swap is cheap, but a one-time
   conversion after loading remains a possible optimization. Not measured:
   **requires validation** (profiling) before changing.
2. **`#pragma pack(push, 1)`** is used in `mushead_t`, `RECORD`,
   `genmidi_op_t`. GCC on m68k supports `#pragma pack`; the game runs with
   these structures on real hardware (music and demos work per the
   changelog), but any new packed structure should be checked the same way.
3. **Full-coverage audit.** The original recommendation still stands as a
   validation item: verify every place that reads GLB data without the
   `LE_*` macros. The game's playable state on big-endian hardware is strong
   evidence, but a systematic audit has not been documented here.

### 4.6 Endianness summary

| Area | Status | Notes |
|------|--------|-------|
| GLB headers (KEYFILE) | OK | Full LE_ conversion |
| Graphics data (pixels) | OK | 8-bit, endianness not applicable |
| Color palettes | OK | 3 bytes per color (R,G,B) |
| Sprite structures | OK | Accessed via LE_LONG/LE_SHORT |
| Map data | OK | Accessed via LE_SHORT |
| Audio patches (DSP/GSS) | OK | Headers via LE_SHORT/LE_LONG, byte-based data |
| MUS music | OK | Header via LE_USHORT, byte-streamed data |
| Save/load game | OK | Explicit LE byte-by-byte serialization |
| Demo recordings | OK | Fields via LE_SHORT |
| Window system (SWD) | OK | All fields via LE_LONG |

---

## 5. Compiler and Build Flags (verified)

### 5.1 Makefile.amiga (primary build path)

```
CPU flags (per profile)   -m68060 -m68881 | -m68060 -msoft-float |
                          -m68030 -m68881 | -m68030 -msoft-float
Optimization              -O2
C++ standard              -std=c++11 (C: -std=c11)
-noixemul                 # libnix (non-ixemul) CRT; supplies access() and
                          # the AmigaOS syscall hooks
-fomit-frame-pointer
-fno-exceptions -fno-rtti # C++ only
-DAMIGA -D__AMIGA__ -DNDEBUG
-DUSE_SDL_STUBS           # default when SDL2_AMIGA_DIR is not set
Link: -noixemul, -lm, -lstdc++ (plus per-profile link flags, see 1.2)
```

Note on optimization: `Makefile.amiga` uses **-O2**. `CMakeLists.txt` sets
**-O3** globally (`CMAKE_CXX_FLAGS`/`CMAKE_C_FLAGS`) for all platforms,
including the Amiga branch. Both values are current; they are intentionally
different files. If you build with CMake on Amiga, be aware the binary is
compiled at -O3 — behavior and size differences between the two paths
**require validation** before shipping a CMake-built Amiga binary.

### 5.2 CMakeLists.txt (Amiga branch)

- Definitions: `AMIGA`, `__AMIGA__`, `USE_SDL_STUBS`, `NDEBUG`.
- Options: `-noixemul`, `-fomit-frame-pointer`, `-fno-exceptions`,
  `-fno-rtti`, plus the cache-variable CPU/link flags
  (`RAPTOR_AMIGA_CPU_FLAGS` / `RAPTOR_AMIGA_LINK_FLAGS`, default
  `-m68060 -m68881`).
- Links `m` and `stdc++`; never links SDL2.

### 5.3 Compiler notes (still applicable)

- `long long` in `rap.cpp` (`wrand()`) — supported by m68k-amigaos-gcc.
- C++11 features (`auto`, default arguments) — supported; the build sets
  `-std=c++11`.
- `#pragma once` and `#pragma pack(push, 1)` — supported; verify packing
  when adding new packed structures.
- The audio-task entry code is compiled with `-O0` in the stubs to prevent
  FPU register spills during `OpenDevice` (see the comment in
  `amiga_sdl_stubs.h`).

---

## 6. Remaining Work and Validation

The original "CRITICAL" list (SDL2 replacement, video, audio, timer) has
been implemented. What follows is the current open-items list, based on the
repository state.

### 6.1 Build configuration coverage

- The soft-float profiles (`raptor_nofpu`, `raptor_030`) pass automated
  instruction checks and were tested in WinUAE (68060 FPU disabled, per
  changelog 0.9.1); real 68030/EC060/LC060 hardware testing: **requires
  validation**.
- The CMake Amiga branch mirrors `Makefile.amiga` but is not the primary
  build path; a CMake-built binary (including its -O3 optimization level)
  **requires validation** before being treated as equivalent.
- The build scripts hardcode their working directory
  (`/mnt/c/amiga-raptor/raptor`); making them location-independent is a
  small quality-of-life improvement, not a blocker.

### 6.2 RTG/video validation

- CGX (CyberGraphX) fallback and the AGA/C2P path are implemented but not
  confirmed on real hardware in the changelog: **requires validation**.
- The 320x240x8 letterbox fallback covers drivers without a 320x200x8 mode;
  behavior across more RTG boards/drivers: **requires validation**.

### 6.3 Input validation

- Joystick/CD32 pad support is implemented via `lowlevel.library`; testing
  across real joysticks and CD32 pads (and phantom-input behavior on more
  machines): **requires validation**.
- The mouse/joystick disable options (`MOUSE=OFF`, `JOYSTICK=OFF`) are new
  troubleshooting aids (changelog "Unreleased"); their effect on the
  reported demo slowdown: **requires validation**.

### 6.4 Audio/music validation

- CAMD music needs a configured MIDI driver or software synth on the
  `out.0` cluster; MHI music needs a hardware MHI decoder. Both backends are
  implemented and documented but depend on external hardware: **requires
  validation** on those setups.
- MHI volume scaling has a known possible dead zone above 1/4 of the slider
  (changelog 0.9.6_MHI).

### 6.5 Big-endian asset and save/load validation

- The endianness mechanisms are in place and the game runs, but a systematic
  audit of all GLB readers without `LE_*` macros (section 4.5) and a
  round-trip test of saves/demos created on little-endian hosts and read on
  Amiga (and vice versa): **requires validation**.
- Path separators: `glbapi.cpp` still contains `strrchr(exePath, '\\')`
  (backslash only) when deriving the executable path. Confirm how `exePath`
  is produced on Amiga (argv from the CLI/Workbench) and that DOS-style
  separators never leak into AmigaOS paths: **requires validation**.

### 6.6 Emulator versus real hardware

- PiStorm/Emu68 revealed several timing/input issues already fixed
  (phantom joystick fire, phantom middle button). Continued emulator-vs-real
  comparison, especially for the AGA path and the 030 profiles: **requires
  validation**.

### 6.7 Performance across supported profiles

- The 68060+FPU profile is the tested baseline; the dbopl core was chosen
  because the Nuked OPL3 core stalled a real 68060. No systematic
  performance measurements exist in the repository for the soft-float or
  030 profiles: **requires validation** (do not assume playable frame rates
  on 030 without testing).

---

## 7. Historical Design Notes (July 2026 pre-port analysis)

The material below is retained for context. It describes the situation
**before** the port was implemented and does not reflect current decisions
except where noted.

### 7.1 SDL2 strategy considered at the time

The original analysis weighed three approaches for the SDL dependency:

- **Approach A:** use an SDL2 port for AmigaOS 3.x (e.g. from the bebbo
  toolchain) — minimal code changes, but an extra abstraction layer and an
  incomplete/stability risk.
- **Approach B (then "recommended"):** native AmigaOS APIs behind the
  existing interfaces (`GFX_InitVideo`, `I_InitGraphics`, `SND_InitSound`,
  ...).
- **Approach C (then "recommended starting strategy"):** hybrid — start
  with SDL, gradually replace with native code.

**What actually happened:** none of these exactly. The port uses a
*minimal in-tree SDL2 stub layer* (`src/amiga/`) that keeps the game's SDL
call sites unchanged while implementing them directly on AmigaOS
(Intuition/graphics.library, P96/CGX, AHI, lowlevel.library, DOS). This
proved simpler than either a full SDL2 port or a full native HAL, and the
clean API boundaries noted in the original analysis did make this possible.

### 7.2 Original phased plan (for the record)

1. Compilation and linking (SDL stubs) — done.
2. Display (title screen) — done.
3. Input + audio (playable game) — done.
4. Optimization/profiling — partially done (dbopl switch, C2P, blit fast
   paths, audio-task chunking); systematic profiling remains open (6.7).

### 7.3 Superseded recommendations

- "Use OPL3/Adlib mode first, postpone TinySoundFont" — implemented, and
  taken further: the Nuked OPL3 core was replaced by dbopl on Amiga, and
  TinySoundFont is compiled but unused there.
- "Postpone the textscreen setup program; edit SETUP.INI manually" —
  superseded: SETUP.INI support was removed entirely; configuration is via
  parameters/ToolTypes and `amiga.cfg`.
- "`SDL_GetPrefPath()` -> replace with a fixed path" — implemented as "use
  the game directory" (section 1.7).
- "Consider lowering the mixer to 22050 Hz" — superseded: the pipeline runs
  at 11025 Hz (the samples' native rate).
- "`SDL_ShowSimpleMessageBox()` -> `EasyRequestArgs()`" — implemented for
  the RTG-required requester.
- "68060-specific assembly for hot loops" — not done; the C2P converter is
  C (delta-swap based) and fast enough so far.

---

*Document origin: July 2026 pre-porting analysis (upstream skynettx/raptor).*
*Revised against the current repository state (port 0.9.6_MHI era); see the
status notice at the top for the authoritative build files.*