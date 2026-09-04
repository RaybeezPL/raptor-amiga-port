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
 * can neither overwrite nor overflow the SFX mixed by GSS/DSP.
 *
 * ------------------------------------------------------------------
 * Resource management (Amiga fix):
 *
 *  - Every access to the shared playback state (wave_data, wave_frames,
 *    wave_pos, wave_playing, wave_loop) is serialized with SND_Lock()/
 *    SND_Unlock().  On Amiga these map to SDL_LockAudioDevice() =
 *    Disable()/Enable(), which suspends the preemptive scheduler while
 *    the lock is held.  WAVE_Mix() runs in the AHI background task,
 *    while WAVE_StopSong()/WAVE_LoadSong() run in the main game task;
 *    without the lock, stop/change-song could free() or overwrite
 *    wave_data while the audio task was still reading it (use-after-
 *    free -> heap corruption -> progressive slowdown / menu hang).
 *
 *  - Song caching: WAVE_PlaySongItem() remembers the last loaded path
 *    and loop flag.  Re-requesting the exact same file (menu handshake
 *    re-triggers SND_PlaySong with the same item) returns immediately
 *    without reopening/re-malloc'ing a multi-MB buffer - this was
 *    fragmenting the heap rapidly in menu<->demo transitions.
 *
 *  - WAVE_DeInit() frees everything; SND_DeInit() calls it before the
 *    AHI task is shut down.
 *
 * All transitions are logged with "WAVE:" diagnostics so leaks and
 * repeated loads can be traced in one go.
 * ------------------------------------------------------------------ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "fx.h"          /* music_volume (0..127), SND_Lock/SND_Unlock */
#include "fileids.h"     /* FILE0xx_*_MUS item ids */
#include "entypes.h"     /* LE_* endian helpers */

/* Portable byte-swap for 16-bit values.  On Amiga SDL_SwapLE16() comes
 * from SDL_endian.h via entypes.h (amiga_sdl_stubs.h); on a host build
 * without SDL we fall back to a local implementation.  This wrapper is
 * ALWAYS available so the decode loop below never depends on SDL headers. */
#ifdef SDL_SwapLE16
static inline uint16_t WAVE_SwapLE16(uint16_t v) { return SDL_SwapLE16(v); }
#else
static inline uint16_t WAVE_SwapLE16(uint16_t v)
{
    uint16_t probe = 1;
    if (*(uint8_t *)&probe == 1)   /* little-endian host */
        return v;
    return (uint16_t)((v << 8) | (v >> 8));
}
#endif

/* ------------------------------------------------------------------ */
/* Playback state                                                      */
/* ------------------------------------------------------------------ */

static int16_t *wave_data = NULL;   /* decoded PCM, stereo S16 interleaved */
static long     wave_frames = 0;    /* number of stereo frames in wave_data */
static long     wave_pos = 0;       /* current playback frame */
static int      wave_playing = 0;
static int      wave_loop = 0;

/* Cache of the currently loaded song so we don't reopen/realloc the same
 * file on every SND_PlaySong(SAME_ITEM) call triggered by menu handshakes. */
static char     wave_path[256] = "";  /* "" = nothing loaded */
static int      wave_path_loaded = 0;

/* Diagnostics: monotonic counters so a log can show balancing alloc/free. */
static unsigned long wave_alloc_count = 0;
static unsigned long wave_free_count  = 0;
static unsigned long wave_open_count  = 0;
static unsigned long wave_close_count = 0;
static unsigned long wave_play_count  = 0;

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

/* ------------------------------------------------------------------ */
/* Amiga diagnostics (no-op elsewhere)                                 */
/* ------------------------------------------------------------------ */

#ifdef __AMIGA__
#define WAVE_LOG(...) do { AmigaLog(__VA_ARGS__); } while (0)
#else
#define WAVE_LOG(...) do { printf(__VA_ARGS__); printf("\n"); fflush(stdout); } while (0)
#endif

/***************************************************************************
WAVE_StopSong() - Stops playback and frees the loaded song.
Safely serialized against WAVE_Mix() running in the audio task.
 ***************************************************************************/
void WAVE_StopSong(void)
{
    SND_Lock();          /* serialize against WAVE_Mix (audio task) */
    {
        int was_playing = wave_playing;
        int had_data    = (wave_data != NULL);

        wave_playing = 0;
        wave_pos = 0;

        if (wave_data)
        {
            free(wave_data);
            wave_data = NULL;
            wave_frames = 0;
            wave_free_count++;
            WAVE_LOG("WAVE: free data (alloc=%lu free=%lu, playing=%d)",
                     wave_alloc_count, wave_free_count, was_playing);
        }

        if (wave_loop != 0 || had_data || was_playing)
            WAVE_LOG("WAVE: stop (playing=%d had_data=%d)",
                     was_playing, had_data ? 1 : 0);

        wave_loop  = 0;
        wave_path_loaded = 0;
        wave_path[0] = '\0';
    }
    SND_Unlock();
}

/* Internal: same as WAVE_StopSong() but caller must already hold SND_Lock.
 * Keeps the load path atomic: stop-old -> (read file outside lock) -> set-new. */
static void WAVE_StopSongLocked(void)
{
    int was_playing = wave_playing;
    int had_data    = (wave_data != NULL);

    wave_playing = 0;
    wave_pos = 0;

    if (wave_data)
    {
        free(wave_data);
        wave_data = NULL;
        wave_frames = 0;
        wave_free_count++;
        WAVE_LOG("WAVE: free data (alloc=%lu free=%lu, playing=%d)",
                 wave_alloc_count, wave_free_count, was_playing);
    }

    if (wave_loop != 0 || had_data || was_playing)
        WAVE_LOG("WAVE: stop (playing=%d had_data=%d)",
                 was_playing, had_data ? 1 : 0);

    wave_loop  = 0;
    wave_path_loaded = 0;
    wave_path[0] = '\0';
}

/***************************************************************************
WAVE_DeInit() - Frees everything. Called from SND_DeInit() BEFORE the AHI
audio task is shut down (so no audio task is reading the buffer anymore).
 ***************************************************************************/
void WAVE_DeInit(void)
{
    if (!wave_data && !wave_path_loaded)
        return;

    WAVE_LOG("WAVE: deinit (alloc=%lu free=%lu open=%lu close=%lu play=%lu)",
             wave_alloc_count, wave_free_count,
             wave_open_count, wave_close_count, wave_play_count);

    WAVE_StopSong();
}

/***************************************************************************
WAVE_LoadSong() - Loads a 11025 Hz stereo 16-bit PCM WAV file into memory.
Returns 1 on success, 0 on failure.
Safe against WAVE_Mix() in the audio task.
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

    if (!path || !*path)
        return 0;

    /* Fast path: the exact same file+loop is already loaded (menu/demo
     * handshake re-triggers SND_PlaySong for the same item).  Avoids a
     * multi-MB malloc/free cycle and the accompanying heap fragmentation. */
    {
        int same = 0;
        SND_Lock();
        if (wave_path_loaded && wave_data &&
            wave_loop == loop && strcmp(wave_path, path) == 0)
        {
            wave_pos = 0;
            wave_playing = 1;
            same = 1;
        }
        SND_Unlock();

        if (same)
        {
            WAVE_LOG("WAVE: reusing cached song '%s' (loop=%d) - no reload",
                     path, loop);
            return 1;
        }
    }

    /* Free the previous song while holding the lock (atomic stop). */
    SND_Lock();
    {
        WAVE_StopSongLocked();
    }
    SND_Unlock();

    /* ---- I/O is done outside SND_Lock (do not hold Disable() across
     * file reads / large mallocs - it would stall the whole system) ---- */

    f = fopen(path, "rb");
    if (!f)
    {
        WAVE_LOG("WAVE: open FAILED '%s' (does the WAVE/ drawer have it?)", path);
        return 0;
    }
    wave_open_count++;
    WAVE_LOG("WAVE: open '%s' (loop=%d)", path, loop);

    if (fread(hdr, 1, 12, f) != 12 ||
        memcmp(hdr, "RIFF", 4) != 0 || memcmp(hdr + 8, "WAVE", 4) != 0)
    {
        WAVE_LOG("WAVE: not a RIFF/WAVE file '%s'", path);
        fclose(f);
        wave_close_count++;
        WAVE_LOG("WAVE: close '%s'", path);
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
            if (csize > 64u * 1024u * 1024u) /* sanity: 64 MB max */
            {
                WAVE_LOG("WAVE: '%s' data chunk too large (%lu bytes) - rejected",
                         path, (unsigned long)csize);
                break;
            }

            pcm = (uint8_t *)malloc(csize ? (size_t)csize : 1);
            if (!pcm)
            {
                WAVE_LOG("WAVE: malloc(%lu) FAILED for '%s'",
                         (unsigned long)csize, path);
                break;
            }
            if (fread(pcm, 1, csize, f) != csize)
            {
                WAVE_LOG("WAVE: short read on data chunk of '%s'", path);
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
    wave_close_count++;
    WAVE_LOG("WAVE: close '%s' (open=%lu close=%lu)",
             path, wave_open_count, wave_close_count);

    if (!got_fmt || !got_data || channels != 2 || bits != 16 || rate != 11025)
    {
        if (pcm)
        {
            free(pcm);
            wave_free_count++;
        }
        WAVE_LOG("WAVE: '%s' rejected (fmt=%d data=%d ch=%u bits=%u rate=%lu; "
                 "need stereo 16-bit 11025 Hz)",
                 path, got_fmt, got_data, channels, bits, (unsigned long)rate);
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
            swp[k] = WAVE_SwapLE16(swp[k]);
    }

    wave_alloc_count++;
    WAVE_LOG("WAVE: alloc %ld frames (%lu bytes) from '%s'",
             pcm_size / 4, (unsigned long)pcm_size, path);

    /* ---- Publish the new buffer atomically (under the lock) ---- */
    SND_Lock();
    {
        /* No need to free the old buffer again - we already stopped it
         * above.  Just install the freshly decoded data. */
        wave_data    = (int16_t *)pcm;
        wave_frames  = pcm_size / 4; /* 2 channels * 2 bytes */
        wave_pos     = 0;
        wave_loop    = loop;
        wave_playing = 1;

        snprintf(wave_path, sizeof(wave_path), "%s", path);
        wave_path_loaded = 1;

        wave_play_count++;
    }
    SND_Unlock();

    WAVE_LOG("WAVE: play start '%s' (loop=%d) [play=%lu alloc=%lu free=%lu]",
             path, loop, wave_play_count, wave_alloc_count, wave_free_count);

    return 1;
}

/***************************************************************************
WAVE_SongPlaying() - 1 while a song is playing (or looping), 0 otherwise.
 ***************************************************************************/
int WAVE_SongPlaying(void)
{
    int r = 0;

    SND_Lock();
    r = wave_playing;
    SND_Unlock();

    return r;
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

/* Builds the underscore variant of a title fragment: every space is
 * replaced by '_' ("Main Menu" -> "Main_Menu").  The original fragment
 * is never modified.  Used as a compatibility fallback for archives
 * that replace spaces automatically. */
static void
WAVE_UnderscoreVariant(
    const char *src,
    char *out,
    int outlen
)
{
    int i = 0;

    if (!src || !out || outlen <= 0)
        return;

    while (*src && i < outlen - 1)
    {
        out[i++] = (*src == ' ') ? '_' : *src;
        src++;
    }
    out[i] = 0;
}

/***************************************************************************
WAVE_PlaySongItem() - map a GLB music item to a WAV file in the WAVE/
drawer and play it.  Returns 1 on success, 0 when no file matched (the
song then stays silent, same convention as the MHI backend).
 ***************************************************************************/
int WAVE_PlaySongItem(int item, int loop)
{
    char path[256];
    char uscore[256];
    char plain_path[256];
    int i;

    for (i = 0; wave_song_map[i].item; i++)
    {
        if (wave_song_map[i].item == item)
        {
            const char *frag = wave_song_map[i].fragment;
            int has_space = 0;

            /* The drawer uses the plain soundtrack titles; try the exact
             * "<fragment>.wav" name first (fast path, no dir scan). */
            snprintf(plain_path, sizeof(plain_path), "WAVE/%s.wav", frag);
            if (WAVE_LoadSong(plain_path, loop))
                return 1;

            /* Compatibility fallback for archives that replace spaces
             * automatically: try "<fragment with underscores>.wav".
             * Only when the fragment actually contains a space - otherwise
             * the underscore variant is identical and would be a duplicate
             * open attempt. */
            {
                const char *p;
                for (p = frag; *p; p++)
                {
                    if (*p == ' ')
                    {
                        has_space = 1;
                        break;
                    }
                }
            }

            if (has_space)
            {
                WAVE_UnderscoreVariant(frag, uscore, sizeof(uscore));
                snprintf(path, sizeof(path), "WAVE/%s.wav", uscore);
                if (WAVE_LoadSong(path, loop))
                    return 1;
            }

            WAVE_LOG("WAVE: no playable file for item=%d (tried '%s'%s)",
                     item, plain_path,
                     has_space ? " and underscore variant" : "");
            return 0;
        }
    }

    WAVE_LOG("WAVE: no song mapping for item=%d", item);
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

Runs in the AHI audio task; the SND_Lock()/Unlock() pair serializes
wave_data access against WAVE_StopSong()/WAVE_LoadSong() in the main task.
 ***************************************************************************/
void WAVE_Mix(int16_t *stream, int frames)
{
    int16_t *data;
    long pos, total;
    int loop;
    int i;

    if (!stream || frames <= 0)
        return;

    /* Snapshot the state under the lock once per mix call (the audio task
     * holds the lock for the whole mix, so no other task can free the
     * buffer while we are iterating). */
    SND_Lock();

    if (!wave_playing || !wave_data)
    {
        SND_Unlock();
        return;
    }

    if (music_volume <= 0)
    {
        SND_Unlock();
        return;
    }

    data  = wave_data;
    pos   = wave_pos;
    total = wave_frames;
    loop  = wave_loop;

    for (i = 0; i < frames; i++)
    {
        int32_t l, r;

        if (pos >= total)
        {
            if (loop)
            {
                pos = 0;
            }
            else
            {
                wave_playing = 0;
                break;
            }
        }

        l = (data[pos * 2]     * music_volume) >> 7;
        r = (data[pos * 2 + 1] * music_volume) >> 7;
        pos++;

        l += stream[i * 2];
        r += stream[i * 2 + 1];

        if (l > 32767)  l = 32767;
        if (l < -32768) l = -32768;
        if (r > 32767)  r = 32767;
        if (r < -32768) r = -32768;

        stream[i * 2]     = (int16_t)l;
        stream[i * 2 + 1] = (int16_t)r;
    }

    wave_pos = pos;

    SND_Unlock();
}