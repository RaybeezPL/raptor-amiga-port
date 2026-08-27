/* Minimal SDL2 type/macro stubs for AmigaOS 3.x port. Active with USE_SDL_STUBS. Targets RTG (Picasso96) 320x200x8 custom screen. Uses WriteChunkyPixels for blitting and LoadRGB32 for palette. Shared globals are defined in amiga_stubs_impl.cpp. */

#ifndef AMIGA_SDL_STUBS_H
#define AMIGA_SDL_STUBS_H

#ifdef USE_SDL_STUBS

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* Minimal Amiga video/window diagnostics logger (AmigaLog).
 * Writes one line to the console (stdout) only - no log file is created.
 * To capture the diagnostics to a file, redirect stdout when starting
 * from a Shell, e.g.:  raptor > RAPTOR.LOG
 * Note that Workbench launches redirect stdout to NIL: (see rap.cpp),
 * so no diagnostics are visible in that case. */
static inline void AmigaLog(const char *fmt, ...)
{
    va_list ap;
    char line[256];

    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);

    printf("%s\n", line);
    fflush(stdout);
}

/* AmigaOS Intuition/Graphics support. */

#ifdef __AMIGA__
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/displayinfo.h>
#include <proto/lowlevel.h>
#include <libraries/lowlevel.h>
#include <proto/Picasso96.h>   /* Official P96 SDK prototypes+inline stubs (installed via /opt/amiga toolchain p96.sdk). */


/* Global storage pattern: AMIGA_STUBS_OWNER defines actual instance, others use extern to guarantee a single instance. */
#ifdef AMIGA_STUBS_OWNER
#  define AMIGA_STUBS_DECL          /* Plain definition. */
#  define AMIGA_STUBS_INIT(v) = v   /* With initializer. */
#else
#  define AMIGA_STUBS_DECL    extern
#  define AMIGA_STUBS_INIT(v)       /* No initializer in extern decl. */
#endif

/* Library bases. */
AMIGA_STUBS_DECL struct IntuitionBase *IntuitionBase AMIGA_STUBS_INIT(NULL);
AMIGA_STUBS_DECL struct GfxBase       *GfxBase       AMIGA_STUBS_INIT(NULL);
AMIGA_STUBS_DECL struct Library       *LowLevelBase  AMIGA_STUBS_INIT(NULL);

/* Set by the -nojoy command line switch (rap.cpp) to hard-disable all
 * joystick polling. Handy for tracking down phantom input on real
 * hardware without recompiling anything. */
AMIGA_STUBS_DECL int AmigaJoyDisabled AMIGA_STUBS_INIT(0);

/* Set by the -nomouse command line switch (rap.cpp) to hard-disable all
 * mouse handling (IDCMP mouse events, cursor, in-game mouse steering).
 * A performance/troubleshooting aid: with the mouse disabled the IDCMP
 * window mask omits MOUSEMOVE/MOUSEBUTTONS, so no mouse events are
 * registered or processed at all. */
AMIGA_STUBS_DECL int AmigaMouseDisabled AMIGA_STUBS_INIT(0);

/* Shared joystick state polled in SDL_PumpEvents and read by controller APIs. */
AMIGA_STUBS_DECL ULONG AmigaJoyState AMIGA_STUBS_INIT(0);

/* Picasso96API.library used for RTG detection only (no function calls). */
AMIGA_STUBS_DECL struct Library *P96Base AMIGA_STUBS_INIT(NULL);
AMIGA_STUBS_DECL int AmigaUsingP96 AMIGA_STUBS_INIT(0);

/* cybergraphics.library used for CGX fallback when P96 is absent or fails. */
AMIGA_STUBS_DECL struct Library *CyberGfxBase AMIGA_STUBS_INIT(NULL);
AMIGA_STUBS_DECL int AmigaUsingCGX AMIGA_STUBS_INIT(0);

/* GFX display mode, selected at runtime via the GFX keyword (CLI "-gfx=..."
 * or icon ToolType "GFX=..."):
 *   AUTO (default) - try RTG (Picasso96) first; if RTG is unavailable, show
 *                    an English requester and DO NOT start (no silent fallback).
 *   RTG            - same as AUTO (RTG required, no silent fallback).
 *   AGA            - force a native chipset 320x200x8 screen. */
#define AMIGA_GFX_AUTO 0
#define AMIGA_GFX_RTG  1
#define AMIGA_GFX_AGA  2
AMIGA_STUBS_DECL int AmigaGfxMode AMIGA_STUBS_INIT(AMIGA_GFX_AUTO);

/* Set to 1 when an RTG 320x240x8 screen was opened instead of 320x200x8:
 * the game draws its 320x200 image at the top of the screen, the bottom
 * 40 rows stay black, and mouse Y is clamped to the 0..199 play area. */
AMIGA_STUBS_DECL int AmigaRTGLetterbox AMIGA_STUBS_INIT(0);

/* Active frame-blit path, selected once in Amiga_OpenGameScreen:
 *   AMIGA_BLIT_WCP     - graphics.library WriteChunkyPixels (generic fallback)
 *   AMIGA_BLIT_P96     - p96WritePixelArray(RGBFB_CLUT)    (Picasso96 fast path)
 *   AMIGA_BLIT_CGX     - CGX WritePixelArray(RECTFMT_LUT8) (CyberGraphX fast path)
 *   AMIGA_BLIT_AGA_C2P - custom 68060 chunky->planar C2P into the screen bitmap
 */
#define AMIGA_BLIT_WCP      0
#define AMIGA_BLIT_P96      1
#define AMIGA_BLIT_CGX      2
#define AMIGA_BLIT_AGA_C2P  3
AMIGA_STUBS_DECL int AmigaBlitMode AMIGA_STUBS_INIT(AMIGA_BLIT_WCP);

/* Physical screen parameters actually opened (filled in by
* Amiga_OpenGameScreen). The game always draws to a logical 320x200
* chunky buffer; only the final blit knows the physical mode. */
AMIGA_STUBS_DECL int AmigaPhysW AMIGA_STUBS_INIT(320);
AMIGA_STUBS_DECL int AmigaPhysH AMIGA_STUBS_INIT(200);
AMIGA_STUBS_DECL int AmigaPhysDepth AMIGA_STUBS_INIT(8);

#ifndef INVALID_ID
#define INVALID_ID 0xFFFFFFFFUL
#endif
#ifndef MONITOR_ID_MASK
#define MONITOR_ID_MASK 0xFFFF0000UL
#endif
#ifndef DEFAULT_MONITOR_ID
#define DEFAULT_MONITOR_ID 0x00000000UL
#endif

/* Fixed game resolution for RTG (320x200x8). */
#define AMIGA_GAME_WIDTH 320
#define AMIGA_GAME_HEIGHT 200
#define AMIGA_GAME_DEPTH 8

/* Single RTG screen/window instance shared across all TUs. */
AMIGA_STUBS_DECL struct Screen *AmigaGameScreen AMIGA_STUBS_INIT(NULL);
AMIGA_STUBS_DECL struct Window *AmigaGameWindow AMIGA_STUBS_INIT(NULL);

/* Pending chunky blit buffer set by SDL_LowerBlit and consumed by SDL_RenderPresent. */
AMIGA_STUBS_DECL const uint8_t *AmigaPendingChunky AMIGA_STUBS_INIT(NULL);
AMIGA_STUBS_DECL int AmigaPendingW AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL int AmigaPendingH AMIGA_STUBS_INIT(0);

/* Opens Picasso96API.library to detect RTG presence. Returns 1 if available, 0 for AGA/ECS fallback. */
static inline int Amiga_OpenP96(void)
{
    if (P96Base != NULL) {
        return 1;
    }

    /* Raw OpenLibrary call without SDK header dependencies. */
    P96Base = OpenLibrary((CONST_STRPTR)"Picasso96API.library", 0);

    if (P96Base) {
        AmigaUsingP96 = 1;
        AmigaLog("[VIDEO] P96 probe: OpenLibrary(Picasso96API.library,0) -> OK");
        return 1;
    }

    AmigaUsingP96 = 0;
    AmigaLog("[VIDEO] P96 probe: OpenLibrary(Picasso96API.library,0) -> absent (native chipset fallback allowed)");
    return 0;
}

static inline void Amiga_CloseP96(void)
{
    if (P96Base) {
        CloseLibrary(P96Base);
        P96Base = NULL;
    }
    AmigaUsingP96 = 0;
}

/* Opens cybergraphics.library to detect CGX RTG. Returns 1 if available, 0 if absent. */
static inline int Amiga_OpenCGX(void)
{
    if (CyberGfxBase != NULL) {
        return 1;
    }

    CyberGfxBase = OpenLibrary((CONST_STRPTR)"cybergraphics.library", 0);

    if (CyberGfxBase) {
        AmigaUsingCGX = 1;
        AmigaLog("[VIDEO] CGX probe: OpenLibrary(cybergraphics.library,0) -> OK");
        return 1;
    }

    AmigaUsingCGX = 0;
    AmigaLog("[VIDEO] CGX probe: OpenLibrary(cybergraphics.library,0) -> absent");
    return 0;
}

static inline void Amiga_CloseCGX(void)
{
    if (CyberGfxBase) {
        CloseLibrary(CyberGfxBase);
        CyberGfxBase = NULL;
    }
    AmigaUsingCGX = 0;
}

static inline int Amiga_IsNativeChipsetMode(ULONG modeid)
{
    /* Native chipset monitors: default (0x0000), NTSC (0x0001), PAL (0x0002).
     * Any other monitor ID (e.g. an RTG board's P96 monitor like 0x5007) is
     * treated as non-native, so the RTG path never mistakes a native AGA/ECS
     * MonitorID for a real P96 mode. Uses direct mask+compare to avoid any
     * compiler-local issue with intermediate ULONG variables. */
    return ((modeid & 0xFFFF0000UL) == 0x00000000UL) ||
           ((modeid & 0xFFFF0000UL) == 0x00010000UL) ||   /* NTSC */
           ((modeid & 0xFFFF0000UL) == 0x00020000UL);     /* PAL  */
}

/* Selects an RTG display ModeID from the live P96 mode list that matches the
 * requested logical resolution EXACTLY (width x height x depth). Returns the
 * DisplayID of the first exact P96 match, or INVALID_ID when no such real RTG
 * mode exists. Unlike BestModeID, it never returns a native chipset ModeID:
 * the candidate always comes straight from the P96 mode list. The list is
 * always released with p96FreeModeList() on every path. */
static inline ULONG Amiga_FindP96GameMode(int width, int height, int depth)
{
    struct TagItem tags[] = { TAG_DONE };
    struct List *ml = p96AllocModeListTagList(tags);

    if (!ml) {
        return INVALID_ID;
    }

    {
        struct P96Mode *mn;
        ULONG found = INVALID_ID;

        for (mn = (struct P96Mode *)(ml->lh_Head);
             mn->Node.ln_Succ;
             mn = (struct P96Mode *)mn->Node.ln_Succ) {
            ULONG mid = mn->DisplayID;

            /* Extra guard: a real P96 mode is never on a native chipset monitor,
             * but reject 0x0000/0x0001/0x0002 just in case. */
            if (Amiga_IsNativeChipsetMode(mid))
                continue;

            if ((ULONG)p96GetModeIDAttr(mid, P96IDA_WIDTH)  == (ULONG)width &&
                (ULONG)p96GetModeIDAttr(mid, P96IDA_HEIGHT) == (ULONG)height &&
                (ULONG)p96GetModeIDAttr(mid, P96IDA_DEPTH)  == (ULONG)depth) {
                found = mid;
                break;
            }
        }

        p96FreeModeList(ml);
        return found;
    }
}

/*--- CGX (CyberGraphX/cybergraphics.library) fallback definitions -------------*/

/* CyberModeNode matches cybergraphics.library AllocCModeTagList nodes.
 * Width/Height/Depth are pre-filled by the library; no CModeIDtoTags needed. */
struct CyberModeNode
{
    struct Node Node;
    char   ModeText[DISPLAYNAMELEN];
    ULONG  DisplayID;
    UWORD  Width;
    UWORD  Height;
    UWORD  Depth;
    struct TagItem *DisplayTagList;
};

/* CGX inline helpers using the LP1/LP1NR pattern (same as P96 SDK uses).
 * CyberGfxBase must be open before calling these.
 * Offsets confirmed from CGraphX-DevKit Release VI FD file (bias 30). */
#define CGX_AllocCModeTagList(tags) \
    LP1(0x48, struct List *, CGX_AllocModeList, struct TagItem *, (tags), a1, , CyberGfxBase)

#define CGX_FreeCModeList(ml) \
    LP1NR(0x4E, CGX_FreeModeList, struct List *, (ml), a0, , CyberGfxBase)

#define CGX_BestCModeIDTagList(tags) \
    LP1(0x3C, ULONG, CGX_BestModeID, struct TagItem *, (tags), a0, , CyberGfxBase)

/* CGX WritePixelArray: fast chunky->CLUT8 blit done by the CGX driver.
 * LVO 0x7E (-126) and register assignment verified against the official
 * CGraphX-DevKit VI files (cybergraphics_lib.fd + inline/cybergraphics.h)
 * and the compiled devkit example binary. RECTFMT_LUT8 = 3 per the official
 * cybergraphx/cybergraphics.h (the minimal NDK stub header says 0 - wrong). */
#define AMIGA_CGX_RECTFMT_LUT8 3
#define CGX_WritePixelArray(src, sx, sy, smod, rp, dx, dy, w, h, fmt) \
    LP10(0x7E, ULONG, CGX_WritePixArray, \
         APTR, (src), a0, \
         UWORD, (sx), d0, \
         UWORD, (sy), d1, \
         UWORD, (smod), d2, \
         struct RastPort *, (rp), a1, \
         UWORD, (dx), d3, \
         UWORD, (dy), d4, \
         UWORD, (w), d5, \
         UWORD, (h), d6, \
         UBYTE, (fmt), d7, \
         , CyberGfxBase)

/* Selects a CGX display ModeID from the live cybergraphics.library mode list
 * that matches the requested resolution EXACTLY (width x height x depth).
 * Returns DisplayID of the first exact match, or INVALID_ID.
 * Unlike BestModeID/BestCModeID, it never returns a native chipset ModeID:
 * the candidate always comes straight from the CGX mode list and is filtered
 * through Amiga_IsNativeChipsetMode. */
static inline ULONG Amiga_FindCGXGameMode(int width, int height, int depth)
{
    struct TagItem tags[] = { TAG_DONE };
    struct List *ml;

    if (!CyberGfxBase)
        return INVALID_ID;

    ml = CGX_AllocCModeTagList(tags);

    if (!ml) {
        return INVALID_ID;
    }

    {
        struct CyberModeNode *mn;
        ULONG found = INVALID_ID;

        for (mn = (struct CyberModeNode *)(ml->lh_Head);
             mn->Node.ln_Succ;
             mn = (struct CyberModeNode *)mn->Node.ln_Succ) {
            ULONG mid = mn->DisplayID;

            /* Extra guard: reject native chipset monitors just in case. */
            if (Amiga_IsNativeChipsetMode(mid))
                continue;

            if ((int)mn->Width == width &&
                (int)mn->Height == height &&
                (int)mn->Depth == depth) {
                found = mid;
                break;
            }
        }

        CGX_FreeCModeList(ml);
        return found;
    }
}

/*----------------------------------------------------------------------------*/

/* Shows an English system requester explaining that the game needs an RTG
 * (Picasso96) display when GFX=AUTO/RTG but no RTG mode is available. The
 * game does NOT fall back to AGA; it aborts instead. */
static inline void Amiga_ShowRtgRequester(const char *detail)
{
    char body[400];
    struct EasyStruct es;

    snprintf(body, sizeof(body),
        "Raptor requires an RTG (Picasso96) display for the selected GFX mode.\r\n\r\n"
        "%s\r\n\r\n"
        "Please install a Picasso96 driver that offers a 320x200x8 or "
        "320x240x8 RTG mode (GFX=AUTO/RTG), or run the game on the classic "
        "chipset screen with:\r\n"
        "  GFX=AGA   (CLI: -gfx=AGA)",
        detail ? detail : "");

    memset(&es, 0, sizeof(es));
    es.es_StructSize = sizeof(es);
    es.es_Flags = 0;
    es.es_Title = "Raptor - RTG required";
    es.es_TextFormat = body;
    es.es_GadgetFormat = "OK";

    AmigaLog("[VIDEO] requester shown: RTG display required (%s)",
             detail ? detail : "");
    EasyRequestArgs(NULL, &es, NULL, NULL);
}

/* Forward decls: helpers used inside Amiga_OpenGameScreen. */
static inline struct Screen* Amiga_OpenRTGScreenByModeid(ULONG modeid, int wantLetterbox,
                                                         const char *label);
static inline void Amiga_CloseGameScreen(void);

/* Opens the game screen (logical 320x200, 8-bit).
 *
 * GFX mode:
 *  - AMIGA_GFX_AGA : native chipset 320x200x8 screen (no RTG required).
 *  - AUTO / RTG    : requires RTG. Tries P96 (Picasso96) first; if P96
 *                    is absent or does not yield a usable screen, falls
 *                    back to CGX (cybergraphics.library). In both RTG paths
 *                    the code tries 320x200x8 first, then 320x240x8 (letter-
 *                    boxed). If neither P96 nor CGX produces a screen, an
 *                    English requester is shown and NULL is returned (no
 *                    silent fallback to AGA). */
static inline struct Screen* Amiga_OpenGameScreen(int gw, int gh, int gdepth)
{
    ULONG modeid;

    if (AmigaGameScreen) {
        return AmigaGameScreen;
    }

    AmigaLog("[VIDEO] === Amiga_OpenGameScreen: GFX=%s (logical %dx%dx%d) ===",
             AmigaGfxMode == AMIGA_GFX_AGA ? "AGA" :
             (AmigaGfxMode == AMIGA_GFX_RTG ? "RTG" : "AUTO"),
             gw, gh, gdepth);

    /* Generic OS blit until a faster path is selected below. */
    AmigaBlitMode = AMIGA_BLIT_WCP;

    if (AmigaGfxMode == AMIGA_GFX_AGA)
    {
        /* Native chipset 320x200x8 custom screen (no RTG required). */
        AmigaRTGLetterbox = 0;
        AmigaGameScreen = OpenScreenTags(NULL,
            SA_Width, (ULONG)AMIGA_GAME_WIDTH,
            SA_Height, (ULONG)AMIGA_GAME_HEIGHT,
            SA_Depth, (ULONG)AMIGA_GAME_DEPTH,
            SA_Quiet, TRUE,
            SA_ShowTitle, FALSE,
            SA_Draggable, FALSE,
            SA_Exclusive, TRUE,
            SA_Type, CUSTOMSCREEN,
            TAG_DONE);

        AmigaLog("[VIDEO] AGA: OpenScreenTags 320x200x8 -> %s",
                 AmigaGameScreen ? "OK" : "FAIL");
        if (!AmigaGameScreen) {
            return NULL;
        }

        AmigaPhysW = AmigaGameScreen->Width;
        AmigaPhysH = AmigaGameScreen->Height;
        AmigaPhysDepth = AMIGA_GAME_DEPTH;
        AmigaLog("[VIDEO] screen opened OK: actual %dx%dx%d, mode=AGA, letterbox=0",
                 AmigaPhysW, AmigaPhysH, AmigaPhysDepth);
        SetRast(&AmigaGameScreen->RastPort, 0);
        AmigaBlitMode = AMIGA_BLIT_AGA_C2P;
        AmigaLog("[VIDEO] blit path: custom 68060 C2P -> bitplanes");
        return AmigaGameScreen;
    }

    /* AUTO / RTG (strict): RTG required, no silent fallback to AGA. */

    /* --- P96 (Picasso96) first attempt --- */
    if (Amiga_OpenP96()) {
        AmigaLog("[VIDEO] RTG: searching P96 mode 320x200x8");
        modeid = Amiga_FindP96GameMode(AMIGA_GAME_WIDTH, AMIGA_GAME_HEIGHT, AMIGA_GAME_DEPTH);
        if (modeid != INVALID_ID) {
            AmigaLog("[VIDEO] RTG: selected P96 mode 0x%08lx for 320x200x8", modeid);
            if (Amiga_OpenRTGScreenByModeid(modeid, 0, "RTG[P96]")) {
                AmigaBlitMode = AMIGA_BLIT_P96;
                AmigaLog("[VIDEO] blit path: p96WritePixelArray (RGBFB_CLUT)");
                return AmigaGameScreen;
            }
        }
        else {
            AmigaLog("[VIDEO] RTG: 320x200x8 unavailable, trying 320x240x8");
            modeid = Amiga_FindP96GameMode(320, 240, 8);
            if (modeid != INVALID_ID) {
                AmigaLog("[VIDEO] RTG: selected P96 mode 0x%08lx for 320x240x8", modeid);
                if (Amiga_OpenRTGScreenByModeid(modeid, 1, "RTG[P96]")) {
                    AmigaBlitMode = AMIGA_BLIT_P96;
                    AmigaLog("[VIDEO] blit path: p96WritePixelArray (RGBFB_CLUT)");
                    return AmigaGameScreen;
                }
            }
            else {
                AmigaLog("[VIDEO] RTG: no matching P96 mode found – will try CGX");
            }
        }
    }
    /* If P96 failed at any point (absent, no mode, OpenScreen failed
     * or bad dimensions), clean up and try the CGX fallback. */
    Amiga_CloseGameScreen();     /* safe if AmigaGameScreen is already NULL */
    Amiga_CloseP96();

    /* --- CGX (CyberGraphX) fallback --- */
    if (Amiga_OpenCGX()) {
        AmigaLog("[VIDEO] CGX: searching mode 320x200x8");
        modeid = Amiga_FindCGXGameMode(AMIGA_GAME_WIDTH, AMIGA_GAME_HEIGHT, AMIGA_GAME_DEPTH);
        if (modeid != INVALID_ID) {
            AmigaLog("[VIDEO] CGX: selected mode 0x%08lx for 320x200x8", modeid);
            if (Amiga_OpenRTGScreenByModeid(modeid, 0, "RTG[CGX]")) {
                AmigaBlitMode = AMIGA_BLIT_CGX;
                AmigaLog("[VIDEO] blit path: CGX WritePixelArray (RECTFMT_LUT8)");
                return AmigaGameScreen;
            }
        }
        else {
            AmigaLog("[VIDEO] CGX: 320x200x8 unavailable, trying 320x240x8");
            modeid = Amiga_FindCGXGameMode(320, 240, 8);
            if (modeid != INVALID_ID) {
                AmigaLog("[VIDEO] CGX: selected mode 0x%08lx for 320x240x8", modeid);
                if (Amiga_OpenRTGScreenByModeid(modeid, 1, "RTG[CGX]")) {
                    AmigaBlitMode = AMIGA_BLIT_CGX;
                    AmigaLog("[VIDEO] blit path: CGX WritePixelArray (RECTFMT_LUT8)");
                    return AmigaGameScreen;
                }
            }
            else {
                AmigaLog("[VIDEO] CGX: no matching mode found");
            }
        }
    }

    /* Neither P96 nor CGX produced a usable screen. */
    Amiga_ShowRtgRequester("No 320x200x8 or 320x240x8 RTG mode was found "
                           "(tried Picasso96 and CyberGraphX).");
    AmigaLog("[VIDEO] RTG: no P96 or CGX screen -> not starting");
    return NULL;
}

/* Shared helper: opens the RTG screen via OpenScreenTags with the given
 * ModeID and applies the standard dimension guard. Returns the screen on
 * success, NULL on failure (with internal requester for bad dimensions).
 * 'label' is a short driver name used for log messages only. */
static inline struct Screen* Amiga_OpenRTGScreenByModeid(ULONG modeid, int wantLetterbox,
                                                         const char *label)
{
    AmigaGameScreen = OpenScreenTags(NULL,
        SA_Depth, (ULONG)AMIGA_GAME_DEPTH,
        SA_DisplayID, modeid,
        SA_Quiet, TRUE,
        SA_ShowTitle, FALSE,
        SA_Draggable, FALSE,
        SA_Exclusive, TRUE,
        SA_Type, CUSTOMSCREEN,
        TAG_DONE);

    AmigaLog("[VIDEO] %s: OpenScreenTags modeid=0x%08lx -> %s",
             label, modeid, AmigaGameScreen ? "OK" : "FAIL");
    if (!AmigaGameScreen) {
        return NULL;
    }

    AmigaPhysW = AmigaGameScreen->Width;
    AmigaPhysH = AmigaGameScreen->Height;
    AmigaPhysDepth = AMIGA_GAME_DEPTH;

    /* Derive letterbox from the actual screen height (not the requested
     * one – the matched mode may legitimately be 320x240 instead of the
     * requested 320x200). */
    AmigaRTGLetterbox = (AmigaPhysW == 320 && AmigaPhysH == 240);

    AmigaLog("[VIDEO] screen opened OK: actual %dx%dx%d, mode=%s, letterbox=%d",
             AmigaPhysW, AmigaPhysH, AmigaPhysDepth, label, AmigaRTGLetterbox);

    /* Guard: only accept 320x200 or 320x240. Anything else is an unexpected
     * mode reported by the driver – close it and fail. */
    if (AmigaPhysW != 320 || (AmigaPhysH != 200 && AmigaPhysH != 240))
    {
        AmigaLog("[VIDEO] %s: unexpected actual screen dimensions %dx%d – closing",
                 label, AmigaPhysW, AmigaPhysH);
        CloseScreen(AmigaGameScreen);
        AmigaGameScreen = NULL;
        Amiga_ShowRtgRequester("Opened screen has unexpected dimensions.");
        return NULL;
    }

    /* Clear the whole screen once; in the letterbox case the bottom 40 rows
     * stay black because the game only blits its top 200 rows. */
    SetRast(&AmigaGameScreen->RastPort, 0);

    return AmigaGameScreen;
}

static inline void Amiga_CloseGameScreen(void)
{
    if (AmigaGameScreen) {
        CloseScreen(AmigaGameScreen);
        AmigaGameScreen = NULL;
    }
}

/* Converts 32 chunky pixels into 8 plane longwords using three 64-bit delta
 * swaps (Hacker's Delight 8x8 bit-matrix transpose per 8-pixel group, with
 * the four group results combined into one longword store per plane).
 * Byte k of the transposed word (MSB first) holds plane (7-k) data.
 * Verified bit-exact against a brute-force reference in a host-side test. */
static inline void Amiga_C2P_Block32(const uint8_t *chunky, uint32_t **planes, int longofs)
{
    uint32_t pw[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int g, k;

    for (g = 0; g < 4; g++) {
        const uint8_t *p = chunky + g * 8;
        uint64_t x = 0, t;

        for (k = 0; k < 8; k++)
            x = (x << 8) | (uint64_t)p[k];

        t = (x ^ (x >>  7)) & 0x00AA00AA00AA00AAULL; x = x ^ t ^ (t <<  7);
        t = (x ^ (x >> 14)) & 0x0000CCCC0000CCCCULL; x = x ^ t ^ (t << 14);
        t = (x ^ (x >> 28)) & 0x00000000F0F0F0F0ULL; x = x ^ t ^ (t << 28);

        for (k = 0; k < 8; k++)
            pw[7 - k] = (pw[7 - k] << 8) | (uint8_t)(x >> (56 - 8 * k));
    }

    for (k = 0; k < 8; k++)
        planes[k][longofs] = pw[k];
}

/* Full-frame chunky->planar conversion for the native AGA screen (320x200x8).
 * Writes plane longwords straight into the screen's BitMap (chip RAM), which
 * needs ~4x fewer chip-memory bus cycles than byte-oriented OS conversion. */
static inline void Amiga_C2P_BlitScreen(struct BitMap *bm, const uint8_t *chunky)
{
    int y, blk, k;

    for (y = 0; y < AMIGA_GAME_HEIGHT; y++) {
        const uint8_t *row = chunky + y * AMIGA_GAME_WIDTH;
        uint32_t *pl[8];

        for (k = 0; k < 8; k++)
            pl[k] = (uint32_t *)((uint8_t *)bm->Planes[k] + y * bm->BytesPerRow);

        for (blk = 0; blk < AMIGA_GAME_WIDTH / 32; blk++)
            Amiga_C2P_Block32(row + blk * 32, pl, blk);
    }
}

/* Blits the game's logical 320x200 frame to the physical screen 1:1.
 * 'chunky' is the game's 8-bit chunky buffer (320 bytes/row).
 * The blit path was selected once in Amiga_OpenGameScreen (AmigaBlitMode). */
static inline void Amiga_BlitScreen(struct Window *win, const uint8_t *chunky)
{
    if (!win || !win->RPort || !chunky) return;

    /* Picasso96 fast path: the P96 driver copies the chunky buffer straight
     * into the CLUT8 screen bitmap - no OS conversion layers in between. */
    if (AmigaBlitMode == AMIGA_BLIT_P96) {
        struct RenderInfo ri;
        ri.Memory = (APTR)chunky;
        ri.BytesPerRow = AMIGA_GAME_WIDTH;
        ri.pad = 0;
        ri.RGBFormat = RGBFB_CLUT;
        p96WritePixelArray(&ri, 0, 0, win->RPort,
                           (UWORD)win->BorderLeft, (UWORD)win->BorderTop,
                           AMIGA_GAME_WIDTH, AMIGA_GAME_HEIGHT);
        return;
    }

    /* CyberGraphX fast path: same idea through the CGX driver API. */
    if (AmigaBlitMode == AMIGA_BLIT_CGX) {
        CGX_WritePixelArray((APTR)chunky, 0, 0, AMIGA_GAME_WIDTH, win->RPort,
                            (UWORD)win->BorderLeft, (UWORD)win->BorderTop,
                            AMIGA_GAME_WIDTH, AMIGA_GAME_HEIGHT,
                            AMIGA_CGX_RECTFMT_LUT8);
        return;
    }

    /* Native AGA path: custom 68060 C2P directly into the screen bitplanes. */
    if (AmigaBlitMode == AMIGA_BLIT_AGA_C2P &&
        AmigaGameScreen && AmigaGameScreen->RastPort.BitMap &&
        AmigaGameScreen->RastPort.BitMap->Depth == 8) {
        Amiga_C2P_BlitScreen(AmigaGameScreen->RastPort.BitMap, chunky);
        return;
    }

    /* Generic OS 1:1 chunky blit (fallback). */
    WriteChunkyPixels(win->RPort,
        win->BorderLeft, win->BorderTop,
        win->BorderLeft + 319, win->BorderTop + 199,
        (UBYTE *)chunky, 320);
}

/* Applies an SDL_Color palette to the screen using LoadRGB32. */
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

/* Hides or restores the native Amiga hardware sprite pointer. */
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
#endif /* End of main AmigaOS block. */

/* Byte order definitions. */

#define SDL_LIL_ENDIAN  1234
#define SDL_BIG_ENDIAN  4321

/* Motorola 68k is Big Endian. */
#define SDL_BYTEORDER   SDL_BIG_ENDIAN

/* Byte-swap functions for Big Endian. */
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

/* SwapLE swaps on Big Endian, SwapBE is no-op. */
#define SDL_SwapLE16(x) SDL_Swap16(x)
#define SDL_SwapLE32(x) SDL_Swap32(x)
#define SDL_SwapBE16(x) (x)
#define SDL_SwapBE32(x) (x)

/* Basic SDL types. */

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

/* SDL version macros. */

#define SDL_MAJOR_VERSION 2
#define SDL_MINOR_VERSION 0
#define SDL_PATCHLEVEL    5

#define SDL_VERSION_ATLEAST(x, y, z) \
    ((SDL_MAJOR_VERSION > (x)) || \
     (SDL_MAJOR_VERSION == (x) && SDL_MINOR_VERSION > (y)) || \
     (SDL_MAJOR_VERSION == (x) && SDL_MINOR_VERSION == (y) && SDL_PATCHLEVEL >= (z)))

/* SDL_Init subsystem flags. */

#define SDL_INIT_TIMER          0x00000001u
#define SDL_INIT_AUDIO          0x00000010u
#define SDL_INIT_VIDEO          0x00000020u
#define SDL_INIT_JOYSTICK       0x00000200u
#define SDL_INIT_HAPTIC         0x00001000u
#define SDL_INIT_GAMECONTROLLER 0x00002000u
#define SDL_INIT_EVENTS         0x00004000u

/* Audio format constants. */

#define AUDIO_S16SYS  0x8010  /* Signed 16-bit, system byte order. */
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

/* Video structures with Amiga-specific pointers. */

typedef struct SDL_Window {
    int w, h;
#ifdef __AMIGA__
    struct Window *amiga_window;   /* Real Intuition window pointer. */
    struct Screen *amiga_screen;   /* Dedicated custom RTG screen. */
#endif
} SDL_Window;

typedef struct SDL_Renderer {
    SDL_Window *window;            /* Back-pointer to owning window. */
    int logical_w, logical_h;      /* Logical size set via SDL_RenderSetLogicalSize. */
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
    SDL_PixelFormat *format;    /* Proper format pointer type. */
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

/* Window flags. */
#define SDL_WINDOW_FULLSCREEN          0x00000001u
#define SDL_WINDOW_SHOWN               0x00000004u
#define SDL_WINDOW_RESIZABLE           0x00000020u
#define SDL_WINDOW_FULLSCREEN_DESKTOP  0x00001001u
#define SDL_WINDOW_ALLOW_HIGHDPI       0x00002000u
#define SDL_WINDOW_BORDERLESS          0x00000010u

/* Window position. */
#define SDL_WINDOWPOS_UNDEFINED        0x1FFF0000u
#define SDL_WINDOWPOS_CENTERED         0x2FFF0000u

/* Renderer flags. */
#define SDL_RENDERER_SOFTWARE          0x00000001u
#define SDL_RENDERER_ACCELERATED       0x00000002u
#define SDL_RENDERER_PRESENTVSYNC      0x00000004u
#define SDL_RENDERER_TARGETTEXTURE     0x00000008u

/* Texture access. */
#define SDL_TEXTUREACCESS_STATIC    0
#define SDL_TEXTUREACCESS_STREAMING 1
#define SDL_TEXTUREACCESS_TARGET    2

/* Pixel format enum. */
#define SDL_PIXELFORMAT_ARGB8888  0x16362004u
#define SDL_PIXELFORMAT_RGBA8888  0x16462004u
#define SDL_PIXELFORMAT_RGB888    0x16161804u
#define SDL_PIXELFORMAT_INDEX8    0x13000001u

#define SDL_ALPHA_OPAQUE 255

/* Event structures. */

/* Event types. */
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

/* Window events. */
#define SDL_WINDOWEVENT_EXPOSED     3
#define SDL_WINDOWEVENT_MOVED       4
#define SDL_WINDOWEVENT_RESIZED     5
#define SDL_WINDOWEVENT_MINIMIZED   6
#define SDL_WINDOWEVENT_MAXIMIZED   7
#define SDL_WINDOWEVENT_RESTORED    8
#define SDL_WINDOWEVENT_FOCUS_GAINED 12
#define SDL_WINDOWEVENT_FOCUS_LOST   13

/* Keysyms and scancodes mapping to standard SDL2 USB-HID numbering. */
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

/* Shared Amiga event globals. */

#ifdef __AMIGA__
/* Event queue and mouse state. */
#define AMIGA_SDL_EVENT_QUEUE_SIZE 64
AMIGA_STUBS_DECL SDL_Event AmigaEventQueue[AMIGA_SDL_EVENT_QUEUE_SIZE];
AMIGA_STUBS_DECL int       AmigaEventQueueHead AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL int       AmigaEventQueueTail AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL int       AmigaMouseX         AMIGA_STUBS_INIT(160); /* start centered on the 320x200 game screen */
AMIGA_STUBS_DECL int       AmigaMouseY         AMIGA_STUBS_INIT(100);
AMIGA_STUBS_DECL int       AmigaMouseButtons   AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL UWORD     AmigaLastMouseButtonCode AMIGA_STUBS_INIT(0xFFFF);
AMIGA_STUBS_DECL uint32_t  AmigaLastMouseButtonTicks AMIGA_STUBS_INIT(0);
#endif /* __AMIGA__ */

/* GameController and Haptic stubs. */

typedef struct SDL_GameController SDL_GameController;
typedef struct SDL_Haptic SDL_Haptic;

/* Controller buttons. */
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

/* Controller axes. */
#define SDL_CONTROLLER_AXIS_LEFTX           0
#define SDL_CONTROLLER_AXIS_LEFTY           1
#define SDL_CONTROLLER_AXIS_RIGHTX          2
#define SDL_CONTROLLER_AXIS_RIGHTY          3
#define SDL_CONTROLLER_AXIS_TRIGGERLEFT     4
#define SDL_CONTROLLER_AXIS_TRIGGERRIGHT    5

/* Controller types. */
typedef enum {
    SDL_CONTROLLER_TYPE_UNKNOWN = 0,
    SDL_CONTROLLER_TYPE_XBOX360,
    SDL_CONTROLLER_TYPE_XBOXONE,
    SDL_CONTROLLER_TYPE_PS3,
    SDL_CONTROLLER_TYPE_PS4,
    SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO,
    SDL_CONTROLLER_TYPE_PS5
} SDL_GameControllerType;

/* SDL Hint constants. */

#define SDL_HINT_RENDER_SCALE_QUALITY "SDL_RENDER_SCALE_QUALITY"
#define SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING "SDL_WINDOWS_DISABLE_THREAD_NAMING"
/* Added for diagnostic SDL_CreateRenderer freeze investigation (i_video.cpp). */
#define SDL_HINT_RENDER_DRIVER "SDL_RENDER_DRIVER"

/* Utility macros. */

#define SDL_max(a, b) ((a) > (b) ? (a) : (b))
#define SDL_min(a, b) ((a) < (b) ? (a) : (b))

/* putenv compatibility for noixemul. */

/* Stub putenv for Amiga if missing. */
#ifdef __AMIGA__
#ifndef putenv
static inline int putenv(char *string) { (void)string; return 0; }
#endif
#endif

/* Inline SDL function stubs. */

#ifdef __cplusplus
extern "C" {
#endif

/* Init and Quit */
static inline int SDL_Init(uint32_t flags)
{
    (void)flags;

#ifdef __AMIGA__

    if (!LowLevelBase)
    {
        LowLevelBase = OpenLibrary((CONST_STRPTR)"lowlevel.library", 40);
        if (LowLevelBase)
        {
            /* Pin the port to plain joystick mode. The first attempt at
             * this passed SJA_TYPE_GAMECTLR as the tag itself - that value
             * is 1, which utility/tagitem.h defines as TAG_IGNORE, so the
             * request was silently dropped and the unterminated tag list
             * made the library parse random stack garbage as attributes.
             *
             * Leaving autosense in charge didn't work either: until the
             * first real wiggle it can't decide what's plugged in, and on
             * real hardware (A2000, A1200/piStorm) the port reports junk
             * in the meantime - phantom buttons that skipped the intro
             * logos and froze the menu until the joystick was touched.
             * WinUAE never showed any of this because its emulated port
             * politely returns 0.
             *
             * Joystick mode is the safe default every classic Amiga game
             * uses: directions and fire are plain digital lines, no serial
             * shift register involved. A CD32 pad still works as a normal
             * 2-button stick here; only the extra pad buttons are lost,
             * and this game doesn't use them anyway. */
            SetJoyPortAttrs(1, SJA_Type, SJA_TYPE_JOYSTK, TAG_DONE);
        }
    }

#endif

    return 0;
}
#ifdef __AMIGA__
/* Defined in the audio section further below; stops the audio task and
 * closes ahi.device. Idempotent. */
static inline void SDL_CloseAudio(void);
#endif
static inline void SDL_QuitSubSystem(uint32_t flags)
{
#ifdef __AMIGA__
    /* SND_DeInit() routes here: really shut down the AHI backend instead of
     * leaving "Raptor Audio Task" and the open ahi.device behind (the old
     * no-op let both survive game exit, looping the last audio buffer). */
    if (flags & SDL_INIT_AUDIO)
        SDL_CloseAudio();
#else
    (void)flags;
#endif
}

static inline void SDL_Quit(void) {
#ifdef __AMIGA__
    /* Last-chance audio shutdown (idempotent): never leave ahi.device or
     * the audio task running after the game exits, even if SND_DeInit()
     * was skipped on some exit path. */
    SDL_CloseAudio();

    /* Idempotent guard to prevent double-closing libraries and redundant output. */
    if (!AmigaGameWindow && !AmigaGameScreen &&
        !IntuitionBase && !GfxBase && !LowLevelBase && !P96Base && !CyberGfxBase)
        return;

    /* Restores the native pointer before destroying window and closing libraries. */
    Amiga_ShowSystemPointer();
    Amiga_CloseGameScreen();
    Amiga_CloseP96();
    Amiga_CloseCGX();
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

/* Error */
static inline const char* SDL_GetError(void) { return "SDL stubs - not implemented"; }

/* Timer */
/* Returns milliseconds since first call using AmigaOS DateStamp. */
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
    /* Calculate relative time avoiding ds_Days overflow. */
    /* Convert minutes and ticks to milliseconds. */
    now = (uint32_t)ds.ds_Minute * 60000u + (uint32_t)ds.ds_Tick * 20u;

    if (first_call) {
        base_ticks = now;
        first_call = 0;
    }
    return now - base_ticks;
#else
    /* Fallback for non-Amiga testing. */
    static uint32_t fake_ticks = 0;
    return fake_ticks++;
#endif
}

static inline void SDL_Delay(uint32_t ms) {
#ifdef __AMIGA__
    if (ms > 0) {
        /* Delay takes ticks (1/50s = 20ms units). */
        uint32_t ticks = (ms + 19) / 20;
        if (ticks < 1) ticks = 1;
        Delay(ticks);
    }
#else
    (void)ms;
#endif
}

/* Hints */
static inline int    SDL_SetHint(const char *n, const char *v) { (void)n; (void)v; return 0; }

/* Display info */
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

/* Creates a borderless window on a custom 320x200x8 screen. */
/* Open intuition.library v39. */


static inline SDL_Window* SDL_CreateWindow(const char *title, int x, int y, int w, int h, uint32_t flags)
{
    (void)title;
    (void)x;
    (void)y;
    (void)flags;
    (void)w;
    (void)h;

    SDL_Window *win = (SDL_Window*)calloc(1, sizeof(SDL_Window));
    if (!win)
        return NULL;

    win->w = AMIGA_GAME_WIDTH;
    win->h = AMIGA_GAME_HEIGHT;

#ifdef AMIGA
    if (!IntuitionBase)
        IntuitionBase = (struct IntuitionBase*)OpenLibrary((CONST_STRPTR)"intuition.library", 39);
    if (!IntuitionBase) {
        AmigaLog("[VIDEO] OpenLibrary(intuition.library,39) -> FAIL");
        free(win);
        return NULL;
    }
    AmigaLog("[VIDEO] OpenLibrary(intuition.library,39) -> OK");

    if (!GfxBase)
        GfxBase = (struct GfxBase*)OpenLibrary((CONST_STRPTR)"graphics.library", 39);
    if (!GfxBase) {
        AmigaLog("[VIDEO] OpenLibrary(graphics.library,39) -> FAIL");
        free(win);
        return NULL;
    }
    AmigaLog("[VIDEO] OpenLibrary(graphics.library,39) -> OK");

    win->amiga_screen = Amiga_OpenGameScreen(AMIGA_GAME_WIDTH, AMIGA_GAME_HEIGHT, AMIGA_GAME_DEPTH);
    if (!win->amiga_screen) {
        AmigaLog("[VIDEO] Amiga_OpenGameScreen -> FAIL (no screen)");
        free(win);
        return NULL;
    }

    win->amiga_window = OpenWindowTags(NULL,
        WA_Left,          0,
        WA_Top,           0,
        WA_Width,         (ULONG)AmigaPhysW,
        WA_Height,        (ULONG)AmigaPhysH,
        WA_InnerWidth,    (ULONG)AmigaPhysW,
        WA_InnerHeight,   (ULONG)AmigaPhysH,
        WA_CustomScreen,  (ULONG)win->amiga_screen,
        WA_Borderless,    TRUE,
        WA_Backdrop,      TRUE,
        WA_DragBar,       FALSE,
        WA_DepthGadget,   FALSE,
        WA_CloseGadget,   FALSE,
        WA_Activate,      TRUE,
        WA_RMBTrap,       TRUE,
        WA_ReportMouse,   TRUE,
        WA_GimmeZeroZero, TRUE,
        /* With -nomouse the window registers no mouse events at all:
         * IDCMP_MOUSEMOVE/MOUSEBUTTONS are omitted, so Intuition never
         * posts them (RAWKEY + CLOSEWINDOW always remain). */
        WA_IDCMP,         IDCMP_CLOSEWINDOW | IDCMP_RAWKEY |
                          (AmigaMouseDisabled ? 0 : (IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE)),
        TAG_DONE);

    if (!win->amiga_window) {
        AmigaLog("[VIDEO] OpenWindowTags %dx%d -> FAIL", AmigaPhysW, AmigaPhysH);
        Amiga_CloseGameScreen();
        free(win);
        return NULL;
    }
    AmigaLog("[VIDEO] OpenWindowTags %dx%d -> OK; AmigaGameWindow set, pointer hidden",
             AmigaPhysW, AmigaPhysH);

    AmigaGameWindow = win->amiga_window;
    Amiga_HideSystemPointer();

    AmigaMouseX = AMIGA_GAME_WIDTH / 2;
    AmigaMouseY = AMIGA_GAME_HEIGHT / 2;
    AmigaMouseButtons = 0;
    AmigaLastMouseButtonCode = 0xFFFF;
    AmigaLastMouseButtonTicks = 0;
#endif

    return win;
}

static inline void SDL_DestroyWindow(SDL_Window *w) {
    if (!w) return;
#ifdef __AMIGA__
    if (w->amiga_window) {
        if (AmigaGameWindow == w->amiga_window) {
            AmigaGameWindow = NULL;
        }
        CloseWindow(w->amiga_window);
        w->amiga_window = NULL;
    }
    Amiga_CloseGameScreen();
    w->amiga_screen = NULL;
#endif
    free(w);
}

static inline uint32_t SDL_GetWindowID(SDL_Window *w) { (void)w; return 1; }
static inline uint32_t SDL_GetWindowFlags(SDL_Window *w) { (void)w; return 0; }
static inline int    SDL_GetWindowDisplayIndex(SDL_Window *w) { (void)w; return 0; }
/* Reports INDEX8 pixel format. */
static inline uint32_t SDL_GetWindowPixelFormat(SDL_Window *w) { (void)w; return SDL_PIXELFORMAT_INDEX8; }

static inline void SDL_GetWindowSize(SDL_Window *w, int *pw, int *ph) {
    if (w) { if(pw) *pw = w->w; if(ph) *ph = w->h; }
    else   { if(pw) *pw = 320;  if(ph) *ph = 200;   }
}

static inline void SDL_SetWindowSize(SDL_Window *w, int ww, int hh) {
    (void)ww; (void)hh;
    /* Resizing is a no-op by design. */
    if (w) { w->w = AMIGA_GAME_WIDTH; w->h = AMIGA_GAME_HEIGHT; }
}

static inline void SDL_SetWindowMinimumSize(SDL_Window *w, int mw, int mh) {
    (void)w; (void)mw; (void)mh;
}

static inline void SDL_SetWindowTitle(SDL_Window *w, const char *t) {
    if (!w || !t) return;
#ifdef __AMIGA__
    if (w->amiga_window) {
        /* Uses sentinel to preserve screen title. */
        SetWindowTitles(w->amiga_window, (CONST_STRPTR)t, (CONST_STRPTR)~0);
    }
#endif

}

static inline void SDL_SetWindowFullscreen(SDL_Window *w, uint32_t f) {
    (void)w; (void)f;
    /* Fullscreen is a no-op. */
}

/* Renderer handling */

static inline SDL_Renderer* SDL_CreateRenderer(SDL_Window *w, int idx, uint32_t flags) {
    /* Minimal elimination test: return a static, pre-existing renderer object.
       No calloc, no complex local ops. Only sets the window field. */
    static SDL_Renderer fake_renderer;
    (void)idx; (void)flags;
    fake_renderer.window = w;
    return &fake_renderer;
}

static inline void SDL_DestroyRenderer(SDL_Renderer *r) {
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

/* Sets logical resolution to enable proper coordinate scaling. */
static inline int    SDL_RenderSetLogicalSize(SDL_Renderer *r, int w, int h) {
    if (r) { r->logical_w = w; r->logical_h = h; }
    return 0;
}

static inline int    SDL_RenderSetIntegerScale(SDL_Renderer *r, SDL_bool e) { (void)r; (void)e; return 0; }
static inline void   SDL_RenderClear(SDL_Renderer *r) { (void)r; }
static inline int    SDL_RenderCopy(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *s, const SDL_Rect *d) { if (!r || !t) return 0; (void)s; (void)d; return 0; }

/* Flips the cached chunky buffer to the screen. */
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

static inline int    SDL_SetRenderTarget(SDL_Renderer *r, SDL_Texture *t) { if (!t) return 0; (void)r; (void)t; return 0; }
static inline int    SDL_SetRenderDrawColor(SDL_Renderer *r, uint8_t rr, uint8_t g, uint8_t b, uint8_t a) { (void)r; (void)rr; (void)g; (void)b; (void)a; return 0; }

/* Calculates scale and viewport using physical and logical dimensions. */
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

/* Texture handling */

static inline SDL_Texture* SDL_CreateTexture(SDL_Renderer *r, uint32_t f, int a, int w, int h) {
    (void)r; (void)f; (void)a;
    SDL_Texture *t = (SDL_Texture*)calloc(1, sizeof(SDL_Texture));
    if (t) { t->w = w; t->h = h; }
    return t;
}

static inline void SDL_DestroyTexture(SDL_Texture *t) { free(t); }

static inline int    SDL_UpdateTexture(SDL_Texture *t, const SDL_Rect *r, const void *p, int pi) { if (!t) return 0; (void)r; (void)p; (void)pi; return 0; }

/* Surface handling */
static inline SDL_Surface* SDL_CreateRGBSurface(uint32_t flags, int w, int h, int depth,
    uint32_t rm, uint32_t gm, uint32_t bm, uint32_t am) {
    (void)flags;
    SDL_Surface *s = (SDL_Surface*)calloc(1, sizeof(SDL_Surface));
    if (!s) return 0;
    s->w = w; s->h = h;
    s->pitch = w * ((depth + 7) / 8);
    s->pixels = calloc(1, (size_t)(s->pitch * h));
    /* Allocate a pixel format. */
    SDL_PixelFormat *fmt = (SDL_PixelFormat*)calloc(1, sizeof(SDL_PixelFormat));
    if (fmt) {
        fmt->BitsPerPixel = (uint8_t)depth;
        fmt->BytesPerPixel = (uint8_t)((depth + 7) / 8);
        fmt->Rmask = rm; fmt->Gmask = gm; fmt->Bmask = bm; fmt->Amask = am;
        if (depth == 8) {
            /* Allocate palette for 8-bit surfaces. */
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

/* Updates the palette and applies it to the hardware screen. */
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
    /* Handles surface conversion or caching based on platform. */
    (void)dr; /* Destination rect matches source rect. */
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

/* Palette conversion skipped on Amiga in favor of raw chunky buffer. */
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
            /* ARGB8888 layout. */
            dp[x] = (0xFFu << 24) | ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | (uint32_t)c.b;
        }
    }
#else
    /* Caches the source buffer for the actual RTG blit in SDL_RenderPresent. */
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

/* Audio handling */

/* Implements an audio API subset using ahi.device with double-buffered CMD_WRITE. Avoids STDIO in the background task. */
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
#include <devices/ahi.h>
#include <stddef.h>

/* The OFFICIAL AHI SDK header (vendored in src/amiga/devices/ahi.h) must be
 * the one included above: the toolchain's ndk-include/devices/ahi.h is an
 * incompatible hand-written substitute with a wrong AHIRequest layout
 * (IORequest instead of IOStdReq, too little driver-private space), which
 * shifted every field this stub wrote past where ahi.device reads them:
 * ahir_Type read as 0 (mono 8-bit), ahir_Frequency 0, ahir_Volume ~0 and
 * ahir_Link as a wild pointer (crash/hang on buffer switch).
 * These compile-time guards fail loudly if the wrong header ever wins again.
 * Official layout: IOStdReq(48) + Version(2) + Pad1(2) + Private[2](8)
 * + Type + Frequency + Volume + Position + Link = 80 bytes total. */
typedef char amiga_ahi_req_size_check[(sizeof(struct AHIRequest) == 80) ? 1 : -1];
typedef char amiga_ahi_type_offs_check[(offsetof(struct AHIRequest, ahir_Type) == 60) ? 1 : -1];
typedef char amiga_ahi_link_offs_check[(offsetof(struct AHIRequest, ahir_Link) == 76) ? 1 : -1];

/* AHI 16.16 fixed-point: full volume and centered stereo position.
 * NOTE: ahir_Position 0x8000 is CENTER - 0x0000 would be full left. */
#define AMIGA_AHI_VOLUME_FULL     (0x00010000UL)
#define AMIGA_AHI_POSITION_CENTER (0x00008000UL)

#define AMIGA_AUDIO_NUM_BUFFERS 2

struct AmigaAudioState
{
    int initialized;
    struct MsgPort         *port;
    struct AHIRequest *req[AMIGA_AUDIO_NUM_BUFFERS];
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
    /* Diagnostics: written by the audio task (no STDIO allowed there),
     * read and logged from the main task only. */
    volatile ULONG buffersFilled;
    volatile LONG  lastError;
    /* Init handshake: 0 = pending, 1 = streaming live, <0 = failed stage
     * (-1 msg port, -2 IO request, -3 OpenDevice, -4 buffers). */
    volatile LONG  openDone;
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

/* Background audio task - no STDIO allowed. */
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

static inline void AmigaAudio_SetupRequest(struct AHIRequest *req, UBYTE *buf, ULONG bytes)

{
    req->ahir_Std.io_Command = CMD_WRITE;
    req->ahir_Std.io_Data    = (APTR)buf;
    req->ahir_Std.io_Length  = (ULONG)bytes;
    req->ahir_Std.io_Offset  = 0;
    req->ahir_Type      = g_AmigaAudio.ahiType;
    req->ahir_Frequency = (ULONG)g_AmigaAudio.freq;
    req->ahir_Volume    = AMIGA_AHI_VOLUME_FULL;
    req->ahir_Position  = AMIGA_AHI_POSITION_CENTER;
}

/* Dedicated background audio task - no STDIO allowed.
 *
 * This task owns the ENTIRE AHI device side: message port, IO requests,
 * OpenDevice/CloseDevice and the streaming loop.  The message port (and
 * therefore its completion signal) MUST belong to this task: a port
 * created by the main task has mp_SigTask = main task, so ahi.device
 * signals the main task on every completed request while this task's
 * WaitIO() sleeps forever - that was the "AHI opens OK but zero buffers
 * play" bug (buffersFilled stayed 0, game silent, music sequencer dead).
 *
 * Failures are reported to the main task through openDone (see above) and
 * lastError (OpenDevice error code); the main task does all the logging. */
/* Compiled with -O0 to prevent FPU traps from opportunistic register spills during OpenDevice. */
__attribute__((optimize("O0")))
static inline void AmigaAudio_TaskEntry(void)

{
    int cur, i;

    g_AmigaAudio.taskRunning = 1;

    if (!g_AmigaAudio.buffer[0] || !g_AmigaAudio.buffer[1])
    {
        g_AmigaAudio.openDone = -4; /* buffers missing (main-side alloc) */
        g_AmigaAudio.taskRunning = 0;
        return;
    }

    g_AmigaAudio.port = CreateMsgPort();
    if (!g_AmigaAudio.port)
    {
        g_AmigaAudio.openDone = -1;
        g_AmigaAudio.taskRunning = 0;
        return;
    }

    for (i = 0; i < AMIGA_AUDIO_NUM_BUFFERS; i++)
    {
        g_AmigaAudio.req[i] = (struct AHIRequest *)CreateIORequest(g_AmigaAudio.port, sizeof(struct AHIRequest));
        if (!g_AmigaAudio.req[i])
        {
            AmigaAudio_FreeIOReqs();
            g_AmigaAudio.openDone = -2;
            g_AmigaAudio.taskRunning = 0;
            return;
        }
        memset(g_AmigaAudio.req[i], 0, sizeof(struct AHIRequest));
        /* Require ahi.device V4 (official AHI examples do the same). */
        g_AmigaAudio.req[i]->ahir_Version = 4;
        g_AmigaAudio.req[i]->ahir_Std.io_Message.mn_Node.ln_Type = NT_MESSAGE;
        g_AmigaAudio.req[i]->ahir_Std.io_Message.mn_Node.ln_Name = NULL;
        g_AmigaAudio.req[i]->ahir_Std.io_Message.mn_ReplyPort    = g_AmigaAudio.port;
        g_AmigaAudio.req[i]->ahir_Std.io_Message.mn_Length       = sizeof(struct AHIRequest);
    }

    {
        BYTE odErr = OpenDevice((CONST_STRPTR)AHINAME, AHI_DEFAULT_UNIT,
                                (struct IORequest *)g_AmigaAudio.req[0], 0);
        if (odErr != 0)
        {
            g_AmigaAudio.lastError = odErr;
            AmigaAudio_FreeIOReqs();
            g_AmigaAudio.openDone = -3;
            g_AmigaAudio.taskRunning = 0;
            return;
        }
    }
    g_AmigaAudio.devopen = 1;

    g_AmigaAudio.req[1]->ahir_Std.io_Device = g_AmigaAudio.req[0]->ahir_Std.io_Device;
    g_AmigaAudio.req[1]->ahir_Std.io_Unit   = g_AmigaAudio.req[0]->ahir_Std.io_Unit;

    /* Canonical double buffering (ahidev.guide, "Writing To The Device"). */
    AmigaAudio_FillBuffer(g_AmigaAudio.buffer[0], g_AmigaAudio.bufferBytes);
    AmigaAudio_FillBuffer(g_AmigaAudio.buffer[1], g_AmigaAudio.bufferBytes);

    AmigaAudio_SetupRequest(g_AmigaAudio.req[0], g_AmigaAudio.buffer[0], g_AmigaAudio.bufferBytes);
    g_AmigaAudio.req[0]->ahir_Link = NULL;
    SendIO((struct IORequest *)g_AmigaAudio.req[0]);

    AmigaAudio_SetupRequest(g_AmigaAudio.req[1], g_AmigaAudio.buffer[1], g_AmigaAudio.bufferBytes);
    g_AmigaAudio.req[1]->ahir_Link = g_AmigaAudio.req[0];
    SendIO((struct IORequest *)g_AmigaAudio.req[1]);

    g_AmigaAudio.openDone = 1; /* tell the main task: streaming is live */
    cur = 0;

    while (!g_AmigaAudio.taskShouldQuit)
    {
        struct AHIRequest *done = g_AmigaAudio.req[cur];
        int other;

        WaitIO((struct IORequest *)done);

        /* No STDIO in this task - record diagnostics for the main task. */
        if (((struct IORequest *)done)->io_Error)
            g_AmigaAudio.lastError = ((struct IORequest *)done)->io_Error;
        g_AmigaAudio.buffersFilled++;

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

    CloseDevice((struct IORequest *)g_AmigaAudio.req[0]);
    g_AmigaAudio.devopen = 0;

    AmigaAudio_FreeIOReqs();

    g_AmigaAudio.taskRunning = 0;
}

static inline void SDL_CloseAudio(void)
{
    if (!g_AmigaAudio.initialized)
        return;

    AmigaLog("AHI: closing audio (buffers played: %lu, last io_Error: %ld)",
             (unsigned long)g_AmigaAudio.buffersFilled, (long)g_AmigaAudio.lastError);

    /* Ask the audio task to stop; it owns the device, the IO requests and
     * the message port, so it does the whole device-side teardown itself. */
    g_AmigaAudio.taskShouldQuit = 1;
    {
        int spins = 0;
        while (g_AmigaAudio.taskRunning && spins < 500)
        {
            Delay(1);
            spins++;
        }
    }

    if (g_AmigaAudio.taskRunning)
    {
        /* The task is wedged (should not happen - its loop is bounded by
         * one buffer period).  Do NOT free the buffers or zero the state
         * while the task may still touch them; leak a few KB instead of
         * corrupting memory under a running task.  SDL_Quit() retries
         * this same idempotent path. */
        AmigaLog("AHI: WARNING - audio task did not stop in time (buffers left allocated)!");
        return;
    }

    /* The task clears taskRunning as its very last g_AmigaAudio access,
     * but the process then still executes the function epilogue and the
     * dos process teardown.  Give it a couple of ticks to really die
     * before we free the buffers here and the main process later exits
     * (which would unload the seglist under a half-dead process). */
    Delay(2);

    AmigaAudio_FreeBuffers();

    memset(&g_AmigaAudio, 0, sizeof(g_AmigaAudio));

    AmigaLog("AHI: audio closed.");
}

/* Compiled with -O0 to prevent FPU traps from opportunistic register spills during OpenDevice. */
__attribute__((optimize("O0")))
static inline int SDL_OpenAudio(const SDL_AudioSpec *desired, SDL_AudioSpec *obtained)
{
    if (g_AmigaAudio.initialized)
        SDL_CloseAudio();

    memset(&g_AmigaAudio, 0, sizeof(g_AmigaAudio));

    if (!desired)
        return -1;

    g_AmigaAudio.freq     = desired->freq > 0 ? desired->freq : 44100;
    g_AmigaAudio.channels = desired->channels > 0 ? desired->channels : 2;
    g_AmigaAudio.callback = desired->callback;
    g_AmigaAudio.userdata = desired->userdata;
    g_AmigaAudio.paused   = 0; /* Audio starts playing. */
    g_AmigaAudio.ahiType  = (g_AmigaAudio.channels >= 2) ? AHIST_S16S : AHIST_M16S;

    AmigaLog("AHI: open request: freq=%ld channels=%ld type=%s",
             (long)g_AmigaAudio.freq, (long)g_AmigaAudio.channels,
             g_AmigaAudio.ahiType == AHIST_S16S ? "AHIST_S16S" : "AHIST_M16S");

    {
        ULONG frames = desired->samples > 0 ? desired->samples : 512;
        g_AmigaAudio.bufferBytes = frames * (ULONG)g_AmigaAudio.channels * sizeof(short);
    }

    AmigaLog("AHI: buffers: %d x %lu bytes", AMIGA_AUDIO_NUM_BUFFERS, (unsigned long)g_AmigaAudio.bufferBytes);

    {
        int i;
        for (i = 0; i < AMIGA_AUDIO_NUM_BUFFERS; i++)
        {
            g_AmigaAudio.buffer[i] = (UBYTE *)AllocMem(g_AmigaAudio.bufferBytes, MEMF_PUBLIC | MEMF_CLEAR);
            if (!g_AmigaAudio.buffer[i])
            {
                AmigaLog("AHI: AllocMem(%lu) FAILED for buffer %d", (unsigned long)g_AmigaAudio.bufferBytes, i);
                AmigaAudio_FreeBuffers();
                return -1;
            }
        }
    }

    /* The whole AHI device side (message port, IO requests, OpenDevice,
     * streaming loop, teardown) lives inside the audio task - see
     * AmigaAudio_TaskEntry for the reason (the port's completion signal
     * must wake the audio task, not the main task).  Here we only start
     * the task and wait for its init handshake. */
    g_AmigaAudio.taskShouldQuit = 0;
    g_AmigaAudio.taskRunning = 0;
    g_AmigaAudio.openDone = 0;

    g_AmigaAudio.audioTask = CreateNewProcTags(
        NP_Entry,     (ULONG)AmigaAudio_TaskEntry,
        NP_Name,      (ULONG)"Raptor Audio Task",
        NP_Priority,  (LONG)10, /* above the game loop (pri 0): preempt and
                                 * refill as soon as a buffer completes,
                                 * otherwise equal-priority timeslicing
                                 * starves the stream (crackle/stutter) */
        NP_StackSize, (ULONG)32768, /* OPL3 music renders in this task */
        TAG_DONE);

    if (!g_AmigaAudio.audioTask)
    {
        AmigaLog("AHI: CreateNewProcTags FAILED");
        AmigaAudio_FreeBuffers();
        return -1;
    }

    /* Wait for the task to open ahi.device and start streaming. */
    {
        int spins = 0;
        while (!g_AmigaAudio.openDone && spins < 300) /* ~6 s worst case */
        {
            Delay(1);
            spins++;
        }
    }

    if (g_AmigaAudio.openDone != 1)
    {
        LONG stage = g_AmigaAudio.openDone;

        if (stage == 0)
        {
            /* Task never answered - request stop and wait for it to die. */
            int spins2 = 0;
            g_AmigaAudio.taskShouldQuit = 1;
            while (g_AmigaAudio.taskRunning && spins2 < 100)
            {
                Delay(1);
                spins2++;
            }
        }

        AmigaLog("AHI: init FAILED in audio task (stage %ld: -1 port, -2 ioreq, -3 OpenDevice, -4 buffers; io_Error=%ld)",
                 (long)stage, (long)g_AmigaAudio.lastError);

        AmigaAudio_FreeBuffers();
        return -1;
    }

    AmigaLog("AHI: ahi.device opened, audio task streaming (%ld Hz, %lu-byte buffers).",
             (long)g_AmigaAudio.freq, (unsigned long)g_AmigaAudio.bufferBytes);

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
    return 1; /* Non-zero device id indicates success. */
}

static inline void SDL_PauseAudio(int pause_on) {
    g_AmigaAudio.paused = pause_on ? 1 : 0;
}
static inline void SDL_PauseAudioDevice(SDL_AudioDeviceID d, int p) {
    (void)d;
    SDL_PauseAudio(p);
}

/* Lightweight critical-section guard using Disable/Enable. */
static inline void SDL_LockAudio(void)   { Disable(); }
static inline void SDL_UnlockAudio(void) { Enable(); }
static inline void SDL_LockAudioDevice(SDL_AudioDeviceID d)   { (void)d; SDL_LockAudio(); }
static inline void SDL_UnlockAudioDevice(SDL_AudioDeviceID d) { (void)d; SDL_UnlockAudio(); }

#else /* Non-Amiga stub fallback. */

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

/* Mouse button constants. */
#define SDL_BUTTON_LEFT     1
#define SDL_BUTTON_MIDDLE   2
#define SDL_BUTTON_RIGHT    3

#ifndef SDL_BUTTON
#define SDL_BUTTON(X) (1 << ((X)-1))
#endif

/* SDL_ShowCursor argument/return values. */
#define SDL_DISABLE 0
#define SDL_ENABLE  1
#define SDL_QUERY   -1

/* Drains IDCMP message port and translates IntuiMessages into SDL_Events. */

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

/* Inlined qualifier bits to avoid header dependency. */
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

/* Standard Amiga raw mouse button codes. */
#ifndef SELECTDOWN
#define SELECTDOWN   (0x68)
#define SELECTUP     (0x68 | IECODE_UP_PREFIX)
#define MENUDOWN     (0x69)
#define MENUUP       (0x69 | IECODE_UP_PREFIX)
#define MIDDLEDOWN   (0x6A)
#define MIDDLEUP     (0x6A | IECODE_UP_PREFIX)
#endif

/* Maps Amiga raw keycodes to SDL USB-HID scancodes. */
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
    /* 0x5F */ 0, /* Help. */
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
AMIGA_STUBS_DECL ULONG AmigaJoyRawPrev   AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL ULONG AmigaJoyPhantomMask AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL int   AmigaJoySeeded    AMIGA_STUBS_INIT(0);
/* Joystick poll rate limiter (50 Hz) + hardened phantom filter state. */
AMIGA_STUBS_DECL Uint32 AmigaJoyLastPoll AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL int    AmigaJoyClearStreak[5] AMIGA_STUBS_INIT({0});
AMIGA_STUBS_DECL int    AmigaJoyFireLogged AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL int    AmigaMiddleDropLogged AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL Uint32 AmigaFrameCount AMIGA_STUBS_INIT(0);
AMIGA_STUBS_DECL Uint8 AmigaKeyboardState[SDL_NUM_SCANCODES] AMIGA_STUBS_INIT({0});

#ifndef JPF_JOY_UP
#define JPF_JOY_UP (1<<3)
#define JPF_JOY_DOWN (1<<2)
#define JPF_JOY_LEFT (1<<1)
#define JPF_JOY_RIGHT (1<<0)
#define JPF_BUTTON_PLAY (1<<17)
#define JPF_BUTTON_RED (1<<22)
#define JPF_BUTTON_BLUE (1<<23) /* 2nd joystick button / right mouse button line */
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

        else if (class_ == IDCMP_MOUSEMOVE && !AmigaMouseDisabled)
        {
            /* Physical window coordinates -> logical 320x200 mouse space.
             * In the RTG letterbox case the window is 320x240, so clamp
             * mouse Y to the 0..199 play area (the bottom 40 rows are not
             * part of the game). */
            AmigaMouseX = mx;
            AmigaMouseY = my;
            if (AmigaRTGLetterbox && AmigaMouseY >= AMIGA_GAME_HEIGHT)
                AmigaMouseY = AMIGA_GAME_HEIGHT - 1;

            SDL_Event ev;

            memset(&ev, 0, sizeof(ev));
            ev.type = SDL_MOUSEMOTION;
            ev.button.x = AmigaMouseX;
            ev.button.y = AmigaMouseY;
            Amiga_PushEvent(&ev);
        }

        else if (class_ == IDCMP_MOUSEBUTTONS && !AmigaMouseDisabled)
{
    uint32_t now = SDL_GetTicks();

    /* The game ignores the middle mouse button entirely (special-weapon
     * cycling lives on SPACE), and on some machines (A1200 + PiStorm/Emu68)
     * the middle/POT lines float and flood MIDDLEDOWN/MIDDLEUP regardless
     * of the screen type - RTG *and* native AGA. The flood acts as a
     * permanent "ack": intro logos skip, demos exit instantly and gameplay
     * falls back to mouse takeover. Drop middle-button traffic always. */
    if (code == MIDDLEDOWN || code == MIDDLEUP)
    {
        if (!AmigaMiddleDropLogged) {
            AmigaMiddleDropLogged = 1;
            AmigaLog("[INPUT] middle mouse event dropped (phantom filter)");
        }
        /* event dropped */
    }

    /* Cheap debounce for noisy mouse-button lines on real RTG hardware:
     * if the exact same raw button code repeats within one tick window,
     * drop it as line chatter. Real clicks are much slower. */
    else if (code == AmigaLastMouseButtonCode &&
             (now - AmigaLastMouseButtonTicks) <= 1)
    {
        /* event dropped */
    }
    else
    {
        SDL_Event ev;
        memset(&ev, 0, sizeof(ev));

        AmigaLastMouseButtonCode = code;
        AmigaLastMouseButtonTicks = now;

        if (code == SELECTDOWN) {
            ev.type = SDL_MOUSEBUTTONDOWN;
            ev.button.button = SDL_BUTTON_LEFT;
            ev.button.state = 1;
            AmigaMouseButtons |= 1;
        }
        else if (code == SELECTUP) {
            ev.type = SDL_MOUSEBUTTONUP;
            ev.button.button = SDL_BUTTON_LEFT;
            ev.button.state = 0;
            AmigaMouseButtons &= ~1;
        }
        else if (code == MENUDOWN) {
            ev.type = SDL_MOUSEBUTTONDOWN;
            ev.button.button = SDL_BUTTON_RIGHT;
            ev.button.state = 1;
            AmigaMouseButtons |= 2;
        }
        else if (code == MENUUP) {
            ev.type = SDL_MOUSEBUTTONUP;
            ev.button.button = SDL_BUTTON_RIGHT;
            ev.button.state = 0;
            AmigaMouseButtons &= ~2;
        }
        else if (code == MIDDLEDOWN) {
            ev.type = SDL_MOUSEBUTTONDOWN;
            ev.button.button = SDL_BUTTON_MIDDLE;
            ev.button.state = 1;
            AmigaMouseButtons |= 4;
        }
        else if (code == MIDDLEUP) {
            ev.type = SDL_MOUSEBUTTONUP;
            ev.button.button = SDL_BUTTON_MIDDLE;
            ev.button.state = 0;
            AmigaMouseButtons &= ~4;
        }

        if (ev.type) {
            ev.button.x = AmigaMouseX;
            ev.button.y = AmigaMouseY;
            Amiga_PushEvent(&ev);
        }
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

    if (LowLevelBase && !AmigaJoyDisabled) {
        /* Poll the gameport at most at ~50 Hz, like when the old slow blit
         * paced the main loop. With the fast RTG blit paths the event pump
         * runs much faster, and on A1200 + PiStorm/Emu68 the floating port
         * lines then passed the two-read debounce below as phantom fire
         * presses (injected as RETURN = ack: intro logos skipped, demos
         * exited instantly). */
        Uint32 joyNow = SDL_GetTicks();
        if (joyNow - AmigaJoyLastPoll < 20)
            return;
        AmigaJoyLastPoll = joyNow;

        /* Trust the digital lines only: directions + fire (RED). Those
         * are pulled high and driven low, exactly like the mouse button,
         * so an empty port reads a clean "nothing pressed" on any board.
         * BLUE (2nd button) and PLAY ride the potentiometer inputs, which
         * float on real hardware and report random "pressed" states -
         * that phantom input is what skipped the intro logos and froze
         * the menu until the joystick was wiggled. The game keeps Fire
         * Special on keyboard/mouse for joystick-only players. */
        const ULONG joy_mask = JPF_JOY_UP | JPF_JOY_DOWN | JPF_JOY_LEFT |
                               JPF_JOY_RIGHT | JPF_BUTTON_RED;
        ULONG raw = ReadJoyPort(1) & joy_mask;

        /* A stick can't physically go up+down or left+right at once.
         * If the port says otherwise it's noise, not input. */
        if (((raw & (JPF_JOY_UP | JPF_JOY_DOWN)) == (JPF_JOY_UP | JPF_JOY_DOWN)) ||
            ((raw & (JPF_JOY_LEFT | JPF_JOY_RIGHT)) == (JPF_JOY_LEFT | JPF_JOY_RIGHT)))
            raw = 0;

        /* Cheap debounce: a new port state only counts when two reads in
         * a row agree. An empty or floating port flickers between values
         * from one poll to the next, while a real joystick holds its
         * state steady. Costs one frame of input latency, which nobody
         * can feel at 50 Hz. */
        if (raw != AmigaJoyRawPrev) {
            AmigaJoyRawPrev = raw;
        } else {
            /* Phantom filtering. On a real Amiga the port can report a
             * non-zero idle state (floating lines, autosense leftovers),
             * and the game reads AmigaJoyState directly as button booleans
             * - so a stuck phantom bit looked like a permanently held
             * button: it skipped the intro logos (IMS_IsAck) and froze the
             * menu inside JOY_Wait() until the user touched the joystick.
             * Rule: any bit that was already set at the first stable read
             * stays masked out until it has been seen CLEAR at least once.
             * A real button always gets released sooner or later, phantom
             * garbage does not. */
            if (!AmigaJoySeeded) {
                AmigaJoySeeded = 1;
                AmigaJoyPhantomMask = raw;
                AmigaLog("[INPUT] joystick seeded: raw=0x%08lx phantom=0x%08lx",
                         (unsigned long)raw, (unsigned long)AmigaJoyPhantomMask);
            }
            /* Unmask a phantom bit only after the line has been CLEAR for 25
             * consecutive stable polls (~0.5 s at the 50 Hz poll rate). A
             * floating line flickers inside that window and stays masked;
             * a real button is steady, so it unmasks quickly after release. */
            {
                static const ULONG jbits[5] = {
                    JPF_JOY_UP, JPF_JOY_DOWN, JPF_JOY_LEFT, JPF_JOY_RIGHT,
                    JPF_BUTTON_RED
                };
                int bi;
                for (bi = 0; bi < 5; bi++) {
                    if (raw & jbits[bi]) {
                        AmigaJoyClearStreak[bi] = 0;
                    } else if (AmigaJoyPhantomMask & jbits[bi]) {
                        if (++AmigaJoyClearStreak[bi] >= 25)
                            AmigaJoyPhantomMask &= ~jbits[bi];
                    }
                }
            }

            ULONG joy = raw & ~AmigaJoyPhantomMask;

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

            /* Only RED (fire 1) is injected as RETURN = "select" in menus.
             * BLUE/PLAY is the separate B button (cancel in menus, Fire
             * Special in game) - injecting it too would conflict. */
            ULONG fire_mask = JPF_BUTTON_RED;
            int prev_fire = (AmigaJoyStatePrev & fire_mask) ? 1 : 0;
            int curr_fire = (joy & fire_mask) ? 1 : 0;
            if (curr_fire != prev_fire) {
                if (curr_fire && !AmigaJoyFireLogged) {
                    AmigaJoyFireLogged = 1;
                    AmigaLog("[INPUT] joystick fire (RED) -> RETURN");
                }
                Amiga_InjectKeyboardEvent(SDL_SCANCODE_RETURN, curr_fire);
            }

            AmigaJoyState = joy;
            AmigaJoyStatePrev = joy;
        }
        AmigaFrameCount++;
    } else if (AmigaJoyDisabled) {
        /* -nojoy on the command line: keep the reported state clean so
         * nothing downstream can mistake a dead port for input. */
        AmigaJoyState = 0;
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
    if (AmigaMouseDisabled) {
        /* -nomouse: report the centered position and no buttons so
         * nothing downstream can mistake a dead mouse for input. */
        if (x) *x = AMIGA_GAME_WIDTH / 2;
        if (y) *y = AMIGA_GAME_HEIGHT / 2;
        return 0;
    }
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

static inline int SDL_NumJoysticks(void) {
#ifdef __AMIGA__
    /* Report the emulated controller only when lowlevel.library is
     * available and the user didn't kill it with -nojoy. */
    return (LowLevelBase && !AmigaJoyDisabled) ? 1 : 0;
#else
    return 1;
#endif
}
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
        const int fire_blue = (AmigaJoyState & JPF_BUTTON_BLUE) ? 1 : 0;

        /* RED (fire 1) = A. The B button stays unmapped for now: BLUE and
         * PLAY are read through the floating pot lines, which made them
         * indistinguishable from noise on real hardware, so SDL_PumpEvents
         * masks them out before they ever reach AmigaJoyState. Fire
         * Special remains available on keyboard/mouse. */
        if (button == SDL_CONTROLLER_BUTTON_A) return fire_red ? 1 : 0;
        if (button == SDL_CONTROLLER_BUTTON_B) return (fire_blue || fire_play) ? 1 : 0;
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

/* Forward declarations for stubs. */
typedef struct SDL_Haptic SDL_Haptic;
typedef struct SDL_GameController SDL_GameController;

/* Haptic */

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

/* Game Controller */

static inline int SDL_GameControllerGetAttached(SDL_GameController *c) {
    (void)c;
#ifdef __AMIGA__
    /* The Amiga emulated controller is always attached while
     * lowlevel.library is open and the user didn't kill it with
     * -nojoy.  Without this the game's joystick state polling
     * (I_HandleJoystickEvent) was dead code. */
    return (LowLevelBase && !AmigaJoyDisabled) ? 1 : 0;
#else
    return 1;
#endif
}

static inline int SDL_GameControllerTypeForIndex(int idx) {
    (void)idx;
    return 0;
}

static inline int SDL_GameControllerRumble(SDL_GameController *c, uint16_t low, uint16_t high, uint32_t dur) {
    (void)c; (void)low; (void)high; (void)dur;
    return -1;
}

/* Message Box */

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

/* Touch Events */

static inline int SDL_GetNumTouchFingers(long long touchID) {
    (void)touchID;
    return 0; /* No multi-touch on Amiga. */
}

#ifdef __cplusplus
}
#endif

#endif /* USE_SDL_STUBS */
#endif /* AMIGA_SDL_STUBS_H */
