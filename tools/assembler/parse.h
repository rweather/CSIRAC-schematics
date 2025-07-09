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

#ifndef PARSE_H
#define PARSE_H

#include "token.h"
#include "code.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Control structure for parsing CSIRAC assembly code.
 */
typedef struct
{
    /** Current tokeniser on the stack of open files */
    tokeniser_t *tokeniser;

    /** Code block that is being built by the parser */
    code_t code;

    /** Number of errors that occurred during the parsing process */
    int num_errors;

    /** Instruction that is in the progress of being built */
    insn_t insn;

    /** Link register to use for the current function */
    link_register_t link_register;

    /** Last destination gate that was encountered */
    csirac_dest_gate_t last_dest;

} parser_t;

/**
 * @brief Initialises the parsing process for CSIRAC assembly code.
 *
 * @param[out] parser Points to the parser state.
 * @param[in] flags Code generation flags.
 */
void parser_init(parser_t *parser, int flags);

/**
 * @brief Frees the parser state for CSIRAC assembly code.
 *
 * @param[in] parser Points to the parser state.
 */
void parser_free(parser_t *parser);

/**
 * @brief Parse the contents of a file.
 *
 * @param[in] parser Points to the parser state.
 * @param[in] filename Name of the file.
 *
 * @return Non-zero on success, zero on error.
 */
int parser_parse(parser_t *parser, const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* PARSE_H */
