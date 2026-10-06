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
 * JOYSTICK / CD32 block:
 * - cd32          : OFF (default, classic joystick) / ON (CD32 pad on
 *                   port 1, lowlevel.library game controller mode).
 * - cd32_red/blue/green/yellow/reverse/forward/play : text action name
 *                   per pad button, one of the AMIGA_CFG_CD32_* ids below.
 *                   Unknown values keep the built-in safe defaults.
 *
 * AmigaCfg_Load() is called at startup (SND_InitSound) - file wins over
 * the built-in defaults.  AmigaCfg_Save() is called when the in-game
 * Options sliders are exited and always writes BOTH blocks, so manual
 * edits, slider changes and CD32 assignments all stick. */

#ifdef __AMIGA__

/* Volumes read from / written to amiga.cfg (0..127, clamped). */
extern int amiga_cfg_music_adlib;
extern int amiga_cfg_music_mhi;
extern int amiga_cfg_music_wave;
extern int amiga_cfg_sfx;

/* CD32 pad mode: 0 = OFF (default), 1 = ON. */
extern int amiga_cfg_cd32;

/* CD32 pad button actions. */
#define AMIGA_CFG_CD32_NONE           0   /* button disabled */
#define AMIGA_CFG_CD32_FIRE           1   /* main guns */
#define AMIGA_CFG_CD32_SPECIAL_SELECT 2   /* fire the selected special weapon */
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

/* Loads amiga.cfg from the current directory.  Missing file = built-in
 * defaults (127 / 127 / 127).  Idempotent in memory (re-reads file). */
void AmigaCfg_Load(void);

/* Writes the current amiga_cfg_* values back to amiga.cfg. */
void AmigaCfg_Save(void);

#endif /* __AMIGA__ */