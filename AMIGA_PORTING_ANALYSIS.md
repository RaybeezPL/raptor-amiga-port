# Raptor - AmigaOS 3.x Porting Analysis (68060 + RTG)

## Target Hardware
- **Processor:** Motorola 68060
- **Memory:** 32 MB Fast RAM
- **Graphics:** RTG card (Picasso96/CyberGraphX) with 4 MB VRAM
- **Compiler:** m68k-amigaos-gcc (cross-compiler)

---

## 1. Repository Structure - Overview

Raptor is a classic top-down shoot'em'up, originally written for DOS,
and ported to modern systems using SDL2. The code is C/C++ (C++11).

### Main source code modules (`src/`):

| Module | Files | Description |
|-------|-------|------|
| **Graphics engine** | `gfxapi.cpp/h`, `gfxapi_a.cpp` | 320x200x8bit software renderer (chunky pixels) |
| **Video layer** | `i_video.cpp/h` | SDL2 Window/Renderer/Texture initialization, display |
| **GLB file system** | `glbapi.cpp/h` | Custom asset format (.GLB), encryption, cache |
| **Virtual memory** | `vmemapi.cpp/h` | Custom memory manager with LRU eviction |
| **Sound** | `fx.cpp/h`, `dspapi.cpp/h` | Software mixer, 8-channel DSP, SDL Audio callbacks |
| **Music** | `musapi.cpp/h`, `i_oplmusic.cpp`, `opl3.cpp` | MUS format, OPL3/Adlib emulation |
| **MIDI (TinySoundFont)** | `mputsf.cpp` | SF2 synthesizer (header-only lib) |
| **MIDI (platforms)** | `mpuwinmm.cpp`, `mpualsa.cpp`, `mpucorea/m.cpp` | WinMM, ALSA, CoreAudio - **not relevant to Amiga** |
| **Input** | `kbdapi.cpp`, `joyapi.cpp`, `ptrapi.cpp`, `input.cpp` | Keyboard, joystick, mouse - SDL2 Events |
| **Window/GUI system** | `swdapi.cpp`, `windows.cpp` | In-game dialogs and UI with data from GLB |
| **Game logic** | `rap.cpp`, `enemy.cpp`, `shots.cpp`, `tile.cpp`, `bonus.cpp`, etc. | Pure logic, minimal platform dependencies |
| **Save/load** | `loadsave.cpp/h` | Serialization with explicit byte-order |
| **Demo** | `demo.cpp/h` | Gameplay recording/playback |
| **Settings** | `prefapi.cpp/h` | Reading/writing INI files (pure C, no SDL) |
| **Intro animations** | `intro.cpp`, `movie.cpp/h`, `movie_a.cpp` | Animated cutscene sequences |
| **Textscreen (setup)** | `include/textscreen/` | Separate setup UI library - SDL2-based |

---

## 2. Endianness (Little Endian -> Big Endian)

### 2.1 Current State - GOOD NEWS

The code **already has extensive endianness handling**. In the file `src/entypes.h`:

```c
#include "SDL_endian.h"
#define LE_USHORT(x) SDL_SwapLE16(x)
#define LE_SHORT(x)  (signed short) SDL_SwapLE16(x)
#define LE_ULONG(x)  SDL_SwapLE32(x)
#define LE_LONG(x)   (signed int) SDL_SwapLE32(x)
```

On Big Endian systems (Amiga 68k), `SDL_SwapLE16/32` automatically perform a byte-swap.
On Little Endian (PC) they are no-ops.

### 2.2 Where the LE_* Macros Are Used

The `LE_LONG`, `LE_SHORT`, etc. macros are used **everywhere** the code reads binary data
from `.GLB` files (game assets in DOS Little Endian format):

- **GLB file system** (`glbapi.cpp`): `KEYFILE` headers - `LE_ULONG(key.offset)`, `LE_ULONG(key.filesize)`
- **Graphics structures** (`gfxapi.cpp`, `gfxapi_a.cpp`): `GFX_PIC.width/height`, `GFX_SPRITE.offset/length` - accessed via `LE_LONG()`
- **Enemy sprites** (`enemy.cpp`): the entire `SPRITE` structure (30+ int/short fields) - every field read via `LE_LONG()/LE_SHORT()`
- **Map data** (`tile.cpp`): `MAZELEVEL`, `MAZEDATA` - `LE_SHORT(mapmem->map[loop].flats)`
- **Audio DSP** (`dspapi.cpp`): `dsp_t.format/freq/length` - `LE_SHORT(dsp->format)`
- **Music** (`musapi.cpp`): `mushead_t.len/offset/channels` - `LE_USHORT(head->len)`
- **OPL/GenMIDI** (`i_oplmusic.cpp`): instrument flags - `LE_USHORT(instrument->flags)`
- **Window system** (`swdapi.cpp`): dozens of fields - `LE_LONG(curfld->x)`, `LE_LONG(curfld->opt)`, etc.
- **Demo** (`demo.cpp`): `RECORD.px/py/playerpic` - `LE_SHORT()`
- **Fonts** (`gfxapi.cpp`): `FONT.charofs[]` - `LE_SHORT(font->charofs[ch])`
- **GSS patches** (`gssapi.cpp`): audio format fields - `LE_SHORT(gss->bank)`

### 2.3 Game Save/Load System

The file `loadsave.cpp` contains **explicit byte-by-byte serialization** - this is a model solution:

```c
static int SaveRead32(void) {
    int convert;
    convert = SaveRead8();
    convert |= SaveRead8() << 8;
    convert |= SaveRead8() << 16;
    convert |= SaveRead8() << 24;
    return convert;
}
```

This is **fully portable** and works correctly on Big Endian.

### 2.4 The ITEM_ID Union in glbapi.cpp

The code already has BE handling for the `ITEM_ID` union:

```c
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
typedef struct { uint16_t filenum; uint16_t itemnum; } ITEM_ID;
#else
typedef struct { uint16_t itemnum; uint16_t filenum; } ITEM_ID;
#endif
```

### 2.5 Potential Endianness Issues

1. **No obvious issues** - the code is well prepared. However, a
   **thorough verification** is needed of every place where GLB data is read without the LE_* macros.
   In particular:
   - Purely byte-based graphics data (palettes, 8-bit pixels) - no issue here
   - The `FLATS` table in `rap.h` - fields `linkflat` (int), `bonus` (short), `bounty` (short) -
     in `tile.cpp` accessed via `LE_SHORT(lib[...].bounty)` OK

2. **Internal memory**: Data from the `SPRITE`, `CSPRITE`, `MAZELEVEL`, etc. structures is **not
   converted in-place** - LE values are swapped on every read. On a 68060 the byte-swap operation
   is cheap (a `ROL`/`ROR` instruction), but it is still a minor performance overhead.
   Alternatively, a one-time conversion after loading could be added.

3. **#pragma pack(push, 1)** - used in `mushead_t`, `RECORD`, `genmidi_op_t`.
   GCC on m68k supports `#pragma pack`, but alignment correctness must be verified.

### 2.6 Endianness Summary

| Area | Status | Notes |
|--------|--------|-------|
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

## 3. Dependencies - External Libraries

### 3.1 SDL2 - **Main Dependency** (CRITICAL)

SDL2 is used in **almost every module**:

| SDL2 Usage | Files | Amiga Replacements |
|--------------|-------|-------------------|
| Video (Window, Renderer, Texture, Surface) | `i_video.cpp` | **Picasso96/CyberGraphX API** or **SDL2 Amiga port** |
| Audio (SDL_AudioDeviceID, callback mixing) | `fx.cpp`, `mputsf.cpp` | **AHI (Audio Hardware Interface)** or SDL2 Amiga |
| Input Events (keyboard, mouse, joystick) | `kbdapi.cpp`, `joyapi.cpp`, `ptrapi.cpp`, `input.cpp` | **input.device, gameport.device, Intuition** or SDL2 Amiga |
| Timer (SDL_GetTicks) | `gfxapi.cpp`, `musapi.cpp` | **timer.device** or `ReadEClock()` |
| Endianness (SDL_SwapLE*) | `entypes.h` | Simple re-implementation (see below) |
| Filesystem (SDL_GetPrefPath) | `loadsave.cpp` | `PROGDIR:` / `ENV:` |
| Init/Quit subsystems | many | Native Amiga equivalents |
| SDL_opengl.h | `i_video.cpp` | **Not actively used** - to be removed |

#### SDL2 Replacement Strategy - **Two Approaches**:

**Approach A: Use an SDL2 port for AmigaOS 3.x**
- An SDL2 port for m68k AmigaOS exists (e.g. in the bebbo GCC toolchain)
- **Pros**: Minimal code modification, quick start
- **Cons**: Extra abstraction layer, possible performance issues, the SDL2
  port may not be complete or stable

**Approach B: Native AmigaOS API (RECOMMENDED)**
- Requires more work, but gives the best performance and control
- HAL (Hardware Abstraction Layer) hidden behind existing interfaces
  (`GFX_InitVideo`, `I_InitGraphics`, `SND_InitSound`, etc.)
- The code architecture makes this easier - the game has clean API boundaries

**Approach C: Hybrid**
- Start with the SDL2 Amiga port -> gradually replace with native API
- **Recommended as the starting strategy**

### 3.2 TinySoundFont (`include/TinySoundFont/tsf.h`)

Header-only C library for MIDI synthesis from SoundFont (.sf2) files.

- **Status**: Should compile on m68k-amigaos-gcc without changes
- **Issue**: Requires float/double - on the 68060 with FPU this is OK, but may be slow
  compared to OPL3 emulation (integer-based)
- **Recommendation**: Initially use OPL3/Adlib mode (emulation in `opl3.cpp` - pure integer),
  postpone TinySoundFont for later

### 3.3 Textscreen (`include/textscreen/`)

Text UI library (from the Chocolate Doom project), used only by the **setup program** (`raptorsetup`).

- **Status**: Depends on SDL2, written in C
- **Recommendation**: Postpone for a later phase. Configuration can be edited manually in SETUP.INI

### 3.4 Platform-Specific MIDI Libraries

| File | Platform | Needed on Amiga? |
|------|-----------|---------------------|
| `mpuwinmm.cpp` | Windows (WinMM) | No |
| `mpualsa.cpp` | Linux (ALSA) | No |
| `mpucorea.cpp` | macOS (CoreAudio) | No |
| `mpucorem.cpp` | macOS (CoreMIDI) | No |
| `mputsf.cpp` | All (TinySoundFont) | Yes (optional) |

### 3.5 Standard C Library

The code makes extensive use of: `stdio.h`, `stdlib.h`, `string.h`, `stdint.h`, `math.h`, `ctype.h`,
`limits.h`, `errno.h`, `time.h`. All available in m68k-amigaos-gcc (newlib or clib2).

### 3.6 Specific POSIX/Win32 Calls

| Call | File | Amiga Replacements |
|-----------|------|-------------------|
| `access()` | `glbapi.cpp`, `loadsave.cpp`, `prefapi.cpp` | Available in clib2/newlib |
| `unistd.h` | `glbapi.cpp`, `rap.cpp`, `loadsave.cpp` | Partially in clib2 |
| `ftruncate()/fileno()` | `prefapi.cpp` | Available in clib2 |
| `strupr()` | `glbapi.cpp` | Already implemented inline for __GNUC__ OK |
| `ltoa()` | `prefapi.cpp` | Already implemented inline for __GNUC__ OK |
| `PATH_MAX` | many | Define if missing (e.g. 256) |
| `SDL_GetPrefPath()` | `loadsave.cpp` | Replace with a fixed path (`PROGDIR:`) |

---

## 4. Main Porting Challenges - Priority List

### CRITICAL (blocking compilation and startup)

1. **Replacing/providing SDL2**
   - Either compile SDL2 for AmigaOS 3.x (ports exist)
   - Or create an abstraction layer with native AmigaOS APIs
   - Initially: **create SDL stub/wrapper files** with a native Amiga implementation

2. **Video layer (`i_video.cpp`)**
   - Replace SDL_Window/Renderer/Texture with an RTG screen (Picasso96)
   - The game renders to a 320x200x8bit buffer -> `WriteChunkyPixels()` or `WriteLUTPixelArray()`
   - 256-color palette -> `LoadRGB32()` or `SetRGB32()`
   - VSync / retrace -> `WaitTOF()` or a VERTB interrupt

3. **Audio layer (`fx.cpp`)**
   - Replace `SDL_OpenAudioDevice()` + callback with AHI
   - The software mixer (DSP_Mix, MUS_Mix) remains unchanged
   - AHI double-buffering or callback mode needed

4. **Timer**
   - `SDL_GetTicks()` -> `ReadEClock()` (50 Hz precision) or `timer.device` (microsecond)
   - `SDL_Init(SDL_INIT_TIMER)` -> `OpenDevice("timer.device", ...)`

### IMPORTANT (needed to play)

5. **Input layer**
   - Keyboard: `SDL_KEYDOWN/UP` -> `IDCMP_RAWKEY` (IntuiMessage) or `input.device`
   - Mouse: `SDL_MOUSEMOTION/BUTTON` -> `IDCMP_MOUSEMOVE` / `IDCMP_MOUSEBUTTONS`
   - Joystick: `SDL_GameController` -> `gameport.device` or `lowlevel.library`
   - SDL scancode -> DOS scancode mapping already exists in `kbdapi.cpp` - Amiga -> DOS mapping still needed

6. **File paths and filesystem**
   - `\` -> `/` (or `:`) in path separators
   - `strrchr(exePath, '\\')` in `glbapi.cpp` -> also handle `/` and `:`
   - `SDL_GetPrefPath()` -> `PROGDIR:` or `S:Raptor/`
   - Case sensitivity: `.GLB` vs `.glb` files - AmigaOS is case-insensitive, OK

7. **Performance on the 68060**
   - 320x200x8bit software rendering should be OK
   - Software mixer at 44100 Hz stereo -> consider lowering to 22050 Hz
   - OPL3 emulation (`opl3.cpp`) - needs profiling
   - TinySoundFont - heavy use of float, on the 68060 FPU that's ~10-20 MFLOPS
   - 68060 @50MHz is approx. ~100 MIPS - should be sufficient with optimization

### LESS IMPORTANT (can wait)

8. **Setup program (`raptorsetup`)**
   - Requires the textscreen library (SDL2-based)
   - Postpone - configuration via manual SETUP.INI editing

9. **SDL_ShowSimpleMessageBox()**
   - Replace with `EasyRequestArgs()` or `printf()`

10. **Other platform-specific details**
    - `#ifdef __ANDROID__` / `#ifdef _WIN32` -> add `#ifdef __AMIGA__`
    - `SDL_free()` -> `free()`
    - `SDL_RWops` (Android file copy) - not applicable

---

## 5. Port Architecture - Recommended Structure

```
src/
├── [existing files - unchanged or minimally changed]
├── amiga/
│   ├── amiga_video.cpp     # Implementation of I_InitGraphics, I_FinishUpdate (RTG)
│   ├── amiga_audio.cpp     # Implementation of SND_InitSound via AHI
│   ├── amiga_input.cpp     # Keyboard, mouse, joystick via Intuition/input.device
│   ├── amiga_timer.cpp     # SDL_GetTicks() replacement via timer.device
│   ├── amiga_system.cpp    # AmigaOS initialization, paths, cleanup
│   └── amiga_sdl_stubs.h   # Minimal definitions for SDL_SwapLE*, SDL_BYTEORDER, etc.
```

### Phase 1: Compilation and Linking (goal: run the program)
- Create SDL stubs or use the SDL2 Amiga port
- Compile all .cpp files
- Link against the required AmigaOS libraries

### Phase 2: Display (goal: title screen)
- Implement the video layer (RTG)
- Test palette and sprite display

### Phase 3: Input + Audio (goal: playable game)
- Keyboard and mouse
- Audio mixer via AHI
- OPL3 music

### Phase 4: Optimization
- Performance profiling
- Optimizing critical loops (mixer, renderer)
- Optional: 68060-specific assembly for hot loops

---

## 6. Notes on the m68k-amigaos-gcc Compiler

### Required flags:
```
-m68060           # Generate code for the 68060
-m68881           # Use FPU (68060 has an integrated FPU)
-O2               # Optimization (not O3 - may generate excessively large code)
-fomit-frame-pointer  # Free the a6 register
-noixemul         # Do not link against ixemul (UNIX emulation), use native libc
```

### Potential compilation issues:
- `long long` in `rap.cpp` (`wrand()`) - m68k-amigaos-gcc supports this
- `bool` / `true` / `false` - requires C++11 or `<stdbool.h>`
- `auto` keyword (C++11) - `auto chan = &dsp_channels[i];` in `dspapi.cpp`
- `#pragma once` - supported by GCC
- `#pragma pack(push, 1)` - supported, but verify correctness on m68k
- Default argument values in C++ (`void I_SetPalette(uint8_t *doompalette, int start = 0)`) - OK in C++

---

*Document prepared: July 2026*
*Source code version: upstream skynettx/raptor*
