# Changelog

All notable changes to this Amiga 68k port of Raptor are documented here.

## [0.9.6] - 2026-08-12

Version name: **0.9.6 (AHI/MHI/CAMD, AGA/RTG)**

### Changed
- README.md: expanded PiStorm compatibility list to explicitly name A500, A600, A1200, A2000, A3000, A4000 as supported with PiStorm/Emu68 + RTG
- Version bumped to 0.9.6 across all files (README.md, README_AMIGA.md, README_AMIGA_PL.md, src/rap.cpp)
- Version string format unified to "0.9.6 (AHI/MHI/CAMD, AGA/RTG)"

## [0.9.5] - 2026-08-11

Version name: **0.9.5 AHI**

This port also works on any Amiga with PiStorm RTG (A500, A600,
A1200, A2000, etc.) or native RTG/AGA (A1200, A4000, and possibly
AA3000 — to be confirmed).

### Added
- MP3 music playback through MHI (the Amiga MPEG-audio driver standard
  by Thomas Wenzel / Paul Qureshi), selected with `MUSIC=MHI` (CLI
  `-music=`, dashless `MUSIC=`, and Workbench icon ToolType `MUSIC=`).
  The new `src/mpumhi.cpp` backend plays MP3 files from the `MP3/`
  drawer in the game directory through an installed MHI decoder driver
  (e.g. `LIBS:MHI/prismamhi.library` for the Prisma Megamix), leaving
  the AHI sound-effects stream untouched. Driver selection order:
  `MHIDRIVER=` override (new optional parameter, CLI and ToolType),
  prismamhi.library, mhimaspro/mhimasstd.library, mhimpegit.library,
  mhimdev.library, then any other driver found in `LIBS:MHI/`. When no
  driver can be opened the game falls back to AdLib/OPL3 music (same as
  the CAMD fallback). Each GLB music item is mapped to a song title
  fragment (`mhi_song_map` in `src/mpumhi.cpp`); the file lookup is a
  case-insensitive substring match (`*.mp3`, first match in the drawer
  wins) - no track numbers are used. The simplified naming scheme
  (`Main Menu.mp3`, `Wave Music 1.mp3`, ..., `Apogee Fanfare.mp3`) is
  documented in README_AMIGA.md. A song without a matching file stays
  silent by design. Streaming runs in a dedicated "Raptor MHI Task"
  (8 x 32 KB buffers, signal-driven refill, ID3v2/ID3v1 tag handling,
  loop support, underrun restart) following the canonical pattern from
  the MHI dev kit's `MHIplay.c`; driver volume control (`MHIP_VOLUME`)
  is forwarded from the in-game music volume when the driver supports
  it.
- Vendored MHI interface headers (`src/amiga/libraries/mhi.h`,
  `src/amiga/clib/mhi_protos.h` from the official MHI developer kit
  v1.2, Aminet `driver/audio/mhi_dev.lha`) plus hand-written
  `proto/mhi.h` / `inline/mhi.h` for GCC m68k (LVO offsets and register
  assignments verified against the official `mhi_lib.fd`; see
  `src/amiga/MHI-HEADERS.txt`).
- Persistent audio volume configuration via `amiga.cfg` in the game
  directory (`src/amiga/amiga_cfg.cpp/.h`): separate startup volumes
  for AdLib/OPL3 music (`music_adlib`), MHI/MP3 music (`music_mhi`)
  and sound effects (`sfx_volume`). The file is created with built-in
  defaults on the first run (127/127/127), read at startup
  (`SND_InitSound`), and rewritten whenever the in-game Options
  sliders are changed (`windows.cpp` OPTS_EXIT).

### Changed
- Removed the 5% MHI volume cap — the issue was specific to the
  author's Prisma Megamix setup. `MHI_SetVolume()` now passes the
  music volume 0..127 directly to the driver (was `(volume * 5) / 127`).
  Default `music_mhi` raised from 100 to 127 (100/100/100).
- Removed the `[VIDEO]` diagnostic dump of the whole P96/CGX mode list
  (one log line per display mode; ~50 lines of noise on startup).
  `Amiga_DumpP96Modes()` / `Amiga_DumpCGXModes()` and their call sites
  were deleted; the mode-selection logs (`selected ... mode`) remain.
- README files updated: the MP3/ drawer and the exact simplified file
  names are now documented (see README_AMIGA.md, "Sound (AHI + CAMD +
  MHI)").

### Fixed
- Non-looping MHI songs never reported their end on drivers that do not
  signal end-of-stream (e.g. Prisma Megamix): `MHI_Service()` only ran
  on driver signals, so `g_mhi.state` stayed `MHISTATE_PLAYING` forever
  and the Apogee/intro title screen waited for input (fire/LMB/Enter).
  `MHI_SongPlaying()` now also reports the song as ended when the
  buffer queue is empty or the driver has been silent for more than
  4 seconds (`g_mhi.traffic_ticks` watchdog). Intro screens now advance
  by themselves once the MP3 has finished.
- Music volume changes in the Options menu (and the resulting music
  volume) are now persisted to `amiga.cfg`, honoring the backend
  actually in use (MHI vs every other backend).

## [0.9.4] - 2026-08-08

Version name: **0.9.4 BLIT**

### Added
- Native RTG blit fast paths, selected once at screen open and logged as
  `[VIDEO] blit path: ...`: on Picasso96 the 320x200 chunky frame is pushed
  with `p96WritePixelArray` (RGBFB_CLUT), on CyberGraphX with
  `WritePixelArray` (RECTFMT_LUT8) - the RTG driver's own copy into the
  screen bitmap, replacing the generic graphics.library `WriteChunkyPixels`
  emulation layer. The CGX `WritePixelArray` LVO (0x7E), its register
  assignment and RECTFMT_LUT8 (=3, not 0 as in the minimal NDK stub header)
  were verified against the official CGraphX-DevKit VI
  (`cybergraphics_lib.fd`, `inline/cybergraphics.h`) and the compiled devkit
  example binary; the existing CGX mode-list offsets (0x48/0x3C/0x4E) were
  re-confirmed against the same FD.
- Custom 68060 chunky->planar (C2P) converter for the `GFX=AGA` path: the
  frame is converted via 8x8 bit-matrix transposes (delta swaps) and written
  as longword plane data straight into the screen's bitplanes in chip RAM -
  about 4x fewer chip-memory bus cycles than the byte-oriented OS
  conversion. Verified bit-exact against a brute-force reference in a
  host-side test. `WriteChunkyPixels` remains the fallback on every path.
  The C2P code is original work written for this port (GPL-2.0) using only
  the well-known published transpose algorithm - no third-party/demo-scene
  C2P code is used.

### Changed
- AHI stream buffers are 1024 frames (~93 ms of runway at 11025 Hz) when
  ADLIB/OPL3 music is active - double the underrun headroom for the audio
  task, which also renders the dbopl emulation. CAMD MIDI and -nomusic keep
  512 frames (~46 ms), so SFX latency stays low there.

### Fixed
- PiStorm (A1200 + Emu68) regression: intro title screens skipped
  themselves and demos exited instantly. Root cause: the accelerated RTG
  blit made the event pump run much faster, so noise on the floating
  joystick port lines passed the two-read debounce as phantom fire presses
  (injected as RETURN = "ack"). The gameport is now polled at most at
  50 Hz (the rate the old pacing produced), and a phantom-masked line only
  unmasks after ~0.5 s of continuously CLEAR reads. One-time `[INPUT]`
  log lines record the seeded port state and the first injected fire
  press for field diagnosis.
- Same symptom with `GFX=AGA` (seen on PiStorm with the soft-float
  build): the phantom middle-mouse-button filter was only active on RTG
  screens, so on a native chipset screen the Emu68 MIDDLEDOWN flood
  passed as a permanent "ack". The middle button is ignored by design
  in this port (special-weapon cycling is on SPACE), so the filter now
  drops MIDDLEDOWN/MIDDLEUP unconditionally on every display path, with
  a one-time `[INPUT]` log line when it first engages.

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
