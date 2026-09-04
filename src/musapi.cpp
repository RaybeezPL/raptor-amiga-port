#include "SDL.h"
#include <stdint.h>
#include "common.h"
#include "musapi.h"
#include "fx.h"
#include "mpumhi.h"
#include "cards.h"
#include "gssapi.h"
#include "entypes.h"

static int musrate = 70;
static int musfaderate = 50;
int music_init;
int music_startoffset;
int music_len;
int music_cmdptr;
int music_active;
int music_loop;
int music_delay;
int music_timer;
int music_vol;
int music_voltarget;
int music_currentvol = 127;
int music_lastvol;
int music_fading;
int music_channels;

int music_faderaccum;
int music_faderadd1;
int music_faderadd2;
int music_fadersteps;

int music_cnt, music_cnt2, music_cnt3;
char *music_ptr = NULL;

musdevice_t *music_device;

int music_chanvel[16];
int music_chanvol[16];
int music_chanvol2[16];

#pragma pack(push, 1)
struct mushead_t {
    char id[4];
    uint16_t len;
    uint16_t offset;
    uint16_t channels;
};
#pragma pack(pop)

/***************************************************************************
MUS_SetupFader() -
 ***************************************************************************/
int 
MUS_SetupFader(
    int s
)
{
    int start, end;
    
    if (s) // starting song
    {
        start = 0;
        end = 127;
    }
    else
    {
        start = music_currentvol;
        end = 0;
    }
    
    int steps = abs(end - start);
    const int count = 51; // ~1000ms
    
    if (steps == 0)
    {
        return 0;
    }
    
    music_fading = 1;
    music_faderadd1 = steps / count;
    music_faderadd2 = steps % count;
    music_fadersteps = steps;
    music_faderaccum = -(music_faderadd2 >> 1);
    music_vol = start;
    music_voltarget = end;
    
    return 1;
}

/***************************************************************************
MUS_UpdateVolume() -
 ***************************************************************************/
void 
MUS_UpdateVolume(
    void
)
{
    int i, vol;
    
    for (i = 0; i < 16; i++)
    {
        if (i == music_channels)
            i = 15;
        
        vol = music_currentvol;
        
        if (music_fading && vol > music_vol)
            vol = music_vol;
        
        if (i != 15 && vol > music_chanvol2[i])
            vol = music_chanvol2[i];
        
        if (vol != music_chanvol[i])
        {
            music_chanvol[i] = vol;
            
            if (music_device && music_device->ControllerEvent)
                music_device->ControllerEvent(i, 3, vol);
        }
    }
}

/***************************************************************************
MUS_Fader() -
 ***************************************************************************/
void 
MUS_Fader(
    void
)
{
    int update = 0;
    
    if (music_lastvol != music_currentvol)
    {
        music_lastvol = music_currentvol;
        update = 1;
    }

    if (music_fading)
    {
        update = 1;
        
        if (music_vol < music_currentvol || music_voltarget < music_currentvol)
        {
            int cnt = music_faderadd1;
            
            music_faderaccum += music_faderadd2;
            
            if (music_faderaccum >= music_fadersteps)
            {
                music_faderaccum -= music_fadersteps;
                cnt++;
            }
            
            if (cnt)
            {
                if (music_voltarget < music_vol) // fade out
                {
                    music_vol -= cnt;
                    
                    if (music_vol <= 0)
                    {
                        music_vol = 0;
                        // stop song
                        music_active = 0;
                        
                        for (int i = 0; i < 16; i++)
                        {
                            if (music_device && music_device->ControllerEvent)
                            {
                                music_device->ControllerEvent(i, 10, 0);
                                music_device->ControllerEvent(i, 11, 0);
                            }
                        }
                        music_ptr = NULL;
                        
                        return;
                    }
                    if (music_vol == music_voltarget)
                        music_fading = 0;
                }
                else // fade in
                {
                    music_vol += cnt;
                    
                    if (music_vol >= music_voltarget)
                    {
                        music_vol = music_voltarget;
                        music_fading = 0;
                    }
                    
                    if (music_vol >= music_currentvol)
                    {
                        music_vol = music_currentvol;
                        music_fading = 0;
                    }
                }
            }
        }
    }
    if (update)
        MUS_UpdateVolume();
}

/***************************************************************************
MUS_Reset() -
 ***************************************************************************/
void 
MUS_Reset(
    void
)
{
    music_cmdptr = 0;
    
    if (music_vol > music_currentvol)
        music_vol = music_currentvol;
    
    if (!music_fading)
        music_vol = music_currentvol;
    
    for (int i = 0; i < 16; i++)
    {
        music_chanvol[i] = 100;
        music_chanvol2[i] = 100;
        
        if (i < music_channels || i == 15)
            if (music_device && music_device->ControllerEvent)
            {
                int newvol = 100;
                
                music_device->ControllerEvent(i, 14, 0); // reset
                
                if (newvol > music_vol)
                    newvol = music_vol;
                
                if (newvol > music_currentvol)
                    newvol = music_currentvol;
                
                music_device->ControllerEvent(i, 3, newvol);
            }
            if (music_device && music_device->AllNotesOffEvent)
                music_device->AllNotesOffEvent(i,0);
    }
}

/***************************************************************************
MUS_Service() -
 ***************************************************************************/
void 
MUS_Service(
    void
)
{
    if (music_active)
    {
        if (music_delay)
            music_delay--;
        
        while (!music_delay)
        {
            uint8_t cmd = music_ptr[music_cmdptr + music_startoffset];
            music_cmdptr++;
            int chan = cmd & 15;
            
            switch ((cmd >> 4) & 7)
            {
                case 0:
                {
                    uint8_t key = music_ptr[music_cmdptr + music_startoffset] & 127;
                    music_cmdptr++;
                    if (music_device && music_device->KeyOffEvent)
                        music_device->KeyOffEvent(chan, key);
                    break;
                }
                
                case 1:
                {
                    uint8_t key = music_ptr[music_cmdptr + music_startoffset];
                    music_cmdptr++;
                    uint8_t vol = music_chanvel[chan];
                    if (key & 128)
                    {
                        vol = music_ptr[music_cmdptr + music_startoffset];
                        music_cmdptr++;
                        music_chanvel[chan] = vol;
                        key &= ~128;
                    }
                    if (music_device && music_device->KeyOnEvent)
                        music_device->KeyOnEvent(chan, key, vol);
                    break;
                }
                
                case 2:
                {
                    uint8_t bend = music_ptr[music_cmdptr + music_startoffset];
                    music_cmdptr++;
                    if (music_device && music_device->PitchBendEvent)
                        music_device->PitchBendEvent(chan, bend);
                    break;
                }
                
                case 3:
                {
                    uint8_t cmd = music_ptr[music_cmdptr + music_startoffset];
                    music_cmdptr++;
                    if (music_device && music_device->ControllerEvent)
                        music_device->ControllerEvent(chan, cmd, 0);
                    break;
                }
                
                case 4:
                {
                    uint8_t cmd = music_ptr[music_cmdptr + music_startoffset];
                    music_cmdptr++;
                    uint8_t param = music_ptr[music_cmdptr + music_startoffset];
                    music_cmdptr++;
                    
                    switch (cmd)
                    {
                        case 0: // Program Channel
                            if(music_device && music_device->ProgramEvent)
                                music_device->ProgramEvent(chan, param);
                            break;
                        
                        case 3: // Volume
                            music_chanvol2[chan] = param;
                            if (param > music_currentvol)
                                param = music_currentvol;
                            if (param > music_vol)
                                param = music_vol;
                            music_chanvol[chan] = param;
                            if (music_device && music_device->ControllerEvent)
                                music_device->ControllerEvent(chan, cmd, param);
                            break;
                        
                        case 4: //SetChannelPan
                            if (music_device && music_device->ControllerEvent)
                                music_device->ControllerEvent(chan, cmd, param);
                            break;
                        
                        case 10:
                        case 11:
                            if (music_device && music_device->AllNotesOffEvent)
                                music_device->AllNotesOffEvent(chan, param);
                            break;
                        
                        case 12:
                        case 13:
                            // TODO: mono/poly mode
                            break;
                        
                        case 14: // Restore volume setting
                            if (music_device && music_device->AllNotesOffEvent)
                                music_device->AllNotesOffEvent(chan, param);

                            param = music_chanvel[chan];
                            if (param > music_currentvol)
                                param = music_currentvol;
                            if (param > music_vol)
                                param = music_vol;
                            if (music_device && music_device->ControllerEvent)
                                music_device->ControllerEvent(chan, 3, param);
                                break;

                        default:
                            break;
                    }
                    break;
                }
                
                case 6:
                {
                    if (music_loop)
                    {
                        MUS_Reset();
                    }
                    else
                    {
                        music_active = 0;
                        return;
                    }
                    break;
                }
            }
            if (cmd & 128)
            {
                music_delay = 0;
                do
                {
                    uint8_t val = music_ptr[music_cmdptr + music_startoffset];
                    music_cmdptr++;
                    music_delay = (music_delay << 7) + (val & 127);
                    if (!(val & 128))
                        break;
                } while (1);
            }
        }
    }
}

/***************************************************************************
MUS_Init() -
 ***************************************************************************/
int 
MUS_Init(
    int card, 
    int option
)
{
    if (music_init)
        return 0;
    
    music_cnt2 = music_cnt = 0;
    music_active = 0;
    music_ptr = NULL;
    
    switch (card)
    {
    case M_NONE:
        music_device = NULL;
        break;
    
    case M_ADLIB:
    case M_PAS:
    case M_SB:
        music_device = &mus_device_opl;
        break;
    
    case M_WAVE:
    case M_CANVAS:
    case M_GMIDI:
    default:
#ifdef __AMIGA__
        /* Amiga: General MIDI music is played through camd.library
         * (mpucamd.cpp).  The TinySoundFont softsynth is deliberately
         * never used here: its TSF_Init() opens its own SDL audio device,
         * which would hijack the AHI backend and kill the sound effects. */
        music_device = &mus_device_camd;
#else
        if (sys_midi)
        {
            #ifdef _WIN32
            music_device = &mus_device_winmm;
            #endif // _WIN32

            #ifndef __ANDROID__
            #ifdef __linux__
            music_device = &mus_device_alsa;
            #endif // __linux__
            #endif //__ANDROID__
            
            #ifdef __APPLE__
            if (core_dls_synth)
                music_device = &mus_device_corea;
            else
                music_device = &mus_device_corem;
            #endif // __APPLE__
        }
        else
        music_device = &mus_device_tsf;
#endif
        break;
    }

#ifdef __AMIGA__
    AmigaLog("MUS: Init card=%d -> %s backend", card,
             music_device == &mus_device_camd ? "CAMD" :
             music_device == &mus_device_opl  ? "OPL3/AdLib" :
             (music_device ? "other" : "none"));
#endif

    if (music_device && music_device->Init)
    {
#ifdef __AMIGA__
        /* Amiga: propagate init failure (e.g. camd.library missing) so
         * SND_InitSound() can switch the selected backend to MUSIC=OFF -
         * there is no automatic fallback to AdLib/OPL3. */
        if (!music_device->Init(option))
        {
            AmigaLog("MUS: backend init FAILED (card=%d)", card);
            music_device = NULL;
            return 0;
        }
#else
        music_device->Init(option);
#endif
    }

    if (music_device && music_device->AllNotesOffEvent)
        music_device->AllNotesOffEvent(0,0);

    music_timer = SDL_GetTicks();
    music_init = 1;
    
    return 1;
}

/***************************************************************************
MUS_DeInit() -
 ***************************************************************************/
void 
MUS_DeInit(
    void
)
{
    if (!music_init)
        return;
    
    for (int i = 0; i < 16; i++)
    {
        if (music_device && music_device->AllNotesOffEvent)
            music_device->AllNotesOffEvent(i,0);
    }
    
    music_init = 0;
    music_active = 0;
    music_ptr = NULL;
    
    if (music_device && music_device->DeInit)
        music_device->DeInit();
    
    music_device = NULL;
}

/***************************************************************************
MUS_PlaySong() -
 ***************************************************************************/
void 
MUS_PlaySong(
    void *ptr, 
    int loop, 
    int fadein
)
{
    mushead_t *head;
    head = (mushead_t*)ptr;

    if (!music_init)
        return;

    if (!ptr)
        return;

    SND_Lock();
    
    if (music_active)
        MUS_StopSong(0);
    
    music_ptr = (char*)ptr;
    music_len = LE_USHORT(head->len);
    music_startoffset = LE_USHORT(head->offset);
    music_cmdptr = 0;
    music_loop = loop;
    music_active = 1;
    music_delay = 0;
    music_vol = 127;
    music_fading = 0;
    music_channels = LE_USHORT(head->channels);
    MUS_Reset();
    
    if (fadein)
        MUS_SetupFader(1);
    
    SND_Unlock();
}

/***************************************************************************
MUS_StopSong() -
 ***************************************************************************/
void 
MUS_StopSong(
    int fadeout
)
{
    if (!music_init)
        return;
    
    SND_Lock();
    
    if (fadeout && MUS_SetupFader(0))
    {
        SND_Unlock();
        return;
    }
    
    music_active = 0;
    
    for (int i = 0; i < 16; i++)
    {
        if (music_device && music_device->ControllerEvent)
        {
            music_device->ControllerEvent(i, 10, 0);
            music_device->ControllerEvent(i, 11, 0);
        }
        if (music_device && music_device->AllNotesOffEvent)
            music_device->AllNotesOffEvent(i,0);
    }
    
    music_ptr = NULL;
    SND_Unlock();
}

/***************************************************************************
MUS_SongPlaying() -
 ***************************************************************************/
int 
MUS_SongPlaying(
    void
)
{
    if (!music_init)
        return 0;
    
    return music_active;
}

/***************************************************************************
MUS_Mix() -
 ***************************************************************************/
void 
MUS_Mix(
    int16_t *stream, 
    int len
)
{
    if (!music_init || !music_device || !music_device->Mix)
        return;
    
#ifdef __AMIGA__
    /* 68060: render in chunks aligned to the service boundaries instead of
     * one Mix(stream, 1) call per sample.  With the OPL/dbopl backend each
     * Mix() call walks all 18 OPL channels (indirect member-function calls
     * plus per-channel Prepare/Silent work), so per-sample calls at 11025 Hz
     * cost more than a whole 68060 - the priority +10 audio task then never
     * sleeps in WaitIO() and starves the game to ~1 FPS.
     *
     * Each chunk ends exactly where the next service (MUS_Service 70 Hz /
     * MUS_Fader 50 Hz / GSS_Service 140 Hz) would fire in the per-sample
     * loop, so the event timing stays bit-exact while the emulator renders
     * ~50-220 sample blocks with the per-channel setup amortised. */
    while (len > 0)
    {
        int chunk = len;
        int todo;
        
        /* Samples until the next service tick: the smallest k with
         * cnt + k*rate >= fx_freq (counters are always < fx_freq here). */
        todo = (fx_freq - music_cnt + musrate - 1) / musrate;
        if (todo < chunk)
            chunk = todo;
        
        todo = (fx_freq - music_cnt2 + musfaderate - 1) / musfaderate;
        if (todo < chunk)
            chunk = todo;
        
        if (gsshack)
        {
            todo = (fx_freq - music_cnt3 + gssrate - 1) / gssrate;
            if (todo < chunk)
                chunk = todo;
        }
        
        if (chunk < 1)
            chunk = 1;
        
        music_device->Mix(stream, chunk);
        
        music_cnt += musrate * chunk;
        while (music_cnt >= fx_freq)
        {
            music_cnt -= fx_freq;
            MUS_Service();
        }
        
        music_cnt2 += musfaderate * chunk;
        while (music_cnt2 >= fx_freq)
        {
            music_cnt2 -= fx_freq;
            MUS_Fader();
        }
        
        if (gsshack)
        {
            music_cnt3 += gssrate * chunk;
            while (music_cnt3 >= fx_freq)
            {
                music_cnt3 -= fx_freq;
                GSS_Service();
            }
        }
        
        stream += chunk * 2;
        len -= chunk;
    }
#else
    int i;
    
    for (i = 0; i < len; i++)
    {
        music_device->Mix(stream, 1);
        music_cnt += musrate;
        
        while (music_cnt >= fx_freq)
        {
            music_cnt -= fx_freq;
            MUS_Service();
        }
        music_cnt2 += musfaderate;
        
        while (music_cnt2 >= fx_freq)
        {
            music_cnt2 -= fx_freq;
            MUS_Fader();
        }
        
        if (gsshack)
        {
            music_cnt3 += gssrate;
            
            while (music_cnt3 >= fx_freq)
            {
                music_cnt3 -= fx_freq;
                GSS_Service();
            }
        }
        stream += 2;
    }
#endif
}

/***************************************************************************
MUS_SetVolume() -
 ***************************************************************************/
void 
MUS_SetVolume(
    int volume
)
{
    if (music_currentvol == volume)
        return;

    music_currentvol = volume;

#ifdef __AMIGA__
    /* MUSIC=MHI: the MUS sequencer never runs in this mode, so forward
     * the volume to the MHI driver (when it supports volume control). */
    if (g_music_mode == MUSIC_MODE_MHI)
        MHI_SetVolume(volume);
#endif
}

/***************************************************************************
MUS_Poll() -
 ***************************************************************************/
void 
MUS_Poll(
    void
)
{
    GSS_Poll();
    
    if (!music_init || !music_device || music_device->Mix != NULL)
        return;

    int now = SDL_GetTicks();
    
    while (music_timer < now)
    {
        music_cnt += musrate;
        
        if (music_cnt >= 1000)
        {
            music_cnt -= 1000;
            MUS_Service();
        }
        music_cnt2 += musfaderate;
        
        while (music_cnt2 >= 1000)
        {
            music_cnt2 -= 1000;
            MUS_Fader();
        }
        music_timer++;
    }
}
