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

#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Token codes in the assembler syntax.
 */
typedef enum
{
    TOK_EOF,            /**< End of file encountered */
    TOK_EOL,            /**< End of the current line encountered */
    TOK_ERROR,          /**< Error token */
    TOK_IDENT,          /**< Identifier */
    TOK_DIRECTIVE,      /**< Identifier that starts with a dot */
    TOK_NUMBER,         /**< Number */
    TOK_STRING,         /**< Quoted string */
    TOK_COMMA,          /**< "," character */
    TOK_COLON,          /**< ":" character for specifying labels */
    TOK_LPAREN,         /**< "(" on its own, not part of a standard token */
    TOK_RPAREN,         /**< ")" on its own, not part of a standard token */
    TOK_MOVE,           /**< "->" for a move operation */
    TOK_M,              /**< "M" for main memory */
    TOK_I,              /**< "(I)" for the input tape */
    TOK_I_NOP,          /**< "I" - no-operation */
    TOK_N1,             /**< "(N1)" for the state of the N1 switches */
    TOK_N2,             /**< "(N2)" for the state of the N2 switches */
    TOK_A_READ,         /**< "(A)" register - read */
    TOK_A_WRITE,        /**< "A" register - write */
    TOK_A_MSB,          /**< "s(A)" - sign bit of the A register */
    TOK_A_LSB,          /**< "p1(A)" - LSB of the A register */
    TOK_A_SHR,          /**< "r(A)" - right shift on the A register */
    TOK_A_SHL,          /**< "2(A)" - left shift on the A register */
    TOK_A_CLEAR,        /**< "c(A)" - fetch the A register and clear */
    TOK_A_TEST,         /**< "z(A)" - test the A register for non-zero */
    TOK_A_PLUS,         /**< "+A" - add to the A register */
    TOK_A_MINUS,        /**< "-A" - subtract from the A register */
    TOK_A_AND,          /**< ".A" - AND with the A register */
    TOK_A_OR,           /**< "vA" - OR with the A register */
    TOK_A_XOR,          /**< "~A" - XOR with the A register */
    TOK_B_READ,         /**< "(B)" register - read */
    TOK_B_WRITE,        /**< "B" register - write */
    TOK_B_MSB,          /**< "s(B)" or "(R)" - sign bit of the B register */
    TOK_B_SHR,          /**< "r(B)" - right shift on the B register */
    TOK_B_MUL,          /**< "xB" - multiply using the B register */
    TOK_C_READ,         /**< "(C)" register - read */
    TOK_C_WRITE,        /**< "C" register - write */
    TOK_C_MSB,          /**< "s(C)" - sign bit of the C register */
    TOK_C_SHR,          /**< "r(C)" - right shift on the C register */
    TOK_C_PLUS,         /**< "+C" - add to the C register */
    TOK_C_MINUS,        /**< "-C" - subtract from the C register */
    TOK_D_READ,         /**< "(Dn)" register - read */
    TOK_D_WRITE,        /**< "Dn" register - write */
    TOK_D_MSB,          /**< "s(Dn)" - sign bit of the Dn register */
    TOK_D_SHR,          /**< "r(Dn)" - right shift on the Dn register */
    TOK_D_PLUS,         /**< "+Dn" - add to the Dn register */
    TOK_D_MINUS,        /**< "-Dn" - subtract from the Dn register */
    TOK_Z_READ,         /**< "(Z)" - load zero */
    TOK_Z_NOP,          /**< "Z" - no-operation */
    TOK_HL_READ,        /**< "(Hl)" - read H into lower bits */
    TOK_HL_WRITE,       /**< "Hl" - write lower bits into H */
    TOK_HU_READ,        /**< "(Hu)" - read H into upper bits */
    TOK_HU_WRITE,       /**< "Hu" - write upper bits into H */
    TOK_S_READ,         /**< "(S)" register - read */
    TOK_S_WRITE,        /**< "S" register - write */
    TOK_S_PLUS,         /**< "+S" register - add */
    TOK_S_SKIP,         /**< "cS" - skip next instruction if non-zero  */
    TOK_K,              /**< "K" - constant */
    TOK_K_PLUS,         /**< "+K" - modify next instruction */
    TOK_P11,            /**< "p11" - load 1 into P11 */
    TOK_P1,             /**< "p1" - load 1 into P1 */
    TOK_P20,            /**< "p20" - load 1 into P20 */
    TOK_DRUM_1,         /**< "a" - drum memory store 1 */
    TOK_DRUM_2,         /**< "b" - drum memory store 2 */
    TOK_DRUM_3,         /**< "c" - drum memory store 3 */
    TOK_DRUM_4,         /**< "d" - drum memory store 4 */
    TOK_OT,             /**< "Ot" - output to teleprinter */
    TOK_OP,             /**< "Op" - output to punch tape */
    TOK_SPEAKER,        /**< "P" - output to the loudspeaker */
    TOK_SHIFT,          /**< "Lx" - cyclic left shift by an amount */
    TOK_STOP            /**< "T" - halt if non-zero */

} token_t;

/**
 * @brief Maximum length of a token name, including the terminating NUL.
 */
#define TOKEN_NAME_MAX 256

/**
 * @brief State of the assembler's tokeniser.
 */
typedef struct tokeniser_s tokeniser_t;
struct tokeniser_s
{
    /** File stream that we are reading from */
    FILE *file;

    /** Filename for error reporting */
    char *filename;

    /** Buffer containing the current line */
    char buffer[BUFSIZ];

    /** Position within the buffer to read from next */
    int posn;

    /** Non-zero if we have already seen EOF */
    int saw_eof;

    /** Number of errors that have been reported so far */
    int num_errors;

    /** Line number the token was read from */
    long line;

    /** Token code for the token that was just recognised */
    token_t token;

    /** Numeric value of the current token, or the D register number */
    int64_t value;

    /** Name of the token */
    char name[TOKEN_NAME_MAX];

    /** Next tokeniser in the parse stack */
    tokeniser_t *next;
};

/**
 * @brief Initialises a tokeniser on a specific file.
 *
 * @param[out] tokeniser The tokeniser to initialise.
 * @param[in] filename Name of the input file to process.
 *
 * @return Non-zero if the tokeniser was initialised, or zero if the
 * file could not be opened.
 */
int tokeniser_init(tokeniser_t *tokeniser, const char *filename);

/**
 * @brief Frees a tokeniser.
 *
 * @param[in] tokeniser The tokeniser to free.
 */
void tokeniser_free(tokeniser_t *tokeniser);

/**
 * @brief Gets the token token from the input stream.
 *
 * @param[out] tokeniser The tokeniser.
 *
 * @return The next token code, TOK_EOF at the end of the stream.
 */
token_t tokeniser_get_next(tokeniser_t *tokeniser);

#ifdef __cplusplus
}
#endif

#endif /* TOKEN_H */
