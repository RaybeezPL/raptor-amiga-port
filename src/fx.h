#pragma once

#include <stdint.h>

#define SND_CLOSE    40
#define SND_FAR      500

enum 
{
    FXHAND_PCS = 0x0000,
    FXHAND_GSS1 = 0x4000,
    // FXHAND_GSS2 = 0x8000,
    FXHAND_DSP = 0xc000,
    FXHAND_MASK = 0x3fff,
    FXHAND_TMASK = 0xc000,
};

typedef enum
{
    SND_NONE,
    SND_PC,
    SND_MIDI,
    SND_CANVAS,
    SND_DIGITAL
}SND_TYPE;

typedef enum
{
    FX_THEME,
    FX_MON1,
    FX_MON2,
    FX_MON3,
    FX_MON4,
    FX_MON5,
    FX_MON6,
    FX_DAVE,
    FX_AIREXPLO,
    FX_AIREXPLO2,
    FX_BONUS,
    FX_CRASH,
    FX_DOOR,
    FX_FLYBY,
    FX_EGRAB,
    FX_GEXPLO,
    FX_GUN,
    FX_JETSND,
    FX_LASER,
    FX_MISSLE,
    FX_SWEP,
    FX_TURRET,
    FX_WARNING,
    FX_BOSS1,
    FX_IJETSND,
    FX_EJETSND,
    FX_INTROHIT,
    FX_INTROGUN,
    FX_ENEMYSHOT,
    FX_ENEMYLASER,
    FX_ENEMYMISSLE,
    FX_ENEMYPLASMA,
    FX_SHIT,
    FX_HIT,
    FX_NOSHOOT,
    FX_PULSE,
    FX_LAST_SND
}DEFX;

extern int music_volume;
extern int fx_volume;
extern int dig_flag;
extern int fx_freq;
extern int fx_gus;
extern int sys_midi, winmm_mpu_device, core_dls_synth, core_midi_port, alsaclient, alsaport;

/* Command-line audio disable flags (-nosound / -nomusic), parsed in main()
 * (src/rap.cpp) and consumed by SND_InitSound() (src/fx.cpp):
 *   -nosound : disables ALL audio (music AND sound effects). ahi.device /
 *              the audio backend is never even opened.
 *   -nomusic : disables music only. Sound effects (gun shots, explosions,
 *              etc.) keep working normally. */
extern int g_nosound;
extern int g_nomusic;

/* MUSIC=<mode> music backend selection (-music= switch / MUSIC= icon
 * ToolType), parsed in main() (src/rap.cpp) and consumed by SND_InitSound()
 * (src/fx.cpp):
 *   MUSIC_MODE_ADLIB (default) : built-in AdLib/OPL3 emulation (dbopl core),
 *              mixed into the AHI audio stream - always audible.
 *   MUSIC_MODE_CAMD            : General MIDI event stream via camd.library;
 *              needs a configured MIDI driver/synth on cluster "out.0",
 *              otherwise the music is silent.
 *   MUSIC_MODE_MHI             : MP3 files from the MP3/ drawer through an
 *              MHI decoder driver (e.g. LIBS:MHI/prismamhi.library); needs
 *              an installed MHI driver, otherwise it falls back to ADLIB.
 *   MUSIC_MODE_WAVE            : 11025 Hz stereo 16-bit PCM WAV files from
 *              the WAVE/ drawer, mixed directly into the AHI audio stream
 *              (mpuwave.cpp) - no external driver needed.
 * MUSIC=OFF (or -nomusic) sets g_nomusic instead and always wins. */
#define MUSIC_MODE_ADLIB 0
#define MUSIC_MODE_CAMD  1
#define MUSIC_MODE_MHI   2
#define MUSIC_MODE_WAVE  3
extern int g_music_mode;

/* WAVE music backend (src/mpuwave.cpp) - plays 11025 Hz stereo 16-bit PCM
 * WAV files from the WAVE/ drawer, mixed into the AHI stream by FX_Fill()
 * when g_music_mode == MUSIC_MODE_WAVE. */
int  WAVE_LoadSong(const char *path, int loop);
int  WAVE_PlaySongItem(int item, int loop);
void WAVE_StopSong(void);
void WAVE_DeInit(void);
int  WAVE_SongPlaying(void);
void WAVE_Mix(int16_t *stream, int frames);

#include "SDL.h"

extern SDL_AudioDeviceID fx_dev;


int SND_InitSound(void);
void SND_DeInit(void);
void SND_Setup(void);
void SND_PlaySong(int item, int chainflag, int fadeflag);
int SND_IsSongPlaying(void);
void SND_Lock(void);
void SND_Unlock(void);
void SND_FadeOutSong(void);
void SND_FreeFX(void);
void SND_CacheFX(void);
void SND_CacheGFX(void);
void SND_CacheIFX(void);
void SND_3DPatch(int type, int x, int y);
void SND_Patch(int type, int xpos);
int SND_IsPatchPlaying(int type);
void SND_StopPatch(int type);
void SND_StopPatches(void);
