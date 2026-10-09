#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "amiga/amiga_cfg.h"

static void
reset_values(void)
{
    amiga_cfg_music_adlib = 127;
    amiga_cfg_music_mhi = 127;
    amiga_cfg_music_wave = 127;
    amiga_cfg_sfx = 127;
    amiga_cfg_cd32 = 0;
    amiga_cfg_cd32_red = AMIGA_CFG_CD32_FIRE;
    amiga_cfg_cd32_blue = AMIGA_CFG_CD32_SPECIAL_SELECT;
    amiga_cfg_cd32_green = AMIGA_CFG_CD32_MEGA_BOMB;
    amiga_cfg_cd32_yellow = AMIGA_CFG_CD32_NONE;
    amiga_cfg_cd32_reverse = AMIGA_CFG_CD32_NONE;
    amiga_cfg_cd32_forward = AMIGA_CFG_CD32_NONE;
    amiga_cfg_cd32_play = AMIGA_CFG_CD32_PAUSE;
    amiga_cfg_joy_button1 = AMIGA_CFG_CD32_FIRE;
    amiga_cfg_joy_button2 = AMIGA_CFG_CD32_SPECIAL_SELECT;
    amiga_cfg_joy_button3 = AMIGA_CFG_CD32_MEGA_BOMB;
    AmigaCfg_TestResetLoad();
}

static void
write_file(const char *path, const char *contents)
{
    FILE *f = fopen(path, "wb");
    assert(f != 0);
    assert(fwrite(contents, 1, strlen(contents), f) == strlen(contents));
    assert(fclose(f) == 0);
}

static char *
read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    long length;
    char *contents;

    assert(f != 0);
    assert(fseek(f, 0, SEEK_END) == 0);
    length = ftell(f);
    assert(length >= 0);
    assert(fseek(f, 0, SEEK_SET) == 0);
    contents = (char *)malloc((size_t)length + 1);
    assert(contents != 0);
    assert(fread(contents, 1, (size_t)length, f) == (size_t)length);
    assert(fclose(f) == 0);
    contents[length] = 0;
    return contents;
}

static int
file_exists(const char *path)
{
    return access(path, F_OK) == 0;
}

static int
count_text(const char *contents, const char *needle)
{
    int count = 0;
    size_t needle_length = strlen(needle);
    const char *p = contents;

    while ((p = strstr(p, needle)) != 0)
    {
        count++;
        p += needle_length;
    }
    return count;
}

/* Removes every amiga.cfg.bakN copy so a migration test can assert on the
 * exact backup name it expects. */
static void
remove_backups(const char *path)
{
    char name[600];
    int number;

    for (number = 0; number < 100; number++)
    {
        snprintf(name, sizeof(name), "%s.bak%d", path, number);
        remove(name);
    }
}

/* Loads the current amiga.cfg once and asserts that the file bytes are
 * byte-identical afterwards and that no migration backup (amiga.cfg.bakN)
 * was created. Used by the cases that must never trigger the migration. */
static void
load_and_assert_untouched(const char *path)
{
    char *before = read_file(path);
    char *after;
    char backup[640];
    int number;

    reset_values();
    AmigaCfg_Load();
    after = read_file(path);
    assert(strcmp(before, after) == 0);
    free(before);
    free(after);

    for (number = 0; number < 100; number++)
    {
        snprintf(backup, sizeof(backup), "%s.bak%d", path, number);
        assert(!file_exists(backup));
    }
}

int
main(void)
{
    char directory[] = "/tmp/raptor-amiga-cfg-XXXXXX";
    char path[512];
    char *contents;

    assert(mkdtemp(directory) != 0);
    snprintf(path, sizeof(path), "%s/amiga.cfg", directory);
    AmigaCfg_TestSetPath(path);

    /* 1. Legacy configuration containing only the audio volumes is read and
     * then migrated once: the own values are kept and the file is completed
     * with the missing keys and the JOYSTICK blocks. */
    remove_backups(path);
    write_file(path, "music_adlib=1\nmusic_mhi=2\nmusic_wave=3\nsfx_volume=4\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_music_adlib == 1 && amiga_cfg_music_mhi == 2);
    assert(amiga_cfg_music_wave == 3 && amiga_cfg_sfx == 4);
    contents = read_file(path);
    assert(strstr(contents,
        "music_adlib=1\nmusic_mhi=2\nmusic_wave=3\nsfx_volume=4\n") == contents);
    assert(strstr(contents, "; ==== JOYSTICK / 1-2-3 BUTTONS ====\n") != 0);
    assert(strstr(contents, "; ==== JOYSTICK / CD32 ====\n") != 0);
    assert(count_text(contents, "cd32 = OFF\n") == 1);
    free(contents);
    /* An Options save still rewrites volume values only, in place. */
    amiga_cfg_music_adlib = 11;
    AmigaCfg_Save();
    contents = read_file(path);
    assert(strstr(contents, "music_adlib=11\n") != 0);
    assert(strstr(contents, "music_mhi=2\n") != 0);
    assert(strstr(contents, "; ==== JOYSTICK / CD32 ====\n") != 0);
    assert(count_text(contents, "music_adlib") == 1);
    free(contents);

    /* 2. Full cfg retains customized CD32 assignments. */
    write_file(path,
        "music_adlib = 10\ncd32 = ON\ncd32_red = CANCEL\n"
        "cd32_blue = PAUSE\ncd32_play = FIRE\nmusic_mhi = 20\n"
        "music_wave = 30\nsfx_volume = 40\n");
    reset_values();
    AmigaCfg_Load();
    amiga_cfg_sfx = 41;
    AmigaCfg_Save();
    contents = read_file(path);
    assert(strstr(contents, "cd32 = ON\ncd32_red = CANCEL\ncd32_blue = PAUSE\ncd32_play = FIRE\n") != 0);
    assert(strstr(contents, "sfx_volume = 41\n") != 0);
    free(contents);

    /* 3. Preserve comments, unknowns, unusual whitespace, CRLF and no final LF. */
    write_file(path,
        "; heading\r\nMUSIC_ADLIB\t=\t7 ; retain this\r\nunknown = value\r\n"
        "music_mhi = 8#also retained\r\nmusic_wave=9\r\nsfx_volume = 10 ; tail");
    reset_values();
    AmigaCfg_Load();
    amiga_cfg_music_mhi = 88;
    AmigaCfg_Save();
    contents = read_file(path);
    assert(strcmp(contents,
        "; heading\r\nMUSIC_ADLIB\t=\t7 ; retain this\r\nunknown = value\r\n"
        "music_mhi = 88#also retained\r\nmusic_wave=9\r\nsfx_volume = 10 ; tail") == 0);
    free(contents);

    /* 4. Missing and repeated, case-insensitive volume keys. */
    write_file(path, "Music_AdLib = 1 ; first\nMUSIC_ADLIB=2 ; last\ncustom=yes\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_music_adlib == 2);
    amiga_cfg_music_adlib = 42;
    amiga_cfg_music_mhi = 43;
    amiga_cfg_music_wave = 44;
    amiga_cfg_sfx = 45;
    AmigaCfg_Save();
    contents = read_file(path);
    assert(strstr(contents, "Music_AdLib = 42 ; first\nMUSIC_ADLIB=42 ; last\ncustom=yes\n") != 0);
    assert(count_text(contents, "music_mhi = 43\n") == 1);
    assert(count_text(contents, "music_wave = 44\n") == 1);
    assert(count_text(contents, "sfx_volume = 45\n") == 1);
    free(contents);
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_music_adlib == 42 && amiga_cfg_music_mhi == 43);
    assert(amiga_cfg_music_wave == 44 && amiga_cfg_sfx == 45);

    /* 5. Load/change one/Save/Load preserves other values and contents. */
    write_file(path, "music_adlib = 12\n; keep\nmusic_mhi = 13\n"
                     "music_wave = 14\nsfx_volume = 15\ncd32_green = CANCEL\n");
    reset_values();
    AmigaCfg_Load();
    amiga_cfg_music_mhi = 77;
    AmigaCfg_Save();
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_music_adlib == 12 && amiga_cfg_music_mhi == 77);
    assert(amiga_cfg_music_wave == 14 && amiga_cfg_sfx == 15);
    contents = read_file(path);
    assert(strstr(contents, "; keep\n") != 0 && strstr(contents, "cd32_green = CANCEL\n") != 0);
    free(contents);

    /* 6. Repeated saves append each missing volume key once and never add the
     * JOYSTICK blocks to an existing (non-migratable) file. The unknown key
     * also keeps the file out of the legacy volumes-only migration. */
    write_file(path, "music_adlib = 1\ncustom = yes\n");
    reset_values();
    AmigaCfg_Load();
    AmigaCfg_Save();
    AmigaCfg_Save();
    contents = read_file(path);
    assert(count_text(contents, "music_mhi = ") == 1);
    assert(count_text(contents, "music_wave = ") == 1);
    assert(count_text(contents, "sfx_volume = ") == 1);
    assert(strstr(contents, "custom = yes\n") != 0);
    assert(strstr(contents, "cd32 = ") == 0);
    assert(strstr(contents, "joy_button") == 0);
    free(contents);

    /* 7. Temp-file/write and replacement failures leave the original intact. */
    write_file(path, "music_adlib = 9\ncustom = original\n");
    amiga_cfg_music_adlib = 99;
    AmigaCfg_TestFailNextTempOpen();
    AmigaCfg_Save();
    contents = read_file(path);
    assert(strcmp(contents, "music_adlib = 9\ncustom = original\n") == 0);
    free(contents);
    AmigaCfg_TestFailNextReplace();
    AmigaCfg_Save();
    contents = read_file(path);
    assert(strcmp(contents, "music_adlib = 9\ncustom = original\n") == 0);
    free(contents);
    AmigaCfg_TestFailReplacementAfterBackup();
    AmigaCfg_Save();
    contents = read_file(path);
    assert(strcmp(contents, "music_adlib = 9\ncustom = original\n") == 0);
    free(contents);

    /* 8. Old cfg without any joystick keys keeps the built-in defaults and
     * an Options save still must not add the block to an existing file. */
    write_file(path, "music_adlib = 5\ncd32 = ON\ncd32_red = CANCEL\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_music_adlib == 5);
    assert(amiga_cfg_cd32 == 1 && amiga_cfg_cd32_red == AMIGA_CFG_CD32_CANCEL);
    assert(amiga_cfg_joy_button1 == AMIGA_CFG_CD32_FIRE);
    assert(amiga_cfg_joy_button2 == AMIGA_CFG_CD32_SPECIAL_SELECT);
    assert(amiga_cfg_joy_button3 == AMIGA_CFG_CD32_MEGA_BOMB);
    AmigaCfg_Save();
    contents = read_file(path);
    assert(count_text(contents, "joy_button1 = ") == 0);
    assert(count_text(contents, "joy3 = ") == 0);
    assert(strstr(contents, "cd32 = ON\ncd32_red = CANCEL\n") != 0);
    free(contents);

    /* 9. New joy_button keys are accepted with mixed case and every action;
     * the last valid value wins on a repeated key. */
    write_file(path,
        "Joy_Button1\t=\tFire\n"
        "joy_button1 = CANCEL\n"
        "JOY_BUTTON2\t=\tspecial_select\n"
        "joy_button3 = none\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_joy_button1 == AMIGA_CFG_CD32_CANCEL);
    assert(amiga_cfg_joy_button2 == AMIGA_CFG_CD32_SPECIAL_SELECT);
    assert(amiga_cfg_joy_button3 == AMIGA_CFG_CD32_NONE);

    /* 10. The deprecated joy3_button aliases are still honoured. */
    write_file(path,
        "JOY3_BUTTON1 = PAUSE\n"
        "joy3_button2 = cancel\n"
        "joy3_button3 = mega_bomb\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_joy_button1 == AMIGA_CFG_CD32_PAUSE);
    assert(amiga_cfg_joy_button2 == AMIGA_CFG_CD32_CANCEL);
    assert(amiga_cfg_joy_button3 == AMIGA_CFG_CD32_MEGA_BOMB);

    /* 11. A valid new joy_buttonN wins over the alias whatever the line
     * order; an invalid new value leaves a valid alias in charge. */
    write_file(path, "joy_button1 = PAUSE\njoy3_button1 = CANCEL\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_joy_button1 == AMIGA_CFG_CD32_PAUSE);
    write_file(path, "joy3_button1 = CANCEL\njoy_button1 = PAUSE\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_joy_button1 == AMIGA_CFG_CD32_PAUSE);
    write_file(path, "joy3_button1 = CANCEL\njoy_button1 = BOGUS\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_joy_button1 == AMIGA_CFG_CD32_CANCEL);
    write_file(path, "joy_button1 = BOGUS\njoy3_button1 = CANCEL\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_joy_button1 == AMIGA_CFG_CD32_CANCEL);

    /* 12. Invalid and empty values keep the defaults when nothing valid is
     * present; an invalid value never erases an earlier valid assignment, and
     * NONE counts as a full valid action. */
    write_file(path,
        "joy_button1 = BOGUS\njoy_button2 =\njoy_button3 = 9\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_joy_button1 == AMIGA_CFG_CD32_FIRE);
    assert(amiga_cfg_joy_button2 == AMIGA_CFG_CD32_SPECIAL_SELECT);
    assert(amiga_cfg_joy_button3 == AMIGA_CFG_CD32_MEGA_BOMB);
    write_file(path,
        "joy_button1 = NONE\njoy_button1 = BOGUS\njoy3_button1 = FIRE\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_joy_button1 == AMIGA_CFG_CD32_NONE);

    /* 13. The deprecated joy3 key is ignored: a leftover joy3 = OFF (or ON)
     * never disables the shared joystick handling. */
    write_file(path, "joy3 = OFF\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_joy_button1 == AMIGA_CFG_CD32_FIRE);
    assert(amiga_cfg_joy_button2 == AMIGA_CFG_CD32_SPECIAL_SELECT);
    assert(amiga_cfg_joy_button3 == AMIGA_CFG_CD32_MEGA_BOMB);
    write_file(path, "joy3 = ON\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_joy_button3 == AMIGA_CFG_CD32_MEGA_BOMB);

    /* 14. Options volume save rewrites volumes only: new and old joystick
     * assignments, comments, unknown keys, spacing and CRLF stay untouched. */
    write_file(path,
        "; own joystick setup\r\nmusic_adlib = 20\r\njoy_button1 = MEGA_BOMB\r\n"
        "joy3_button2 = NONE\r\njoy_button3 = FIRE ; custom\r\n"
        "unknown = keep\r\nmusic_mhi = 30\r\nmusic_wave = 40\r\nsfx_volume = 50\r\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_joy_button1 == AMIGA_CFG_CD32_MEGA_BOMB);
    assert(amiga_cfg_joy_button2 == AMIGA_CFG_CD32_NONE);
    assert(amiga_cfg_joy_button3 == AMIGA_CFG_CD32_FIRE);
    amiga_cfg_sfx = 66;                       /* in-game Options change */
    AmigaCfg_Save();
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_sfx == 66 && amiga_cfg_music_adlib == 20);
    assert(amiga_cfg_joy_button1 == AMIGA_CFG_CD32_MEGA_BOMB);
    assert(amiga_cfg_joy_button2 == AMIGA_CFG_CD32_NONE);
    assert(amiga_cfg_joy_button3 == AMIGA_CFG_CD32_FIRE);
    contents = read_file(path);
    assert(strstr(contents, "; own joystick setup\r\n") != 0);
    assert(strstr(contents, "joy_button1 = MEGA_BOMB\r\n") != 0);
    assert(strstr(contents, "joy3_button2 = NONE\r\n") != 0);
    assert(strstr(contents, "joy_button3 = FIRE ; custom\r\n") != 0);
    assert(strstr(contents, "unknown = keep\r\n") != 0);
    assert(strstr(contents, "sfx_volume = 66\r\n") != 0);
    assert(count_text(contents, "joy_button1 = ") == 1);
    assert(count_text(contents, "joy3_button2 = ") == 1);
    free(contents);

    /* 15. Legacy volumes-only cfg migrated once at load: comments, blank
     * lines, CRLF and the own values stay verbatim, missing volumes get the
     * defaults and the current JOYSTICK blocks are appended with the file's
     * own line endings; the original is kept as amiga.cfg.bak0. */
    remove_backups(path);
    write_file(path,
        "; my legacy volumes\r\n"
        "\r\n"
        "music_adlib = 5\r\n"
        "music_mhi = 6 ; kept\r\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_music_adlib == 5 && amiga_cfg_music_mhi == 6);
    assert(amiga_cfg_music_wave == 127 && amiga_cfg_sfx == 127);
    contents = read_file(path);
    assert(strstr(contents,
        "; my legacy volumes\r\n\r\nmusic_adlib = 5\r\nmusic_mhi = 6 ; kept\r\n") == contents);
    assert(count_text(contents, "music_wave = 127\r\n") == 1);
    assert(count_text(contents, "sfx_volume = 127\r\n") == 1);
    assert(count_text(contents, "; ==== JOYSTICK / 1-2-3 BUTTONS ====\r\n") == 1);
    assert(count_text(contents, "; ==== JOYSTICK / CD32 ====\r\n") == 1);
    assert(strstr(contents,
        "joy_button1 = FIRE ; Button 1 (DB9 pin 6, fire line)\r\n") != 0);
    assert(strstr(contents, "cd32 = OFF\r\n") != 0);
    assert(strstr(contents, "; ==== JOYSTICK / 1-2-3 BUTTONS ====\r\n") <
           strstr(contents, "; ==== JOYSTICK / CD32 ====\r\n"));
    free(contents);
    {
        char backup[640];
        snprintf(backup, sizeof(backup), "%s.bak0", path);
        contents = read_file(backup);
        assert(strcmp(contents,
            "; my legacy volumes\r\n\r\nmusic_adlib = 5\r\nmusic_mhi = 6 ; kept\r\n") == 0);
        free(contents);
    }

    /* 16. Re-loading the migrated file appends nothing: no duplicate blocks
     * and no duplicate keys. */
    reset_values();
    AmigaCfg_Load();
    contents = read_file(path);
    assert(count_text(contents, "; ==== JOYSTICK / 1-2-3 BUTTONS ====\r\n") == 1);
    assert(count_text(contents, "; ==== JOYSTICK / CD32 ====\r\n") == 1);
    assert(count_text(contents, "music_wave = ") == 1);
    assert(count_text(contents, "sfx_volume = ") == 1);
    assert(count_text(contents, "cd32 = OFF\r\n") == 1);
    free(contents);

    /* 17. Files with controller assignments or any other active key are never
     * migrated: the bytes stay untouched and the old joy3_button alias still
     * works. */
    write_file(path, "music_adlib = 5\ncd32 = ON\ncd32_red = CANCEL\nmusic_mhi = 6\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_cd32 == 1 && amiga_cfg_cd32_red == AMIGA_CFG_CD32_CANCEL);
    contents = read_file(path);
    assert(strcmp(contents,
        "music_adlib = 5\ncd32 = ON\ncd32_red = CANCEL\nmusic_mhi = 6\n") == 0);
    free(contents);
    write_file(path, "music_adlib = 5\ncustom = keep\n");
    reset_values();
    AmigaCfg_Load();
    contents = read_file(path);
    assert(strcmp(contents, "music_adlib = 5\ncustom = keep\n") == 0);
    free(contents);
    write_file(path, "music_adlib = 5\njoy3_button1 = CANCEL\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_joy_button1 == AMIGA_CFG_CD32_CANCEL);
    contents = read_file(path);
    assert(strcmp(contents, "music_adlib = 5\njoy3_button1 = CANCEL\n") == 0);
    free(contents);

    /* 18. A failed temp open or replacement during the migration leaves the
     * original intact; the read values stay in memory. */
    write_file(path, "music_adlib = 7\n");
    reset_values();
    AmigaCfg_TestFailNextTempOpen();
    AmigaCfg_Load();
    assert(amiga_cfg_music_adlib == 7);
    contents = read_file(path);
    assert(strcmp(contents, "music_adlib = 7\n") == 0);
    free(contents);
    write_file(path, "music_adlib = 8\n");
    reset_values();
    AmigaCfg_TestFailNextReplace();
    AmigaCfg_Load();
    assert(amiga_cfg_music_adlib == 8);
    contents = read_file(path);
    assert(strcmp(contents, "music_adlib = 8\n") == 0);
    free(contents);
    write_file(path, "music_adlib = 9\n");
    reset_values();
    AmigaCfg_TestFailReplacementAfterBackup();
    AmigaCfg_Load();
    assert(amiga_cfg_music_adlib == 9);
    contents = read_file(path);
    assert(strcmp(contents, "music_adlib = 9\n") == 0);
    free(contents);

    /* 19. The migration never overwrites an earlier backup copy: an existing
     * amiga.cfg.bak0 is left alone and the original goes to amiga.cfg.bak1. */
    {
        char backup0[640];
        char backup1[640];
        snprintf(backup0, sizeof(backup0), "%s.bak0", path);
        snprintf(backup1, sizeof(backup1), "%s.bak1", path);
        remove_backups(path);
        write_file(path, "music_adlib = 21\n");
        write_file(backup0, "older backup\n");
        reset_values();
        AmigaCfg_Load();
        contents = read_file(path);
        assert(strstr(contents, "music_adlib = 21\n") != 0);
        assert(strstr(contents, "; ==== JOYSTICK / CD32 ====\n") != 0);
        free(contents);
        contents = read_file(backup0);
        assert(strcmp(contents, "older backup\n") == 0);
        free(contents);
        contents = read_file(backup1);
        assert(strcmp(contents, "music_adlib = 21\n") == 0);
        free(contents);
    }

    /* 20. A file created from scratch carries the JOYSTICK / 1-2-3 BUTTONS
     * block above the JOYSTICK / CD32 block, with the built-in defaults and
     * no deprecated joy3 keys. */
    unlink(path);
    reset_values();
    AmigaCfg_Load();                          /* missing file -> template */
    assert(amiga_cfg_joy_button1 == AMIGA_CFG_CD32_FIRE);
    assert(amiga_cfg_joy_button2 == AMIGA_CFG_CD32_SPECIAL_SELECT);
    assert(amiga_cfg_joy_button3 == AMIGA_CFG_CD32_MEGA_BOMB);
    contents = read_file(path);
    assert(strstr(contents, "; ==== AUDIO ====\n") != 0);
    assert(strstr(contents, "; ==== JOYSTICK / 1-2-3 BUTTONS ====\n") != 0);
    assert(strstr(contents, "; ==== JOYSTICK / CD32 ====\n") != 0);
    assert(strstr(contents, "; ==== AUDIO ====\n") <
           strstr(contents, "; ==== JOYSTICK / 1-2-3 BUTTONS ====\n"));
    assert(strstr(contents, "; ==== JOYSTICK / 1-2-3 BUTTONS ====\n") <
           strstr(contents, "; ==== JOYSTICK / CD32 ====\n"));
    assert(strstr(contents,
        "joy_button1 = FIRE ; Button 1 (DB9 pin 6, fire line)\n"
        "joy_button2 = SPECIAL_SELECT ; Button 2 (DB9 pin 9)\n"
        "joy_button3 = MEGA_BOMB ; Button 3 (DB9 pin 5)\n") != 0);
    assert(strstr(contents, "joy3 = ") == 0);
    assert(strstr(contents, "joy3_button") == 0);
    free(contents);

    /* 21. Files that must never be migrated: comments/blank lines only, a
     * lone deprecated joy3 = OFF, and a volumes file that also carries a
     * deprecated joy3_button1 alias. In every case the bytes must stay
     * identical after Load and no migration backup may appear. */
    remove_backups(path);
    write_file(path, "; only a comment\n\n; and a second one\r\n\r\n");
    load_and_assert_untouched(path);

    remove_backups(path);
    write_file(path, "joy3 = OFF\n");
    load_and_assert_untouched(path);
    assert(amiga_cfg_joy_button1 == AMIGA_CFG_CD32_FIRE);
    assert(amiga_cfg_joy_button2 == AMIGA_CFG_CD32_SPECIAL_SELECT);
    assert(amiga_cfg_joy_button3 == AMIGA_CFG_CD32_MEGA_BOMB);

    remove_backups(path);
    write_file(path, "music_adlib = 5\njoy3_button1 = CANCEL\n");
    load_and_assert_untouched(path);
    assert(amiga_cfg_music_adlib == 5);
    assert(amiga_cfg_joy_button1 == AMIGA_CFG_CD32_CANCEL);

    /* Remove this run's own files (amiga.cfg, its .bakN copies and any
     * leftover .tmpN scratch file) before removing the run directory. */
    unlink(path);
    remove_backups(path);
    {
        char tmp[640];
        int number;

        for (number = 0; number < 100; number++)
        {
            snprintf(tmp, sizeof(tmp), "%s.tmp%d", path, number);
            remove(tmp);
        }
    }
    assert(rmdir(directory) == 0);

    puts("amiga_cfg_test: PASS");
    return 0;
}