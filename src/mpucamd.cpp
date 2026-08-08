/***************************************************************************
 * mpucamd.cpp - CAMD (camd.library) music backend, Amiga port only.
 *
 * Plays the game's MUS music as a live General-MIDI event stream through
 * the standard Amiga MIDI API (camd.library) - the same role the upstream
 * mpuwinmm.cpp / mpualsa.cpp / mpucorem.cpp backends fill with WinMM /
 * ALSA / CoreMIDI on their platforms.
 *
 * Timing: this device has no Mix() callback, so the MUS sequencer is
 * driven by MUS_Poll() from the main loop (i_video.cpp), exactly like the
 * other system-MIDI backends.  Sound effects are unaffected - they keep
 * playing through the AHI audio backend.
 *
 * This backend is opt-in: the game plays music through the built-in
 * AdLib/OPL3 emulation by default; MUSIC=CAMD (CLI parameter or icon
 * ToolType) selects this General MIDI path instead.  MIDI data is sent
 * to the fixed CAMD output cluster "out.0".  A MIDI driver (e.g. the
 * serial driver from the camd40 package) or a software synthesizer with
 * a CAMD interface must provide the cluster - otherwise the events are
 * routed nowhere (standard MIDI behaviour, same as an unplugged MIDI
 * cable behind a DOS MPU-401) and the music is silent.
 ***************************************************************************/
#ifdef __AMIGA__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <proto/exec.h>
#include <proto/camd.h>

#include <midi/camd.h>
#include <midi/mididefs.h>

#include "SDL.h"
#include "common.h"
#include "musapi.h"

/* camd.library base (extern in proto/camd.h). */
struct Library *CamdBase = NULL;

/* Log through AmigaLog on the SDL-stub build, plain console otherwise.
 * Output goes to stdout only; run "raptor > RAPTOR.LOG" to capture it. */
#ifdef USE_SDL_STUBS
#define CAMD_LOG(...) AmigaLog(__VA_ARGS__)
#else
#define CAMD_LOG(...) do { printf(__VA_ARGS__); printf("\n"); fflush(stdout); } while (0)
#endif

static struct MidiNode *camd_node;
static struct MidiLink *camd_link;
static char camd_cluster[64];

/***************************************************************************
 CAMD_Put() - Send one packed channel MIDI message.
 CAMD expects the status byte in bits 24-31, data1 in bits 16-23 and data2
 in bits 8-15 (big-endian MidiMsg union layout - verified against the
 camd.library sources).  Data bytes are 7-bit masked as per MIDI spec.
 ***************************************************************************/
static void
CAMD_Put(
    unsigned int status,
    unsigned int data1,
    unsigned int data2
)
{
    if (!camd_link)
        return;

    /* Never write to an unconnected link (cluster gone). */
    if (!MidiLinkConnected(camd_link))
        return;

    PutMidi(camd_link, (LONG)((status << 24) | ((data1 & 0x7f) << 16) | ((data2 & 0x7f) << 8)));
}

/***************************************************************************
 MPU_MapChannel() - Map MUS channels to General MIDI channels.
 MUS uses channels 0-14 for melody and 15 for percussion; in GM the
 percussion channel is 9.  Same mapping as the other mpu* backends.
 ***************************************************************************/
static unsigned int
MPU_MapChannel(
    unsigned int chan
)
{
    if (chan < 9)
        return chan;

    if (chan == 15)
        return 9;

    return chan + 1;
}

/***************************************************************************
 KeyOffEvent() -
 ***************************************************************************/
static void
KeyOffEvent(
    unsigned int chan,
    unsigned int key
)
{
    CAMD_Put(MS_NoteOff | MPU_MapChannel(chan), key, 0);
}

/***************************************************************************
 KeyOnEvent() -
 ***************************************************************************/
static void
KeyOnEvent(
    int chan,
    unsigned int key,
    unsigned int volume
)
{
    /* One-shot diagnostic: proves the MUS sequencer is feeding CAMD.
     * Logged BEFORE PutMidi, so it is recorded even if a full MIDI
     * driver buffer would block the PutMidi call itself. */
    static int first_note = 0;
    if (!first_note)
    {
        first_note = 1;
        CAMD_LOG("CAMD: first KeyOn event (ch=%u key=%u vel=%u).",
                 (unsigned)MPU_MapChannel(chan), key, volume);
    }

    CAMD_Put(MS_NoteOn | MPU_MapChannel(chan), key, volume);
}


/***************************************************************************
 ProgramEvent() -
 ***************************************************************************/
static void
ProgramEvent(
    unsigned int chan,
    unsigned int param
)
{
    CAMD_Put(MS_Prog | MPU_MapChannel(chan), param, 0);
}

/***************************************************************************
 PitchBendEvent() -
 ***************************************************************************/
static void
PitchBendEvent(
    unsigned int chan,
    int bend
)
{
    /* MUS bend byte -> 14-bit MIDI pitch bend value (same shift the WinMM
     * backend uses), transmitted as LSB/MSB data bytes. */
    bend <<= 6;

    CAMD_Put(MS_PitchBend | MPU_MapChannel(chan), bend & 127, (bend >> 7) & 127);
}

/***************************************************************************
 ControllerEvent() -
 ***************************************************************************/
static void
ControllerEvent(
    unsigned int chan,
    unsigned int controller,
    unsigned int param
)
{
    /* MUS controller numbers -> General MIDI CC numbers (same table as the
     * other mpu* backends); entries marked -1 have no GM equivalent and
     * are dropped. */
    static const int event_map[16] = {
        0, 0, 1, 7, 10, 11, 91, 93, 64, 67, 120, 123, -1, -1, 121, -1
    };

    if (controller > 15)
        return;

    if (event_map[controller] < 0)
        return;

    CAMD_Put(MS_Ctrl | MPU_MapChannel(chan), (unsigned int)event_map[controller], param);
}

/***************************************************************************
 AllNotesOffEvent() -
 ***************************************************************************/
static void
AllNotesOffEvent(
    unsigned int chan,
    unsigned int param
)
{
    (void)param;

    CAMD_Put(MS_Ctrl | MPU_MapChannel(chan), MM_AllOff, 0);
}

/***************************************************************************
 CAMD_Init() - Open camd.library, create the MIDI node and link it to the
 * configured output cluster.  Returns 1 on success, 0 on failure (the
 * caller then falls back to the OPL3 music backend).
 ***************************************************************************/
static int
CAMD_Init(
    int option
)
{
    struct TagItem node_tags[3];
    struct TagItem link_tags[2];

    (void)option;

    camd_node = NULL;
    camd_link = NULL;

    CAMD_LOG("CAMD: opening camd.library...");
    CamdBase = OpenLibrary((CONST_STRPTR)"camd.library", 0);
    if (!CamdBase)
    {
        CAMD_LOG("CAMD: camd.library not found - MIDI music unavailable.");
        return 0;
    }

    CAMD_LOG("CAMD: camd.library v%d.%d opened.",
             (int)CamdBase->lib_Version, (int)CamdBase->lib_Revision);

    /* No SETUP.INI in this Amiga port: the output cluster is fixed to
     * "out.0", the standard CAMD output cluster name. */
    strcpy(camd_cluster, "out.0");

    node_tags[0].ti_Tag  = MIDI_Name;       node_tags[0].ti_Data = (ULONG)"Raptor";
    node_tags[1].ti_Tag  = MIDI_ClientType; node_tags[1].ti_Data = CCType_EventGenerator | CCType_Sequencer;
    node_tags[2].ti_Tag  = TAG_END;         node_tags[2].ti_Data = 0;

    camd_node = CreateMidiA(node_tags);
    if (!camd_node)
    {
        CAMD_LOG("CAMD: CreateMidiA() failed - MIDI music unavailable.");
        CloseLibrary(CamdBase);
        CamdBase = NULL;
        return 0;
    }

    link_tags[0].ti_Tag  = MLINK_Location;  link_tags[0].ti_Data = (ULONG)camd_cluster;
    link_tags[1].ti_Tag  = TAG_END;         link_tags[1].ti_Data = 0;

    camd_link = AddMidiLinkA(camd_node, MLTYPE_Sender, link_tags);
    if (!camd_link)
    {
        CAMD_LOG("CAMD: AddMidiLinkA('%s') failed - MIDI music unavailable.", camd_cluster);
        DeleteMidi(camd_node);
        camd_node = NULL;
        CloseLibrary(CamdBase);
        CamdBase = NULL;
        return 0;
    }

    /* NOTE: CAMD creates a named cluster on demand, so the link succeeds
     * even when no MIDI driver is attached yet; events sent before a
     * driver appears are simply routed nowhere. */
    CAMD_LOG("CAMD: MIDI music out on cluster '%s' (link %s).", camd_cluster,
             MidiLinkConnected(camd_link) ? "connected" : "pending");

    /* The CaffeineOS trap: camd.library can be installed but unconfigured,
     * so the link above succeeds while nothing is listening on the
     * cluster - the music would be silently routed nowhere.  Say so
     * loudly on the console. */
    if (!MidiLinkConnected(camd_link))
    {
        CAMD_LOG("CAMD: WARNING - no MIDI driver/synth attached to cluster '%s':", camd_cluster);
        CAMD_LOG("CAMD: MIDI music will be SILENT (sound FX still play).");
        CAMD_LOG("CAMD: Configure a CAMD driver, or run without MUSIC=CAMD");
        CAMD_LOG("CAMD: to use the default AdLib/OPL3 music instead.");
    }

    return 1;
}

/***************************************************************************
 CAMD_DeInit() - Remove the link, delete the node, close the library.
 * MUS_DeInit() sends all-notes-off on every channel before calling this.
 ***************************************************************************/
static void
CAMD_DeInit(
    void
)
{
    if (CamdBase || camd_node)
        CAMD_LOG("CAMD: closing MIDI node and camd.library.");

    if (camd_node)
    {
        if (camd_link)
        {
            RemoveMidiLink(camd_link);
            camd_link = NULL;
        }

        DeleteMidi(camd_node);
        camd_node = NULL;
    }

    if (CamdBase)
    {
        CloseLibrary(CamdBase);
        CamdBase = NULL;
    }
}

musdevice_t mus_device_camd = {
    CAMD_Init,
    CAMD_DeInit,
    NULL,

    KeyOffEvent,
    KeyOnEvent,
    ControllerEvent,
    PitchBendEvent,
    ProgramEvent,
    AllNotesOffEvent,
};
#endif /* __AMIGA__ */
