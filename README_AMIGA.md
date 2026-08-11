Raptor: Call of the Shadows - Amiga Port (68060 & EC/LC, RTG/AGA)

Version: 0.9.5 AHI

=====================================================

Amiga port of Raptor: Call of the Shadows, based on the open source
engine reconstruction (reverse engineering) by skynettx/raptor.

This port is developed exclusively for classic Amiga systems:
AmigaOS 3.2, a 68060-class CPU (full 68060 with FPU, or
68EC060/68LC060 via the soft-float raptor_nofpu binary), and RTG
graphics (Picasso96/CyberGraphX) or a native AGA chipset screen
(GFX=AGA). The game renders in 320x200 resolution with an 8-bit
palette on a dedicated screen.

NOTE: This repository does NOT contain game data files. You need
your own legal copy of the original game files. Only the full
version 1.2 is supported. The game may be purchased on Steam:
https://store.steampowered.com/app/358360/Raptor_Call_of_the_Shadows_1994_Classic_Edition/


Requirements
------------

Processor:  Motorola 68060 (or PiStorm / Emu68)
FPU:        Built-in 68060 / 68881 / 68882 - REQUIRED for the raptor
            binary (compiled with -m68881).
            Systems WITHOUT an FPU (68LC060/68EC060 or a broken FPU) can
            use the raptor_nofpu binary instead - a soft-float build in
            which floating point runs in software through
            mathieeedoubbas.library (present in Kickstart ROM). Build it
            with build_amiga_nofpu.sh or "make -f Makefile.amiga NOFPU=1".
System:     AmigaOS 3.2 (intuition.library v39+, graphics.library v39+)
Memory:     4 MB Fast RAM minimum (8 MB recommended) + 2 MB Chip RAM
            (standard). The game itself uses ~3 MB of Fast RAM; on RTG
            the screen bitmap lives in graphics card memory, so Chip
            RAM is only needed by the OS/Workbench.

Graphics:   RTG with Picasso96 or CyberGraphX (CGX)
            (e.g. CyberVision 64/3D, Picasso IV, UAEGFX/PiStorm)
            - 320x200x8 mode required
Disk space: approx. 25 MB free (game files + saved games)
Joystick:   Optional - port 1 (DB9); requires lowlevel.library v40+
            (included in AmigaOS 3.2); CD32 pads are also supported
Sound:      AHI (ahi.device v4+) for sound effects and for the
            built-in AdLib/OPL3 music emulation (default music);
            optionally music via MIDI out through camd.library
            (CAMD) when started with MUSIC=CAMD

The default graphics mode (GFX=AUTO / GFX=RTG) requires RTG: the game
tries Picasso96 first, then falls back to CyberGraphX (CGX /
cybergraphics.library). If neither works, an English requester is
shown and the game does NOT start (no silent fallback). On a machine
without an RTG card start the game with GFX=AGA, which forces a
standard custom chipset 320x200x8 screen. The supported and tested
configuration is Picasso96 RTG.

Frame presentation uses the fastest path available for the active
driver: p96WritePixelArray on Picasso96, WritePixelArray on
CyberGraphX, and a custom 68060 chunky-to-planar (C2P) converter on
AGA. The active path is logged at startup ("[VIDEO] blit path: ...")
when running from a Shell (raptor > RAPTOR.LOG).


Installation
------------

1. Unpack the archive into a directory of your choice, e.g. Games:.
   It contains a ready-to-use "Raptor" drawer with its own icon:

   Raptor/           the game drawer (with icon)
     raptor          main binary (68060 + FPU build), with icon
     raptor_nofpu    soft-float binary for 68060 systems WITHOUT an
                     FPU (68LC060/68EC060 or a broken FPU), with icon
     README_AMIGA.md this file, with icon

2. Copy the game data files from the original game into the Raptor
   drawer:

   FILE0000.GLB
   FILE0001.GLB
   FILE0002.GLB
   FILE0003.GLB
   FILE0004.GLB

   All five files are required. Only the full version 1.2 of the
   game provides all of them. File names are case insensitive.

3. This Amiga port does not use SETUP.INI at all - all settings have
   built-in defaults and are selected only with command line
   parameters or icon ToolTypes (GFX=, MUSIC=, NOSOUND, ...). The
   game writes saved games and pilot profiles to the program
   directory, so it must be writable.


Running
-------

From Shell/CLI simply start the game:

   raptor

Sound effects play through AHI (ahi.device) and music plays through
the built-in AdLib/OPL3 emulation - the authentic Raptor sound -
mixed into the AHI audio stream. To hear the music on an external
synthesizer or a CAMD software synth instead, start the game with
the MUSIC=CAMD parameter. To play the soundtrack as MP3 files
through an MHI decoder driver (e.g. a Prisma Megamix card), use
MUSIC=MHI. See the "Sound (AHI + CAMD + MHI)" section below
for details and MIDI/MHI configuration.


Sound (AHI + CAMD + MHI)
------------------------

Sound effects and music use two separate, native Amiga subsystems:

   Sound effects:  AHI (ahi.device), 11025 Hz 16-bit stereo - the
                   native rate of the game's samples - played through
                   the standard double-buffered device interface by a
                   dedicated audio task. AHI v4+ must be installed
                   (AHI user package, freely available on Aminet). Any
                   AHI-capable sound card works, as does the built-in
                   Paula through an AHI audio mode.

   Music:          By default the game's MUS tracks play through the
                   built-in AdLib/OPL3 emulation (the authentic
                   Raptor sound), mixed into the AHI audio stream -
                   this always works, out of the box. The emulator
                   uses the lightweight DOSBox dbopl core, which
                   costs only a few percent of a 68060.

                   Alternatively, the MUSIC=CAMD parameter plays the
                   MUS tracks as a General MIDI event stream through
                   camd.library (CAMD), the standard Amiga MIDI API -
                   the same way the Windows and Linux versions of the
                   engine use WinMM and ALSA MIDI. MIDI data is sent
                   to the fixed CAMD output cluster "out.0".

                   To actually hear CAMD music you need one of:
                   - a MIDI interface with an external synthesizer and
                     a CAMD MIDI driver running (e.g. the serial
                     driver from the camd40 package on Aminet), or
                   - a software synthesizer with a CAMD interface
                     (e.g. CAMD Toolkit, or Timidity with a CAMD
                     driver) attached to the "out.0" cluster.

                   WARNING: camd.library alone is NOT enough - if no
                   MIDI driver or synthesizer is attached to the
                   cluster, CAMD music is SILENT (sound effects still
                   play). This includes CaffeineOS, where camd.library
                   is installed but NOT configured by default. If you
                   get no music with MUSIC=CAMD, simply go back to the
                   default AdLib/OPL3 mode. (If camd.library cannot be
                   opened at all, the game falls back to AdLib/OPL3
                   automatically, so music still plays.)

   Music (MHI):   The MUSIC=MHI parameter plays the soundtrack as MP3
                   files through an MHI decoder driver - the Amiga
                   MPEG-audio standard used by hardware decoders such
                   as the Prisma Megamix (prismamhi.library), MAS
                   Player (mhimaspro/mhimasstd.library), Prelude
                   MPEGit (mhimpegit.library) or mpeg.device hardware
                   like the Delfina (mhimdev.library). The driver
                   decodes and outputs the MP3 by itself, so it does
                   not touch the AHI stream used by the sound effects.

                   Create a drawer named "MP3" inside the game
                   directory and copy the MP3 soundtrack files into
                   it. Each in-game song is matched to a file by a
                   case-insensitive title fragment (substring match,
                   "*.mp3" only, first match in the drawer wins) - no
                   track numbers are used, so keep exactly one file per
                   song. The recommended (simplified) file names are:

                       Main Menu.mp3
                       Game Over.mp3
                       Boss 1.mp3
                       Boss 2.mp3
                       Boss 3.mp3
                       Credits.mp3
                       Wave Music 1.mp3
                       Wave Music 2.mp3
                       Wave Music 3.mp3
                       Wave Music 4.mp3
                       Wave Music 5.mp3
                       Wave Music 6.mp3
                       Night Waves.mp3
                       Hangar.mp3
                       Raptor Intro.mp3
                       Apogee Fanfare.mp3

                   ("Fanfare for Duke II.mp3" is the DOS v1.1+
                   replacement name of the Apogee fanfare; this port
                   maps the Apogee logo to "Apogee Fanfare.mp3"
                   instead, so that file is not required.)

                   A song without a matching file simply stays silent
                   (sound effects still play). The mapping (title
                   fragment -> in-game song) is documented in
The MHI volume is capped in code at **5%** of the driver's
                   range (`(volume * 5) / 127`); this was tested on a
                   Prisma Megamix and matches the AHI SFX level. If
                   another MHI driver (MAS Player, Delfina, ...) sounds
                   too quiet with this cap, please report it.
                   src/mpumhi.cpp (mhi_song_map).

                   The game picks the driver automatically: it tries
                   prismamhi.library, mhimaspro/mhimasstd.library,
                   mhimpegit.library, mhimdev.library, then scans
                   LIBS:MHI/ for any other installed driver. The
                   MHIDRIVER= parameter (e.g. -mhidriver=mhimaspro.library)
                   forces a specific driver. If no MHI driver can be
                   opened, the game falls back to AdLib/OPL3 music
                   automatically. Note: there is no software-only MHI
                   decoder for 68060 machines - MUSIC=MHI needs one of
                   the hardware decoders above.

Audio status and diagnostics are printed to the console at startup
(Shell/CLI). No log file is created automatically - to save them to a
file, redirect stdout, e.g.: "raptor > RAPTOR.LOG".


Command Line Parameters
-----------------------

All parameters are recognized case insensitively and may be given
either with a leading dash (GNU style) or without it (AmigaDOS
style). Examples: "-nosound", "NOSOUND" and "nosound" are all
accepted. Parameters may be combined in any order.

   -nosound    Disables ALL audio (music and sound effects). Neither
               ahi.device nor camd.library is ever opened. This is
               the fastest startup path.

   -nomusic    Disables music only; sound effects (gun shots,
               explosions, etc.) keep playing through AHI.
               camd.library is never opened in this mode.

   -music=M    Selects the music backend. M may be:
                 ADLIB - built-in AdLib/OPL3 emulation mixed into
                         the AHI audio stream (default; the
                         authentic Raptor sound, always audible);
                 CAMD  - General MIDI event stream through
                         camd.library; needs a configured MIDI
                         driver or a CAMD software synthesizer on
                         the "out.0" cluster, otherwise the music
                         is SILENT (see "Sound (AHI + CAMD + MHI)"
                         above);
                 MHI   - MP3 files from the MP3/ drawer through an
                         MHI decoder driver (e.g. the Prisma Megamix);
                         falls back to ADLIB when no MHI driver is
                         installed (see "Sound (AHI + CAMD + MHI)"
                         above);
                 OFF   - no music (same as -nomusic; sound effects
                         keep playing through AHI).
               The dashless form "MUSIC=MHI" also works.
               -nomusic / MUSIC=OFF always wins over MUSIC=ADLIB,
               MUSIC=CAMD or MUSIC=MHI, regardless of parameter
               order.

   -mhidriver=D Overrides the MHI decoder driver auto-detection
               (only relevant together with MUSIC=MHI). D is a
               driver library name or full path, e.g.
               "-mhidriver=prismamhi.library" or
               "MHIDRIVER=LIBS:MHI/mhimaspro.library".

   -gfx=M      Selects the graphics driver path used for the game
               screen. M may be:
                 AUTO  - try P96 (Picasso96) RTG first; if P96 is not
                         available or does not offer a matching mode,
                         fall back to CGX (CyberGraphX). If neither
                         driver produces a usable screen, an English
                         requester is shown and the game DOES NOT start
                         (no silent fallback to the classic chipset —
                         this is the default);
                 RTG   - same as AUTO (RTG required, no silent
                         fallback);
                 AGA   - force a native chipset 320x200x8 screen
                         (no RTG required, works on every Amiga).

               The dashless form "GFX=RTG" also works. When the RTG
               path cannot open a 320x200x8 screen it looks for
               320x240x8 instead, opening that with a 40-row black
               bar at the bottom ("letterbox") — this covers RTG
               boards and drivers whose default mode list does NOT
               include 320x200x8 but includes 320x240x8 (a common
               Picasso96 ScreenModes default). This fallback applies
               to both P96 and CGX modes.

               Upon failure the game shows an English requester with
               troubleshooting information: which mode is required,
               and which parameter to use to force a classic chipset
               screen (GFX=AGA / -gfx=AGA).



   REC <file>  Records a demo of your gameplay to the given file
               (e.g. "raptor REC demo1.dem").

   PLAY <file> Plays back a previously recorded demo file
               (e.g. "raptor PLAY demo1.dem").



Running from Shell/CLI
----------------------

Open a Shell (CLI) window, change to the game directory and start
the game, e.g.:

   cd Games:Raptor
   raptor

Parameters work with or without a dash, case insensitively:

   raptor NOMUSIC

Combining parameters:

   raptor -nomusic REC demo1.dem


Running from a Workbench Icon
-----------------------------

The game can be started by double-clicking its icon on the
Workbench. Parameters are passed via icon ToolTypes:

1. Click the raptor icon once, then select
   "Icons / Information..." from the Workbench menu (or press
   Right Amiga + I).

2. In the icon information window add the desired ToolTypes, one
   per line, e.g.:

      NOSOUND
      (MUSIC=CAMD)
      GFX=AGA

   A ToolType enclosed in parentheses is INACTIVE (ignored by the
   game) - this is a convenient way to keep an option in the icon
   without enabling it. Only the active, non-parenthesized forms
   are honored.


3. Click "Save" and double-click the icon to start the game.

Note: to run without audio from the Workbench, add the NOSOUND
ToolType to the icon (NOMUSIC - or MUSIC=OFF - keeps sound effects
enabled). MUSIC=CAMD selects MIDI music and MUSIC=MHI selects MP3
music via an MHI driver, instead of the default AdLib/OPL3 emulation.

Note: when started from the Workbench icon the game produces no
console output (no console window is opened at all). To see the
startup messages, start the game from Shell/CLI instead.



PiStorm / Emu68 Display Troubleshooting
---------------------------------------

On some RTG drivers (notably the PiStorm/Emu68 VideoCore driver)
the native 320x200x8 screen may be displayed incorrectly - the
game image appears squeezed into the left half of the screen,
or with the bottom half cut off. This port tries a 320x200x8 RTG
screen first; if that resolution is not in the driver's mode list
it falls back to a 320x240x8 screen (with a 40-row black bar at
the bottom). If neither mode is available, an English requester
appears asking you to either configure a suitable RTG mode or
start the game with the classic chipset screen (GFX=AGA).

Note: the middle mouse button is ignored. On some machines (notably
A1200 + PiStorm/Emu68, on both RTG and AGA screens) it produces
phantom presses that skip the intro logos, exit demos instantly and
fight the steering. Cycling the special weapon is always available
on SPACE. Left and right mouse buttons work
normally in all modes.


Controls
--------


In this port keyboard, mouse and joystick work simultaneously -
there is no need to select a device in the options. The mouse takes
over ship control when you physically move it (or hold a mouse
button); keyboard and joystick remain active at all times.

Keyboard - in game:

   Arrow keys       - Move ship
   Left CTRL        - Fire primary weapon
   Left ALT         - Fire special weapon
   SPACE            - Cycle active special weapon
   Right SHIFT      - MegaFire (mega bomb)
   1 ... 0, "-"     - Direct special weapon selection (see below)
   P                - Pause
   F1               - Help
   ESC              - Abort mission - return to main menu
   ALT + X          - Quit to system (with confirmation)
   BACKSPACE        - Full version only: adds Death Ray + energy,
                      resets score

Direct special weapon selection (if available in inventory):

   1  Dumb Missile        6  Ground Missile
   2  Mini Gun            7  Bomb
   3  Turret              8  Energy Grab

   4  Missile Pods        9  Pulse Cannon
   5  Air Missile         0  Death Ray
   -  Forward Laser

Mouse - in game:

   Mouse movement   - Ship control (cursor position)
   Left button      - Fire primary weapon
   Right button     - Fire special weapon
   Middle button    - Cycle active special weapon

Joystick / CD32 pad (port 1):

Requires lowlevel.library v40+ (standard in AmigaOS 3.2). The port
is switched to game controller mode, so standard 1- and 2-button
joysticks and CD32 pads are supported.

   Stick / D-pad    - Move ship
   FIRE 1 (red)     - Fire primary weapon
   FIRE 2 (blue)    - Fire special weapon
   CD32 PLAY        - Fire special weapon

Notes:
- Special weapon cycling and MegaFire are only available from
  keyboard (SPACE / Right SHIFT) or mouse (middle button).
- Pause is only available from keyboard (P) - the Amiga joystick
  has no Start button.
- Without lowlevel.library the joystick is unavailable, but the
  game works normally with keyboard and mouse.

Menu controls (keyboard / mouse / joystick):

   Arrow keys / joystick D-pad  - Navigate options
   ENTER or SPACE               - Select option
   Left mouse button            - Select option (click)
   FIRE 1 (red)                 - Select option
   ESC                          - Back / cancel
   FIRE 2 (blue) / CD32 PLAY    - Back / cancel
   F1                           - Context help
   ALT + X                      - Quit to system
   Arrows / PgUp / PgDn / Home / End - Scroll help window
   BACKSPACE (in text fields)   - Delete character
   CTRL + Y (in text fields)    - Clear entire field


Known Limitations
-----------------

- MIDI music (MUSIC=CAMD) needs a MIDI driver or a CAMD software
  synthesizer attached to the "out.0" cluster - otherwise the music
  is silent while sound effects still play. Note for CaffeineOS
  users: camd.library is installed there but NOT configured by
  default, so MUSIC=CAMD plays to nowhere - keep the default
  AdLib/OPL3 music (or configure a CAMD driver first).
- MP3 music (MUSIC=MHI) needs an MHI decoder driver installed in
  LIBS:MHI/ (Prisma Megamix, MAS Player, Prelude MPEGit or
  mpeg.device hardware such as the Delfina). There is no
  software-only MHI decoder for 68060 machines; without a driver
  the game falls back to AdLib/OPL3 music. Songs whose MP3 file is
  missing from the MP3/ drawer stay silent by design.
- Music and sound-effect volumes changed in the in-game options menu
  are saved to amiga.cfg in the game directory (created on first run)
  and restored on the next start. Detail level is not persisted.
- No pause or menu exit directly from joystick (use keyboard).
- No rumble / haptic support.
- The Amiga system mouse pointer is hidden while the game is
  running (restored on exit to system).
- Fixed 320x200 resolution - the game always opens its own screen.


Credits & Contact
-----------------

   Amiga Port Author:  Marcin "Raybeez" Bednarczyk (aka Cichy)
   AI Collaboration:   Built with assistance from AI tools.
   Contact / Feedback: cichy@cichy.com.pl
   GitHub Repository:  https://github.com/RaybeezPL

   Special thanks to all Amiga users keeping the scene alive.



License and Thanks
------------------


This port does NOT contain game data files. You need your own legal
copy of Raptor: Call of the Shadows. Only the full version 1.2 is
supported.

This port is based on the skynettx/raptor project.
Thanks to nukeykt and the contributors of the reconstructed Raptor
code, and to the Amiga community and the creators of RTG and AHI
tools.


Third-party components used by this port:

- Game engine: skynettx/raptor reconstruction, GNU GPL-2.0 (see the
  LICENSE file); includes code derived from Chocolate Doom (GNU GPL-2.0+).
- dbopl OPL emulator (src/dbopl.cpp/h): DOSBox Team, GNU GPL-2.0+.
- TinySoundFont (include/TinySoundFont/tsf.h): Bernhard Schelling, MIT
  license (GPL-compatible).
- AHI and CAMD interface headers (src/amiga/devices/ahi.h,
  src/amiga/midi/, src/amiga/clib/camd_protos.h): vendored unmodified
  from the official, freely distributable developer archives on Aminet
  (AHI dev archive by Martin Blom; CAMD developer kit). They contain
  interface definitions only (structures, constants, prototypes) and are
  used to call the respective AmigaOS system libraries at runtime.
- MHI interface headers (src/amiga/libraries/mhi.h,
  src/amiga/clib/mhi_protos.h): vendored from the official MHI
  developer kit v1.2 (Aminet driver/audio/mhi_dev.lha, MHI by Thomas
  Wenzel and Paul Qureshi).
- src/amiga/proto/camd.h and src/amiga/inline/camd.h were hand-written
  for this port (LVO offsets verified against the official camd_lib.fd).
- src/amiga/proto/mhi.h and src/amiga/inline/mhi.h were hand-written
  for this port (LVO offsets and register assignments verified against
  the official mhi_lib.fd from the MHI developer kit).
- Display blit acceleration (src/amiga/amiga_sdl_stubs.h): the RTG paths
  call the official Picasso96/CyberGraphX driver APIs; the CGX
  WritePixelArray inline stub was hand-written for this port (LVO
  offset verified against the official CGraphX-DevKit VI
  cybergraphics_lib.fd).
- The AGA chunky-to-planar (C2P) converter in the same file is original
  code written for this port (GNU GPL-2.0), validated bit-exact against
  a brute-force reference. It uses only the well-known published 8x8
  bit-matrix transpose algorithm - no third-party/demo-scene C2P code
  is used.


GNU License Compliance / Source Code Notice:
--------------------------------------------

This project is currently in the beta testing phase. In compliance
with the GNU license, the full source code will be made publicly
available upon the final release.

Once finished, the source code will be included directly within the
Aminet archive alongside the compiled port. Additionally, the GitHub
repository will be changed from private to fully public at that time.


