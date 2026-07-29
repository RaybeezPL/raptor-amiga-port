#include <stdint.h>
#include <stdio.h>
#include "SDL.h"
#include "common.h"
#include "dspapi.h"
#include "fx.h"
#include "entypes.h"


int dsp_init = 0;
int dsp_freq = 11025;
int dsp_channelnum = 8;
int dsp_cnt = 0;
int dsp_rsmp = 0;
int dsp_samp[2] = {0, 0};


/* Pomocnicza pewna konwersja Little-Endian do Host (m68k/Amiga) */
static inline uint16_t SwapLE16(uint16_t val)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    return (uint16_t)((val << 8) | (val >> 8));
#else
    return val;
#endif
}


static inline uint32_t SwapLE32(uint32_t val)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    return ((val << 24) |
            ((val << 8) & 0x00FF0000u) |
            ((val >> 8) & 0x0000FF00u) |
            (val >> 24));
#else
    return val;
#endif
}


uint16_t pitchtable[256] = {
    0x0040, 0x0040, 0x0041, 0x0042, 0x0042, 0x0043,
    0x0044, 0x0045, 0x0045, 0x0046, 0x0047, 0x0048,
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004C,
    0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052,
    0x0053, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D,
    0x005E, 0x005F, 0x0060, 0x0061, 0x0062, 0x0063,
    0x0064, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A,
    0x006B, 0x006C, 0x006E, 0x006F, 0x0070, 0x0071,
    0x0072, 0x0074, 0x0075, 0x0076, 0x0077, 0x0079,
    0x007A, 0x007B, 0x007D, 0x007E, 0x0080, 0x0081,
    0x0082, 0x0084, 0x0085, 0x0087, 0x0088, 0x008A,
    0x008B, 0x008D, 0x008E, 0x0090, 0x0091, 0x0093,
    0x0095, 0x0096, 0x0098, 0x0099, 0x009B, 0x009D,
    0x009F, 0x00A0, 0x00A2, 0x00A4, 0x00A6, 0x00A7,
    0x00A9, 0x00AB, 0x00AD, 0x00AF, 0x00B1, 0x00B3,
    0x00B5, 0x00B7, 0x00B9, 0x00BB, 0x00BD, 0x00BF,
    0x00C1, 0x00C3, 0x00C5, 0x00C7, 0x00C9, 0x00CB,
    0x00CE, 0x00D0, 0x00D2, 0x00D5, 0x00D7, 0x00D9,
    0x00DC, 0x00DE, 0x00E0, 0x00E3, 0x00E5, 0x00E8,
    0x00EA, 0x00ED, 0x00EF, 0x00F2, 0x00F5, 0x00F7,
    0x00FA, 0x00FD, 0x0100, 0x0102, 0x0105, 0x0108,
    0x010B, 0x010E, 0x0111, 0x0114, 0x0117, 0x011A,
    0x011D, 0x0120, 0x0123, 0x0126, 0x0129, 0x012D,
    0x0130, 0x0133, 0x0137, 0x013A, 0x013D, 0x0141,
    0x0144, 0x0148, 0x014C, 0x014F, 0x0153, 0x0157,
    0x015A, 0x015E, 0x0162, 0x0166, 0x016A, 0x016E,
    0x0172, 0x0176, 0x017A, 0x017E, 0x0182, 0x0186,
    0x018A, 0x018F, 0x0193, 0x0197, 0x019C, 0x01A0,
    0x01A5, 0x01A9, 0x01AE, 0x01B3, 0x01B8, 0x01BC,
    0x01C1, 0x01C6, 0x01CB, 0x01D0, 0x01D5, 0x01DA,
    0x01DF, 0x01E5, 0x01EA, 0x01EF, 0x01F5, 0x01FA,
    0x0200, 0x0205, 0x020B, 0x0210, 0x0216, 0x021C,
    0x0222, 0x0228, 0x022E, 0x0234, 0x023A, 0x0240,
    0x0247, 0x024D, 0x0253, 0x025A, 0x0260, 0x0267,
    0x026E, 0x0275, 0x027B, 0x0282, 0x0289, 0x0290,
    0x0298, 0x029F, 0x02A6, 0x02AD, 0x02B5, 0x02BC,
    0x02C4, 0x02CC, 0x02D4, 0x02DC, 0x02E3, 0x02EC,
    0x02F4, 0x02FC, 0x0304, 0x030D, 0x0315, 0x031E,
    0x0326, 0x032F, 0x0338, 0x0341, 0x034A, 0x0353,
    0x035D, 0x0366, 0x036F, 0x0379, 0x0383, 0x038D,
    0x0396, 0x03A0, 0x03AB, 0x03B5, 0x03BF, 0x03CA,
    0x03D4, 0x03DF, 0x03EA, 0x03F5
};


uint8_t pantable[256] = {
    0x0E, 0x13, 0x19, 0x1D, 0x22, 0x26, 0x2A, 0x2F, 0x33, 0x37, 0x3B, 0x3F,
    0x41, 0x45, 0x49, 0x4C, 0x50, 0x53, 0x55, 0x59, 0x5C, 0x5E, 0x61, 0x65,
    0x67, 0x6A, 0x6C, 0x6F, 0x71, 0x74, 0x75, 0x78, 0x7A, 0x7D, 0x7F, 0x80,
    0x83, 0x85, 0x86, 0x89, 0x8B, 0x8C, 0x8E, 0x90, 0x92, 0x93, 0x95, 0x96,
    0x99, 0x9A, 0x9C, 0x9D, 0x9E, 0xA0, 0xA2, 0xA3, 0xA5, 0xA6, 0xA7, 0xA9,
    0xAA, 0xAB, 0xAC, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB5, 0xB6, 0xB7,
    0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3,
    0xC4, 0xC5, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCD,
    0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD2, 0xD3, 0xD3, 0xD4, 0xD5, 0xD6,
    0xD6, 0xD6, 0xD7, 0xD8, 0xD9, 0xD9, 0xD9, 0xDA, 0xDB, 0xDC, 0xDC, 0xDC,
    0xDD, 0xDE, 0xDE, 0xDF, 0xDF, 0xE0, 0xE0, 0xE1, 0xE1, 0xE1, 0xE2, 0xE3,
    0xE4, 0xE4, 0xE4, 0xE5, 0xE5, 0xE6, 0xE6, 0xE6, 0xE7, 0xE7, 0xE7, 0xE8,
    0xE9, 0xE9, 0xE9, 0xE9, 0xEA, 0xEB, 0xEB, 0xEB, 0xEC, 0xEC, 0xEC, 0xEC,
    0xED, 0xED, 0xED, 0xEE, 0xEE, 0xEF, 0xEF, 0xEF, 0xF0, 0xF0, 0xF0, 0xF0,
    0xF1, 0xF1, 0xF1, 0xF2, 0xF2, 0xF2, 0xF2, 0xF3, 0xF3, 0xF3, 0xF3, 0xF4,
    0xF4, 0xF4, 0xF5, 0xF5, 0xF5, 0xF5, 0xF6, 0xF6, 0xF6, 0xF6, 0xF7, 0xF7,
    0xF7, 0xF7, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9,
    0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 0xFA, 0xFA, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB,
    0xFB, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFD, 0xFD, 0xFD, 0xFD,
    0xFD, 0xFD, 0xFD, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF
};


struct channel_t {
    dsp_t *dsp;
    char *data;
    int handle;
    int priority;
    int samples;
    int phase_inc;
    int phase_sub;
    int phase_sub_inc;
    int voltable1[256];
    int voltable2[256];
};


static channel_t dsp_channels[8];


/***************************************************************************
DSP_Init() -
 ***************************************************************************/
void DSP_Init(int channels, int freq)
{
    int i;

    if (dsp_init)
        return;

    if (channels < 1)
        channels = 1;
    if (channels > 8)
        channels = 8;

    dsp_channelnum = channels;
    dsp_freq = (freq > 0) ? freq : 11025;

    for (i = 0; i < 8; i++) {
        dsp_channels[i].samples = 0;
        dsp_channels[i].dsp = NULL;
        dsp_channels[i].data = NULL;
        dsp_channels[i].handle = -1;
        dsp_channels[i].priority = -2147483647 - 1;
        dsp_channels[i].phase_inc = 0;
        dsp_channels[i].phase_sub = 0;
        dsp_channels[i].phase_sub_inc = 0;
    }

    dsp_samp[0] = 0;
    dsp_samp[1] = 0;
    dsp_rsmp = 0;
    dsp_cnt = 0;
    dsp_init = 1;
}


/***************************************************************************
DSP_DeInit() -
 ***************************************************************************/
void DSP_DeInit(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        dsp_channels[i].samples = 0;
        dsp_channels[i].dsp = NULL;
        dsp_channels[i].data = NULL;
        dsp_channels[i].handle = -1;
        dsp_channels[i].priority = -2147483647 - 1;
        dsp_channels[i].phase_inc = 0;
        dsp_channels[i].phase_sub = 0;
        dsp_channels[i].phase_sub_inc = 0;
    }

    dsp_samp[0] = 0;
    dsp_samp[1] = 0;
    dsp_rsmp = 0;
    dsp_init = 0;
}


/***************************************************************************
DSP_Mix() -
 ***************************************************************************/
void DSP_Mix(int16_t *buffer, int len)
{
    channel_t *chan;
    int i, l, r, j;

    if (!dsp_init || !buffer || len <= 0)
        return;

    if (fx_freq <= 0)
        fx_freq = dsp_freq;

    for (i = 0; i < len; i++) {
        dsp_rsmp += dsp_freq;

        while (dsp_rsmp >= fx_freq) {
            dsp_rsmp -= fx_freq;
            l = 0;
            r = 0;

            for (j = 0; j < dsp_channelnum; j++) {
                chan = &dsp_channels[j];

                if (chan->samples > 0 && chan->data != NULL) {
                    l += chan->voltable1[(uint8_t)*chan->data];
                    r += chan->voltable2[(uint8_t)*chan->data];

                    chan->phase_sub += chan->phase_sub_inc;
                    if (chan->phase_sub >= 256) {
                        chan->phase_sub -= 256;
                        chan->data++;
                    }

                    chan->data += chan->phase_inc;
                    chan->samples--;

                    if (chan->samples <= 0) {
                        chan->samples = 0;
                        chan->dsp = NULL;
                        chan->data = NULL;
                        chan->handle = -1;
                        chan->priority = -2147483647 - 1;
                        chan->phase_inc = 0;
                        chan->phase_sub = 0;
                        chan->phase_sub_inc = 0;
                    }
                }
            }

            if (l < -128)
                l = -128;
            else if (l > 127)
                l = 127;

            if (r < -128)
                r = -128;
            else if (r > 127)
                r = 127;

            dsp_samp[0] = l * 128;
            dsp_samp[1] = r * 128;
        }

        l = buffer[0] + dsp_samp[0];
        r = buffer[1] + dsp_samp[1];

        if (l < -32768)
            l = -32768;
        else if (l > 32767)
            l = 32767;

        if (r < -32768)
            r = -32768;
        else if (r > 32767)
            r = 32767;

        buffer[0] = (int16_t)l;
        buffer[1] = (int16_t)r;
        buffer += 2;
    }
}


/***************************************************************************
DSP_PatchIsPlaying() -
 ***************************************************************************/
int DSP_PatchIsPlaying(int handle)
{
    int i, stat = 0;

    handle &= FXHAND_MASK;

    SND_Lock();
    for (i = 0; i < dsp_channelnum; i++) {
        if (dsp_channels[i].samples > 0 && dsp_channels[i].handle == handle) {
            stat = 1;
            break;
        }
    }
    SND_Unlock();

    return stat;
}


/***************************************************************************
DSP_VolTable() -
 ***************************************************************************/
void DSP_VolTable(int *table, int vol)
{
    int val, step, sub_step, accm, i;

    val = -(vol / 2);
    step = vol >> 8;
    sub_step = vol & 255;
    accm = sub_step >> 1;

    for (i = 0; i < 256; i++) {
        table[i] = val;
        val += step;
        accm += sub_step;
        while (accm >= 256) {
            val++;
            accm -= 256;
        }
    }
}


/***************************************************************************
DSP_StartPatch() -
 ***************************************************************************/
int DSP_StartPatch(dsp_t *dsp, int sep, int pitch, int volume, int priority)
{
    channel_t *chan = NULL;
    int i, step, lvol, rvol;
    int handle;
    uint16_t fmt;
    uint32_t len;
    uint16_t freq;
    int samples;
    int dsp_f;
    int replace_idx;
    int lowest_priority;

    if (!dsp_init || !dsp)
        return -1;

    handle = (dsp_cnt++) & FXHAND_MASK;

    fmt = SwapLE16(dsp->format);
    len = SwapLE32(dsp->length);
    freq = SwapLE16(dsp->freq);

    if (fmt != 3 || len <= 32 || freq == 0)
        return -1;

    if (volume < 0)
        volume = 0;
    if (volume > 255)
        volume = 255;

    if (sep < 0)
        sep = 0;
    if (sep > 255)
        sep = 255;

    samples = (int)len - 32;
    if (samples <= 0)
        return -1;

    dsp_f = (dsp_freq > 0) ? dsp_freq : 11025;
    step = (int)((pitchtable[pitch & 255] * (uint32_t)freq + (uint32_t)(dsp_f / 2)) / (uint32_t)dsp_f);
    if (step <= 0)
        step = 1;

    lvol = (pantable[255 - (sep & 255)] * volume) / 256;
    rvol = (pantable[sep & 255] * volume) / 256;

    SND_Lock();

    for (i = 0; i < dsp_channelnum; i++) {
        if (dsp_channels[i].samples <= 0) {
            chan = &dsp_channels[i];
            break;
        }
    }

    if (!chan) {
        replace_idx = 0;
        lowest_priority = dsp_channels[0].priority;

        for (i = 1; i < dsp_channelnum; i++) {
            if (dsp_channels[i].priority < lowest_priority) {
                lowest_priority = dsp_channels[i].priority;
                replace_idx = i;
            }
        }

        if (priority < lowest_priority) {
            SND_Unlock();
            return -1;
        }

        chan = &dsp_channels[replace_idx];
    }

    DSP_VolTable(chan->voltable1, lvol);
    DSP_VolTable(chan->voltable2, rvol);

    /* DMX raw-PCM patch layout hypothesis: 8-byte header (format/freq/length)
     * + 16 bytes leading silence padding + real samples + 16 bytes trailing
     * padding. "length" field includes the 32 bytes of total padding (hence
     * samples = len - 32 above), but the real sample data starts right after
     * the 8-byte header + 16-byte leading pad = offset 24, not 32. */
    chan->data = ((char *)dsp) + 24;
    chan->samples = samples;
    chan->handle = handle;
    chan->priority = priority;
    chan->phase_inc = step >> 8;
    chan->phase_sub_inc = step & 255;
    chan->phase_sub = 0;
    chan->dsp = dsp;

    SND_Unlock();

    return handle | FXHAND_DSP;
}


/***************************************************************************
DSP_StopPatch() -
 ***************************************************************************/
void DSP_StopPatch(int handle)
{
    int i;

    handle &= FXHAND_MASK;

    SND_Lock();

    for (i = 0; i < dsp_channelnum; i++) {
        if (dsp_channels[i].samples > 0 && dsp_channels[i].handle == handle) {
            dsp_channels[i].samples = 0;
            dsp_channels[i].dsp = NULL;
            dsp_channels[i].data = NULL;
            dsp_channels[i].handle = -1;
            dsp_channels[i].priority = -2147483647 - 1;
            dsp_channels[i].phase_inc = 0;
            dsp_channels[i].phase_sub = 0;
            dsp_channels[i].phase_sub_inc = 0;
            break;
        }
    }

    SND_Unlock();
}