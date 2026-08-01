#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#ifdef __AMIGA__
#include <proto/dos.h>   /* Exit() - terminate process at OS level */
#endif


/* Global shutdown callback - set by EXIT_Install, called before exit */
typedef void (*exit_shutdown_func_t)(int);
#ifdef __cplusplus
extern "C" {
#endif
/* Defined once in rap.cpp (or wherever main lives) */
extern exit_shutdown_func_t g_exit_shutdown_func;
#ifdef __cplusplus
}
#endif

static inline void EXIT_Error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "\n*** EXIT_Error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, " ***\n");
    fflush(stderr);
    fflush(stdout);
    va_end(args);
    if (g_exit_shutdown_func)
        g_exit_shutdown_func(1);
#ifdef __AMIGA__
    /* On Amiga, terminate via dos.library Exit() instead of the C
     * library exit(): the C runtime teardown (stdio/malloc arena
     * cleanup) trips on heap corruption accumulated during the game
     * run and throws a recoverable alert (AN_FreeTwice 0100 0009).
     * dos Exit() reclaims all process resources at the OS level
     * without walking the malloc arena. */
    Exit(1);
#else
    exit(1);
#endif
}

static inline void EXIT_Clean(void)
{
    if (g_exit_shutdown_func)
        g_exit_shutdown_func(0);
#ifdef __AMIGA__
    Exit(0);   /* see EXIT_Error above */
#else
    exit(0);
#endif
}



static inline void EXIT_Install(void (*func)(int))
{
    g_exit_shutdown_func = func;
}
