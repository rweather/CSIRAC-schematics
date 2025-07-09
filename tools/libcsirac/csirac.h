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

#ifndef CSIRAC_H
#define CSIRAC_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Representation of a CSIRAC word.
 *
 * Only the bottom 20 bits of the value are valid.  The rest is masked
 * off whenever an operation is performed on a word.
 */
typedef uint32_t csirac_word_t;

/**
 * @brief Representation of a CSIRAC half word.
 *
 * Only the bottom 10 bits of the value are valid.  The rest is masked
 * off whenever an operation is performed on a half word.
 */
typedef uint16_t csirac_half_word_t;

/**
 * @brief Mask to remove the unnecessary bits from a CSIRAC word.
 */
#define CSIRAC_WORD_MASK        ((((csirac_word_t)1) << 20) - 1U)

/**
 * @brief Mask to extract the most significant bit from a CSIRAC word.
 */
#define CSIRAC_WORD_MSB_MASK    (((csirac_word_t)1) << 19)

/**
 * @brief Mask to extract the least significant bit from a CSIRAC word.
 */
#define CSIRAC_WORD_LSB_MASK    ((csirac_word_t)1)

/**
 * @brief Mask to extract the middle bit (P11) from a CSIRAC word.
 */
#define CSIRAC_WORD_MID_MASK    (((csirac_word_t)1) << 10)

/**
 * @brief Mask to remove the unnecessary bits from a CSIRAC half word.
 */
#define CSIRAC_HALF_WORD_MASK   ((((csirac_half_word_t)1) << 10) - 1U)

/**
 * @brief X punch position on 12-hole tape.
 */
#define CSIRAC_WORD_X           (((csirac_word_t)1) << 18)

/**
 * @brief Y punch position on 12-hole tape.
 */
#define CSIRAC_WORD_Y           (((csirac_word_t)1) << 19)

/**
 * @brief Builds a CSIRAC word out of four 5-bit components.
 *
 * @param[in] a The first component (most significant).
 * @param[in] b The second component.
 * @param[in] c The third component.
 * @param[in] d The fourth component (least significant).
 *
 * @return The full 20-bit CSIRAC word.
 */
#define CSIRAC_WORD(a, b, c, d) \
    ((((csirac_word_t)(a)) << 15) | \
     (((csirac_word_t)(b)) << 10) | \
     (((csirac_word_t)(c)) <<  5) | \
      ((csirac_word_t)(d)))

/**
 * @brief Number of D registers in the machine.
 */
#define CSIRAC_NUM_D_REGISTERS  16

/**
 * @brief Size of main memory in words.
 */
#define CSIRAC_MAIN_MEMORY_SIZE 1024

/**
 * @brief Number of stores that make up the magnetic drum memory.
 */
#define CSIRAC_NUM_DRUM_STORES  4

/**
 * @brief Size of a magnetic drum memory store in words.
 */
#define CSIRAC_DRUM_STORE_SIZE  1024

/**
 * @brief Number of breakpoints that can be set.
 */
#define CSIRAC_NUM_BREAKPOINTS  30

/**
 * @brief Source gates for the inputs to instructions.
 */
typedef enum
{
    /** Source is a location in main memory */
    CSIRAC_SRC_MAIN_MEMORY      = 0,

    /** Source is a word from the input tape */
    CSIRAC_SRC_INPUT_TAPE       = 1,

    /** Source is the general purpose input register N1 */
    CSIRAC_SRC_N1               = 2,

    /** Source is the general purpose input register N2 */
    CSIRAC_SRC_N2               = 3,

    /** Source is the A register */
    CSIRAC_SRC_A                = 4,

    /** Source is the MSB (sign bit) of the A register */
    CSIRAC_SRC_A_MSB            = 5,

    /** Source is the A register, shifted right by 1 bit */
    CSIRAC_SRC_A_RIGHT_SHIFT    = 6,

    /** Source is the A register, shifted left by 1 bit */
    CSIRAC_SRC_A_LEFT_SHIFT     = 7,

    /** Source is the LSB of the A register */
    CSIRAC_SRC_A_LSB            = 8,

    /** Source is the A register, which is cleared after it is read */
    CSIRAC_SRC_A_CLEAR          = 9,

    /** Source is 1 if the A register is non-zero, 0 if it is zero */
    CSIRAC_SRC_A_TEST           = 10,

    /** Source is the B register */
    CSIRAC_SRC_B                = 11,

    /** Source is the MSB (sign bit) of the B register */
    CSIRAC_SRC_B_MSB            = 12,

    /** Source is the B register, shifted right by 1 bit */
    CSIRAC_SRC_B_RIGHT_SHIFT    = 13,

    /** Source is the C register */
    CSIRAC_SRC_C                = 14,

    /** Source is the MSB (sign bit) of the C register */
    CSIRAC_SRC_C_MSB            = 15,

    /** Source is the C register, shifted right by 1 bit */
    CSIRAC_SRC_C_RIGHT_SHIFT    = 16,

    /** Source is a Dn register */
    CSIRAC_SRC_D                = 17,

    /** Source is the MSB (sign bit) of a Dn register */
    CSIRAC_SRC_D_MSB            = 18,

    /** Source is a Dn register, shifted right by 1 bit */
    CSIRAC_SRC_D_RIGHT_SHIFT    = 19,

    /** Source is the value zero */
    CSIRAC_SRC_ZERO             = 20,

    /** Source is the value of the H register, positioned in the lower bits */
    CSIRAC_SRC_H_LOWER          = 21,

    /** Source is the value of the H register, positioned in the upper bits */
    CSIRAC_SRC_H_UPPER          = 22,

    /** Source is the value of the S register, positioned in the upper bits */
    CSIRAC_SRC_S                = 23,

    /** Source is the value 1, positioned in the upper bits */
    CSIRAC_SRC_P11              = 24,

    /** Source is the value 1, positioned in the LSB */
    CSIRAC_SRC_P1               = 25,

    /** Source is the constant in the upper bits of the interpreter register */
    CSIRAC_SRC_I                = 26,

    /** Source is a location in the first store of magnetic drum memory */
    CSIRAC_SRC_DRUM_MEMORY_1    = 27,

    /** Source is a location in the second store of magnetic drum memory */
    CSIRAC_SRC_DRUM_MEMORY_2    = 28,

    /** Source is a location in the third store of magnetic drum memory */
    CSIRAC_SRC_DRUM_MEMORY_3    = 29,

    /** Source is a location in the fourth store of magnetic drum memory */
    CSIRAC_SRC_DRUM_MEMORY_4    = 30,

    /** Source is the value 1, positioned in the MSB */
    CSIRAC_SRC_P20              = 31

} csirac_src_gate_t;

/**
 * @brief Destination gates for the outputs from instructions.
 */
typedef enum
{
    /** Destination is a location in main memory */
    CSIRAC_DEST_MAIN_MEMORY     = 0,

    /** No operation */
    CSIRAC_DEST_NOP_I           = 1,

    /** Destination is the teleprinter */
    CSIRAC_DEST_TELEPRINTER     = 2,

    /** Destination is the punch tape */
    CSIRAC_DEST_PUNCH_TAPE      = 3,

    /** Destination is the A register */
    CSIRAC_DEST_A               = 4,

    /** Destination is adding to the A register */
    CSIRAC_DEST_A_PLUS          = 5,

    /** Destination is subtracting from the A register */
    CSIRAC_DEST_A_MINUS         = 6,

    /** Destination is AND'ing with the A register (conjunction) */
    CSIRAC_DEST_A_AND           = 7,

    /** Destination is OR'ing with the A register (disjunction) */
    CSIRAC_DEST_A_OR            = 8,

    /** Destination is XOR'ing with the A register (inequality test) */
    CSIRAC_DEST_A_XOR           = 9,

    /** Destination is the loudspeaker */
    CSIRAC_DEST_LOUDSPEAKER     = 10,

    /** Destination is the B register */
    CSIRAC_DEST_B               = 11,

    /** Destination is the B register with multiplication */
    CSIRAC_DEST_B_TIMES         = 12,

    /** Destination is a cyclic shift on the AB register pair */
    CSIRAC_DEST_CYCLIC_SHIFT    = 13,

    /** Destination is the C register */
    CSIRAC_DEST_C               = 14,

    /** Destination is adding to the C register */
    CSIRAC_DEST_C_PLUS          = 15,

    /** Destination is subtracting from the C register */
    CSIRAC_DEST_C_MINUS         = 16,

    /** Destination is a Dn register */
    CSIRAC_DEST_D               = 17,

    /** Destination is adding to a Dn register */
    CSIRAC_DEST_D_PLUS          = 18,

    /** Destination is subtracting from a Dn register */
    CSIRAC_DEST_D_MINUS         = 19,

    /** No operation */
    CSIRAC_DEST_NOP_Z           = 20,

    /** Destination is the H register, writing the lower bits of the value */
    CSIRAC_DEST_H_LOWER         = 21,

    /** Destination is the H register, writing the upper bits of the value */
    CSIRAC_DEST_H_UPPER         = 22,

    /** Destination is the S register, writing the upper bits of the value */
    CSIRAC_DEST_S               = 23,

    /** Destination is the S register, adding to the current value */
    CSIRAC_DEST_S_PLUS          = 24,

    /** Destination is the S register, skipping an instruction if non-zero */
    CSIRAC_DEST_S_SKIP          = 25,

    /** Destination is the next instruction; add to the instruction value */
    CSIRAC_DEST_INSTRUCTION_ADD = 26,

    /** Destination is a location in the first store of magnetic drum memory */
    CSIRAC_DEST_DRUM_MEMORY_1   = 27,

    /** Destination is a location in the second store of magnetic drum memory */
    CSIRAC_DEST_DRUM_MEMORY_2   = 28,

    /** Destination is a location in the third store of magnetic drum memory */
    CSIRAC_DEST_DRUM_MEMORY_3   = 29,

    /** Destination is a location in the fourth store of magnetic drum memory */
    CSIRAC_DEST_DRUM_MEMORY_4   = 30,

    /** Destination is the stop register T; write a 1 to halt the machine */
    CSIRAC_DEST_STOP            = 31

} csirac_dest_gate_t;

/**
 * @brief Type of media that may be input to or output from a CSIRAC.
 */
typedef enum
{
    /** 12-hole tape for programs */
    CSIRAC_MEDIA_12HOLE_TAPE,

    /** 5-hole tape for program data input and output */
    CSIRAC_MEDIA_5HOLE_TAPE,

    /** Teleprinter */
    CSIRAC_MEDIA_TELEPRINTER,

    /** Flexowriter */
    CSIRAC_MEDIA_FLEXOWRITER,

    /** 8-hole tape for binary data (extension to the original design) */
    CSIRAC_MEDIA_8HOLE_TAPE,

    /** ASCII teletype (extension to the original design) */
    CSIRAC_MEDIA_ASCII

} csirac_media_type_t;

/**
 * @brief Complete state of the CSIRAC machine.
 */
typedef struct csirac_state_s csirac_state_t;

/**
 * @brief Information about input or output media for a CSIRAC.
 *
 * Media are organised as a list.  When reading, if the current media
 * item is exhausted, then the CSIRAC engine will remove the media
 * item and proceed to the next input media that is found.
 *
 * When writing, only a single media item can be present.  Additional
 * media is ignored as output media are assumed to be infinite in capacity.
 * Well, at least until you run out of paper to print on or tape to punch!
 */
typedef struct csirac_media_s
{
    /** Type of media */
    csirac_media_type_t type;

    /** Non-zero if figure shift is active on this media */
    int figure_shift;

    /** Next character to read if we just changed figure/letter shift mode */
    csirac_word_t ch;

    /** Stdio stream for accessing the media */
    FILE *file;

    /** Next media object that is attached to the CSIRAC */
    struct csirac_media_s *next;

    /** CSIRAC machine state that owns this media */
    csirac_state_t *state;

} csirac_media_t;

/**
 * @brief Callback that plays a sample through the loudspeaker.
 *
 * @param[in] state Points to the CSIRAC machine state.
 * @param[in] sample The 20-bit sample value to be played.
 *
 * The "cycle_counter" field of @a state can be used to establish a
 * timebase for the sound output based on a 1kHz instruction clock speed.
 *
 * Note that this callback will be invoked for every instruction,
 * with a value of 0 for instructions that do not write to the loudspeaker.
 * The "cycle_counter" may advance by more than 1 each time if the
 * instruction took multiple cycles to execute.
 */
typedef void (*csirac_play_sample_t)
        (csirac_state_t *state, csirac_word_t sample);

/**
 * @brief State of the machine after running a single instruction step.
 */
typedef enum
{
    /** Step was successful, machine is still running */
    CSIRAC_STEP_OK,

    /** Machine has halted on a "* -> T" instruction */
    CSIRAC_STEP_HALT,

    /** Encountered a "hoot" sequence which would loop forever
     *  blasting a tone on the loudspeaker. */
    CSIRAC_STEP_HOOT,

    /** The input tape has been exhausted but the program has attempted
     *  to read another tape symbol.  The caller can load another tape
     *  and then call csirac_step() or csirac_run() to continue execution. */
    CSIRAC_STEP_TAPE_EXHAUSTED,

    /** Encountered a "(0 M) -> 0 M" instruction which probably means that the
     *  code has jumped off into unused memory.  We treat this as an illegal
     *  instruction and stop the program. */
    CSIRAC_STEP_ILLEGAL_INSTRUCTION,

    /** Encountered a "(31, x M) -> (31, x M)" instruction which the
     *  emulator treats as a "breakpoint" with "x" indicating the
     *  breakpoint number.  The caller can call csirac_step() or
     *  csirac_run() to resume execution from the breakpoint onwards. */
    CSIRAC_STEP_BREAKPOINT

} csirac_step_state_t;

/**
 * @brief Complete state of the CSIRAC machine.
 */
struct csirac_state_s
{
    /** Contents of the A register */
    csirac_word_t       A;

    /** Contents of the B register */
    csirac_word_t       B;

    /** Contents of the C register */
    csirac_word_t       C;

    /** Contents of the H register */
    csirac_half_word_t  H;

    /** Contents of the sequence register (program counter) */
    csirac_word_t       S;

    /** Contents of the interpreter register (instruction register) */
    csirac_word_t       I;

    /** Contents of the general purpose input register N1 */
    csirac_word_t       N1;

    /** Contents of the general purpose input register N2 */
    csirac_word_t       N2;

    /** Contents of the D0 to D15 registers */
    csirac_word_t       D[CSIRAC_NUM_D_REGISTERS];

    /** Main memory for the machine */
    csirac_word_t       M[CSIRAC_MAIN_MEMORY_SIZE];

    /** Magnetic drum memory for the machine */
    csirac_word_t       MD[CSIRAC_NUM_DRUM_STORES][CSIRAC_DRUM_STORE_SIZE];

    /** Contents of the stop register T.  A non-zero value halts the machine */
    csirac_word_t       T;

    /* The following fields are for emulator housekeeping */

    /** Result of running the last instruction step */
    csirac_step_state_t step_state;

    /** Previous contents of the interpreter register, for restoration of I
     *  after a breakpoint or tape exhaustion. */
    csirac_word_t       prev_I;

    /** Linked list of input tapes.  Items are deleted from this list
     *  as each input tape source reports EOF. */
    csirac_media_t     *input_tape;

    /** Output media for the teleprinter */
    csirac_media_t     *teleprinter;

    /** Output media for the output punch tape */
    csirac_media_t     *output_punch;

    /** Number of instruction fetch-execute cycles that have occurred so far */
    uint64_t            cycle_counter;

    /** Number of words that have been read from the input tape so far */
    uint64_t            tape_counter;

    /** Callback to play a sample through the loudspeaker; NULL if unused */
    csirac_play_sample_t play_sample;

    /** Location of each of the breakpoints + 1, 0 if no breakpoint */
    csirac_half_word_t  breakpoint_addr[CSIRAC_NUM_BREAKPOINTS];

    /** Instructions that were underneath the breakpoints for re-execution */
    csirac_word_t       breakpoint_insn[CSIRAC_NUM_BREAKPOINTS];

    /** Non-zero to dump the instructions as they are executed */
    int                 dump_instructions;
};

/**
 * @brief Standard object file metadata section types.
 */
typedef enum
{
    /** Number of instruction fetch-execute cycles that have been done */
    CSIRAC_META_CYCLE_COUNTER   = 32,

    /** Number of words that have been read from the input tape */
    CSIRAC_META_TAPE_COUNTER    = 33

} csirac_metadata_t;

/**
 * @brief Code for the primary bootstrap routine.
 */
extern csirac_word_t const csirac_primary_routine[];

/**
 * @brief Length of the primary bootstrap routine in words.
 */
extern size_t const csirac_primary_routine_len;

/**
 * @brief Length of the primary bootstrap routine in words, up to just
 * before the control routine is loaded.
 */
extern size_t const csirac_short_primary_routine_len;

/**
 * @brief Code for the control bootstrap routine.
 */
extern csirac_word_t const csirac_control_routine[];

/**
 * @brief Length of the control bootstrap routine in words.
 */
extern size_t const csirac_control_routine_len;

/**
 * @brief Initialises the state of the CSIRAC machine.
 *
 * @param[out] state Points to the CSIRAC machine state.
 */
void csirac_init(csirac_state_t *state);

/**
 * @brief Frees the state of a CSIRAC machine.
 *
 * @param[in] state Points to the CSIRAC machine state.
 */
void csirac_free(csirac_state_t *state);

/**
 * @brief Step a single instruction using the CSIRAC machine.
 *
 * @param[in,out] state Points to the CSIRAC machine state.
 *
 * @return The result of running the step.
 */
csirac_step_state_t csirac_step(csirac_state_t *state);

/**
 * @brief Runs the CSIRAC machine until halt or hoot.
 *
 * @param[in,out] state Points to the CSIRAC machine state.
 *
 * @return Returns CSIRAC_STEP_HALT, CSIRAC_STEP_HOOT, or some other
 * step code that means the machine has stopped.
 */
csirac_step_state_t csirac_run(csirac_state_t *state);

/**
 * @brief Sets a breakpoint in the CSIRAC machine's main memory.
 *
 * @param[in] state Points to the CSIRAC machine state.
 * @param[in] n The number of the breakpoint, 0 to CSIRAC_NUM_BREAKPOINTS-1.
 * @param[in] address The address to set the breakpoint at, 0 to 1023.
 *
 * @return Non-zero if the breakpoint was set, zero if @a n is out of range,
 * or zero if there is already a breakpoint set at @a address.
 */
int csirac_state_set_breakpoint
    (csirac_state_t *state, int n, csirac_half_word_t address);

/**
 * @brief Clears a breakpoint from the CSIRAC machine.
 *
 * @param[in] state Points to the CSIRAC machine state.
 * @param[in] n The number of the breakpoint, 0 to CSIRAC_NUM_BREAKPOINTS-1.
 *
 * The original instruction at the breakpoint will be restored.
 */
void csirac_state_clear_breakpoint(csirac_state_t *state, int n);

/**
 * @brief Saves the contents of magnetic drum memory to a file.
 *
 * @param[in] state Points to the CSIRAC machine state.
 * @param[in] filename Name of the file to save to.
 *
 * @return Non-zero if the file was saved, or zero on error (in errno).
 *
 * This function simulates the non-volatile nature of drum memory,
 * where the rest of the state is volatile.
 *
 * The 20-bit words on the drum are written to @a filename as
 * 24-bit values in big-endian byte order.  Store 1 is written first,
 * followed by the other stores.
 */
int csirac_save_drum(const csirac_state_t *state, const char *filename);

/**
 * @brief Loads the contents of magnetic drum memory from a file.
 *
 * @param[in] state Points to the CSIRAC machine state.
 * @param[in] filename Name of the file to load from.
 *
 * @return Non-zero if the file was saved, or zero on error (in errno).
 *
 * This function simulates the non-volatile nature of drum memory,
 * where the rest of the state is volatile.
 */
int csirac_load_drum(csirac_state_t *state, const char *filename);

/**
 * @brief Opens an input tape media source and attaches it to the CSIRAC state.
 *
 * @param[in] state Points to the CSIRAC machine state.
 * @param[in] type The type of tape media to be opened.
 * @param[in] filename Points to the file to open.  Set this to "-" or
 * NULL for standard input.
 *
 * @return Non-zero if the file was opened, or zero on error (in errno).
 *
 * The new media source is appended to the end of the tape input media list.
 * As each media source is exhausted, the first input media source will be
 * deleted and then input will be taken from the next source in the list.
 */
int csirac_open_input_tape
    (csirac_state_t *state, csirac_media_type_t type, const char *filename);

/**
 * @brief Closes all tape input sources that are attached to the CSIRAC state.
 *
 * @param[in] state Points to the CSIRAC machine state.
 */
void csirac_close_input_tape(csirac_state_t *state);

/**
 * @brief Moves the last input tape source to the front of the list.
 *
 * @param[in] state Points to the CSIRAC machine state.
 */
void csirac_move_input_tape_to_front(csirac_state_t *state);

/**
 * @brief Opens an output media sink and attaches it to the CSIRAC state
 * as the teleprinter.
 *
 * @param[in] state Points to the CSIRAC machine state.
 * @param[in] type The type of media to be opened.
 * @param[in] filename Points to the file to open.  Set this to "-" or
 * NULL for standard output.
 *
 * @return Non-zero if the file was opened, or zero on error (in errno).
 *
 * Any existing teleprinter media is closed by calling this function.
 */
int csirac_open_teleprinter
    (csirac_state_t *state, csirac_media_type_t type, const char *filename);

/**
 * @brief Closes the teleprinter media sink that is attached to the
 * CSIRAC state.
 *
 * @param[in] state Points to the CSIRAC machine state.
 */
void csirac_close_teleprinter(csirac_state_t *state);

/**
 * @brief Opens an output media sink and attaches it to the CSIRAC state
 * as the output punch tape.
 *
 * @param[in] state Points to the CSIRAC machine state.
 * @param[in] type The type of media to be opened.
 * @param[in] filename Points to the file to open.  Set this to "-" or
 * NULL for standard output.
 *
 * @return Non-zero if the file was opened, or zero on error (in errno).
 *
 * Any existing output punch media is closed by calling this function.
 */
int csirac_open_output_punch
    (csirac_state_t *state, csirac_media_type_t type, const char *filename);

/**
 * @brief Closes the output punch media sink that is attached to the
 * CSIRAC state.
 *
 * @param[in] state Points to the CSIRAC machine state.
 */
void csirac_close_output_punch(csirac_state_t *state);

/**
 * @brief Gets the next word from the current input source.
 *
 * @param[in] state Points to the CSIRAC machine state.
 * @param[out] word Returns the next word.
 *
 * @return Returns non-zero if a word was read, or zero if all input
 * sources have been exhausted.
 */
int csirac_media_get(csirac_state_t *state, csirac_word_t *word);

/**
 * @brief Puts a word to a media output sink.
 *
 * @param[in] media The media output sink.
 * @param[in] word The word to put.
 *
 * Many outputs only use a subset of the bits in @a word.
 */
void csirac_media_put(csirac_media_t *media, csirac_word_t word);

/**
 * @brief String starts or ends in figure shift mode.
 *
 * This flag is set on both input and output.
 */
#define CSIRAC_STR_CONVERT_FIGURE_SHIFT 0x01

/**
 * @brief Do not insert an initial shift if not started in the right mode.
 * Assume the mode from the first character instead.
 */
#define CSIRAC_STR_CONVERT_NO_SHIFT 0x02

/**
 * @brief Converts a string into teleprinter codes.
 *
 * @param[out] out Buffer to write the teleprinter codes to.
 * @param[in] out_size Size of the @a out buffer in bytes.
 * @param[in] in Buffer to read the input UTF-8 characters from.
 * @param[in] in_size Number of bytes in the UTF-8 string @a in.
 * @param[in,out] flags Flags that modify the conversion process.
 *
 * @return The number of bytes written to @a out, -1 if an unconvertible
 * character was encountered, or -2 if the @a out buffer is not large enough.
 */
int csirac_string_to_teleprinter
    (uint8_t *out, size_t out_size, const char *in, size_t in_size, int *flags);

/**
 * @brief Converts a string into Flexowriter codes.
 *
 * @param[out] out Buffer to write the Flexowriter codes to.
 * @param[in] out_size Size of the @a out buffer in bytes.
 * @param[in] in Buffer to read the input UTF-8 characters from.
 * @param[in] in_size Number of bytes in the UTF-8 string @a in.
 * @param[in,out] flags Flags that modify the conversion process.
 *
 * @return The number of bytes written to @a out, -1 if an unconvertible
 * character was encountered, or -2 if the @a out buffer is not large enough.
 */
int csirac_string_to_flexowriter
    (uint8_t *out, size_t out_size, const char *in, size_t in_size, int *flags);

/**
 * @brief Gets the length of a UTF-8 string in Unicode code points.
 *
 * @param[in] str Points to the UTF-8 string.
 * @param[in] size Number of bytes in the UTF-8 string.
 *
 * @return The size of @a str in Unicode code points.
 */
size_t csirac_string_unicode_length(const char *str, size_t size);

/**
 * @brief Gets the next Unicode character from a UTF-8 string.
 *
 * @param[in] str Points to the UTF-8 string.
 * @param[in] size Number of bytes in the UTF-8 string.
 * @param[in,out] posn Current position in the string which is updated as
 * the contents of the string are processed.
 *
 * @return The next Unicode code point or -1 at the end of the string.
 */
int32_t csirac_string_get_next_unicode
    (const char *str, size_t size, size_t *posn);

#ifdef __cplusplus
}
#endif

#endif /* CSIRAC_H */
