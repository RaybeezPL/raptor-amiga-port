# Changelog

All notable changes to this Amiga 68k port of Raptor are documented here.

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
