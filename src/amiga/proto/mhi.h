/* MHI (MPEG audio decoder driver) proto header for the m68k Amiga port
 * (GCC only).
 *
 * Follows the NDK proto-header convention used by this toolchain: pull in
 * the plain C prototypes, then the register-argument inline definitions,
 * and declare the library base pointer (defined by the user code).
 *
 * NOTE: MHIBase here is not a single system library - it is the base of
 * whichever MHI decoder driver the application opened (e.g.
 * "LIBS:MHI/prismamhi.library").  All drivers share this one interface. */

#ifndef PROTO_MHI_H
#define PROTO_MHI_H

#include <clib/mhi_protos.h>

#ifndef _NO_INLINE
# if defined(__GNUC__)
#  include <inline/mhi.h>
# else
#  error "proto/mhi.h: only GCC inline calls are supported by this port"
# endif
#endif /* _NO_INLINE */

#ifndef __NOLIBBASE__
extern struct Library *MHIBase;
#endif /* !__NOLIBBASE__ */

#endif /* !PROTO_MHI_H */
