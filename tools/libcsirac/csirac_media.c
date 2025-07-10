/*
 * Copyright (C) 2025 Rhys Weatherley
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "csirac.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static csirac_media_t *csirac_open_media
    (csirac_state_t *state, csirac_media_type_t type,
     const char *filename, const char *mode)
{
    csirac_media_t *media;
    FILE *file;

    /* Attempt to open the media */
    if (filename && strcmp(filename, "-") != 0) {
        file = fopen(filename, mode);
        if (!file) {
            return 0;
        }
    } else if (!strcmp(mode, "r") || !strcmp(mode, "rb")) {
        file = stdin;
    } else {
        file = stdout;
    }

    /* Allocate a media object and populate it */
    media = (csirac_media_t *)calloc(1, sizeof(csirac_media_t));
    if (!media) {
        fclose(file);
        errno = ENOMEM;
        return 0;
    }
    media->type = type;
    media->figure_shift = 0;
    media->ch = CSIRAC_WORD_MASK;
    media->file = file;
    media->next = 0;
    media->state = state;
    return media;
}

static void csirac_free_media(csirac_media_t *media)
{
    if (media->file != stdin && media->file != stdout) {
        fclose(media->file);
    }
    free(media);
}

int csirac_open_input_tape
    (csirac_state_t *state, csirac_media_type_t type, const char *filename)
{
    csirac_media_t *media;
    csirac_media_t **list;

    /* Attempt to open the media */
    if (type == CSIRAC_MEDIA_8HOLE_TAPE) {
        media = csirac_open_media(state, type, filename, "rb");
    } else {
        media = csirac_open_media(state, type, filename, "r");
    }
    if (!media) {
        return 0;
    }

    /* Add the new media object to the end of the input tape list */
    list = &(state->input_tape);
    while (*list != 0) {
        list = &((*list)->next);
    }
    *list = media;
    return 1;
}

static void csirac_close_current_input_tape(csirac_state_t *state)
{
    csirac_media_t *media = state->input_tape;
    state->input_tape = media->next;
    csirac_free_media(media);
}

void csirac_close_input_tape(csirac_state_t *state)
{
    while (state->input_tape != 0) {
        csirac_close_current_input_tape(state);
    }
}

void csirac_move_input_tape_to_front(csirac_state_t *state)
{
    if (state->input_tape != 0) {
        csirac_media_t **last = &(state->input_tape);
        csirac_media_t *media;
        while ((*last)->next != 0) {
            last = &((*last)->next);
        }
        media = *last;
        *last = 0;
        media->next = state->input_tape;
        state->input_tape = media;
    }
}

int csirac_open_teleprinter
    (csirac_state_t *state, csirac_media_type_t type, const char *filename)
{
    csirac_close_teleprinter(state);
    state->teleprinter = csirac_open_media(state, type, filename, "w");
    return state->teleprinter != 0;
}

void csirac_close_teleprinter(csirac_state_t *state)
{
    if (state->teleprinter) {
        csirac_free_media(state->teleprinter);
        state->teleprinter = 0;
    }
}

int csirac_open_output_punch
    (csirac_state_t *state, csirac_media_type_t type, const char *filename)
{
    csirac_close_output_punch(state);
    if (type == CSIRAC_MEDIA_8HOLE_TAPE) {
        state->output_punch = csirac_open_media(state, type, filename, "wb");
    } else {
        state->output_punch = csirac_open_media(state, type, filename, "w");
    }
    return state->output_punch != 0;
}

void csirac_close_output_punch(csirac_state_t *state)
{
    if (state->output_punch) {
        csirac_free_media(state->output_punch);
        state->output_punch = 0;
    }
}

/**
 * @brief Mapping from teleprinter codes to Unicode.
 */
static uint16_t const teleprinter_to_unicode[64] = {
    /* Letter shift symbols */
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z',
    0x039B,     /* U+039B GREEK CAPITAL LETTER LAMDA */
    0x000E,     /* Figure shift - U+000E SHIFT OUT */
    0x000F,     /* Letter shift - U+000F SHIFT IN */
    0x000A,     /* Line feed */
    0x000D,     /* Carriage return */
    ' ',        /* Space */

    /* Figure shift symbols */
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '+', '-', '.', ')', '(', 'i', 'j', 'k',
    0x2207,     /* U+2207 NABLA */
    0x03D5,     /* U+03D5 GREEK PHI SYMBOL */
    0x03C8,     /* U+03C8 GREEK SMALL LETTER PSI */
    0x03B8,     /* U+03B8 GREEK SMALL LETTER THETA */
    0x03A9,     /* U+03A9 GREEK CAPITAL LETTER OMEGA */
    0x0393,     /* U+0393 GREEK CAPITAL LETTER GAMMA */
    0x03C0,     /* U+03C0 GREEK SMALL LETTER PI */
    0x03A3,     /* U+03A3 GREEK CAPITAL LETTER SIGMA */
    0x039E,     /* U+039E GREEK CAPITAL LETTER XI */
    0x000E,     /* Figure shift - U+000E SHIFT OUT */
    0x000F,     /* Letter shift - U+000F SHIFT IN */
    0x000A,     /* Line feed */
    0x000D,     /* Carriage return */
    ' '         /* Space */
};

/**
 * @brief Mapping from Flexowriter codes to Unicode.
 */
static uint16_t const flexowriter_to_unicode[64] = {
    /* Letter shift symbols */
    'b',        /* Blank (not the same as space) */
    'Q', 'W', 'C', 'R', 'K', 'L', 'U', 'I', 'D', 'V',
    'A', 'F', 'M', 'G', 'N', 'P', 'J', 'H', 'E', 'B',
    'T', 'Y', 'S', 'X', 'O', 'Z',
    0x000E,     /* Figure shift - U+000E SHIFT OUT */
    ' ',        /* Space */
    0x000D,     /* Carriage return */
    0x000F,     /* Letter shift - U+000F SHIFT IN */
    0x000A,     /* Line feed */

    /* Figure shift symbols */
    'b',        /* Blank (not the same as space) */
    '1', '2', '*', '4', '(', ')', '7', '8', '#', '=',
    '-', '&', '.',
    0x0009,     /* TAB */
    ',', '0',
    's',        /* STOP */
    0x00A3,     /* U+00A3 - POUND SIGN */
    '3', '\'', '5', '6', '/', 'x', '9', '+',
    0x000E,     /* Figure shift - U+000E SHIFT OUT */
    ' ',        /* Space */
    0x000D,     /* Carriage return */
    0x000F,     /* Letter shift - U+000F SHIFT IN */
    0x000A      /* Line feed */

    /*
     * The 'b' and 's' symbols are compatible with the Windows 98 emulator.
     * On input the following symbols are also supported:
     *
     *      f - Figure shift
     *      l - Letter shift
     *      e - Erase
     */
};

static int csirac_get_num(const char *str)
{
    int value;
    if (str[0] != ' ') {
        if (str[0] < '0' || str[0] > '9') {
            return -1;
        }
        if (str[1] < '0' || str[1] > '9') {
            return -1;
        }
        value = (str[0] - '0') * 10 + (str[1] - '0');
        return (value < 32) ? value : -1;
    } else if (str[1] >= '0' && str[1] <= '9') {
        return str[1] - '0';
    } else if (str[1] == ' ') {
        return 0;
    } else {
        return -1;
    }
}

static int csirac_media_is_meta(const char *line, const char *meta)
{
    size_t len = strlen(meta);
    if (strncmp(line, meta, len) != 0) {
        return 0;
    }
    return line[len] == ' ' || line[len] == '\t' ||
           line[len] == '\r' || line[len] == '\n' ||
           line[len] == '\0';
}

static void csirac_media_change_type
    (csirac_media_t *media, csirac_media_type_t type)
{
    if (media != 0) {
        media->type = type;
    }
}

static int csirac_media_get_12hole(csirac_media_t *media, csirac_word_t *word)
{
    /* Format is "NN MMXY" where "NN" is the source group, "MM" is the
     * destination group, and "X"/"Y" are the X and Y punch holes. */
    char linebuf[256];
    int value1, value2;
    while (fgets(linebuf, sizeof(linebuf), media->file)) {
        if (linebuf[0] == '+') {
            /* Tape metadata for setting input and output sources.
             * Adjust the type of output teleprinter if necessary. */
            if (csirac_media_is_meta(linebuf, "+Ot=teleprinter")) {
                csirac_media_change_type
                    (media->state->teleprinter, CSIRAC_MEDIA_TELEPRINTER);
            } else if (csirac_media_is_meta(linebuf, "+Ot=flexowriter")) {
                csirac_media_change_type
                    (media->state->teleprinter, CSIRAC_MEDIA_FLEXOWRITER);
            } else if (csirac_media_is_meta(linebuf, "+Ot=ascii")) {
                csirac_media_change_type
                    (media->state->teleprinter, CSIRAC_MEDIA_ASCII);
            }
            continue;
        }
        value1 = csirac_get_num(linebuf);
        value2 = csirac_get_num(linebuf + 3);
        if (value1 >= 0 && linebuf[2] == ' ' && value2 >= 0) {
            *word = value1 * 32 + value2;
            if (linebuf[5] == 'X') {
                *word |= CSIRAC_WORD_X;
                if (linebuf[6] == 'Y') {
                    *word |= CSIRAC_WORD_Y;
                }
            } else if (linebuf[5] == ' ' && linebuf[6] == 'Y') {
                *word |= CSIRAC_WORD_Y;
            }
            return 1;
        }
    }
    *word = 0;
    return 0;
}

static int csirac_media_get_5hole(csirac_media_t *media, csirac_word_t *word)
{
    char linebuf[256];
    int value;
    while (fgets(linebuf, sizeof(linebuf), media->file)) {
        /* The values are expected to be in decimal between 0 and 31.
         * Blank lines and lines without numbers are ignored. */
        if (sscanf(linebuf, "%d", &value) == 1) {
            *word = value & 0x1F;
            return 1;
        }
    }
    *word = 0;
    return 0;
}

static int csirac_read_unicode(FILE *file)
{
    int ch = fgetc(file);
    int code = 0;
    if (ch < 0xC0) {
        /* Single-character UTF-8, EOF, or invalid */
        return ch;
    } else if (ch < 0xE0) {
        /* Two-character UTF-8 */
        code = (ch - 0xC0) << 6;
        ch = fgetc(file);
        if (ch < 0x80 || ch > 0xBF) {
            ungetc(ch, file);
            return code;
        }
        return code + (ch - 0x80);
    } else if (ch < 0xF0) {
        /* Three-character UTF-8 */
        code = (ch - 0xE0) << 12;
        ch = fgetc(file);
        if (ch < 0x80 || ch > 0xBF) {
            ungetc(ch, file);
            return code;
        }
        code += (ch - 0x80) << 6;
        ch = fgetc(file);
        if (ch < 0x80 || ch > 0xBF) {
            ungetc(ch, file);
            return code;
        }
        return code + (ch - 0x80);
    } else {
        /* 4 or more character UTF-8 sequences are not yet supported */
        return ch;
    }
}

static int csirac_media_lookup_teleprinter
    (int figure_shift, uint16_t ch, const uint16_t *table,
     csirac_word_t *word, int *shift)
{
    unsigned index;

    /* Convert lower case to upper case if necessary */
    if (ch >= 'a' && ch <= 'z') {
        ch  = ch - 'a' + 'A';
    }

    /* Look for the Unicode character in the table.  We first look in the
     * table that is active (figure or letter) and then the other one.
     * This avoids an unnecessary shift for common characters like space. */
    if (figure_shift) {
        for (index = 0; index < 32; ++index) {
            if (table[index + 32] == ch) {
                /* Found it in the figure shift table */
                *shift = 1;
                *word = index;
                return 1;
            }
        }
        for (index = 0; index < 32; ++index) {
            if (table[index] == ch) {
                /* Found it in the letter shift table */
                *shift = 0;
                *word = index;
                return 1;
            }
        }
    } else {
        for (index = 0; index < 32; ++index) {
            if (table[index] == ch) {
                /* Found it in the letter shift table */
                *shift = 0;
                *word = index;
                return 1;
            }
        }
        for (index = 0; index < 32; ++index) {
            if (table[index + 32] == ch) {
                /* Found it in the figure shift table */
                *shift = 0;
                *word = index;
                return 1;
            }
        }
    }
    return 0;
}

static int csirac_media_get_teleprinter
    (csirac_media_t *media, csirac_word_t *word)
{
    int ch, shift;
    if ((media->ch & CSIRAC_WORD_MASK) == 0) {
        /* We inserted a figure shift, letter shift, or line feed, so now
         * return the actual character that needs to be read. */
        *word = media->ch;
        media->ch = CSIRAC_WORD_MASK;
        return 1;
    }
    while ((ch = csirac_read_unicode(media->file)) >= 0) {
        if (ch == '\r') {
            /* Eat the next character as well if we have a CRLF sequence */
            ch = fgetc(media->file);
            if (ch >= 0 && ch != '\n') {
                ungetc(ch, media->file);
            }
            media->ch = 0x001D; /* Report LF on the next call */
            *word = 0x001E;     /* Report CR now */
            return 1;
        } else if (ch == '\n') {
            /* Line feed on its own in plain text becomes CRLF */
            media->ch = 0x001D; /* Report LF on the next call */
            *word = 0x001E;     /* Report CR now */
            return 1;
        } else if (csirac_media_lookup_teleprinter
                        (media->figure_shift, ch, teleprinter_to_unicode,
                         word, &shift)) {
            /* We have found a mapping, but we may need to insert a
             * figure or letter shift to change to the correct mode. */
            if (media->figure_shift != shift) {
                media->figure_shift = shift;
                media->ch = *word;
                if (shift) {
                    *word = 0x001B;
                } else {
                    *word = 0x001C;
                }
            }
            return 1;
        } else {
            /* Everything else is ignored */
        }
    }
    *word = 0;
    return 0;
}

static int csirac_media_get_flexowriter
    (csirac_media_t *media, csirac_word_t *word)
{
    int ch, shift;
    if ((media->ch & CSIRAC_WORD_MASK) == 0) {
        /* We inserted a figure or letter shift, so now return the
         * actual character that needs to be read. */
        *word = media->ch;
        media->ch = CSIRAC_WORD_MASK;
        return 1;
    }
    while ((ch = csirac_read_unicode(media->file)) >= 0) {
        if (ch == 'e') {
            /* Ignore erase characters */
            continue;
        } else if (ch == '\r') {
            /* Eat the next character as well if we have a CRLF sequence */
            ch = fgetc(media->file);
            if (ch >= 0 && ch != '\n') {
                ungetc(ch, media->file);
            }
            *word = 0x001D;
            return 1;
        } else if (ch == '\n') {
            /* Line feed on its own in plain text becomes CR for Flexowriter,
             * as the actual Flexowriter line feed is used for "erase". */
            *word = 0x001D;
            return 1;
        } else if (ch == 'b') {
            /* Convert 'b' into the "blank" symbol */
            *word = 0;
            return 1;
        } else if (ch == 's') {
            /* Convert 's' into the "stop" symbol, which needs figure shift */
            if (!(media->figure_shift)) {
                media->figure_shift = 1;
                media->ch = 0x0011;
                *word = 0x001B;
            } else {
                *word = 0x0011;
            }
            return 1;
        } else if (ch == 'f' ) {
            /* Explicit change to figure shift mode */
            media->figure_shift = 1;
            *word = 0x001B;
            return 1;
        } else if (ch == 'l' ) {
            /* Explicit change to letter shift mode */
            media->figure_shift = 0;
            *word = 0x001E;
            return 1;
        } else if (ch == '$' ) {
            /* Map the dollar sign to the Flexowriter's pound sign */
            if (!(media->figure_shift)) {
                media->figure_shift = 1;
                media->ch = 0x0012;
                *word = 0x001B;
            } else {
                *word = 0x0012;
            }
            return 1;
        } else if (csirac_media_lookup_teleprinter
                        (media->figure_shift, ch, flexowriter_to_unicode,
                         word, &shift)) {
            /* We have found a mapping, but we may need to insert a
             * figure or letter shift to change to the correct mode. */
            if (media->figure_shift != shift) {
                media->figure_shift = shift;
                media->ch = *word;
                if (shift) {
                    *word = 0x001B;
                } else {
                    *word = 0x001E;
                }
            }
            return 1;
        } else {
            /* Everything else is ignored */
        }
    }
    *word = 0;
    return 0;
}

static int csirac_media_get_ascii
    (csirac_media_t *media, csirac_word_t *word)
{
    int ch = fgetc(media->file);
    if (ch >= 0) {
        /* Return the ASCII character in the low 8 bits of the word */
        *word = (csirac_word_t)ch;
        return 1;
    } else {
        *word = 0;
        return 0;
    }
}

static int csirac_media_get_word(csirac_media_t *media, csirac_word_t *word)
{
    switch (media->type) {
    case CSIRAC_MEDIA_12HOLE_TAPE:
        return csirac_media_get_12hole(media, word);

    case CSIRAC_MEDIA_5HOLE_TAPE:
        return csirac_media_get_5hole(media, word);

    case CSIRAC_MEDIA_TELEPRINTER:
        return csirac_media_get_teleprinter(media, word);

    case CSIRAC_MEDIA_FLEXOWRITER:
        return csirac_media_get_flexowriter(media, word);

    case CSIRAC_MEDIA_8HOLE_TAPE:
    case CSIRAC_MEDIA_ASCII:
        return csirac_media_get_ascii(media, word);

    default:
        break;
    }
    return 0;
}

int csirac_media_get(csirac_state_t *state, csirac_word_t *word)
{
    csirac_media_t *media;

    /* Credit a symbol read from tape */
    ++(state->tape_counter);

    /* Find the next input media that is willing to give us a word */
    while (state->input_tape != 0) {
        media = state->input_tape;
        if (csirac_media_get_word(media, word)) {
            return 1;
        }
        if (media->figure_shift) {
            /* If the current media has figure shift active, then we
             * need to turn it off before proceeding to the next tape. */
            csirac_media_type_t type = media->type;
            csirac_close_current_input_tape(state);
            if (type == CSIRAC_MEDIA_TELEPRINTER) {
                *word = 0x1C; /* Letter shift for the teleprinter */
                return 1;
            } else if (type == CSIRAC_MEDIA_FLEXOWRITER) {
                *word = 0x1E; /* Letter shift for the Flexowriter */
                return 1;
            }
        } else {
            csirac_close_current_input_tape(state);
        }
    }

    /* If we get here, then all input tapes have been exhausted */
    return 0;
}

static void csirac_media_write_unicode(csirac_media_t *media, uint16_t ch)
{
    /* Convert the UCS-2 code point into UTF-8 */
    if (ch < 0x0080) {
        putc((char)ch, media->file);
    } else if (ch < 0x0800) {
        putc((char)((ch >> 6) | 0xC0), media->file);
        putc((char)((ch & 0x3F) | 0x80), media->file);
    } else {
        putc((char)((ch >> 12) | 0xE0), media->file);
        putc((char)(((ch >> 6) & 0x3F) | 0x80), media->file);
        putc((char)((ch & 0x3F) | 0x80), media->file);
    }
}

static void csirac_media_write_to_printer
    (csirac_media_t *media, csirac_word_t word, const uint16_t *table)
{
    /* Combine P1..P5 with P11..P15 to get the printer code to use */
    int code = (int)((word | (word >> 10)) & 0x1F);
    uint16_t unicode;
    if (media->figure_shift) {
        unicode = table[(code & 0x1F) | 0x20];
    } else {
        unicode = table[code & 0x1F];
    }
    if (unicode == 0x000E) {
        media->figure_shift = 1;
    } else if (unicode == 0x000F) {
        media->figure_shift = 0;
    } else {
        csirac_media_write_unicode(media, unicode);
    }
}

static void csirac_media_write_to_ascii
    (csirac_media_t *media, csirac_word_t word)
{
    /* Combine P1..P8 with P11..P18 to get the ASCII character to write */
    int ch = (int)((word | (word >> 10)) & 0xFF);
    putc(ch, media->file);
}

void csirac_media_put(csirac_media_t *media, csirac_word_t word)
{
    if (!media) {
        /* Do nothing if we don't have any output media to work with */
        return;
    }
    switch (media->type) {
    case CSIRAC_MEDIA_12HOLE_TAPE:
        /* Format is "NN MMXY" where "NN" is the source group,
         * "MM" is the destination group, and "X"/"Y" are the
         * X and Y punch holes. */
        fprintf(media->file, "%2d %2d",
                (int)((word >> 5) & 0x1F), (int)(word & 0x1F));
        if (word & CSIRAC_WORD_Y) {
            if (word & CSIRAC_WORD_X) {
                putc('X', media->file);
            } else {
                putc(' ', media->file);
            }
            putc('Y', media->file);
        } else if (word & CSIRAC_WORD_X) {
            putc('X', media->file);
        }
        putc('\n', media->file);
        break;

    case CSIRAC_MEDIA_5HOLE_TAPE:
        /* Combine P1..P5 with P11..P15 and format as a number 0..31 */
        fprintf(media->file, "%2d\n",
                (int)((word | (word >> 10)) & 0x1F));
        break;

    case CSIRAC_MEDIA_TELEPRINTER:
        csirac_media_write_to_printer(media, word, teleprinter_to_unicode);
        break;

    case CSIRAC_MEDIA_FLEXOWRITER:
        csirac_media_write_to_printer(media, word, flexowriter_to_unicode);
        break;

    case CSIRAC_MEDIA_8HOLE_TAPE:
    case CSIRAC_MEDIA_ASCII:
        csirac_media_write_to_ascii(media, word);
        break;
    }
}

static int csirac_string_to_codes
    (uint8_t *out, size_t out_size, const char *in, size_t in_size,
     int *flags, const uint16_t *table)
{
    size_t out_posn = 0;
    size_t in_posn = 0;
    int32_t ch;
    int figure_shift = ((*flags) & CSIRAC_STR_CONVERT_FIGURE_SHIFT) != 0;
    int next_shift;
    csirac_word_t word;
    while ((ch = csirac_string_get_next_unicode(in, in_size, &in_posn)) >= 0) {
        if (ch >= 0x10000L) {
            return -1;
        }
        if (!csirac_media_lookup_teleprinter
                (figure_shift, (uint16_t)ch, table, &word, &next_shift)) {
            return -1;
        }
        if (next_shift != figure_shift &&
                ((*flags) & CSIRAC_STR_CONVERT_NO_SHIFT) == 0) {
            /* Insert a figure or letter shift before the next character */
            uint8_t shift_char;
            if (table == flexowriter_to_unicode) {
                shift_char = next_shift ? 0x1B : 0x1E;
            } else {
                shift_char = next_shift ? 0x1B : 0x1C;
            }
            if (out_size == 0) {
                return -2;
            }
            *out++ = shift_char;
            ++out_posn;
            --out_size;
        }
        if (out_size == 0) {
            return -2;
        }
        *out++ = (uint8_t)word;
        ++out_posn;
        --out_size;
        figure_shift = next_shift;
        *flags &= ~CSIRAC_STR_CONVERT_NO_SHIFT;
    }
    if (figure_shift) {
        *flags |= CSIRAC_STR_CONVERT_FIGURE_SHIFT;
    } else {
        *flags &= ~CSIRAC_STR_CONVERT_FIGURE_SHIFT;
    }
    return (int)out_posn;
}

int csirac_string_to_teleprinter
    (uint8_t *out, size_t out_size, const char *in, size_t in_size, int *flags)
{
    return csirac_string_to_codes
        (out, out_size, in, in_size, flags, teleprinter_to_unicode);
}

int csirac_string_to_flexowriter
    (uint8_t *out, size_t out_size, const char *in, size_t in_size, int *flags)
{
    return csirac_string_to_codes
        (out, out_size, in, in_size, flags, flexowriter_to_unicode);
}

size_t csirac_string_unicode_length(const char *str, size_t size)
{
    size_t result = 0;
    while (size > 0) {
        uint8_t ch = (uint8_t)(*str++);
        if ((ch & 0xC0) != 0x80) {
            ++result;
        }
        --size;
    }
    return result;
}

int32_t csirac_string_get_next_unicode
    (const char *str, size_t size, size_t *posn)
{
    int ch, ch2, ch3, ch4;
    if (*posn >= size) {
        return -1;
    }
    ch = (str[(*posn)++] & 0xFF);
    if (ch < 0x80) {
        return ch;
    } else if ((ch & 0xE0) == 0xC0) {
        if (*posn < size) {
            ch2 = (str[(*posn)++] & 0xFF);
            if (ch2 >= 0x80 && ch2 <= 0xBF) {
                return ((ch & 0x1F) << 6) | (ch2 & 0x3F);
            }
        }
    } else if ((ch & 0xF0) == 0xE0) {
        if (*posn < size) {
            ch2 = (str[(*posn)++] & 0xFF);
            if (ch2 >= 0x80 && ch2 <= 0xBF) {
                if (*posn < size) {
                    ch3 = (str[(*posn)++] & 0xFF);
                    if (ch3 >= 0x80 && ch3 <= 0xBF) {
                        return ((ch & 0x0F) << 12) | ((ch2 & 0x3F) << 6) |
                               (ch3 & 0x3F);
                    }
                }
            }
        }
    } else if ((ch & 0xF8) == 0xF0) {
        if (*posn < size) {
            ch2 = (str[(*posn)++] & 0xFF);
            if (ch2 >= 0x80 && ch2 <= 0xBF) {
                if (*posn < size) {
                    ch3 = (str[(*posn)++] & 0xFF);
                    if (ch3 >= 0x80 && ch3 <= 0xBF) {
                        if (*posn < size) {
                            ch4 = (str[(*posn)++] & 0xFF);
                            if (ch4 >= 0x80 && ch4 <= 0xBF) {
                                return ((ch & 0x07) << 18) | ((ch2 & 0x3F) << 12) |
                                      ((ch3 & 0x3F) <<  6) | (ch4 & 0x3F);
                            }
                        }
                    }
                }
            }
        }
    }
    return ch;
}
