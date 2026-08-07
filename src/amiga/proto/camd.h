/* CAMD (camd.library) proto header for the m68k Amiga port (GCC only).
 *
 * Follows the NDK proto-header convention used by this toolchain: pull in the
 * plain C prototypes, then the register-argument inline definitions, and
 * declare the library base pointer (defined by the user code). */

#ifndef PROTO_CAMD_H
#define PROTO_CAMD_H

#include <clib/camd_protos.h>

#ifndef _NO_INLINE
# if defined(__GNUC__)
#  include <inline/camd.h>
# else
#  error "proto/camd.h: only GCC inline calls are supported by this port"
# endif
#endif /* _NO_INLINE */

#ifndef __NOLIBBASE__
extern struct Library *CamdBase;
#endif /* !__NOLIBBASE__ */

#endif /* !PROTO_CAMD_H */
