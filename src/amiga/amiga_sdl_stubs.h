/*
 * amiga_sdl_stubs.h - Minimal SDL2 type/macro stubs for AmigaOS 3.x port.
 *
 * Provides enough SDL2 definitions to compile Raptor without a real SDL2.
 * Active only when USE_SDL_STUBS is defined.
 *
 * Targeting: RTG (Picasso96) boards exclusively.  Game opens its own
 * 320x200x8 custom screen (never Workbench).
 *
 * RTG strategy:
 *  - Picasso96API.library opened by name to detect RTG; no p96*() calls.
 *  - Screen/window opened via standard BestModeID()/OpenScreenTags()/
 *    OpenWindowTags() which Picasso96 patches transparently.
 *  - Blit: WriteChunkyPixels() (gfx.lib v50+) with SetAPen/WritePixel
 *    fallback for older systems.
 *  - Palette: LoadRGB32() patched by Picasso96 for CLUT updates.
 *
 * Shared globals:
 *  All Amiga-specific globals (library bases, window/screen ptrs, joystick
 *  state, event queue, audio state) are declared extern here and defined
 *  exactly once in amiga_stubs_impl.cpp (AMIGA_STUBS_OWNER).
 *  This prevents per-TU copies that broke joystick state sharing between
 *  i_video.cpp (SDL_PumpEvents) and joyapi.cpp (GetButton/Axis).
 */

#ifndef AMIGA_SDL_STUBS_H
#define AMIGA_SDL_STUBS_H

#ifdef USE_SDL_STUBS

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* AmigaOS Intuition / Graphics for real screen+window support               */

#ifdef __AMIGA__
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/lowlevel.h>
#include <libraries/lowlevel.h>

/* -------------------------------------------------------------------------
 * Global storage pattern:
 *   AMIGA_STUBS_OWNER (defined in amiga_stubs_impl.cpp) -> actual definition
 *   All other TUs                                        -> extern declaration
 * This guarantees a single instance of every shared variable.
 * ------------------------------------------------------------------------- */
#ifdef AMIGA_STUBS_OWNER
#  define AMIGA_STUBS_DECL          /* plain definition */
#  define AMIGA_STUBS_INIT(v) = v   /* with initialiser */
#else
#  define AMIGA_STUBS_DECL    extern
#  define AMIGA_STUBS_INIT(v)       /* no initialiser in extern decl */
#endif

/* Library bases */
AMIGA_STUBS_DECL struct IntuitionBase *IntuitionBase AMIGA_STUBS_INIT(NULL);
AMIGA_STUBS_DECL struct GfxBase       *GfxBase       AMIGA_STUBS_INIT(NULL);
AMIGA_STUBS_DECL struct Library       *LowLevelBase  AMIGA_STUBS_INIT(NULL);

/* Joystick state - polled once per frame in SDL_PumpEvents(), read by
 * SDL_GameControllerGetButton/Axis() from any TU. Must be shared. */
AMIGA_STUBS_DECL ULONG AmigaJoyState AMIGA_STUBS_INIT(0);

/* Picasso96API.library - opened by name for RTG detection only.
 * Never called through (no LVO offsets needed). */
AMIGA_STUBS_DECL struct Library *P96Base      AMIGA_STUBS_INIT(NULL);
AMIGA_STUBS_DECL int             AmigaUsingP96 AMIGA_STUBS_INIT(0);

/* BestModeID() tag values from <graphics/displayinfo.h> - inlined so we
 * don't depend on that header being present. */
#ifndef BIDTAG_DesiredWidth
#define BIDTAG_DesiredWidth     (TAG_USER + 0x0000UL)
#endif
#ifndef BIDTAG_DesiredHeight
#define BIDTAG_DesiredHeight    (TAG_USER + 0x0001UL)
#endif
#ifndef BIDTAG_SourceID
#define BIDTAG_SourceID         (TAG_USER + 0x0002UL)
#endif
#ifndef BIDTAG_Depth
#define BIDTAG_Depth            (TAG_USER + 0x0004UL)
#endif
#ifndef BIDTAG_MonitorID
#define BIDTAG_MonitorID        (TAG_USER + 0x000AUL)
#endif
#ifndef INVALID_ID
#define INVALID_ID              0xFFFFFFFFUL
#endif

/* Game fixed native resolution - always 320x200x8 for RTG. */
#define AMIGA_GAME_WIDTH   320
#define AMIGA_GAME_HEIGHT  200
#define AMIGA_GAME_DEPTH   8

/* RTG screen / window - single instance shared across all TUs. */
AMIGA_STUBS_DECL struct Screen  *AmigaGameScreen    AMIGA_STUBS_INIT(NULL);
AMIGA_STUBS_DECL struct Window  *AmigaGameWindow    AMIGA_STUBS_INIT(NULL);

/* Pending chunky blit buffer set by SDL_LowerBlit(), consumed by
 * SDL_RenderPresent() - mirrors real SDL2 LowerBlit/Present flow. */
AMIGA_STUBS_DECL const uint8_t *AmigaPendingChunky AMIGA_STUBS_INIT(NULL);
AMIGA_STUBS_DECL int            AmigaPendingW       AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL int            AmigaPendingH       AMIGA_STUBS_INIT(0);

/*
 * Amiga_OpenP96: try to open Picasso96API.library, purely to detect whether
 * an RTG board/driver is present on this system. Returns 1 if RTG is
 * available, 0 otherwise (base AGA/ECS Intuition custom screen will be used
 * as a fallback in that case).
 */
static inline int Amiga_OpenP96(void)
{
    if (P96Base != NULL) {
        return 1;
    }

    printf("[AMIGA] Attempting to open Picasso96API.library (RTG detection)...\n");
    fflush(stdout);

    /* Raw OpenLibrary() call by name/version - no SDK header dependency. */
    P96Base = OpenLibrary((CONST_STRPTR)"Picasso96API.library", 0);

    if (P96Base) {
        printf("[AMIGA] Picasso96API.library opened OK - RTG board detected.\n");
        fflush(stdout);
        AmigaUsingP96 = 1;
        return 1;
    }

    printf("[AMIGA] Picasso96API.library NOT found - falling back to standard "
           "Intuition custom screen (no RTG acceleration).\n");
    fflush(stdout);
    AmigaUsingP96 = 0;
    return 0;
}

static inline void Amiga_CloseP96(void)
{
    if (P96Base) {
        printf("[AMIGA] Closing Picasso96API.library\n"); fflush(stdout);
        CloseLibrary(P96Base);
        P96Base = NULL;
    }
    AmigaUsingP96 = 0;
}

/*
 * Amiga_FindBestModeID: locate the best matching display ModeID for our
 * fixed 320x200x8 game resolution. On an RTG system with Picasso96 active,
 * BestModeID() will happily return one of the RTG board's own chunky
 * ModeIDs (Picasso96 registers these with graphics.library); on a plain
 * AGA/ECS system it returns a standard chipset ModeID instead.
 */
static inline ULONG Amiga_FindBestModeID(int w, int h, int depth)
{
    ULONG modeid = BestModeID(
        BIDTAG_DesiredWidth,  (ULONG)w,
        BIDTAG_DesiredHeight, (ULONG)h,
        BIDTAG_Depth,         (ULONG)depth,
        TAG_DONE);

    printf("[AMIGA] BestModeID(%dx%dx%d) -> 0x%08lx\n", w, h, depth,
           (unsigned long)modeid);
    fflush(stdout);

    return modeid;
}

/*
 * Amiga_OpenGameScreen: opens our dedicated custom 320x200x8-bit screen
 * (RTG-backed if Picasso96 is present, otherwise plain AGA/ECS chipset
 * screen at the closest matching mode). This screen is never Workbench -
 * we always create our own custom screen for the game.
 */
static inline struct Screen* Amiga_OpenGameScreen(int w, int h, int depth)
{
    ULONG modeid;

    if (AmigaGameScreen) {
        return AmigaGameScreen;
    }

    Amiga_OpenP96();

    modeid = Amiga_FindBestModeID(w, h, depth);
    if (modeid == INVALID_ID) {
        printf("[AMIGA] BestModeID() failed to find a matching mode! "
               "Trying INVALID_ID anyway (Intuition may pick a default).\n");
        fflush(stdout);
    }

    printf("[AMIGA] Opening custom %dx%dx%d game screen (ModeID=0x%08lx, "
           "RTG=%d)...\n", w, h, depth, (unsigned long)modeid, AmigaUsingP96);
    fflush(stdout);

    AmigaGameScreen = OpenScreenTags(NULL,
        SA_Width,      (ULONG)w,
        SA_Height,     (ULONG)h,
        SA_Depth,      (ULONG)depth,
        SA_DisplayID,  modeid,
        SA_Quiet,      TRUE,
        SA_ShowTitle,  FALSE,
        SA_Draggable,  FALSE,
        SA_Exclusive,  TRUE,
        SA_Type,       CUSTOMSCREEN,
        TAG_DONE);

    if (!AmigaGameScreen) {
        printf("[AMIGA] Amiga_OpenGameScreen: OpenScreenTags FAILED!\n");
        fflush(stdout);
        return NULL;
    }

    printf("[AMIGA] Custom game screen opened at %p\n", (void*)AmigaGameScreen);
    fflush(stdout);

    return AmigaGameScreen;
}

static inline void Amiga_CloseGameScreen(void)
{
    if (AmigaGameScreen) {
        printf("[AMIGA] Closing custom game screen %p\n", (void*)AmigaGameScreen);
        fflush(stdout);
        CloseScreen(AmigaGameScreen);
        AmigaGameScreen = NULL;
    }
}

/*
 * Amiga_BlitScreen: raw chunky pixel blit of an 8-bit paletted buffer
 * (I_VideoBuffer, 320x200) directly onto the game window's RastPort.
 *
 * Preferred path: WriteChunkyPixels() - a *standard* graphics.library call
 * (v50+, present whenever RTG software such as Picasso96/CyberGraphX has
 * patched graphics.library, which is always true on an RTG system). It
 * writes an 8-bit-per-pixel chunky array straight into the bitmap, exactly
 * matching our 320x200x8 chunky screen buffer - no palette conversion, no
 * intermediate ARGB surface required.
 *
 * Fallback path (graphics.library < v50 / WriteChunkyPixels unavailable):
 * a raw per-pixel copy via SetAPen()+WritePixel(). Slow, but keeps the game
 * functionally working on any system (e.g. plain AGA fallback).
 */
static inline void Amiga_BlitScreen(struct Window *win, const uint8_t *chunky)
{
    if (!win || !win->RPort || !chunky) return;

    WriteChunkyPixels(win->RPort,
                      win->BorderLeft, win->BorderTop,
                      win->BorderLeft + 319, win->BorderTop + 199,
                      (UBYTE *)chunky, 320);
}

/*
 * Amiga_ApplyPalette: pushes an SDL_Color[] palette (0-255 range per
 * channel) to the hardware/RTG screen using the standard graphics.library
 * LoadRGB32() call. Works correctly for both AGA and RTG (Picasso96)
 * screens - Picasso96 patches LoadRGB32() to update its own CLUT when the
 * ViewPort belongs to an RTG screen.
 */
static inline void Amiga_ApplyPalette(struct Screen *scr, const void *sdlcolors, int first, int n)
{
    struct RawColor { uint8_t r, g, b, a; };
    const struct RawColor *colors = (const struct RawColor *)sdlcolors;
    static ULONG table[1 + 256 * 3 + 1];
    int i;

    if (!scr || !colors || n <= 0 || first < 0 || first > 255) return;
    if (first + n > 256) n = 256 - first;
    if (n <= 0) return;

    table[0] = ((ULONG)n << 16) | (ULONG)first;
    for (i = 0; i < n; ++i)
    {
        table[1 + i * 3 + 0] = (ULONG)colors[i].r << 24;
        table[1 + i * 3 + 1] = (ULONG)colors[i].g << 24;
        table[1 + i * 3 + 2] = (ULONG)colors[i].b << 24;
    }
    table[1 + n * 3] = 0;

    LoadRGB32(&scr->ViewPort, table);
}

/*
 * Amiga_HideSystemPointer / Amiga_ShowSystemPointer: hide or restore the
 * native Amiga hardware sprite mouse pointer on our game window.
 */
static inline void Amiga_HideSystemPointer(void)
{
    if (!AmigaGameWindow) return;
    static UWORD emptyPointer[16]={0};
    SetPointer(AmigaGameWindow, emptyPointer, 1, 16, 0, 0);
}

static inline void Amiga_ShowSystemPointer(void)
{
    if (AmigaGameWindow) {
        ClearPointer(AmigaGameWindow);
    }
}

#endif /* __AMIGA__ (main AmigaOS block - library bases, globals, helper functions) */

/* Byte order / Endianness                                                   */

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

/* Basic SDL types                                                           */

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

/* SDL version macros                                                        */

#define SDL_MAJOR_VERSION 2
#define SDL_MINOR_VERSION 0
#define SDL_PATCHLEVEL    5

#define SDL_VERSION_ATLEAST(x, y, z) \
    ((SDL_MAJOR_VERSION > (x)) || \
     (SDL_MAJOR_VERSION == (x) && SDL_MINOR_VERSION > (y)) || \
     (SDL_MAJOR_VERSION == (x) && SDL_MINOR_VERSION == (y) && SDL_PATCHLEVEL >= (z)))

/* SDL_Init subsystem flags (stubs)                                          */

#define SDL_INIT_TIMER          0x00000001u
#define SDL_INIT_AUDIO          0x00000010u
#define SDL_INIT_VIDEO          0x00000020u
#define SDL_INIT_JOYSTICK       0x00000200u
#define SDL_INIT_HAPTIC         0x00001000u
#define SDL_INIT_GAMECONTROLLER 0x00002000u
#define SDL_INIT_EVENTS         0x00004000u

/* Audio format constants                                                    */

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

/* Video structures                                                          */
/* Now with real struct bodies so we can hold Amiga window/screen pointers.   */

typedef struct SDL_Window {
    int w, h;
#ifdef __AMIGA__
    struct Window *amiga_window;   /* Real Intuition window pointer */
    struct Screen *amiga_screen;   /* Dedicated custom RTG screen this window is on */
#endif
} SDL_Window;

typedef struct SDL_Renderer {
    SDL_Window *window;            /* Back-pointer to owning window */
    int logical_w, logical_h;      /* Set via SDL_RenderSetLogicalSize(); 0 = unset
                                     * (falls back to the real window/output size). */
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

/* Event structures (stubs)                                                  */

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

/* Keysym / Scancode
 *
 * These follow the standard SDL2 USB-HID-based scancode numbering (i.e. the
 * exact values real SDL2's SDL_SCANCODE_* enum uses). This matters because
 * src/kbdapi.cpp's I_HandleKeyboardEvent() / ScanCodeMap[] table indexes
 * directly into these numeric values (e.g. ScanCodeMap[4] == 'A' key), so
 * whatever produces SDL_Event key.keysym.scancode values (see the native
 * Amiga IDCMP raw-key translation further below) MUST emit these same
 * numbers for the existing keyboard code to work correctly.
 */
#define SDL_SCANCODE_A          4
#define SDL_SCANCODE_B          5
#define SDL_SCANCODE_C          6
#define SDL_SCANCODE_D          7
#define SDL_SCANCODE_E          8
#define SDL_SCANCODE_F          9
#define SDL_SCANCODE_G          10
#define SDL_SCANCODE_H          11
#define SDL_SCANCODE_I          12
#define SDL_SCANCODE_J          13
#define SDL_SCANCODE_K          14
#define SDL_SCANCODE_L          15
#define SDL_SCANCODE_M          16
#define SDL_SCANCODE_N          17
#define SDL_SCANCODE_O          18
#define SDL_SCANCODE_P          19
#define SDL_SCANCODE_Q          20
#define SDL_SCANCODE_R          21
#define SDL_SCANCODE_S          22
#define SDL_SCANCODE_T          23
#define SDL_SCANCODE_U          24
#define SDL_SCANCODE_V          25
#define SDL_SCANCODE_W          26
#define SDL_SCANCODE_X          27
#define SDL_SCANCODE_Y          28
#define SDL_SCANCODE_Z          29
#define SDL_SCANCODE_1          30
#define SDL_SCANCODE_2          31
#define SDL_SCANCODE_3          32
#define SDL_SCANCODE_4          33
#define SDL_SCANCODE_5          34
#define SDL_SCANCODE_6          35
#define SDL_SCANCODE_7          36
#define SDL_SCANCODE_8          37
#define SDL_SCANCODE_9          38
#define SDL_SCANCODE_0          39
#define SDL_SCANCODE_RETURN     40
#define SDL_SCANCODE_ESCAPE     41
#define SDL_SCANCODE_BACKSPACE  42
#define SDL_SCANCODE_TAB        43
#define SDL_SCANCODE_SPACE      44
#define SDL_SCANCODE_MINUS      45
#define SDL_SCANCODE_EQUALS     46
#define SDL_SCANCODE_LEFTBRACKET  47
#define SDL_SCANCODE_RIGHTBRACKET 48
#define SDL_SCANCODE_BACKSLASH  49
#define SDL_SCANCODE_SEMICOLON  51
#define SDL_SCANCODE_APOSTROPHE 52
#define SDL_SCANCODE_GRAVE      53
#define SDL_SCANCODE_COMMA      54
#define SDL_SCANCODE_PERIOD     55
#define SDL_SCANCODE_SLASH      56
#define SDL_SCANCODE_CAPSLOCK   57
#define SDL_SCANCODE_F1         58
#define SDL_SCANCODE_F2         59
#define SDL_SCANCODE_F3         60
#define SDL_SCANCODE_F4         61
#define SDL_SCANCODE_F5         62
#define SDL_SCANCODE_F6         63
#define SDL_SCANCODE_F7         64
#define SDL_SCANCODE_F8         65
#define SDL_SCANCODE_F9         66
#define SDL_SCANCODE_F10        67
#define SDL_SCANCODE_F11        68
#define SDL_SCANCODE_F12        69
#define SDL_SCANCODE_INSERT     73
#define SDL_SCANCODE_HOME       74
#define SDL_SCANCODE_PAGEUP     75
#define SDL_SCANCODE_DELETE     76
#define SDL_SCANCODE_END        77
#define SDL_SCANCODE_PAGEDOWN   78
#define SDL_SCANCODE_RIGHT      79
#define SDL_SCANCODE_LEFT       80
#define SDL_SCANCODE_DOWN       81
#define SDL_SCANCODE_UP         82
#define SDL_SCANCODE_KP_DIVIDE   84
#define SDL_SCANCODE_KP_MULTIPLY 85
#define SDL_SCANCODE_KP_MINUS   86
#define SDL_SCANCODE_KP_PLUS    87
#define SDL_SCANCODE_KP_ENTER   88
#define SDL_SCANCODE_KP_1       89
#define SDL_SCANCODE_KP_2       90
#define SDL_SCANCODE_KP_3       91
#define SDL_SCANCODE_KP_4       92
#define SDL_SCANCODE_KP_5       93
#define SDL_SCANCODE_KP_6       94
#define SDL_SCANCODE_KP_7       95
#define SDL_SCANCODE_KP_8       96
#define SDL_SCANCODE_KP_9       97
#define SDL_SCANCODE_KP_0       98
#define SDL_SCANCODE_KP_PERIOD  99
#define SDL_SCANCODE_LCTRL      224
#define SDL_SCANCODE_LSHIFT     225
#define SDL_SCANCODE_LALT       226
#define SDL_SCANCODE_LGUI       227
#define SDL_SCANCODE_RCTRL      228
#define SDL_SCANCODE_RSHIFT     229
#define SDL_SCANCODE_RALT       230
#define SDL_SCANCODE_RGUI       231
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

/* Shared Amiga globals that reference SDL_Event (placed after typedef)      */

#ifdef __AMIGA__
/* Event queue, mouse state - single instance owned by amiga_stubs_impl.cpp */
#define AMIGA_SDL_EVENT_QUEUE_SIZE 64
AMIGA_STUBS_DECL SDL_Event AmigaEventQueue[AMIGA_SDL_EVENT_QUEUE_SIZE];
AMIGA_STUBS_DECL int       AmigaEventQueueHead AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL int       AmigaEventQueueTail AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL int       AmigaMouseX         AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL int       AmigaMouseY         AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL int       AmigaMouseButtons   AMIGA_STUBS_INIT(0);
#endif /* __AMIGA__ */

/* Gamecontroller / Haptic stubs                                             */

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

/* SDL Hint constants                                                        */

#define SDL_HINT_RENDER_SCALE_QUALITY "SDL_RENDER_SCALE_QUALITY"
#define SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING "SDL_WINDOWS_DISABLE_THREAD_NAMING"

/* Utility macros                                                            */

#define SDL_max(a, b) ((a) > (b) ? (a) : (b))
#define SDL_min(a, b) ((a) < (b) ? (a) : (b))

/* putenv compatibility for noixemul                                         */

/* noixemul may not provide putenv; stub it out for Amiga */
#ifdef __AMIGA__
#ifndef putenv
static inline int putenv(char *string) { (void)string; return 0; }
#endif
#endif

/* SDL function stubs - TO BE IMPLEMENTED in amiga_*.cpp                     */
/*                                                                           */
/* These are declared as static inline so that the Amiga                     */
/* implementation files can provide the real versions later.                  */

#ifdef __cplusplus
extern "C" {
#endif

/* --- Init / Quit --- */
static inline int SDL_Init(uint32_t flags)
{
    (void)flags;

#ifdef __AMIGA__

    if (!LowLevelBase)
    {
        LowLevelBase = OpenLibrary((CONST_STRPTR)"lowlevel.library", 40);
        if (LowLevelBase)
        {
            printf("[AMIGA] SDL_Init: lowlevel.library v40 opened OK (joystick enabled)\n");
            fflush(stdout);
        }
        else
        {
            printf("[AMIGA] SDL_Init: lowlevel.library v40 NOT available (joystick disabled)\n");
            fflush(stdout);
        }
    }

#endif

    return 0;
}
static inline void   SDL_QuitSubSystem(uint32_t flags) { (void)flags; }

static inline void SDL_Quit(void) {
#ifdef __AMIGA__
    /* Idempotent guard: jeśli wszystkie biblioteki już zamknięte (np. przy
     * drugim wywołaniu przez atexit/EXIT_Clean po ShutDown()), wyjdź od razu.
     * Zapobiega podwójnemu zamknięciu bibliotek i podwójnemu printf. */
    if (!AmigaGameWindow && !AmigaGameScreen &&
        !IntuitionBase && !GfxBase && !LowLevelBase && !P96Base)
        return;

    /* Restore the native Amiga system pointer before destroying the game
     * window and closing libraries.  This is the single canonical "show"
     * call that matches the single "hide" call in SDL_CreateWindow(). */
    Amiga_ShowSystemPointer();
    printf("[AMIGA] SDL_Quit: system pointer restored, closing libraries\n"); fflush(stdout);
    Amiga_CloseGameScreen();
    Amiga_CloseP96();
    if (IntuitionBase) {
        CloseLibrary((struct Library *)IntuitionBase);
        IntuitionBase = NULL;
    }
    if (GfxBase) {
        CloseLibrary((struct Library *)GfxBase);
        GfxBase = NULL;
    }
    if (LowLevelBase)
    {
        CloseLibrary(LowLevelBase);
        LowLevelBase = NULL;
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

/* --- Window --- RTG (Picasso96) custom screen + borderless window          */
/*                                                                           */
/* Strictly targets RTG: opens a dedicated, custom 320x200x8 screen (never   */
/* Workbench) via Picasso96-if-present / plain Intuition-if-not, then a      */
/* borderless GimmeZeroZero window filling that screen for the game to draw  */
/* into. See file header comment for the full inline/raw Picasso96 rationale.*/

static inline SDL_Window* SDL_CreateWindow(const char *title, int x, int y,
                                           int w, int h, uint32_t flags) {
    (void)x; (void)y; (void)flags;

    printf("[AMIGA] SDL_CreateWindow: title='%s' requested size=%dx%d flags=0x%x\n",
           title ? title : "(null)", w, h, flags);
    fflush(stdout);

    SDL_Window *win = (SDL_Window*)calloc(1, sizeof(SDL_Window));
    if (!win) {
        printf("[AMIGA] SDL_CreateWindow: calloc FAILED!\n"); fflush(stdout);
        return NULL;
    }

    /* This port strictly targets our own 320x200 8-bit RTG screen,
     * regardless of what window size the generic i_video.cpp layer asked
     * for - the game's native resolution is fixed. */
    win->w = AMIGA_GAME_WIDTH;
    win->h = AMIGA_GAME_HEIGHT;

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

    /* graphics.library is needed for BestModeID()/WriteChunkyPixels()/LoadRGB32() */
    if (!GfxBase) {
        GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library", 39);
        if (!GfxBase) {
            printf("[AMIGA] SDL_CreateWindow: Cannot open graphics.library v39!\n");
            fflush(stdout);
            free(win);
            return NULL;
        }
        printf("[AMIGA] Opened graphics.library v39 OK (version=%u)\n",
               (unsigned)GfxBase->LibNode.lib_Version);
        fflush(stdout);
    }

    /* Force our own dedicated custom RTG screen - NEVER Workbench. */
    struct Screen *scr = Amiga_OpenGameScreen(AMIGA_GAME_WIDTH, AMIGA_GAME_HEIGHT, AMIGA_GAME_DEPTH);
    if (!scr) {
        printf("[AMIGA] SDL_CreateWindow: Amiga_OpenGameScreen FAILED!\n");
        fflush(stdout);
        free(win);
        return NULL;
    }
    win->amiga_screen = scr;

    printf("[AMIGA] Opening borderless %dx%d window on custom game screen...\n",
           win->w, win->h);
    fflush(stdout);

    win->amiga_window = OpenWindowTags(NULL,
        WA_Left,          0,
        WA_Top,           0,
        WA_Width,         (ULONG)win->w,
        WA_Height,        (ULONG)win->h,
        WA_InnerWidth,    (ULONG)win->w,
        WA_InnerHeight,   (ULONG)win->h,
        WA_CustomScreen,  (ULONG)scr,
        WA_Borderless,    TRUE,
        WA_Backdrop,      TRUE,
        WA_DragBar,       FALSE,
        WA_DepthGadget,   FALSE,
        WA_CloseGadget,   FALSE,
        WA_Activate,      TRUE,
        WA_RMBTrap,       TRUE,
        WA_ReportMouse,   TRUE,
        WA_GimmeZeroZero, TRUE,
        WA_IDCMP,         IDCMP_CLOSEWINDOW | IDCMP_RAWKEY |
                          IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE,
        TAG_DONE);

    if (!win->amiga_window) {
        printf("[AMIGA] SDL_CreateWindow: OpenWindowTags FAILED!\n");
        fflush(stdout);
        Amiga_CloseGameScreen();
        free(win);
        return NULL;
    }

    printf("[AMIGA] Borderless game window opened at %p (%dx%d inner) on RTG=%d screen\n",
           (void*)win->amiga_window, win->w, win->h, AmigaUsingP96);
    fflush(stdout);

    /* Remember this window at header scope so the native IDCMP event pump
     * (Amiga_PumpWindowEvents(), see the --- Events --- section below) can
     * find its IDCMP UserPort message port and MouseX/MouseY fields without
     * needing an SDL_Window* argument (SDL_PumpEvents() takes none). */
    AmigaGameWindow = win->amiga_window;

    /* Hide the system Amiga pointer immediately after the game window opens.
     * It remains hidden for the entire lifetime of the application and is
     * restored only in SDL_Quit() / ShutDown(). This is the single, canonical
     * hide call - SDL_SetRelativeMouseMode() is now a no-op for Amiga so it
     * cannot accidentally re-show the pointer when returning to the menu. */
    Amiga_HideSystemPointer();
    printf("[AMIGA] System pointer hidden (will be restored at SDL_Quit)\n");
    fflush(stdout);
#endif /* __AMIGA__ */

    return win;
}

static inline void SDL_DestroyWindow(SDL_Window *w) {
    if (!w) return;
    printf("[AMIGA] SDL_DestroyWindow: %p\n", (void*)w); fflush(stdout);
#ifdef __AMIGA__
    if (w->amiga_window) {
        if (AmigaGameWindow == w->amiga_window) {
            AmigaGameWindow = NULL;
        }
        CloseWindow(w->amiga_window);
        w->amiga_window = NULL;
        printf("[AMIGA] Game window closed\n"); fflush(stdout);
    }
    Amiga_CloseGameScreen();
    w->amiga_screen = NULL;
#endif
    free(w);
}

static inline uint32_t SDL_GetWindowID(SDL_Window *w) { (void)w; return 1; }
static inline uint32_t SDL_GetWindowFlags(SDL_Window *w) { (void)w; return 0; }
static inline int    SDL_GetWindowDisplayIndex(SDL_Window *w) { (void)w; return 0; }
/* Our window is always an 8-bit chunky/CLUT surface - report INDEX8. */
static inline uint32_t SDL_GetWindowPixelFormat(SDL_Window *w) { (void)w; return SDL_PIXELFORMAT_INDEX8; }

static inline void SDL_GetWindowSize(SDL_Window *w, int *pw, int *ph) {
    if (w) { if(pw) *pw = w->w; if(ph) *ph = w->h; }
    else   { if(pw) *pw = 320;  if(ph) *ph = 200;   }
}

static inline void SDL_SetWindowSize(SDL_Window *w, int ww, int hh) {
    (void)ww; (void)hh;
    /* Fixed 320x200 RTG game screen - resizing is a no-op by design. */
    if (w) { w->w = AMIGA_GAME_WIDTH; w->h = AMIGA_GAME_HEIGHT; }
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
    /* Already exclusive fullscreen on our own custom screen - no-op. */
}

/* --- Renderer --- Allocates real struct, links back to window             */

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
        info->name = "amiga_rtg";
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

/*
 * SDL_RenderSetLogicalSize: remembers the "logical" (game-space) render
 * resolution requested by i_video.cpp (SCREENWIDTH x actualheight, i.e.
 * 320x200 or 320x240 depending on aspect_ratio_correct). This is exactly
 * what real SDL2 does internally, and is essential for I_GetMousePos()/
 * I_SetMousePos() (src/i_video.cpp) to correctly translate between real
 * window pixel coordinates and the game's native 320x200 coordinate space
 * via SDL_RenderGetScale()/SDL_RenderGetViewport() below - previously
 * those two stubs ignored this entirely and always reported a hardcoded
 * scale of 1.0 / a fixed 320x200 viewport, which silently broke the
 * mouse-position math whenever logical_h != real window height (e.g. the
 * default aspect_ratio_correct=1 case, where logical height is 240 but
 * our fixed Amiga game window/screen is always the real native 200px
 * tall) - causing the reported mouse Y to be compressed/offset.
 */
static inline int    SDL_RenderSetLogicalSize(SDL_Renderer *r, int w, int h) {
    if (r) { r->logical_w = w; r->logical_h = h; }
    return 0;
}

static inline int    SDL_RenderSetIntegerScale(SDL_Renderer *r, SDL_bool e) { (void)r; (void)e; return 0; }
static inline void   SDL_RenderClear(SDL_Renderer *r) { (void)r; }
static inline int    SDL_RenderCopy(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *s, const SDL_Rect *d) { (void)r; (void)t; (void)s; (void)d; return 0; }

/*
 * SDL_RenderPresent: this is the per-frame "flip" call. On our RTG backend
 * there is no separate texture/renderer pipeline - the 8-bit chunky
 * screenbuffer (I_VideoBuffer, wrapped by the 8-bit SDL_Surface) is what
 * actually gets blitted onto the game window's RastPort, using either
 * WriteChunkyPixels() (preferred, standard graphics.library v50+ call, RTG
 * accelerated when Picasso96 is active) or the slow per-pixel fallback.
 *
 * SDL_LowerBlit() (called earlier this frame by I_FinishUpdate() in
 * i_video.cpp) has already cached a pointer to the source 8-bit chunky
 * buffer + dimensions in AmigaPendingChunky/W/H; here at "present" time we
 * actually push those pixels to the screen. This mirrors real SDL2
 * semantics (LowerBlit fills an intermediate surface, RenderPresent flips
 * it to the display) while doing the genuinely visible hardware blit at
 * the correct point in the frame.
 */
static inline void SDL_RenderPresent(SDL_Renderer *r) {
    (void)r;
#ifdef __AMIGA__
    if (AmigaGameScreen && AmigaGameScreen->FirstWindow &&
        AmigaPendingChunky && AmigaPendingW > 0 && AmigaPendingH > 0)
    {
        Amiga_BlitScreen(AmigaGameScreen->FirstWindow, AmigaPendingChunky);
    }
#endif
}

static inline int    SDL_SetRenderTarget(SDL_Renderer *r, SDL_Texture *t) { (void)r; (void)t; return 0; }
static inline int    SDL_SetRenderDrawColor(SDL_Renderer *r, uint8_t rr, uint8_t g, uint8_t b, uint8_t a) { (void)r; (void)rr; (void)g; (void)b; (void)a; return 0; }

/*
 * SDL_RenderGetViewport / SDL_RenderGetScale
 *
 * THE FIX for the reported mouse coordinate offset/misalignment bug:
 *
 * Previously these two stubs unconditionally returned a hardcoded
 * viewport of {0,0,320,200} and scale of {1.0,1.0}, completely ignoring
 * the "logical size" requested via SDL_RenderSetLogicalSize(renderer,
 * SCREENWIDTH, actualheight) in src/i_video.cpp's SetVideoMode(). Whenever
 * aspect_ratio_correct=1 (the DEFAULT setting, see VIDEO_LoadPrefs()),
 * actualheight is 240 instead of 200 (SCREENHEIGHT_4_3), so
 * I_GetMousePos()/I_SetMousePos() - which explicitly divide/multiply by
 * these scale values to translate between real window pixels and the
 * game's native 320x200 coordinate space - silently applied the WRONG
 * conversion (an implicit x1.2 Y stretch with no matching viewport
 * offset), producing exactly the vertical mouse-cursor drift/offset
 * described in the bug report.
 *
 * On real desktop SDL2 targets, the renderer actually letterboxes/scales
 * the drawn content to fit an arbitrarily-resizable window, so a nonzero
 * viewport offset + non-unity scale legitimately matter there. On this
 * Amiga RTG port however, the physical output is always a FIXED, NATIVE
 * 320x200 chunky screen (AMIGA_GAME_WIDTH/HEIGHT) that we blit into 1:1,
 * pixel-for-pixel, with no scaling or letterboxing step at all (see
 * Amiga_BlitScreen() above - it just calls WriteChunkyPixels() directly on
 * the exact source dimensions). So the correct/consistent viewport for us
 * is simply the *entire* fixed output (no pillarbox/letterbox offset),
 * and the correct per-axis scale is real-output-size / logical-size --
 * which correctly reduces to sx=1.0 (SCREENWIDTH==logical width always)
 * and sy=SCREENHEIGHT/actualheight (canceling out the *actualheight*
 * factor i_video.cpp's math re-applies), giving an exact identity mapping
 * back onto the native 0-319 / 0-199 pixel space regardless of the
 * aspect_ratio_correct setting - fixing the offset.
 */
static inline int    SDL_RenderGetViewport(SDL_Renderer *r, SDL_Rect *rect) {
    (void)r;
    int outw = 320, outh = 200;
    SDL_GetRendererOutputSize(r, &outw, &outh);
    if (rect) { rect->x = 0; rect->y = 0; rect->w = outw; rect->h = outh; }
    return 0;
}
static inline int    SDL_RenderGetScale(SDL_Renderer *r, float *sx, float *sy) {
    int outw = 320, outh = 200;
    SDL_GetRendererOutputSize(r, &outw, &outh);
    if (sx) {
        *sx = (r && r->logical_w > 0) ? ((float)outw / (float)r->logical_w) : 1.0f;
    }
    if (sy) {
        *sy = (r && r->logical_h > 0) ? ((float)outh / (float)r->logical_h) : 1.0f;
    }
    return 0;
}

/* --- Texture --- Allocates real struct with dimensions                    */

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

/*
 * SDL_SetPaletteColors: stores the palette into the SDL_Palette struct (as
 * before, for internal consistency with the rest of i_video.cpp), AND - on
 * Amiga - immediately pushes it to the real hardware/RTG screen via
 * Amiga_ApplyPalette()/LoadRGB32() so the 256-color palette is actually
 * applied on screen.
 */
static inline int SDL_SetPaletteColors(SDL_Palette *p, const SDL_Color *c, int first, int n) {
    if (p && p->colors && c) {
        int i;
        for (i = 0; i < n && (first + i) < p->ncolors; ++i)
            p->colors[first + i] = c[i];
    }
#ifdef __AMIGA__
    if (AmigaGameScreen && c) {
        Amiga_ApplyPalette(AmigaGameScreen, c, first, n);
    }
#endif
    return 0;
}

static inline int SDL_LowerBlit(SDL_Surface *src, SDL_Rect *sr, SDL_Surface *dst, SDL_Rect *dr) {
    /*
     * Palette-indexed 8-bit surface -> 32-bit ARGB surface conversion.
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

    uint8_t *srcpix = (uint8_t*)src->pixels;
    int srcpitch = src->pitch;

/* Konwersja palety omijana na Amidze na rzecz surowego bufora chunky */
#ifndef __AMIGA__
    SDL_Color *colors = src->format->palette->colors;
    uint32_t *dstpix = (uint32_t*)dst->pixels;
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
#else
    /*
     * Cache the source 8-bit chunky buffer pointer + dimensions for the
     * actual RTG screen blit, which happens later at SDL_RenderPresent()
     * time (the real "flip" point in the frame).
     */
    AmigaPendingChunky = srcpix + y0 * srcpitch + x0;
    AmigaPendingW = w;
    AmigaPendingH = h;
#endif

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

/* --- Audio: real ahi.device backend (double-buffered CMD_WRITE) ---        */

/*
 * Implements the small subset of the SDL2 "simple"/global audio API that
 * src/fx.cpp (SND_InitSound(), via SDL_OpenAudioDevice() below, which just
 * forwards to SDL_OpenAudio()) and src/mputsf.cpp (TSF_Init(), which calls
 * SDL_OpenAudio() directly) need:
 *
 *      SDL_OpenAudio() / SDL_OpenAudioDevice() / SDL_CloseAudio()
 *      SDL_PauseAudio() / SDL_PauseAudioDevice()
 *      SDL_LockAudio()  / SDL_LockAudioDevice()
 *      SDL_UnlockAudio()/ SDL_UnlockAudioDevice()
 *
 * IMPORTANT - scope of this fix: this backend is ONLY ever reached on the
 * NORMAL (audio-enabled) path. When -nosound is passed on the command
 * line, src/fx.cpp's SND_InitSound() returns at its very first statement
 * (before calling SDL_Init(SDL_INIT_AUDIO) or any function in this file),
 * so none of the code below ever executes and ahi.device is never opened.
 * See the "if (g_nosound) { ...; return 1; }" early-out at the top of
 * SND_InitSound() in src/fx.cpp, and the -nosound/-nomusic argv parsing in
 * src/rap.cpp's main().
 *
 * Design: ahi.device is opened directly via OpenDevice("ahi.device", ...)
 * with a raw struct AHIRequest (defined locally below, since the minimal
 * devices/ahi.h shipped with this cross toolchain is missing the
 * io_Data/io_Length/io_Offset/ahir_Link fields required for CMD_WRITE
 * streaming). Only the raw device I/O interface is used - no ahi.library,
 * no AHI_AllocAudio()/AHI_Play()/AHI_LoadSound() convenience calls.
 *
 * Streaming uses classic double buffering (AHI developer guide, "Writing
 * To The Device" chapter): two AHIRequests, each owning one sample buffer,
 * chained via ahir_Link so ahi.device continues seamlessly from one buffer
 * into the next. A dedicated background task (CreateNewProcTags()) pumps
 * the buffers: it waits for the oldest outstanding request to complete,
 * refills it via the SDL_AudioCallback, and resends it chained after the
 * other (still in-flight) buffer.
 *
 * NO STDIO ON THE AUDIO TASK: a process created via CreateNewProcTags()
 * without NP_Output/NP_Input/NP_CloseOutput has no valid DOS console
 * filehandle - calling printf()/fflush() from AudioTaskEntry() or the
 * fill-buffer helper it calls would dereference a bad/foreign BPTR and
 * fault. All diagnostic logging below therefore happens only in
 * SDL_OpenAudio()/SDL_CloseAudio()/SDL_PauseAudio(), which always run on
 * the caller's (main) task.
 */
#ifdef __AMIGA__

#include <exec/types.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>

#define AMIGA_AHINAME          "ahi.device"
#define AMIGA_AHI_DEFAULT_UNIT 0
#define AMIGA_AHIST_M16S 0x00000003UL /* 16 bit mono signed */
#define AMIGA_AHIST_S16S 0x00000006UL /* 16 bit stereo signed */

/* AHI's 16.16 fixed-point type (normally "Fixed" from <graphics/gfx.h>). */
typedef LONG AmigaAHIFixed;
#define AMIGA_AHI_FIXED_1_0 ((AmigaAHIFixed)0x00010000L)

/*
 * Local, ABI-correct replacement for <devices/ahi.h>'s struct AHIRequest.
 * Real ahi.device (per the AHI SDK/RKM) reserves a UWORD pad + FOUR ULONGs
 * of driver-private scratch space right after ahir_Version, and every
 * consumer/driver relies on ahir_Link existing for double-buffered
 * streaming - both are required and both are correctly sized here.
 */
struct AmigaAHIRequest
{
    struct IOStdReq  ahir_Std;        /* io_Data/io_Length/io_Offset/io_Command/io_Error */
    UWORD            ahir_Version;
    UWORD            ahir_Reserved;
    ULONG            ahir_Private[4]; /* driver-private - hands off, must be 4 ULONGs */
    ULONG            ahir_Type;
    ULONG            ahir_Frequency;
    AmigaAHIFixed    ahir_Volume;
    AmigaAHIFixed    ahir_Position;
    struct AmigaAHIRequest *ahir_Link;
};

#define AMIGA_AUDIO_NUM_BUFFERS 2

struct AmigaAudioState
{
    int initialized;
    struct MsgPort         *port;
    struct AmigaAHIRequest *req[AMIGA_AUDIO_NUM_BUFFERS];
    int devopen;
    UBYTE *buffer[AMIGA_AUDIO_NUM_BUFFERS];
    ULONG  bufferBytes;
    int freq;
    int channels;
    ULONG ahiType;
    void (*callback)(void *userdata, uint8_t *stream, int len);
    void *userdata;
    struct Process *audioTask;
    volatile int taskRunning;
    volatile int taskShouldQuit;
    volatile int paused;
};

AMIGA_STUBS_DECL struct AmigaAudioState g_AmigaAudio;

static inline void AmigaAudio_FreeBuffers(void)

{
    int i;
    for (i = 0; i < AMIGA_AUDIO_NUM_BUFFERS; i++)
    {
        if (g_AmigaAudio.buffer[i])
        {
            FreeMem(g_AmigaAudio.buffer[i], g_AmigaAudio.bufferBytes);
            g_AmigaAudio.buffer[i] = NULL;
        }
    }
}

static inline void AmigaAudio_FreeIOReqs(void)

{
    int i;
    for (i = 0; i < AMIGA_AUDIO_NUM_BUFFERS; i++)
    {
        if (g_AmigaAudio.req[i])
        {
            DeleteIORequest((struct IORequest *)g_AmigaAudio.req[i]);
            g_AmigaAudio.req[i] = NULL;
        }
    }
    if (g_AmigaAudio.port)
    {
        DeleteMsgPort(g_AmigaAudio.port);
        g_AmigaAudio.port = NULL;
    }
}

/* NO STDIO HERE - runs on the headless background audio task. */
static inline void AmigaAudio_FillBuffer(UBYTE *dst, ULONG bytes)

{
    if (!dst || bytes == 0)
        return;

    if (g_AmigaAudio.paused || !g_AmigaAudio.callback)
    {
        memset(dst, 0, bytes);
        return;
    }

    g_AmigaAudio.callback(g_AmigaAudio.userdata, dst, (int)bytes);
}

static inline void AmigaAudio_SetupRequest(struct AmigaAHIRequest *req, UBYTE *buf, ULONG bytes)

{
    req->ahir_Std.io_Command = CMD_WRITE;
    req->ahir_Std.io_Data    = (APTR)buf;
    req->ahir_Std.io_Length  = (ULONG)bytes;
    req->ahir_Std.io_Offset  = 0;
    req->ahir_Type      = g_AmigaAudio.ahiType;
    req->ahir_Frequency = (ULONG)g_AmigaAudio.freq;
    req->ahir_Volume    = AMIGA_AHI_FIXED_1_0;
    req->ahir_Position  = 0x00000000; /* centered */
}

/* NO STDIO IN THIS FUNCTION - dedicated background audio task. */
static inline void AmigaAudio_TaskEntry(void)

{
    int cur;

    g_AmigaAudio.taskRunning = 1;

    if (!g_AmigaAudio.buffer[0] || !g_AmigaAudio.buffer[1] ||
        !g_AmigaAudio.req[0]    || !g_AmigaAudio.req[1]    ||
        !g_AmigaAudio.devopen)
    {
        g_AmigaAudio.taskRunning = 0;
        return;
    }

    AmigaAudio_FillBuffer(g_AmigaAudio.buffer[0], g_AmigaAudio.bufferBytes);
    AmigaAudio_FillBuffer(g_AmigaAudio.buffer[1], g_AmigaAudio.bufferBytes);

    AmigaAudio_SetupRequest(g_AmigaAudio.req[0], g_AmigaAudio.buffer[0], g_AmigaAudio.bufferBytes);
    g_AmigaAudio.req[0]->ahir_Link = NULL;
    SendIO((struct IORequest *)g_AmigaAudio.req[0]);

    AmigaAudio_SetupRequest(g_AmigaAudio.req[1], g_AmigaAudio.buffer[1], g_AmigaAudio.bufferBytes);
    g_AmigaAudio.req[1]->ahir_Link = g_AmigaAudio.req[0];
    SendIO((struct IORequest *)g_AmigaAudio.req[1]);

    cur = 0;

    while (!g_AmigaAudio.taskShouldQuit)
    {
        struct AmigaAHIRequest *done = g_AmigaAudio.req[cur];
        int other;

        WaitIO((struct IORequest *)done);

        AmigaAudio_FillBuffer(g_AmigaAudio.buffer[cur], g_AmigaAudio.bufferBytes);

        other = cur ^ 1;
        AmigaAudio_SetupRequest(done, g_AmigaAudio.buffer[cur], g_AmigaAudio.bufferBytes);
        done->ahir_Link = g_AmigaAudio.req[other];
        SendIO((struct IORequest *)done);

        cur ^= 1;
    }

    AbortIO((struct IORequest *)g_AmigaAudio.req[0]);
    AbortIO((struct IORequest *)g_AmigaAudio.req[1]);
    WaitIO((struct IORequest *)g_AmigaAudio.req[0]);
    WaitIO((struct IORequest *)g_AmigaAudio.req[1]);

    g_AmigaAudio.taskRunning = 0;
}

static inline void SDL_CloseAudio(void)
{
    if (!g_AmigaAudio.initialized)
        return;

    printf("[AMIGA][AUDIO] SDL_CloseAudio: shutting down\n"); fflush(stdout);

    g_AmigaAudio.taskShouldQuit = 1;
    {
        int spins = 0;
        while (g_AmigaAudio.taskRunning && spins < 500)
        {
            Delay(1);
            spins++;
        }
    }

    if (g_AmigaAudio.devopen)
    {
        CloseDevice((struct IORequest *)g_AmigaAudio.req[0]);
        g_AmigaAudio.devopen = 0;
    }

    AmigaAudio_FreeIOReqs();
    AmigaAudio_FreeBuffers();

    memset(&g_AmigaAudio, 0, sizeof(g_AmigaAudio));

    printf("[AMIGA][AUDIO] SDL_CloseAudio: done\n"); fflush(stdout);
}

/*
 * DIAGNOSIS / FIX: #8000000B (Line 1111/F-line emulator, FPU trap) crashing
 * exactly at OpenDevice(). This project is built with -m68060 -m68881
 * together; at -O2 GCC's register allocator may opportunistically spill a
 * plain integer/pointer value into an FPU register via an "fmove" that a
 * real 68060 doesn't implement in hardware (relying on FPSP emulation that
 * may not be resident/active for that opcode), producing an unhandled
 * trap that looks like it happens "at" OpenDevice(). Forcing this function
 * to compile at -O0 (Makefile.amiga's global -O2 cannot be changed per
 * project rules) removes the opportunity for that spill entirely.
 */
__attribute__((optimize("O0")))
static inline int SDL_OpenAudio(const SDL_AudioSpec *desired, SDL_AudioSpec *obtained)
{
    if (g_AmigaAudio.initialized)
        SDL_CloseAudio();

    memset(&g_AmigaAudio, 0, sizeof(g_AmigaAudio));

    if (!desired)
        return -1;

    printf("[AMIGA][AUDIO] SDL_OpenAudio: ENTRY desired->freq=%d desired->format=0x%04x "
           "desired->channels=%d desired->samples=%u desired->callback=%p desired->userdata=%p\n",
           desired->freq, (unsigned)desired->format, desired->channels,
           (unsigned)desired->samples, (void *)desired->callback, desired->userdata);
    fflush(stdout);

    g_AmigaAudio.freq     = desired->freq > 0 ? desired->freq : 44100;
    g_AmigaAudio.channels = desired->channels > 0 ? desired->channels : 2;
    g_AmigaAudio.callback = desired->callback;
    g_AmigaAudio.userdata = desired->userdata;
    g_AmigaAudio.paused   = 1; /* SDL2 semantics: audio starts paused */
    g_AmigaAudio.ahiType  = (g_AmigaAudio.channels >= 2) ? AMIGA_AHIST_S16S : AMIGA_AHIST_M16S;

    {
        ULONG frames = desired->samples > 0 ? desired->samples : 512;
        g_AmigaAudio.bufferBytes = frames * (ULONG)g_AmigaAudio.channels * sizeof(short);
    }

    printf("[AMIGA][AUDIO] SDL_OpenAudio: using freq=%d channels=%d ahiType=%s bufferBytes=%lu\n",
           g_AmigaAudio.freq, g_AmigaAudio.channels,
           (g_AmigaAudio.ahiType == AMIGA_AHIST_S16S) ? "AHIST_S16S" : "AHIST_M16S",
           (unsigned long)g_AmigaAudio.bufferBytes);
    fflush(stdout);

    {
        int i;
        for (i = 0; i < AMIGA_AUDIO_NUM_BUFFERS; i++)
        {
            g_AmigaAudio.buffer[i] = (UBYTE *)AllocMem(g_AmigaAudio.bufferBytes, MEMF_PUBLIC | MEMF_CLEAR);
            if (!g_AmigaAudio.buffer[i])
            {
                printf("[AMIGA][AUDIO] SDL_OpenAudio: AllocMem failed for buffer %d\n", i);
                fflush(stdout);
                AmigaAudio_FreeBuffers();
                return -1;
            }
        }
    }

    g_AmigaAudio.port = CreateMsgPort();
    if (!g_AmigaAudio.port)
    {
        printf("[AMIGA][AUDIO] SDL_OpenAudio: CreateMsgPort failed\n"); fflush(stdout);
        AmigaAudio_FreeBuffers();
        return -1;
    }

    {
        int i;
        for (i = 0; i < AMIGA_AUDIO_NUM_BUFFERS; i++)
        {
            g_AmigaAudio.req[i] = (struct AmigaAHIRequest *)CreateIORequest(g_AmigaAudio.port, sizeof(struct AmigaAHIRequest));
            if (!g_AmigaAudio.req[i])
            {
                printf("[AMIGA][AUDIO] SDL_OpenAudio: CreateIORequest failed for req %d\n", i);
                fflush(stdout);
                AmigaAudio_FreeIOReqs();
                AmigaAudio_FreeBuffers();
                return -1;
            }
            memset(g_AmigaAudio.req[i], 0, sizeof(struct AmigaAHIRequest));
            g_AmigaAudio.req[i]->ahir_Version = 2;
            g_AmigaAudio.req[i]->ahir_Std.io_Message.mn_Node.ln_Type = NT_MESSAGE;
            g_AmigaAudio.req[i]->ahir_Std.io_Message.mn_Node.ln_Name = NULL;
            g_AmigaAudio.req[i]->ahir_Std.io_Message.mn_ReplyPort    = g_AmigaAudio.port;
            g_AmigaAudio.req[i]->ahir_Std.io_Message.mn_Length       = sizeof(struct AmigaAHIRequest);
        }
    }

    {
        BYTE odErr;
        extern struct ExecBase *SysBase;

        printf("[AMIGA][AUDIO] SDL_OpenAudio: SysBase=0x%08lx\n", (unsigned long)SysBase);
        fflush(stdout);

        if (!SysBase)
        {
            printf("[AMIGA][AUDIO] SDL_OpenAudio: FATAL - SysBase is NULL, aborting audio init.\n");
            fflush(stdout);
            AmigaAudio_FreeIOReqs();
            AmigaAudio_FreeBuffers();
            return -1;
        }

        printf("[AMIGA][AUDIO] SDL_OpenAudio: about to call OpenDevice()\n");
        fflush(stdout);

        odErr = OpenDevice((CONST_STRPTR)AMIGA_AHINAME, AMIGA_AHI_DEFAULT_UNIT,
                            (struct IORequest *)g_AmigaAudio.req[0], 0);

        printf("[AMIGA][AUDIO] SDL_OpenAudio: OpenDevice(\"%s\", unit=%d) returned %d (0=success)\n",
               AMIGA_AHINAME, AMIGA_AHI_DEFAULT_UNIT, (int)odErr);
        fflush(stdout);

        if (odErr != 0)
        {
            printf("[AMIGA][AUDIO] SDL_OpenAudio: OpenDevice FAILED (io_Error=%d) - "
                   "ahi.device could not be opened. Check DEVS:AHI mode/driver config.\n",
                   (int)g_AmigaAudio.req[0]->ahir_Std.io_Error);
            fflush(stdout);
            AmigaAudio_FreeIOReqs();
            AmigaAudio_FreeBuffers();
            return -1;
        }
    }
    g_AmigaAudio.devopen = 1;

    g_AmigaAudio.req[1]->ahir_Std.io_Device = g_AmigaAudio.req[0]->ahir_Std.io_Device;
    g_AmigaAudio.req[1]->ahir_Std.io_Unit   = g_AmigaAudio.req[0]->ahir_Std.io_Unit;

    printf("[AMIGA][AUDIO] ahi.device opened OK (unit %d)\n", AMIGA_AHI_DEFAULT_UNIT);
    fflush(stdout);

    g_AmigaAudio.taskShouldQuit = 0;
    g_AmigaAudio.taskRunning = 0;

    g_AmigaAudio.audioTask = CreateNewProcTags(
        NP_Entry,     (ULONG)AmigaAudio_TaskEntry,
        NP_Name,      (ULONG)"Raptor Audio Task",
        NP_Priority,  (LONG)0,
        NP_StackSize, (ULONG)16384,
        TAG_DONE);

    if (!g_AmigaAudio.audioTask)
    {
        printf("[AMIGA][AUDIO] SDL_OpenAudio: CreateNewProcTags failed\n"); fflush(stdout);
        CloseDevice((struct IORequest *)g_AmigaAudio.req[0]);
        g_AmigaAudio.devopen = 0;
        AmigaAudio_FreeIOReqs();
        AmigaAudio_FreeBuffers();
        return -1;
    }

    printf("[AMIGA][AUDIO] Background audio task started\n"); fflush(stdout);

    g_AmigaAudio.initialized = 1;

    if (obtained)
    {
        obtained->freq     = g_AmigaAudio.freq;
        obtained->format   = AUDIO_S16SYS;
        obtained->channels = (uint8_t)g_AmigaAudio.channels;
        obtained->samples  = desired->samples;
        obtained->size     = g_AmigaAudio.bufferBytes;
        obtained->callback = g_AmigaAudio.callback;
        obtained->userdata = g_AmigaAudio.userdata;
        obtained->silence  = 0;
    }

    return 0;
}

static inline SDL_AudioDeviceID SDL_OpenAudioDevice(const char *d, int ic,
    const SDL_AudioSpec *desired, SDL_AudioSpec *obtained, int changes)
{
    (void)d; (void)ic; (void)changes;
    if (SDL_OpenAudio(desired, obtained) < 0)
        return 0;
    return 1; /* non-zero device id == success, matching SDL2 semantics */
}

static inline void SDL_PauseAudio(int pause_on) { g_AmigaAudio.paused = pause_on ? 1 : 0; }
static inline void SDL_PauseAudioDevice(SDL_AudioDeviceID d, int p) { (void)d; SDL_PauseAudio(p); }

/* Disable()/Enable(): lightweight critical-section guard between the main
 * task (e.g. TinySoundFont tsf_channel_* calls) and the background audio
 * task's callback invocation - the traditional AmigaOS technique for a
 * tiny critical section like this one. */
static inline void SDL_LockAudio(void)   { Disable(); }
static inline void SDL_UnlockAudio(void) { Enable(); }
static inline void SDL_LockAudioDevice(SDL_AudioDeviceID d)   { (void)d; SDL_LockAudio(); }
static inline void SDL_UnlockAudioDevice(SDL_AudioDeviceID d) { (void)d; SDL_UnlockAudio(); }

#else /* !__AMIGA__ : non-Amiga stub/test-compile fallback, unchanged */

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
static inline void   SDL_LockAudio(void) {}
static inline void   SDL_UnlockAudio(void) {}

#endif /* __AMIGA__ */

/* --- Mouse button constants ---
 * (Moved above the native event pump below, since Amiga_PumpWindowEvents()
 * needs these already defined when translating IDCMP_MOUSEBUTTONS codes.) */
#define SDL_BUTTON_LEFT     1
#define SDL_BUTTON_MIDDLE   2
#define SDL_BUTTON_RIGHT    3

#ifndef SDL_BUTTON
#define SDL_BUTTON(X) (1 << ((X)-1))
#endif

/* SDL_ShowCursor() argument/return values (real SDL2 numbering). */
#define SDL_DISABLE 0
#define SDL_ENABLE  1
#define SDL_QUERY   -1

/* --- Native Amiga IDCMP event pump --- feeds the SDL_Event queue          */
/*                                                                           */
/* This is the piece that was completely missing before: SDL_PumpEvents()   */
/* and SDL_PollEvent() were both no-op stubs that never touched             */
/* AmigaGameWindow->UserPort at all, so no keyboard, mouse or window event   */
/* ever reached the game loop (I_GetEvent() in i_video.cpp), even though     */
/* the window itself was correctly asking for IDCMP_RAWKEY |                */
/* IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_CLOSEWINDOW in               */
/* SDL_CreateWindow() above.                                                 */
/*                                                                           */
/* Amiga_PumpWindowEvents() drains the window's IDCMP message port with the  */
/* standard GetMsg()/ReplyMsg() Intuition event loop pattern and translates  */
/* each IntuiMessage into an equivalent SDL_Event, pushed onto a small ring  */
/* buffer. SDL_PollEvent() then just pops events back out of that buffer,   */
/* exactly mirroring how real SDL2 works internally.                        */

#ifdef __AMIGA__
static inline void Amiga_PushEvent(const SDL_Event *ev)
{
    int next = (AmigaEventQueueTail + 1) % AMIGA_SDL_EVENT_QUEUE_SIZE;
    if (next == AmigaEventQueueHead) {
        AmigaEventQueueHead = (AmigaEventQueueHead + 1) % AMIGA_SDL_EVENT_QUEUE_SIZE;
    }
    AmigaEventQueue[AmigaEventQueueTail] = *ev;
    AmigaEventQueueTail = next;
}

static inline int Amiga_PopEvent(SDL_Event *out)
{
    if (AmigaEventQueueHead == AmigaEventQueueTail)
        return 0;
    if (out) *out = AmigaEventQueue[AmigaEventQueueHead];
    AmigaEventQueueHead = (AmigaEventQueueHead + 1) % AMIGA_SDL_EVENT_QUEUE_SIZE;
    return 1;
}
#else
static inline void Amiga_PushEvent(const SDL_Event *ev) { (void)ev; }
static inline int  Amiga_PopEvent(SDL_Event *out)       { (void)out; return 0; }
#endif /* __AMIGA__ */

#ifdef __AMIGA__

#ifndef IECODE_UP_PREFIX
#define IECODE_UP_PREFIX 0x80
#endif

/* Qualifier bits (normally in devices/inputevent.h). Defined raw/inline here
 * for the same reason as the BIDTAG_* values near the top of this file: we
 * don't want to depend on that header necessarily being present. */
#ifndef IEQUALIFIER_LALT
#define IEQUALIFIER_LSHIFT   0x0001
#define IEQUALIFIER_RSHIFT   0x0002
#define IEQUALIFIER_CAPSLOCK 0x0004
#define IEQUALIFIER_CONTROL  0x0008
#define IEQUALIFIER_LALT     0x0010
#define IEQUALIFIER_RALT     0x0020
#define IEQUALIFIER_LCOMMAND 0x0040
#define IEQUALIFIER_RCOMMAND 0x0080
#endif

/* Mouse button IDCMP codes (normally in intuition/intuition.h - defined
 * here as a fallback in case they aren't pulled in transitively). These
 * are the standard, stable Amiga raw mouse button codes. */
#ifndef SELECTDOWN
#define SELECTDOWN   (0x68)
#define SELECTUP     (0x68 | IECODE_UP_PREFIX)
#define MENUDOWN     (0x69)
#define MENUUP       (0x69 | IECODE_UP_PREFIX)
#define MIDDLEDOWN   (0x6A)
#define MIDDLEUP     (0x6A | IECODE_UP_PREFIX)
#endif

/*
 * Amiga raw keycode -> SDL USB-HID SDL_SCANCODE_* translation table.
 *
 * Index = raw key code (devices/rawkeycodes.h layout, standard Amiga 500/
 * 2000/3000 keyboard) with the IECODE_UP_PREFIX (0x80) release bit already
 * stripped off, so the whole keyboard fits in 0x00-0x67. A value of 0 means
 * "no mapping" (unused/reserved Amiga raw code, or a key with no sensible
 * PC-style equivalent, e.g. Amiga/Help keys).
 *
 * The resulting scancodes are consumed unmodified by the existing
 * src/kbdapi.cpp I_HandleKeyboardEvent()/ScanCodeMap[] logic, which is why
 * the SDL_SCANCODE_* values defined earlier in this file must (and do)
 * match real SDL2's numbering exactly.
 */
static const uint8_t AmigaRawKeyToScancode[0x68] = {
    /* 0x00 */ SDL_SCANCODE_GRAVE,
    /* 0x01 */ SDL_SCANCODE_1,
    /* 0x02 */ SDL_SCANCODE_2,
    /* 0x03 */ SDL_SCANCODE_3,
    /* 0x04 */ SDL_SCANCODE_4,
    /* 0x05 */ SDL_SCANCODE_5,
    /* 0x06 */ SDL_SCANCODE_6,
    /* 0x07 */ SDL_SCANCODE_7,
    /* 0x08 */ SDL_SCANCODE_8,
    /* 0x09 */ SDL_SCANCODE_9,
    /* 0x0A */ SDL_SCANCODE_0,
    /* 0x0B */ SDL_SCANCODE_MINUS,
    /* 0x0C */ SDL_SCANCODE_EQUALS,
    /* 0x0D */ SDL_SCANCODE_BACKSLASH,
    /* 0x0E */ 0,
    /* 0x0F */ SDL_SCANCODE_KP_0,
    /* 0x10 */ SDL_SCANCODE_Q,
    /* 0x11 */ SDL_SCANCODE_W,
    /* 0x12 */ SDL_SCANCODE_E,
    /* 0x13 */ SDL_SCANCODE_R,
    /* 0x14 */ SDL_SCANCODE_T,
    /* 0x15 */ SDL_SCANCODE_Y,
    /* 0x16 */ SDL_SCANCODE_U,
    /* 0x17 */ SDL_SCANCODE_I,
    /* 0x18 */ SDL_SCANCODE_O,
    /* 0x19 */ SDL_SCANCODE_P,
    /* 0x1A */ SDL_SCANCODE_LEFTBRACKET,
    /* 0x1B */ SDL_SCANCODE_RIGHTBRACKET,
    /* 0x1C */ 0,
    /* 0x1D */ SDL_SCANCODE_KP_1,
    /* 0x1E */ SDL_SCANCODE_KP_2,
    /* 0x1F */ SDL_SCANCODE_KP_3,
    /* 0x20 */ SDL_SCANCODE_A,
    /* 0x21 */ SDL_SCANCODE_S,
    /* 0x22 */ SDL_SCANCODE_D,
    /* 0x23 */ SDL_SCANCODE_F,
    /* 0x24 */ SDL_SCANCODE_G,
    /* 0x25 */ SDL_SCANCODE_H,
    /* 0x26 */ SDL_SCANCODE_J,
    /* 0x27 */ SDL_SCANCODE_K,
    /* 0x28 */ SDL_SCANCODE_L,
    /* 0x29 */ SDL_SCANCODE_SEMICOLON,
    /* 0x2A */ SDL_SCANCODE_APOSTROPHE,
    /* 0x2B */ 0,
    /* 0x2C */ 0,
    /* 0x2D */ SDL_SCANCODE_KP_4,
    /* 0x2E */ SDL_SCANCODE_KP_5,
    /* 0x2F */ SDL_SCANCODE_KP_6,
    /* 0x30 */ 0,
    /* 0x31 */ SDL_SCANCODE_Z,
    /* 0x32 */ SDL_SCANCODE_X,
    /* 0x33 */ SDL_SCANCODE_C,
    /* 0x34 */ SDL_SCANCODE_V,
    /* 0x35 */ SDL_SCANCODE_B,
    /* 0x36 */ SDL_SCANCODE_N,
    /* 0x37 */ SDL_SCANCODE_M,
    /* 0x38 */ SDL_SCANCODE_COMMA,
    /* 0x39 */ SDL_SCANCODE_PERIOD,
    /* 0x3A */ SDL_SCANCODE_SLASH,
    /* 0x3B */ 0,
    /* 0x3C */ SDL_SCANCODE_KP_PERIOD,
    /* 0x3D */ SDL_SCANCODE_KP_7,
    /* 0x3E */ SDL_SCANCODE_KP_8,
    /* 0x3F */ SDL_SCANCODE_KP_9,
    /* 0x40 */ SDL_SCANCODE_SPACE,
    /* 0x41 */ SDL_SCANCODE_BACKSPACE,
    /* 0x42 */ SDL_SCANCODE_TAB,
    /* 0x43 */ SDL_SCANCODE_KP_ENTER,
    /* 0x44 */ SDL_SCANCODE_RETURN,
    /* 0x45 */ SDL_SCANCODE_ESCAPE,
    /* 0x46 */ SDL_SCANCODE_DELETE,
    /* 0x47 */ 0,
    /* 0x48 */ 0,
    /* 0x49 */ 0,
    /* 0x4A */ SDL_SCANCODE_KP_MINUS,
    /* 0x4B */ 0,
    /* 0x4C */ SDL_SCANCODE_UP,
    /* 0x4D */ SDL_SCANCODE_DOWN,
    /* 0x4E */ SDL_SCANCODE_RIGHT,
    /* 0x4F */ SDL_SCANCODE_LEFT,
    /* 0x50 */ SDL_SCANCODE_F1,
    /* 0x51 */ SDL_SCANCODE_F2,
    /* 0x52 */ SDL_SCANCODE_F3,
    /* 0x53 */ SDL_SCANCODE_F4,
    /* 0x54 */ SDL_SCANCODE_F5,
    /* 0x55 */ SDL_SCANCODE_F6,
    /* 0x56 */ SDL_SCANCODE_F7,
    /* 0x57 */ SDL_SCANCODE_F8,
    /* 0x58 */ SDL_SCANCODE_F9,
    /* 0x59 */ SDL_SCANCODE_F10,
    /* 0x5A */ 0,
    /* 0x5B */ 0,
    /* 0x5C */ 0,
    /* 0x5D */ 0,
    /* 0x5E */ 0,
    /* 0x5F */ 0, /* Help */
    /* 0x60 */ SDL_SCANCODE_LSHIFT,
    /* 0x61 */ SDL_SCANCODE_RSHIFT,
    /* 0x62 */ SDL_SCANCODE_CAPSLOCK,
    /* 0x63 */ SDL_SCANCODE_LCTRL,
    /* 0x64 */ SDL_SCANCODE_LALT,
    /* 0x65 */ SDL_SCANCODE_RALT,
    /* 0x66 */ SDL_SCANCODE_LGUI,
    /* 0x67 */ SDL_SCANCODE_RGUI
};

#ifndef SDL_NUM_SCANCODES
#define SDL_NUM_SCANCODES 512
#endif

AMIGA_STUBS_DECL ULONG AmigaJoyStatePrev AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL Uint32 AmigaFrameCount AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL Uint8 AmigaKeyboardState[SDL_NUM_SCANCODES] AMIGA_STUBS_INIT({0});

#ifndef JPF_JOY_UP
#define JPF_JOY_UP (1<<3)
#define JPF_JOY_DOWN (1<<2)
#define JPF_JOY_LEFT (1<<1)
#define JPF_JOY_RIGHT (1<<0)
#define JPF_BUTTON_PLAY (1<<17)
#define JPF_BUTTON_RED (1<<22)
#endif

static inline void Amiga_InjectKeyboardEvent(int scancode, int pressed) {
    if (scancode <= 0 || scancode >= SDL_NUM_SCANCODES) return;
    AmigaKeyboardState[scancode] = pressed ? 1 : 0;
    
    SDL_Event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = pressed ? SDL_KEYDOWN : SDL_KEYUP;
    ev.key.keysym.scancode = scancode;
    ev.key.keysym.sym = scancode;
    ev.key.keysym.mod = 0;
    Amiga_PushEvent(&ev);
}

static inline void Amiga_PumpWindowEvents(void)
{
    if (!AmigaGameWindow) return;
    struct IntuiMessage *msg;
    while ((msg = (struct IntuiMessage *)GetMsg(AmigaGameWindow->UserPort)))
    {
        ULONG class_ = msg->Class;
        UWORD code = msg->Code;
        WORD mx = msg->MouseX;
        WORD my = msg->MouseY;
        ReplyMsg((struct Message *)msg);

        if (class_ == IDCMP_CLOSEWINDOW)
        {
            SDL_Event ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = SDL_QUIT;
            Amiga_PushEvent(&ev);
        }
        else if (class_ == IDCMP_RAWKEY)
        {
            int pressed = !(code & IECODE_UP_PREFIX);
            code &= ~IECODE_UP_PREFIX;
            if (code < sizeof(AmigaRawKeyToScancode))
            {
                int scancode = AmigaRawKeyToScancode[code];
                if (scancode)
                {
                    Amiga_InjectKeyboardEvent(scancode, pressed);
                }
            }
        }
        else if (class_ == IDCMP_MOUSEMOVE)
        {
            AmigaMouseX = mx;
            AmigaMouseY = my;
            SDL_Event ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = SDL_MOUSEMOTION;
            ev.button.x = mx;
            ev.button.y = my;
            Amiga_PushEvent(&ev);
        }
        else if (class_ == IDCMP_MOUSEBUTTONS)
        {
            SDL_Event ev;
            memset(&ev, 0, sizeof(ev));
            if (code == SELECTDOWN) { ev.type = SDL_MOUSEBUTTONDOWN; ev.button.button = SDL_BUTTON_LEFT; ev.button.state = 1; AmigaMouseButtons |= 1; }
            else if (code == SELECTUP) { ev.type = SDL_MOUSEBUTTONUP; ev.button.button = SDL_BUTTON_LEFT; ev.button.state = 0; AmigaMouseButtons &= ~1; }
            else if (code == MENUDOWN) { ev.type = SDL_MOUSEBUTTONDOWN; ev.button.button = SDL_BUTTON_RIGHT; ev.button.state = 1; AmigaMouseButtons |= 2; }
            else if (code == MENUUP) { ev.type = SDL_MOUSEBUTTONUP; ev.button.button = SDL_BUTTON_RIGHT; ev.button.state = 0; AmigaMouseButtons &= ~2; }
            else if (code == MIDDLEDOWN) { ev.type = SDL_MOUSEBUTTONDOWN; ev.button.button = SDL_BUTTON_MIDDLE; ev.button.state = 1; AmigaMouseButtons |= 4; }
            else if (code == MIDDLEUP) { ev.type = SDL_MOUSEBUTTONUP; ev.button.button = SDL_BUTTON_MIDDLE; ev.button.state = 0; AmigaMouseButtons &= ~4; }
            
            if (ev.type) {
                ev.button.x = AmigaMouseX;
                ev.button.y = AmigaMouseY;
                Amiga_PushEvent(&ev);
            }
        }
    }
}
#else
static inline void Amiga_PumpWindowEvents(void) {}
#endif

static inline void SDL_PumpEvents(void) {
#ifdef __AMIGA__
    Amiga_PumpWindowEvents();

    if (LowLevelBase) {
        ULONG joy = ReadJoyPort(1);
        
        ULONG changed = joy ^ AmigaJoyStatePrev;
        
        if (changed & JPF_JOY_UP) {
            Amiga_InjectKeyboardEvent(SDL_SCANCODE_UP, (joy & JPF_JOY_UP) != 0);
        }
        if (changed & JPF_JOY_DOWN) {
            Amiga_InjectKeyboardEvent(SDL_SCANCODE_DOWN, (joy & JPF_JOY_DOWN) != 0);
        }
        if (changed & JPF_JOY_LEFT) {
            Amiga_InjectKeyboardEvent(SDL_SCANCODE_LEFT, (joy & JPF_JOY_LEFT) != 0);
        }
        if (changed & JPF_JOY_RIGHT) {
            Amiga_InjectKeyboardEvent(SDL_SCANCODE_RIGHT, (joy & JPF_JOY_RIGHT) != 0);
        }
        
        ULONG fire_mask = JPF_BUTTON_RED | JPF_BUTTON_PLAY;
        int prev_fire = (AmigaJoyStatePrev & fire_mask) ? 1 : 0;
        int curr_fire = (joy & fire_mask) ? 1 : 0;
        if (curr_fire != prev_fire) {
            Amiga_InjectKeyboardEvent(SDL_SCANCODE_RETURN, curr_fire);
        }
        
        AmigaJoyState = joy;
        AmigaJoyStatePrev = joy;
        AmigaFrameCount++;
    }
#endif
}

static inline int SDL_PollEvent(SDL_Event *event) {
#ifdef __AMIGA__
    return Amiga_PopEvent(event);
#else
    (void)event;
    return 0;
#endif
}

static inline int SDL_WaitEvent(SDL_Event *event) {
    while (!SDL_PollEvent(event)) {
        SDL_Delay(10);
    }
    return 1;
}

static inline const Uint8* SDL_GetKeyboardState(int *numkeys) {
    if (numkeys) *numkeys = SDL_NUM_SCANCODES;
#ifdef __AMIGA__
    return AmigaKeyboardState;
#else
    static Uint8 empty[SDL_NUM_SCANCODES] = {0};
    return empty;
#endif
}

static inline uint32_t SDL_GetModState(void) {
    return 0;
}

static inline uint32_t SDL_GetMouseState(int *x, int *y) {
#ifdef __AMIGA__
    if (x) *x = AmigaMouseX;
    if (y) *y = AmigaMouseY;
    
    uint32_t mask = 0;
    if (AmigaMouseButtons & 1) mask |= SDL_BUTTON(SDL_BUTTON_LEFT);
    if (AmigaMouseButtons & 2) mask |= SDL_BUTTON(SDL_BUTTON_RIGHT);
    if (AmigaMouseButtons & 4) mask |= SDL_BUTTON(SDL_BUTTON_MIDDLE);
    return mask;
#else
    if (x) *x = 0; if (y) *y = 0; return 0;
#endif
}

static inline uint32_t SDL_GetRelativeMouseState(int *x, int *y) {
    if (x) *x = 0;
    if (y) *y = 0;
    return SDL_GetMouseState(NULL, NULL);
}

static inline int SDL_SetRelativeMouseMode(SDL_bool enabled) { (void)enabled; return 0; }
static inline void SDL_WarpMouseInWindow(SDL_Window *window, int x, int y) { (void)window; (void)x; (void)y; }
static inline int SDL_ShowCursor(int toggle) { (void)toggle; return 0; }

static inline int SDL_NumJoysticks(void) { return 1; }
static inline SDL_bool SDL_IsGameController(int idx) { (void)idx; return SDL_TRUE; }
static inline SDL_GameController* SDL_GameControllerOpen(int idx) { (void)idx; return (SDL_GameController*)1; }
static inline void SDL_GameControllerClose(SDL_GameController *gc) { (void)gc; }

static inline Sint16 SDL_GameControllerGetAxis(SDL_GameController *gamecontroller, int axis)
{
    (void)gamecontroller;
#ifdef __AMIGA__
    if (!LowLevelBase) return 0;

    if (axis == SDL_CONTROLLER_AXIS_LEFTX) {
        const int left  = (AmigaJoyState & JPF_JOY_LEFT)  ? 1 : 0;
        const int right = (AmigaJoyState & JPF_JOY_RIGHT) ? 1 : 0;

        if (left && !right)  return -32768;
        if (right && !left)  return 32767;
        return 0;
    }

    if (axis == SDL_CONTROLLER_AXIS_LEFTY) {
        const int up   = (AmigaJoyState & JPF_JOY_UP)   ? 1 : 0;
        const int down = (AmigaJoyState & JPF_JOY_DOWN) ? 1 : 0;

        if (up && !down)   return -32768;
        if (down && !up)   return 32767;
        return 0;
    }
#endif
    return 0;
}

static inline Uint8 SDL_GameControllerGetButton(SDL_GameController *gamecontroller, int button)
{
    (void)gamecontroller;
#ifdef __AMIGA__
    if (!LowLevelBase) return 0;

    {
        const int fire_red  = (AmigaJoyState & JPF_BUTTON_RED)  ? 1 : 0;
        const int fire_play = (AmigaJoyState & JPF_BUTTON_PLAY) ? 1 : 0;
        const int is_fire   = (fire_red || fire_play) ? 1 : 0;

        /*
         * Oba amigowe FIRE mają robić to samo.
         * SDL_GameControllerGetButton ma zwracać rzeczywisty, stabilny stan
         * przycisku, a nie sztucznie migający sygnał co drugą klatkę.
         * Miganie przez AmigaFrameCount psuło ciągły odczyt fire podczas trzymania.
         */
        if (button == SDL_CONTROLLER_BUTTON_A) return is_fire ? 1 : 0;
        if (button == SDL_CONTROLLER_BUTTON_B) return is_fire ? 1 : 0;
    }

    if (button == SDL_CONTROLLER_BUTTON_DPAD_UP)
        return (AmigaJoyState & JPF_JOY_UP) ? 1 : 0;

    if (button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
        return (AmigaJoyState & JPF_JOY_DOWN) ? 1 : 0;

    if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        return (AmigaJoyState & JPF_JOY_LEFT) ? 1 : 0;

    if (button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        return (AmigaJoyState & JPF_JOY_RIGHT) ? 1 : 0;
#endif
    return 0;
}

/* Wstępne deklaracje typów dla atrapy */
typedef struct SDL_Haptic SDL_Haptic;
typedef struct SDL_GameController SDL_GameController;

/* -------------------- HAPTIC ---------------------- */

static inline SDL_Haptic* SDL_HapticOpen(int idx) {
    (void)idx;
    return NULL;
}

static inline SDL_Haptic* SDL_HapticOpenFromJoystick(void *j) {
    (void)j;
    return NULL;
}

static inline int SDL_HapticRumbleInit(SDL_Haptic *h) {
    (void)h;
    return -1;
}

static inline void SDL_HapticClose(SDL_Haptic *h) {
    (void)h;
}

/* ---------------- GAME CONTROLLER ----------------- */

static inline int SDL_GameControllerGetAttached(SDL_GameController *c) {
    (void)c;
    return 0;
}

static inline int SDL_GameControllerTypeForIndex(int idx) {
    (void)idx;
    return 0;
}

static inline int SDL_GameControllerRumble(SDL_GameController *c, uint16_t low, uint16_t high, uint32_t dur) {
    (void)c; (void)low; (void)high; (void)dur;
    return -1;
}

/* -------------------- MESSAGE BOX ----------------- */

#define SDL_MESSAGEBOX_ERROR        0x00000010u
#define SDL_MESSAGEBOX_WARNING      0x00000020u
#define SDL_MESSAGEBOX_INFORMATION  0x00000040u

static inline int SDL_ShowSimpleMessageBox(uint32_t flags, const char *title, const char *message, void *window) {
    (void)flags;
    (void)window;
    if (title && message) {
        fprintf(stderr, "[%s] %s\n", title, message);
    }
    return 0;
}

/* -------------------- TOUCH EVENTS ---------------- */

static inline int SDL_GetNumTouchFingers(long long touchID) {
    (void)touchID;
    return 0; /* Brak multi-touch na Amidze */
}

#ifdef __cplusplus
}
#endif

#endif /* USE_SDL_STUBS */
#endif /* AMIGA_SDL_STUBS_H */