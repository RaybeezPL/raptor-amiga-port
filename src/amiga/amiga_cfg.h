#pragma once

/* amiga_cfg.h - persistent audio volume settings for the Amiga port.
 *
 * Stored in "amiga.cfg" in the game directory (next to the MP3/ drawer).
 * Simple "key = value" lines, range 0..127, ';' starts a comment.
 *
 * - music_adlib : ADLIB/OPL3 music, mixed into the AHI stream.
 * - music_mhi   : MP3 music via MHI (Prisma Megamix), separate hardware
 *                 output, extra 2% software cap in mpumhi.cpp.
 * - sfx_volume  : sound effects through the AHI stream.
 *
 * AmigaCfg_Load() is called at startup (SND_InitSound) - file wins over
 * the built-in 127/127/100 defaults.  AmigaCfg_Save() is called when the
 * in-game Options sliders are exited, so manual edits and slider changes
 * both stick. */

#ifdef __AMIGA__

/* Volumes read from / written to amiga.cfg (0..127, clamped). */
extern int amiga_cfg_music_adlib;
extern int amiga_cfg_music_mhi;
extern int amiga_cfg_sfx;

/* Loads amiga.cfg from the current directory.  Missing file = built-in
 * defaults (127 / 127 / 100).  Idempotent in memory (re-reads file). */
void AmigaCfg_Load(void);

/* Writes the current amiga_cfg_* values back to amiga.cfg. */
void AmigaCfg_Save(void);

#endif /* __AMIGA__ */