#pragma once

#ifdef __AMIGA__

int MHI_WarpMusicInit(const char *library_path, const char *driver_name,
                      int volume);
void MHI_WarpMusicDeInit(void);
int MHI_WarpIsActive(void);
const char *MHI_WarpDriverName(void);
void MHI_WarpPlayPath(const char *path, int loop);
void MHI_WarpStopSong(void);
int MHI_WarpSongPlaying(void);
void MHI_WarpSetVolume(int volume);

#endif /* __AMIGA__ */
