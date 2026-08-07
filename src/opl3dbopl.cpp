/***************************************************************************
 * opl3dbopl.cpp - OPL3_* API (opl3.h/Nuked-compatible) on top of the
 * DOSBox dbopl core.  Amiga/68060 build only (see opl3dbopl.h).
 ***************************************************************************/
#ifdef __AMIGA__

#include <string.h>
#include <new>

#include "opl3dbopl.h"

namespace DBOPL {
void InitTables(void); /* table generator from dbopl.cpp (idempotent) */
}

/***************************************************************************
 OPL3_Reset() - Reinitialise the chip to power-on state and set the
 * output sample rate (11025 Hz on the Amiga build).
 ***************************************************************************/
void OPL3_Reset(opl3_chip *chip, uint32_t samplerate)
{
    /* Reconstruct the chip for a clean power-on state, then (re)build the
     * static tables and the rate-dependent scalers.  InitTables() is
     * idempotent; Setup() runs once per reset (FPU math, init-time only). */
    chip->chip.~Chip();
    new (&chip->chip) DBOPL::Chip(true);

    DBOPL::InitTables();
    chip->chip.Setup(samplerate);
}

/***************************************************************************
 OPL3_WriteReg() -
 ***************************************************************************/
void OPL3_WriteReg(opl3_chip *chip, uint16_t reg, uint8_t v)
{
    chip->chip.WriteReg(reg, v);
}

/***************************************************************************
 OPL3_WriteRegBuffered() - dbopl register writes are cheap and immediate,
 * so there is nothing to buffer; the register numbering (including the
 * 0x100 second-array bit) matches the Nuked convention used by
 * i_oplmusic.cpp.
 ***************************************************************************/
void OPL3_WriteRegBuffered(opl3_chip *chip, uint16_t reg, uint8_t v)
{
    chip->chip.WriteReg(reg, v);
}

/***************************************************************************
 OPL3_GenerateStream() - Render interleaved stereo int16 samples.
 * dbopl outputs int32 with a per-channel peak of about +/-4084, so a x2
 * gain lands at roughly the loudness of the Nuked core; the clamp covers
 * multi-channel summing peaks.
 ***************************************************************************/
void OPL3_GenerateStream(opl3_chip *chip, int16_t *sndptr, uint32_t numsamples)
{
    /* Static scratch: this stream is produced only by the audio task. */
    static int32_t buf[512 * 2];

    while (numsamples > 0)
    {
        uint32_t chunk = numsamples > 512 ? 512 : numsamples;
        uint32_t i;

        chip->chip.GenerateBlock3(chunk, buf);

        for (i = 0; i < chunk * 2; i++)
        {
            int32_t s = buf[i] << 1;

            if (s > 32767)
                s = 32767;
            else if (s < -32768)
                s = -32768;

            sndptr[i] = (int16_t)s;
        }

        sndptr += chunk * 2;
        numsamples -= chunk;
    }
}
#endif /* __AMIGA__ */
