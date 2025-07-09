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

#ifndef CODE_H
#define CODE_H

#include <libcsirac/csirac.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of instructions, which is greater than the
 * main memory size to allow for labels and directives to appear.
 */
#define MAX_INSNS (CSIRAC_MAIN_MEMORY_SIZE * 4)

/**
 * @brief Special address value for an undefined symbol.
 */
#define SYMBOL_UNDEFINED ((csirac_half_word_t)(~0))

/**
 * @brief Name of a link register.
 */
typedef enum
{
    LINK_REG_NONE,          /**< No link register (not in a function) */
    LINK_REG_D0,            /**< D0 */
    LINK_REG_D1,            /**< D1 */
    LINK_REG_D2,            /**< D2 */
    LINK_REG_D3,            /**< D3 */
    LINK_REG_D4,            /**< D4 */
    LINK_REG_D5,            /**< D5 */
    LINK_REG_D6,            /**< D6 */
    LINK_REG_D7,            /**< D7 */
    LINK_REG_D8,            /**< D8 */
    LINK_REG_D9,            /**< D9 */
    LINK_REG_D10,           /**< D10 */
    LINK_REG_D11,           /**< D11 */
    LINK_REG_D12,           /**< D12 */
    LINK_REG_D13,           /**< D13 */
    LINK_REG_D14,           /**< D14 */
    LINK_REG_D15,           /**< D15 */
    LINK_REG_A,             /**< A */
    LINK_REG_C              /**< C */

} link_register_t;

/**
 * @brief Address of an instruction in memory.
 */
typedef struct
{
    /** Address of this instruction in main memory */
    csirac_half_word_t address;

    /** Relocation parameter for the address; 0 if absolute, or >= 1 if the
     *  instruction is relative to a relocation number. */
    csirac_half_word_t reloc_param;

    /** Absolute address of the instruction word in memory */
    csirac_half_word_t absolute;

} address_t;

/**
 * @brief Information about a named symbol.
 */
typedef struct symbol_s symbol_t;
struct symbol_s
{
    /** Next symbol in the symbol lookup table */
    symbol_t *next;

    /** Address associated with this symbol */
    address_t address;

    /** Non-zero if the symbol is valid, or zero if no longer local */
    int valid;

    /** Link register for function definitions */
    link_register_t link_register;

    /** Buffer for the NUL-terminated name */
    char    name[1];
};

/**
 * @brief Type of CSIRAC assembly instruction.
 */
typedef enum
{
    INSN_REGULAR,       /**< Regular instruction */
    INSN_ORIGIN,        /**< Change of origin */
    INSN_LABEL,         /**< Label at the current address */
    INSN_FUNCTION       /**< Library function header, reset relocations */

} insn_type_t;

/**
 * @brief Information about a single CSIRAC assembly instruction or label.
 */
typedef struct
{
    /** Type of instruction */
    insn_type_t type;

    /** Address of this instruction in main memory */
    address_t address;

    /** Word to be written to the final machine code.  If the instruction
     *  uses a label reference, then this is the value to add to the
     *  label's actual address to get the full instruction word. */
    csirac_word_t word;

    /** Label if non-NULL rather than a machine code word */
    symbol_t *label;

    /** Label reference for performing fixups on the next pass */
    symbol_t *reference;

    /** The name of the file that contains the instruction, NULL if unknown */
    symbol_t *filename;

    /** The line number of the instruction, 0 if unknown */
    long line;

} insn_t;

/**
 * @brief Instructions for the CSIRAC assembly code.
 */
typedef struct
{
    /** List of instructions in the current program. */
    insn_t insn[MAX_INSNS];

    /** Number of instructions we have seen so far */
    unsigned num_insn;

    /** Address for the next instruction */
    address_t address;

    /** Relocation number to assign to the next function definition we see */
    csirac_half_word_t next_function_number;

    /** List of symbols in the program */
    symbol_t *symbols;

    /** Symbol for the entry point to an executable program */
    symbol_t *entry_point;

    /** Title for the tape */
    symbol_t *title;

    /** Flags that modify the generated output */
    int flags;

    /** Type of teleprinter to use for string data */
    csirac_media_type_t teleprinter;

    /** Type of input tape */
    csirac_media_type_t input_tape;

    /** Type of output punch */
    csirac_media_type_t output_punch;

} code_t;

/** Add the primary routine to the generated output */
#define CODE_FLAG_PRIMARY       0x0001
/** Add the control routine to the generated output */
#define CODE_FLAG_CONTROL       0x0002
/** Align functions on a group boundary; e.g. (1, 0), (2, 0), ... */
#define CODE_FLAG_ALIGN         0x0004

/**
 * @brief Initialises a CSIRAC code block.
 *
 * @param[out] code The code to initialise.
 * @param[in] flags Flags that modify the generated output.
 */
void code_init(code_t *code, int flags);

/**
 * @brief Frees a CSIRAC code block.
 *
 * @param[out] code The code to free.
 */
void code_free(code_t *code);

/**
 * @brief Finds a symbol by name in the code.
 *
 * @param[in] code The code to find the symbol in.
 * @param[in] name The name of the symbol.
 * @param[in] create Non-zero to create a new undefined symbol if the
 * name was not found.
 *
 * @return A pointer to the symbol, or NULL if not found.
 */
symbol_t *code_find_symbol(code_t *code, const char *name, int create);

/**
 * @brief Copies an instruction into a code block and assigns it an address.
 *
 * @param[in] code The code to add the instruction to.
 * @param[in] insn The instruction to add.
 *
 * @return Non-zero if added, or zero if the code block is already full.
 */
int code_add_insn(code_t *code, const insn_t *insn);

/**
 * @brief Mark the end of a function definition.
 *
 * @param[in] code The code to modify.
 */
void code_end_function(code_t *code);

/**
 * @brief Writes the final code to an output file in 12-hole punch format.
 *
 * @param[in] code The code to write.
 * @param[in] filename The name of the file to write to.
 *
 * @return Non-zero on success, or 0 on error.
 */
int code_write(code_t *code, const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* CODE_H */
