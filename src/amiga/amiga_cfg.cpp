/***************************************************************************
 * amiga_cfg.cpp - persistent settings for the Amiga port.
 *
 * Reads/writes "amiga.cfg" in the current directory (the game drawer,
 * same place as the MP3/ folder).  Pure stdio, no SDL / no OS calls.
 * Format:  key = value        (volumes 0..127, clamped)
 *          key = ACTION       (CD32 buttons, text action names)
 *          ';' or '#' at line start = comment, blank lines allowed.
 *
 * Defaults when the file or a key is missing: all volumes 127,
 * cd32=OFF (classic joystick) and the built-in CD32 button assignments
 * below.  Unknown action names keep the safe defaults.
 ***************************************************************************/

#ifdef __AMIGA__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "amiga/amiga_cfg.h"

int amiga_cfg_music_adlib = 127;
int amiga_cfg_music_mhi   = 127;
int amiga_cfg_music_wave  = 127;
int amiga_cfg_sfx         = 127;

int amiga_cfg_cd32 = 0;

/* Safe defaults: only the classic pad layout is active. */
int amiga_cfg_cd32_red     = AMIGA_CFG_CD32_FIRE;
int amiga_cfg_cd32_blue    = AMIGA_CFG_CD32_SPECIAL_SELECT;
int amiga_cfg_cd32_green   = AMIGA_CFG_CD32_MEGA_BOMB;
int amiga_cfg_cd32_yellow  = AMIGA_CFG_CD32_NONE;
int amiga_cfg_cd32_reverse = AMIGA_CFG_CD32_NONE;
int amiga_cfg_cd32_forward = AMIGA_CFG_CD32_NONE;
int amiga_cfg_cd32_play    = AMIGA_CFG_CD32_PAUSE;

/* Names must match the AMIGA_CFG_CD32_* ids in amiga_cfg.h. */
static const char *const cd32_action_names[AMIGA_CFG_CD32_NUM_ACTIONS] = {
    "NONE",
    "FIRE",
    "SPECIAL_SELECT",
    "MEGA_BOMB",
    "PAUSE",
    "CANCEL"
};

/***************************************************************************
 * cd32_action_name() - name for an action id; out-of-range ids (should
 * never happen) print as NONE so the saved file stays valid.
 ***************************************************************************/
static const char *
cd32_action_name(int action)
{
    if (action < 0 || action >= AMIGA_CFG_CD32_NUM_ACTIONS)
        action = AMIGA_CFG_CD32_NONE;
    return cd32_action_names[action];
}

/***************************************************************************
 * cd32_parse_action() - map a text action name to its id; unknown names
 * keep the current (default) value.
 ***************************************************************************/
static int
cd32_parse_action(const char *word, int current)
{
    int i;

    for (i = 0; i < AMIGA_CFG_CD32_NUM_ACTIONS; i++)
    {
        if (strcasecmp(word, cd32_action_names[i]) == 0)
            return i;
    }
    return current;
}

/***************************************************************************
 * AmigaCfg_Load() - read amiga.cfg; missing/unreadable file keeps the
 * built-in defaults above.
 ***************************************************************************/
void
AmigaCfg_Load(void)
{
    FILE *f;
    char line[160];
    static int loaded = 0;

    /* Load once per session: the first call happens in SDL_Init() before
     * the gameport mode is chosen (cd32 key); the later SND_InitSound()
     * call for the volume keys then reuses the values already in memory. */
    if (loaded)
        return;
    loaded = 1;

    f = fopen("amiga.cfg", "r");
    if (!f)
    {
        /* First run: create amiga.cfg with the built-in defaults so the
         * user immediately has a visible, editable config file. */
        AmigaCfg_Save();
        return;
    }

    while (fgets(line, sizeof(line), f))
    {
        char key[64];
        char word[32];
        char *p = line;

        /* Skip leading whitespace. */
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
            p++;

        /* Comment / empty line. */
        if (*p == 0 || *p == ';' || *p == '#')
            continue;

        if (sscanf(p, "%63[A-Za-z0-9_] = %31s", key, word) != 2)
            continue;

        /* AUDIO block: numeric volumes, clamped to 0..127.  A non-numeric
         * value keeps the current (default) volume. */
        if (strcmp(key, "music_adlib") == 0 ||
            strcmp(key, "music_mhi") == 0 ||
            strcmp(key, "music_wave") == 0 ||
            strcmp(key, "sfx_volume") == 0)
        {
            char *end;
            long val = strtol(word, &end, 10);

            if (end == word)
                continue;
            if (val < 0)
                val = 0;
            if (val > 127)
                val = 127;

            if (strcmp(key, "music_adlib") == 0)
                amiga_cfg_music_adlib = (int)val;
            else if (strcmp(key, "music_mhi") == 0)
                amiga_cfg_music_mhi = (int)val;
            else if (strcmp(key, "music_wave") == 0)
                amiga_cfg_music_wave = (int)val;
            else
                amiga_cfg_sfx = (int)val;
        }
        /* JOYSTICK / CD32 block: only ON enables the pad; anything else
         * keeps the safe OFF default. */
        else if (strcmp(key, "cd32") == 0)
        {
            if (strcasecmp(word, "ON") == 0)
                amiga_cfg_cd32 = 1;
            else if (strcasecmp(word, "OFF") == 0)
                amiga_cfg_cd32 = 0;
        }
        else if (strcmp(key, "cd32_red") == 0)
            amiga_cfg_cd32_red = cd32_parse_action(word, amiga_cfg_cd32_red);
        else if (strcmp(key, "cd32_blue") == 0)
            amiga_cfg_cd32_blue = cd32_parse_action(word, amiga_cfg_cd32_blue);
        else if (strcmp(key, "cd32_green") == 0)
            amiga_cfg_cd32_green = cd32_parse_action(word, amiga_cfg_cd32_green);
        else if (strcmp(key, "cd32_yellow") == 0)
            amiga_cfg_cd32_yellow = cd32_parse_action(word, amiga_cfg_cd32_yellow);
        else if (strcmp(key, "cd32_reverse") == 0)
            amiga_cfg_cd32_reverse = cd32_parse_action(word, amiga_cfg_cd32_reverse);
        else if (strcmp(key, "cd32_forward") == 0)
            amiga_cfg_cd32_forward = cd32_parse_action(word, amiga_cfg_cd32_forward);
        else if (strcmp(key, "cd32_play") == 0)
            amiga_cfg_cd32_play = cd32_parse_action(word, amiga_cfg_cd32_play);
        /* Unknown keys are ignored, so old and future files both load. */
    }

    fclose(f);
}

/***************************************************************************
 * AmigaCfg_Save() - write the current values back to amiga.cfg.  Always
 * writes BOTH blocks, so saving volumes from the Options menu never
 * drops the CD32 assignments.
 ***************************************************************************/
void
AmigaCfg_Save(void)
{
    FILE *f;

    f = fopen("amiga.cfg", "w");
    if (!f)
        return;

    fprintf(f, "; ==== AUDIO ====\n");
    fprintf(f, "; Raptor Amiga volumes (0..127, 127 = loud)\n");
    fprintf(f, "music_adlib = %d\n", amiga_cfg_music_adlib);
    fprintf(f, "music_mhi   = %d\n", amiga_cfg_music_mhi);
    fprintf(f, "music_wave  = %d\n", amiga_cfg_music_wave);
    fprintf(f, "sfx_volume  = %d\n", amiga_cfg_sfx);

    fprintf(f, "\n");
    fprintf(f, "; ==== JOYSTICK / CD32 ====\n");
    fprintf(f, "; cd32 = OFF : classic joystick on port 1 (default, unchanged behaviour)\n");
    fprintf(f, "; cd32 = ON  : CD32 pad on port 1 (lowlevel.library game controller mode)\n");
    fprintf(f, "; NOJOY / JOYSTICK=OFF takes precedence over this block.\n");
    fprintf(f, "; Button assignments, one value per physical pad button:\n");
    fprintf(f, ";   FIRE           - main guns\n");
    fprintf(f, ";   SPECIAL_SELECT - fire the selected special weapon\n");
    fprintf(f, ";   MEGA_BOMB      - launch a mega bomb\n");
    fprintf(f, ";   PAUSE          - pause the game\n");
    fprintf(f, ";   CANCEL         - cancel / back out\n");
    fprintf(f, ";   NONE           - button disabled\n");
    fprintf(f, "cd32 = %s\n", amiga_cfg_cd32 ? "ON" : "OFF");
    fprintf(f, "cd32_red     = %s ; Red button\n",
            cd32_action_name(amiga_cfg_cd32_red));
    fprintf(f, "cd32_blue    = %s ; Blue button\n",
            cd32_action_name(amiga_cfg_cd32_blue));
    fprintf(f, "cd32_green   = %s ; Green button\n",
            cd32_action_name(amiga_cfg_cd32_green));
    fprintf(f, "cd32_yellow  = %s ; Yellow button\n",
            cd32_action_name(amiga_cfg_cd32_yellow));
    fprintf(f, "cd32_reverse = %s ; Reverse button (left shoulder)\n",
            cd32_action_name(amiga_cfg_cd32_reverse));
    fprintf(f, "cd32_forward = %s ; Forward button (right shoulder)\n",
            cd32_action_name(amiga_cfg_cd32_forward));
    fprintf(f, "cd32_play    = %s ; Play button (triangle)\n",
            cd32_action_name(amiga_cfg_cd32_play));

    fclose(f);
}

#endif /* __AMIGA__ */