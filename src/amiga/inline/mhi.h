/* MHI (MPEG audio decoder driver) GCC register-argument inlines for the
 * m68k Amiga port.
 *
 * Hand-written in the NDK <inline/macros.h> style (the same convention the
 * toolchain's ndk-include inline headers use), because the official MHI
 * developer kit (Aminet driver/audio/mhi_dev.lha) only ships SAS/C pragmas.
 *
 * LVO offsets (bias 30) and register assignments verified against the
 * official fd/mhi_lib.fd from the MHI developer kit:
 *
 *     MHIAllocDecoder(task,mhisignal)(a0,d0)
 *     MHIFreeDecoder(handle)(a3)
 *     MHIQueueBuffer(handle,buffer,size)(a3,a0,d0)
 *     MHIGetEmpty(handle)(a3)
 *     MHIGetStatus(handle)(a3)
 *     MHIPlay(handle)(a3)
 *     MHIStop(handle)(a3)
 *     MHIPause(handle)(a3)
 *     MHIQuery(query)(d1)
 *     MHISetParam(handle,param,value)(a3,d0,d1)
 *
 * Only the functions used by the Raptor MHI music backend (mpumhi.cpp)
 * are defined here. */

#ifndef _INLINE_MHI_H
#define _INLINE_MHI_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef MHI_BASE_NAME
#define MHI_BASE_NAME MHIBase
#endif /* !MHI_BASE_NAME */

#define MHIAllocDecoder(___task, ___mhisignal) \
      LP2(0x1e, APTR, MHIAllocDecoder , struct Task *, ___task, a0, ULONG, ___mhisignal, d0,\
      , MHI_BASE_NAME)

#define MHIFreeDecoder(___handle) \
      LP1NR(0x24, MHIFreeDecoder , APTR, ___handle, a3,\
      , MHI_BASE_NAME)

#define MHIQueueBuffer(___handle, ___buffer, ___size) \
      LP3(0x2a, BOOL, MHIQueueBuffer , APTR, ___handle, a3, APTR, ___buffer, a0, ULONG, ___size, d0,\
      , MHI_BASE_NAME)

#define MHIGetEmpty(___handle) \
      LP1(0x30, APTR, MHIGetEmpty , APTR, ___handle, a3,\
      , MHI_BASE_NAME)

#define MHIGetStatus(___handle) \
      LP1(0x36, UBYTE, MHIGetStatus , APTR, ___handle, a3,\
      , MHI_BASE_NAME)

#define MHIPlay(___handle) \
      LP1NR(0x3c, MHIPlay , APTR, ___handle, a3,\
      , MHI_BASE_NAME)

#define MHIStop(___handle) \
      LP1NR(0x42, MHIStop , APTR, ___handle, a3,\
      , MHI_BASE_NAME)

#define MHIPause(___handle) \
      LP1NR(0x48, MHIPause , APTR, ___handle, a3,\
      , MHI_BASE_NAME)

#define MHIQuery(___query) \
      LP1(0x4e, ULONG, MHIQuery , ULONG, ___query, d1,\
      , MHI_BASE_NAME)

#define MHISetParam(___handle, ___param, ___value) \
      LP3NR(0x54, MHISetParam , APTR, ___handle, a3, UWORD, ___param, d0, ULONG, ___value, d1,\
      , MHI_BASE_NAME)

#endif /* _INLINE_MHI_H */
