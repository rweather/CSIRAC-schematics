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

static int csirac_write_drum_word(FILE *file, csirac_word_t word)
{
    uint8_t buf[3] = {
        (uint8_t)((word & CSIRAC_WORD_MASK) >> 16),
        (uint8_t)(word >> 8),
        (uint8_t)word
    };
    if (fwrite(buf, 1, sizeof(buf), file) != sizeof(buf)) {
        return 0;
    }
    return 1;
}

static int csirac_read_drum_word(FILE *file, csirac_word_t *word)
{
    uint8_t buf[3];
    if (fread(buf, 1, sizeof(buf), file) != sizeof(buf)) {
        return 0;
    }
    *word = (((csirac_word_t)buf[0]) << 16) |
            (((csirac_word_t)buf[1]) << 8)  |
             ((csirac_word_t)buf[2]);
    *word &= CSIRAC_WORD_MASK; /* Restrict the word to 20 bits */
    return 1;
}

int csirac_save_drum(const csirac_state_t *state, const char *filename)
{
    FILE *file;
    int store, address;
    int ok = 1;
    if ((file = fopen(filename, "wb")) == NULL) {
        return 0;
    }
    for (store = 0; ok && store < CSIRAC_NUM_DRUM_STORES; ++store) {
        for (address = 0; ok && address < CSIRAC_DRUM_STORE_SIZE; ++address) {
            ok = csirac_write_drum_word(file, state->MD[store][address]);
        }
    }
    fclose(file);
    return ok;
}

int csirac_load_drum(csirac_state_t *state, const char *filename)
{
    FILE *file;
    csirac_word_t word;
    int store, address;
    int ok = 1;
    if ((file = fopen(filename, "rb")) == NULL) {
        return 0;
    }
    for (store = 0; ok && store < CSIRAC_NUM_DRUM_STORES; ++store) {
        for (address = 0; ok && address < CSIRAC_DRUM_STORE_SIZE; ++address) {
            ok = csirac_read_drum_word(file, &word);
            if (ok) {
                state->MD[store][address] = word;
            }
        }
    }
    fclose(file);
    return ok;
}
