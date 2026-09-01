/* mpuwave.cpp - WAVE music backend.
 *
 * Plays 11025 Hz stereo 16-bit PCM WAV files from the "WAVE/" drawer
 * (in the game directory) by mixing them directly into the AHI audio
 * stream.  Selected with MUSIC=WAVE; WAVE_Mix() is called from
 * FX_Fill() (src/fx.cpp) when g_music_mode == MUSIC_MODE_WAVE, after
 * the stream has been zeroed and before GSS_Mix()/DSP_Mix() add the
 * sound effects on top.
 *
 * The mixer only ever ADDS volume-scaled samples into the stream and
 * hard-clamps the 32-bit intermediate sum to the int16 range, so it
 * can neither overwrite nor overflow the SFX mixed by GSS/DSP. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "fx.h"          /* music_volume (0..127) */
#include "fileids.h"     /* FILE0xx_*_MUS item ids */
#include "entypes.h"     /* SDL_SwapLE16 / LE_* endian helpers */

/* ------------------------------------------------------------------ */
/* Playback state                                                      */
/* ------------------------------------------------------------------ */

static int16_t *wave_data = NULL;   /* decoded PCM, stereo S16 interleaved */
static long     wave_frames = 0;    /* number of stereo frames in wave_data */
static long     wave_pos = 0;       /* current playback frame */
static int      wave_playing = 0;
static int      wave_loop = 0;

/* ------------------------------------------------------------------ */
/* Little-endian helpers (WAV is always LE)                            */
/* ------------------------------------------------------------------ */

static uint16_t rd_u16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/***************************************************************************
WAVE_LoadSong() - Loads a 11025 Hz stereo 16-bit PCM WAV file into memory.
Returns 1 on success, 0 on failure.
 ***************************************************************************/
int WAVE_LoadSong(const char *path, int loop)
{
    FILE *f;
    uint8_t hdr[12];
    uint8_t *pcm = NULL;
    long pcm_size = 0;
    int got_fmt = 0, got_data = 0;
    uint16_t channels = 0, bits = 0;
    uint32_t rate = 0;

    WAVE_StopSong();

    f = fopen(path, "rb");
    if (!f)
        return 0;

    if (fread(hdr, 1, 12, f) != 12 ||
        memcmp(hdr, "RIFF", 4) != 0 || memcmp(hdr + 8, "WAVE", 4) != 0)
    {
        fclose(f);
        return 0;
    }

    /* Walk the RIFF chunks looking for fmt and data. */
    for (;;)
    {
        uint8_t chdr[8];
        uint32_t csize;

        if (fread(chdr, 1, 8, f) != 8)
            break;
        csize = rd_u32(chdr + 4);

        if (memcmp(chdr, "fmt ", 4) == 0)
        {
            uint8_t fmt[16];
            if (csize < 16 || fread(fmt, 1, 16, f) != 16)
                break;
            if (csize > 16)
                fseek(f, (long)(csize - 16), SEEK_CUR);

            if (rd_u16(fmt) != 1) /* PCM only */
                break;
            channels = rd_u16(fmt + 2);
            rate     = rd_u32(fmt + 4);
            bits     = rd_u16(fmt + 14);
            got_fmt  = 1;
        }
        else if (memcmp(chdr, "data", 4) == 0)
        {
            pcm = (uint8_t *)malloc(csize);
            if (!pcm)
                break;
            if (fread(pcm, 1, csize, f) != csize)
            {
                free(pcm);
                pcm = NULL;
                break;
            }
            pcm_size = (long)csize;
            got_data = 1;
        }
        else
        {
            fseek(f, (long)csize, SEEK_CUR);
        }

        if (csize & 1) /* chunks are word-aligned */
            fseek(f, 1, SEEK_CUR);

        if (got_fmt && got_data)
            break;
    }

    fclose(f);

    if (!got_fmt || !got_data || channels != 2 || bits != 16 || rate != 11025)
    {
        free(pcm);
        return 0;
    }

    /* WAV stores 16-bit PCM samples in little-endian byte order; convert
     * to the host's native int16 representation so WAVE_Mix() reads correct
     * values on both little- and big-endian targets.  SDL_SwapLE16() is a
     * no-op on little-endian hosts and swaps bytes on big-endian ones (e.g.
     * the Motorola 68k Amiga).  Without this conversion every sample is
     * byte-swapped on BE, producing noise/static that also masks all SFX
     * mixed in by DSP_Mix()/GSS_Mix() (the garbage music fills the int16
     * range so the quieter SFX are clamped into inaudibility). */
    {
        long nsamp = pcm_size / 2;  /* total 16-bit samples (stereo) */
        uint16_t *swp = (uint16_t *)pcm;
        long k;

        for (k = 0; k < nsamp; k++)
            swp[k] = SDL_SwapLE16(swp[k]);
    }

    wave_data    = (int16_t *)pcm;
    wave_frames  = pcm_size / 4; /* 2 channels * 2 bytes */
    wave_pos     = 0;
    wave_loop    = loop;
    wave_playing = 1;

    return 1;
}

/***************************************************************************
WAVE_StopSong() - Stops playback and frees the loaded song.
 ***************************************************************************/
void WAVE_StopSong(void)
{
    wave_playing = 0;
    wave_pos = 0;
    free(wave_data);
    wave_data = NULL;
    wave_frames = 0;
}

/***************************************************************************
WAVE_SongPlaying() - 1 while a song is playing (or looping), 0 otherwise.
 ***************************************************************************/
int WAVE_SongPlaying(void)
{
    return wave_playing;
}

/***************************************************************************
 * Song mapping: GLB music item -> WAV file title fragment.
 *
 * The WAVE/ drawer holds one file per song named by the plain soundtrack
 * title ("Wave Music 1.wav", ...) - the same names the MP3/ drawer uses
 * for MUSIC=MHI.  Lookup is a case-insensitive substring match of the
 * fragment against the file name.  Keep exactly one matching file per
 * song in the drawer.
 ***************************************************************************/
static const struct
{
    int         item;               /* FILE0xx_*_MUS from fileids.h */
    const char *fragment;           /* title fragment (substring match) */
} wave_song_map[] = {
    { FILE061_APOGEE_MUS,   "Apogee Fanfare" },
    { FILE056_RINTRO_MUS,   "Raptor Intro"   },
    { FILE057_MAINMENU_MUS, "Main Menu"      },
    { FILE058_HANGAR_MUS,   "Hangar"         },
    { FILE060_RAP8_MUS,     "Wave Music 1"   },
    { FILE05a_RAP2_MUS,     "Wave Music 2"   },
    { FILE05c_RAP4_MUS,     "Wave Music 3"   },
    { FILE05f_RAP7_MUS,     "Wave Music 4"   },
    { FILE05e_RAP6_MUS,     "Wave Music 5"   },
    { FILE05b_RAP3_MUS,     "Wave Music 6"   },
    { FILE059_RAP1_MUS,     "Night Waves"    },
    { FILE05d_RAP5_MUS,     "Game Over"      },
    { FILE052_BOSS1_MUS,    "Boss 1"         },
    { FILE053_BOSS2_MUS,    "Boss 2"         },
    { FILE054_BOSS3_MUS,    "Boss 3"         },
    { FILE055_BOSS4_MUS,    "Credits"        },
    { 0, NULL }
};

/***************************************************************************
WAVE_PlaySongItem() - map a GLB music item to a WAV file in the WAVE/
drawer and play it.  Returns 1 on success, 0 when no file matched (the
song then stays silent, same convention as the MHI backend).
 ***************************************************************************/
int WAVE_PlaySongItem(int item, int loop)
{
    char path[256];
    int i;

    for (i = 0; wave_song_map[i].item; i++)
    {
        if (wave_song_map[i].item == item)
        {
            const char *frag = wave_song_map[i].fragment;

            /* The drawer uses the plain soundtrack titles; try the exact
             * "<fragment>.wav" name first (fast path, no dir scan). */
            snprintf(path, sizeof(path), "WAVE/%s.wav", frag);
            if (WAVE_LoadSong(path, loop))
                return 1;

            return 0;
        }
    }

    return 0;
}

/***************************************************************************
WAVE_Mix() - Adds the loaded WAV music into the output stream.

stream: stereo S16 buffer, already zeroed by FX_Fill(); SFX are mixed
        into the same buffer by GSS_Mix()/DSP_Mix().
frames: number of stereo frames to mix.

Each WAV sample is scaled by music_volume (0..127) as
    sample * music_volume >> 7
then ADDED to the existing stream sample; the 32-bit sum is clamped to
[-32768, 32767] so overlapping music + SFX can never wrap around.
 ***************************************************************************/
void WAVE_Mix(int16_t *stream, int frames)
{
    int i;

    if (!wave_playing || !wave_data || !stream || frames <= 0)
        return;

    if (music_volume <= 0)
        return;

    for (i = 0; i < frames; i++)
    {
        int32_t l, r;

        if (wave_pos >= wave_frames)
        {
            if (wave_loop)
            {
                wave_pos = 0;
            }
            else
            {
                wave_playing = 0;
                break;
            }
        }

        l = (wave_data[wave_pos * 2]     * music_volume) >> 7;
        r = (wave_data[wave_pos * 2 + 1] * music_volume) >> 7;
        wave_pos++;

        l += stream[i * 2];
        r += stream[i * 2 + 1];

        if (l > 32767)  l = 32767;
        if (l < -32768) l = -32768;
        if (r > 32767)  r = 32767;
        if (r < -32768) r = -32768;

        stream[i * 2]     = (int16_t)l;
        stream[i * 2 + 1] = (int16_t)r;
    }
}
