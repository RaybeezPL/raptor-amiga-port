/*
 * amiga_sdl_stubs.h - Minimal SDL2 type/macro stubs for AmigaOS 3.x port
 *
 * This header provides just enough SDL2 definitions to allow the Raptor
 * source code to compile without a real SDL2 library. Actual functionality
 * is implemented natively here via AmigaOS/RTG (Picasso96) APIs.
 *
 * This file is only used when USE_SDL_STUBS is defined (i.e., when no
 * real SDL2 Amiga port is available).
 *
 * ---------------------------------------------------------------------------
 * RTG (Picasso96) TARGETING NOTES
 * ---------------------------------------------------------------------------
 * This port strictly targets RTG (Picasso96) boards. The game does NOT use
 * the standard Workbench screen: it always opens its own dedicated custom
 * screen at the native game resolution (320x200, 8-bit chunky/CLUT).
 *
 * We deliberately do NOT #include <libraries/picasso96.h> or
 * <proto/Picasso96API.h> anywhere in this file, because those headers are
 * third-party (not part of the base AmigaOS 3.x NDK) and might simply be
 * missing from a minimal cross-compiler environment. Instead:
 *
 *   - Picasso96API.library is opened by raw name via OpenLibrary() only to
 *     detect whether an RTG board/driver is actually present at runtime.
 *     We do not call any p96*() functions directly (no vector table, no
 *     LVO offsets needed for that).
 *   - The actual custom screen/window is opened with completely standard,
 *     always-available graphics.library / intuition.library calls
 *     (BestModeID(), OpenScreenTagList(), OpenWindowTagList()) - these are
 *     part of the base NDK (proto/graphics.h, proto/intuition.h, already
 *     included below) and Picasso96 transparently hooks/patches them for
 *     its own RTG "friend" bitmaps. This gives full RTG support without any
 *     Picasso96 SDK headers or raw LVO offsets.
 *   - Pixel blit uses WriteChunkyPixels() - again a *standard*
 *     graphics.library v50+ call, which on an RTG system writes directly
 *     into the RTG bitmap's chunky buffer. A slow raw SetAPen()+WritePixel()
 *     fallback is provided for older graphics.library versions.
 *   - Palette is applied with LoadRGB32() - also a standard graphics.library
 *     call, patched by Picasso96 to update the RTG board's CLUT/gamma
 *     tables for 8-bit screens.
 *
 * If Picasso96API.library cannot be opened (no RTG board installed), we
 * fall back to plain Intuition custom-screen creation using whatever
 * default/best matching ModeID BestModeID() returns for the requested
 * dimensions/depth - this still works on AGA machines, just without RTG
 * acceleration.
 * ---------------------------------------------------------------------------
 */

#ifndef AMIGA_SDL_STUBS_H
#define AMIGA_SDL_STUBS_H

#ifdef USE_SDL_STUBS

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ========================================================================= */
/* AmigaOS Intuition / Graphics for real screen+window support               */
/* ========================================================================= */

#ifdef __AMIGA__
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>

/* Library bases needed by proto header inline stubs.                        */
/* __attribute__((weak)): safe if this header is included from multiple TUs. */
__attribute__((weak)) struct IntuitionBase *IntuitionBase = NULL;
__attribute__((weak)) struct GfxBase *GfxBase = NULL;

/* ------------------------------------------------------------------------- */
/* Picasso96API.library - RTG detection only (see file header comment for    */
/* the full rationale). We only need the pointer to know it opened OK; we    */
/* never call through it directly.                                          */
/* ------------------------------------------------------------------------- */
__attribute__((weak)) struct Library *P96Base = NULL;
__attribute__((weak)) int AmigaUsingP96 = 0;

/* ------------------------------------------------------------------------- */
/* Raw/inline BestModeID() tag values (normally in <graphics/displayinfo.h>, */
/* a standard AmigaOS 3.x NDK header, stable since Kickstart 2.04/           */
/* graphics.library v39). Defined raw/inline here so we don't depend on     */
/* that header being present either.                                        */
/* ------------------------------------------------------------------------- */
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

/* Game's fixed native resolution/depth. Always 320x200x8 - RTG target. */
#define AMIGA_GAME_WIDTH   320
#define AMIGA_GAME_HEIGHT  200
#define AMIGA_GAME_DEPTH   8

/* ------------------------------------------------------------------------- */
/* RTG screen/window state - the single dedicated custom screen we open for  */
/* the game. Declared weak at header scope so every TU that includes SDL.h   */
/* shares the same instance via the linker (same trick as IntuitionBase/     */
/* GfxBase above).                                                          */
/* ------------------------------------------------------------------------- */
__attribute__((weak)) struct Screen *AmigaGameScreen = NULL;

/* The single dedicated game window (matches AmigaGameScreen's only window).
 * Declared weak/shared here (same trick as IntuitionBase/GfxBase above) so
 * the native IDCMP event pump (Amiga_PumpWindowEvents(), further down this
 * file) can find the window's IDCMP message port from anywhere, without
 * having to thread an SDL_Window* through the generic SDL_PumpEvents(void)
 * call signature used by i_video.cpp's I_GetEvent(). */
__attribute__((weak)) struct Window *AmigaGameWindow = NULL;

/* Cached "pending" chunky pixel buffer + dimensions, set by SDL_LowerBlit()
 * each frame and consumed by SDL_RenderPresent() to perform the actual
 * hardware blit. This mirrors the real SDL2 flow (LowerBlit fills a
 * surface, RenderPresent flips it to the screen) while letting us do the
 * real Amiga blit at the correct "present" point in the frame. */
__attribute__((weak)) const uint8_t *AmigaPendingChunky = NULL;
__attribute__((weak)) int AmigaPendingW = 0;
__attribute__((weak)) int AmigaPendingH = 0;


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
static inline void Amiga_BlitScreen(struct Window *win, const uint8_t *chunky, int w, int h)
{
    if (!win || !win->RPort || !chunky) return;

    if (GfxBase && GfxBase->LibNode.lib_Version >= 50)
    {
        WriteChunkyPixels(win->RPort,
                           win->BorderLeft, win->BorderTop,
                           win->BorderLeft + w - 1, win->BorderTop + h - 1,
                           (UBYTE *)chunky, w);
    }
    else
    {
        int x, y;
        const uint8_t *row = chunky;
        for (y = 0; y < h; y++)
        {
            for (x = 0; x < w; x++)
            {
                SetAPen(win->RPort, row[x]);
                WritePixel(win->RPort, win->BorderLeft + x, win->BorderTop + y);
            }
            row += w;
        }
    }
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
    /* sdlcolors points at an array of {r,g,b,a} uint8_t structs (SDL_Color),
     * but SDL_Color isn't declared yet at this point in the header, so we
     * take a void* and reinterpret via a local byte-compatible struct. */
    struct RawColor { uint8_t r, g, b, a; };
    const struct RawColor *colors = (const struct RawColor *)sdlcolors;
    static ULONG table[1 + 256 * 3 + 1];
    int i;

    if (!scr || !colors || n <= 0) return;
    if (n > 256) n = 256;

    table[0] = ((ULONG)n << 16) | (ULONG)(first & 0xFFFF);
    for (i = 0; i < n; i++)
    {
        table[1 + i * 3 + 0] = (ULONG)colors[i].r << 24;
        table[1 + i * 3 + 1] = (ULONG)colors[i].g << 24;
        table[1 + i * 3 + 2] = (ULONG)colors[i].b << 24;
    }
    table[1 + n * 3] = 0; /* terminator */

    LoadRGB32(&scr->ViewPort, table);
}

/*
 * Amiga_HideSystemPointer / Amiga_ShowSystemPointer: hide or restore the
 * native Amiga hardware sprite mouse pointer on our game window.
 *
 * AmigaOS/Intuition has no direct "hide cursor" call (unlike modern OSes).
 * The standard, well-known trick (used by countless native Amiga programs)
 * is to install a completely blank 1x1 two-bitplane sprite image via
 * SetPointer() - this makes the hardware pointer sprite effectively
 * invisible while it's still technically "present" (so the game's own
 * software-drawn crosshair/cursor, blitted into the chunky screen buffer
 * by ptrapi.cpp, is the only cursor actually visible - fixing the reported
 * "two cursors visible at the same time" bug). ClearPointer() restores the
 * default system arrow pointer.
 *
 * The image data layout follows the classic Intuition sprite format: 2
 * reserved zero words, then (height) rows of 2 words each (one per
 * bitplane - both zero, i.e. fully transparent), then a mandatory
 * terminating zero word pair. All-zero data of the right size is all
 * that's needed for a blank/invisible pointer.
 */
static UWORD AmigaBlankPointerData[2 + 1 * 2 + 2] = { 0 };

static inline void Amiga_HideSystemPointer(void)
{
    if (AmigaGameWindow) {
        SetPointer(AmigaGameWindow, AmigaBlankPointerData, 1, 1, 0, 0);
    }
}

static inline void Amiga_ShowSystemPointer(void)
{
    if (AmigaGameWindow) {
        ClearPointer(AmigaGameWindow);
    }
}

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
/* Now with real struct bodies so we can hold Amiga window/screen pointers.   */
/* ========================================================================= */

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
#define SDL_SCANCODE_RCTRL      228
#define SDL_SCANCODE_RSHIFT     229
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
/* --- Window --- RTG (Picasso96) custom screen + borderless window          */
/*                                                                           */
/* Strictly targets RTG: opens a dedicated, custom 320x200x8 screen (never   */
/* Workbench) via Picasso96-if-present / plain Intuition-if-not, then a      */
/* borderless GimmeZeroZero window filling that screen for the game to draw  */
/* into. See file header comment for the full inline/raw Picasso96 rationale.*/
/* ========================================================================= */

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
        Amiga_BlitScreen(AmigaGameScreen->FirstWindow, AmigaPendingChunky,
                         AmigaPendingW, AmigaPendingH);
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
     * Palette-indexed 8-bit surface â†’ 32-bit ARGB surface conversion.
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

#ifdef __AMIGA__
    /*
     * Cache the source 8-bit chunky buffer pointer + dimensions for the
     * actual RTG screen blit, which happens later at SDL_RenderPresent()
     * time (the real "flip" point in the frame). The ARGB conversion above
     * is kept only so the rest of the generic i_video.cpp code path
     * continues to work unmodified; the genuinely visible on-screen update
     * is the raw chunky blit performed in SDL_RenderPresent().
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

/* --- Mouse button constants ---
 * (Moved above the native event pump below, since Amiga_PumpWindowEvents()
 * needs these already defined when translating IDCMP_MOUSEBUTTONS codes.) */
#define SDL_BUTTON_LEFT     1
#define SDL_BUTTON_MIDDLE   2
#define SDL_BUTTON_RIGHT    3

/* SDL_ShowCursor() argument/return values (real SDL2 numbering). */
#define SDL_DISABLE 0
#define SDL_ENABLE  1
#define SDL_QUERY   -1


/* ========================================================================= */
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
/* ========================================================================= */

#define AMIGA_SDL_EVENT_QUEUE_SIZE 64

__attribute__((weak)) SDL_Event AmigaEventQueue[AMIGA_SDL_EVENT_QUEUE_SIZE];
__attribute__((weak)) int AmigaEventQueueHead = 0;
__attribute__((weak)) int AmigaEventQueueTail = 0;

/* Current mouse position (window-relative) + button bitmask (bit0=left,
 * bit1=right, bit2=middle), updated live by Amiga_PumpWindowEvents() and
 * read back by SDL_GetMouseState()/SDL_GetRelativeMouseState() below. */
__attribute__((weak)) int AmigaMouseX = 0;
__attribute__((weak)) int AmigaMouseY = 0;
__attribute__((weak)) int AmigaMouseButtons = 0;

static inline void Amiga_PushEvent(const SDL_Event *ev)
{
    int next = (AmigaEventQueueTail + 1) % AMIGA_SDL_EVENT_QUEUE_SIZE;
    if (next == AmigaEventQueueHead) {
        /* Queue full: drop the oldest event to make room for the newest
         * (most relevant/current) input rather than losing new input. */
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
    /* 0x5C */ SDL_SCANCODE_KP_DIVIDE,
    /* 0x5D */ SDL_SCANCODE_KP_MULTIPLY,
    /* 0x5E */ SDL_SCANCODE_KP_PLUS,
    /* 0x5F */ 0,
    /* 0x60 */ SDL_SCANCODE_LSHIFT,
    /* 0x61 */ SDL_SCANCODE_RSHIFT,
    /* 0x62 */ SDL_SCANCODE_CAPSLOCK,
    /* 0x63 */ SDL_SCANCODE_LCTRL,
    /* 0x64 */ SDL_SCANCODE_LALT,
    /* 0x65 */ SDL_SCANCODE_RALT,
    /* 0x66 */ 0,
    /* 0x67 */ 0,
};

/*
 * Amiga_PumpWindowEvents() - drains the game window's IDCMP message port
 * (GetMsg/ReplyMsg, the standard AmigaOS Intuition event loop pattern) and
 * translates each IntuiMessage into an equivalent SDL_Event pushed onto
 * our ring buffer for SDL_PollEvent() to hand out.
 */
static inline void Amiga_PumpWindowEvents(void)
{
    struct IntuiMessage *imsg;

    if (!AmigaGameWindow || !AmigaGameWindow->UserPort)
        return;

    while ((imsg = (struct IntuiMessage *)GetMsg(AmigaGameWindow->UserPort)) != NULL)
    {
        ULONG mclass = imsg->Class;
        UWORD mcode  = imsg->Code;
        UWORD mqual  = imsg->Qualifier;
        WORD  mx     = imsg->MouseX;
        WORD  my     = imsg->MouseY;

        /* Copy out everything we need BEFORE ReplyMsg(): Intuition is free
         * to recycle/reuse the message the instant it has been replied. */
        ReplyMsg((struct Message *)imsg);

        /* MouseX/MouseY are valid on every single IntuiMessage regardless
         * of class, so we get an up-to-date pointer position for free on
         * every event, not just IDCMP_MOUSEMOVE ones. */
        AmigaMouseX = mx;
        AmigaMouseY = my;

        switch (mclass)
        {
        case IDCMP_CLOSEWINDOW:
        {
            SDL_Event ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = SDL_QUIT;
            printf("[AMIGA] IDCMP_CLOSEWINDOW received -> SDL_QUIT\n"); fflush(stdout);
            Amiga_PushEvent(&ev);
            break;
        }

        case IDCMP_RAWKEY:
        {
            int up  = (mcode & IECODE_UP_PREFIX) ? 1 : 0;
            int raw = mcode & ~IECODE_UP_PREFIX;
            int scancode = 0;

            if (raw >= 0 && raw < (int)(sizeof(AmigaRawKeyToScancode) / sizeof(AmigaRawKeyToScancode[0])))
                scancode = AmigaRawKeyToScancode[raw];

            if (scancode != 0)
            {
                SDL_Event ev;
                memset(&ev, 0, sizeof(ev));
                ev.type = up ? SDL_KEYUP : SDL_KEYDOWN;
                ev.key.type = ev.type;
                ev.key.keysym.scancode = scancode;
                ev.key.keysym.sym = scancode;
                ev.key.keysym.mod = 0;
                if (mqual & IEQUALIFIER_LALT) ev.key.keysym.mod |= KMOD_LALT;
                if (mqual & IEQUALIFIER_RALT) ev.key.keysym.mod |= KMOD_RALT;
                Amiga_PushEvent(&ev);
            }
            else
            {
                printf("[AMIGA] IDCMP_RAWKEY: unmapped raw code 0x%02x (%s)\n",
                       raw, up ? "up" : "down");
                fflush(stdout);
            }
            break;
        }

        case IDCMP_MOUSEBUTTONS:
        {
            int button = 0, down = 0;

            switch (mcode)
            {
            case SELECTDOWN: button = SDL_BUTTON_LEFT;   down = 1; break;
            case SELECTUP:   button = SDL_BUTTON_LEFT;   down = 0; break;
            case MENUDOWN:   button = SDL_BUTTON_RIGHT;  down = 1; break;
            case MENUUP:     button = SDL_BUTTON_RIGHT;  down = 0; break;
            case MIDDLEDOWN: button = SDL_BUTTON_MIDDLE; down = 1; break;
            case MIDDLEUP:   button = SDL_BUTTON_MIDDLE; down = 0; break;
            default: break;
            }

            if (button)
            {
                SDL_Event ev;
                memset(&ev, 0, sizeof(ev));
                ev.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
                ev.button.type = ev.type;
                ev.button.button = (uint8_t)button;
                ev.button.state = (uint8_t)down;
                ev.button.x = mx;
                ev.button.y = my;

                if (button == SDL_BUTTON_LEFT)   { if (down) AmigaMouseButtons |= 1; else AmigaMouseButtons &= ~1; }
                if (button == SDL_BUTTON_RIGHT)  { if (down) AmigaMouseButtons |= 2; else AmigaMouseButtons &= ~2; }
                if (button == SDL_BUTTON_MIDDLE) { if (down) AmigaMouseButtons |= 4; else AmigaMouseButtons &= ~4; }

                Amiga_PushEvent(&ev);
            }
            break;
        }

        case IDCMP_MOUSEMOVE:
            /* Position already latched into AmigaMouseX/Y above; the game
             * polls the mouse position on demand every frame via
             * I_GetMousePos() -> SDL_GetMouseState() rather than needing a
             * discrete SDL_MOUSEMOTION event pushed here. */
            break;

        default:
            break;
        }
    }
}

#else /* !__AMIGA__ */

static inline void Amiga_PumpWindowEvents(void) { }

#endif /* __AMIGA__ */

/* --- Events ---
 *
 * SDL_PumpEvents() drains the real Amiga IDCMP message port (via
 * Amiga_PumpWindowEvents() above) into our internal SDL_Event ring buffer;
 * SDL_PollEvent() then just pops events back out of that buffer, exactly
 * like real SDL2 does internally. This is what makes I_GetEvent()'s
 * SDL_PumpEvents() + while(SDL_PollEvent()) loop in i_video.cpp actually
 * see real keyboard/mouse/window-close events instead of nothing.
 */
static inline void   SDL_PumpEvents(void) { Amiga_PumpWindowEvents(); }
static inline int    SDL_PollEvent(SDL_Event *e) { return Amiga_PopEvent(e); }

/* --- Touch --- */
static inline int    SDL_GetNumTouchFingers(int64_t touchId) { (void)touchId; return 0; }

/* --- Mouse ---
 *
 * SDL_GetMouseState()/SDL_GetRelativeMouseState() now report the real,
 * live mouse position + button bitmask kept up to date by
 * Amiga_PumpWindowEvents() above (fed by every IDCMP message's
 * MouseX/MouseY fields and IDCMP_MOUSEBUTTONS codes), instead of always
 * reporting (0,0)/no buttons.
 */
/*
 * SDL_SetRelativeMouseMode: i_video.cpp's SetShowCursor() calls this with
 * !show to hide the cursor (relative mode implicitly hides the pointer on
 * every other SDL2 backend). On Amiga we don't have a real relative/warp
 * mouse mode, but we DO need the hide/show side effect - this is one of
 * the two places (along with SDL_ShowCursor() below) that must hide the
 * native Amiga hardware sprite pointer, or else it stays visible on top
 * of the game's own software-drawn crosshair (the reported "two cursors"
 * bug). SDL_TRUE (relative/hidden) -> hide; SDL_FALSE -> show.
 */
static inline int    SDL_SetRelativeMouseMode(SDL_bool e) {
#ifdef __AMIGA__
    if (e) Amiga_HideSystemPointer();
    else   Amiga_ShowSystemPointer();
#else
    (void)e;
#endif
    return 0;
}


static inline uint32_t SDL_GetRelativeMouseState(int *x, int *y) {
    static int last_x = 0, last_y = 0;
    int dx = AmigaMouseX - last_x;
    int dy = AmigaMouseY - last_y;
    last_x = AmigaMouseX;
    last_y = AmigaMouseY;
    if (x) *x = dx;
    if (y) *y = dy;
    return (uint32_t)AmigaMouseButtons;
}

static inline uint32_t SDL_GetMouseState(int *x, int *y) {
    if (x) *x = AmigaMouseX;
    if (y) *y = AmigaMouseY;
    return (uint32_t)AmigaMouseButtons;
}

/*
 * SDL_ShowCursor: the other call site (i_video.cpp's I_ShutdownGraphics()
 * calls SetShowCursor(true) directly, and the #else branch of
 * SetShowCursor() - kept for parity with upstream desktop ports - calls
 * this directly with the raw show/hide flag) that must actually hide/show
 * the real Amiga hardware sprite pointer. Accepts real SDL2 semantics:
 * SDL_DISABLE (0) hides, SDL_ENABLE (1) shows, SDL_QUERY (-1) just queries
 * current state without changing it. Returns the previous (or current, for
 * SDL_QUERY) shown state, exactly like real SDL2.
 */
static inline int    SDL_ShowCursor(int t) {
    static int shown = 1; /* SDL2 default: cursor visible until told otherwise */
    int prev = shown;

    if (t == SDL_QUERY)
        return prev;

#ifdef __AMIGA__
    if (t == SDL_DISABLE) Amiga_HideSystemPointer();
    else                  Amiga_ShowSystemPointer();
#endif

    shown = (t != SDL_DISABLE);
    return prev;
}


static inline void   SDL_WarpMouseInWindow(SDL_Window *w, int x, int y) {
    (void)w;
    /* Keep our tracked mouse position consistent with an explicit warp
     * (e.g. I_SetMousePos() clamping the cursor to the screen edges),
     * since on Amiga there's no hardware call to actually move the real
     * pointer sprite position under program control the way SDL's
     * relative-mode warp does on other platforms. */
    AmigaMouseX = x;
    AmigaMouseY = y;
}


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
