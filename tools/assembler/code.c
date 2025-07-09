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

#include "code.h"
#include <stdlib.h>
#include <string.h>

void code_init(code_t *code, int flags)
{
    memset(code, 0, sizeof(code_t));
    code->next_function_number = 2;
    code->flags = flags;
    code->teleprinter = CSIRAC_MEDIA_TELEPRINTER;
    code->input_tape = CSIRAC_MEDIA_5HOLE_TAPE;
    code->output_punch = CSIRAC_MEDIA_5HOLE_TAPE;
    if (flags & CODE_FLAG_PRIMARY) {
        if (flags & CODE_FLAG_CONTROL) {
            /* With both primary and control, code starts at (1, 0) = 32 */
            code->address.absolute = 32;
        } else {
            /* With just the primary, code starts at (0, 14) absolute */
            code->address.address = 14;
            code->address.absolute = 14;

            /* Must not align functions when using this method as we need
             * to fall immediately out of the primary into the code. */
            code->flags &= ~CODE_FLAG_ALIGN;
        }
    } else if (flags & CODE_FLAG_CONTROL) {
        /* Cannot use the control routine without a primary routine */
        flags &= ~(CODE_FLAG_CONTROL | CODE_FLAG_ALIGN);
    } else {
        /* Alignment requires both a primary and a control */
        code->flags &= ~CODE_FLAG_ALIGN;
    }
}

void code_free(code_t *code)
{
    symbol_t *next;
    if (!code) {
        return;
    }
    while (code->symbols) {
        next = code->symbols->next;
        free(code->symbols);
        code->symbols = next;
    }
    memset(code, 0, sizeof(code_t));
}

symbol_t *code_find_symbol(code_t *code, const char *name, int create)
{
    symbol_t *symbol = code->symbols;
    symbol_t **prev = &(code->symbols);
    while (symbol) {
        if (!strcmp(symbol->name, name) && symbol->valid) {
            /* Move the symbol to the front of the list and return it.
             * Most symbol references should be to nearby labels, so this
             * should make subsequent lookups faster. */
            *prev = symbol->next;
            symbol->next = code->symbols;
            code->symbols = symbol;
            return symbol;
        }
        prev = &(symbol->next);
        symbol = symbol->next;
    }
    if (create) {
        size_t len = strlen(name);
        symbol = (symbol_t *)malloc(sizeof(symbol_t) + len);
        if (!symbol) {
            fputs("out of memory\n", stderr);
            exit(1);
        }
        symbol->next = code->symbols;
        code->symbols = symbol;
        symbol->address.address = SYMBOL_UNDEFINED;
        symbol->address.reloc_param = 0;
        symbol->address.absolute = SYMBOL_UNDEFINED;
        symbol->valid = 1;
        symbol->link_register = LINK_REG_NONE;
        strcpy(symbol->name, name);
    }
    return symbol;
}

static void code_invalidate_local_labels(code_t *code)
{
    insn_t *insn;
    unsigned index = code->num_insn;
    while (index > 0) {
        --index;
        insn = &(code->insn[index]);
        if (insn->type == INSN_FUNCTION) {
            /* Stop at the previous function heading */
            break;
        }
        if (insn->label) {
            insn->label->valid = 0;
        }
    }
}

int code_add_insn(code_t *code, const insn_t *insn)
{
    insn_t *next;
    if (code->num_insn >= MAX_INSNS) {
        return 0;
    }
    next = &(code->insn[(code->num_insn)++]);
    *next = *insn;
    switch (insn->type) {
    case INSN_REGULAR:
        /* Advance to the next instruction address */
        next->address = code->address;
        ++(code->address.address);
        ++(code->address.absolute);
        break;

    case INSN_ORIGIN:
        /* Reset the origin to an explicit absolute address */
        next->address.absolute = next->address.address;
        next->address.reloc_param = 0;
        code->address = next->address;
        break;

    case INSN_LABEL:
        /* Set the address of the label but do not advance the address */
        next->address = code->address;
        next->label->address = code->address;
        break;

    case INSN_FUNCTION:
        /* Invalidate all local labels since the previous function heading.
         * Local labels always use relocation parameter 1 but it is a
         * different parameter 1 for each function.  We don't want to
         * accidentally jump into a different function using the current
         * function's relocation parameter number. */
        if (code->flags & CODE_FLAG_CONTROL) {
            code_invalidate_local_labels(code);
        }

        /* Align the absolute address of the function if necessary */
        if (code->flags & CODE_FLAG_ALIGN) {
            if ((code->address.absolute % 32) != 0) {
                code->address.absolute += 32 - (code->address.absolute % 32);
            }
        }

        /* Advance to the next relocation range to start the new function.
         * If there is no control, then use the absolute address instead. */
#if 0 // TODO: relocations for functions
        if (code->flags & CODE_FLAG_CONTROL) {
            code->address.address = 0;
            code->address.reloc_param = (code->next_function_number)++;
        }
#endif
        next->address = code->address;
        next->label->address = code->address;

#if 0 // TODO: relocations for functions
        /* Switch to using relocation parameter 1 for the function body */
        code->address.reloc_param = 1;
#endif
        break;
    }
    return 1;
}

void code_end_function(code_t *code)
{
    code_invalidate_local_labels(code);
}

static void code_punch(FILE *file, csirac_word_t word, int punch)
{
    fprintf(file, "%2d %2d", (int)((word >> 5) & 0x1F), (int)(word & 0x1F));
    if (punch == 'X') {
        fputc('X', file);
    } else if (punch == 'Y') {
        fputc(' ', file);
        fputc('Y', file);
    }
    fputc('\n', file);
}

static void code_punch_full_word(FILE *file, csirac_word_t word)
{
    csirac_word_t high = word >> 10;
    csirac_word_t low  = word & 0x000003FF;
    if (high != 0) {
        code_punch(file, high, 0);
    }
    code_punch(file, low, 'X');
}

int code_write(code_t *code, const char *filename)
{
    FILE *file;
    unsigned index;
    const insn_t *insn;
    csirac_half_word_t shift_absolute = 0;
    csirac_half_word_t address;
    csirac_half_word_t high_word;
    int ok = 1;
    if (!strcmp(filename, "-")) {
        file = stdout;
    } else if ((file = fopen(filename, "w")) == NULL) {
        perror(filename);
        return 0;
    }
    if (code->title) {
        fprintf(file, "       * %s\n", code->title->name);
    }
    if (code->teleprinter == CSIRAC_MEDIA_FLEXOWRITER) {
        fprintf(file, "+Ot=flexowriter\n");
    } else if (code->teleprinter == CSIRAC_MEDIA_ASCII) {
        fprintf(file, "+Ot=ascii\n");
    }
    if (code->input_tape == CSIRAC_MEDIA_8HOLE_TAPE) {
        fprintf(file, "+I=tape8\n");
    } else if (code->input_tape == CSIRAC_MEDIA_8HOLE_TAPE) {
        fprintf(file, "+I=tape12\n");
    }
    if (code->output_punch == CSIRAC_MEDIA_8HOLE_TAPE) {
        fprintf(file, "+Op=punch8\n");
    } else if (code->output_punch == CSIRAC_MEDIA_8HOLE_TAPE) {
        fprintf(file, "+Op=punch12\n");
    }
    if ((code->flags & CODE_FLAG_CONTROL) != 0 &&
            code->next_function_number > 8) {
        /* We originally started placing code at (1, 0) but that will now
         * overlap with the relocation table.  Shift all addresses up. */
        int fn = code->next_function_number - 8;
        while (fn > 0) {
            shift_absolute += 32;
            fn -= 32;
        }
    }
    address = shift_absolute;
    if (code->flags & CODE_FLAG_PRIMARY) {
        /* Output the primary routine */
        for (index = 0; index < csirac_primary_routine_len; ++index) {
            code_punch(file, csirac_primary_routine[index], 0);
        }
        address += csirac_primary_routine_len;
        fputc('\n', file);

        /* Output the control routine */
        if (code->flags & CODE_FLAG_CONTROL) {
            code_punch(file, csirac_short_primary_routine_len, 'Y');
            for (index = 0; index < csirac_control_routine_len; ++index) {
                code_punch_full_word(file, csirac_control_routine[index]);
            }
            address += csirac_control_routine_len;
            fputc('\n', file);
        }
    }
    for (index = 0; index < code->num_insn; ++index) {
        insn = &(code->insn[index]);
        if (insn->address.reloc_param == 0 &&
                insn->address.absolute != address) {
            address = insn->address.absolute;
            if (code->flags & CODE_FLAG_CONTROL) {
                code_punch(file, address, 0);
                code_punch(file, 0, 'Y');
            } else {
                code_punch(file, address, 'Y');
            }
        }
        switch (insn->type) {
        case INSN_REGULAR:
            /* For label references, we need to relocate the instruction */
            high_word = insn->word >> 10;
            if (insn->reference) {
                if (insn->reference->address.absolute == SYMBOL_UNDEFINED) {
                    fprintf(stderr, "%s:%ld: label `%s' is not defined\n",
                            insn->filename->name, insn->line,
                            insn->reference->name);
                    ok = 0;
                }
                if (insn->reference->address.reloc_param != 0) {
                    /* Relocate using a relocation parameter */
                    code_punch(file, insn->reference->address.reloc_param, 0);
                    code_punch(file, 2, 'Y');
                } else {
                    /* Absolute label reference that needs to be shifted */
                    high_word +=
                        insn->reference->address.absolute + shift_absolute;
                }
            }

            /* Punch a regular instruction; high 10 bits can be omitted if 0 */
            if (high_word != 0) {
                code_punch(file, high_word, 0);
            }
            code_punch(file, insn->word & 0x000003FF, 'X');
            ++address;
            break;

        case INSN_ORIGIN:
            /* Change the origin to a new address */
            address = insn->address.address + shift_absolute;
            code_punch(file, address, 0);
            code_punch(file, 0, 'Y');
            break;

        case INSN_LABEL:
            /* Nothing to do for labels */
            break;

        case INSN_FUNCTION:
            if (code->flags & CODE_FLAG_CONTROL) {
                /* Output an explicit address if we are aligning functions */
                if (code->flags & CODE_FLAG_ALIGN) {
                    address = insn->address.address + shift_absolute;
                    code_punch(file, address, 0);
                    code_punch(file, 0, 'Y');
                }

#if 0 // TODO: relocations for functions
                /* Put the function's address into a relocation parameter */
                code_punch(file, insn->address.reloc_param, 0);
                code_punch(file, 1, 'Y');

                /* Change to relocation parameter 1 for the function body */
                code_punch(file, 1, 0);
                code_punch(file, 1, 'Y');
#endif
            }
            break;
        }
    }
    if (code->entry_point && (code->flags & CODE_FLAG_CONTROL) != 0) {
        /* Output a jump instruction to activate the entry point at runtime */
        if (code->entry_point->address.reloc_param > 1) {
            code_punch(file, code->entry_point->address.reloc_param, 0);
            code_punch(file, 2, 'Y');
        }
        code_punch(file, 6, 'Y');
        if (shift_absolute && code->entry_point->address.reloc_param == 0) {
            code_punch
                (file, code->entry_point->address.absolute + shift_absolute, 0);
        } else if (code->entry_point->address.absolute != 0) {
            code_punch(file, code->entry_point->address.absolute, 0);
        } else if (code->entry_point->address.address != 0) {
            code_punch(file, code->entry_point->address.address, 0);
        }
        code_punch(file, CSIRAC_SRC_I * 32 + CSIRAC_DEST_S, 'X');
    } else if (code->entry_point && (code->flags & CODE_FLAG_PRIMARY) != 0) {
        /* We have an entry point but no control routine to jump to it.
         * Assume that the entry point is just past the primary and
         * output a dummy control command to exit the primary. */
        code_punch(file, csirac_short_primary_routine_len, 'Y');
    }
    if (file != stdout) {
        fclose(file);
    }
    return ok;
}
