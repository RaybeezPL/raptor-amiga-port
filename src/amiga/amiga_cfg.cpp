/***************************************************************************
 * amiga_cfg.cpp - persistent audio volume settings for the Amiga port.
 *
 * Reads/writes "amiga.cfg" in the current directory (the game drawer,
 * same place as the MP3/ folder).  Pure stdio, no SDL / no OS calls.
 * Format:  key = value        (0..127, value clamped)
 *          ';' or '#' at line start = comment, blank lines allowed.
 *
 * Defaults when the file is missing: music_adlib=127, music_mhi=127,
 * sfx_volume=127.
 ***************************************************************************/

#ifdef __AMIGA__

#include <stdio.h>
#include <string.h>

#include "amiga/amiga_cfg.h"

int amiga_cfg_music_adlib = 127;
int amiga_cfg_music_mhi   = 127;
int amiga_cfg_music_wave  = 127;
int amiga_cfg_sfx         = 127;

/***************************************************************************
 * AmigaCfg_Load() - read amiga.cfg; missing/unreadable file keeps the
 * built-in defaults above.
 ***************************************************************************/
void
AmigaCfg_Load(void)
{
    FILE *f;
    char line[160];

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
        int  val;
        char *p = line;

        /* Skip leading whitespace. */
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
            p++;

        /* Comment / empty line. */
        if (*p == 0 || *p == ';' || *p == '#')
            continue;

        if (sscanf(p, "%63[A-Za-z0-9_] = %d", key, &val) == 2)
        {
            if (val < 0)
                val = 0;
            if (val > 127)
                val = 127;

            if (strcmp(key, "music_adlib") == 0)
                amiga_cfg_music_adlib = val;
            else if (strcmp(key, "music_mhi") == 0)
                amiga_cfg_music_mhi = val;
            else if (strcmp(key, "music_wave") == 0)
                amiga_cfg_music_wave = val;
            else if (strcmp(key, "sfx_volume") == 0)
                amiga_cfg_sfx = val;
        }
    }

    fclose(f);
}

/***************************************************************************
 * AmigaCfg_Save() - write the current values back to amiga.cfg.
 ***************************************************************************/
void
AmigaCfg_Save(void)
{
    FILE *f;

    f = fopen("amiga.cfg", "w");
    if (!f)
        return;

    fprintf(f, "; Raptor Amiga audio (0..127, 127 = loud)\n");
    fprintf(f, "music_adlib = %d\n", amiga_cfg_music_adlib);
    fprintf(f, "music_mhi   = %d\n", amiga_cfg_music_mhi);
    fprintf(f, "music_wave  = %d\n", amiga_cfg_music_wave);
    fprintf(f, "sfx_volume  = %d\n", amiga_cfg_sfx);

    fclose(f);
}

#endif /* __AMIGA__ */