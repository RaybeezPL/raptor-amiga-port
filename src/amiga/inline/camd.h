/* CAMD (camd.library) GCC register-argument inlines for the m68k Amiga port.
 *
 * Hand-written in the NDK <inline/macros.h> style (the same convention the
 * toolchain's ndk-include inline headers use), because the available CAMD
 * developer kits only ship SAS/C pragmas or MorphOS PPC inlines, which GCC
 * for m68k cannot use.
 *
 * LVO offsets (bias 30) and register assignments verified against the
 * official fd/camd_lib.fd from the CAMD developer kit, cross-checked with
 * the MorphOS dev kit inline offsets.
 *
 * Only the functions used by the Raptor CAMD music backend (mpucamd.cpp)
 * are defined here. */

#ifndef _INLINE_CAMD_H
#define _INLINE_CAMD_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef CAMD_BASE_NAME
#define CAMD_BASE_NAME CamdBase
#endif /* !CAMD_BASE_NAME */

#define CreateMidiA(___tags) \
      LP1(0x2a, struct MidiNode *, CreateMidiA , CONST struct TagItem *, ___tags, a0,\
      , CAMD_BASE_NAME)

#define DeleteMidi(___mi) \
      LP1NR(0x30, DeleteMidi , struct MidiNode *, ___mi, a0,\
      , CAMD_BASE_NAME)

#define FlushMidi(___mi) \
      LP1NR(0x4e, FlushMidi , struct MidiNode *, ___mi, a0,\
      , CAMD_BASE_NAME)

#define AddMidiLinkA(___mi, ___type, ___tags) \
      LP3(0x54, struct MidiLink *, AddMidiLinkA , struct MidiNode *, ___mi, a0, LONG, ___type, d0, CONST struct TagItem *, ___tags, a1,\
      , CAMD_BASE_NAME)

#define RemoveMidiLink(___ml) \
      LP1NR(0x5a, RemoveMidiLink , struct MidiLink *, ___ml, a0,\
      , CAMD_BASE_NAME)

#define MidiLinkConnected(___ml) \
      LP1(0x78, BOOL, MidiLinkConnected , struct MidiLink *, ___ml, a0,\
      , CAMD_BASE_NAME)

#define PutMidi(___ml, ___msgdata) \
      LP2NR(0x8a, PutMidi , struct MidiLink *, ___ml, a0, LONG, ___msgdata, d0,\
      , CAMD_BASE_NAME)

#define GetMidiErr(___mi) \
      LP1(0xb4, UBYTE, GetMidiErr , struct MidiNode *, ___mi, a0,\
      , CAMD_BASE_NAME)

#endif /* _INLINE_CAMD_H */
