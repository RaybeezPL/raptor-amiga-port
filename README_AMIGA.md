Raptor: Call of the Shadows - Amiga Port (68060/RTG)

Version: BETA 0.8.1 NOSOUND

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

Graphics:   RTG with Picasso96 (e.g. CyberVision 64/3D, Picasso IV,
            UAEGFX/PiStorm) - 320x200x8 mode required
Disk space: approx. 25 MB free (game files + saved games)
Joystick:   Optional - port 1 (DB9); requires lowlevel.library v40+
            (included in AmigaOS 3.2); CD32 pads are also supported
Sound:      Not available yet - AHI support is work in progress;
            the game must be started with -nosound

Without an RTG card the game will attempt to open a standard custom
chipset screen, but the supported and tested configuration is
Picasso96 RTG.


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

The only currently tested and working configuration is with sound
disabled. From Shell/CLI:

   raptor -nosound

The Amiga-style syntax without a dash also works, case insensitive:

   raptor NOSOUND

Sound status: The audio path (AHI / ahi.device) is not yet complete.
Starting the game without -nosound is not currently supported and
may result in a device open error or a hang, depending on your AHI
configuration. Always start the game with -nosound (this is also
the fastest startup path for testing). The -nomusic option exists
in the code but has no practical use until audio is working.


Command Line Parameters
-----------------------

All parameters are recognized case insensitively and may be given
either with a leading dash (GNU style) or without it (AmigaDOS
style). Examples: "-nosound", "NOSOUND" and "nosound" are all
accepted. Parameters may be combined in any order.

   -nosound    Disables ALL audio (music and sound effects). The
               ahi.device / audio backend is never opened. This is
               the fastest startup path and is currently REQUIRED.

   -nomusic    Disables music only; sound effects remain enabled.
               Note: until the AHI audio path is finished this option
               has no practical use - use -nosound.

   -rtgmode=M  Selects the RTG display mode used for the game screen.
               M may be:
                 8    - native 320x200x8 palette screen (default,
                        best for classic RTG cards and WinUAE);
                 8X2L - 640x240x8 screen, the game image is doubled
                        horizontally in software (320 -> 640 per row)
                        and blitted 1:1 starting at the top of the
                        screen (workaround for RTG drivers that show
                        the 320x200x8 mode squeezed to half the screen
                        width, e.g. PiStorm/Emu68; 8X2 accepted as an
                        alias). 640x240 matches the native buffer
                        geometry of such drivers.

               If the requested mode is unavailable the game falls
               back to the native 320x200x8 mode. The dashless form
               "RTGMODE=8X2L" works as well.



   REC <file>  Records a demo of your gameplay to the given file
               (e.g. "raptor -nosound REC demo1.dem").

   PLAY <file> Plays back a previously recorded demo file
               (e.g. "raptor -nosound PLAY demo1.dem").



Running from Shell/CLI
----------------------

Open a Shell (CLI) window, change to the game directory and start
the game with the desired parameters, e.g.:

   cd Games:Raptor
   raptor -nosound

or in AmigaDOS style:

   raptor NOSOUND

Combining parameters:

   raptor -nosound REC demo1.dem


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
      RTGMODE=8X2L

   A ToolType enclosed in parentheses is INACTIVE (ignored by the
   game) - this is a convenient way to keep an option in the icon
   without enabling it. Only the active, non-parenthesized forms
   are honored.

   The RTGMODE ToolType accepts the same values as the -rtgmode
   command line parameter: RTGMODE=8 (default) or RTGMODE=8X2L.
   See "Command Line Parameters" above.



3. Click "Save" and double-click the icon to start the game.

Note: with sound support still being worked on, the icon should
contain the NOSOUND ToolType, otherwise the game may fail to start.

Note: when started from the Workbench icon the game produces no
console output (no console window is opened at all). To see the
startup messages, start the game from Shell/CLI instead.



PiStorm / Emu68 Display Troubleshooting
---------------------------------------

On some RTG drivers (notably the PiStorm/Emu68 VideoCore driver)
the native 320x200x8 screen may be displayed incorrectly - the
game image appears squeezed into the left half of the screen,
or with the bottom half cut off. If you see this, start the
game with the 8X2L display mode:

   raptor -nosound -rtgmode=8x2l

This opens a 640x240x8 screen, which matches the native buffer
geometry of such drivers (640 bytes/row, up to 240 rows - so the
driver does not need to crop or rescale the bitmap), doubles the
game image horizontally in software (320 -> 640 per row) and
blits it 1:1 starting at the top of the screen. The driver's own
scaling stretches the 240-row screen to the full display height.
The mode is also available as an icon ToolType (RTGMODE=8X2L).


If a requested mode is not available on your system, the game
falls back to the native 320x200x8 mode automatically.

Note: in the 8X2L mode the middle mouse button is ignored. On
some machines (e.g. A1200 + PiStorm/Emu68 in this scan-doubled
mode) the middle button line produces phantom presses that skip
the intro logos, exit demos instantly and fight the steering.
Cycling the special weapon is always available on SPACE. Left
and right mouse buttons work normally in all modes.


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

- No sound - the game must be started with -nosound (AHI work in
  progress).
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


