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

csirac_word_t const csirac_primary_routine[] = {
    CSIRAC_WORD( 0,  0, 17, 18),
    CSIRAC_WORD( 0,  0, 18, 25),
    CSIRAC_WORD( 0,  0, 22,  5),
    CSIRAC_WORD( 0,  0, 18, 25),
    CSIRAC_WORD( 0,  0, 23, 24),
    CSIRAC_WORD( 0,  0, 21,  5),
    CSIRAC_WORD( 0,  0, 11, 24),
    CSIRAC_WORD( 0,  0, 14, 26),
    CSIRAC_WORD( 0,  0,  9,  0),
    CSIRAC_WORD( 0,  0, 24, 15),
    CSIRAC_WORD( 0,  0,  1, 17),
    CSIRAC_WORD( 0,  0, 17, 21),
    CSIRAC_WORD( 0,  0, 18, 25),
    CSIRAC_WORD( 0,  0, 20, 23),
    CSIRAC_WORD( 0,  0, 22, 14),
    CSIRAC_WORD( 0,  0, 17, 19),
    CSIRAC_WORD( 0,  0, 20, 22),
    CSIRAC_WORD( 0,  0, 20, 23)
};

size_t const csirac_primary_routine_len =
    sizeof(csirac_primary_routine) / sizeof(csirac_primary_routine[0]);
size_t const csirac_short_primary_routine_len = 14;

csirac_word_t const csirac_control_routine[] = {
    CSIRAC_WORD( 0,  0, 22, 24),
    CSIRAC_WORD( 0, 24,  0,  5),
    CSIRAC_WORD( 0, 23,  0,  5),
    CSIRAC_WORD( 0, 15,  0,  5),
    CSIRAC_WORD( 0, 19,  9,  0),
    CSIRAC_WORD( 0,  0, 31, 31),
    CSIRAC_WORD(31, 21, 26, 26),
    CSIRAC_WORD( 0, 11, 26, 11),
    CSIRAC_WORD( 0, 10, 26, 23),
    CSIRAC_WORD( 0,  0, 13, 27),
    CSIRAC_WORD(31,  8, 12, 14)
};

size_t const csirac_control_routine_len =
    sizeof(csirac_control_routine) / sizeof(csirac_control_routine[0]);
