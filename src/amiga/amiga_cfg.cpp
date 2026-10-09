/***************************************************************************
 * amiga_cfg.cpp - persistent settings for the Amiga port.
 *
 * Existing amiga.cfg files are updated without rebuilding their contents.
 * The single exception is a legacy file that holds nothing but audio volume
 * keys: it is migrated once at load time (see cfg_migrate) so that a later
 * save finds every key and never duplicates a block.
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

/* Shared Amiga 1/2/3-button joystick (DB9 pins 6/9/5). Active whenever the
 * CD32 pad is off (see Amiga_ConfigureJoyPort); there is no enable key. */
int amiga_cfg_joy_button1 = AMIGA_CFG_CD32_FIRE;
int amiga_cfg_joy_button2 = AMIGA_CFG_CD32_SPECIAL_SELECT;
int amiga_cfg_joy_button3 = AMIGA_CFG_CD32_MEGA_BOMB;

/* Shared by the joystick and CD32 blocks. */
static const char *const cd32_action_names[AMIGA_CFG_CD32_NUM_ACTIONS] = {
    "NONE", "FIRE", "SPECIAL_SELECT", "MEGA_BOMB", "PAUSE", "CANCEL"
};

/* CFG_JOY3_B* are the deprecated joy3_button* aliases; a bare "joy3" key is
 * deliberately not recognized (deprecated, ignored as an unknown key). */
enum cfg_key {
    CFG_NONE, CFG_ADLIB, CFG_MHI, CFG_WAVE, CFG_SFX, CFG_CD32,
    CFG_RED, CFG_BLUE, CFG_GREEN, CFG_YELLOW, CFG_REVERSE, CFG_FORWARD,
    CFG_PLAY, CFG_JOY_B1, CFG_JOY_B2, CFG_JOY_B3,
    CFG_JOY3_B1, CFG_JOY3_B2, CFG_JOY3_B3
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
    if (cfg_key_equal(line + key_start, key_end - key_start, "joy_button1")) return CFG_JOY_B1;
    if (cfg_key_equal(line + key_start, key_end - key_start, "joy_button2")) return CFG_JOY_B2;
    if (cfg_key_equal(line + key_start, key_end - key_start, "joy_button3")) return CFG_JOY_B3;
    /* Deprecated aliases: still accepted so existing files keep their
     * assignments. */
    if (cfg_key_equal(line + key_start, key_end - key_start, "joy3_button1")) return CFG_JOY3_B1;
    if (cfg_key_equal(line + key_start, key_end - key_start, "joy3_button2")) return CFG_JOY3_B2;
    if (cfg_key_equal(line + key_start, key_end - key_start, "joy3_button3")) return CFG_JOY3_B3;
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

/* Strict parse: returns 1 and stores the action id when word names a known
 * action (NONE included), 0 otherwise. Used by the joystick keys, where an
 * invalid or empty value must not overwrite an already recognized
 * assignment. */
static int
cd32_parse_action_strict(const char *word, int *action)
{
    int i;
    for (i = 0; i < AMIGA_CFG_CD32_NUM_ACTIONS; i++)
        if (strcasecmp(word, cd32_action_names[i]) == 0)
        {
            *action = i;
            return 1;
        }
    return 0;
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
 * original as a temporary backup until the new name is known to be in place.
 * keep_backup = 0 removes that backup once the replacement succeeded (a normal
 * volume save); keep_backup = 1 keeps it, so the pre-migration file survives
 * as the first free amiga.cfg.bakN without overwriting an earlier copy. */
static int
cfg_replace(const char *temp_name, int old_file_exists, int keep_backup)
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
    if (!keep_backup) cfg_delete(backup_name);
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
cfg_write_audio(FILE *f)
{
    return fprintf(f,
        "; ==== AUDIO ====\n"
        "; Raptor Amiga volumes (0..127, 127 = loud)\n"
        "music_adlib = %d\n"
        "music_mhi   = %d\n"
        "music_wave  = %d\n"
        "sfx_volume  = %d\n",
        amiga_cfg_music_adlib, amiga_cfg_music_mhi, amiga_cfg_music_wave,
        amiga_cfg_sfx) >= 0;
}

/* Leading blank line, then the JOYSTICK / 1-2-3 BUTTONS block and the
 * JOYSTICK / CD32 block, in that order.  Shared by the new-file template
 * (eol = "\n") and the legacy volumes-only migration (eol taken from the
 * migrated file), so both always write the same values and comments. */
static int
cfg_write_joystick(FILE *f, const char *eol)
{
    return fprintf(f,
        "%s"
        "; ==== JOYSTICK / 1-2-3 BUTTONS ====%s"
        "; Amiga DB9 joystick on the second physical port.%s"
        "; Active when cd32 = OFF. No separate enable setting is required.%s"
        "; Buttons: 1 = pin 6, 2 = pin 9, 3 = pin 5.%s"
        "; Action names (same set as the CD32 block below):%s"
        ";   FIRE           - primary guns + selected secondary weapon%s"
        ";   SPECIAL_SELECT - change (cycle) the secondary weapon%s"
        ";   MEGA_BOMB      - launch a mega bomb%s"
        ";   PAUSE          - pause the game%s"
        ";   CANCEL         - cancel / back out%s"
        ";   NONE           - button disabled%s"
        "joy_button1 = %s ; Button 1 (DB9 pin 6, fire line)%s"
        "joy_button2 = %s ; Button 2 (DB9 pin 9)%s"
        "joy_button3 = %s ; Button 3 (DB9 pin 5)%s"
        "%s"
        "; ==== JOYSTICK / CD32 ====%s"
        "; cd32 = OFF : shared joystick handling on port 1 (default)%s"
        "; cd32 = ON  : CD32 pad on port 1 (lowlevel.library game controller mode)%s"
        "; NOJOY / JOYSTICK=OFF takes precedence over this block.%s"
        "; Button assignments, one value per physical pad button:%s"
        ";   FIRE           - primary guns + selected secondary weapon%s"
        ";   SPECIAL_SELECT - change (cycle) the secondary weapon%s"
        ";   MEGA_BOMB      - launch a mega bomb%s"
        ";   PAUSE          - pause the game%s"
        ";   CANCEL         - cancel / back out%s"
        ";   NONE           - button disabled%s"
        "cd32 = %s%scd32_red     = %s ; Red button%s"
        "cd32_blue    = %s ; Blue button%s"
        "cd32_green   = %s ; Green button%s"
        "cd32_yellow  = %s ; Yellow button%s"
        "cd32_reverse = %s ; Reverse button (left shoulder)%s"
        "cd32_forward = %s ; Forward button (right shoulder)%s"
        "cd32_play    = %s ; Play button (triangle)%s",
        eol,
        eol, eol, eol, eol, eol, eol, eol, eol, eol, eol, eol,
        cd32_action_name(amiga_cfg_joy_button1), eol,
        cd32_action_name(amiga_cfg_joy_button2), eol,
        cd32_action_name(amiga_cfg_joy_button3), eol,
        eol,
        eol, eol, eol, eol, eol, eol, eol, eol, eol, eol, eol,
        amiga_cfg_cd32 ? "ON" : "OFF", eol,
        cd32_action_name(amiga_cfg_cd32_red), eol,
        cd32_action_name(amiga_cfg_cd32_blue), eol,
        cd32_action_name(amiga_cfg_cd32_green), eol,
        cd32_action_name(amiga_cfg_cd32_yellow), eol,
        cd32_action_name(amiga_cfg_cd32_reverse), eol,
        cd32_action_name(amiga_cfg_cd32_forward), eol,
        cd32_action_name(amiga_cfg_cd32_play), eol) >= 0;
}

/* Full template written when amiga.cfg does not exist yet. */
static int
cfg_write_default(FILE *f)
{
    if (!cfg_write_audio(f)) return 0;
    return cfg_write_joystick(f, "\n");
}

/* One-time migration of a legacy amiga.cfg that holds nothing but audio
 * volume keys (comments and blank lines allowed).  The original bytes are
 * kept verbatim, missing volume keys are appended with the built-in defaults
 * and the current JOYSTICK / 1-2-3 BUTTONS and JOYSTICK / CD32 blocks are
 * appended exactly as the full template writes them, so a later save finds
 * every key and never duplicates a block.  The original is kept as the first
 * free amiga.cfg.bakN copy.  Returns 1 on success; 0 on any failure, leaving
 * the original file untouched for the caller, which keeps the values it has
 * already read. */
static int
cfg_migrate(const char *contents, size_t length,
            int saw_adlib, int saw_mhi, int saw_wave, int saw_sfx)
{
    char temp_name[320];
    const char *eol = "\n";
    FILE *f;
    int write_ok = 1;

    if (!cfg_open_temp(temp_name, sizeof(temp_name), &f))
        return 0;
    if (strstr(contents, "\r\n")) eol = "\r\n";
    if (length && fwrite(contents, 1, length, f) != length) write_ok = 0;
    if (write_ok && length && contents[length - 1] != '\n' &&
        fputs(eol, f) == EOF) write_ok = 0;
    if (write_ok && !saw_adlib &&
        fprintf(f, "music_adlib = %d%s", amiga_cfg_music_adlib, eol) < 0) write_ok = 0;
    if (write_ok && !saw_mhi &&
        fprintf(f, "music_mhi = %d%s", amiga_cfg_music_mhi, eol) < 0) write_ok = 0;
    if (write_ok && !saw_wave &&
        fprintf(f, "music_wave = %d%s", amiga_cfg_music_wave, eol) < 0) write_ok = 0;
    if (write_ok && !saw_sfx &&
        fprintf(f, "sfx_volume = %d%s", amiga_cfg_sfx, eol) < 0) write_ok = 0;
    if (write_ok && !cfg_write_joystick(f, eol)) write_ok = 0;
    if (ferror(f)) write_ok = 0;
    if (fclose(f) != 0) write_ok = 0;
    if (!write_ok)
    {
        cfg_delete(temp_name);
        return 0;
    }
    return cfg_replace(temp_name, 1, 1);
}

void
AmigaCfg_Load(void)
{
    char *contents;
    size_t length;
    size_t offset;
    int read_result;
    /* Legacy-file detection for the one-time migration: saw_* record the four
     * volume keys, other_content marks any active line that is not a volume
     * key (a recognised controller key or an unknown key).  A file with at
     * least one volume key and no other_content is a volumes-only file. */
    int saw_adlib = 0, saw_mhi = 0, saw_wave = 0, saw_sfx = 0;
    int other_content = 0;
    /* Joystick assignments collected across the file: -1 = nothing valid
     * seen yet. joy_new* holds valid joy_buttonN values, joy_alias* the
     * deprecated joy3_buttonN aliases. A valid new key wins regardless of
     * line order; an alias only applies when no valid new key was seen. */
    int joy_new1 = -1, joy_new2 = -1, joy_new3 = -1;
    int joy_alias1 = -1, joy_alias2 = -1, joy_alias3 = -1;

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
        /* A line that is neither blank nor a comment but does not name one of
         * the four volume keys disqualifies the file from the migration.  A
         * CRLF blank line arrives here as a lone '\r', so treat it as blank. */
        {
            size_t probe = offset;
            while (probe < line_end &&
                   (cfg_space(contents[probe]) || contents[probe] == '\r')) probe++;
            if (probe < line_end && contents[probe] != ';' && contents[probe] != '#' &&
                !(key >= CFG_ADLIB && key <= CFG_SFX))
                other_content = 1;
        }
        word_length = value_end - value_start;
        if (word_length >= sizeof(word)) word_length = sizeof(word) - 1;
        memcpy(word, contents + offset + value_start, word_length);
        word[word_length] = 0;
        if (key >= CFG_ADLIB && key <= CFG_SFX)
        {
            char *end;
            long value = strtol(word, &end, 10);
            if (key == CFG_ADLIB) saw_adlib = 1;
            else if (key == CFG_MHI) saw_mhi = 1;
            else if (key == CFG_WAVE) saw_wave = 1;
            else saw_sfx = 1;
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
        /* Joystick keys: only a recognized action counts, so an invalid or
         * empty value never overwrites an earlier valid assignment. */
        else if (key == CFG_JOY_B1) { int a; if (cd32_parse_action_strict(word, &a)) joy_new1 = a; }
        else if (key == CFG_JOY_B2) { int a; if (cd32_parse_action_strict(word, &a)) joy_new2 = a; }
        else if (key == CFG_JOY_B3) { int a; if (cd32_parse_action_strict(word, &a)) joy_new3 = a; }
        else if (key == CFG_JOY3_B1) { int a; if (cd32_parse_action_strict(word, &a)) joy_alias1 = a; }
        else if (key == CFG_JOY3_B2) { int a; if (cd32_parse_action_strict(word, &a)) joy_alias2 = a; }
        else if (key == CFG_JOY3_B3) { int a; if (cd32_parse_action_strict(word, &a)) joy_alias3 = a; }
        offset = line_end < length ? line_end + 1 : length;
    }
    /* Defaults stay in place only when neither a valid new key nor a valid
     * alias was found for the button. */
    if (joy_new1 >= 0) amiga_cfg_joy_button1 = joy_new1;
    else if (joy_alias1 >= 0) amiga_cfg_joy_button1 = joy_alias1;
    if (joy_new2 >= 0) amiga_cfg_joy_button2 = joy_new2;
    else if (joy_alias2 >= 0) amiga_cfg_joy_button2 = joy_alias2;
    if (joy_new3 >= 0) amiga_cfg_joy_button3 = joy_new3;
    else if (joy_alias3 >= 0) amiga_cfg_joy_button3 = joy_alias3;
    /* One-time completion of a legacy volumes-only file.  The values above are
     * kept either way; on a failed migration the original file stays untouched
     * and the game continues with the read settings. */
    if ((saw_adlib || saw_mhi || saw_wave || saw_sfx) && !other_content)
        cfg_migrate(contents, length, saw_adlib, saw_mhi, saw_wave, saw_sfx);
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
    cfg_replace(temp_name, old_file_exists, 0);
}

#ifdef AMIGA_CFG_TEST
void AmigaCfg_TestSetPath(const char *path) { cfg_test_path = path; }
void AmigaCfg_TestResetLoad(void) { cfg_loaded = 0; }
void AmigaCfg_TestFailNextTempOpen(void) { cfg_test_fail_temp_open = 1; }
void AmigaCfg_TestFailNextReplace(void) { cfg_test_fail_rename_number = 1; }
void AmigaCfg_TestFailReplacementAfterBackup(void) { cfg_test_fail_rename_number = 2; }
#endif

#endif /* __AMIGA__ */