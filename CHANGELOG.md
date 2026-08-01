# Changelog

All notable changes to this Amiga 68k port of Raptor are documented here.

## [0.8.1-beta] - 2026-08-01

Version name: **BETA 0.8.1 NOSOUND**

### Added
- `RTGMODE` parameter (CLI and Workbench icon ToolType): `RTGMODE=8X2L`
  opens a 640x240x8 screen and doubles the game image horizontally in
  software (320 -> 640 per row) - a workaround for PiStorm/Emu68 drivers
  that display the native 320x200x8 mode squeezed to half the screen
  width. Mouse coordinates are normalized to the same logical 320x200
  space as native mode. The game falls back to native 320x200x8 when the
  requested mode is unavailable.
- Workbench icon ToolTypes via the official WBStartup + icon.library
  `FindToolType()` mechanism (NOSOUND, NOMUSIC, NOJOY, RTGMODE);
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
  ~VBlank rate on some machines, e.g. A1200 + PiStorm/Emu68 in the
  scan-doubled 640x240 mode) are filtered out in the 8X2L mode. The
  phantom button previously acted as a permanent "ack": intro logos
  skipped themselves, demos exited instantly and the mouse-takeover check
  fought joystick/keyboard steering in game. Special-weapon cycling
  remains available on SPACE.
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

## [Unreleased]


### Added
- Initial Amiga 68k port setup: dedicated `Makefile.amiga`, SDL stub layer, and porting analysis document.  
- Detailed video init logging and critical SDL stub fixes for Amiga 68k, including real timer implementation (`SDL_GetTicks`), `SDL_Delay` using AmigaOS, and functional `SDL_LowerBlit` for visible output.  
- Real Amiga Intuition window support in SDL stubs: SDL window/renderer/texture structs now hold actual Intuition window pointers, and `SDL_CreateWindow`/`SDL_DestroyWindow` manage Amiga windows and libraries correctly.  
- RTG (Picasso96) custom screen support in SDL stubs and matching rebuild of Amiga binaries.  
- English translations for `AMIGA_PORTING_ANALYSIS.md` and `README.md`.  
- `.clineignore` configuration and Cline project rules tailored to the Amiga port.  
- Command‑line flags `-nosound` and `-nomusic` to control the Amiga audio subsystem and diagnose hangs on real 68k hardware.  
- Temporary FX/DSP/SFX audio debug probes and early AHI probes to understand callback behaviour.  

### Changed
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
