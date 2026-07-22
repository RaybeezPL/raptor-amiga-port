#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

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
    exit(1);
}

static inline void EXIT_Clean(void)
{
    fprintf(stderr, "[EXIT_Clean] Normal exit requested.\n");
    fflush(stderr);
    if (g_exit_shutdown_func)
        g_exit_shutdown_func(0);
    exit(0);
}

static inline void EXIT_Install(void (*func)(int))
{
    g_exit_shutdown_func = func;
}
