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

#include "parse.h"
#include <stdlib.h>
#include <string.h>

void parser_init(parser_t *parser, int flags)
{
    parser->tokeniser = 0;
    code_init(&(parser->code), flags);
    parser->num_errors = 0;
    parser->link_register = LINK_REG_NONE;
    parser->last_dest = CSIRAC_DEST_NOP_Z;
}

void parser_free(parser_t *parser)
{
    tokeniser_t *next;
    while (parser->tokeniser != NULL) {
        next = parser->tokeniser->next;
        tokeniser_free(parser->tokeniser);
        free(parser->tokeniser);
        parser->tokeniser = next;
    }
    code_free(&(parser->code));
}

/**
 * @brief Fetches the next token from the input file.
 *
 * @param[in,out] parser State of the parser.
 *
 * If the current file is exhausted, then this will pop out to the parent
 * file and keep parsing until all files on the stack are exhausted.
 */
static void next_token(parser_t *parser)
{
    while (parser->tokeniser) {
        token_t token = tokeniser_get_next(parser->tokeniser);
        if (token == TOK_EOF) {
            /* This tokeniser is finished, so pop the stack and try again */
            tokeniser_t *next = parser->tokeniser->next;
            tokeniser_free(parser->tokeniser);
            free(parser->tokeniser);
            parser->tokeniser = next;
        } else {
            /* We have a useful token now */
            parser->num_errors += parser->tokeniser->num_errors;
            parser->tokeniser->num_errors = 0;
            break;
        }
    }
}

/**
 * @brief Parse an old-style group literal of the form "n, m" where "n"
 * has already been parsed and we are positioned on the comma.
 *
 * @param[in,out] parser State of the parser.
 * @param[out] value Constant value associated with the source.
 */
static int parse_group_literal(parser_t *parser, csirac_word_t *value)
{
    next_token(parser);
    if (parser->tokeniser->token == TOK_NUMBER) {
        *value = (*value & 0x1F) * 32 + (parser->tokeniser->value & 0x1F);
        next_token(parser);
        return 1;
    }
    fprintf(stderr, "%s:%ld: invalid group literal\n",
            parser->tokeniser->filename, parser->tokeniser->line);
    ++(parser->num_errors);
    return 0;
}

/**
 * @brief Parses a source gate.
 *
 * @param[in,out] parser State of the parser.
 * @param[out] value Constant value associated with the source.
 * @param[out] label Label associated with the source, or NULL.
 *
 * @return The source gate number or -1 on error.
 */
static int parse_source
    (parser_t *parser, csirac_word_t *value, symbol_t **label)
{
    size_t size;
    int mem_type;
    *value = 0;
    *label = 0;
    switch (parser->tokeniser->token) {
    case TOK_IDENT:
        /* This case should have already been handled by the caller */
        break;

    case TOK_NUMBER:
        /* 10-bit numeric literal, aligned in the top 10 bits */
        *value = parser->tokeniser->value & CSIRAC_HALF_WORD_MASK;
        next_token(parser);
        if (parser->tokeniser->token == TOK_K) {
            /* Literal followed by "K" is the same as the literal */
            next_token(parser);
        } else if (parser->tokeniser->token == TOK_COMMA) {
            /* Old-style group literal of the form "n, m" */
            if (!parse_group_literal(parser, value)) {
                break;
            }
        }
        return CSIRAC_SRC_I;

    case TOK_LPAREN:
        /* Memory reference, drum reference, or (n K) constant expected */
        next_token(parser);
        if (parser->tokeniser->token == TOK_IDENT) {
            *label = code_find_symbol
                (&(parser->code), parser->tokeniser->name, 1);
            next_token(parser);
        } else if (parser->tokeniser->token == TOK_NUMBER) {
            *value = parser->tokeniser->value & CSIRAC_HALF_WORD_MASK;
            next_token(parser);
            if (parser->tokeniser->token == TOK_COMMA) {
                /* Old-style group literal of the form "n, m" */
                if (!parse_group_literal(parser, value)) {
                    break;
                }
            }
        } else {
            fprintf(stderr, "%s:%ld: memory address or constant expected\n",
                    parser->tokeniser->filename, parser->tokeniser->line);
            ++(parser->num_errors);
            next_token(parser);
            break;
        }
        if (parser->tokeniser->token == TOK_M) {
            mem_type = CSIRAC_SRC_MAIN_MEMORY;
            next_token(parser);
        } else if (parser->tokeniser->token == TOK_DRUM_1) {
            mem_type = CSIRAC_SRC_DRUM_MEMORY_1;
            next_token(parser);
        } else if (parser->tokeniser->token == TOK_DRUM_2) {
            mem_type = CSIRAC_SRC_DRUM_MEMORY_2;
            next_token(parser);
        } else if (parser->tokeniser->token == TOK_DRUM_3) {
            mem_type = CSIRAC_SRC_DRUM_MEMORY_3;
            next_token(parser);
        } else if (parser->tokeniser->token == TOK_DRUM_4) {
            mem_type = CSIRAC_SRC_DRUM_MEMORY_4;
            next_token(parser);
        } else if (parser->tokeniser->token == TOK_K) {
            mem_type = CSIRAC_SRC_I;
            next_token(parser);
        } else {
            mem_type = CSIRAC_SRC_MAIN_MEMORY;
        }
        if (parser->tokeniser->token != TOK_RPAREN) {
            fprintf(stderr, "%s:%ld: expected `)'\n",
                    parser->tokeniser->filename, parser->tokeniser->line);
            ++(parser->num_errors);
            break;
        } else {
            next_token(parser);
        }
        return mem_type;

    case TOK_STRING:
        /* String literal, which must be a single character */
        size = strlen(parser->tokeniser->name);
        if (csirac_string_unicode_length(parser->tokeniser->name, size) != 1) {
            fprintf(stderr, "%s:%ld: single-character literal expected\n",
                    parser->tokeniser->filename, parser->tokeniser->line);
            ++(parser->num_errors);
            *value = 0;
        } else {
            size_t posn = 0;
            int32_t ch;
            uint8_t buf[1];
            int flags = CSIRAC_STR_CONVERT_NO_SHIFT;
            size = strlen(parser->tokeniser->name);
            switch (parser->code.teleprinter) {
                case CSIRAC_MEDIA_TELEPRINTER:
                default:
                    /* Convert the first character into a teleprinter code */
                    if (csirac_string_to_teleprinter
                            (buf, sizeof(buf), parser->tokeniser->name,
                             size, &flags) != 1) {
                        fprintf(stderr, "%s:%ld: invalid teleprinter character\n",
                                parser->tokeniser->filename, parser->tokeniser->line);
                        ++(parser->num_errors);
                        *value = 0;
                    } else {
                        *value = buf[0];
                    }
                    break;

                case CSIRAC_MEDIA_FLEXOWRITER:
                    /* Convert the first character into a Flexowriter code */
                    if (csirac_string_to_flexowriter
                            (buf, sizeof(buf), parser->tokeniser->name,
                             size, &flags) != 1) {
                        fprintf(stderr, "%s:%ld: invalid flexowriter character\n",
                                parser->tokeniser->filename, parser->tokeniser->line);
                        ++(parser->num_errors);
                        *value = 0;
                    } else {
                        *value = buf[0];
                    }
                    break;

                case CSIRAC_MEDIA_ASCII:
                    /* Only Unicode U+0000 to U+03FF can be represented
                     * in the 10-bit literal space. */
                    ch = csirac_string_get_next_unicode
                        (parser->tokeniser->name, size, &posn);
                    if (ch < 0 || ch > 0x03FF) {
                        fprintf(stderr, "%s:%ld: invalid unicode character\n",
                                parser->tokeniser->filename, parser->tokeniser->line);
                        *value = (csirac_word_t)'?';
                    } else {
                        *value = (csirac_word_t)ch;
                    }
                    break;
            }
            next_token(parser);
            return CSIRAC_SRC_I;
        }
        break;

    case TOK_I:
        /* (I) - Read from the input tape */
        next_token(parser);
        return CSIRAC_SRC_INPUT_TAPE;

    case TOK_N1:
        /* (N1) - Read the N1 input switches */
        next_token(parser);
        return CSIRAC_SRC_N1;

    case TOK_N2:
        /* (N2) - Read the N2 input switches */
        next_token(parser);
        return CSIRAC_SRC_N2;

    case TOK_A_READ:
        /* (A) - Read the A register */
        next_token(parser);
        return CSIRAC_SRC_A;

    case TOK_A_MSB:
        /* s(A) - Read the sign bit of the A register */
        next_token(parser);
        return CSIRAC_SRC_A_MSB;

    case TOK_A_LSB:
        /* p1(A) - Read the least significant bit of the A register */
        next_token(parser);
        return CSIRAC_SRC_A_LSB;

    case TOK_A_SHR:
        /* r(A) - Shift A register right by 1 bit */
        next_token(parser);
        return CSIRAC_SRC_A_RIGHT_SHIFT;

    case TOK_A_SHL:
        /* 2(A) - Shift A register left by 1 bit */
        next_token(parser);
        return CSIRAC_SRC_A_LEFT_SHIFT;

    case TOK_A_CLEAR:
        /* c(A) - Read the A register and clear it */
        next_token(parser);
        return CSIRAC_SRC_A_CLEAR;

    case TOK_A_TEST:
        /* z(A) - Test the A register for non-zero */
        next_token(parser);
        return CSIRAC_SRC_A_TEST;

    case TOK_B_READ:
        /* (B) - Read the B register */
        next_token(parser);
        return CSIRAC_SRC_B;

    case TOK_B_MSB:
        /* (R) - Read the sign bit of the B register */
        next_token(parser);
        return CSIRAC_SRC_B_MSB;

    case TOK_B_SHR:
        /* r(B) - Shift B register right by 1 bit */
        next_token(parser);
        return CSIRAC_SRC_B_RIGHT_SHIFT;

    case TOK_C_READ:
        /* (C) - Read the C register */
        next_token(parser);
        return CSIRAC_SRC_C;

    case TOK_C_MSB:
        /* s(C) - Read the sign bit of the C register */
        next_token(parser);
        return CSIRAC_SRC_C_MSB;

    case TOK_C_SHR:
        /* r(C) - Shift C register right by 1 bit */
        next_token(parser);
        return CSIRAC_SRC_C_RIGHT_SHIFT;

    case TOK_D_READ:
        /* (Dn) - Read a D register */
        *value = parser->tokeniser->value;
        next_token(parser);
        return CSIRAC_SRC_D;

    case TOK_D_MSB:
        /* s(Dn) - Read the sign bit of a D register */
        *value = parser->tokeniser->value;
        next_token(parser);
        return CSIRAC_SRC_D_MSB;

    case TOK_D_SHR:
        /* r(Dn) - Shift a D register right by 1 bit */
        *value = parser->tokeniser->value;
        next_token(parser);
        return CSIRAC_SRC_D_RIGHT_SHIFT;

    case TOK_Z_READ:
        /* (Z) - Read the value zero */
        next_token(parser);
        return CSIRAC_SRC_ZERO;

    case TOK_HL_READ:
        /* (Hl) - Read H into the lower 10 bits of a word */
        next_token(parser);
        return CSIRAC_SRC_H_LOWER;

    case TOK_HU_READ:
        /* (Hu) - Read H into the upper 10 bits of a word */
        next_token(parser);
        return CSIRAC_SRC_H_UPPER;

    case TOK_S_READ:
        /* (S) - Read the S register */
        next_token(parser);
        return CSIRAC_SRC_S;

    case TOK_P11:
        /* p11 - Read the value with a 1 in P11 */
        next_token(parser);
        return CSIRAC_SRC_P11;

    case TOK_P1:
        /* p1 - Read the value with a 1 in P1 */
        next_token(parser);
        return CSIRAC_SRC_P1;

    case TOK_P20:
        /* p20 - Read the value with a 1 in P20 */
        next_token(parser);
        return CSIRAC_SRC_P20;

    default:
        fprintf(stderr, "%s:%ld: source operand expected\n",
                parser->tokeniser->filename, parser->tokeniser->line);
        ++(parser->num_errors);
        break;
    }
    return -1;
}

/**
 * @brief Parses a destination gate.
 *
 * @param[in,out] parser State of the parser.
 * @param[out] value Constant value associated with the destination.
 * @param[out] label Label associated with the destination, or NULL.
 *
 * @return The destination gate number or -1 on error.
 */
static int parse_destination
    (parser_t *parser, csirac_word_t *value, symbol_t **label)
{
    int mem_type;
    *value = 0;
    *label = 0;
    switch (parser->tokeniser->token) {
    case TOK_IDENT:
    case TOK_NUMBER:
        /* Label or literal for a memory or magnetic drum address */
        if (parser->tokeniser->token == TOK_IDENT) {
            *label = code_find_symbol
                (&(parser->code), parser->tokeniser->name, 1);
            next_token(parser);
        } else if (parser->tokeniser->token == TOK_NUMBER) {
            *value = parser->tokeniser->value & CSIRAC_HALF_WORD_MASK;
            next_token(parser);
            if (parser->tokeniser->token == TOK_COMMA) {
                /* Old-style group literal of the form "n, m" */
                if (!parse_group_literal(parser, value)) {
                    break;
                }
            }
        } else {
            fprintf(stderr, "%s:%ld: memory address or constant expected\n",
                    parser->tokeniser->filename, parser->tokeniser->line);
            ++(parser->num_errors);
            next_token(parser);
            break;
        }
        if (parser->tokeniser->token == TOK_M) {
            mem_type = CSIRAC_DEST_MAIN_MEMORY;
            next_token(parser);
        } else if (parser->tokeniser->token == TOK_DRUM_1) {
            mem_type = CSIRAC_DEST_DRUM_MEMORY_1;
            next_token(parser);
        } else if (parser->tokeniser->token == TOK_DRUM_2) {
            mem_type = CSIRAC_DEST_DRUM_MEMORY_2;
            next_token(parser);
        } else if (parser->tokeniser->token == TOK_DRUM_3) {
            mem_type = CSIRAC_DEST_DRUM_MEMORY_3;
            next_token(parser);
        } else if (parser->tokeniser->token == TOK_DRUM_4) {
            mem_type = CSIRAC_DEST_DRUM_MEMORY_4;
            next_token(parser);
        } else {
            mem_type = CSIRAC_DEST_MAIN_MEMORY;
        }
        return mem_type;

    case TOK_I:
        /* I - No operation */
        next_token(parser);
        return CSIRAC_DEST_NOP_I;

    case TOK_OT:
        /* Ot - Output to the teleprinter */
        next_token(parser);
        return CSIRAC_DEST_TELEPRINTER;

    case TOK_OP:
        /* Op - Output to punch tape */
        next_token(parser);
        return CSIRAC_DEST_PUNCH_TAPE;

    case TOK_A_WRITE:
        /* A - Write to the A register */
        next_token(parser);
        return CSIRAC_DEST_A;

    case TOK_A_PLUS:
        /* +A - Add to the A register */
        next_token(parser);
        return CSIRAC_DEST_A_PLUS;

    case TOK_A_MINUS:
        /* -A - Subtract from the A register */
        next_token(parser);
        return CSIRAC_DEST_A_MINUS;

    case TOK_A_AND:
        /* .A - AND with the A register */
        next_token(parser);
        return CSIRAC_DEST_A_AND;

    case TOK_A_OR:
        /* vA - OR with the A register */
        next_token(parser);
        return CSIRAC_DEST_A_OR;

    case TOK_A_XOR:
        /* ~A - XOR with the A register */
        next_token(parser);
        return CSIRAC_DEST_A_XOR;

    case TOK_SPEAKER:
        /* P - Write to the loudspeaker */
        next_token(parser);
        return CSIRAC_DEST_LOUDSPEAKER;

    case TOK_B_WRITE:
        /* B - Write to the B register */
        next_token(parser);
        return CSIRAC_DEST_B;

    case TOK_B_MUL:
        /* xB - Multiplication */
        next_token(parser);
        return CSIRAC_DEST_B_TIMES;

    case TOK_SHIFT:
        /* Lx - Shift the AB register pair */
        next_token(parser);
        return CSIRAC_DEST_CYCLIC_SHIFT;

    case TOK_C_WRITE:
        /* C - Write to the C register */
        next_token(parser);
        return CSIRAC_DEST_C;

    case TOK_C_PLUS:
        /* +C - Add to the C register */
        next_token(parser);
        return CSIRAC_DEST_C_PLUS;

    case TOK_C_MINUS:
        /* -C - Subtract from the C register */
        next_token(parser);
        return CSIRAC_DEST_C_MINUS;

    case TOK_D_WRITE:
        /* Dn - Write to a D register */
        *value = parser->tokeniser->value;
        next_token(parser);
        return CSIRAC_DEST_D;

    case TOK_D_PLUS:
        /* +Dn - Add to a D register */
        *value = parser->tokeniser->value;
        next_token(parser);
        return CSIRAC_DEST_D_PLUS;

    case TOK_D_MINUS:
        /* -D - Subtract from a D register */
        *value = parser->tokeniser->value;
        next_token(parser);
        return CSIRAC_DEST_D_MINUS;

    case TOK_Z_NOP:
        /* Z - No operation */
        next_token(parser);
        return CSIRAC_DEST_NOP_Z;

    case TOK_HL_WRITE:
        /* Hl - Write the lower 10 bits of a word into H */
        next_token(parser);
        return CSIRAC_DEST_H_LOWER;

    case TOK_HU_WRITE:
        /* Hu - Write the upper 10 bits of a word into H */
        next_token(parser);
        return CSIRAC_DEST_H_UPPER;

    case TOK_S_WRITE:
        /* S - Write to the S register */
        next_token(parser);
        return CSIRAC_DEST_S;

    case TOK_S_PLUS:
        /* +S - Add to the S register */
        next_token(parser);
        return CSIRAC_DEST_S_PLUS;

    case TOK_S_SKIP:
        /* cS - Skip the next instruction if non-zero */
        next_token(parser);
        return CSIRAC_DEST_S_SKIP;

    case TOK_K_PLUS:
        /* +K - Add to the next instruction */
        next_token(parser);
        return CSIRAC_DEST_INSTRUCTION_ADD;

    case TOK_STOP:
        /* T - Halt the machine if non-zero */
        next_token(parser);
        return CSIRAC_DEST_STOP;

    default:
        fprintf(stderr, "%s:%ld: destination operand expected\n",
                parser->tokeniser->filename, parser->tokeniser->line);
        ++(parser->num_errors);
        break;
    }
    return -1;
}

/**
 * @brief Skip to the end of the current line.
 *
 * @param[in,out] parser State of the parser.
 */
static void parse_skip_to_eol(parser_t *parser)
{
    token_t token = parser->tokeniser->token;
    while (token != TOK_EOF && token != TOK_EOL) {
        next_token(parser);
        token = parser->tokeniser->token;
    }
}

/**
 * @brief Skip the rest of the current line.
 *
 * @param[in,out] parser State of the parser.
 */
static void parse_skip_line(parser_t *parser)
{
    parse_skip_to_eol(parser);
    if (parser->tokeniser->token == TOK_EOL) {
        next_token(parser);
    }
}

/**
 * @brief Add an instruction to the code block under construction.
 *
 * @param[in,out] parser State of the parser.
 * @param[in] insn The instruction to add.
 */
static void add_insn(parser_t *parser, const insn_t *insn)
{
    if (insn->type == INSN_REGULAR) {
        parser->last_dest = insn->word & 0x1F;
    }
    if (!code_add_insn(&(parser->code), insn)) {
        fprintf(stderr, "%s:%ld: too many instructions\n",
                parser->tokeniser->filename, parser->tokeniser->line);
        ++(parser->num_errors);
    }
}

/**
 * @brief Parses an entry point directive from the assembly code input.
 *
 * @param[in,out] parser State of the parser.
 */
static void parse_directive_entry_point(parser_t *parser)
{
    if (parser->tokeniser->token == TOK_IDENT) {
        const char *name = parser->tokeniser->name;
        symbol_t *symbol = code_find_symbol(&(parser->code), name, 0);
        if (symbol != 0 && symbol->address.address != SYMBOL_UNDEFINED) {
            if (parser->code.entry_point) {
                fprintf(stderr, "%s:%ld: entry point was already defined\n",
                        parser->tokeniser->filename, parser->tokeniser->line);
                ++(parser->num_errors);
            } else {
                parser->code.entry_point = symbol;
            }
        } else {
            fprintf(stderr, "%s:%ld: entry point label `%s' is not defined\n",
                    parser->tokeniser->filename, parser->tokeniser->line,
                    name);
            ++(parser->num_errors);
        }
        next_token(parser);
    } else {
        fprintf(stderr, "%s:%ld: entry point label expected\n",
                parser->tokeniser->filename, parser->tokeniser->line);
        ++(parser->num_errors);
    }
}

/* Packing types to use with data directives */
#define PACKING_UPPER_HALVES    0
#define PACKING_FULL_WORDS      1
#define PACKING_HALF_WORDS      2
#define PACKING_QUARTER_WORDS   3

/**
 * @brief Adds a data item.
 *
 * @param[in,out] parser State of the parser.
 * @param[in] value The value to add.
 * @param[in] packing Packing type to use in the output words.
 * @param[in,out] subword Subword index for packing multiple data items
 * into the same data word.
 */
static void parse_add_data_item
    (parser_t *parser, csirac_word_t value, int packing, int *subword)
{
    insn_t *insn = &(parser->insn);
    switch (packing) {
    case PACKING_UPPER_HALVES:
        insn->word = ((value & CSIRAC_HALF_WORD_MASK) << 10);
        add_insn(parser, insn);
        break;

    case PACKING_FULL_WORDS:
        insn->word = (value & CSIRAC_WORD_MASK);
        add_insn(parser, insn);
        break;

    case PACKING_HALF_WORDS:
        if (*subword) {
            insn->word |= (value & CSIRAC_HALF_WORD_MASK);
            add_insn(parser, insn);
        } else {
            insn->word = ((value & CSIRAC_HALF_WORD_MASK) << 10);
        }
        *subword = !(*subword);
        break;

    case PACKING_QUARTER_WORDS:
        if (*subword == 3) {
            insn->word |= (value & 0x1F);
            add_insn(parser, insn);
            *subword = 0;
        } else if (*subword == 2) {
            insn->word |= (value & 0x1F) << 5;
            *subword = 3;
        } else if (*subword == 1) {
            insn->word |= (value & 0x1F) << 10;
            *subword = 2;
        } else {
            insn->word = (value & 0x1F) << 15;
            *subword = 0;
        }
        break;
    }
}

/**
 * @brief Parses a single data item from the assembly code input.
 *
 * @param[in,out] parser State of the parser.
 * @param[in] packing Packing type to use in the output words.
 * @param[in,out] subword Subword index for packing multiple data items
 * into the same data word.
 *
 * @return Non-zero on success, zero if a parse error occurred.
 */
static int parse_directive_data_item
    (parser_t *parser, int packing, int *subword)
{
    switch (parser->tokeniser->token) {
    case TOK_IDENT:
        /* Labels can only be used in ".dd" and ".dw" directives */
        if (packing == PACKING_UPPER_HALVES || packing == PACKING_FULL_WORDS) {
            insn_t insn = parser->insn;
            insn.word = 0;
            insn.reference =
                code_find_symbol(&(parser->code), parser->tokeniser->name, 1);
            add_insn(parser, &insn);
            insn.reference = 0;
        } else {
            fprintf(stderr, "%s:%ld: label not permitted here\n",
                    parser->tokeniser->filename, parser->tokeniser->line);
            ++(parser->num_errors);
        }
        break;

    case TOK_NUMBER:
        /* Simple numeric quantity */
        parse_add_data_item(parser, parser->tokeniser->value, packing, subword);
        break;

    case TOK_STRING: {
        /* String that needs to be broken up into individual character codes */
        size_t size = strlen(parser->tokeniser->name);
        uint8_t out[TOKEN_NAME_MAX * 2];
        int out_size, posn;
        int flags = 0;
        if (parser->code.teleprinter == CSIRAC_MEDIA_ASCII) {
            memcpy(out, parser->tokeniser->name, size);
            out_size = (int)size;
        } else if (parser->code.teleprinter == CSIRAC_MEDIA_FLEXOWRITER) {
            out_size = csirac_string_to_flexowriter
                (out, sizeof(out), parser->tokeniser->name, size, &flags);
        } else {
            out_size = csirac_string_to_teleprinter
                (out, sizeof(out), parser->tokeniser->name, size, &flags);
        }
        if (out_size >= 0) {
            for (posn = 0; posn < out_size; ++posn) {
                parse_add_data_item(parser, out[posn], packing, subword);
            }
        } else {
            fprintf(stderr, "%s:%ld: could not convert string\n",
                parser->tokeniser->filename, parser->tokeniser->line);
            ++(parser->num_errors);
        }
        break; }

    default:
        fprintf(stderr, "%s:%ld: `%s' is not permitted in a data directive\n",
                parser->tokeniser->filename, parser->tokeniser->line,
                parser->tokeniser->name);
        ++(parser->num_errors);
        parse_skip_to_eol(parser);
        return 0;
    }
    next_token(parser);
    return 1;
}

/**
 * @brief Parses a data directive from the assembly code input.
 *
 * @param[in,out] parser State of the parser.
 * @param[in] packing Packing type to use in the output words.
 */
static void parse_directive_data(parser_t *parser, int packing)
{
    int subword = 0;
    parser->insn.type = INSN_REGULAR;
    if (!parse_directive_data_item(parser, packing, &subword)) {
        return;
    }
    while (parser->tokeniser->token == TOK_COMMA) {
        next_token(parser);
        if (!parse_directive_data_item(parser, packing, &subword)) {
            return;
        }
    }
    if (subword != 0) {
        /* The last word was partial.  Add it to the code with zero padding. */
        add_insn(parser, &(parser->insn));
    }
}

/**
 * @brief Parses a ".return" directive from the assembly code input.
 *
 * @param[in,out] parser State of the parser.
 */
static void parse_directive_return(parser_t *parser)
{
    /* Must be within a function definition */
    if (parser->link_register == LINK_REG_NONE) {
        fprintf(stderr, "%s:%ld: return outside function definition\n",
                parser->tokeniser->filename, parser->tokeniser->line);
        ++(parser->num_errors);
        return;
    }

    /* Output a "linkreg -> S" instruction */
    parser->insn.type = INSN_REGULAR;
    parser->insn.word = CSIRAC_DEST_S;
    switch (parser->link_register) {
    case LINK_REG_D0:
    case LINK_REG_D1:
    case LINK_REG_D2:
    case LINK_REG_D3:
    case LINK_REG_D4:
    case LINK_REG_D5:
    case LINK_REG_D6:
    case LINK_REG_D7:
    case LINK_REG_D8:
    case LINK_REG_D9:
    case LINK_REG_D10:
    case LINK_REG_D11:
    case LINK_REG_D12:
    case LINK_REG_D13:
    case LINK_REG_D14:
    case LINK_REG_D15:
        parser->insn.word |= ((csirac_word_t)CSIRAC_SRC_D) << 5;
        parser->insn.word |=
            ((csirac_word_t)(parser->link_register - LINK_REG_D0)) << 10;
        break;

    case LINK_REG_A:
        parser->insn.word |= ((csirac_word_t)CSIRAC_SRC_A) << 5;
        break;

    case LINK_REG_C:
        parser->insn.word |= ((csirac_word_t)CSIRAC_SRC_C) << 5;
        break;

    case LINK_REG_NONE:
        break;
    }
    add_insn(parser, &(parser->insn));
}

/**
 * @brief Parses a ".function" directive from the assembly code input.
 *
 * @param[in,out] parser State of the parser.
 */
static void parse_directive_function(parser_t *parser)
{
    symbol_t *symbol;
    insn_t insn;

    /* Nested functions are not supported */
    if (parser->link_register != LINK_REG_NONE) {
        fprintf(stderr, "%s:%ld: nested function definitions are not supported\n",
                parser->tokeniser->filename, parser->tokeniser->line);
        ++(parser->num_errors);
        parse_skip_to_eol(parser);
        return;
    }

    /* Need a label for the function name */
    if (parser->tokeniser->token != TOK_IDENT) {
        fprintf(stderr, "%s:%ld: function name expected\n",
                parser->tokeniser->filename, parser->tokeniser->line);
        ++(parser->num_errors);
        parse_skip_to_eol(parser);
        return;
    }
    symbol = code_find_symbol(&(parser->code), parser->tokeniser->name, 1);
    if (symbol->address.address != SYMBOL_UNDEFINED) {
        fprintf(stderr, "%s:%ld: symbol `%s' is already defined\n",
                parser->tokeniser->filename, parser->tokeniser->line,
                parser->tokeniser->name);
        ++(parser->num_errors);
        parse_skip_to_eol(parser);
        return;
    }
    next_token(parser);

    /* We expect a comma and then a link register name */
    if (parser->tokeniser->token != TOK_COMMA) {
        fprintf(stderr, "%s:%ld: comma expected\n",
                parser->tokeniser->filename, parser->tokeniser->line);
        ++(parser->num_errors);
        parse_skip_to_eol(parser);
        return;
    }
    next_token(parser);
    switch (parser->tokeniser->token) {
    case TOK_D_WRITE:
        symbol->link_register =
            (link_register_t)(LINK_REG_D0 + parser->tokeniser->value);
        break;

    case TOK_A_WRITE:
        symbol->link_register = LINK_REG_A;
        break;

    case TOK_C_WRITE:
        symbol->link_register = LINK_REG_C;
        break;

    default:
        fprintf(stderr, "%s:%ld: link register name expected\n",
                parser->tokeniser->filename, parser->tokeniser->line);
        ++(parser->num_errors);
        parse_skip_to_eol(parser);
        return;
    }
    parser->link_register = symbol->link_register;

    /* Define the label for the entry point to the function */
    insn = parser->insn;
    insn.type = INSN_FUNCTION;
    insn.label = symbol;
    add_insn(parser, &insn);

    /* Add 1 to the link register to set up the correct return address */
    insn = parser->insn;
    insn.type = INSN_REGULAR;
    insn.label = 0;
    insn.word = ((csirac_word_t)CSIRAC_SRC_P11) << 5;
    switch (parser->tokeniser->token) {
    case TOK_D_WRITE:
        insn.word |= CSIRAC_DEST_D_PLUS;
        insn.word |= ((csirac_word_t)(parser->tokeniser->value)) << 10;
        break;

    case TOK_A_WRITE:
        insn.word |= CSIRAC_DEST_A_PLUS;
        break;

    case TOK_C_WRITE:
        insn.word |= CSIRAC_DEST_C_PLUS;
        break;

    default: break;
    }
    add_insn(parser, &insn);

    /* Skip the link register name */
    next_token(parser);
}

/**
 * @brief Parses an ".endfunction" directive from the assembly code input.
 *
 * @param[in,out] parser State of the parser.
 */
static void parse_directive_end_function(parser_t *parser)
{
    if (parser->link_register != LINK_REG_NONE) {
        if (parser->last_dest != CSIRAC_DEST_S) {
            /* Add an implicit ".return" instruction at the end */
            parse_directive_return(parser);
        }
        code_end_function(&(parser->code));
        parser->link_register = LINK_REG_NONE;
    } else {
        fprintf(stderr, "%s:%ld: not currently in a function definition\n",
                parser->tokeniser->filename, parser->tokeniser->line);
        ++(parser->num_errors);
    }
}

/**
 * @brief Parses a ".call" directive from the assembly code input.
 *
 * @param[in,out] parser State of the parser.
 */
static void parse_directive_call(parser_t *parser)
{
    link_register_t link_register;
    symbol_t *symbol;
    insn_t insn;

    /* Need an identifier for the function name */
    if (parser->tokeniser->token != TOK_IDENT) {
        fprintf(stderr, "%s:%ld: function name expected\n",
                parser->tokeniser->filename, parser->tokeniser->line);
        ++(parser->num_errors);
        parse_skip_to_eol(parser);
        return;
    }

    /* Look up the symbol and find the default link register */
    symbol = code_find_symbol(&(parser->code), parser->tokeniser->name, 1);
    link_register = symbol->link_register;
    next_token(parser);

    /* Do we have an explicit link register specified? */
    if (parser->tokeniser->token == TOK_COMMA) {
        next_token(parser);
    }

    /* We should have a link register by now */
    if (link_register == LINK_REG_NONE) {
        fprintf(stderr, "%s:%ld: unknown link register for function `%s'\n",
                parser->tokeniser->filename, parser->tokeniser->line,
                symbol->name);
        ++(parser->num_errors);
        parse_skip_to_eol(parser);
        return;
    }

    /* Output the "(S) -> linkreg" instruction for the call */
    insn = parser->insn;
    insn.type = INSN_REGULAR;
    insn.word = ((csirac_word_t)CSIRAC_SRC_S) << 5;
    switch (link_register) {
    case LINK_REG_D0:
    case LINK_REG_D1:
    case LINK_REG_D2:
    case LINK_REG_D3:
    case LINK_REG_D4:
    case LINK_REG_D5:
    case LINK_REG_D6:
    case LINK_REG_D7:
    case LINK_REG_D8:
    case LINK_REG_D9:
    case LINK_REG_D10:
    case LINK_REG_D11:
    case LINK_REG_D12:
    case LINK_REG_D13:
    case LINK_REG_D14:
    case LINK_REG_D15:
        insn.word |= ((csirac_word_t)CSIRAC_DEST_D);
        insn.word |= ((csirac_word_t)(link_register - LINK_REG_D0)) << 10;
        break;

    case LINK_REG_A:
        insn.word |= ((csirac_word_t)CSIRAC_DEST_A);
        break;

    case LINK_REG_C:
        insn.word |= ((csirac_word_t)CSIRAC_DEST_C);
        break;

    case LINK_REG_NONE:
        break;
    }
    add_insn(parser, &insn);

    /* Output the "label -> S" instruction for the call */
    insn = parser->insn;
    insn.type = INSN_REGULAR;
    insn.word = ((csirac_word_t)CSIRAC_SRC_I) << 5;
    insn.word |= ((csirac_word_t)CSIRAC_DEST_S);
    insn.reference = symbol;
    add_insn(parser, &insn);
}

/**
 * @brief Parses a ".title" directive from the assembly code input.
 *
 * @param[in,out] parser State of the parser.
 */
static void parse_directive_title(parser_t *parser)
{
    /* We expect a string to appear after ".title" */
    if (parser->tokeniser->token != TOK_STRING) {
        fprintf(stderr, "%s:%ld: title string expected\n",
                parser->tokeniser->filename, parser->tokeniser->line);
        ++(parser->num_errors);
        parse_skip_to_eol(parser);
        return;
    }

    /* Set the title */
    parser->code.title = code_find_symbol
        (&(parser->code), parser->tokeniser->name, 1);
    next_token(parser);
}

/**
 * @brief Parses a directive from the assembly code input.
 *
 * @param[in,out] parser State of the parser.
 * @param[in] name Name of the directive, which has already been skipped.
 */
static void parse_directive(parser_t *parser, const char *name)
{
    if (!strcmp(name, ".entry")) {
        /* Set the entry point for the program */
        parse_directive_entry_point(parser);
    } else if (!strcmp(name, ".ascii") || !strcmp(name, ".utf8")) {
        /* Switch to ASCII as the character encoding */
        parser->code.teleprinter = CSIRAC_MEDIA_ASCII;
    } else if (!strcmp(name, ".teleprinter")) {
        /* Switch to the old-style teleprinter as the character encoding */
        parser->code.teleprinter = CSIRAC_MEDIA_TELEPRINTER;
    } else if (!strcmp(name, ".flexowriter")) {
        /* Switch to the Flexowriter as the character encoding */
        parser->code.teleprinter = CSIRAC_MEDIA_FLEXOWRITER;
    } else if (!strcmp(name, ".align")) {
        /* Enable alignment for future function definitions.  Alignment
         * is not possible if don't have both a primary and a control. */
        if ((parser->code.flags & (CODE_FLAG_PRIMARY | CODE_FLAG_CONTROL))
                == (CODE_FLAG_PRIMARY | CODE_FLAG_CONTROL)) {
            parser->code.flags |= CODE_FLAG_ALIGN;
        }
    } else if (!strcmp(name, ".noalign")) {
        /* Disable alignment of function definitions */
        parser->code.flags &= ~CODE_FLAG_ALIGN;
    } else if (!strcmp(name, ".dd")) {
        /* Output 10-bit values in the upper half of 20-bit words */
        parse_directive_data(parser, PACKING_UPPER_HALVES);
    } else if (!strcmp(name, ".dw")) {
        /* Output 20-bit word values */
        parse_directive_data(parser, PACKING_FULL_WORDS);
    } else if (!strcmp(name, ".dh")) {
        /* Output 10-bit half-word values */
        parse_directive_data(parser, PACKING_HALF_WORDS);
    } else if (!strcmp(name, ".dg")) {
        /* Output 5-bit group values */
        parse_directive_data(parser, PACKING_QUARTER_WORDS);
    } else if (!strcmp(name, ".function")) {
        /* Define a new function */
        parse_directive_function(parser);
    } else if (!strcmp(name, ".endfunction")) {
        /* End of the current function definition */
        parse_directive_end_function(parser);
    } else if (!strcmp(name, ".call")) {
        /* Call a function */
        parse_directive_call(parser);
    } else if (!strcmp(name, ".return")) {
        /* Return from the current function */
        parse_directive_return(parser);
    } else if (!strcmp(name, ".include")) {
        /* Include the contents of another file here */
        // TODO
    } else if (!strcmp(name, ".punch5")) {
        /* Set the output punch to 5-hole punch tape */
        parser->code.output_punch = CSIRAC_MEDIA_5HOLE_TAPE;
    } else if (!strcmp(name, ".punch8")) {
        /* Set the output punch to 8-hole punch tape */
        parser->code.output_punch = CSIRAC_MEDIA_8HOLE_TAPE;
    } else if (!strcmp(name, ".punch12")) {
        /* Set the output punch to 12-hole punch tape */
        parser->code.output_punch = CSIRAC_MEDIA_12HOLE_TAPE;
    } else if (!strcmp(name, ".tape5")) {
        /* Set the input tape to 5-hole punch tape */
        parser->code.input_tape = CSIRAC_MEDIA_5HOLE_TAPE;
    } else if (!strcmp(name, ".tape8")) {
        /* Set the input tape to 8-hole punch tape */
        parser->code.input_tape = CSIRAC_MEDIA_8HOLE_TAPE;
    } else if (!strcmp(name, ".tape12")) {
        /* Set the input tape to 12-hole punch tape */
        parser->code.input_tape = CSIRAC_MEDIA_12HOLE_TAPE;
    } else if (!strcmp(name, ".title")) {
        /* Set the title for the tape */
        parse_directive_title(parser);
    } else {
        fprintf(stderr, "%s:%ld: unknown directive `%s'\n",
                parser->tokeniser->filename, parser->tokeniser->line, name);
        ++(parser->num_errors);
        parse_skip_to_eol(parser);
    }
}

/**
 * @brief Parses an instruction from the assembly code input.
 *
 * @param[in,out] parser State of the parser.
 * @param[in] name Name of an identifier that has already been read, or NULL.
 * @param[in] insn Instruction object to be populated.
 *
 * @return Non-zero if @a insn has been populated, or zero if no
 * new instruction.
 */
static int parse_instruction(parser_t *parser, insn_t *insn, const char *name)
{
    symbol_t *src_label = 0;
    symbol_t *dest_label = 0;
    symbol_t *label = 0;
    csirac_word_t src_value = 0;
    csirac_word_t dest_value = 0;
    csirac_word_t value = 0;
    int src, dest;
    int ok = 1;
    int invalid = 0;

    /* Parse the source of the instruction */
    if (name) {
        /* Identifier that was already recognised - this is a constant */
        src_label = code_find_symbol(&(parser->code), name, 1);
        src = CSIRAC_SRC_I;
    } else {
        src = parse_source(parser, &src_value, &src_label);
        if (src == CSIRAC_SRC_I) {
            /* Check for some well-defined constants and replace them with
             * special instructions.  This allows D-register operations
             * like "1 -> +D15" to work even though technically invalid. */
            if (src_value == 0) {
                src = CSIRAC_SRC_ZERO;
            } else if (src_value == 1) {
                src_value = 0;
                src = CSIRAC_SRC_P11;
            } else if (src_value == 512) {
                src_value = 0;
                src = CSIRAC_SRC_P20;
            }
        }
    }
    if (src < 0) {
        /* Invalid source encountered.  The error should already have been
         * reported so fall back to a default source instead. */
        src = CSIRAC_SRC_P20;
        ok = 0;
    }

    /* Parse the "->" move operator */
    if (parser->tokeniser->token != TOK_MOVE) {
        fprintf(stderr, "%s:%ld: expected `->` operator\n",
                parser->tokeniser->filename, parser->tokeniser->line);
        ++(parser->num_errors);
        ok = 0;
    } else {
        next_token(parser);
    }

    /* Parse the destination of the instruction */
    dest = parse_destination(parser, &dest_value, &dest_label);
    if (dest < 0) {
        /* Invalid destination encountered.  The error should already have
         * been reported so fall back to a default source instead. */
        dest = CSIRAC_DEST_NOP_I;
        ok = 0;
    }

    /* Figure out where the constant value / label goes (src or dest).
     * Also check for invalid combinations of source and destination. */
    label = src_label;
    value = src_value;
    if (src == CSIRAC_SRC_MAIN_MEMORY ||
            src == CSIRAC_SRC_DRUM_MEMORY_1 ||
            src == CSIRAC_SRC_DRUM_MEMORY_2 ||
            src == CSIRAC_SRC_DRUM_MEMORY_3 ||
            src == CSIRAC_SRC_DRUM_MEMORY_4 ||
            src == CSIRAC_SRC_I) {
        /* Source involves a label or constant of some kind */
        if (dest == CSIRAC_DEST_MAIN_MEMORY ||
                dest == CSIRAC_DEST_DRUM_MEMORY_1 ||
                dest == CSIRAC_DEST_DRUM_MEMORY_2 ||
                dest == CSIRAC_DEST_DRUM_MEMORY_3 ||
                dest == CSIRAC_DEST_DRUM_MEMORY_4) {
            /* If the destination is memory, it must have the same address. */
            if (src_label != 0 || dest_label != 0 || src_value != dest_value) {
                invalid = 1;
            }
        } else if (dest == CSIRAC_DEST_D ||
                   dest == CSIRAC_DEST_D_PLUS ||
                   dest == CSIRAC_DEST_D_MINUS) {
            /* If the destination is a D register, then the low 4 bits of the
             * memory address must match the D register number. */
            if (src_label != 0 || (src_value & 0x0F) != dest_value) {
                invalid = 1;
            }
        }
    } else if (src == CSIRAC_SRC_D ||
               src == CSIRAC_SRC_D_MSB ||
               src == CSIRAC_SRC_D_RIGHT_SHIFT) {
        /* Source involves a D register */
        if (dest == CSIRAC_DEST_MAIN_MEMORY ||
                dest == CSIRAC_DEST_DRUM_MEMORY_1 ||
                dest == CSIRAC_DEST_DRUM_MEMORY_2 ||
                dest == CSIRAC_DEST_DRUM_MEMORY_3 ||
                dest == CSIRAC_DEST_DRUM_MEMORY_4) {
            /* If the destination is memory, it must have the same low
             * 4 bits as the D register number. */
            if (dest_label != 0 || src_value != (dest_value & 0x0F)) {
                invalid = 1;
            }
            label = dest_label;
            value = dest_value;
        } else if (dest == CSIRAC_DEST_D ||
                   dest == CSIRAC_DEST_D_PLUS ||
                   dest == CSIRAC_DEST_D_MINUS) {
            /* If the destination is a D register, then it must be the
             * same D register as in the source. */
            if (src_value != dest_value) {
                invalid = 1;
            }
        }
    } else if (dest == CSIRAC_DEST_MAIN_MEMORY ||
               dest == CSIRAC_DEST_DRUM_MEMORY_1 ||
               dest == CSIRAC_DEST_DRUM_MEMORY_2 ||
               dest == CSIRAC_DEST_DRUM_MEMORY_3 ||
               dest == CSIRAC_DEST_DRUM_MEMORY_4) {
        /* Destination is a memory address */
        label = dest_label;
        value = dest_value;
    } else if (dest == CSIRAC_DEST_D ||
               dest == CSIRAC_DEST_D_PLUS ||
               dest == CSIRAC_DEST_D_MINUS) {
        /* Destination is a D register */
        label = dest_label;
        value = dest_value;
    }
    if (invalid) {
        fprintf(stderr, "%s:%ld: invalid combination of source and destination\n",
                parser->tokeniser->filename, parser->tokeniser->line);
        ++(parser->num_errors);
        ok = 0;
    }

    /* Construct the final instruction word */
    insn->type = INSN_REGULAR;
    insn->reference = label;
    insn->word = (value << 10) | (src << 5) | dest;
    return ok;
}

/**
 * @brief Parses a single line from the assembly code input.
 *
 * @param[in,out] parser State of the parser.
 */
static void parse_line(parser_t *parser)
{
    token_t token = parser->tokeniser->token;
    char name[TOKEN_NAME_MAX];
    symbol_t *symbol;
    insn_t *insn = &(parser->insn);

    /* Handle the easy whitespace cases */
    if (token == TOK_EOF) {
        /* End of the file */
        return;
    } else if (token == TOK_EOL) {
        /* End of a blank or comment line */
        next_token(parser);
        return;
    }

    /* Initialise an instruction object */
    memset(insn, 0, sizeof(insn_t));
    insn->filename = code_find_symbol
        (&(parser->code), parser->tokeniser->filename, 1);
    insn->line = parser->tokeniser->line;

    /* Do we have a directive, label, or instruction? */
    if (token == TOK_DIRECTIVE) {
        /* Parse the directive */
        memcpy(name, parser->tokeniser->name, TOKEN_NAME_MAX);
        next_token(parser);
        parse_directive(parser, name);
    } else if (token == TOK_IDENT) {
        /* Identifiers on the start of the line may be a label or constant.
         * Look ahead to know what we are dealing with. */
        memcpy(name, parser->tokeniser->name, TOKEN_NAME_MAX);
        next_token(parser);
        token = parser->tokeniser->token;
        if (token == TOK_COLON) {
            /* We have a label */
            next_token(parser);
            symbol = code_find_symbol(&(parser->code), name, 1);
            if (symbol->address.address != SYMBOL_UNDEFINED) {
                fprintf(stderr, "%s:%ld: symbol `%s' is already defined\n",
                        parser->tokeniser->filename, parser->tokeniser->line,
                        parser->tokeniser->name);
                ++(parser->num_errors);
            } else {
                insn->type = INSN_LABEL;
                insn->label = symbol;
                add_insn(parser, insn);
            }
        } else {
            /* Ordinary instruction that starts with an identifier */
            if (parse_instruction(parser, insn, name)) {
                add_insn(parser, insn);
            }
        }
    } else {
        /* For everything else, we expect the source for an instruction */
        if (parse_instruction(parser, insn, 0)) {
            add_insn(parser, insn);
        }
    }

    /* We should be at the end of the line (or file) by now */
    token = parser->tokeniser->token;
    if (token == TOK_EOF) {
        return;
    } else if (token != TOK_EOL) {
        /* Print an error and then skip to the end of the line */
        fprintf(stderr, "%s:%ld: unexpected `%s'\n",
                parser->tokeniser->filename, parser->tokeniser->line,
                parser->tokeniser->name);
        ++(parser->num_errors);
        parse_skip_line(parser);
    } else {
        next_token(parser);
    }
}

int parser_parse(parser_t *parser, const char *filename)
{
    tokeniser_t *tokeniser = (tokeniser_t *)malloc(sizeof(tokeniser_t));
    if (!tokeniser) {
        fputs("out of memory\n", stderr);
        return 0;
    }
    if (!tokeniser_init(tokeniser, filename)) {
        perror(filename);
        return 0;
    }
    tokeniser->next = parser->tokeniser;
    parser->tokeniser = tokeniser;
    next_token(parser);
    while (parser->tokeniser) {
        parse_line(parser);
    }
    return parser->num_errors == 0;
}
