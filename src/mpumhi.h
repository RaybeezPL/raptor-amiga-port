#pragma once

/* mpumhi.h - MHI (MPEG audio) music backend interface, Amiga port only.
 *
 * Plays MP3 files from the "MP3/" drawer (in the game directory) through
 * an MHI decoder driver (e.g. LIBS:MHI/prismamhi.library for the Prisma
 * Megamix).  Selected with MUSIC=MHI (see RAP_ParseMusic in src/rap.cpp);
 * consumed by the SND_* song entry points in src/fx.cpp.  All functions
 * are no-ops/failure when the backend is not active. */

#ifdef __AMIGA__

/* Opens an MHI driver and starts the feeder task.  Returns 1 on success,
 * 0 on failure (the caller then switches the selected backend to
 * MUSIC=OFF - there is no automatic fallback to AdLib/OPL3). */
int MHI_MusicInit(void);

/* Stops playback and shuts the feeder task down.  Idempotent. */
void MHI_MusicDeInit(void);

/* 1 when MHI_MusicInit() succeeded and the feeder task is alive. */
int MHI_IsActive(void);

/* Name of the opened MHI decoder driver (for logging). */
const char *MHI_DriverName(void);

/* Maps a GLB music item id (FILE0xx_*_MUS) to an MP3 file in the MP3/
 * drawer and starts playing it.  loop != 0 restarts the file forever.
 * When no matching MP3 exists the music is simply silent (logged). */
void MHI_PlaySongItem(int item, int loop);

/* Stops the current song immediately (MHI has no fade-out). */
void MHI_StopSong(void);

/* 1 while a song is playing (or looping), 0 when idle/ended/stopped. */
int MHI_SongPlaying(void);

/* Music volume 0..127 (Raptor scale, same as the in-game slider and
 * MUS_SetVolume).  Internally stored as-is; scaled to the MHI driver's
 * 0..100 MHIP_VOLUME range (percent) inside the feeder task via
 * MHI_ScaleVolume() when the driver supports MHIQ_VOLUME_CONTROL,
 * otherwise ignored. Note: Volume scaling from 0..127 to 0..100 is handled internally. */
void MHI_SetVolume(int volume);

/* Optional driver override from the MHIDRIVER= CLI parameter / icon
 * ToolType: a driver library name or full path (e.g. "prismamhi.library"
 * or "LIBS:MHI/mhimaspro.library").  Call before MHI_MusicInit(). */
void MHI_SetDriverOverride(const char *name);

#endif /* __AMIGA__ */
