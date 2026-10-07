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

int
main(void)
{
    char directory[] = "/tmp/raptor-amiga-cfg-XXXXXX";
    char path[512];
    char *contents;

    assert(mkdtemp(directory) != 0);
    snprintf(path, sizeof(path), "%s/amiga.cfg", directory);
    AmigaCfg_TestSetPath(path);

    /* 1. Old configuration containing only audio volumes. */
    write_file(path, "music_adlib=1\nmusic_mhi=2\nmusic_wave=3\nsfx_volume=4\n");
    reset_values();
    AmigaCfg_Load();
    assert(amiga_cfg_music_adlib == 1 && amiga_cfg_music_mhi == 2);
    assert(amiga_cfg_music_wave == 3 && amiga_cfg_sfx == 4);
    amiga_cfg_music_adlib = 11;
    AmigaCfg_Save();
    contents = read_file(path);
    assert(strcmp(contents, "music_adlib=11\nmusic_mhi=2\nmusic_wave=3\nsfx_volume=4\n") == 0);
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

    /* 6. Repeated saves do not append duplicate missing keys or CD32 blocks. */
    write_file(path, "music_adlib = 1\n");
    reset_values();
    AmigaCfg_Load();
    AmigaCfg_Save();
    AmigaCfg_Save();
    contents = read_file(path);
    assert(count_text(contents, "music_mhi = ") == 1);
    assert(count_text(contents, "music_wave = ") == 1);
    assert(count_text(contents, "sfx_volume = ") == 1);
    assert(strstr(contents, "cd32 = ") == 0);
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

    unlink(path);
    rmdir(directory);
    puts("amiga_cfg_test: PASS");
    return 0;
}