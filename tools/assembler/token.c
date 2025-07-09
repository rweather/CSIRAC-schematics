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

#include "token.h"
#include <string.h>
#include <stdlib.h>

int tokeniser_init(tokeniser_t *tokeniser, const char *filename)
{
    memset(tokeniser, 0, sizeof(tokeniser_t));
    if (!filename || !strcmp(filename, "-")) {
        tokeniser->file = stdin;
        filename = "standard input";
    } else {
        tokeniser->file = fopen(filename, "r");
        if (!(tokeniser->file)) {
            return 0;
        }
    }
    tokeniser->filename = strdup(filename);
    tokeniser->token = TOK_EOL;
    return 1;
}

void tokeniser_free(tokeniser_t *tokeniser)
{
    if (tokeniser->file != NULL && tokeniser->file != stdin) {
        fclose(tokeniser->file);
    }
    if (tokeniser->filename != NULL) {
        free(tokeniser->filename);
    }
    memset(tokeniser, 0, sizeof(tokeniser_t));
}

static int tokeniser_is_space(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '\v';
}

static int tokeniser_is_alpha(char ch)
{
    if (ch >= 'A' && ch <= 'Z') {
        return 1;
    } else if (ch >= 'a' && ch <= 'z') {
        return 1;
    } else {
        return ch == '_';
    }
}

static int tokeniser_is_digit(char ch)
{
    return ch >= '0' && ch <= '9';
}

static int tokeniser_is_alphanum(char ch)
{
    return tokeniser_is_alpha(ch) || tokeniser_is_digit(ch);
}

static void tokeniser_set_token
    (tokeniser_t *tokeniser, token_t token, int size)
{
    tokeniser->token = token;
    if (size < (int)(sizeof(tokeniser->name))) {
        memcpy(tokeniser->name, tokeniser->buffer + tokeniser->posn, size);
        tokeniser->name[size] = '\0';
    } else {
        strncpy(tokeniser->name, tokeniser->buffer + tokeniser->posn,
                sizeof(tokeniser->name));
        tokeniser->name[sizeof(tokeniser->name) - 1] = '\0';
    }
    tokeniser->posn += size;
}

static int64_t tokeniser_get_number(const char *str)
{
    int64_t value = 0;
    int is_neg = 0;
    char ch;
    if (*str == '-') {
        is_neg = 1;
        ++str;
    }
    while ((ch = *str++) != '\0') {
        if (tokeniser_is_digit(ch)) {
            value = value * 10 + (ch - '0');
        }
    }
    return is_neg ? (-value) : value;
}

static int tokeniser_match_keyword(tokeniser_t *tokeniser)
{
    typedef struct {
        token_t token;      /**< Token code */
        const char *name;   /**< Name of the token */
        int len;            /**< Number of characters in the name */
        int ends_in_ident;  /**< 1 if the name ends in an identifier char */
    } keyword_t;
    static keyword_t const keywords[] = {
        {TOK_MOVE,      "->",       2,  0},
        {TOK_M,         "M",        1,  1},
        {TOK_I,         "(I)",      3,  0},
        {TOK_I_NOP,     "I",        1,  1},
        {TOK_N1,        "(N1)",     4,  0},
        {TOK_N2,        "(N2)",     4,  0},
        {TOK_A_READ,    "(A)",      3,  0},
        {TOK_A_WRITE,   "A",        1,  1},
        {TOK_A_MSB,     "s(A)",     4,  0},
        {TOK_A_LSB,     "p1(A)",    5,  0},
        {TOK_A_SHR,     "r(A)",     4,  0},
        {TOK_A_SHL,     "2(A)",     4,  0},
        {TOK_A_CLEAR,   "c(A)",     4,  0},
        {TOK_A_TEST,    "z(A)",     4,  0},
        {TOK_A_PLUS,    "+A",       2,  1},
        {TOK_A_MINUS,   "-A",       2,  1},
        {TOK_A_AND,     ".A",       2,  1},
        {TOK_A_OR,      "vA",       2,  1},
        {TOK_A_XOR,     "~A",       2,  1},
        {TOK_B_READ,    "(B)",      3,  0},
        {TOK_B_WRITE,   "B",        1,  1},
        {TOK_B_MSB,     "(R)",      3,  0},
        {TOK_B_SHR,     "r(B)",     4,  0},
        {TOK_B_MUL,     "xB",       2,  1},
        {TOK_C_READ,    "(C)",      3,  0},
        {TOK_C_WRITE,   "C",        1,  1},
        {TOK_C_MSB,     "s(C)",     4,  0},
        {TOK_C_SHR,     "r(C)",     4,  0},
        {TOK_C_PLUS,    "+C",       2,  1},
        {TOK_C_MINUS,   "-C",       2,  1},
        {TOK_D_READ,    "(D15)",    5,  0},
        {TOK_D_READ,    "(D14)",    5,  0},
        {TOK_D_READ,    "(D13)",    5,  0},
        {TOK_D_READ,    "(D12)",    5,  0},
        {TOK_D_READ,    "(D11)",    5,  0},
        {TOK_D_READ,    "(D10)",    5,  0},
        {TOK_D_READ,    "(D9)",     4,  0},
        {TOK_D_READ,    "(D8)",     4,  0},
        {TOK_D_READ,    "(D7)",     4,  0},
        {TOK_D_READ,    "(D6)",     4,  0},
        {TOK_D_READ,    "(D5)",     4,  0},
        {TOK_D_READ,    "(D4)",     4,  0},
        {TOK_D_READ,    "(D3)",     4,  0},
        {TOK_D_READ,    "(D2)",     4,  0},
        {TOK_D_READ,    "(D1)",     4,  0},
        {TOK_D_READ,    "(D0)",     4,  0},
        {TOK_D_WRITE,   "D15",      3,  1},
        {TOK_D_WRITE,   "D14",      3,  1},
        {TOK_D_WRITE,   "D13",      3,  1},
        {TOK_D_WRITE,   "D12",      3,  1},
        {TOK_D_WRITE,   "D11",      3,  1},
        {TOK_D_WRITE,   "D10",      3,  1},
        {TOK_D_WRITE,   "D9",       2,  1},
        {TOK_D_WRITE,   "D8",       2,  1},
        {TOK_D_WRITE,   "D7",       2,  1},
        {TOK_D_WRITE,   "D6",       2,  1},
        {TOK_D_WRITE,   "D5",       2,  1},
        {TOK_D_WRITE,   "D4",       2,  1},
        {TOK_D_WRITE,   "D3",       2,  1},
        {TOK_D_WRITE,   "D2",       2,  1},
        {TOK_D_WRITE,   "D1",       2,  1},
        {TOK_D_WRITE,   "D0",       2,  1},
        {TOK_D_MSB,     "s(D15)",   6,  0},
        {TOK_D_MSB,     "s(D14)",   6,  0},
        {TOK_D_MSB,     "s(D13)",   6,  0},
        {TOK_D_MSB,     "s(D12)",   6,  0},
        {TOK_D_MSB,     "s(D11)",   6,  0},
        {TOK_D_MSB,     "s(D10)",   6,  0},
        {TOK_D_MSB,     "s(D9)",    5,  0},
        {TOK_D_MSB,     "s(D8)",    5,  0},
        {TOK_D_MSB,     "s(D7)",    5,  0},
        {TOK_D_MSB,     "s(D6)",    5,  0},
        {TOK_D_MSB,     "s(D5)",    5,  0},
        {TOK_D_MSB,     "s(D4)",    5,  0},
        {TOK_D_MSB,     "s(D3)",    5,  0},
        {TOK_D_MSB,     "s(D2)",    5,  0},
        {TOK_D_MSB,     "s(D1)",    5,  0},
        {TOK_D_MSB,     "s(D0)",    5,  0},
        {TOK_D_MSB,     "r(D15)",   6,  0},
        {TOK_D_MSB,     "r(D14)",   6,  0},
        {TOK_D_MSB,     "r(D13)",   6,  0},
        {TOK_D_MSB,     "r(D12)",   6,  0},
        {TOK_D_MSB,     "r(D11)",   6,  0},
        {TOK_D_MSB,     "r(D10)",   6,  0},
        {TOK_D_MSB,     "r(D9)",    5,  0},
        {TOK_D_MSB,     "r(D8)",    5,  0},
        {TOK_D_MSB,     "r(D7)",    5,  0},
        {TOK_D_MSB,     "r(D6)",    5,  0},
        {TOK_D_MSB,     "r(D5)",    5,  0},
        {TOK_D_MSB,     "r(D4)",    5,  0},
        {TOK_D_MSB,     "r(D3)",    5,  0},
        {TOK_D_MSB,     "r(D2)",    5,  0},
        {TOK_D_MSB,     "r(D1)",    5,  0},
        {TOK_D_MSB,     "r(D0)",    5,  0},
        {TOK_D_PLUS,    "+D15",     4,  1},
        {TOK_D_PLUS,    "+D14",     4,  1},
        {TOK_D_PLUS,    "+D13",     4,  1},
        {TOK_D_PLUS,    "+D12",     4,  1},
        {TOK_D_PLUS,    "+D11",     4,  1},
        {TOK_D_PLUS,    "+D10",     4,  1},
        {TOK_D_PLUS,    "+D9",      3,  1},
        {TOK_D_PLUS,    "+D8",      3,  1},
        {TOK_D_PLUS,    "+D7",      3,  1},
        {TOK_D_PLUS,    "+D6",      3,  1},
        {TOK_D_PLUS,    "+D5",      3,  1},
        {TOK_D_PLUS,    "+D4",      3,  1},
        {TOK_D_PLUS,    "+D3",      3,  1},
        {TOK_D_PLUS,    "+D2",      3,  1},
        {TOK_D_PLUS,    "+D1",      3,  1},
        {TOK_D_PLUS,    "+D0",      3,  1},
        {TOK_D_MINUS,   "-D15",     4,  1},
        {TOK_D_MINUS,   "-D14",     4,  1},
        {TOK_D_MINUS,   "-D13",     4,  1},
        {TOK_D_MINUS,   "-D12",     4,  1},
        {TOK_D_MINUS,   "-D11",     4,  1},
        {TOK_D_MINUS,   "-D10",     4,  1},
        {TOK_D_MINUS,   "-D9",      3,  1},
        {TOK_D_MINUS,   "-D8",      3,  1},
        {TOK_D_MINUS,   "-D7",      3,  1},
        {TOK_D_MINUS,   "-D6",      3,  1},
        {TOK_D_MINUS,   "-D5",      3,  1},
        {TOK_D_MINUS,   "-D4",      3,  1},
        {TOK_D_MINUS,   "-D3",      3,  1},
        {TOK_D_MINUS,   "-D2",      3,  1},
        {TOK_D_MINUS,   "-D1",      3,  1},
        {TOK_D_MINUS,   "-D0",      3,  1},
        {TOK_Z_READ,    "(Z)",      3,  0},
        {TOK_Z_NOP,     "Z",        1,  1},
        {TOK_HL_READ,   "(Hl)",     4,  0},
        {TOK_HL_WRITE,  "Hl",       2,  1},
        {TOK_HU_READ,   "(Hu)",     4,  0},
        {TOK_HU_WRITE,  "Hu",       2,  1},
        {TOK_S_READ,    "(S)",      3,  0},
        {TOK_S_WRITE,   "S",        1,  1},
        {TOK_S_PLUS,    "+S",       2,  1},
        {TOK_S_SKIP,    "cS",       2,  1},
        {TOK_K,         "K",        1,  1},
        {TOK_K_PLUS,    "+K",       2,  1},
        {TOK_P11,       "p11",      3,  1},
        {TOK_P1,        "p1",       2,  1},
        {TOK_P20,       "p20",      3,  1},
        {TOK_DRUM_1,    "a",        1,  1},
        {TOK_DRUM_2,    "b",        1,  1},
        {TOK_DRUM_3,    "c",        1,  1},
        {TOK_DRUM_4,    "d",        1,  1},
        {TOK_OT,        "Ot",       2,  1},
        {TOK_OP,        "Op",       2,  1},
        {TOK_SPEAKER,   "P",        1,  1},
        {TOK_SHIFT,     "Lx",       2,  1},
        {TOK_STOP,      "T",        1,  1},
        {TOK_COMMA,     ",",        1,  0},
        {TOK_COLON,     ":",        1,  0},
        {TOK_LPAREN,    "(",        1,  0},
        {TOK_RPAREN,    ")",        1,  0},
        {TOK_EOF,       0,          0,  0},
    };
    const char *buf = tokeniser->buffer + tokeniser->posn;
    int index, len;
    for (index = 0; keywords[index].token != TOK_EOF; ++index) {
        len = keywords[index].len;
        if (!memcmp(buf, keywords[index].name, len)) {
            /* We have found a prefix match */
            if (!keywords[index].ends_in_ident ||
                    !tokeniser_is_alphanum(buf[len])) {
                /* We have found a legitimate keyword */
                tokeniser_set_token(tokeniser, keywords[index].token, len);
                if (tokeniser->token >= TOK_D_READ &&
                        tokeniser->token <= TOK_D_MINUS) {
                    /* Extract the D register number */
                    tokeniser->value = tokeniser_get_number
                        (tokeniser->name + 1);
                }
                return 1;
            }
        }
    }
    return 0;
}

static int tokeniser_get_ident_length(tokeniser_t *tokeniser, int posn)
{
    int len = 0;
    char ch;
    while ((ch = tokeniser->buffer[posn]) != '\0') {
        if (tokeniser_is_alphanum(ch)) {
            ++posn;
            ++len;
        } else {
            break;
        }
    }
    return len;
}

static int tokeniser_get_number_length(tokeniser_t *tokeniser, int posn)
{
    int len = 0;
    char ch;
    while ((ch = tokeniser->buffer[posn]) != '\0') {
        if (tokeniser_is_digit(ch)) {
            ++posn;
            ++len;
        } else {
            break;
        }
    }
    return len;
}

static void tokeniser_get_string(tokeniser_t *tokeniser)
{
    int len = 0;
    char quote = tokeniser->buffer[(tokeniser->posn)++];
    int too_long_reported = 0;
    char ch;
    for (;;) {
        ch = tokeniser->buffer[(tokeniser->posn)++];
        if (ch == quote) {
            break;
        } else if (ch == '\\') {
            ch = tokeniser->buffer[(tokeniser->posn)++];
            if (ch == '\r' || ch == '\n') {
                fprintf(stderr, "%s:%ld: unterminated string\n",
                        tokeniser->filename, tokeniser->line);
                ++(tokeniser->num_errors);
                --(tokeniser->posn);
                break;
            } else if (ch == 'n') {
                ch = '\n';
            } else if (ch == 'r') {
                ch = '\r';
            } else if (ch == 't') {
                ch = '\t';
            } else if (ch == 'v') {
                ch = '\v';
            } else if (ch == 'b') {
                ch = '\b';
            } else if (ch == 'a') {
                ch = '\a';
            } else if (ch == 'e') {
                ch = 0x1B;
            }
        } else if (ch == '\r' || ch == '\n') {
            fprintf(stderr, "%s:%ld: unterminated string\n",
                    tokeniser->filename, tokeniser->line);
            ++(tokeniser->num_errors);
            --(tokeniser->posn);
            break;
        }
        if (len < (TOKEN_NAME_MAX - 1)) {
            tokeniser->name[len++] = ch;
        } else if (!too_long_reported) {
            fprintf(stderr, "%s:%ld: string is too long, max %d characters\n",
                    tokeniser->filename, tokeniser->line, TOKEN_NAME_MAX - 1);
            ++(tokeniser->num_errors);
            too_long_reported = 1;
        }
    }
    tokeniser->name[len] = '\0';
    tokeniser->token = TOK_STRING;
}

token_t tokeniser_get_next(tokeniser_t *tokeniser)
{
    int len;
    char ch;
    while (tokeniser->token != TOK_EOF) {
        if (tokeniser->token != TOK_EOL) {
            /* Skip whitespace after the last token and see if we are at EOL */
            while ((ch = tokeniser->buffer[tokeniser->posn]) != '\0') {
                if (!tokeniser_is_space(ch)) {
                    break;
                }
                ++(tokeniser->posn);
            }
            ch = tokeniser->buffer[tokeniser->posn];
            if (ch == '\0') {
                tokeniser_set_token(tokeniser, TOK_EOL, 0);
                break;
            }

            /* Determine what kind of token we have from the first character */
            if (ch == ';') {
                /* Comment from here until the end of the line */
                tokeniser->posn = strlen(tokeniser->buffer);
                tokeniser_set_token(tokeniser, TOK_EOL, 0);
                break;
            } else if (tokeniser_match_keyword(tokeniser)) {
                /* We have matched a known keyword token */
                break;
            } else if (ch == '(') {
                /* Encountered a bare left parenthesis */
                tokeniser_set_token(tokeniser, TOK_LPAREN, 1);
                break;
            } else if (ch == '.' &&
                       tokeniser_is_alpha
                          (tokeniser->buffer[tokeniser->posn + 1])) {
                /* Encountered an assembler directive */
                len = tokeniser_get_ident_length
                        (tokeniser, tokeniser->posn + 1);
                tokeniser_set_token(tokeniser, TOK_DIRECTIVE, len + 1);
                break;
            } else if (tokeniser_is_alpha(ch)) {
                /* Encountered an identifier */
                len = tokeniser_get_ident_length(tokeniser, tokeniser->posn);
                tokeniser_set_token(tokeniser, TOK_IDENT, len);
                break;
            } else if (tokeniser_is_digit(ch)) {
                /* Encountered an unsigned decimal number */
                len = tokeniser_get_number_length(tokeniser, tokeniser->posn);
                tokeniser_set_token(tokeniser, TOK_NUMBER, len);
                tokeniser->value = tokeniser_get_number(tokeniser->name);
                break;
            } else if ((ch == '-' || ch == '+') &&
                       tokeniser_is_digit
                          (tokeniser->buffer[tokeniser->posn + 1])) {
                /* Encountered a signed decimal number */
                len = tokeniser_get_number_length
                    (tokeniser, tokeniser->posn + 1);
                tokeniser_set_token(tokeniser, TOK_NUMBER, len + 1);
                tokeniser->value = tokeniser_get_number(tokeniser->name);
                break;
            } else if (ch == '"' || ch == '\'') {
                /* Encountered a quoted string */
                tokeniser_get_string(tokeniser);
                break;
            } else {
                /* The token's leading character is not recognised */
                tokeniser_set_token(tokeniser, TOK_ERROR, 1);
                break;
            }
        } else {
            /* Read the next line from the input */
            tokeniser->posn = 0;
            if (!fgets(tokeniser->buffer, sizeof(tokeniser->buffer),
                       tokeniser->file)) {
                tokeniser_set_token(tokeniser, TOK_EOF, 0);
                break;
            }
            ++(tokeniser->line);
            tokeniser->token = TOK_ERROR; /* Until we know better */
        }
    }
    return tokeniser->token;
}
