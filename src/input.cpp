#include "SDL.h"
#include "stdio.h"
#include "common.h"
#include "windows.h"
#include "kbdapi.h"
#include "ptrapi.h"
#include "rap.h"
#include "demo.h"
#include "input.h"
#include "i_video.h"
#include "joyapi.h"

#define MAX_ADDX 10
#define MAX_ADDY 8

int control = 1;
int haptic;
int joy_ipt_MenuNew;

int k_Up;
int k_Down;
int k_Left;
int k_Right;
int k_Fire;
int k_FireSp;
int k_ChangeSp;
int k_Mega;

int m_lookup[3];
int j_lookup[4];

int buttons[4];

int xm, ym;
int g_addx, g_addy;

int ipt_start;
int control_pause;

/*------------------------------------------------------------------------
   IPT_GetButtons () - Reads in Joystick and Keyboard game buttons
  ------------------------------------------------------------------------*/
void 
IPT_GetButtons(
    void
)
{
    /* TEST: Amiga input lag regression — remove the 26 Hz frame limiter.
     * The original code throttled input polling to ~26 FPS (1000/26 ms per
     * call).  On the Amiga port this introduces a 1-2 frame delay between
     * a keypress/joystick move and the game responding, making both keyboard
     * and joystick control feel sluggish.  Removing the rate-limiter lets
     * IPT_GetButtons() run at the full game loop rate (~50-70 Hz) so input
     * is sampled every frame.
     *
     * If this patch fixes the input lag, the limiter was the primary cause
     * of the regression and should remain removed permanently for the Amiga
     * 68k RTG build. */
    
    if (!ipt_start)
        return;

#if 0
    int num;
    
    if (control == I_JOYSTICK)
    {
        num = inp(0x2);
        
        num >>= 4;
        
        if ((num & 1) == 0)
            buttons[j_lookup[0]] = 1;
        if ((num & 2) == 0)
            buttons[j_lookup[1]] = 1;
        if ((num & 4) == 0)
            buttons[j_lookup[2]] = 1;
        if ((num & 8) == 0)
            buttons[j_lookup[3]] = 1;
    }
#endif
    
    if (KBD_Key(k_Fire))
        buttons[0] = 1;
    
    if (KBD_Key(k_FireSp))
        buttons[1] = 1;
    
    if (KBD_Key(k_ChangeSp))
        buttons[2] = 1;
    
    if (KBD_Key(k_Mega))
        buttons[3] = 1;

#ifdef __AMIGA__
    /* Amiga: joystick buttons work in parallel with the keyboard.
     * A = RED (fire button 1), B = BLUE/PLAY (fire button 2 on 2-button
     * joysticks and CD32 pads).  Mapping follows j_lookup ([JoyStick]
     * section of setup.ini): A -> Fire, B -> Fire Special by default. */
    if (AButton)
        buttons[j_lookup[0]] = 1;
    if (BButton)
        buttons[j_lookup[1]] = 1;
    
    /* Amiga: mouse buttons are always active in-game too, regardless of
     * the selected control device (m_lookup = [Mouse] in setup.ini):
     * LMB -> Fire, RMB -> Fire Special, MMB -> Change Special. */
    if (mouseb1)
        buttons[m_lookup[0]] = 1;
    if (mouseb2)
        buttons[m_lookup[1]] = 1;
    if (mouseb3)
        buttons[m_lookup[2]] = 1;
#endif
}

/*------------------------------------------------------------------------
IPT_GetJoyStick()
  ------------------------------------------------------------------------*/
void 
IPT_GetJoyStick(
    void
)
{
    //Get Button
    
    if (AButton)
    {
        if (AButtonconvert == j_lookup[0])                        //Fire
            buttons[0] = 1;
        if (AButtonconvert == j_lookup[1])                        //Fire Special
            buttons[1] = 1;
        if (AButtonconvert == j_lookup[2])                        //Change Special
            buttons[2] = 1;
        if (AButtonconvert == j_lookup[3])                        //Mega
            buttons[3] = 1;
    }
    
    if (BButton)
    {
        if (BButtonconvert == j_lookup[0])                        //Fire
            buttons[0] = 1;
        if (BButtonconvert == j_lookup[1])                        //Fire Special
            buttons[1] = 1;
        if (BButtonconvert == j_lookup[2])                        //Change Special
            buttons[2] = 1;
        if (BButtonconvert == j_lookup[3])                        //Mega
            buttons[3] = 1;
    }
    
    if (XButton)
    {
        if (XButtonconvert == j_lookup[0])                        //Fire
            buttons[0] = 1;
        if (XButtonconvert == j_lookup[1])                        //Fire Special
            buttons[1] = 1;
        if (XButtonconvert == j_lookup[2])                        //Change Special
            buttons[2] = 1;
        if (XButtonconvert == j_lookup[3])                        //Mega
            buttons[3] = 1;
    }
    
    if (YButton)
    {
        if (YButtonconvert == j_lookup[0])                        //Fire
            buttons[0] = 1;
        if (YButtonconvert == j_lookup[1])                        //Fire Special
            buttons[1] = 1;
        if (YButtonconvert == j_lookup[2])                        //Change Special
            buttons[2] = 1;
        if (YButtonconvert == j_lookup[3])                        //Mega
            buttons[3] = 1;
    }
    
    if (TriggerRight > 0)                                         //Fire
        buttons[0] = 1;
    if (TriggerLeft > 0)                                          //Fire Special
        buttons[1] = 1;
    if (LeftShoulder)                                             //Change Special
        buttons[2] = 1;
    if (RightShoulder)                                            //Mega
        buttons[3] = 1;
       
    //Move Player DPad
           
    if (Left)
    {
        if (g_addx >= 0)
            g_addx = -1;
        g_addx--;
        if (-g_addx > MAX_ADDX)
            g_addx = -MAX_ADDX;
    }
    else if (Right)
    {
             if (g_addx <= 0)
                 g_addx = 1;
             g_addx++;
             if (g_addx > MAX_ADDX)
                 g_addx = MAX_ADDX;
    }
    else
    {
        if (g_addx)
            g_addx /= 2;
    }
    
    if (Up)
    {
        if (g_addy >= 0)
            g_addy = -1;
        g_addy--;
        if (-g_addy > MAX_ADDY)
            g_addy = -MAX_ADDY;
    }
    else if (Down)
    {
             if (g_addy <= 0)
                 g_addy = 1;
             g_addy++;
             if (g_addy > MAX_ADDY)
                 g_addy = MAX_ADDY;
    }
    else
    {
        if (g_addy)
            g_addy /= 2;
    }

    //Move Player Analog Stick

    if (StickX != 0)
    {
        if (StickX > 0)
            StickX *= 2;
        if (StickX > MAX_ADDX)
            StickX = MAX_ADDX;
        if (StickX < 0)
            StickX *= 2;
        if (StickX < -MAX_ADDX)
            StickX = -MAX_ADDX;
        g_addx = StickX;
    }
    
    if (StickY != 0)
    {
        if (StickY > 0)
            StickY *= 2;
        if (StickY > MAX_ADDY)
            StickY = MAX_ADDY;
        if (StickY < 0)
            StickY *= 2;
        if (StickY < -MAX_ADDY)
            StickY = -MAX_ADDY;
        g_addy = StickY;
    }
}

/*------------------------------------------------------------------------
IPT_GetKeyBoard (
  ------------------------------------------------------------------------*/
void 
IPT_GetKeyBoard(
    void
)
{
    if (KBD_Key(k_Left) || KBD_Key(k_Right))
    {
        if (KBD_Key(k_Left))
        {
            if (g_addx >= 0)
                g_addx = -1;
            g_addx--;
            if (-g_addx > MAX_ADDX)
                g_addx = -MAX_ADDX;
        }
        else if (KBD_Key(k_Right))
        {
            if (g_addx <= 0)
                g_addx = 1;
            g_addx++;
            if (g_addx > MAX_ADDX)
                g_addx = MAX_ADDX;
        }
    }
    else
    {
        if (g_addx)
            g_addx /= 2;
    }
    
    if (KBD_Key(k_Up) || KBD_Key(k_Down))
    {
        if (KBD_Key(k_Up))
        {
            if (g_addy >= 0)
                g_addy = -1;
            g_addy--;
            if (-g_addy > MAX_ADDY)
                g_addy = -MAX_ADDY;
        }
        else if (KBD_Key(k_Down))
        {
            if (g_addy <= 0)
                g_addy = 1;
            g_addy++;
            if (g_addy > MAX_ADDY)
                g_addy = MAX_ADDY;
        }
    }
    else
    {
        if (g_addy)
            g_addy /= 2;
    }
}

/*------------------------------------------------------------------------
IPT_GetMouse (
  ------------------------------------------------------------------------*/
void 
IPT_GetMouse(
    void
)
{
    int plx, ply, ptrx, ptry;
    
    plx = playerx + (PLAYERWIDTH / 2);
    ply = playery + (PLAYERHEIGHT / 2);
    
    ptrx = cur_mx;
    ptry = cur_my;
    
    xm = ptrx - plx;
    ym = ptry - ply;
    
    if (xm)
    {
        xm >>= 3;
        
        if (!xm)
            xm = 1;
        else if (xm > 10)
            xm = 10;
        else if (xm < -10)
            xm = -10;
    }
    
    if (ym)
    {
        ym >>= 3;
        
        if (!ym)
            ym = 1;
        else if (ym > 10)
            ym = 10;
        else if (ym < -10)
            ym = -10;
    }
    
    g_addx = xm;
    g_addy = ym;
    
    if (mouseb1)
        buttons[m_lookup[0]] = 1;
    
    if (mouseb2)
        buttons[m_lookup[1]] = 1;
    
    if (mouseb3)
        buttons[m_lookup[2]] = 1;
}

/*------------------------------------------------------------------------
IPT_MouseGrab (
  ------------------------------------------------------------------------*/
bool 
IPT_MouseGrab(
    void
)
{
    return ipt_start;
}

/***************************************************************************
IPT_Init () - Initializes INPUT system
 ***************************************************************************/
void 
IPT_Init(
    void
)
{
    
    I_SetGrabMouseCallback(IPT_MouseGrab);
    // ipt_tsm = TSM_NewService(IPT_GetButtons, 26, 254, 1);
    IPT_CalJoy();
#ifdef __AMIGA__
    control = I_JOYSTICK;
#endif
}

/***************************************************************************
IPT_DeInit() - Freeze up resources used by INPUT system
 ***************************************************************************/
void 
IPT_DeInit(
    void
)
{
    // TSM_DelService(ipt_tsm);
}

/***************************************************************************
IPT_Start() - Tranfers control from PTRAPI stuff to IPT stuff
 ***************************************************************************/
void 
IPT_Start(
    void
)
{
    PTR_DrawCursor(0);
    PTR_Pause(1);
    ipt_start = 1;
    // TSM_ResumeService(ipt_tsm);
}

/***************************************************************************
IPT_End() - Tranfers control from IPT stuff to PTR stuff
 ***************************************************************************/
void 
IPT_End(
    void
)
{
    ipt_start = 0;
    // TSM_PauseService(ipt_tsm);
    PTR_Pause(0);
    PTR_DrawCursor(0);
    /* On Amiga: the system pointer is hidden once at window open (SDL_CreateWindow)
     * and stays hidden for the entire game session.  Do NOT restore it here when
     * returning from gameplay to the menu - that would cause the native Amiga
     * arrow to appear on top of the software crosshair.  The system pointer is
     * restored only at full application exit via SDL_Quit() / ShutDown(). */
}

/***************************************************************************
IPT_MovePlayer() - Perform player movement for current input device
 ***************************************************************************/
void 
IPT_MovePlayer(
    void
)
{
    static int oldx = PLAYERINITX;
    int delta;
    
    if (demo_mode == DEMO_PLAYBACK)
        return;
    
    if (!control_pause)
    {
        switch (control)
        {
        case I_KEYBOARD:
        default:
            IPT_GetKeyBoard();
            break;
        
        case I_JOYSTICK:
#ifdef __AMIGA__
            /* Keyboard always runs for arrow-key support.
             * Joystick runs ONLY when it has active directional input,
             * so its decay (else) branches don't cut keyboard
             * acceleration in half every frame. */
            IPT_GetKeyBoard();
            if (Left || Right || Up || Down || StickX != 0 || StickY != 0)
                IPT_GetJoyStick();
            
            /* Amiga: mouse steering in-game, parallel to keyboard+joystick.
             * "Last active device wins": mouse control (absolute cursor-to-
             * player positioning, original I_MOUSE mode) engages only when
             * the physical mouse actually moved since the previous frame or
             * a mouse button is held.  cur_mx/cur_my are refreshed every
             * frame by PTR_MouseHandler() in I_GetEvent(). */
            {
                static int old_cur_mx = -1;
                static int old_cur_my = -1;
                
                if (cur_mx != old_cur_mx || cur_my != old_cur_my ||
                    mouseb1 || mouseb2 || mouseb3)
                {
                    IPT_GetMouse();
                }
                
                old_cur_mx = cur_mx;
                old_cur_my = cur_my;
            }
#else
            IPT_GetJoyStick();
#endif
            break;

        case I_MOUSE:
            IPT_GetMouse();
            break;
        }
    }
    
    playerx += g_addx;
    playery += g_addy;
    
    if (startendwave == -1)
    {
        if (playery < MINPLAYERY)
        {
            playery = MINPLAYERY;
            g_addy = 0;
        }
        else if (playery > MAXPLAYERY)
        {
            playery = MAXPLAYERY;
            g_addy = 0;
        }
        
        if (playerx < PLAYERMINX)
        {
            playerx = PLAYERMINX;
            g_addx = 0;
        }
        else if (playerx + PLAYERWIDTH > PLAYERMAXX)
        {
            playerx = PLAYERMAXX - PLAYERWIDTH;
            g_addx = 0;
        }
    }
    
    delta = abs(playerx - oldx);
    delta >>= 2;
    
    if (delta > 3)
        delta = 3;
    
    if (playerx < oldx)
    {
        if (playerbasepic + delta > playerpic)
            playerpic++;
    }
    else if (playerx > oldx)
    {
        if (playerbasepic - delta < playerpic)
            playerpic--;
    }
    else
    {
        if (playerpic > playerbasepic)
            playerpic--;
        else if (playerpic < playerbasepic)
            playerpic++;
    }
    
    oldx = playerx;
    
    player_cx = playerx + (PLAYERWIDTH / 2);
    player_cy = playery + (PLAYERHEIGHT / 2);
}

/***************************************************************************
IPT_PauseControl() - Lets routines run without letting user control anyting
 ***************************************************************************/
void 
IPT_PauseControl(
    int flag
)
{
    control_pause = flag;
}

/***************************************************************************
IPT_FMovePlayer() - Forces player to move addx/addy
 ***************************************************************************/
void 
IPT_FMovePlayer(
    int addx,              // INPUT : add to x pos
    int addy               // INPUT : add to y pos
)
{
    g_addx = addx;
    g_addy = addy;
    
    IPT_MovePlayer();
}

/***************************************************************************
IPT_LoadPrefs() - Set input prefs (fixed built-in defaults, no SETUP.INI)
 ***************************************************************************/
void
IPT_LoadPrefs(
    void
)
{
    /* This Amiga port does not use SETUP.INI at all - all preferences are
     * fixed built-in defaults (the classic Raptor mapping). */
    opt_detail = 1;

    /* Amiga: always joystick mode - keyboard, mouse and joystick all work
     * in parallel, both in the menus and in the game. */
    control = I_JOYSTICK;

    haptic = 1;

    /* Amiga: always use joy_ipt_MenuNew mode so the D-pad/analog stick
     * navigates menus with discrete steps (like keyboard arrows) instead of
     * floating the crosshair like a mouse.  This also disables
     * PTR_JoyHandler() which was the source of the analog cursor drift in
     * menus. */
    joy_ipt_MenuNew = 1;

    k_Up = SC_UP;
    k_Down = SC_DOWN;
    k_Left = SC_LEFT;
    k_Right = SC_RIGHT;
    k_Fire = SC_CTRL;
    k_FireSp = SC_ALT;
    k_ChangeSp = SC_SPACE;
    k_Mega = SC_RIGHT_SHIFT;

    m_lookup[0] = 0;
    m_lookup[1] = 1;
    m_lookup[2] = 2;

    j_lookup[0] = 0;
    j_lookup[1] = 1;
    j_lookup[2] = 2;
    j_lookup[3] = 3;
}
