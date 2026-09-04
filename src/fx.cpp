#include <stdio.h>
#include <string.h>
#include "SDL.h"
#include "common.h"
#include "glbapi.h"
#include "i_video.h"
#include "i_oplmusic.h"
#include "musapi.h"
#include "cards.h"
#include "fx.h"
#include "dspapi.h"
#include "rap.h"
#include "gssapi.h"
#include "fileids.h"
#include "mpumhi.h"
#include "amiga/amiga_cfg.h"
#include "entypes.h"

int music_volume;
int dig_flag;
int fx_device;
int fx_volume;
static int fx_init = 0;
static int lockcount;
#ifdef __AMIGA__
/* Amiga: run the audio pipeline at 11025 Hz - the native rate of Raptor's
 * sound effects.  The DSP mixer then ticks 1:1 with the output (no
 * resampling work) and the AHI stream is 4x lighter than 44100 Hz on the
 * 68060.  Music timing is rate-independent (MUS counts services against
 * fx_freq) and the CAMD MIDI path is pure events. */
int fx_freq = 11025;
#else
int fx_freq = 44100;
#endif

int music_song = -1;

int fx_gus;
int fx_channels;
int sys_midi, winmm_mpu_device, core_dls_synth, core_midi_port, alsaclient, alsaport;

/* -nosound / -nomusic command-line flags and the MUSIC= backend selector -
 * parsed in main() (src/rap.cpp). See fx.h for the full description of each
 * flag's semantics. Defaulting to 0 (audio fully enabled) preserves the
 * exact previous behaviour when neither switch is passed on the command
 * line. On Amiga g_music_mode defaults to MUSIC_MODE_OFF: no music backend
 * is initialized unless the user explicitly selects one with MUSIC=ADLIB,
 * MUSIC=CAMD, MUSIC=MHI or MUSIC=WAVE. A failed MHI/CAMD initialization
 * switches the selected backend to MUSIC_MODE_OFF - never to AdLib. */
int g_nosound = 0;
int g_nomusic = 0;
#ifdef __AMIGA__
int g_music_mode = MUSIC_MODE_OFF;   /* Amiga: brak backendu domyślnie */
#else
int g_music_mode = MUSIC_MODE_ADLIB; /* upstream: AdLib/OPL3 */
#endif

typedef struct
{
    int item;         // GLB ITEM
    int pri;          // PRIORITY 0=LOW
    int pitch;        // PITCH TO PLAY PATCH
    int rpflag;       // TRUE = RANDOM PITCHES
    int sid;          // DMX EFFECT ID
    int vol;          // VOLUME
    int gcache;       // CACHE FOR IN GAME USE ?
    int odig;         // TRUE = ONLY PLAY DIGITAL
} DFX;

DFX fx_items[FX_LAST_SND];
int fx_loaded;

SDL_AudioDeviceID fx_dev;

char cards[M_LAST][23] = {
    "None",
    "PC Speaker",
    "Adlib",
    "Gravis Ultra Sound",
    "Pro Audio Spectrum",
    "Sound Blaster",
    "WaveBlaster",
    "Roland Sound Canvas",
    "General Midi",
    "Sound Blaster AWE 32",
};

/***************************************************************************
FX_Fill() - Audio callback invoked by the AHI backend (background task).
Must stay as cheap as possible: no STDIO, no float math, no unnecessary
loops. Any change here should be re-checked against the 68060 performance
requirements described in the porting notes.
 ***************************************************************************/
void FX_Fill(void *userdata, uint8_t *stream, int len)
{
    (void)userdata;

    if (g_nosound || !stream || len <= 0) {
        if (stream && len > 0) memset(stream, 0, len);
        return;
    }

    /* Clear the output buffer (stereo S16 silence = 0). */
    memset(stream, 0, len);

    /* 1 stereo S16 frame (2 ch * 2 B) = 4 bytes; frame count = len / 4. */
    int frames = len / 4;
    if (frames <= 0) return;

    if (music_volume > 0 && !g_nomusic) {
        if (g_music_mode == MUSIC_MODE_WAVE)
            WAVE_Mix((int16_t*)stream, frames);
        else
            MUS_Mix((int16_t*)stream, frames);
    }

    GSS_Mix((int16_t*)stream, frames);

    DSP_Mix((int16_t*)stream, frames);

    /* --- AMIGA OUTPUT CLAMP ---
     * No gain is applied here: the mixed software stream (SFX + AdLib)
     * is only hard-clamped to the int16 range.  The mixers above can
     * overshoot when several sources overlap; clamping through a 32-bit
     * variable prevents wrap-around clicks on overflow. */

    int16_t *buf = (int16_t *)stream;
    int total_samples = frames * 2; // 2 channels (Stereo)
    
    for (int i = 0; i < total_samples; i++) {
        int32_t val = buf[i];
        
        if (val > 32767) val = 32767;
        else if (val < -32768) val = -32768;
        
        buf[i] = (int16_t)val;
    }
}

/***************************************************************************
SND_InitSound () - Does bout all i can think of for Music/FX initing
 ***************************************************************************/
int SND_InitSound(void)
{
    int music_card, fx_card, fx_chans;
    char *genmidi = NULL;
    SDL_AudioSpec spec = {}, actual = {};
    
    if (fx_init)
        return 0;

#ifdef __AMIGA__
    AmigaLog("AUDIO: SND_InitSound (nosound=%d nomusic=%d music_mode=%d)", g_nosound, g_nomusic, g_music_mode);
#endif

    /* -nosound : skip audio entirely - never open SDL_INIT_AUDIO / the
     * ahi.device backend at all. Music AND all sound effects (gun shots,
     * explosions, etc.) are disabled; music_volume/fx_volume stay at 0
     * (their static/global initial value) so every other SND_* call in
     * the game (SND_Patch/SND_3DPatch/SND_PlaySong/...) becomes a no-op
     * via their existing "if (fx_volume < 1) return;" / "if (music_volume
     * <= 1) return;" early-out checks - no other code needs to change. */
    if (g_nosound)
    {
        printf("[AUDIO] -nosound specified: audio subsystem disabled.\n");
        fflush(stdout);
#ifdef __AMIGA__
        AmigaLog("AUDIO: -nosound: audio subsystem disabled (no AHI, no CAMD).");
#endif
        fx_device = SND_NONE;
        music_volume = 0;
        fx_volume = 0;
        fx_init = 1;
        return 1;
    }

    if (SDL_Init(SDL_INIT_AUDIO) < 0)
        return 0;

    spec.freq = fx_freq;
    spec.format = AUDIO_S16SYS;
    spec.channels = 2;
    /* Buffer runway per AHI buffer at 11025 Hz: 512 frames ~= 46 ms,
     * 1024 frames ~= 93 ms. ADLIB/OPL3 music keeps the audio task busy
     * with dbopl rendering, so give it the larger buffer as underrun
     * headroom. CAMD MIDI and -nomusic keep 512: no OPL rendering there,
     * and SFX latency stays lower. */
    spec.samples = (g_music_mode == MUSIC_MODE_ADLIB && !g_nomusic) ? 1024 : 512;
    spec.callback = FX_Fill;
    spec.userdata = NULL;

    if ((fx_dev = SDL_OpenAudioDevice(NULL, 0, &spec, &actual, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE)) == 0)
    {
#ifdef __AMIGA__
        AmigaLog("AUDIO: SDL_OpenAudioDevice FAILED - game runs without sound.");
#endif
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return 0;
    }

#ifdef __AMIGA__
    AmigaLog("AUDIO: AHI stream open: freq=%ld format=0x%04x channels=%d",
             (long)actual.freq, (unsigned)actual.format, (int)actual.channels);
#endif
    fx_freq = actual.freq;
    
    if (actual.format != AUDIO_S16SYS || actual.channels != 2)
    {
#ifdef __AMIGA__
        AmigaLog("AUDIO: unexpected audio format/channels - closing.");
#endif
        SDL_CloseAudio();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return 0;
    }

    dig_flag = 0;
    fx_device = SND_NONE;

#ifdef __AMIGA__
    /* No MUSIC= option given (default state): report MUSIC=OFF once.  The
     * user can still select a backend explicitly with MUSIC=ADLIB, CAMD,
     * MHI or WAVE; with no backend selected no music engine is started
     * below (music_card = M_NONE), while AHI sound effects keep working. */
    if (g_music_mode == MUSIC_MODE_OFF && !g_nomusic)
    {
        AmigaLog("MUSIC: no backend selected; MUSIC=OFF (music disabled).");
        AmigaLog("MUSIC: sound effects remain enabled.");
    }
#endif

    /* No SETUP.INI is used anywhere in this Amiga port - all settings are
     * fixed built-in defaults, overridable by amiga.cfg (music_adlib /
     * music_mhi / sfx_volume).  Music volume picks the key of the active
     * backend; the in-game options sliders adjust it for the session and
     * write it back to amiga.cfg. */
    music_volume = 127;
#ifdef __AMIGA__
    AmigaCfg_Load();
    music_volume = (g_music_mode == MUSIC_MODE_MHI)
                       ? amiga_cfg_music_mhi
                       : (g_music_mode == MUSIC_MODE_WAVE)
                             ? amiga_cfg_music_wave
                             : amiga_cfg_music_adlib;
    if (music_volume < 0)   music_volume = 0;
    if (music_volume > 127) music_volume = 127;
    /* Amiga: the music backend is selected with the MUSIC= command-line
     * parameter / icon ToolType (parsed in main(), src/rap.cpp).  The
     * default state is MUSIC_MODE_OFF: no music backend is initialized
     * unless the user explicitly selects MUSIC=ADLIB, MUSIC=CAMD,
     * MUSIC=MHI or MUSIC=WAVE.  A failed MHI/CAMD initialization switches
     * the selected backend to MUSIC_MODE_OFF - never to AdLib. */
    if (g_music_mode == MUSIC_MODE_OFF)
        music_volume = 0;
    music_card = (g_music_mode == MUSIC_MODE_CAMD) ? M_GMIDI :
                 (g_music_mode == MUSIC_MODE_OFF)  ? M_NONE : M_ADLIB;

    /* MUSIC=WAVE: WAV files from the WAVE/ drawer are mixed straight into
     * the AHI stream by WAVE_Mix() (FX_Fill) - no MUS sequencer, no GLB
     * music items, no external driver.  SND_PlaySong/_IsSongPlaying/
     * _FadeOutSong route to the WAVE backend instead. */
    if (g_music_mode == MUSIC_MODE_WAVE)
        music_card = M_NONE;

    /* MUSIC=MHI: stream MP3 files from the MP3/ drawer through an MHI
     * decoder driver (e.g. LIBS:MHI/prismamhi.library).  On success the
     * MUS/OPL3 sequencer is bypassed entirely (music_card = M_NONE and
     * MUS_Init() is never called below); the SND_PlaySong/_StopSong/
     * _IsSongPlaying entry points route to the MHI backend instead.
     * On failure (no driver/decoder/task) the selected backend switches
     * to MUSIC_MODE_OFF - there is no automatic fallback to AdLib/OPL3.
     * -nomusic wins: MHI is never started then. */
    if (g_music_mode == MUSIC_MODE_MHI && !g_nomusic)
    {
        if (MHI_MusicInit())
        {
            music_card = M_NONE;
            AmigaLog("AUDIO: MHI music enabled (driver '%s')", MHI_DriverName());
            MUS_SetVolume(music_volume);
        }
        else
        {
            AmigaLog("MHI: initialization failed; MUSIC=MHI changed to MUSIC=OFF.");
            AmigaLog("MHI: sound effects remain enabled. Select MUSIC=ADLIB, MUSIC=CAMD,");
            AmigaLog("MUSIC=WAVE or MUSIC=OFF to choose music explicitly.");
            g_music_mode = MUSIC_MODE_OFF;
            music_volume = 0;
            music_card = M_NONE;
        }
    }
#else
    music_card = M_NONE;
    sys_midi = 0;
#endif

    switch (music_card)
    {
    case M_ADLIB:
    case M_PAS:
    case M_SB:
        genmidi = GLB_GetItem(FILE00e_GENMIDI_OP2);
        if (genmidi)
        {
            LoadInstrumentTable(genmidi);
            GLB_FreeItem(14);
        }
        break;
    }

    /* -nomusic : disable music only - sound effects (gun shots, explosions,
     * etc.) still work normally via fx_volume/fx_device below. Forcing
     * music_volume to 0 makes every music-related SND_* entry point
     * (SND_PlaySong/SND_FadeOutSong/SND_IsSongPlaying) a no-op via their
     * existing "if (music_volume <= 1) return;" checks, and skipping
     * MUS_Init() below means the OPL/TinySoundFont music backend is never
     * even initialized. */
    if (g_nomusic)
    {
        printf("[AUDIO] -nomusic specified: music disabled (sound FX still enabled).\n");
        fflush(stdout);
#ifdef __AMIGA__
        AmigaLog("AUDIO: -nomusic: music disabled, SFX via AHI stay enabled.");
#endif
        music_volume = 0;
    }

    if (music_card != M_NONE && !g_nomusic)
    {
        int mus_ok = MUS_Init(music_card, 0);

#ifdef __AMIGA__
        if (!mus_ok && music_card == M_GMIDI)
        {
            /* CAMD (camd.library / node / cluster link) unavailable: the
             * selected backend switches to MUSIC_MODE_OFF - there is no
             * automatic fallback to AdLib/OPL3. */
            AmigaLog("CAMD: initialization failed; MUSIC=CAMD changed to MUSIC=OFF.");
            AmigaLog("CAMD: sound effects remain enabled. Select MUSIC=ADLIB, MUSIC=MHI,");
            AmigaLog("MUSIC=WAVE or MUSIC=OFF to choose music explicitly.");
            g_music_mode = MUSIC_MODE_OFF;
            music_volume = 0;
            music_card = M_NONE;
        }
#endif
#ifdef __AMIGA__
        AmigaLog("AUDIO: music backend: card=%d (%s) init_ok=%d",
                 music_card, cards[music_card], mus_ok);
#endif
        if (mus_ok)
            MUS_SetVolume(music_volume);
    }

    /* Printed after backend selection so it names the real music device. */
#ifdef __AMIGA__
    if (!g_nomusic && g_music_mode == MUSIC_MODE_WAVE)
        printf("Music Enabled (WAV files from the WAVE/ drawer)\n");
    else if (!g_nomusic && g_music_mode == MUSIC_MODE_MHI && MHI_IsActive())
        printf("Music Enabled (MP3 via MHI: %s)\n", MHI_DriverName());
    else if (!g_nomusic && g_music_mode == MUSIC_MODE_OFF)
        printf("Music Disabled (MUSIC=OFF)\n");
    else if (!g_nomusic && music_card == M_GMIDI)
        printf("Music Enabled (General Midi via CAMD)\n");
    else if (!g_nomusic && music_card == M_ADLIB)
        printf("Music Enabled (AdLib/OPL3)\n");
    else
#endif
    printf("Music Enabled (%s)\n", g_nomusic ? "Disabled (-nomusic)" : cards[music_card]);

#ifdef __AMIGA__
    /* Amiga: digital SFX through the DSP mixer into the AHI stream.
    * Two mixer channels reduce 68060 CPU usage. */
    fx_volume = amiga_cfg_sfx;
    if (fx_volume < 0)   fx_volume = 0;
    if (fx_volume > 127) fx_volume = 127;
    fx_card = M_SB;
    fx_chans = 2;
#else
    fx_volume = 127;
    fx_card = M_NONE;
    fx_chans = 2;
#endif

    switch (fx_card)
    {
    default:
    case M_NONE:
        fx_device = SND_NONE;
        break;
    
    case M_PC:
        fx_device = SND_PC;
        break;
    
    case M_ADLIB:
        fx_device = SND_MIDI;
        if (!genmidi)
        {
            genmidi = GLB_GetItem(FILE00e_GENMIDI_OP2);
            if (genmidi)
            {
                LoadInstrumentTable(genmidi);
                GLB_FreeItem(14);
            }
        }
        break;
    
    case M_GUS:
    case M_PAS:
    case M_SB:
        fx_device = SND_DIGITAL;
        dig_flag = 1;
        break;
    
    case M_WAVE:
    case M_CANVAS:
    case M_GMIDI:
        fx_device = SND_MIDI;
        break;
    }

    printf("SoundFx Enabled (%s)\n", cards[fx_card]);

    if (fx_chans < 1 || fx_chans > 8)
        fx_chans = 2;

    if (fx_card == M_SB || fx_card == M_GUS || fx_card == M_PAS)
    {
        fx_channels = fx_chans;
        if (fx_card == M_GUS && fx_channels < 2)
            fx_gus = 1;
        DSP_Init(fx_channels, 11025);
#ifdef __AMIGA__
        AmigaLog("AUDIO: DSP_Init done: channels=%d rate=11025 fx_device=%d", fx_channels, fx_device);
#endif
    }
    else
        fx_channels = 1;

    if (fx_card == M_ADLIB || fx_card == M_WAVE || fx_card == M_CANVAS || fx_card == M_GMIDI)
        GSS_Init(fx_card, 0);

    SDL_PauseAudioDevice(fx_dev, 0);

#ifdef __AMIGA__
    AmigaLog("AUDIO: init complete (fx_card=%d fx_volume=%d fx_channels=%d music_card=%d music_volume=%d)",
             fx_card, fx_volume, fx_channels, music_card, music_volume);
#endif
    fx_init = 1;

    return 1;
}

/***************************************************************************
SND_DeInit () -
 ***************************************************************************/
void SND_DeInit(void)
{
    if (!fx_init)
        return;

    /* Shut down the music backend first: sends all-notes-off on every
     * channel and (on Amiga) removes the CAMD link/node and closes
     * camd.library.  MUS_DeInit() early-outs when music was never
     * initialised.  MUSIC=MHI never initialises MUS: it has its own
     * shutdown (stops the feeder task and closes the MHI driver). */
#ifdef __AMIGA__
    if (g_music_mode == MUSIC_MODE_MHI)
        MHI_MusicDeInit();
#endif
    MUS_DeInit();

    /* MUSIC=WAVE: free the decoded buffer while the audio task is still
     * running (it may read wave_data on its last/buffered mix), then the
     * task stop below guarantees no stale reads afterwards.  WAVE_DeInit()
     * serializes with WAVE_Mix() via SND_Lock/SND_Unlock. */
    if (g_music_mode == MUSIC_MODE_WAVE)
        WAVE_DeInit();

    /* Stops the audio task and closes ahi.device (Amiga SDL stub). */
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    fx_init = 0;
}

/***************************************************************************
SND_Setup() - Inits SND System  called after SND_InitSound() and GLB_Init
 ***************************************************************************/
void SND_Setup(void)
{
    int loop;
    DFX *lib;
    
    memset(fx_items, 0, sizeof(fx_items));
    
    if (fx_device == SND_NONE)
    {
        fx_loaded = 0;
        return;
    }
    
    fx_loaded = 1;

    // MONKEY 1 EFFECT ======================
    lib = &fx_items[FX_MON1];
    lib->item = GLB_GetItemID("MON1_FX");
    lib->pri = 1;
    lib->pitch = 128;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 0;
    lib->odig = 1;

    // MONKEY 2 EFFECT ======================
    lib = &fx_items[FX_MON2];
    lib->item = GLB_GetItemID("MON2_FX");
    lib->pri = 1;
    lib->pitch = 128;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 0;
    lib->odig = 1;

    // MONKEY 3 EFFECT ======================
    lib = &fx_items[FX_MON3];
    lib->item = GLB_GetItemID("MON3_FX");
    lib->pri = 1;
    lib->pitch = 128;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 0;
    lib->odig = 1;

    // MONKEY 4 EFFECT ======================
    lib = &fx_items[FX_MON4];
    lib->item = GLB_GetItemID("MON4_FX");
    lib->pri = 1;
    lib->pitch = 128;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 0;
    lib->odig = 1;

    // MONKEY 5 EFFECT ======================
    lib = &fx_items[FX_MON5];
    lib->item = GLB_GetItemID("MON5_FX");
    lib->pri = 1;
    lib->pitch = 128;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 0;
    lib->odig = 1;

    // MONKEY 6 EFFECT ======================
    lib = &fx_items[FX_MON6];
    lib->item = GLB_GetItemID("MON6_FX");
    lib->pri = 1;
    lib->pitch = 128;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 0;
    lib->odig = 1;

    // DAVE =================================
    lib = &fx_items[FX_DAVE];
    lib->item = GLB_GetItemID("DAVE_FX");
    lib->pri = 1;
    lib->pitch = 128;
    lib->rpflag = 0;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 0;
    lib->odig = 1;

    // THEME SONG ======================
    lib = &fx_items[FX_THEME];
    lib->item = GLB_GetItemID("THEME_FX");
    lib->pri = 0;
    lib->pitch = 128;
    lib->rpflag = 0;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 0;
    lib->odig = 1;

    // AIR EXPLOSION ======================
    lib = &fx_items[FX_AIREXPLO];
    lib->item = GLB_GetItemID("EXPLO_FX");
    lib->pri = 1;
    lib->pitch = 128;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 1;
    lib->odig = 0;

    // AIR EXPLOSION 2 ( BOSS ) ============
    lib = &fx_items[FX_AIREXPLO2];
    lib->item = GLB_GetItemID("EXPLO2_FX");
    lib->pri = 1;
    lib->pitch = 128;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 1;
    lib->odig = 0;

    // PICK UP BONUS ======================
    lib = &fx_items[FX_BONUS];
    lib->item = GLB_GetItemID("BONUS_FX");
    lib->pri = 0;
    lib->pitch = 128;
    lib->rpflag = 0;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 1;
    lib->odig = 0;

    // SHIP LOSES SOMTHING CRASH ==========
    lib = &fx_items[FX_CRASH];
    lib->item = GLB_GetItemID("CRASH_FX");
    lib->pri = 0;
    lib->pitch = 128;
    lib->rpflag = 0;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 1;
    lib->odig = 0;

    // DOOR OPENING =======================
    lib = &fx_items[FX_DOOR];
    lib->item = GLB_GetItemID("DOOR_FX");
    lib->pri = 0;
    lib->pitch = 120;
    lib->rpflag = 0;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 0;
    lib->odig = 0;

    // FLY BY SOUND =======================
    lib = &fx_items[FX_FLYBY];
    lib->item = GLB_GetItemID("FLYBY_FX");
    lib->pri = 0;
    lib->pitch = 120;
    lib->rpflag = 0;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 1;
    lib->odig = 0;

    // ENERGY GRAB ========================
    lib = &fx_items[FX_EGRAB];
    lib->item = GLB_GetItemID("EGRAB_FX");
    lib->pri = 2;
    lib->pitch = 128;
    lib->rpflag = 0;
    lib->sid = -1;
    lib->vol = 40;
    lib->gcache = 1;
    lib->odig = 0;

    // GROUND EXPLOSION ===================
    lib = &fx_items[FX_GEXPLO];
    lib->item = GLB_GetItemID("GEXPLO_FX");
    lib->pri = 2;
    lib->pitch = 128;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 1;
    lib->odig = 0;

    // NORM GUN ===========================
    lib = &fx_items[FX_GUN];
    lib->item = GLB_GetItemID("GUN_FX");
    lib->pri = 10;
    lib->pitch = 125;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 30;
    lib->gcache = 1;
    lib->odig = 0;

    // JET SOUND ==========================
    lib = &fx_items[FX_JETSND];
    lib->item = GLB_GetItemID("JETSND_FX");
    lib->pri = 4;
    lib->pitch = 120;
    lib->rpflag = 0;
    lib->sid = -1;
    lib->vol = 120;
    lib->gcache = 0;
    lib->odig = 0;

    // ====================================
    lib = &fx_items[FX_LASER];
    lib->item = GLB_GetItemID("LASER_FX");
    lib->pri = 2;
    lib->pitch = 120;
    lib->rpflag = 0;
    lib->sid = -1;
    lib->vol = 50;
    lib->gcache = 1;
    lib->odig = 0;

    // ====================================
    lib = &fx_items[FX_MISSLE];
    lib->item = GLB_GetItemID("MISSLE_FX");
    lib->pri = 3;
    lib->pitch = 120;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 50;
    lib->gcache = 1;
    lib->odig = 0;

    // ====================================
    lib = &fx_items[FX_SWEP];
    lib->item = GLB_GetItemID("SWEP_FX");
    lib->pri = 1;
    lib->pitch = 128;
    lib->rpflag = 0;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 1;
    lib->odig = 0;

    // ====================================
    lib = &fx_items[FX_TURRET];
    lib->item = GLB_GetItemID("TURRET_FX");
    lib->pri = 1;
    lib->pitch = 128;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 60;
    lib->gcache = 1;
    lib->odig = 0;

    // ====================================
    lib = &fx_items[FX_WARNING];
    lib->item = GLB_GetItemID("WARN_FX");
    lib->pri = 2;
    lib->pitch = 128;
    lib->rpflag = 0;
    lib->sid = -1;
    lib->vol = 100;
    lib->gcache = 1;
    lib->odig = 0;

    // ====================================
    lib = &fx_items[FX_BOSS1];
    lib->item = GLB_GetItemID("BOSS_FX");
    lib->pri = 1;
    lib->pitch = 127;
    lib->rpflag = 0;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 1;
    lib->odig = 1;

    // INSIDE JET SOUND ==========================
    lib = &fx_items[FX_IJETSND];
    lib->item = GLB_GetItemID("JETSND_FX");
    lib->pri = 1;
    lib->pitch = 235;
    lib->rpflag = 0;
    lib->sid = -1;
    lib->vol = 45;
    lib->gcache = 0;
    lib->odig = 0;

    // ENEMY JET SOUND ==========================
    lib = &fx_items[FX_EJETSND];
    lib->item = GLB_GetItemID("JETSND_FX");
    lib->pri = 1;
    lib->pitch = 65;
    lib->rpflag = 0;
    lib->sid = -1;
    lib->vol = 120;
    lib->gcache = 0;
    lib->odig = 0;

    // INTRO E HIT ===========================
    lib = &fx_items[FX_INTROHIT];
    lib->item = GLB_GetItemID("GUN_FX");
    lib->pri = 1;
    lib->pitch = 215;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 0;
    lib->odig = 0;

    // INTRO GUN ===========================
    lib = &fx_items[FX_INTROGUN];
    lib->item = GLB_GetItemID("GUN_FX");
    lib->pri = 1;
    lib->pitch = 110;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 0;
    lib->odig = 0;

    // ENEMY SHOT ==========================
    lib = &fx_items[FX_ENEMYSHOT];
    lib->item = GLB_GetItemID("ESHOT_FX");
    lib->pri = 1;
    lib->pitch = 100;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 50;
    lib->gcache = 1;
    lib->odig = 0;

    // ENEMY LASER ==========================
    lib = &fx_items[FX_ENEMYLASER];
    lib->item = GLB_GetItemID("LASER_FX");
    lib->pri = 1;
    lib->pitch = 70;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 120;
    lib->gcache = 1;
    lib->odig = 0;

    // ENEMY MISSLE =========================
    lib = &fx_items[FX_ENEMYMISSLE];
    lib->item = GLB_GetItemID("MISSLE_FX");
    lib->pri = 2;
    lib->pitch = 140;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 55;
    lib->gcache = 1;
    lib->odig = 0;

    // ENEMY SHOT ==========================
    lib = &fx_items[FX_ENEMYPLASMA];
    lib->item = GLB_GetItemID("ESHOT_FX");
    lib->pri = 2;
    lib->pitch = 127;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 1;
    lib->odig = 0;

    // SHIELD HIT =========================
    lib = &fx_items[FX_SHIT];
    lib->item = GLB_GetItemID("HIT_FX");
    lib->pri = 1;
    lib->pitch = 132;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 127;
    lib->gcache = 1;
    lib->odig = 0;

    // SHIP HIT =========================
    lib = &fx_items[FX_HIT];
    lib->item = GLB_GetItemID("GUN_FX");
    lib->pri = 1;
    lib->pitch = 214;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 100;
    lib->gcache = 1;
    lib->odig = 0;

    // NO_SHOOT ==========================
    lib = &fx_items[FX_NOSHOOT];
    lib->item = GLB_GetItemID("ESHOT_FX");
    lib->pri = 2;
    lib->pitch = 254;
    lib->rpflag = 0;
    lib->sid = -1;
    lib->vol = 40;
    lib->gcache = 1;
    lib->odig = 0;

    // PULSE CANNON ======================
    lib = &fx_items[FX_PULSE];
    lib->item = GLB_GetItemID("ESHOT_FX");
    lib->pri = 2;
    lib->pitch = 100;
    lib->rpflag = 1;
    lib->sid = -1;
    lib->vol = 50;
    lib->gcache = 1;
    lib->odig = 1;

    for (loop = 0; loop < FX_LAST_SND; loop++)
    {
        lib = &fx_items[loop];
        lib->sid = -1;
        
        if ((unsigned int)lib->item > 0)
        {
            lib->item += fx_device;
            GLB_CacheItem(lib->item);
        }
        else
            lib->item = -1;
    }
}

/***************************************************************************
SND_FreeFX () - Frees up Fx's
 ***************************************************************************/
void SND_FreeFX(void)
{
    int loop;
    DFX *lib;
    
    SND_StopPatches();
    
    for (loop = 0; loop < FX_LAST_SND; loop++)
    {
        lib = &fx_items[loop];
        
        if ((unsigned int)lib->item > 0)
            GLB_FreeItem(lib->item);
    }
}

/***************************************************************************
SND_CacheFX () Caches all FX's
 ***************************************************************************/
void SND_CacheFX(void)
{
    int loop;
    DFX *lib;
    
    for (loop = 0; loop < FX_LAST_SND; loop++)
    {
        lib = &fx_items[loop];
        
        if (lib->item != -1)
            GLB_CacheItem(lib->item);
    }
}

/***************************************************************************
SND_CacheGFX () Caches in Game FX's
 ***************************************************************************/
void SND_CacheGFX(void)
{
    int loop;
    DFX *lib;
    
    SND_StopPatches();
    
    for (loop = 0; loop < FX_LAST_SND; loop++)
    {
        lib = &fx_items[loop];
        
        if (lib->item != -1 && lib->gcache != 0)
            GLB_CacheItem(lib->item);
    }
}

/***************************************************************************
SND_CacheIFX () Caches intro and menu FX
 ***************************************************************************/
void SND_CacheIFX(void)
{
    int loop;
    DFX *lib;
    
    SND_StopPatches();
    
    for (loop = 0; loop < FX_LAST_SND; loop++)
    {
        lib = &fx_items[loop];
        
        if (lib->item != -1 && lib->gcache == 0)
            GLB_CacheItem(lib->item);
    }
}

/***************************************************************************
SFX_Playing () - Checks whether a sound is still playing on the backend
 ***************************************************************************/
int SFX_Playing(int handle)
{
    if (handle == -1)
        return 0;

    switch (handle & FXHAND_TMASK)
    {
    case FXHAND_GSS1:
        return GSS_PatchIsPlaying(handle);
    
    case FXHAND_DSP:
        return DSP_PatchIsPlaying(handle);
    }
    
    return 0;
}

/***************************************************************************
SFX_PlayPatch () -
 ***************************************************************************/
int SFX_PlayPatch(char* patch, int pitch, int sep, int vol, int priority)
{
#ifdef __AMIGA__
    /* One-shot diagnostic: proves the game requested a sound effect and
     * reports the AHI pump state at that moment. */
    {
        static int first_sfx_logged = 0;
        if (!first_sfx_logged)
        {
            first_sfx_logged = 1;
            AmigaLog("AUDIO: first SFX_PlayPatch (pitch=%d vol=%d | buffers filled=%lu last io_Error=%ld)",
                     pitch, vol,
                     (unsigned long)g_AmigaAudio.buffersFilled,
                     (long)g_AmigaAudio.lastError);
        }
    }
#endif

    if (!patch)
        return -1;

    int type = LE_SHORT(*(int16_t*)patch);
    
    switch (type)
    {
    case 0:
        break;
    
    case 1:
    case 2:
        return GSS_PlayPatch(patch, sep, pitch, vol, priority);
    case 3:
        return DSP_StartPatch((dsp_t*)patch, sep, pitch, vol, priority);
    }
    
    return -1;
}

/***************************************************************************
SFX_StopPatch () -
 ***************************************************************************/
void SFX_StopPatch(int handle)
{
    switch (handle & FXHAND_TMASK)
    {
    case FXHAND_GSS1:
        GSS_StopPatch(handle);
        break;
    
    case FXHAND_DSP:
        DSP_StopPatch(handle);
        break;
    }
}

/***************************************************************************
SND_Patch () - Test patch to see if it will be played by SND_Play
 ***************************************************************************/
void SND_Patch(int type, int xpos)
{
    char *patch;
    int rnd, numsnds, volume;
    DFX *curfld;
    int loop;
    
    if (fx_volume < 1)
        return;
    
    rnd = 0;
    numsnds = 0;
    curfld = fx_items;
    
    for (loop = 0; loop < FX_LAST_SND; loop++, curfld++)
    {
        if (curfld->sid != -1)
        {
            if (!SFX_Playing(curfld->sid) && loop != type)
                SND_StopPatch(loop);
            else
                numsnds++;
        }
    }
    
    if (numsnds <= fx_channels + 2)
    {
        curfld = &fx_items[type];
        
        if ((!curfld->odig || dig_flag) && curfld->item != -1)
        {
            if (curfld->rpflag)
            {
                rnd = wrand() % 40;
                rnd -= 20;
            }
            
            patch = GLB_LockItem(curfld->item);
            
            volume = (curfld->vol * fx_volume) / 127;
            
            curfld->sid = SFX_PlayPatch(patch, curfld->pitch + rnd, xpos, volume, curfld->pri);
        }
    }
}

/***************************************************************************
SND_3DPatch () - playes a patch in 3d for player during game play
 ***************************************************************************/
void SND_3DPatch(int type, int x, int y)
{
    int rnd;
    int numsnds, xpos;
    int loop;
    int dx, dy, dist, volume, vol, getdxdy;
    char *patch;
    DFX *curfld;
    
    if (fx_volume < 1)
        return;

    rnd = 0;
    numsnds = 0;
    curfld = fx_items;
    
    for (loop = 0; loop < FX_LAST_SND; loop++, curfld++)
    {
        if (curfld->sid != -1)
        {
            if (!SFX_Playing(curfld->sid))
                SND_StopPatch(loop);
            else
                numsnds++;
        }
    }
    
    if (numsnds <= fx_channels + 2)
    {
        dx = x - player_cx;
        dy = y - player_cy;
        
        xpos = dx + 127;
        
        if (xpos < 1)
            xpos = 1;
        else if (xpos > 255)
            xpos = 255;
        
        dx = abs(dx);
        dy = abs(dy);
        
        if (dx < dy)
            getdxdy = dx;
        else
            getdxdy = dy;
        
        dist = dx + dy - (getdxdy / 2);
        
        if (dist < SND_CLOSE)
            vol = 127;
        else if (dist > SND_FAR)
            vol = 1;
        else
            vol = 127 - ((dist - SND_CLOSE) * 127) / (SND_FAR - SND_CLOSE);
        
        curfld = &fx_items[type];
        
        if (!curfld->odig || dig_flag)
        {
            if (curfld->rpflag)
            {
                rnd = wrand() % 40;
                rnd -= 20;
            }
            
            volume = (vol * fx_volume) / 127;
            volume = (volume * curfld->vol) / 127;
            
            patch = GLB_LockItem(curfld->item);
            curfld->sid = SFX_PlayPatch(patch, curfld->pitch + rnd, xpos, volume, curfld->pri);
        }
    }
}

/***************************************************************************
SND_IsPatchPlaying() - Returns TRUE if patch is playing
 ***************************************************************************/
int SND_IsPatchPlaying(int type)
{
    DFX *curfld;
    
    curfld = &fx_items[type];
    
    if (curfld->sid != -1 && SFX_Playing(curfld->sid))
        return 1;
    
    return 0;
}

/***************************************************************************
SND_StopPatch () - Stops Type patch
 ***************************************************************************/
void SND_StopPatch(int type)
{
    DFX *curfld;
    
    curfld = &fx_items[type];
    
    if (curfld->sid != -1)
    {
        SFX_StopPatch(curfld->sid);
        GLB_UnlockItem(curfld->item);
        curfld->sid = -1;
    }
}

/***************************************************************************
SND_StopPatches () - Stops all currently playing patches
 ***************************************************************************/
void SND_StopPatches(void)
{
    int loop;
    DFX *curfld;
    
    curfld = fx_items;
    
    for (loop = 0; loop < FX_LAST_SND; loop++, curfld++)
    {
        if (curfld->sid != -1)
            SFX_StopPatch(curfld->sid);
    }
    
    curfld = fx_items;
    
    for (loop = 0; loop < FX_LAST_SND; loop++, curfld++)
    {
        if (curfld->sid != -1)
        {
            GLB_UnlockItem(curfld->item);
            curfld->sid = -1;
        }
    }
}

/***************************************************************************
SND_PlaySong() - Plays song associated with song id
 ***************************************************************************/
void SND_PlaySong(int item, int chainflag, int fadeflag)
{
    char *song;
    
    if (music_volume <= 1)
        return;

    /* MUSIC=OFF: music is disabled - never touch the MHI/WAVE/MUS
     * backends, no song is ever started. */
    if (g_music_mode == MUSIC_MODE_OFF)
        return;
    
#ifdef __AMIGA__
    /* MUSIC=MHI: play the MP3 mapped to this song item through the MHI
     * driver instead of the MUS sequencer.  No GLB item is locked (the
     * MUS data is never loaded), and fadeflag is ignored: MHI stops are
     * immediate.  When no MP3 matches the item, the song stays silent. */
    if (g_music_mode == MUSIC_MODE_MHI)
    {
        if (music_song == item)
            return;
        
        if (music_song != -1)
        {
            MHI_StopSong();
            music_song = -1;
        }
        
        if (item != -1)
        {
            music_song = item;
            AmigaLog("AUDIO: SND_PlaySong MHI item=%d loop=%d", item, chainflag);
            MHI_PlaySongItem(item, chainflag);
        }
        return;
    }

    /* MUSIC=WAVE: play the WAV mapped to this song item (WAVE/ drawer)
     * mixed into the AHI stream by WAVE_Mix().  No GLB item is locked and
     * fadeflag is ignored: WAVE stops are immediate.  When no WAV matches
     * the item, the song stays silent. */
    if (g_music_mode == MUSIC_MODE_WAVE)
    {
        if (music_song == item)
            return;

        if (music_song != -1)
        {
            WAVE_StopSong();
            music_song = -1;
        }

        if (item != -1)
        {
            music_song = item;
            AmigaLog("AUDIO: SND_PlaySong WAVE item=%d loop=%d", item, chainflag);
            WAVE_PlaySongItem(item, chainflag);
        }
        return;
    }
#endif

    if (music_song == item)
        return;
    
    if (music_song != -1)
    {
        MUS_StopSong(fadeflag);
        
        if (fadeflag)
        {
            while (MUS_SongPlaying())
            {
                I_GetEvent();
            }
        }
        
        GLB_UnlockItem(music_song);
        music_song = -1;
    }
    
    if (item != -1)
    {
        music_song = item;
        song = GLB_LockItem(item);
#ifdef __AMIGA__
        AmigaLog("AUDIO: SND_PlaySong item=%d loop=%d fade=%d", item, chainflag, fadeflag);
#endif
        MUS_PlaySong(song, chainflag, fadeflag);
    }
}

/***************************************************************************
SND_IsSongPlaying () - Is current song playing
 ***************************************************************************/
int SND_IsSongPlaying(void) 
{
    /* MUSIC=OFF: no song can be playing. */
    if (g_music_mode == MUSIC_MODE_OFF)
        return 0;

#ifdef __AMIGA__
    if (g_music_mode == MUSIC_MODE_MHI)
        return MHI_SongPlaying();
    if (g_music_mode == MUSIC_MODE_WAVE)
        return WAVE_SongPlaying();
#endif
    return MUS_SongPlaying();
}

/***************************************************************************
SND_FadeOutSong () - Fades current song out and stops playing music
 ***************************************************************************/
void SND_FadeOutSong(void)
{
    /* MUSIC=OFF: nothing to fade or stop. */
    if (g_music_mode == MUSIC_MODE_OFF)
        return;

#ifdef __AMIGA__
    /* MUSIC=MHI: no fade-out on the MHI driver - stop immediately.  No
     * GLB item was ever locked in this mode (see SND_PlaySong). */
    if (g_music_mode == MUSIC_MODE_MHI)
    {
        if (music_song != -1)
        {
            MHI_StopSong();
            music_song = -1;
        }
        return;
    }

    /* MUSIC=WAVE: no fade-out - stop immediately.  No GLB item was ever
     * locked in this mode (see SND_PlaySong). */
    if (g_music_mode == MUSIC_MODE_WAVE)
    {
        if (music_song != -1)
        {
            WAVE_StopSong();
            music_song = -1;
        }
        return;
    }
#endif
    if (music_song != -1)
    {
        if (MUS_SongPlaying())
            MUS_StopSong(1);
        
        while (MUS_SongPlaying())
        {
            I_GetEvent();
        }
        
        GLB_UnlockItem(music_song);
    }
    
    music_song = -1;
}

/***************************************************************************
SND_Lock () -
 ***************************************************************************/
void SND_Lock(void)
{
    if (!lockcount)
        SDL_LockAudioDevice(fx_dev);
    
    lockcount++;
}

/***************************************************************************
SND_Unlock () -
 ***************************************************************************/
void SND_Unlock(void)
{
    lockcount--;
    
    if (!lockcount)
        SDL_UnlockAudioDevice(fx_dev);
}