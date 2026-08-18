/***************************************************************************
* mpumhi.cpp - MHI (MPEG audio) music backend, Amiga port only.
 *
 * Plays the game's music as MP3 files through an MHI decoder driver
 * instead of the built-in MUS/OPL3 sequencer.  Opt-in via MUSIC=MHI
 * (CLI parameter or icon ToolType, see RAP_ParseMusic in src/rap.cpp).
 *
 * MHI (Thomas Wenzel / Paul Qureshi) is the Amiga MPEG-audio driver
 * standard: there is no central library for applications - the game opens
 * a decoder DRIVER library directly (drivers live in LIBS:MHI/, e.g.
 * prismamhi.library for the Prisma Megamix).  The driver decodes and
 * outputs the audio by itself, so nothing is mixed into the game's AHI
 * stream and the MUS sequencer is never started in this mode.
 *
 * Files come from the "MP3/" drawer (in the game directory, relative path
 * like the GLB data files).  See mhi_song_map below for the item->file
 * mapping and the file lookup rules.  A song without a matching file is
 * simply silent (logged).
 *
 * Streaming follows the canonical pattern from the MHI dev kit's
 * MHIplay.c: buffers are queued to the driver, which returns them with a
 * signal when consumed.  A dedicated feeder task owns the driver library,
 * the file I/O and the refill loop, so the music keeps playing no matter
 * what the game is doing (level loads, menus, ...).  The main task only
 * sends commands (PLAY/STOP/VOLUME/QUIT) through a shared command block
 * plus a task signal, and reads the playback state.
 *
 * Notes:
 *  - No STDIO inside the feeder task (same rule as the AHI audio task);
 *    diagnostics are recorded in g_mhi and logged from the main task.
 *  - MHI has no fade-out: stops are immediate (the game's fade waits are
 *    skipped by the MHI branches in fx.cpp).
 ***************************************************************************/
#ifdef __AMIGA__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "SDL.h"
#include "common.h"
#include "fileids.h"
#include "mpumhi.h"

#include <libraries/mhi.h>
#include <proto/mhi.h>


/* Volume scaling helper function */
static ULONG MHI_ScaleVolume(int volume)
{
    if (volume < 0) volume = 0;
    if (volume > 127) volume = 127;
    return (ULONG)((volume * 100 + 63) / 127);
}


/* MHI driver library base - the inline calls in inline/mhi.h use it.
 * Written by the feeder task, which is also the only caller of MHI
 * functions, so there is no cross-task contention on the driver API. */
struct Library *MHIBase = NULL;

/* Log through AmigaLog on the SDL-stub build, plain console otherwise.
 * Output goes to stdout only; run "raptor > RAPTOR.LOG" to capture it.
 * NEVER call from the feeder task (no STDIO there) - main task only. */
#ifdef USE_SDL_STUBS
#define MHI_LOG(...) AmigaLog(__VA_ARGS__)
#else
#define MHI_LOG(...) do { printf(__VA_ARGS__); printf("\n"); fflush(stdout); } while (0)
#endif

/* Stream buffers: 4 x 32 KB = 128 KB total (~8 s at 128 kbit/s), owned
 * by the main task (MEMF_PUBLIC) and only used by the feeder task. */
#define MHI_NUM_BUFS   4
#define MHI_BUF_SIZE   (32 * 1024)

/* Commands main task -> feeder task (g_mhi.cmd). */
enum
{
    MHICMD_NONE = 0,
    MHICMD_PLAY,
    MHICMD_STOP,
    MHICMD_VOLUME,
    MHICMD_QUIT
};

/* Feeder playback states (g_mhi.state), read by the main task. */
enum
{
    MHISTATE_IDLE = 0,      /* nothing playing */
    MHISTATE_PLAYING,       /* song active */
    MHISTATE_ENDED,         /* non-looping song played to the end */
    MHISTATE_ERROR          /* init/open failure */
};

static struct MHIState
{
    /* Written by the main task before MHI_MusicInit() (MHIDRIVER=). */
    char driver_override[256];

    /* Main-owned buffer memory (feeder only reads/writes the contents). */
    UBYTE *buffers[MHI_NUM_BUFS];

    /* Feeder <-> main handshake. */
    struct Process *proc;           /* main: CreateNewProcTags result */
    struct Task    *task;           /* feeder: its own task (for Signal) */
    volatile LONG   ready;          /* 0 pending, 1 up, <0 failed stage */
    volatile LONG   running;        /* 1 while the feeder process exists */
    volatile LONG   state;          /* MHISTATE_* */

    /* Command channel: main writes the parameter fields first, then cmd;
     * feeder clears cmd when the command has been consumed. */
    volatile ULONG  cmd;
    volatile ULONG  cmd_loop;       /* PLAY: loop flag */
    volatile ULONG  cmd_volume;     /* VOLUME: 0..127 (Raptor scale) */
    char            cmd_path[300];  /* PLAY: full path to the MP3 file */

    /* Owned by the feeder task (main must not touch these). */

    ULONG  cmd_sigmask;             /* feeder signal poked by commands */
    struct Library *base;           /* opened MHI driver library (== MHIBase) */
    APTR   decoder;                 /* MHI decoder handle */
    BPTR   file;                    /* currently streamed MP3 file */
    LONG   file_pos;                /* current read offset */
    LONG   file_start;              /* first byte after any ID3v2 tag */
    LONG   file_end;                /* effective end (ID3v1 trimmed) */
    LONG   eof;                     /* all file data has been queued */
    LONG   loop;                    /* current song loops */
    volatile LONG queued;           /* stream buffers owned by the driver */
    volatile LONG stop_incomplete;  /* a stop left buffers in the driver */
    volatile ULONG traffic_ticks;   /* last driver traffic (SDL_GetTicks) */
    LONG   vol_supported;           /* driver has MHIQ_VOLUME_CONTROL */
    LONG   volume;                  /* last requested volume 0..127 (Raptor scale; scaled to 0..100 for MHI driver) */

    volatile LONG open_error;       /* last PLAY could not open the file */
    volatile LONG debug_open;
    volatile LONG debug_seek_end;
    volatile LONG debug_seek_back;
    volatile LONG debug_read10;
    volatile LONG debug_start;
    volatile LONG debug_end;
    char   driver_name[64];         /* MHIQ_DECODER_NAME copy */
    char   opened_path[256];        /* driver library path that opened */
} g_mhi;

/***************************************************************************
 * Song mapping: GLB music item -> MP3 file title fragment.
 *
 * The MP3/ drawer holds one file per song named by the plain soundtrack
 * title ("Wave Music 1.mp3", ...).  Lookup is a case-insensitive
 * substring match of the fragment against the file name - no track
 * numbers are used.  Keep exactly one file per song in the drawer (first
 * match wins).  A song without a matching file plays silence (logged).
 *
 * The RAP1..RAP8 assignment follows the original DOS game (wave song
 * tables in windows.cpp): Bravo wave 1 = RAP8 = "Wave Music 1", the
 * demo/Bravo wave 3 = RAP4 = "Wave Music 3"; the rest follows the order
 * of first in-game appearance (Bravo W1..W9): RAP2 = WM2, RAP7 = WM4,
 * RAP6 = WM5, RAP3 = WM6.  RAP1 plays only in the Tango sector (night
 * missions) and Outer Regions, hence "Night Waves".  RAP5 is the death
 * tune ("Game Over"), not a wave song.  "Fanfare for Duke II" (the DOS
 * v1.1+ replacement for the Apogee fanfare) is intentionally unmapped.
 ***************************************************************************/
static const struct
{
    int         item;               /* FILE0xx_*_MUS from fileids.h */
    const char *fragment;           /* title fragment (substring match) */
} mhi_song_map[] = {
    { FILE061_APOGEE_MUS,   "Apogee Fanfare" },  /* Apogee logo         */
    { FILE056_RINTRO_MUS,   "Raptor Intro"   },  /* intro               */
    { FILE057_MAINMENU_MUS, "Main Menu"      },  /* main menu           */
    { FILE058_HANGAR_MUS,   "Hangar"         },  /* hangar              */
    { FILE060_RAP8_MUS,     "Wave Music 1"   },  /* Bravo W1, Outer W1  */
    { FILE05a_RAP2_MUS,     "Wave Music 2"   },  /* waves 2/6           */
    { FILE05c_RAP4_MUS,     "Wave Music 3"   },  /* Bravo W3/W8, demo   */
    { FILE05f_RAP7_MUS,     "Wave Music 4"   },  /* Bravo W4, Outer W4  */
    { FILE05e_RAP6_MUS,     "Wave Music 5"   },  /* waves 5/9           */
    { FILE05b_RAP3_MUS,     "Wave Music 6"   },  /* Bravo W7, Tango W1  */
    { FILE059_RAP1_MUS,     "Night Waves"    },  /* Tango W8, Outer W3  */
    { FILE05d_RAP5_MUS,     "Game Over"      },  /* death jingle        */
    { FILE052_BOSS1_MUS,    "Boss 1"         },  /* (unused in-game)    */
    { FILE053_BOSS2_MUS,    "Boss 2"         },  /* credits rotation    */
    { FILE054_BOSS3_MUS,    "Boss 3"         },  /* credits rotation    */
    /* There is no "Boss 4" recording in the soundtrack; in this codebase
     * BOSS4 is only used by the credits screen rotation, so it gets the
     * "Credits" track. */
    { FILE055_BOSS4_MUS,    "Credits"        },
    { 0, NULL }
};

/* MHI decoder driver candidates, tried in order when no MHIDRIVER=
 * override is given.  The Prisma Megamix driver comes first (primary
 * test hardware of this port); afterwards the LIBS:MHI/ drawer is
 * scanned for any other installed driver. */
static const char * const mhi_default_drivers[] = {
    "LIBS:MHI/prismamhi.library",       /* Prisma Megamix (clockport MP3)   */
    "LIBS:MHI/mhimaspro.library",       /* MAS Player Pro                   */
    "LIBS:MHI/mhiArmedWarp.library",    /* Warp                             */
    "LIBS:MHI/mhimasstd.library",       /* MAS Player standard              */
    "LIBS:MHI/mhimpegit.library",       /* Prelude / MPEGit module          */
    "LIBS:MHI/mhimdev.library",         /* mpeg.device bridge (Delfina, ...)*/
    NULL
};

/***************************************************************************
 * Small case-insensitive string helpers (ASCII only, no locale).
 ***************************************************************************/
static char
MHI_ToUpper(
    char c
)
{
    if (c >= 'a' && c <= 'z')
        c = (char)(c - 'a' + 'A');

    return c;
}

static int
MHI_ContainsCI(
    const char *hay,
    const char *needle
)
{
    if (!hay || !needle || !*needle)
        return 0;

    for (; *hay; hay++)
    {
        const char *h = hay;
        const char *n = needle;

        while (*h && *n && MHI_ToUpper(*h) == MHI_ToUpper(*n))
        {
            h++;
            n++;
        }

        if (!*n)
            return 1;
    }

    return 0;
}

static int
MHI_EqualCI(
    const char *a,
    const char *b
)
{
    if (!a || !b)
        return 0;

    while (*a && *b)
    {
        if (MHI_ToUpper(*a) != MHI_ToUpper(*b))
            return 0;
        a++;
        b++;
    }

    return (*a == 0 && *b == 0);
}

/***************************************************************************
 * MP3 drawer lookup (main task side).
 *
 * The drawer is "MP3/" relative to the current directory - the same
 * convention the GLB data files use (plain relative fopen()).  Songs are
 * matched by title fragment only; track-number file names are not used.
 ***************************************************************************/
static const char *
MHI_MP3Base(
void
)
{
    return "MP3/";
}

/***************************************************************************
 * MHI_ScanDrawer() - scan the MP3 drawer for the song's file: the first
 * ".mp3" file whose name contains the title fragment (case-insensitive).
 * Keep exactly one matching file per song in the drawer - first match
 * wins.  Returns 1 with the full path in out, 0 when not found.
 ***************************************************************************/
static int
MHI_ScanDrawer(
    const char *base,
    const char *fragment,
    char *out,
    int outlen
)
{
    struct AnchorPath *ap;
    char pat[40];
    LONG rc;
    int found = 0;

    (void)outlen;   /* base + fib_FileName[108] always fits in 300 */

    if (!fragment || !*fragment)
        return 0;

    /* Old-style AnchorPath: the path buffer follows the struct. */
    ap = (struct AnchorPath *)AllocVec(sizeof(struct AnchorPath) + 512, MEMF_CLEAR);
    if (!ap)
        return 0;

    ap->ap_Strlen = 512;

    sprintf(pat, "%s#?", base);

    rc = MatchFirst((CONST_STRPTR)pat, ap);

    while (rc == 0 && !found)
    {
        const char *name = ap->ap_Info.fib_FileName;

        if (MHI_ContainsCI(name, ".mp3") && MHI_ContainsCI(name, fragment))
        {
            sprintf(out, "%s%s", base, name);
            found = 1;
        }
        else
            rc = MatchNext(ap);
    }

    MatchEnd(ap);
    FreeVec(ap);
    return found;
}

/***************************************************************************
 * MHI_FindFileForItem() - resolve a GLB music item to an MP3 path.
 * Pure title-fragment lookup - no track numbers.  Returns 1 and the
 * path, or 0 when nothing matched (-> silence).
 ***************************************************************************/
static int
MHI_FindFileForItem(
    int item,
    char *out,
    int outlen
)
{
    int i;

    out[0] = 0;

    for (i = 0; mhi_song_map[i].item; i++)
    {
        if (mhi_song_map[i].item == item)
            return MHI_ScanDrawer(MHI_MP3Base(), mhi_song_map[i].fragment, out, outlen);
    }

    return 0;
}

/***************************************************************************
 * Feeder task side - all MHI driver calls and all MP3 file I/O live here
 * (never in the main task).  No STDIO in any of these functions.
 ***************************************************************************/

/******************************************************************************************
 * MHI_FeederStop() - stop playback, reclaim ALL stream buffers from the
 * driver, close the file.  A buffer still owned by the driver must never
 * be refilled/requeued for the next song (double ownership corrupts the
 * driver state), so the stop waits until the queued count reaches zero.
 ******************************************************************************************/
static void
MHI_FeederStop(
    void
)
{
    /* Prevent any further refills while the decoder is being stopped. */
    g_mhi.eof = 1;
    g_mhi.loop = 0;

    if (g_mhi.decoder)
    {
        int i;

        /* Only stop a decoder that is actually running - MHIStop on an
         * already stopped decoder is driver-dependent behaviour. */
        if (MHIGetStatus(g_mhi.decoder) != MHIF_STOPPED)
        {
            MHIStop(g_mhi.decoder);

            for (i = 0; i < 50; i++)
            {
                if (MHIGetStatus(g_mhi.decoder) == MHIF_STOPPED)
                    break;

                Delay(1);
            }
        }

        /* Reclaim every buffer the driver hands back after the stop; the
         * last ones may arrive with a delay, so poll the queued count
         * instead of stopping at the first NULL. */
        for (i = 0; i < 100 && g_mhi.queued > 0; i++)
        {
            while (MHIGetEmpty(g_mhi.decoder) != NULL)
            {
                if (g_mhi.queued > 0)
                    g_mhi.queued--;
            }

            if (g_mhi.queued > 0)
                Delay(1);
        }

        if (g_mhi.queued > 0)
        {
            /* The driver never returned all buffers (logged from the main
             * task on the next PLAY).  Resync: stale buffers may still come
             * back later and are safely refilled by MHI_Service(). */
            g_mhi.stop_incomplete = g_mhi.queued;
            g_mhi.queued = 0;
        }
    }

    if (g_mhi.file)
    {
        Close(g_mhi.file);
        g_mhi.file = 0;
    }
}

/***************************************************************************
 * MHI_FillBuffer() - refill one driver-returned buffer from the file and
 * queue it again.  Handles loop seek and end-of-file.  Returns 1 when
 * data was queued.
 ***************************************************************************/
static int
MHI_FillBuffer(
    UBYTE *buf
)
{
    LONG room, got;

    if (!g_mhi.file || g_mhi.eof)
        return 0;

    room = g_mhi.file_end - g_mhi.file_pos;

    if (room <= 0)
    {
        if (!g_mhi.loop)
        {
            g_mhi.eof = 1;
            return 0;
        }

        /* Looping song: wrap to the first byte after the ID3v2 tag. */
        Seek(g_mhi.file, g_mhi.file_start, OFFSET_BEGINNING);
        g_mhi.file_pos = g_mhi.file_start;
        room = g_mhi.file_end - g_mhi.file_pos;
    }

    if (room > MHI_BUF_SIZE)
        room = MHI_BUF_SIZE;

    got = Read(g_mhi.file, buf, room);

    if (got <= 0)
    {
        /* Read error (or zero bytes): treat as end of file. */
        g_mhi.eof = 1;
        return 0;
    }

    g_mhi.file_pos += got;

    if (!MHIQueueBuffer(g_mhi.decoder, buf, (ULONG)got))
    {
        g_mhi.eof = 1;  /* queue failed: stop feeding */
        return 0;
    }

    g_mhi.queued++;

    if (g_mhi.file_pos >= g_mhi.file_end)
    {
        if (g_mhi.loop)
        {
            Seek(g_mhi.file, g_mhi.file_start, OFFSET_BEGINNING);
            g_mhi.file_pos = g_mhi.file_start;
        }
        else
            g_mhi.eof = 1;
    }

    return 1;
}

/***************************************************************************
 * MHI_FeederOpen() - open an MP3 file (passed to the driver complete,
 * tags included) and preload all stream buffers.  Returns 1 when ready
 * to MHIPlay().
 ***************************************************************************/
static int
MHI_FeederOpen(
    const char *path
)
{
    BPTR f;
    LONG start = 0, end, i;
    g_mhi.debug_open = 0;
    g_mhi.debug_seek_end = 0;
    g_mhi.debug_seek_back = 0;
    g_mhi.debug_read10 = 0;
    g_mhi.debug_start = 0;
    g_mhi.debug_end = 0;

    f = Open((CONST_STRPTR)path, MODE_OLDFILE);
    if (!f)
    {
        g_mhi.open_error = IoErr();
        return 0;
    }
    
    g_mhi.debug_open = 1;

    Seek(f, 0, OFFSET_END);
    end = Seek(f, 0, OFFSET_CURRENT);
    Seek(f, 0, OFFSET_BEGINNING);
    g_mhi.debug_seek_end = end;
    g_mhi.debug_seek_back = IoErr();

    /* Test mode: pass the complete MP3 file to the MHI driver.
     * Do not strip ID3v2 or ID3v1 tags. */
    start = 0;

    if (end <= start)
    {
        Close(f);
        return 0;
    }

    g_mhi.file = f;
    g_mhi.file_start = start;
    g_mhi.file_end = end;
    g_mhi.file_pos = start;
    g_mhi.eof = 0;

    g_mhi.debug_start = start;
    g_mhi.debug_end = end;

    Seek(f, start, OFFSET_BEGINNING);

    /* Preload: queue as many buffers as possible before starting. */
    for (i = 0; i < MHI_NUM_BUFS; i++)
    {
        if (!MHI_FillBuffer(g_mhi.buffers[i]))
            break;
    }

    return 1;
}

/***************************************************************************
 * MHI_Service() - driver signal handler: refill returned buffers, restart
 * on underrun, detect the end of a non-looping song.
 ***************************************************************************/
static void
MHI_Service(
    void
)
{
    APTR p;

    if (!g_mhi.decoder)
        return;

    /* Remember that the driver is alive/traffic for the end-of-song
     * watchdog in MHI_SongPlaying() (some drivers never signal the end). */
    g_mhi.traffic_ticks = SDL_GetTicks();

    /* The current song may have been stopped and the file closed before
     * the driver returned all buffers. Never refill after Close().
     */
    if (!g_mhi.file)
        return;

    /* Refill only while a song is actually playing: between STOP and the
     * next PLAY the driver may still hand back buffers of the previous
     * song, which must not be re-queued to the stopped decoder. */
    if (g_mhi.state != MHISTATE_PLAYING)
        return;

    /* Refill and requeue every buffer the driver has consumed. */
    while ((p = MHIGetEmpty(g_mhi.decoder)) != NULL)
    {
        if (g_mhi.queued > 0)
            g_mhi.queued--;

        MHI_FillBuffer((UBYTE *)p);
    }

    {
        UBYTE st = MHIGetStatus(g_mhi.decoder);

        if (st == MHIF_OUT_OF_DATA)
        {
            if (!g_mhi.eof)
            {
                /* Buffer underrun (e.g. slow disk during a level load):
                 * data is queued again, so restart the decoder. */
                MHIPlay(g_mhi.decoder);
            }
            else
            {
                /* Non-looping song played to the end. */
                MHI_FeederStop();
                g_mhi.state = MHISTATE_ENDED;
            }
        }
        else if (st == MHIF_STOPPED && g_mhi.eof)
        {
            /* Same end-of-stream case, reported as STOPPED. */
            MHI_FeederStop();
            g_mhi.state = MHISTATE_ENDED;
        }
    }
}

/******************************************************************************************
 * MHI_HandleCommand() - consume a pending command.  Returns 1 on QUIT.
 ******************************************************************************************/

static int
MHI_HandleCommand(
    void
)
{
    ULONG cmd = g_mhi.cmd;
    int quit = 0;

    switch (cmd)
    {
        case MHICMD_PLAY:
            MHI_FeederStop();

            g_mhi.open_error = 0;
            g_mhi.loop = g_mhi.cmd_loop ? 1 : 0;

            if (MHI_FeederOpen((const char *)g_mhi.cmd_path))
            {
                g_mhi.traffic_ticks = SDL_GetTicks();
                if (g_mhi.vol_supported)
                    MHISetParam(g_mhi.decoder, MHIP_VOLUME, MHI_ScaleVolume(g_mhi.volume));

                MHIPlay(g_mhi.decoder);

                g_mhi.state = MHISTATE_PLAYING;
            }
            else
            {
                g_mhi.open_error = 1;
                g_mhi.state = MHISTATE_IDLE;    /* silence */
            }
            break;

        case MHICMD_STOP:
            MHI_FeederStop();
            g_mhi.state = MHISTATE_IDLE;
            break;

        case MHICMD_VOLUME:
            g_mhi.volume = (LONG)g_mhi.cmd_volume;
            if (g_mhi.vol_supported && g_mhi.decoder)
                MHISetParam(g_mhi.decoder, MHIP_VOLUME, MHI_ScaleVolume(g_mhi.volume));
            break;

        case MHICMD_QUIT:
            MHI_FeederStop();
            g_mhi.state = MHISTATE_IDLE;
            quit = 1;
            break;

        default:
            break;
    }

    g_mhi.cmd = MHICMD_NONE;    /* consumed - releases the main task */
    return quit;
}
/***************************************************************************
 * MHI_TryDriver() - open one driver library and allocate its decoder.
 * Called by the feeder task only.  Returns 1 on success.
 ***************************************************************************/
static int
MHI_TryDriver(
    const char *path,
    ULONG mhi_mask
)
{
    struct Library *base;
    APTR h;

    base = OpenLibrary((CONST_STRPTR)path, 0);
    if (!base)
        return 0;

    MHIBase = base;

    h = MHIAllocDecoder(FindTask(NULL), mhi_mask);
    if (!h)
    {
        MHIBase = NULL;
        CloseLibrary(base);
        return 0;
    }

    g_mhi.base = base;
    g_mhi.decoder = h;

    strncpy(g_mhi.opened_path, path, sizeof(g_mhi.opened_path) - 1);
    g_mhi.opened_path[sizeof(g_mhi.opened_path) - 1] = 0;

    {
        /* MHIQuery(MHIQ_DECODER_NAME) returns a string pointer. */
        const char *nm = (const char *)MHIQuery(MHIQ_DECODER_NAME);
        if (nm)
        {
            strncpy(g_mhi.driver_name, nm, sizeof(g_mhi.driver_name) - 1);
            g_mhi.driver_name[sizeof(g_mhi.driver_name) - 1] = 0;
        }
    }

    g_mhi.vol_supported =
        (MHIQuery(MHIQ_VOLUME_CONTROL) == MHIF_SUPPORTED) ? 1 : 0;

    return 1;
}

/***************************************************************************
 * MHI_FeederOpenDriver() - choose and open the decoder driver: the
 * MHIDRIVER= override first (if given), then the known list, then a scan
 * of LIBS:MHI/ for any other installed driver.  Returns 1 on success.
 ***************************************************************************/
static int
MHI_FeederOpenDriver(
    ULONG mhi_mask
)
{
    int i;

    /* Explicit override: try as given; a bare name also looks in
     * LIBS:MHI/ first. */
    if (g_mhi.driver_override[0])
    {
        if (MHI_TryDriver(g_mhi.driver_override, mhi_mask))
            return 1;

        if (!strchr(g_mhi.driver_override, '/') &&
            !strchr(g_mhi.driver_override, ':'))
        {
            char path[300];
            sprintf(path, "LIBS:MHI/%s", g_mhi.driver_override);
            if (MHI_TryDriver(path, mhi_mask))
                return 1;
        }

        return 0;   /* user asked for a specific driver: no fallbacks */
    }

    for (i = 0; mhi_default_drivers[i]; i++)
    {
        if (MHI_TryDriver(mhi_default_drivers[i], mhi_mask))
            return 1;
    }

    /* Last resort: any other driver installed in LIBS:MHI/. */
    {
        struct AnchorPath *ap;

        ap = (struct AnchorPath *)AllocVec(sizeof(struct AnchorPath) + 512, MEMF_CLEAR);
        if (ap)
        {
            int found = 0;

            ap->ap_Strlen = 512;

            if (MatchFirst((CONST_STRPTR)"LIBS:MHI/#?.library", ap) == 0)
            {
                LONG rc = 0;

                while (rc == 0 && !found)
                {
                    char path[300];
                    const char *name = ap->ap_Info.fib_FileName;
                    int known = 0;

                    sprintf(path, "LIBS:MHI/%s", name);

                    /* Skip drivers already tried from the known list. */
                    for (i = 0; mhi_default_drivers[i]; i++)
                    {
                        if (MHI_EqualCI(path, mhi_default_drivers[i]))
                        {
                            known = 1;
                            break;
                        }
                    }

                    if (!known && MHI_TryDriver(path, mhi_mask))
                        found = 1;
                    else
                        rc = MatchNext(ap);
                }
            }

            MatchEnd(ap);
            FreeVec(ap);

            if (found)
                return 1;
        }
    }

    return 0;
}

/***************************************************************************
 * MHI_FeederEntry() - the feeder task: owns the driver library, the MP3
 * file and the buffer refill loop.
 *
 * Init handshake for the main task (g_mhi.ready):
 *   0 = pending, 1 = streaming ready,
 *  -1 = no driver signal, -2 = no command signal,
 *  -3 = no MHI driver found/opened, -4 = MHIAllocDecoder failed
 * (all known-list drivers failed, LIBS:MHI/ scan included).
 ***************************************************************************/
/* Compiled with -O0 like the AHI audio task: no FPU traps from
 * opportunistic register spills around library/device calls. */
__attribute__((optimize("O0")))
static void
MHI_FeederEntry(
    void
)
{
    BYTE sig, csig;
    ULONG mhi_mask, wait_mask;

    g_mhi.running = 1;
    g_mhi.task = FindTask(NULL);

    sig = AllocSignal(-1);          /* driver buffer signal */
    if (sig < 0)
    {
        g_mhi.ready = -1;
        g_mhi.running = 0;
        return;
    }

    csig = AllocSignal(-1);         /* command signal from the main task */
    if (csig < 0)
    {
        FreeSignal(sig);
        g_mhi.ready = -2;
        g_mhi.running = 0;
        return;
    }

    mhi_mask = 1UL << sig;
    g_mhi.cmd_sigmask = 1UL << csig;
    wait_mask = mhi_mask | g_mhi.cmd_sigmask;

    if (!MHI_FeederOpenDriver(mhi_mask))
    {
        FreeSignal(sig);
        FreeSignal(csig);
        g_mhi.ready = -3;
        g_mhi.running = 0;
        return;
    }

    g_mhi.ready = 1;

    for (;;)
    {
        ULONG sigs = Wait(wait_mask);

        if (sigs & g_mhi.cmd_sigmask)
        {
            if (MHI_HandleCommand())
                break;
        }

        if (sigs & mhi_mask)
            MHI_Service();
    }

    /* Teardown (the file is already closed by the QUIT handler). */
    if (g_mhi.decoder)
    {
        MHIFreeDecoder(g_mhi.decoder);
        g_mhi.decoder = NULL;
    }

    if (g_mhi.base)
    {
        CloseLibrary(g_mhi.base);
        g_mhi.base = NULL;
        MHIBase = NULL;
    }

    FreeSignal(sig);
    FreeSignal(csig);

    g_mhi.ready = 0;
    g_mhi.running = 0;
}

/***************************************************************************
 * Main task side - public interface (see mpumhi.h).
 ***************************************************************************/

/* Dup of the game directory lock, handed to the feeder process so its
 * relative file I/O ("MP3/...") resolves like the main task's. */
static BPTR g_mhi_cdlock = 0;

/***************************************************************************
 * MHI_SendCommand() - deliver a command to the feeder task and wait for
 * it to be consumed (so the shared parameter fields stay owned by the
 * sender until then).  Returns 1 on success.
 ***************************************************************************/
static int
MHI_SendCommand(
    ULONG cmd
)
{
    int spins;

    if (!g_mhi.running || !g_mhi.task || !g_mhi.cmd_sigmask)
        return 0;

    /* A previous command is normally consumed within milliseconds. */
    spins = 0;
    while (g_mhi.cmd != MHICMD_NONE && spins < 100)
    {
        Delay(1);
        spins++;
    }

    if (g_mhi.cmd != MHICMD_NONE)
        return 0;

    g_mhi.cmd = cmd;
    Signal(g_mhi.task, g_mhi.cmd_sigmask);

    /* Wait for consumption: the feeder may be prefilling buffers from
     * disk on PLAY, so allow generous time (song changes happen during
     * game transitions where a short wait is invisible). */
    spins = 0;
    while (g_mhi.cmd != MHICMD_NONE && spins < 300)     /* ~6 s max */
    {
        Delay(1);
        spins++;
    }

    if (g_mhi.cmd != MHICMD_NONE)
    {
        g_mhi.cmd = MHICMD_NONE;    /* feeder stuck: retract */
        return 0;
    }

    return 1;
}

/***************************************************************************
 * MHI_MusicInit() - allocate the stream buffers and start the feeder
 * task (which opens the MHI driver).  Returns 1 on success.
 ***************************************************************************/
int
MHI_MusicInit(
    void
)
{
    int i, spins;

    if (MHI_IsActive())
        return 1;

    /* Reset the state (driver_override survives - set before init). */
    g_mhi.proc = NULL;
    g_mhi.task = NULL;
    g_mhi.ready = 0;
    g_mhi.running = 0;
    g_mhi.state = MHISTATE_IDLE;
    g_mhi.cmd = MHICMD_NONE;
    g_mhi.cmd_sigmask = 0;
    g_mhi.decoder = NULL;
    g_mhi.file = 0;
    g_mhi.eof = 0;
    g_mhi.loop = 0;
    g_mhi.queued = 0;
    g_mhi.stop_incomplete = 0;
    g_mhi.vol_supported = 0;
    g_mhi.volume = 127;
    g_mhi.open_error = 0;
    g_mhi.traffic_ticks = 0;
    g_mhi.driver_name[0] = 0;
    g_mhi.debug_open = 0;
    g_mhi.debug_seek_end = 0;
    g_mhi.debug_seek_back = 0;
    g_mhi.debug_read10 = 0;
    g_mhi.debug_start = 0;
    g_mhi.debug_end = 0;
    g_mhi.opened_path[0] = 0;

    for (i = 0; i < MHI_NUM_BUFS; i++)
{
    g_mhi.buffers[i] = (UBYTE *)AllocMem(MHI_BUF_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
    if (!g_mhi.buffers[i])
    {
        while (--i >= 0)
        {
            FreeMem(g_mhi.buffers[i], MHI_BUF_SIZE);
            g_mhi.buffers[i] = NULL;
        }
        MHI_LOG("MHI: stream buffer allocation FAILED");
        return 0;
    }
}

    /* Hand the feeder a copy of the game directory lock, so its relative
     * file I/O ("MP3/...") resolves exactly like the main task's. */
    g_mhi_cdlock = DupLock(((struct Process *)FindTask(NULL))->pr_CurrentDir);

    if (g_mhi_cdlock)
    {
        g_mhi.proc = CreateNewProcTags(
            NP_Entry,      (ULONG)MHI_FeederEntry,
            NP_Name,       (ULONG)"Raptor MHI Task",
            NP_Priority,   (LONG)5, /* below the AHI audio task (10) */
            NP_StackSize,  (ULONG)16384,
            NP_CurrentDir, (ULONG)g_mhi_cdlock,
            TAG_DONE);
    }
    else
    {
        g_mhi.proc = CreateNewProcTags(
            NP_Entry,      (ULONG)MHI_FeederEntry,
            NP_Name,       (ULONG)"Raptor MHI Task",
            NP_Priority,   (LONG)5,
            NP_StackSize,  (ULONG)16384,
            TAG_DONE);
    }

    if (!g_mhi.proc)
    {
        MHI_LOG("MHI: CreateNewProcTags FAILED");
        goto init_fail;
    }

    /* Wait for the feeder to open the driver and allocate the decoder. */
    spins = 0;
    while (!g_mhi.ready && spins < 300)     /* ~6 s worst case */
    {
        Delay(1);
        spins++;
    }

    if (g_mhi.ready != 1)
    {
        MHI_LOG("MHI: init FAILED in feeder task (stage %ld: -1/-2 no signal, -3 no MHI driver; tried: %s)",
                (long)g_mhi.ready,
                g_mhi.driver_override[0] ? g_mhi.driver_override : "auto-detect list + LIBS:MHI/ scan");

        /* The feeder exits by itself on failure; wait for it to die. */
        spins = 0;
        while (g_mhi.running && spins < 100)
        {
            Delay(1);
            spins++;
        }
        goto init_fail;
    }

    MHI_LOG("MHI: decoder driver '%s' (%s), volume control: %s",
            g_mhi.driver_name[0] ? g_mhi.driver_name : "unknown",
            g_mhi.opened_path,
            g_mhi.vol_supported ? "yes" : "no");

    return 1;

init_fail:
    g_mhi.proc = NULL;
    g_mhi.task = NULL;

    for (i = 0; i < MHI_NUM_BUFS; i++)
    {
        if (g_mhi.buffers[i])
        {
            FreeMem(g_mhi.buffers[i], MHI_BUF_SIZE);
            g_mhi.buffers[i] = NULL;
        }
    }

    if (g_mhi_cdlock)
    {
        UnLock(g_mhi_cdlock);
        g_mhi_cdlock = 0;
    }

    return 0;
}

/***************************************************************************
 * MHI_MusicDeInit() - stop the feeder task and free the buffers.
 ***************************************************************************/
void
MHI_MusicDeInit(
    void
)
{
    int i;

    if (!g_mhi.proc && !g_mhi.running)
        return;

    if (g_mhi.running)
    {
        int spins;

        MHI_SendCommand(MHICMD_QUIT);

        spins = 0;
        while (g_mhi.running && spins < 200)    /* ~4 s max */
        {
            Delay(1);
            spins++;
        }

        if (g_mhi.running)
            MHI_LOG("MHI: WARNING - feeder task did not stop in time!");
    }

    g_mhi.proc = NULL;
    g_mhi.task = NULL;
    g_mhi.ready = 0;
    g_mhi.state = MHISTATE_IDLE;

    for (i = 0; i < MHI_NUM_BUFS; i++)
    {
        if (g_mhi.buffers[i])
        {
            FreeMem(g_mhi.buffers[i], MHI_BUF_SIZE);
            g_mhi.buffers[i] = NULL;
        }
    }

    if (g_mhi_cdlock)
    {
        UnLock(g_mhi_cdlock);
        g_mhi_cdlock = 0;
    }
}

/***************************************************************************
 * MHI_IsActive() - 1 when the backend is up (init succeeded).
 ***************************************************************************/
int
MHI_IsActive(
    void
)
{
    return g_mhi.running && g_mhi.ready == 1;
}

/***************************************************************************
 * MHI_DriverName() - opened decoder driver, for logging.
 ***************************************************************************/
const char *
MHI_DriverName(
    void
)
{
    if (g_mhi.driver_name[0])
        return g_mhi.driver_name;

    if (g_mhi.opened_path[0])
        return g_mhi.opened_path;

    return "unknown";
}

/***************************************************************************
 * MHI_PlaySongItem() - map a GLB music item to an MP3 file and play it.
 ***************************************************************************/
void
MHI_PlaySongItem(
    int item,
    int loop
)
{
    char path[300];

    if (!MHI_IsActive())
        return;

    if (!MHI_FindFileForItem(item, path, sizeof(path)))
    {
        MHI_LOG("MHI: no MP3 found for song item 0x%04x - silence", item);
        MHI_StopSong();
        return;
    }

    strncpy(g_mhi.cmd_path, path, sizeof(g_mhi.cmd_path) - 1);
    g_mhi.cmd_path[sizeof(g_mhi.cmd_path) - 1] = 0;
    g_mhi.cmd_loop = loop ? 1 : 0;

    if (!MHI_SendCommand(MHICMD_PLAY))
    {
        MHI_LOG("MHI: PLAY command failed");
    }
    else
    {
        if (g_mhi.stop_incomplete)
        {
            g_mhi.stop_incomplete = 0;
        }
    }
}

/***************************************************************************
 * MHI_StopSong() - stop the current song immediately (MHI has no fade).
 ***************************************************************************/
void
MHI_StopSong(
    void
)
{
    if (!MHI_IsActive())
        return;

    MHI_SendCommand(MHICMD_STOP);
}

/***************************************************************************
 * MHI_SongPlaying() - 1 while a song is active, 0 when idle/ended.
 ***************************************************************************/
int
MHI_SongPlaying(
    void
)
{
    if (!MHI_IsActive())
        return 0;

    /* MHI end-of-song fallback: report the song as ended when the feeder
     * has finished it even though the driver never said so.  Some MHI
     * drivers (e.g. Prisma MegaMix) never hand the last buffers back and
     * never send an end-of-stream signal, so g_mhi.state would stay
     * MHISTATE_PLAYING forever and the intro would wait for user input.
     * "Editorial end":
     *   - queued == 0:  all stream buffers have been played back;
     *   - idle > 4 s:   the driver has gone completely silent (no buffer
     *                   traffic, no signals) on a non-looping song.
     * traffic_ticks is refreshed by MHI_Service() on every driver signal
     * and at PLAY, so loops never trip this. */
    if (g_mhi.state == MHISTATE_PLAYING)
    {
        if (g_mhi.queued == 0)
            return 0;

        if ((ULONG)(SDL_GetTicks() - g_mhi.traffic_ticks) > 4000)
            return 0;
    }

    return g_mhi.state == MHISTATE_PLAYING;
}

/***************************************************************************
 * MHI_SetVolume() - Raptor music volume 0..127, stored as-is; the actual
 * scaling to the MHI driver's 0..100 range happens in MHI_ScaleVolume()
 * at the MHISetParam() call sites in MHI_HandleCommand().
 ***************************************************************************/
void
MHI_SetVolume(
    int volume
)
{
    if (volume < 0)
        volume = 0;
    if (volume > 127)
        volume = 127;

    g_mhi.volume = volume;

    if (!MHI_IsActive())
        return;

    g_mhi.cmd_volume = (ULONG)g_mhi.volume;
    MHI_SendCommand(MHICMD_VOLUME);
}

/***************************************************************************
 * MHI_SetDriverOverride() - MHIDRIVER= parameter: driver library name or
 * full path.  Must be called before MHI_MusicInit().
 ***************************************************************************/
void
MHI_SetDriverOverride(
    const char *name
)
{
    if (!name)
        return;

    strncpy(g_mhi.driver_override, name, sizeof(g_mhi.driver_override) - 1);
    g_mhi.driver_override[sizeof(g_mhi.driver_override) - 1] = 0;
}

#endif /* __AMIGA__ */
