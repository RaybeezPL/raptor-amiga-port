/***************************************************************************
 * mpumhi_warp.cpp - dedicated resident-memory ArmedWARP MHI backend.
 ***************************************************************************/
#ifdef __AMIGA__

#include <stdio.h>
#include <string.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include "common.h"
#include "mpumhi_warp.h"
#include <libraries/mhi.h>
#include <proto/mhi.h>

#ifdef USE_SDL_STUBS
#define MHI_LOG(...) AmigaLog(__VA_ARGS__)
#else
#define MHI_LOG(...) do { printf(__VA_ARGS__); printf("\n"); fflush(stdout); } while (0)
#endif

#define MHI_WARP_BUF_SIZE 0x8000UL

static ULONG MHI_WarpScaleVolume(int volume)
{
    if (volume < 0) volume = 0;
    if (volume > 127) volume = 127;
    return (ULONG)((volume * 100 + 63) / 127);
}

/*
 * ArmedWARP per-track state. The playback task receives this object
 * through NP_ExitData and operates on it exclusively; the parent API
 * reads and writes the same authoritative fields.
 */
struct MHIWarpTrack
{
    UBYTE *allocation;
    ULONG allocation_size;
    UBYTE *mpeg;
    ULONG mpeg_length;
    UBYTE *current;
    ULONG remaining;
    UBYTE *loop_start;
    ULONG loop_length;
    struct Task *parent_task;
    struct Process *playback_process;
    struct Task *playback_task;
    struct Library *library_base;
    APTR decoder;
    volatile LONG volume;
    volatile LONG loop;
    volatile LONG playing;
    volatile LONG running;
    volatile LONG ended;
};

/* ArmedWARP backend/global owner state. */
struct MHIWarpState
{
    LONG active;
    /* Backend-wide volume; exists even without an active track. */
    volatile LONG volume;
    char library_path[256];
    char driver_name[64];
    /* Current ArmedWARP per-track state handed to the playback task. */
    struct MHIWarpTrack track;
};

static struct MHIWarpState g_mhi_warp;

static ULONG MHI_WarpReadLE32(const UBYTE *p)
{
    return (ULONG)p[0] | ((ULONG)p[1] << 8) |
           ((ULONG)p[2] << 16) | ((ULONG)p[3] << 24);
}

static int MHI_WarpMPEGSync(const UBYTE *p, ULONG available)
{
    if (available < 4 || p[0] != 0xff || (p[1] & 0xe0) != 0xe0)
        return 0;
    if ((p[1] & 0x18) == 0x08 || (p[1] & 0x06) == 0x00)
        return 0;
    if ((p[2] & 0xf0) == 0x00 || (p[2] & 0xf0) == 0xf0 ||
        (p[2] & 0x0c) == 0x0c)
        return 0;
    return 1;
}

/* Parse only the closed file's resident allocation. */
static int MHI_WarpFindMPEG(UBYTE *allocation, ULONG allocation_size,
                            UBYTE **out_mpeg, ULONG *out_length)
{
    UBYTE *start = allocation;
    UBYTE *end = allocation + allocation_size;
    UBYTE *p;

    if (allocation_size >= 12 && !memcmp(start, "RIFF", 4) &&
        !memcmp(start + 8, "WAVE", 4))
    {
        UBYTE *chunk = start + 12;
        int found = 0;
        while ((ULONG)(end - chunk) >= 8)
        {
            ULONG size = MHI_WarpReadLE32(chunk + 4);
            UBYTE *data = chunk + 8;
            ULONG available = (ULONG)(end - data);
            if (size > available)
                return 0;
            if (!memcmp(chunk, "data", 4))
            {
                start = data;
                end = data + size;
                found = 1;
                break;
            }
            if ((size & 1) && size == available)
                return 0;
            chunk = data + size + (size & 1);
        }
        if (!found)
            return 0;
    }

    if ((ULONG)(end - start) >= 10 && !memcmp(start, "ID3", 3) &&
        start[3] >= 2 && start[3] <= 4 &&
        !(start[6] & 0x80) && !(start[7] & 0x80) &&
        !(start[8] & 0x80) && !(start[9] & 0x80))
    {
        ULONG body = ((ULONG)start[6] << 21) | ((ULONG)start[7] << 14) |
                     ((ULONG)start[8] << 7) | (ULONG)start[9];
        ULONG overhead = (start[3] == 4 && (start[5] & 0x10)) ? 20 : 10;
        ULONG available = (ULONG)(end - start);
        if (overhead > available || body > available - overhead)
            return 0;
        start += overhead + body;
    }

    if ((ULONG)(end - start) >= 128 && !memcmp(end - 128, "TAG", 3))
    {
        end -= 128;
        if ((ULONG)(end - start) >= 227 && !memcmp(end - 227, "TAG+", 4))
            end -= 227;
    }
    else if ((ULONG)(end - start) >= 227 && !memcmp(end - 227, "TAG+", 4))
        end -= 227;

    for (p = start; (ULONG)(end - p) >= 4; p++)
    {
        if (MHI_WarpMPEGSync(p, (ULONG)(end - p)))
        {
            *out_mpeg = p;
            *out_length = (ULONG)(end - p);
            return 1;
        }
    }
    return 0;
}

/* Queue one resident-memory block for this ArmedWARP track. */
static int MHI_WarpQueueNext(APTR decoder, struct MHIWarpTrack *track)
{
    ULONG size;
    if (!track->remaining)
    {
        if (!track->loop)
            return 0;
        track->current = track->loop_start;
        track->remaining = track->loop_length;
    }
    size = track->remaining;
    if (size > MHI_WARP_BUF_SIZE)
        size = MHI_WARP_BUF_SIZE;
    if (!size || !MHIQueueBuffer(decoder, track->current, size))
        return 0;
    track->current += size;
    track->remaining -= size;
    return 1;
}

/* Queue the two initial resident-memory blocks for this ArmedWARP track. */
static int MHI_WarpQueueInitial(APTR decoder, struct MHIWarpTrack *track)
{
    ULONG first, second;

    if (track->remaining <= MHI_WARP_BUF_SIZE)
        return 0;

    first = track->remaining;
    if (first > MHI_WARP_BUF_SIZE)
        first = MHI_WARP_BUF_SIZE;

    if (!MHIQueueBuffer(decoder, track->current, first))
        return 0;
    track->current += first;
    track->remaining -= first;

    second = track->remaining;
    if (second > MHI_WARP_BUF_SIZE)
        second = MHI_WARP_BUF_SIZE;
    if (!second || !MHIQueueBuffer(decoder, track->current, second))
        return 0;
    track->current += second;
    track->remaining -= second;
    return 1;
}

/*
 * ArmedWARP playback task. All per-track state arrives through NP_ExitData;
 * this task never touches the backend-global state.
 */
__attribute__((optimize("O0")))
static void MHI_WarpPlaybackEntry(void)
{
    struct Process *self = (struct Process *)FindTask(NULL);
    struct MHIWarpTrack *track = (struct MHIWarpTrack *)self->pr_ExitData;
    struct Task *playback_task;
    APTR decoder;
    int done = 0;

    self->pr_ExitData = NULL;
    playback_task = &self->pr_Task;
    track->playback_task = playback_task;
    MHIBase = track->library_base;
    decoder = MHIAllocDecoder(playback_task, SIGBREAKF_CTRL_F);
    track->decoder = decoder;

    if (!decoder || !MHI_WarpQueueInitial(decoder, track))
        done = 1;
    else
    {
        MHISetParam(decoder, MHIP_VOLUME,
                    MHI_WarpScaleVolume((int)track->volume));
        MHIPlay(decoder);
        track->playing = 1;
    }

    while (!done)
    {
        ULONG signals = Wait(SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D |
                             SIGBREAKF_CTRL_F);
        if (signals & SIGBREAKF_CTRL_C)
            done = 1;
        if (!done && (signals & SIGBREAKF_CTRL_D))
            MHISetParam(decoder, MHIP_VOLUME,
                        MHI_WarpScaleVolume((int)track->volume));
        if (!done && (signals & SIGBREAKF_CTRL_F))
        {
            while (MHIGetEmpty(decoder))
                ;

            if (!track->remaining)
            {
                if (track->loop)
                {
                    track->current = track->loop_start;
                    track->remaining = track->loop_length;
                }
                else
                {
                    track->playing = 0;
                    track->ended = 1;
                    done = 1;
                }
            }

            if (!done)
            {
                if (!MHI_WarpQueueNext(decoder, track))
                    done = 1;
                else if (MHIGetStatus(decoder) == MHIF_OUT_OF_DATA)
                    MHIPlay(decoder);
            }
        }
    }

    track->playing = 0;
    if (decoder)
    {
        MHIStop(decoder);
        MHIFreeDecoder(decoder);
    }
    track->decoder = NULL;
    track->running = 0;
    Forbid();
    Signal(track->parent_task, SIGBREAKF_CTRL_E);
    return;
}

/* Parent-side natural-end reap; decoder teardown happened in the process. */
static void MHI_WarpReap(void)
{
    struct MHIWarpTrack *track = &g_mhi_warp.track;
    if (track->running)
        return;
    MHIBase = NULL;
    if (track->library_base)
    {
        CloseLibrary(track->library_base);
        track->library_base = NULL;
    }
    if (track->allocation)
    {
        FreeMem(track->allocation, track->allocation_size);
        track->allocation = NULL;
    }
    track->allocation_size = 0;
    track->mpeg = track->current = track->loop_start = NULL;
    track->mpeg_length = track->remaining = track->loop_length = 0;
    track->playback_process = NULL;
    track->playback_task = NULL;
    track->decoder = NULL;
    track->playing = 0;
}

static void MHI_WarpDestroyTrack(void)
{
    struct MHIWarpTrack *track = &g_mhi_warp.track;
    if (!track->allocation && !track->library_base)
        return;
    if (track->playback_process)
    {
        if (track->running && track->playback_task)
            Signal(track->playback_task, SIGBREAKF_CTRL_C);
        Wait(SIGBREAKF_CTRL_E);
    }
    MHI_WarpReap();
}

int MHI_WarpMusicInit(const char *library_path, const char *driver_name,
                      int volume)
{
    memset(&g_mhi_warp, 0, sizeof(g_mhi_warp));
    g_mhi_warp.track.parent_task = FindTask(NULL);
    g_mhi_warp.volume = volume;
    g_mhi_warp.track.volume = volume;
    g_mhi_warp.active = 1;
    strncpy(g_mhi_warp.library_path, library_path,
            sizeof(g_mhi_warp.library_path) - 1);
    if (driver_name)
        strncpy(g_mhi_warp.driver_name, driver_name,
                sizeof(g_mhi_warp.driver_name) - 1);
    return 1;
}

void MHI_WarpMusicDeInit(void)
{
    MHI_WarpDestroyTrack();
    g_mhi_warp.active = 0;
}

int MHI_WarpIsActive(void) { return g_mhi_warp.active; }

const char *MHI_WarpDriverName(void)
{
    if (g_mhi_warp.driver_name[0])
        return g_mhi_warp.driver_name;
    return g_mhi_warp.library_path[0] ? g_mhi_warp.library_path : "unknown";
}

void MHI_WarpPlayPath(const char *path, int loop)
{
    struct MHIWarpTrack *track = &g_mhi_warp.track;
    BPTR file;
    LONG size, got;
    APTR probe;

    if (!g_mhi_warp.active || !path)
        return;
    MHI_WarpDestroyTrack();
    SetSignal(0, SIGBREAKF_CTRL_E);
    track->ended = 0;

    file = Open((CONST_STRPTR)path, MODE_OLDFILE);
    if (!file)
        return;
    if (Seek(file, 0, OFFSET_END) == -1 ||
        (size = Seek(file, 0, OFFSET_CURRENT)) <= 0 ||
        Seek(file, 0, OFFSET_BEGINNING) == -1)
    {
        Close(file);
        return;
    }
    track->allocation = (UBYTE *)AllocMem((ULONG)size, MEMF_PUBLIC);
    if (!track->allocation)
    {
        Close(file);
        return;
    }
    track->allocation_size = (ULONG)size;
    got = Read(file, track->allocation, size);
    Close(file);
    if (got != size ||
        !MHI_WarpFindMPEG(track->allocation, track->allocation_size,
                          &track->mpeg, &track->mpeg_length))
    {
        MHI_WarpReap();
        return;
    }

    track->library_base = OpenLibrary(
        (CONST_STRPTR)g_mhi_warp.library_path, 0);
    if (!track->library_base)
    {
        MHI_WarpReap();
        return;
    }
    MHIBase = track->library_base;
    probe = MHIAllocDecoder(track->parent_task, SIGBREAKF_CTRL_F);
    if (!probe)
    {
        MHI_WarpReap();
        return;
    }
    MHIFreeDecoder(probe);

    track->current = track->loop_start = track->mpeg;
    track->remaining = track->loop_length = track->mpeg_length;
    track->loop = loop ? 1 : 0;
    track->volume = g_mhi_warp.volume;
    track->playing = 0;
    track->running = 1;
    track->playback_process = CreateNewProcTags(
        NP_Entry,     (ULONG)MHI_WarpPlaybackEntry,
        NP_Name,      (ULONG)"Raptor ArmedWARP Playback Task",
        NP_Priority,  (LONG)5,
        NP_StackSize, (ULONG)16384,
        NP_ExitData,  (ULONG)track,
        TAG_DONE);
    if (!track->playback_process)
    {
        track->running = 0;
        MHI_WarpReap();
        return;
    }
    track->playback_task = &track->playback_process->pr_Task;
}

void MHI_WarpStopSong(void)
{
    MHI_WarpDestroyTrack();
    g_mhi_warp.track.ended = 0;
}

int MHI_WarpSongPlaying(void)
{
    struct MHIWarpTrack *track = &g_mhi_warp.track;
    if (!g_mhi_warp.active)
        return 0;
    if (!track->running && track->playback_process)
    {
        Wait(SIGBREAKF_CTRL_E);
        MHI_WarpReap();
    }
    return track->playing ? 1 : 0;
}

void MHI_WarpSetVolume(int volume)
{
    struct MHIWarpTrack *track = &g_mhi_warp.track;
    if (volume < 0) volume = 0;
    if (volume > 127) volume = 127;
    g_mhi_warp.volume = volume;
    if (track->running && track->playback_task)
    {
        track->volume = volume;
        Signal(track->playback_task, SIGBREAKF_CTRL_D);
    }
}

#endif /* __AMIGA__ */
