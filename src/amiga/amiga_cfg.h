#pragma once

/* amiga_cfg.h - persistent settings for the Amiga port.
 *
 * Stored in "amiga.cfg" in the game directory (next to the MP3/ drawer).
 * Simple "key = value" lines, ';' starts a comment.
 *
 * AUDIO block (volumes, range 0..127, clamped):
 * - music_adlib : ADLIB/OPL3 music, mixed into the AHI stream.
 * - music_mhi   : MP3 music via MHI (any compatible MHI decoder driver), separate hardware
 *                 output.
 * - music_wave  : WAV music from the WAVE/ drawer, mixed into the AHI
 *                 stream.
 * - sfx_volume  : sound effects through the AHI stream.
 *
 * Action names shared by the joystick and CD32 blocks (one of the
 * AMIGA_CFG_CD32_* ids below, case-insensitive; NONE is a full, valid
 * action):
 * - FIRE           : fire the primary guns and the selected secondary weapon.
 * - SPECIAL_SELECT : change (cycle) the secondary weapon.
 * - MEGA_BOMB      : launch a mega bomb.
 * - PAUSE          : pause the game.
 * - CANCEL         : cancel / back out.
 * - NONE           : button disabled.
 *
 * Block order in a newly generated amiga.cfg: AUDIO, then the shared
 * JOYSTICK / 1-2-3 BUTTONS block, then the JOYSTICK / CD32 block.  The order
 * applies to the template, the documented examples and the one-time legacy
 * migration described below; an existing amiga.cfg is otherwise never
 * reordered or extended automatically.
 *
 * Legacy migration (the only exception to "existing files are never
 * extended"): an amiga.cfg that holds nothing but audio volume keys (comments
 * and blank lines allowed) is completed once at load time - the missing
 * volume keys and the JOYSTICK blocks are appended with the built-in
 * defaults, keeping the original values, comments and line endings.  The
 * original file is retained as the first free amiga.cfg.bakN copy.  A file
 * that already contains controller keys (cd32* / joy_button* / joy3_button*)
 * or any other active key is never migrated automatically.
 *
 * JOYSTICK / 1-2-3 BUTTONS block (an Amiga DB9 joystick on the second
 * physical port; active whenever cd32 = OFF, which is the default - there is
 * no separate enable key):
 * - joy_button1/2/3 : text action name per physical DB9 button (pin 6, pin
 *                   9, pin 5), same action ids as the CD32 block. Unknown
 *                   values keep the built-in safe defaults.
 * - joy3_button1/2/3 : deprecated aliases of joy_button1/2/3, still
 *                   accepted so existing files keep their assignments. A
 *                   valid joy_buttonN wins over the alias regardless of
 *                   line order; the alias applies only when no valid
 *                   joy_buttonN is present.
 * - joy3          : deprecated and ignored. A leftover "joy3 = OFF" no
 *                   longer disables the shared joystick handling.
 *
 * JOYSTICK / CD32 block:
 * - cd32          : OFF (default, shared joystick handling) / ON (CD32 pad
 *                   on port 1, lowlevel.library game controller mode).
 * - cd32_red/blue/green/yellow/reverse/forward/play : text action name
 *                   per pad button, one of the AMIGA_CFG_CD32_* ids below.
 *                   Unknown values keep the built-in safe defaults.
 *
 * AmigaCfg_Load() is called once at startup (SDL_Init, before the gameport
 * mode is chosen; SND_InitSound calls it again harmlessly for the volume
 * keys) - file wins over
 * the built-in defaults.  A legacy volumes-only file is completed once at
 * this point (see the migration note above).  AmigaCfg_Save() is called when
 * the in-game Options sliders are exited and only rewrites the volume values.
 * Existing files retain manual edits, CD32 and joystick assignments and
 * unknown content when their volume values are updated. */

#ifdef __AMIGA__

/* Volumes read from / written to amiga.cfg (0..127, clamped). */
extern int amiga_cfg_music_adlib;
extern int amiga_cfg_music_mhi;
extern int amiga_cfg_music_wave;
extern int amiga_cfg_sfx;

/* CD32 pad mode: 0 = OFF (default), 1 = ON. */
extern int amiga_cfg_cd32;

/* CD32 pad button actions. The three-button joystick block reuses the same
 * ids, so both blocks accept exactly the same action names. */
#define AMIGA_CFG_CD32_NONE           0   /* button disabled */
#define AMIGA_CFG_CD32_FIRE           1   /* primary guns + selected secondary */
#define AMIGA_CFG_CD32_SPECIAL_SELECT 2   /* change (cycle) secondary weapon */
#define AMIGA_CFG_CD32_MEGA_BOMB      3   /* launch a mega bomb */
#define AMIGA_CFG_CD32_PAUSE          4   /* pause the game */
#define AMIGA_CFG_CD32_CANCEL         5   /* cancel / back out */
#define AMIGA_CFG_CD32_NUM_ACTIONS    6

/* Assignment per physical CD32 pad button (AMIGA_CFG_CD32_* ids). */
extern int amiga_cfg_cd32_red;
extern int amiga_cfg_cd32_blue;
extern int amiga_cfg_cd32_green;
extern int amiga_cfg_cd32_yellow;
extern int amiga_cfg_cd32_reverse;
extern int amiga_cfg_cd32_forward;
extern int amiga_cfg_cd32_play;

/* Assignment per physical joystick button, DB9 pin 6 / pin 9 / pin 5
 * (AMIGA_CFG_CD32_* action ids). Active whenever cd32 is OFF. */
extern int amiga_cfg_joy_button1;
extern int amiga_cfg_joy_button2;
extern int amiga_cfg_joy_button3;

/* Loads amiga.cfg from the current directory.  Missing file = built-in
 * defaults: all four volumes (music_adlib, music_mhi, music_wave,
 * sfx_volume) default to 127 (maximum), CD32 mode defaults to OFF
 * (shared joystick handling) and the CD32 button assignments default to
 * red = FIRE, blue = SPECIAL_SELECT, green = MEGA_BOMB,
 * yellow / reverse / forward = NONE, play = PAUSE.
 * The joystick buttons default to button1 = FIRE,
 * button2 = SPECIAL_SELECT and button3 = MEGA_BOMB.
 * Loads once per session; later calls are no-ops. */
void AmigaCfg_Load(void);

/* Updates current volume values in amiga.cfg without rebuilding an existing
 * file. */
void AmigaCfg_Save(void);

#ifdef AMIGA_CFG_TEST
void AmigaCfg_TestSetPath(const char *path);
void AmigaCfg_TestResetLoad(void);
void AmigaCfg_TestFailNextTempOpen(void);
void AmigaCfg_TestFailNextReplace(void);
void AmigaCfg_TestFailReplacementAfterBackup(void);
#endif

#endif /* __AMIGA__ */