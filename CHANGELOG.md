# Changelog

All notable changes to this Amiga 68k port of Raptor are documented here.

## [0.9.2] - 2026-08-08

Version name: **0.9.2 MUSIC**

### Added
- Music backend selection parameter `MUSIC=ADLIB|CAMD|OFF`
  (CLI `-music=`, dashless `MUSIC=`, and Workbench icon ToolType
  `MUSIC=`): ADLIB = built-in OPL3 emulation (default), CAMD =
  General MIDI via camd.library, OFF = no music (same as -nomusic;
  -nomusic / MUSIC=OFF always wins). When MUSIC=CAMD finds no MIDI
  driver/synth attached to the "out.0" cluster, a clear console
  warning explains that the music would be silent (the CaffeineOS
  case: camd.library installed but unconfigured).

### Changed
- Default music backend changed from CAMD (General MIDI) to the
  built-in AdLib/OPL3 emulation: camd.library can be present but
  unconfigured (e.g. CaffeineOS), and a CAMD link succeeds even with
  nothing attached, which played silence. AdLib/OPL3 always plays;
  CAMD music is now opt-in via MUSIC=CAMD.

### Removed
- SETUP.INI support removed entirely from the Amiga build
  (`src/prefapi.cpp/h` deleted; all INI_GetPreference* /
  INI_PutPreference* call sites replaced with the former built-in
  defaults): audio card selection, volumes (music 127 / sfx 127),
  4 DSP channels, video and input preferences (classic Raptor
  mapping), and the `[Setup] camd_cluster` option (the CAMD output
  cluster is now fixed to "out.0"). Consequence: volume/detail
  changes made in the in-game options menu apply to the current
  session only and are no longer saved between runs.

### Fixed
- Startup banner corrected: it still read "beta version -no sfx and
  music" (a leftover from the 0.8.x era, before audio existed) and
  used the wrong game title; it now shows "Raptor: Call of the
  Shadows" and the release version.
- Remaining Polish source comments translated to English (fx.cpp).
- README_AMIGA: removed an outdated claim that the game falls back
  to a custom chipset screen by itself - AUTO/RTG are strict (an
  English requester appears, no silent fallback); only GFX=AGA opens
  the classic chipset screen.

## [0.9.1] - 2026-08-08

Version name: **0.9.1 EC060**

### Added
- Support for 68060 systems without an FPU (68EC060/68LC060 or a broken
  FPU): `make -f Makefile.amiga NOFPU=1` (or `build_amiga_nofpu.sh`)
  produces `raptor_nofpu`. Objects are compiled with `-m68060
  -msoft-float` but linked with `-m68000 -msoft-float`, which sidesteps a
  bebbo gcc 6.5.0b quirk: the toolchain has no soft-float multilib, so a
  plain `-m68060 -msoft-float` link still pulls `__adddf3` & co. from the
  libm020/libm881 libnix `libm.a` as thunks returning through fp0 (FPU
  instructions). Linking as m68000 selects the base libnix `libm.a`,
  whose thunks call mathieeedoubbas.library (Kickstart ROM, software IEEE
  double) with d0/d1 returns - the resulting binary contains zero FPU
  instructions, verified automatically by the build script. Tested in
  WinUAE with the 68060 FPU disabled. Runtime cost is negligible: the
  game uses floating point only for the one-time OPL table init and
  minor volume math.

### Fixed
- OPL music no longer starves the game: on Amiga, MUS_Mix renders in
  chunks aligned to the MUS_Service (70 Hz) / MUS_Fader (50 Hz) /
  GSS_Service (140 Hz) boundaries instead of one Mix() call per sample.
  Per-sample calls at 11025 Hz made the dbopl backend walk all 18 OPL
  channels per sample, so the priority +10 audio task never slept in
  WaitIO() and the game crawled at ~1 FPS. Event timing stays bit-exact.
- Intermittent recoverable alert AN_BogusExcpt (0100 0009) at game
  exit: the normal quit path called ShutDown(0) directly and then
  EXIT_Clean(), which ran the whole shutdown sequence a SECOND time
  (double free of g_highmem and the GLB arena, GLB_GetItem() after
  GLB_FreeAll()). ShutDown() is now idempotent, the redundant direct
  call is removed, and SND_DeInit() runs before GLB_FreeAll() so the
  background audio task is fully stopped before the song/sample
  buffers it reads are freed. SDL_CloseAudio() additionally waits for
  the audio process to really die before freeing the AHI buffers, and
  no longer frees them at all if the task failed to stop in time.

### Changed
- RAPTOR.LOG is no longer created automatically; all startup/audio
  diagnostics go to the console (stdout) only. To capture them to a
  file, run "raptor > RAPTOR.LOG" from a Shell. (Workbench launches
  still redirect stdout to NIL:.)

## [0.9.0] - 2026-08-07

Version name: **0.9.0 SOUND**

Tested on real hardware (Amiga 2000, 68060, AmigaOS 3.2.1, Picasso96
RTG): full game with sound effects and music, correct intro pacing,
clean startup and exit (no leftover tasks or devices).

### Added
- Sound effects through AHI (ahi.device): 11025 Hz 16-bit stereo - the
  native rate of the game's samples, so the DSP mixer ticks 1:1 with
  the output (no resampling work) - streamed by a dedicated audio task
  using the canonical double-buffered CMD_WRITE scheme from the AHI
  documentation.
- Music through CAMD (camd.library): new `mpucamd.cpp` backend plays
  the MUS tracks as a live General MIDI event stream to a CAMD cluster
  (default "out.0", configurable via SETUP.INI
  `[Setup] camd_cluster=<name>`), like the upstream WinMM/ALSA/CoreMIDI
  backends on other platforms.
- Automatic music fallback: without camd.library the game uses AdLib
  (OPL) emulation mixed into the AHI stream.  The emulator core is the
  lightweight DOSBox **dbopl** (vendored `src/dbopl.cpp/h`, GPL,
  DOSBox Team) behind an opl3.h-compatible shim
  (`src/opl3dbopl.cpp/h`) - it renders at the 11025 Hz output rate
  (a few percent of a 68060) and is skipped entirely when no OPL voice
  is playing.
- Vendored official headers under `src/amiga/`: AHI SDK
  `devices/ahi.h`; CAMD `midi/camd.h`, `midi/mididefs.h`,
  `clib/camd_protos.h`; hand-written NDK-style GCC `proto/camd.h` /
  `inline/camd.h` (LVO offsets verified against the official fd file).
- RAPTOR.LOG audio diagnostics: AHI/CAMD/MUS/DSP init steps, streaming
  counters and io_Error capture.

### Fixed
- Silent audio + hang right after the Apogee logo: the AHI message
  port was created by the main task, so ahi.device signalled the main
  task on completed requests while the audio task's WaitIO() slept
  forever (zero buffers played; the MUS sequencer never advanced, and
  the intro waits for the song).  The audio task now owns the whole
  device side: port, IO requests, OpenDevice/CloseDevice, streaming
  loop and teardown.
- AHI backend wrote every request field 8 bytes past the official
  AHIRequest layout (broken hand-written substitute header in the
  toolchain NDK): ahi.device read Type=0 (mono 8-bit), Frequency=0,
  Volume~0 and a wild Link pointer.  Compile-time layout guards
  (sizeof/offsetof) were added so the wrong header can never silently
  win again.
- Wrong AHI constants: AHIST_S16S 0x06 -> 0x03, ahir_Version 2 -> 4,
  stereo position 0 (full left!) -> 0x8000 (center).
- Crackling/stuttering audio and slow title-screen transitions: the
  audio task ran at the same priority as the game loop and was starved
  by equal-priority timeslicing.  It now runs at priority +10, with
  512-frame buffers (~46 ms of runway at 11025 Hz).
- The upstream Nuked OPL3 core stalled the whole game on a real 68060
  (it steps the chip at a fixed internal ~49716 Hz even for silence);
  replaced by the DOSBox dbopl core (see above).
- `dspapi.cpp` restored to the upstream version: fixes the effect
  length unit (reads past the sample buffer at randomized pitch) and
  the effect volume divisor (/127 instead of /256, about 6 dB louder).
- `MUS_DeInit()` is now called from `SND_DeInit()`: CAMD link/node and
  camd.library are properly released on exit, after all-notes-off is
  sent on every channel; ahi.device and the audio task are really shut
  down via SDL_QuitSubSystem(SDL_INIT_AUDIO) and SDL_Quit().

### Changed
- The audio pipeline runs at 11025 Hz (native SFX rate; 4x lighter
  AHI stream than 44100 Hz on the 68060).
- Defaults without SETUP.INI: music card = General MIDI (CAMD), DSP
  mixer channels = 4 (upstream default 2, max 8; oldest voice is
  stolen when full).
- Parameter behaviour finalized: no parameter = SFX (AHI) + music
  (CAMD, or AdLib fallback); NOMUSIC = SFX only, camd.library never
  opened; NOSOUND = no audio at all, neither ahi.device nor
  camd.library is opened (still the fastest startup path).

## [0.8.1-beta] - 2026-08-01

Version name: **BETA 0.8.1 NOSOUND**

### Added
- Workbench icon ToolTypes via the official WBStartup + icon.library
  `FindToolType()` mechanism (NOSOUND, NOMUSIC, NOJOY);
  parenthesized ToolTypes are ignored per AmigaOS convention.
- Startup banner with port version and contact info.
- On Workbench launches no console window is opened at all (stdout and
  stderr are redirected to NIL:), so nothing is left behind when the
  game exits.


### Changed
- All temporary diagnostic output removed for the beta: `[AMIGA]`,
  `[AMIGA][DIAG]`, `[AMIGA][AUDIO]`, `[VIDEO]`, `[INIT]`, `[GFX]` and
  `[EXIT_Clean]` log lines stripped from the build; startup output is now
  limited to the banner, parameter confirmations and original game
  messages.
- Duplicate `SETUP.INI` status message and duplicate parameter
  confirmations eliminated.

### Fixed
- Phantom middle mouse button events (MIDDLEDOWN/MIDDLEUP flood at
  ~VBlank rate on some RTG machines, e.g. A1200 + PiStorm/Emu68) are
  filtered out. The phantom button previously acted as a permanent
  "ack": intro logos skipped themselves, demos exited instantly and
  the mouse-takeover check fought joystick/keyboard steering in game.
  Special-weapon cycling remains available on SPACE.
- Exit crashes caused by attempts to close the Workbench console window
  (Software Failure #8700000E from a double `Close()`, then #81000005
  BadFreeAddr from `fclose()` of the standard streams): solved by never
  opening the console window on Workbench launches instead of trying to
  close it afterwards.
- Workbench icon ToolTypes now actually reach the game (previously they
  were looked for in argv[], which is empty on Workbench launches).


### Removed
- Experimental `RTGMODE=32` truecolor mode (did not work correctly on
  PiStorm/Emu68).
- The `RTGMODE`/`-rtgmode` parameter and the `8X2L` 640x240x8 pixel-
  doubling display mode, together with the RTG screen-mode requester.

## [Unreleased]

### Added
- **CGX (CyberGraphX) fallback:** when the P96 (Picasso96) RTG path does
  not produce a usable screen, the game now tries `cybergraphics.library`
  (CGX). The same strict 320x200x8 / 320x240x8 mode matching and native-
  chipset rejection logic is applied to the CGX mode list. GFX=AUTO/RTG
  will use the first working RTG driver (P96 preferred, CGX as fallback);
  the English "RTG required" requester appears only if both fail.


### Added
- `GFX=AUTO|RTG|AGA` parameter (CLI and Workbench icon ToolType):
  controls the graphics driver path — AUTO/RTG require RTG (strict,
  English requester on failure, no silent fallback); AGA forces a
  classic chipset screen.
- Automatic 320x240x8 RTG letterbox fallback: when the RTG path cannot
  open 320x200x8 it tries 320x240x8 (a common Picasso96 default),
  drawing the 320x200 game image at the top with a 40-row black bar at
  the bottom.
- Detailed `[VIDEO]` diagnostic log written to `RAPTOR.LOG` on every start,
  covering P96 detection, BestModeID selection, GFX mode choice, and
  the final physical screen parameters.
- Initial Amiga 68k port setup: dedicated `Makefile.amiga`, SDL stub layer, and porting analysis document.  
- Detailed video init logging and critical SDL stub fixes for Amiga 68k, including real timer implementation (`SDL_GetTicks`), `SDL_Delay` using AmigaOS, and functional `SDL_LowerBlit` for visible output.  
- Real Amiga Intuition window support in SDL stubs: SDL window/renderer/texture structs now hold actual Intuition window pointers, and `SDL_CreateWindow`/`SDL_DestroyWindow` manage Amiga windows and libraries correctly.  
- RTG (Picasso96) custom screen support in SDL stubs and matching rebuild of Amiga binaries.  
- English translations for `AMIGA_PORTING_ANALYSIS.md` and `README.md`.  
- `.clineignore` configuration and Cline project rules tailored to the Amiga port.  
- Command‑line flags `-nosound` and `-nomusic` to control the Amiga audio subsystem and diagnose hangs on real 68k hardware.  
- Temporary FX/DSP/SFX audio debug probes and early AHI probes to understand callback behaviour.  

### Changed
- RTG display mode is now chosen directly from the live Picasso96 mode
  list (`p96AllocModeListTagList`) instead of `BestModeID`: the game
  selects the exact 320x200x8 mode (fallback: letterboxed 320x240x8) and
  never uses a native chipset ModeID as a substitute. GFX=AUTO tries RTG
  first, GFX=RTG strictly requires RTG, and GFX=AGA keeps forcing the
  native 320x200x8 screen; a missing P96 mode never triggers a silent
  AUTO→AGA fallback.
- Documented actual memory requirements from code analysis: the game uses ~3 MB of Fast RAM (2 MB game heap, binary, blit/surface buffers); requirements updated to **4 MB Fast RAM minimum (8 MB recommended) + 2 MB Chip RAM** in README files.
- README updated and then rewritten to describe the Amiga 68060 RTG build environment and port details in English.  

- Amiga SDL stubs repeatedly refined:  
  - Fixed Raptor SDL rendering stubs and input handling for RTG build.  
  - Implemented a real IDCMP event pump for keyboard/mouse, added full SDL scancode table, proper raw‑key mapping, and real `SDL_PumpEvents`/`SDL_PollEvent` integration.  
  - Fixed mouse cursor alignment, hid the Amiga system pointer during gameplay, and unified cursor handling while preserving the working `-nosound` path.  
  - Standardized and shortened English comments throughout `src/amiga/amiga_sdl_stubs.h` for clarity and consistency.  
- Video and controls stabilized on Amiga (pre‑AHI), with a known good baseline version of the port.  
- Project structure documentation updated with a current snapshot, replacing an obsolete structure overview.  
- Amiga porting analysis moved into `docs/` to avoid clutter in the repository root, and an obsolete project file was removed.  
- Repository root cleaned up to keep a single Amiga build script (`build_amiga.sh`) that encapsulates your preferred build workflow (cleaning objects, deleting `raptor`, and rebuilding).  
- SDL2 CMake config (`sdl2-config.cmake`) updated to use a modern `cmake_minimum_required` version syntax compatible with current CMake policy requirements.  
- `.gitignore` patterns cleaned up to consistently ignore CMake build artefacts (`CMakeFiles/`, `CMakeCache.txt`, generated `Makefile`, `build/`), editor config (`.vscode/`), macOS `.DS_Store` files, the `raptor` binary, and the local Amiga build directory.  

### Removed
- Obsolete project structure text file from the repository root after replacing it with the current structure snapshot.  
- Android Gradle project (`android/`) and MSVC Visual Studio solution/projects (`msvc/`) from the Amiga‑focused fork to reduce noise and make the repo clearly Amiga‑centric.  
- Temporary helper/log files and stray artefacts from the root directory (Docker logs, helper binaries, misnamed curl output files, and unused build scripts).  
- Local `build.amiga/` directory from the repository and added it to `.gitignore` so future Amiga builds remain local artefacts and never pollute version control.  

### Fixed
- Critical timing and rendering bugs on Amiga 68k:  
  - Broken fake timer and missing delays that caused unpredictable fade and update loops.  
  - No‑op SDL blit stubs which prevented `I_FinishUpdate` from producing visible graphics.  
- Input handling issues:  
  - SDL stubs previously ignored IDCMP messages, so keyboard/mouse/joystick input never reached the game loop; this has been replaced with a full IDCMP event pump and SDL event queue.  
  - Mouse position reporting and relative motion now use live IDCMP coordinates and respect SDL’s logical size and viewport semantics, fixing vertical cursor drift in aspect‑correct modes.  
- Cursor handling and system pointer behaviour:  
  - Native Amiga hardware pointer is hidden for the entire in‑game session and restored only on full application exit, avoiding double‑cursor and missing‑cursor states.  
- Build reliability and error reporting:  
  - Amiga heap size reduced and allocation failures guarded with clear error messages.  
  - Additional `EXIT_Error` logging and NULL checks around video initialization and screenbuffer allocation.  
  - Amiga build artefacts and the `raptor` binary now consistently ignored so `git status` reflects only real source changes.  
