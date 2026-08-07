Raptor: Call of the Shadows - Amiga Port (68060/RTG)

Version: 0.9.0 SOUND

=====================================================

Amiga port of Raptor: Call of the Shadows, based on the open source
engine reconstruction (reverse engineering) by skynettx/raptor.

This port is developed exclusively for classic Amiga systems:
AmigaOS 3.2, 68060 CPU with FPU, and RTG graphics (Picasso96).
The game renders in 320x200 resolution with 8-bit palette on a
dedicated RTG screen.

NOTE: This repository does NOT contain game data files. You need
your own legal copy of the original game files. Only the full
version 1.2 is supported. The game may be purchased on Steam:
https://store.steampowered.com/app/358360/Raptor_Call_of_the_Shadows_1994_Classic_Edition/


Requirements
------------

Processor:  Motorola 68060 (or PiStorm / Emu68)
FPU:        Built-in 68060 / 68881 / 68882 - REQUIRED
            (binary compiled with -m68881)
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
Sound:      AHI (ahi.device v4+) for sound effects;
            music via MIDI out through camd.library (CAMD), with
            automatic AdLib/OPL3 fallback when CAMD is unavailable

Without an RTG card the game will attempt to open a standard custom
chipset screen, but the supported and tested configuration is
Picasso96 RTG.  When Picasso96 is not available the game falls back to
CyberGraphX (CGX / cybergraphics.library) automatically.


Installation
------------

1. Copy the raptor executable to a directory of your choice,
   e.g. Games:Raptor/

2. Copy the game data files from the original game into the same
   directory:

   FILE0000.GLB
   FILE0001.GLB
   FILE0002.GLB
   FILE0003.GLB
   FILE0004.GLB

   All five files are required. Only the full version 1.2 of the
   game provides all of them. File names are case insensitive.

3. This Amiga port bypasses the existence of setup.ini. The game
   does not require generating or placing a setup.ini file in the
   directory. The game writes saved games and pilot profiles to the
   program directory, so it must be writable.


Running
-------

From Shell/CLI simply start the game:

   raptor

Sound effects play through AHI (ahi.device) and music is sent as a
MIDI stream through camd.library (CAMD). If camd.library is not
installed, the game automatically falls back to the built-in
AdLib/OPL3 emulation, so music always plays. See the "Sound
(AHI + CAMD)" section below for details and MIDI configuration.


Sound (AHI + CAMD)
------------------

Sound effects and music use two separate, native Amiga subsystems:

   Sound effects:  AHI (ahi.device), 11025 Hz 16-bit stereo - the
                   native rate of the game's samples - played through
                   the standard double-buffered device interface by a
                   dedicated audio task. AHI v4+ must be installed
                   (AHI user package, freely available on Aminet). Any
                   AHI-capable sound card works, as does the built-in
                   Paula through an AHI audio mode.

   Music:          The game's MUS tracks are played as a General MIDI
                   event stream through camd.library (CAMD), the
                   standard Amiga MIDI API - the same way the Windows
                   and Linux versions of the engine use WinMM and
                   ALSA MIDI. MIDI data is sent to the CAMD cluster
                   "out.0" by default; a different cluster name can be
                   set with the SETUP.INI option:

                       [Setup]
                       camd_cluster=<name>

                   To actually hear CAMD music you need one of:
                   - a MIDI interface with an external synthesizer and
                     a CAMD MIDI driver running (e.g. the serial
                     driver from the camd40 package on Aminet), or
                   - a software synthesizer with a CAMD interface
                     (e.g. CAMD Toolkit, or Timidity with a CAMD
                     driver) attached to the configured cluster.

                   If camd.library is not installed at all, the game
                   automatically falls back to the built-in AdLib/OPL3
                   emulation (the authentic Raptor sound) mixed into
                   the AHI audio stream - music always works.  The
                   emulator uses the lightweight DOSBox dbopl core,
                   which costs only a few percent of a 68060.

Audio status and diagnostics are printed at startup (Shell/CLI) and
also written to RAPTOR.LOG in the game directory.


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
      (NOMUSIC)
      GFX=AGA

   A ToolType enclosed in parentheses is INACTIVE (ignored by the
   game) - this is a convenient way to keep an option in the icon
   without enabling it. Only the active, non-parenthesized forms
   are honored.


3. Click "Save" and double-click the icon to start the game.

Note: to run without audio from the Workbench, add the NOSOUND
ToolType to the icon (NOMUSIC keeps sound effects enabled).

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

Note: the middle mouse button is ignored. On some RTG machines it
produces phantom presses that skip the intro logos, exit demos
instantly and fight the steering. Cycling the special weapon is
always available on SPACE. Left and right mouse buttons work
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

- MIDI music through CAMD needs a MIDI driver or a CAMD software
  synthesizer attached to the output cluster (default "out.0") -
  otherwise CAMD music is silent (sound effects still play; install
  no camd.library at all to force the AdLib/OPL3 fallback instead).
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


GNU License Compliance / Source Code Notice:
--------------------------------------------

This project is currently in the beta testing phase. In compliance
with the GNU license, the full source code will be made publicly
available upon the final release.

Once finished, the source code will be included directly within the
Aminet archive alongside the compiled port. Additionally, the GitHub
repository will be changed from private to fully public at that time.


