/*
 * amiga_sdl_stubs.h - Minimal SDL2 type/macro stubs for AmigaOS 3.x port
 *
 * This header provides just enough SDL2 definitions to allow the Raptor
 * source code to compile without a real SDL2 library. Actual functionality
 * must be implemented via native AmigaOS APIs in the amiga_*.cpp files.
 *
 * This file is only used when USE_SDL_STUBS is defined (i.e., when no
 * real SDL2 Amiga port is available).
 */

#ifndef AMIGA_SDL_STUBS_H
#define AMIGA_SDL_STUBS_H

#ifdef USE_SDL_STUBS

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ========================================================================= */
/* AmigaOS Intuition / Graphics for real window support                      */
/* ========================================================================= */

#ifdef __AMIGA__
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>

/* Library bases needed by proto header inline stubs.                         */
/* __attribute__((weak)): safe if this header is included from multiple TUs.  */
__attribute__((weak)) struct IntuitionBase *IntuitionBase = NULL;
__attribute__((weak)) struct GfxBase *GfxBase = NULL;
#endif /* __AMIGA__ */

/* ========================================================================= */
/* Byte order / Endianness                                                   */
/* ========================================================================= */

#define SDL_LIL_ENDIAN  1234
#define SDL_BIG_ENDIAN  4321

/* Motorola 68k is Big Endian */
#define SDL_BYTEORDER   SDL_BIG_ENDIAN

/* Byte-swap functions for Big Endian reading Little Endian data */
static inline uint16_t SDL_Swap16(uint16_t x)
{
    return (uint16_t)((x << 8) | (x >> 8));
}

static inline uint32_t SDL_Swap32(uint32_t x)
{
    return ((x << 24) |
            ((x << 8) & 0x00FF0000u) |
            ((x >> 8) & 0x0000FF00u) |
            (x >> 24));
}

/* On Big Endian: SwapLE must byte-swap, SwapBE is no-op */
#define SDL_SwapLE16(x) SDL_Swap16(x)
#define SDL_SwapLE32(x) SDL_Swap32(x)
#define SDL_SwapBE16(x) (x)
#define SDL_SwapBE32(x) (x)

/* ========================================================================= */
/* Basic SDL types                                                           */
/* ========================================================================= */

typedef uint8_t   Uint8;
typedef int8_t    Sint8;
typedef uint16_t  Uint16;
typedef int16_t   Sint16;
typedef uint32_t  Uint32;
typedef int32_t   Sint32;
typedef int64_t   Sint64;
typedef int       SDL_bool;

#define SDL_FALSE 0
#define SDL_TRUE  1

/* ========================================================================= */
/* SDL version macros                                                        */
/* ========================================================================= */

#define SDL_MAJOR_VERSION 2
#define SDL_MINOR_VERSION 0
#define SDL_PATCHLEVEL    5

#define SDL_VERSION_ATLEAST(x, y, z) \
    ((SDL_MAJOR_VERSION > (x)) || \
     (SDL_MAJOR_VERSION == (x) && SDL_MINOR_VERSION > (y)) || \
     (SDL_MAJOR_VERSION == (x) && SDL_MINOR_VERSION == (y) && SDL_PATCHLEVEL >= (z)))

/* ========================================================================= */
/* SDL_Init subsystem flags (stubs)                                          */
/* ========================================================================= */

#define SDL_INIT_TIMER          0x00000001u
#define SDL_INIT_AUDIO          0x00000010u
#define SDL_INIT_VIDEO          0x00000020u
#define SDL_INIT_JOYSTICK       0x00000200u
#define SDL_INIT_HAPTIC         0x00001000u
#define SDL_INIT_GAMECONTROLLER 0x00002000u
#define SDL_INIT_EVENTS         0x00004000u

/* ========================================================================= */
/* Audio format constants                                                    */
/* ========================================================================= */

#define AUDIO_S16SYS  0x8010  /* Signed 16-bit, system byte order */
#define SDL_AUDIO_ALLOW_FREQUENCY_CHANGE 0x00000001

typedef uint32_t SDL_AudioDeviceID;

typedef struct SDL_AudioSpec {
    int freq;
    uint16_t format;
    uint8_t channels;
    uint8_t silence;
    uint16_t samples;
    uint32_t size;
    void (*callback)(void *userdata, uint8_t *stream, int len);
    void *userdata;
} SDL_AudioSpec;

/* ========================================================================= */
/* Video structures                                                          */
/* Now with real struct bodies so we can hold Amiga window pointers.          */
/* ========================================================================= */

typedef struct SDL_Window {
    int w, h;
#ifdef __AMIGA__
    struct Window *amiga_window;   /* Real Intuition window pointer */
#endif
} SDL_Window;

typedef struct SDL_Renderer {
    SDL_Window *window;            /* Back-pointer to owning window */
} SDL_Renderer;

typedef struct SDL_Texture {
    int w, h;
} SDL_Texture;

typedef struct SDL_Color {
    uint8_t r, g, b, a;
} SDL_Color;

typedef struct SDL_Rect {
    int x, y, w, h;
} SDL_Rect;

typedef struct SDL_Palette {
    int ncolors;
    SDL_Color *colors;
} SDL_Palette;

typedef struct SDL_PixelFormat {
    uint32_t format;
    SDL_Palette *palette;
    uint8_t BitsPerPixel;
    uint8_t BytesPerPixel;
    uint32_t Rmask, Gmask, Bmask, Amask;
} SDL_PixelFormat;

typedef struct SDL_Surface {
    uint32_t flags;
    SDL_PixelFormat *format;    /* Proper type, not void* */
    int w, h;
    int pitch;
    void *pixels;
} SDL_Surface;

typedef struct SDL_DisplayMode {
    uint32_t format;
    int w, h;
    int refresh_rate;
    void *driverdata;
} SDL_DisplayMode;

typedef struct SDL_RendererInfo {
    const char *name;
    uint32_t flags;
    uint32_t num_texture_formats;
    uint32_t texture_formats[16];
    int max_texture_width;
    int max_texture_height;
} SDL_RendererInfo;

/* Window flags */
#define SDL_WINDOW_FULLSCREEN          0x00000001u
#define SDL_WINDOW_SHOWN               0x00000004u
#define SDL_WINDOW_RESIZABLE           0x00000020u
#define SDL_WINDOW_FULLSCREEN_DESKTOP  0x00001001u
#define SDL_WINDOW_ALLOW_HIGHDPI       0x00002000u
#define SDL_WINDOW_BORDERLESS          0x00000010u

/* Window position */
#define SDL_WINDOWPOS_UNDEFINED        0x1FFF0000u
#define SDL_WINDOWPOS_CENTERED         0x2FFF0000u

/* Renderer flags */
#define SDL_RENDERER_SOFTWARE          0x00000001u
#define SDL_RENDERER_ACCELERATED       0x00000002u
#define SDL_RENDERER_PRESENTVSYNC      0x00000004u
#define SDL_RENDERER_TARGETTEXTURE     0x00000008u

/* Texture access */
#define SDL_TEXTUREACCESS_STATIC    0
#define SDL_TEXTUREACCESS_STREAMING 1
#define SDL_TEXTUREACCESS_TARGET    2

/* Pixel format enum */
#define SDL_PIXELFORMAT_ARGB8888  0x16362004u
#define SDL_PIXELFORMAT_RGBA8888  0x16462004u
#define SDL_PIXELFORMAT_RGB888    0x16161804u
#define SDL_PIXELFORMAT_INDEX8    0x13000001u

#define SDL_ALPHA_OPAQUE 255

/* ========================================================================= */
/* Event structures (stubs)                                                  */
/* ========================================================================= */

/* Event types */
#define SDL_QUIT                 0x100
#define SDL_KEYDOWN              0x300
#define SDL_KEYUP                0x301
#define SDL_MOUSEMOTION          0x400
#define SDL_MOUSEBUTTONDOWN      0x401
#define SDL_MOUSEBUTTONUP        0x402
#define SDL_MOUSEWHEEL           0x403
#define SDL_CONTROLLERAXISMOTION 0x650
#define SDL_CONTROLLERBUTTONDOWN 0x651
#define SDL_CONTROLLERBUTTONUP   0x652
#define SDL_CONTROLLERDEVICEADDED   0x653
#define SDL_CONTROLLERDEVICEREMOVED 0x654
#define SDL_FINGERDOWN           0x700
#define SDL_FINGERUP             0x701
#define SDL_WINDOWEVENT          0x200

/* Window events */
#define SDL_WINDOWEVENT_EXPOSED     3
#define SDL_WINDOWEVENT_MOVED       4
#define SDL_WINDOWEVENT_RESIZED     5
#define SDL_WINDOWEVENT_MINIMIZED   6
#define SDL_WINDOWEVENT_MAXIMIZED   7
#define SDL_WINDOWEVENT_RESTORED    8
#define SDL_WINDOWEVENT_FOCUS_GAINED 12
#define SDL_WINDOWEVENT_FOCUS_LOST   13

/* Keysym / Scancode */
#define SDL_SCANCODE_RETURN     40
#define SDL_SCANCODE_KP_ENTER   88
#define SDL_SCANCODE_LCTRL      224
#define SDL_SCANCODE_RCTRL      228
#define SDL_SCANCODE_LSHIFT     225
#define SDL_SCANCODE_RSHIFT     229
#define SDL_SCANCODE_LALT       226
#define SDL_SCANCODE_RALT       230
#define SDL_SCANCODE_AC_BACK    270

#define KMOD_LALT   0x0100
#define KMOD_RALT   0x0200
#define KMOD_LGUI   0x0400
#define KMOD_RGUI   0x0800

typedef struct SDL_Keysym {
    int scancode;
    int sym;
    uint16_t mod;
} SDL_Keysym;

typedef struct SDL_KeyboardEvent {
    uint32_t type;
    SDL_Keysym keysym;
} SDL_KeyboardEvent;

typedef struct SDL_WindowEvent {
    uint32_t type;
    uint32_t windowID;
    uint8_t event;
    int data1, data2;
} SDL_WindowEvent;

typedef struct SDL_MouseButtonEvent {
    uint32_t type;
    uint8_t button;
    uint8_t state;
    int x, y;
} SDL_MouseButtonEvent;

typedef struct SDL_MouseWheelEvent {
    uint32_t type;
    int x, y;
} SDL_MouseWheelEvent;

typedef struct SDL_ControllerButtonEvent {
    uint32_t type;
    uint8_t button;
    uint8_t state;
} SDL_ControllerButtonEvent;

typedef struct SDL_ControllerAxisEvent {
    uint32_t type;
    uint8_t axis;
    int16_t value;
} SDL_ControllerAxisEvent;

typedef struct SDL_TouchFingerEvent {
    uint32_t type;
    int64_t touchId;
    int64_t fingerId;
    float x, y, dx, dy, pressure;
} SDL_TouchFingerEvent;

typedef union SDL_Event {
    uint32_t type;
    SDL_KeyboardEvent key;
    SDL_WindowEvent window;
    SDL_MouseButtonEvent button;
    SDL_MouseWheelEvent wheel;
    SDL_ControllerButtonEvent cbutton;
    SDL_ControllerAxisEvent caxis;
    SDL_TouchFingerEvent tfinger;
} SDL_Event;

/* ========================================================================= */
/* Gamecontroller / Haptic stubs                                             */
/* ========================================================================= */

typedef struct SDL_GameController SDL_GameController;
typedef struct SDL_Haptic SDL_Haptic;

/* Controller buttons */
#define SDL_CONTROLLER_BUTTON_A             0
#define SDL_CONTROLLER_BUTTON_B             1
#define SDL_CONTROLLER_BUTTON_X             2
#define SDL_CONTROLLER_BUTTON_Y             3
#define SDL_CONTROLLER_BUTTON_BACK          4
#define SDL_CONTROLLER_BUTTON_START         6
#define SDL_CONTROLLER_BUTTON_LEFTSTICK     7
#define SDL_CONTROLLER_BUTTON_RIGHTSTICK    8
#define SDL_CONTROLLER_BUTTON_LEFTSHOULDER  9
#define SDL_CONTROLLER_BUTTON_RIGHTSHOULDER 10
#define SDL_CONTROLLER_BUTTON_DPAD_UP       11
#define SDL_CONTROLLER_BUTTON_DPAD_DOWN     12
#define SDL_CONTROLLER_BUTTON_DPAD_LEFT     13
#define SDL_CONTROLLER_BUTTON_DPAD_RIGHT    14

/* Controller axes */
#define SDL_CONTROLLER_AXIS_LEFTX           0
#define SDL_CONTROLLER_AXIS_LEFTY           1
#define SDL_CONTROLLER_AXIS_RIGHTX          2
#define SDL_CONTROLLER_AXIS_RIGHTY          3
#define SDL_CONTROLLER_AXIS_TRIGGERLEFT     4
#define SDL_CONTROLLER_AXIS_TRIGGERRIGHT    5

/* Controller types */
typedef enum {
    SDL_CONTROLLER_TYPE_UNKNOWN = 0,
    SDL_CONTROLLER_TYPE_XBOX360,
    SDL_CONTROLLER_TYPE_XBOXONE,
    SDL_CONTROLLER_TYPE_PS3,
    SDL_CONTROLLER_TYPE_PS4,
    SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO,
    SDL_CONTROLLER_TYPE_PS5
} SDL_GameControllerType;

/* ========================================================================= */
/* SDL Hint constants                                                        */
/* ========================================================================= */

#define SDL_HINT_RENDER_SCALE_QUALITY "SDL_RENDER_SCALE_QUALITY"
#define SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING "SDL_WINDOWS_DISABLE_THREAD_NAMING"

/* ========================================================================= */
/* Utility macros                                                            */
/* ========================================================================= */

#define SDL_max(a, b) ((a) > (b) ? (a) : (b))
#define SDL_min(a, b) ((a) < (b) ? (a) : (b))

/* ========================================================================= */
/* SDL_messagebox constants                                                  */
/* ========================================================================= */

#define SDL_MESSAGEBOX_ERROR       0x00000010u
#define SDL_MESSAGEBOX_WARNING     0x00000020u
#define SDL_MESSAGEBOX_INFORMATION 0x00000040u

/* ========================================================================= */
/* putenv compatibility for noixemul                                         */
/* ========================================================================= */

/* noixemul may not provide putenv; stub it out for Amiga */
#ifdef __AMIGA__
#ifndef putenv
static inline int putenv(char *string) { (void)string; return 0; }
#endif
#endif

/* ========================================================================= */
/* SDL function stubs - TO BE IMPLEMENTED in amiga_*.cpp                     */
/*                                                                           */
/* These are declared as static inline so that the Amiga                     */
/* implementation files can provide the real versions later.                  */
/* ========================================================================= */

#ifdef __cplusplus
extern "C" {
#endif

/* --- Init / Quit --- */
static inline int    SDL_Init(uint32_t flags) { (void)flags; return 0; }
static inline void   SDL_QuitSubSystem(uint32_t flags) { (void)flags; }

static inline void SDL_Quit(void) {
#ifdef __AMIGA__
    printf("[AMIGA] SDL_Quit: closing libraries\n"); fflush(stdout);
    if (IntuitionBase) {
        CloseLibrary((struct Library *)IntuitionBase);
        IntuitionBase = NULL;
    }
    if (GfxBase) {
        CloseLibrary((struct Library *)GfxBase);
        GfxBase = NULL;
    }
#endif
}

/* --- Error --- */
static inline const char* SDL_GetError(void) { return "SDL stubs - not implemented"; }

/* --- Timer --- */
/*
 * SDL_GetTicks: returns milliseconds since first call.
 * Uses AmigaOS DateStamp() for a working (50Hz resolution) timer.
 * For production, replace with ReadEClock() for higher precision.
 */
#ifdef __AMIGA__
#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#endif

static inline uint32_t SDL_GetTicks(void) {
#ifdef __AMIGA__
    static int first_call = 1;
    static uint32_t base_ticks = 0;
    struct DateStamp ds;
    uint32_t now;

    DateStamp(&ds);
    /* ds.ds_Days * 86400000 would overflow, but we only need relative time */
    /* ds.ds_Minute is minutes since midnight, ds.ds_Tick is 1/50s ticks */
    now = (uint32_t)ds.ds_Minute * 60000u + (uint32_t)ds.ds_Tick * 20u;

    if (first_call) {
        base_ticks = now;
        first_call = 0;
    }
    return now - base_ticks;
#else
    /* Fallback for non-Amiga compilation testing */
    static uint32_t fake_ticks = 0;
    return fake_ticks++;
#endif
}

static inline void SDL_Delay(uint32_t ms) {
#ifdef __AMIGA__
    if (ms > 0) {
        /* Delay() takes ticks (1/50s = 20ms units) */
        uint32_t ticks = (ms + 19) / 20;
        if (ticks < 1) ticks = 1;
        Delay(ticks);
    }
#else
    (void)ms;
#endif
}

/* --- Hints --- */
static inline int    SDL_SetHint(const char *n, const char *v) { (void)n; (void)v; return 0; }

/* --- Display info --- */
static inline int    SDL_GetNumVideoDisplays(void) { return 1; }
static inline int    SDL_GetDisplayBounds(int idx, SDL_Rect *rect) {
    (void)idx;
    if (rect) { rect->x = 0; rect->y = 0; rect->w = 320; rect->h = 200; }
    return 0;
}
static inline int    SDL_GetCurrentDisplayMode(int idx, SDL_DisplayMode *mode) {
    (void)idx;
    if (mode) { mode->format = 0; mode->w = 320; mode->h = 200; mode->refresh_rate = 50; mode->driverdata = 0; }
    return 0;
}

/* ========================================================================= */
/* --- Window --- Real Amiga Intuition window implementation               */
/* ========================================================================= */

static inline SDL_Window* SDL_CreateWindow(const char *title, int x, int y,
                                           int w, int h, uint32_t flags) {
    (void)x; (void)y; (void)flags;

    printf("[AMIGA] SDL_CreateWindow: title='%s' size=%dx%d flags=0x%x\n",
           title ? title : "(null)", w, h, flags);
    fflush(stdout);

    SDL_Window *win = (SDL_Window*)calloc(1, sizeof(SDL_Window));
    if (!win) {
        printf("[AMIGA] SDL_CreateWindow: calloc FAILED!\n"); fflush(stdout);
        return NULL;
    }
    win->w = w > 0 ? w : 320;
    win->h = h > 0 ? h : 200;

#ifdef __AMIGA__
    /* Open intuition.library v39+ if not already open */
    if (!IntuitionBase) {
        IntuitionBase = (struct IntuitionBase *)OpenLibrary(
            (CONST_STRPTR)"intuition.library", 39);
        if (!IntuitionBase) {
            printf("[AMIGA] SDL_CreateWindow: Cannot open intuition.library v39!\n");
            fflush(stdout);
            free(win);
            return NULL;
        }
        printf("[AMIGA] Opened intuition.library v39 OK\n"); fflush(stdout);
    }

    printf("[AMIGA] Opening Intuition window %dx%d on Workbench...\n",
           win->w, win->h);
    fflush(stdout);

    win->amiga_window = OpenWindowTags(NULL,
        WA_Left,          0,
        WA_Top,           20,
        WA_InnerWidth,    (ULONG)win->w,
        WA_InnerHeight,   (ULONG)win->h,
        WA_Title,         (ULONG)(title ? title : "Raptor"),
        WA_DragBar,       TRUE,
        WA_DepthGadget,   TRUE,
        WA_CloseGadget,   TRUE,
        WA_Activate,      TRUE,
        WA_ReportMouse,   TRUE,
        WA_RMBTrap,       TRUE,
        WA_GimmeZeroZero, TRUE,
        WA_IDCMP,         IDCMP_CLOSEWINDOW | IDCMP_RAWKEY |
                          IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE,
        TAG_DONE);

    if (!win->amiga_window) {
        printf("[AMIGA] SDL_CreateWindow: OpenWindowTags FAILED!\n");
        fflush(stdout);
        free(win);
        return NULL;
    }

    printf("[AMIGA] Intuition window opened at %p (%dx%d inner)\n",
           (void*)win->amiga_window, win->w, win->h);
    fflush(stdout);
#endif /* __AMIGA__ */

    return win;
}

static inline void SDL_DestroyWindow(SDL_Window *w) {
    if (!w) return;
    printf("[AMIGA] SDL_DestroyWindow: %p\n", (void*)w); fflush(stdout);
#ifdef __AMIGA__
    if (w->amiga_window) {
        CloseWindow(w->amiga_window);
        w->amiga_window = NULL;
        printf("[AMIGA] Intuition window closed\n"); fflush(stdout);
    }
#endif
    free(w);
}

static inline uint32_t SDL_GetWindowID(SDL_Window *w) { (void)w; return 1; }
static inline uint32_t SDL_GetWindowFlags(SDL_Window *w) { (void)w; return 0; }
static inline int    SDL_GetWindowDisplayIndex(SDL_Window *w) { (void)w; return 0; }
static inline uint32_t SDL_GetWindowPixelFormat(SDL_Window *w) { (void)w; return SDL_PIXELFORMAT_ARGB8888; }

static inline void SDL_GetWindowSize(SDL_Window *w, int *pw, int *ph) {
    if (w) { if(pw) *pw = w->w; if(ph) *ph = w->h; }
    else   { if(pw) *pw = 320;  if(ph) *ph = 200;   }
}

static inline void SDL_SetWindowSize(SDL_Window *w, int ww, int hh) {
    if (w) { w->w = ww; w->h = hh; }
    /* TODO: ChangeWindowBox() to resize Amiga window */
}

static inline void SDL_SetWindowMinimumSize(SDL_Window *w, int mw, int mh) {
    (void)w; (void)mw; (void)mh;
}

static inline void SDL_SetWindowTitle(SDL_Window *w, const char *t) {
    if (!w || !t) return;
#ifdef __AMIGA__
    if (w->amiga_window) {
        /* (CONST_STRPTR)~0 = AmigaOS sentinel "don't change screen title" */
        SetWindowTitles(w->amiga_window, (CONST_STRPTR)t, (CONST_STRPTR)~0);
        printf("[AMIGA] SDL_SetWindowTitle: '%s'\n", t); fflush(stdout);
    }
#endif
}

static inline void SDL_SetWindowFullscreen(SDL_Window *w, uint32_t f) {
    (void)w; (void)f;
    /* TODO: Fullscreen toggle via screen mode change */
}

/* ========================================================================= */
/* --- Renderer --- Allocates real struct, links back to window             */
/* ========================================================================= */

static inline SDL_Renderer* SDL_CreateRenderer(SDL_Window *w, int idx, uint32_t flags) {
    (void)idx; (void)flags;
    printf("[AMIGA] SDL_CreateRenderer: window=%p idx=%d flags=0x%x\n",
           (void*)w, idx, flags);
    fflush(stdout);

    SDL_Renderer *r = (SDL_Renderer*)calloc(1, sizeof(SDL_Renderer));
    if (r) r->window = w;
    return r;
}

static inline void SDL_DestroyRenderer(SDL_Renderer *r) {
    printf("[AMIGA] SDL_DestroyRenderer: %p\n", (void*)r); fflush(stdout);
    free(r);
}

static inline int SDL_GetRendererInfo(SDL_Renderer *r, SDL_RendererInfo *info) {
    (void)r;
    if (info) {
        info->name = "amiga_stub";
        info->flags = 0;
        info->max_texture_width = 2048;
        info->max_texture_height = 2048;
    }
    return 0;
}

static inline int SDL_GetRendererOutputSize(SDL_Renderer *r, int *w, int *h) {
    if (r && r->window) {
        if(w) *w = r->window->w;
        if(h) *h = r->window->h;
    } else {
        if(w) *w = 320;
        if(h) *h = 200;
    }
    return 0;
}

static inline int    SDL_RenderSetLogicalSize(SDL_Renderer *r, int w, int h) { (void)r; (void)w; (void)h; return 0; }
static inline int    SDL_RenderSetIntegerScale(SDL_Renderer *r, SDL_bool e) { (void)r; (void)e; return 0; }
static inline void   SDL_RenderClear(SDL_Renderer *r) { (void)r; }
static inline int    SDL_RenderCopy(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *s, const SDL_Rect *d) { (void)r; (void)t; (void)s; (void)d; return 0; }
static inline void   SDL_RenderPresent(SDL_Renderer *r) { (void)r; }
static inline int    SDL_SetRenderTarget(SDL_Renderer *r, SDL_Texture *t) { (void)r; (void)t; return 0; }
static inline int    SDL_SetRenderDrawColor(SDL_Renderer *r, uint8_t rr, uint8_t g, uint8_t b, uint8_t a) { (void)r; (void)rr; (void)g; (void)b; (void)a; return 0; }
static inline int    SDL_RenderGetViewport(SDL_Renderer *r, SDL_Rect *rect) {
    (void)r;
    if (rect) { rect->x = 0; rect->y = 0; rect->w = 320; rect->h = 200; }
    return 0;
}
static inline int    SDL_RenderGetScale(SDL_Renderer *r, float *sx, float *sy) {
    (void)r; if(sx)*sx=1.0f; if(sy)*sy=1.0f; return 0;
}

/* ========================================================================= */
/* --- Texture --- Allocates real struct with dimensions                    */
/* ========================================================================= */

static inline SDL_Texture* SDL_CreateTexture(SDL_Renderer *r, uint32_t f, int a, int w, int h) {
    (void)r; (void)f; (void)a;
    SDL_Texture *t = (SDL_Texture*)calloc(1, sizeof(SDL_Texture));
    if (t) { t->w = w; t->h = h; }
    return t;
}

static inline void SDL_DestroyTexture(SDL_Texture *t) { free(t); }

static inline int    SDL_UpdateTexture(SDL_Texture *t, const SDL_Rect *r, const void *p, int pi) { (void)t; (void)r; (void)p; (void)pi; return 0; }

/* --- Surface --- */
static inline SDL_Surface* SDL_CreateRGBSurface(uint32_t flags, int w, int h, int depth,
    uint32_t rm, uint32_t gm, uint32_t bm, uint32_t am) {
    (void)flags;
    SDL_Surface *s = (SDL_Surface*)calloc(1, sizeof(SDL_Surface));
    if (!s) return 0;
    s->w = w; s->h = h;
    s->pitch = w * ((depth + 7) / 8);
    s->pixels = calloc(1, (size_t)(s->pitch * h));
    /* Allocate a pixel format */
    SDL_PixelFormat *fmt = (SDL_PixelFormat*)calloc(1, sizeof(SDL_PixelFormat));
    if (fmt) {
        fmt->BitsPerPixel = (uint8_t)depth;
        fmt->BytesPerPixel = (uint8_t)((depth + 7) / 8);
        fmt->Rmask = rm; fmt->Gmask = gm; fmt->Bmask = bm; fmt->Amask = am;
        if (depth == 8) {
            /* Allocate palette for 8-bit surfaces */
            SDL_Palette *pal = (SDL_Palette*)calloc(1, sizeof(SDL_Palette));
            if (pal) {
                pal->ncolors = 256;
                pal->colors = (SDL_Color*)calloc(256, sizeof(SDL_Color));
            }
            fmt->palette = pal;
        }
    }
    s->format = fmt;
    return s;
}

static inline void SDL_FreeSurface(SDL_Surface *s) {
    if (!s) return;
    if (s->format) {
        if (s->format->palette) {
            free(s->format->palette->colors);
            free(s->format->palette);
        }
        free(s->format);
    }
    free(s->pixels);
    free(s);
}

static inline int SDL_FillRect(SDL_Surface *s, const SDL_Rect *r, uint32_t color) {
    (void)r; (void)color;
    if (s && s->pixels) memset(s->pixels, 0, (size_t)(s->pitch * s->h));
    return 0;
}

static inline int SDL_SetPaletteColors(SDL_Palette *p, const SDL_Color *c, int first, int n) {
    if (p && p->colors && c) {
        int i;
        for (i = 0; i < n && (first + i) < p->ncolors; ++i)
            p->colors[first + i] = c[i];
    }
    return 0;
}

static inline int SDL_LowerBlit(SDL_Surface *src, SDL_Rect *sr, SDL_Surface *dst, SDL_Rect *dr) {
    /*
     * Palette-indexed 8-bit surface → 32-bit ARGB surface conversion.
     * This is the critical blit path used by I_FinishUpdate() every frame.
     * src = 8-bit paletted screenbuffer, dst = 32-bit argbbuffer.
     */
    (void)dr; /* destination rect same as source rect for our usage */
    if (!src || !dst || !src->pixels || !dst->pixels) return -1;
    if (!src->format || !src->format->palette || !src->format->palette->colors) return -1;

    int x0 = 0, y0 = 0, w = src->w, h = src->h;
    if (sr) { x0 = sr->x; y0 = sr->y; w = sr->w; h = sr->h; }
    if (w > src->w) w = src->w;
    if (h > src->h) h = src->h;
    if (w > dst->w) w = dst->w;
    if (h > dst->h) h = dst->h;

    SDL_Color *colors = src->format->palette->colors;
    uint8_t *srcpix = (uint8_t*)src->pixels;
    uint32_t *dstpix = (uint32_t*)dst->pixels;
    int srcpitch = src->pitch;
    int dstpitch = dst->pitch / 4; /* pitch in uint32_t units */
    int x, y;

    for (y = 0; y < h; y++) {
        uint8_t  *sp = srcpix + (y0 + y) * srcpitch + x0;
        uint32_t *dp = dstpix + y * dstpitch;
        for (x = 0; x < w; x++) {
            uint8_t idx = sp[x];
            SDL_Color c = colors[idx];
            /* ARGB8888 layout: 0xAARRGGBB */
            dp[x] = (0xFFu << 24) | ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | (uint32_t)c.b;
        }
    }
    return 0;
}

static inline int SDL_PixelFormatEnumToMasks(uint32_t format, int *bpp,
    uint32_t *Rmask, uint32_t *Gmask, uint32_t *Bmask, uint32_t *Amask) {
    (void)format;
    if (bpp)   *bpp   = 32;
    if (Rmask) *Rmask = 0x00FF0000u;
    if (Gmask) *Gmask = 0x0000FF00u;
    if (Bmask) *Bmask = 0x000000FFu;
    if (Amask) *Amask = 0xFF000000u;
    return SDL_TRUE;
}

/* --- Audio --- */
/* TODO: Implement via AHI in amiga_audio.cpp */
static inline SDL_AudioDeviceID SDL_OpenAudioDevice(const char *d, int ic,
    const SDL_AudioSpec *desired, SDL_AudioSpec *obtained, int changes)
{
    (void)d; (void)ic; (void)desired; (void)obtained; (void)changes;
    return 0;
}
static inline void   SDL_CloseAudio(void) {}
static inline void   SDL_PauseAudioDevice(SDL_AudioDeviceID d, int p) { (void)d; (void)p; }
static inline void   SDL_PauseAudio(int p) { (void)p; }
static inline int    SDL_OpenAudio(const SDL_AudioSpec *d, SDL_AudioSpec *o) { (void)d; (void)o; return -1; }
static inline void   SDL_LockAudioDevice(SDL_AudioDeviceID d) { (void)d; }
static inline void   SDL_UnlockAudioDevice(SDL_AudioDeviceID d) { (void)d; }

/* --- Events --- */
static inline void   SDL_PumpEvents(void) {}
static inline int    SDL_PollEvent(SDL_Event *e) { (void)e; return 0; }

/* --- Touch --- */
static inline int    SDL_GetNumTouchFingers(int64_t touchId) { (void)touchId; return 0; }

/* --- Mouse button constants --- */
#define SDL_BUTTON_LEFT     1
#define SDL_BUTTON_MIDDLE   2
#define SDL_BUTTON_RIGHT    3

/* --- Mouse --- */
static inline int    SDL_SetRelativeMouseMode(SDL_bool e) { (void)e; return 0; }
static inline uint32_t SDL_GetRelativeMouseState(int *x, int *y) { if(x)*x=0; if(y)*y=0; return 0; }
static inline uint32_t SDL_GetMouseState(int *x, int *y) { if(x)*x=0; if(y)*y=0; return 0; }
static inline void   SDL_ShowCursor(int t) { (void)t; }
static inline void   SDL_WarpMouseInWindow(SDL_Window *w, int x, int y) { (void)w; (void)x; (void)y; }

/* --- MessageBox --- */
static inline int    SDL_ShowSimpleMessageBox(uint32_t f, const char *t, const char *m, SDL_Window *w)
    { (void)f; (void)t; (void)m; (void)w; return 0; }

/* --- Joystick/GameController --- */
static inline int    SDL_NumJoysticks(void) { return 0; }
static inline int    SDL_IsGameController(int i) { (void)i; return 0; }
static inline SDL_GameController* SDL_GameControllerOpen(int i) { (void)i; return 0; }
static inline void   SDL_GameControllerClose(SDL_GameController *g) { (void)g; }
static inline int    SDL_GameControllerGetAttached(SDL_GameController *g) { (void)g; return 0; }
static inline int16_t SDL_GameControllerGetAxis(SDL_GameController *g, int axis) { (void)g; (void)axis; return 0; }
static inline uint8_t SDL_GameControllerGetButton(SDL_GameController *g, int btn) { (void)g; (void)btn; return 0; }
static inline SDL_GameControllerType SDL_GameControllerTypeForIndex(int idx) { (void)idx; return SDL_CONTROLLER_TYPE_UNKNOWN; }
static inline int    SDL_GameControllerRumble(SDL_GameController *g, uint16_t lo, uint16_t hi, uint32_t ms) { (void)g; (void)lo; (void)hi; (void)ms; return -1; }

/* --- Haptic --- */
static inline SDL_Haptic* SDL_HapticOpen(int idx) { (void)idx; return 0; }
static inline SDL_Haptic* SDL_HapticOpenFromJoystick(void *j) { (void)j; return 0; }
static inline int    SDL_HapticRumbleInit(SDL_Haptic *h) { (void)h; return -1; }
static inline void   SDL_HapticClose(SDL_Haptic *h) { (void)h; }

/* --- Filesystem --- */
static inline char*  SDL_GetPrefPath(const char *org, const char *app) { (void)org; (void)app; return 0; }
static inline void   SDL_free(void *p) { (void)p; }

#ifdef __cplusplus
}
#endif

#endif /* USE_SDL_STUBS */
#endif /* AMIGA_SDL_STUBS_H */
