/*
 * amiga_stubs_impl.cpp - Single translation unit that owns (defines) all
 * shared Amiga globals declared extern in amiga_sdl_stubs.h.
 *
 * Every other TU that includes SDL.h / amiga_sdl_stubs.h sees "extern"
 * declarations and links against the single instance defined here.
 * This eliminates the per-TU copies that caused the joystick state
 * (AmigaJoyState) written by SDL_PumpEvents in i_video.cpp to be invisible
 * to SDL_GameControllerGetButton/Axis calls in joyapi.cpp/input.cpp.
 */

#define AMIGA_STUBS_OWNER   /* trigger definitions, not extern declarations */
#define USE_SDL_STUBS
#include "amiga_sdl_stubs.h"
