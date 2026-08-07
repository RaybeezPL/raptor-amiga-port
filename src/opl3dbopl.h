/***************************************************************************
 * opl3dbopl.h - drop-in replacement for opl3.h (Nuked OPL3) built on the
 * lightweight DOSBox dbopl core, for the Amiga/68060 build.
 *
 * Why: Nuked OPL3 always steps the chip at its fixed internal rate
 * (~49716 Hz) regardless of the output rate, which costs most of a real
 * 68060's CPU.  dbopl generates directly at the output rate (11025 Hz
 * here), which is only a few percent of the CPU.  Sound character is the
 * classic DOSBox AdLib/OPL3 - period-authentic for this game.
 *
 * Only the three symbols used by i_oplmusic.cpp are provided, with the
 * same signatures as opl3.h.
 ***************************************************************************/
#ifndef OPL3_DBOPL_SHIM_H
#define OPL3_DBOPL_SHIM_H

#include <stdint.h>

#include "dbopl.h"

/* Compatible-by-name replacement for the Nuked opl3_chip; wraps the
 * dbopl core in OPL3 mode (18 channels, stereo). */
struct opl3_chip {
    DBOPL::Chip chip;
    opl3_chip() : chip(true) {}
};

void OPL3_Reset(opl3_chip *chip, uint32_t samplerate);
void OPL3_WriteReg(opl3_chip *chip, uint16_t reg, uint8_t v);
void OPL3_WriteRegBuffered(opl3_chip *chip, uint16_t reg, uint8_t v);
void OPL3_GenerateStream(opl3_chip *chip, int16_t *sndptr, uint32_t numsamples);

#endif /* OPL3_DBOPL_SHIM_H */
