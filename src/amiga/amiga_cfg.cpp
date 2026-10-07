/***************************************************************************
 * amiga_cfg.cpp - persistent settings for the Amiga port.
 *
 * Existing amiga.cfg files are updated without rebuilding their contents.
 ***************************************************************************/

#ifdef __AMIGA__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#ifdef AMIGA_CFG_TEST
#include <errno.h>
#else
#include <dos/dos.h>
#include <proto/dos.h>
#endif

#include "amiga/amiga_cfg.h"

int amiga_cfg_music_adlib = 127;
int amiga_cfg_music_mhi   = 127;
int amiga_cfg_music_wave  = 127;
int amiga_cfg_sfx         = 127;

int amiga_cfg_cd32 = 0;
int amiga_cfg_cd32_red     = AMIGA_CFG_CD32_FIRE;
int amiga_cfg_cd32_blue    = AMIGA_CFG_CD32_SPECIAL_SELECT;
int amiga_cfg_cd32_green   = AMIGA_CFG_CD32_MEGA_BOMB;
int amiga_cfg_cd32_yellow  = AMIGA_CFG_CD32_NONE;
int amiga_cfg_cd32_reverse = AMIGA_CFG_CD32_NONE;
int amiga_cfg_cd32_forward = AMIGA_CFG_CD32_NONE;
int amiga_cfg_cd32_play    = AMIGA_CFG_CD32_PAUSE;

static const char *const cd32_action_names[AMIGA_CFG_CD32_NUM_ACTIONS] = {
    "NONE", "FIRE", "SPECIAL_SELECT", "MEGA_BOMB", "PAUSE", "CANCEL"
};

enum cfg_key {
    CFG_NONE, CFG_ADLIB, CFG_MHI, CFG_WAVE, CFG_SFX, CFG_CD32,
    CFG_RED, CFG_BLUE, CFG_GREEN, CFG_YELLOW, CFG_REVERSE, CFG_FORWARD,
    CFG_PLAY
};

static int cfg_loaded = 0;

#ifdef AMIGA_CFG_TEST
static const char *cfg_test_path = "amiga.cfg";
static int cfg_test_fail_temp_open = 0;
static int cfg_test_fail_rename_number = 0;
#endif

static const char *
cfg_filename(void)
{
#ifdef AMIGA_CFG_TEST
    return cfg_test_path;
#else
    return "amiga.cfg";
#endif
}

static int
cfg_space(char c)
{
    return c == ' ' || c == '\t';
}

static int
cfg_key_equal(const char *text, size_t length, const char *key)
{
    size_t i;

    if (strlen(key) != length)
        return 0;
    for (i = 0; i < length; i++)
    {
        char a = text[i];
        char b = key[i];

        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
        if (a != b) return 0;
    }
    return 1;
}

/* value_start/value_end delimit only the value token.  Everything outside
 * that range, including spacing, comments and EOL bytes, is retained. */
static enum cfg_key
cfg_parse_line(const char *line, size_t length, size_t *value_start,
               size_t *value_end)
{
    size_t p = 0;
    size_t key_start;
    size_t key_end;

    *value_start = 0;
    *value_end = 0;
    while (p < length && cfg_space(line[p])) p++;
    if (p == length || line[p] == ';' || line[p] == '#') return CFG_NONE;
    key_start = p;
    while (p < length && ((line[p] >= 'A' && line[p] <= 'Z') ||
                          (line[p] >= 'a' && line[p] <= 'z') ||
                          (line[p] >= '0' && line[p] <= '9') ||
                          line[p] == '_')) p++;
    key_end = p;
    while (p < length && cfg_space(line[p])) p++;
    if (key_start == key_end || p == length || line[p] != '=') return CFG_NONE;
    p++;
    while (p < length && cfg_space(line[p])) p++;
    *value_start = p;
    while (p < length && !cfg_space(line[p]) && line[p] != '\r' &&
           line[p] != '\n' && line[p] != ';' && line[p] != '#') p++;
    *value_end = p;

    if (cfg_key_equal(line + key_start, key_end - key_start, "music_adlib")) return CFG_ADLIB;
    if (cfg_key_equal(line + key_start, key_end - key_start, "music_mhi")) return CFG_MHI;
    if (cfg_key_equal(line + key_start, key_end - key_start, "music_wave")) return CFG_WAVE;
    if (cfg_key_equal(line + key_start, key_end - key_start, "sfx_volume")) return CFG_SFX;
    if (cfg_key_equal(line + key_start, key_end - key_start, "cd32")) return CFG_CD32;
    if (cfg_key_equal(line + key_start, key_end - key_start, "cd32_red")) return CFG_RED;
    if (cfg_key_equal(line + key_start, key_end - key_start, "cd32_blue")) return CFG_BLUE;
    if (cfg_key_equal(line + key_start, key_end - key_start, "cd32_green")) return CFG_GREEN;
    if (cfg_key_equal(line + key_start, key_end - key_start, "cd32_yellow")) return CFG_YELLOW;
    if (cfg_key_equal(line + key_start, key_end - key_start, "cd32_reverse")) return CFG_REVERSE;
    if (cfg_key_equal(line + key_start, key_end - key_start, "cd32_forward")) return CFG_FORWARD;
    if (cfg_key_equal(line + key_start, key_end - key_start, "cd32_play")) return CFG_PLAY;
    return CFG_NONE;
}

static const char *
cd32_action_name(int action)
{
    if (action < 0 || action >= AMIGA_CFG_CD32_NUM_ACTIONS)
        action = AMIGA_CFG_CD32_NONE;
    return cd32_action_names[action];
}

static int
cd32_parse_action(const char *word, int current)
{
    int i;
    for (i = 0; i < AMIGA_CFG_CD32_NUM_ACTIONS; i++)
        if (strcasecmp(word, cd32_action_names[i]) == 0) return i;
    return current;
}

/* Returns 1 for a complete read, -1 for a missing file, 0 for any error. */
static int
cfg_read_file(char **contents, size_t *length)
{
    FILE *f;
    long file_length;
    char *buffer;
    int close_result;

    f = fopen(cfg_filename(), "rb");
    if (!f)
    {
#ifdef AMIGA_CFG_TEST
        return errno == ENOENT ? -1 : 0;
#else
        return IoErr() == ERROR_OBJECT_NOT_FOUND ? -1 : 0;
#endif
    }
    if (fseek(f, 0, SEEK_END) != 0 || (file_length = ftell(f)) < 0 ||
        fseek(f, 0, SEEK_SET) != 0)
    {
        fclose(f);
        return 0;
    }
    buffer = (char *)malloc((size_t)file_length + 1);
    if (!buffer)
    {
        fclose(f);
        return 0;
    }
    if (file_length && fread(buffer, 1, (size_t)file_length, f) != (size_t)file_length)
    {
        fclose(f);
        free(buffer);
        return 0;
    }
    close_result = fclose(f);
    if (close_result != 0)
    {
        free(buffer);
        return 0;
    }
    buffer[file_length] = 0;
    *contents = buffer;
    *length = (size_t)file_length;
    return 1;
}

static int
cfg_path_exists(const char *path)
{
#ifdef AMIGA_CFG_TEST
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
#else
    BPTR lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock) return 0;
    UnLock(lock);
    return 1;
#endif
}

static int
cfg_rename(const char *from, const char *to)
{
#ifdef AMIGA_CFG_TEST
    if (cfg_test_fail_rename_number)
    {
        cfg_test_fail_rename_number--;
        if (!cfg_test_fail_rename_number)
            return 0;
    }
    return rename(from, to) == 0;
#else
    return Rename((STRPTR)from, (STRPTR)to) != 0;
#endif
}

static void
cfg_delete(const char *path)
{
#ifdef AMIGA_CFG_TEST
    remove(path);
#else
    DeleteFile((STRPTR)path);
#endif
}

static int
cfg_open_temp(char *temp_name, size_t name_size, FILE **result)
{
    int number;
    for (number = 0; number < 100; number++)
    {
        snprintf(temp_name, name_size, "%s.tmp%d", cfg_filename(), number);
        if (cfg_path_exists(temp_name)) continue;
#ifdef AMIGA_CFG_TEST
        if (cfg_test_fail_temp_open)
        {
            cfg_test_fail_temp_open = 0;
            return 0;
        }
#endif
        *result = fopen(temp_name, "wb");
        return *result != 0;
    }
    return 0;
}

/* AmigaDOS Rename does not rely on POSIX replacement semantics.  Retain the
 * original as a temporary backup until the new name is known to be in place. */
static int
cfg_replace(const char *temp_name, int old_file_exists)
{
    char backup_name[320];
    int number;

    if (!old_file_exists)
    {
        if (cfg_rename(temp_name, cfg_filename())) return 1;
        cfg_delete(temp_name);
        return 0;
    }
    for (number = 0; number < 100; number++)
    {
        snprintf(backup_name, sizeof(backup_name), "%s.bak%d", cfg_filename(), number);
        if (!cfg_path_exists(backup_name)) break;
    }
    if (number == 100 || !cfg_rename(cfg_filename(), backup_name))
    {
        cfg_delete(temp_name);
        return 0;
    }
    if (!cfg_rename(temp_name, cfg_filename()))
    {
        /* Do not delete backup_name when recovery fails: it is the original. */
        if (cfg_rename(backup_name, cfg_filename())) cfg_delete(temp_name);
        return 0;
    }
    cfg_delete(backup_name);
    return 1;
}

static int
cfg_volume_value(enum cfg_key key)
{
    if (key == CFG_ADLIB) return amiga_cfg_music_adlib;
    if (key == CFG_MHI) return amiga_cfg_music_mhi;
    if (key == CFG_WAVE) return amiga_cfg_music_wave;
    return amiga_cfg_sfx;
}

static void
cfg_clamp_volumes(void)
{
    int *volumes[] = { &amiga_cfg_music_adlib, &amiga_cfg_music_mhi,
                       &amiga_cfg_music_wave, &amiga_cfg_sfx };
    unsigned int i;
    for (i = 0; i < sizeof(volumes) / sizeof(volumes[0]); i++)
    {
        if (*volumes[i] < 0) *volumes[i] = 0;
        else if (*volumes[i] > 127) *volumes[i] = 127;
    }
}

static int
cfg_write_default(FILE *f)
{
    return fprintf(f,
        "; ==== AUDIO ====\n"
        "; Raptor Amiga volumes (0..127, 127 = loud)\n"
        "music_adlib = %d\n"
        "music_mhi   = %d\n"
        "music_wave  = %d\n"
        "sfx_volume  = %d\n\n"
        "; ==== JOYSTICK / CD32 ====\n"
        "; cd32 = OFF : classic joystick on port 1 (default, unchanged behaviour)\n"
        "; cd32 = ON  : CD32 pad on port 1 (lowlevel.library game controller mode)\n"
        "; NOJOY / JOYSTICK=OFF takes precedence over this block.\n"
        "; Button assignments, one value per physical pad button:\n"
        ";   FIRE           - main guns\n"
        ";   SPECIAL_SELECT - fire the selected special weapon\n"
        ";   MEGA_BOMB      - launch a mega bomb\n"
        ";   PAUSE          - pause the game\n"
        ";   CANCEL         - cancel / back out\n"
        ";   NONE           - button disabled\n"
        "cd32 = %s\ncd32_red     = %s ; Red button\n"
        "cd32_blue    = %s ; Blue button\ncd32_green   = %s ; Green button\n"
        "cd32_yellow  = %s ; Yellow button\n"
        "cd32_reverse = %s ; Reverse button (left shoulder)\n"
        "cd32_forward = %s ; Forward button (right shoulder)\n"
        "cd32_play    = %s ; Play button (triangle)\n",
        amiga_cfg_music_adlib, amiga_cfg_music_mhi, amiga_cfg_music_wave,
        amiga_cfg_sfx, amiga_cfg_cd32 ? "ON" : "OFF",
        cd32_action_name(amiga_cfg_cd32_red), cd32_action_name(amiga_cfg_cd32_blue),
        cd32_action_name(amiga_cfg_cd32_green), cd32_action_name(amiga_cfg_cd32_yellow),
        cd32_action_name(amiga_cfg_cd32_reverse), cd32_action_name(amiga_cfg_cd32_forward),
        cd32_action_name(amiga_cfg_cd32_play)) >= 0;
}

void
AmigaCfg_Load(void)
{
    char *contents;
    size_t length;
    size_t offset;
    int read_result;

    if (cfg_loaded) return;
    cfg_loaded = 1;
    read_result = cfg_read_file(&contents, &length);
    if (read_result < 0)
    {
        AmigaCfg_Save();
        return;
    }
    if (read_result == 0) return;

    for (offset = 0; offset < length; )
    {
        size_t line_end = offset;
        size_t value_start;
        size_t value_end;
        size_t word_length;
        char word[32];
        enum cfg_key key;

        while (line_end < length && contents[line_end] != '\n') line_end++;
        key = cfg_parse_line(contents + offset, line_end - offset, &value_start, &value_end);
        word_length = value_end - value_start;
        if (word_length >= sizeof(word)) word_length = sizeof(word) - 1;
        memcpy(word, contents + offset + value_start, word_length);
        word[word_length] = 0;
        if (key >= CFG_ADLIB && key <= CFG_SFX)
        {
            char *end;
            long value = strtol(word, &end, 10);
            if (end != word)
            {
                if (value < 0) value = 0;
                if (value > 127) value = 127;
                if (key == CFG_ADLIB) amiga_cfg_music_adlib = (int)value;
                else if (key == CFG_MHI) amiga_cfg_music_mhi = (int)value;
                else if (key == CFG_WAVE) amiga_cfg_music_wave = (int)value;
                else amiga_cfg_sfx = (int)value;
            }
        }
        else if (key == CFG_CD32)
        {
            if (strcasecmp(word, "ON") == 0) amiga_cfg_cd32 = 1;
            else if (strcasecmp(word, "OFF") == 0) amiga_cfg_cd32 = 0;
        }
        else if (key == CFG_RED) amiga_cfg_cd32_red = cd32_parse_action(word, amiga_cfg_cd32_red);
        else if (key == CFG_BLUE) amiga_cfg_cd32_blue = cd32_parse_action(word, amiga_cfg_cd32_blue);
        else if (key == CFG_GREEN) amiga_cfg_cd32_green = cd32_parse_action(word, amiga_cfg_cd32_green);
        else if (key == CFG_YELLOW) amiga_cfg_cd32_yellow = cd32_parse_action(word, amiga_cfg_cd32_yellow);
        else if (key == CFG_REVERSE) amiga_cfg_cd32_reverse = cd32_parse_action(word, amiga_cfg_cd32_reverse);
        else if (key == CFG_FORWARD) amiga_cfg_cd32_forward = cd32_parse_action(word, amiga_cfg_cd32_forward);
        else if (key == CFG_PLAY) amiga_cfg_cd32_play = cd32_parse_action(word, amiga_cfg_cd32_play);
        offset = line_end < length ? line_end + 1 : length;
    }
    free(contents);
    cfg_clamp_volumes();
}

void
AmigaCfg_Save(void)
{
    char *contents = 0;
    size_t length = 0;
    char temp_name[320];
    FILE *f;
    size_t offset;
    int read_result;
    int old_file_exists;
    int saw_adlib = 0, saw_mhi = 0, saw_wave = 0, saw_sfx = 0;
    const char *append_eol = "\n";
    int write_ok = 1;

    cfg_clamp_volumes();
    read_result = cfg_read_file(&contents, &length);
    if (read_result == 0) return;
    old_file_exists = read_result > 0;
    if (!cfg_open_temp(temp_name, sizeof(temp_name), &f))
    {
        if (contents) free(contents);
        return;
    }
    if (!old_file_exists)
        write_ok = cfg_write_default(f);
    else
    {
        for (offset = 0; offset < length && write_ok; )
        {
            size_t line_end = offset;
            size_t full_end;
            size_t value_start;
            size_t value_end;
            enum cfg_key key;

            while (line_end < length && contents[line_end] != '\n') line_end++;
            full_end = line_end < length ? line_end + 1 : line_end;
            key = cfg_parse_line(contents + offset, line_end - offset, &value_start, &value_end);
            if (key >= CFG_ADLIB && key <= CFG_SFX)
            {
                if (fwrite(contents + offset, 1, value_start, f) != value_start ||
                    fprintf(f, "%d", cfg_volume_value(key)) < 0 ||
                    fwrite(contents + offset + value_end, 1, full_end - offset - value_end, f) !=
                    full_end - offset - value_end) write_ok = 0;
                if (key == CFG_ADLIB) saw_adlib = 1;
                else if (key == CFG_MHI) saw_mhi = 1;
                else if (key == CFG_WAVE) saw_wave = 1;
                else saw_sfx = 1;
            }
            else if (fwrite(contents + offset, 1, full_end - offset, f) != full_end - offset)
                write_ok = 0;
            offset = full_end;
        }
        if (write_ok && (!saw_adlib || !saw_mhi || !saw_wave || !saw_sfx))
        {
            if (strstr(contents, "\r\n")) append_eol = "\r\n";
            if (length && contents[length - 1] != '\n' && fputs(append_eol, f) == EOF) write_ok = 0;
            if (!saw_adlib && fprintf(f, "music_adlib = %d%s", amiga_cfg_music_adlib, append_eol) < 0) write_ok = 0;
            if (!saw_mhi && fprintf(f, "music_mhi = %d%s", amiga_cfg_music_mhi, append_eol) < 0) write_ok = 0;
            if (!saw_wave && fprintf(f, "music_wave = %d%s", amiga_cfg_music_wave, append_eol) < 0) write_ok = 0;
            if (!saw_sfx && fprintf(f, "sfx_volume = %d%s", amiga_cfg_sfx, append_eol) < 0) write_ok = 0;
        }
    }
    if (contents) free(contents);
    if (ferror(f)) write_ok = 0;
    if (fclose(f) != 0) write_ok = 0;
    if (!write_ok)
    {
        cfg_delete(temp_name);
        return;
    }
    cfg_replace(temp_name, old_file_exists);
}

#ifdef AMIGA_CFG_TEST
void AmigaCfg_TestSetPath(const char *path) { cfg_test_path = path; }
void AmigaCfg_TestResetLoad(void) { cfg_loaded = 0; }
void AmigaCfg_TestFailNextTempOpen(void) { cfg_test_fail_temp_open = 1; }
void AmigaCfg_TestFailNextReplace(void) { cfg_test_fail_rename_number = 1; }
void AmigaCfg_TestFailReplacementAfterBackup(void) { cfg_test_fail_rename_number = 2; }
#endif

#endif /* __AMIGA__ */