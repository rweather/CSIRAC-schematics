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

static const char * const sources[32] = {
    "M", "I", "NA", "NB", "A", "SA", "HA", "TA", "LA", "CA", "ZA",
    "B", "R", "RB", "C", "SC", "RC", "D", "SD", "RD", "Z", "HL",
    "HU", "S", "PE", "PL", "K", "MA", "MB", "MC", "MD", "PS"
};
static const char * const destinations[32] = {
    "M", "I", "OT", "OP", "A", "PA", "SA", "CA", "DA", "NA", "P",
    "B", "XB", "L", "C", "PC", "SC", "D", "PD", "SD", "Z", "HL",
    "HU", "S", "PS", "CS", "PK", "MA", "MB", "MC", "MD", "T"
};

/**
 * @brief Multiply two signed 19-bit numbers to produce a signed 38-bit result.
 *
 * @param[in] x First number.
 * @param[in] y Second number.
 *
 * @return The product of @a x and @a y.
 *
 * CSIRAC multiplication is not standard integer multiplication.
 * The input words are interpreted as real numbered values between
 * -1 (inclusive) and 1 (exclusive).
 *
 * Effectively we have a multiplication of two signed 19-bit numbers
 * to produce a signed 38-bit result.  After adding the sign bit back
 * and shifting up by 1 bit position to put the sign back into the MSB,
 * we get a final 40-bit double word as the result.
 */
static uint64_t csirac_multiply(csirac_word_t x, csirac_word_t y)
{
    int sign = 0;
    int32_t x2 = (int32_t)x;
    int32_t y2 = (int32_t)y;
    int64_t product;

    /* Convert the values into their absolute counterparts and
     * figure out the sign of the result */
    if ((x2 & CSIRAC_WORD_MSB_MASK) != 0) {
        sign = 1;
        x2 = (x2 ^ CSIRAC_WORD_MASK) + 1;
    }
    if ((y2 & CSIRAC_WORD_MSB_MASK) != 0) {
        sign = !sign;
        y2 = (y2 ^ CSIRAC_WORD_MASK) + 1;
    }

    /* Calculate the product of two 19-bit numbers to get a 38-bit result */
    product = ((int64_t)x2) * y2;

    /* Multiply by 2 to create a 39-bit result */
    product *= 2;

    /* Negate the result if the sign is negative and clamp to 40 bits */
    if (sign) {
        product = -product;
    }
    product &= 0x000000FFFFFFFFFFL;

    /* Return the result to the caller */
    return (uint64_t)product;
}

csirac_step_state_t csirac_step(csirac_state_t *state)
{
    csirac_word_t I;
    csirac_word_t W;
    csirac_half_word_t addr;

    /* Did we previously stop due to tape exhaustion or a breakpoint? */
    W = 0;
    if (state->step_state == CSIRAC_STEP_TAPE_EXHAUSTED) {
        /* Set up to re-execute the previous "(I) -> *" instruction */
        state->S = (state->S - CSIRAC_WORD_MID_MASK) & CSIRAC_WORD_MASK;
        state->I = state->prev_I;
    } else if (state->step_state == CSIRAC_STEP_BREAKPOINT) {
        /* Set up to execute the replacement instruction under the breakpoint.
         * Note: It is possible it has already been replaced. */
        state->S = (state->S - CSIRAC_WORD_MID_MASK) & CSIRAC_WORD_MASK;
        state->I = state->prev_I;
        I = state->I + state->M[state->S >> 10];
        I &= CSIRAC_WORD_MASK;
        if ((I & 0x0000001F) == 0x0000001F) {
            /* The previous instruction is a breakpoint instruction.
             * Arrange so that when state->I is added to the breakpoint
             * instruction below, we will modify the breakpoint into the
             * actual instruction to be executed. */
            int n = (int)(I >> 10);
            if (n >= 2 && n < (CSIRAC_NUM_BREAKPOINTS + 2)) {
                W = state->breakpoint_insn[n - 2] - I;
                W &= CSIRAC_WORD_MASK;
            }
        }
    }

    /* Save the previous state of I because we will need it to restart
     * the machine after tape exhaustion or a breakpoint. */
    state->prev_I = state->I;

    /* Fetch the next instruction and add it to I (for +K) */
    I = state->I + state->M[state->S >> 10] + W;
    I &= CSIRAC_WORD_MASK;
    ++(state->cycle_counter);

    /* Clear I before the next instruction fetch */
    state->I = 0;

    /* Dump the instruction that we are about to execute */
    if (state->dump_instructions) {
        printf("[%2d %2d]: %2d %2d %2s %2s\n",
               (int)((state->S >> 15) & 0x1F),
               (int)((state->S >> 10) & 0x1F),
               (int)((I >> 15) & 0x1F),
               (int)((I >> 10) & 0x1F),
               sources[(I >>  5) & 0x1F],
               destinations[I & 0x1F]);
    }

    /* Advance S to the next instruction */
    state->S = (state->S + CSIRAC_WORD_MID_MASK) & CSIRAC_WORD_MASK;

    /* If the word is all-zeroes, then raise an illegal instruction.
     * Technically, no instructions are "illegal" for CSIRAC but this one
     * probably means that the program has jumped off into unused memory.
     * We abort rather than loop executing "(0 M) -> 0 M" forever. */
    if (I == 0) {
        state->step_state = CSIRAC_STEP_ILLEGAL_INSTRUCTION;
        return state->step_state;
    }

    /* Extract the address field in P11..P20 */
    addr = ((csirac_half_word_t)(I >> 10)) & CSIRAC_HALF_WORD_MASK;

    /* Load the source gate's value into W */
    switch ((I >> 5) & 0x1F) {
    case CSIRAC_SRC_MAIN_MEMORY:
        /* Read from main memory */
        W = state->M[addr];
        break;

    case CSIRAC_SRC_INPUT_TAPE:
        /* Read from the input tape */
        if (!csirac_media_get(state, &W)) {
            state->step_state = CSIRAC_STEP_TAPE_EXHAUSTED;
            return state->step_state;
        }
        break;

    case CSIRAC_SRC_N1:
        /* General purpose input register N1 */
        W = state->N1;
        break;

    case CSIRAC_SRC_N2:
        /* General purpose input register N2 */
        W = state->N2;
        break;

    case CSIRAC_SRC_A:
        /* Content of the A register */
        W = state->A;
        break;

    case CSIRAC_SRC_A_MSB:
        /* Content of the sign bit of the A register */
        W = state->A & CSIRAC_WORD_MSB_MASK;
        break;

    case CSIRAC_SRC_A_RIGHT_SHIFT:
        /* A register shifted right by one bit */
        W = state->A >> 1;
        break;

    case CSIRAC_SRC_A_LEFT_SHIFT:
        /* A register shifted left by one bit */
        W = (state->A << 1) & CSIRAC_WORD_MASK;
        break;

    case CSIRAC_SRC_A_LSB:
        /* Content of the least significant bit of the A register */
        W = state->A & CSIRAC_WORD_LSB_MASK;
        break;

    case CSIRAC_SRC_A_CLEAR:
        /* Content of the A register and also clear the A register */
        W = state->A;
        state->A = 0;
        break;

    case CSIRAC_SRC_A_TEST:
        /* Test the A register for non-zero */
        if (state->A != 0) {
            W = 1;
        } else {
            W = 0;
        }
        break;

    case CSIRAC_SRC_B:
        /* Content of the B register */
        W = state->B;
        break;

    case CSIRAC_SRC_B_MSB:
        /* Content of the sign bit of the B register */
        W = state->B & CSIRAC_WORD_MSB_MASK;
        break;

    case CSIRAC_SRC_B_RIGHT_SHIFT:
        /* B register shifted right by one bit */
        W = state->B >> 1;
        break;

    case CSIRAC_SRC_C:
        /* Content of the C register */
        W = state->C;
        break;

    case CSIRAC_SRC_C_MSB:
        /* Content of the sign bit of the C register */
        W = state->C & CSIRAC_WORD_MSB_MASK;
        break;

    case CSIRAC_SRC_C_RIGHT_SHIFT:
        /* C register shifted right by one bit */
        W = state->C >> 1;
        break;

    case CSIRAC_SRC_D:
        /* Content of one of the D registers (0..15) */
        W = state->D[addr & 0x0F];
        break;

    case CSIRAC_SRC_D_MSB:
        /* Content of the sign bit of one of the D registers (0..15) */
        W = state->D[addr & 0x0F] & CSIRAC_WORD_MSB_MASK;
        break;

    case CSIRAC_SRC_D_RIGHT_SHIFT:
        /* One of the D registers (0..15) shifted right by one bit */
        W = state->D[addr & 0x0F] >> 1;
        break;

    case CSIRAC_SRC_ZERO:
    default:
        /* The value zero */
        W = 0;
        break;

    case CSIRAC_SRC_H_LOWER:
        /* Content of the H register, returned in bits P1..P10 */
        W = state->H;
        break;

    case CSIRAC_SRC_H_UPPER:
        /* Content of the H register, returned in bits P11..P20 */
        W = ((csirac_word_t)(state->H)) << 10;
        break;

    case CSIRAC_SRC_S:
        /* Content of the S register */
        W = state->S;
        break;

    case CSIRAC_SRC_P11:
        /* The value with all zero bits except for P11 */
        W = ((csirac_word_t)1) << 10;
        break;

    case CSIRAC_SRC_P1:
        /* The value with all zero bits except for P1 */
        W = CSIRAC_WORD_LSB_MASK;
        break;

    case CSIRAC_SRC_I:
        /* Content of the address portion of the current instruction,
         * leaving it in bits P11..P20 */
        W = I & 0x000FFC00U;
        break;

    case CSIRAC_SRC_DRUM_MEMORY_1:
        /* Read from magnetic drum store 1 */
        W = state->MD[0][addr];
        break;

    case CSIRAC_SRC_DRUM_MEMORY_2:
        /* Read from magnetic drum store 1 */
        W = state->MD[1][addr];
        break;

    case CSIRAC_SRC_DRUM_MEMORY_3:
        /* Read from magnetic drum store 1 */
        W = state->MD[2][addr];
        break;

    case CSIRAC_SRC_DRUM_MEMORY_4:
        /* Read from magnetic drum store 1 */
        W = state->MD[3][addr];
        break;

    case CSIRAC_SRC_P20:
        /* The value with all zero bits except for P20 */
        W = CSIRAC_WORD_MSB_MASK;
        break;
    }

    /* Write to the destination gate */
    switch (I & 0x1F) {
    case CSIRAC_DEST_MAIN_MEMORY:
        /* Write the word to main memory */
        state->M[addr] = W;
        break;

    case CSIRAC_DEST_NOP_I:
    case CSIRAC_DEST_NOP_Z:
    default:
        /* No operation */
        break;

    case CSIRAC_DEST_TELEPRINTER:
        /* Write W to the teleprinter */
        csirac_media_put(state->teleprinter, W);
        break;

    case CSIRAC_DEST_PUNCH_TAPE:
        /* Punch W to the output tape */
        csirac_media_put(state->output_punch, W);
        break;

    case CSIRAC_DEST_A:
        /* Write W to the A register */
        state->A = W;
        break;

    case CSIRAC_DEST_A_PLUS:
        /* Add W to the A register */
        state->A += W;
        state->A &= CSIRAC_WORD_MASK;
        break;

    case CSIRAC_DEST_A_MINUS:
        /* Subtract W from the A register */
        state->A -= W;
        state->A &= CSIRAC_WORD_MASK;
        break;

    case CSIRAC_DEST_A_AND:
        /* AND W with the A register */
        state->A &= W;
        break;

    case CSIRAC_DEST_A_OR:
        /* OR W with the A register */
        state->A |= W;
        break;

    case CSIRAC_DEST_A_XOR:
        /* XOR W with the A register (inequality check) */
        state->A ^= W;
        break;

    case CSIRAC_DEST_LOUDSPEAKER:
        /* Write W to the loudspeaker */
        if (state->play_sample) {
            (*(state->play_sample))(state, W);
        }

        /* Check if this is a "hoot" sequence, which will put us into an
         * infinite loop playing a continuous sound if we aren't careful.
         * The next instruction will be "31, 30 -> +S" for a "hoot".
         * Hooting is used to indicate "program has finished" or "something
         * went horribly wrong" in traditional CSIRAC programs. */
        if (state->M[state->S >> 10] == 0x000FFB58) {
            state->step_state = CSIRAC_STEP_HOOT;
        } else {
            state->step_state = CSIRAC_STEP_OK;
        }
        return state->step_state;

    case CSIRAC_DEST_B:
        /* Write W to the B register */
        state->B = W;
        break;

    case CSIRAC_DEST_B_TIMES: {
        /*
         * Multiply W by C, add the high word of the result to A, and
         * put the low word of the result in B.
         */
        uint64_t product = csirac_multiply(W, state->C);
        state->A += (csirac_word_t)(product >> 20);
        state->A &= CSIRAC_WORD_MASK;
        state->B = (csirac_word_t)(product & CSIRAC_WORD_MASK);

        /* I'm assuming that the multiplication is done in 20 clock cycles,
         * one bit at a time.  Credit 19 extra clock cycles. */
        state->cycle_counter += 19;
        break; }

    case CSIRAC_DEST_CYCLIC_SHIFT: {
        /*
         * Perform a cyclic shift on the AB register pair.
         *
         * This instruction has a number of special forms based on the
         * source gate and address:
         *
         *      (0, 0, 31, 13) or (16, 0, 26, 13) are a shift by 1.
         *      (16, 2, 26, 13) is a shift by 2.
         *      (16, 4, 26, 13) is a shift by 3.
         *      ...
         *
         * The diagram in Chapter 1 shows AB rotated through a carry bit,
         * but the examples in Chapter 2 only talk about shifts.  The diagram
         * is also confusing: on alternate cycles it rotates AB or BA.
         *
         * My best guess as to the behaviour is that on each cycle it
         * rotates B left through the carry bit and then swaps A and B.
         * After two cycles this has the effect of replacing AB with 2AB
         * if the sign bit of A was initially 0.
         *
         * There is no guidance in the manual as to what happens if the
         * instruction does not have one of the special forms.  Does it
         * halt the computer?  Do something random?  I have chosen to use
         * bits P11..P15 of the W value as the cycle count, no matter
         * where the W value may have came from.  This would allow shifting
         * by a dynamic amount taken from a register, memory location, or tape.
         *
         * It is possible that this implementation is wrong.
         */
        csirac_word_t carry = (state->A & CSIRAC_WORD_MSB_MASK) ? 1 : 0;
        csirac_word_t T;
        unsigned n = (unsigned)((W >> 10) & 0x1F);
        if (n < 15) {
            /* Count is encoded as n-2 */
            n += 2;
        } else {
            /* Count is encoded as 14+n */
            n -= 14;
        }
        state->cycle_counter += (n - 1);
        for (; n > 0; --n) {
            /* Rotate B through the carry bit and then swap A and B */
            T = (state->B << 1) | carry;
            carry = (state->B & CSIRAC_WORD_MSB_MASK) ? 1 : 0;
            state->B = state->A;
            state->A = T & CSIRAC_WORD_MASK;
        }
        break; }

    case CSIRAC_DEST_C:
        /* Write W to the C register */
        state->C = W;
        break;

    case CSIRAC_DEST_C_PLUS:
        /* Add W to the C register */
        state->C += W;
        state->C &= CSIRAC_WORD_MASK;
        break;

    case CSIRAC_DEST_C_MINUS:
        /* Subtract W from the C register */
        state->C -= W;
        state->C &= CSIRAC_WORD_MASK;
        break;

    case CSIRAC_DEST_D:
        /* Write W to the one of the D registers (0..15) */
        state->D[addr & 0x0F] = W;
        break;

    case CSIRAC_DEST_D_PLUS:
        /* Add W to the one of the D registers (0..15) */
        state->D[addr & 0x0F] += W;
        state->D[addr & 0x0F] &= CSIRAC_WORD_MASK;
        break;

    case CSIRAC_DEST_D_MINUS:
        /* Subtract W from the one of the D registers (0..15) */
        state->D[addr & 0x0F] -= W;
        state->D[addr & 0x0F] &= CSIRAC_WORD_MASK;
        break;

    case CSIRAC_DEST_H_LOWER:
        /* Write the lower 10 bits of W to H */
        state->H = (csirac_word_t)(W & CSIRAC_HALF_WORD_MASK);
        break;

    case CSIRAC_DEST_H_UPPER:
        /* Write the upper 10 bits of W to H */
        state->H = (csirac_word_t)((W >> 10) & CSIRAC_HALF_WORD_MASK);
        break;

    case CSIRAC_DEST_S:
        /* Write the upper 10 bits of W to S and zero the lower 10 bits */
        state->S = W & ~(CSIRAC_WORD_MID_MASK - 1);
        break;

    case CSIRAC_DEST_S_PLUS:
        /* Add W to S */
        state->S = (state->S + W) & CSIRAC_WORD_MASK;
        break;

    case CSIRAC_DEST_S_SKIP: {
        /* Skip an instruction if selected bits of W are non-zero.
         * Checks P1..P11 and P15..P20.  Skips if either group is
         * non-zero, but not both groups. */
        int group1 = (W & 0x000007FF) != 0;
        int group2 = (W & 0x000FC000) != 0;
        if ((group1 ^ group2) != 0) {
            state->S = (state->S + CSIRAC_WORD_MID_MASK) & CSIRAC_WORD_MASK;
        }
        break; }

    case CSIRAC_DEST_INSTRUCTION_ADD:
        /* Set up to add W to I during the next instruction (+K) */
        state->I = W;
        break;

    case CSIRAC_DEST_DRUM_MEMORY_1:
        /* Write to magnetic drum store 1 */
        state->MD[0][addr] = W;
        break;

    case CSIRAC_DEST_DRUM_MEMORY_2:
        /* Write to magnetic drum store 2 */
        state->MD[1][addr] = W;
        break;

    case CSIRAC_DEST_DRUM_MEMORY_3:
        /* Write to magnetic drum store 3 */
        state->MD[2][addr] = W;
        break;

    case CSIRAC_DEST_DRUM_MEMORY_4:
        /* Write to magnetic drum store 4 */
        state->MD[3][addr] = W;
        break;

    case CSIRAC_DEST_STOP:
        /* Stop the machine if W is non-zero */
        state->T = W;
        if (W != 0) {
            if (state->play_sample) {
                /* Turn the loudspeaker off when stopping the machine */
                (*(state->play_sample))(state, 0);
            }
            W = (W & 0x00007C00) >> 10;
            if (W >= 2 && W < (CSIRAC_NUM_BREAKPOINTS + 2)) {
                /* This is looks like a breakpoint instruction */
                state->step_state = CSIRAC_STEP_BREAKPOINT;
            } else {
                /* Regular halt instruction */
                state->step_state = CSIRAC_STEP_HALT;
            }
            return state->step_state;
        }
        break;
    }

    /* This instruction did not use the loudspeaker, so we implicitly play 0 */
    if (state->play_sample) {
        (*(state->play_sample))(state, 0);
    }

    /* If we get here, then the machine has not halted */
    state->step_state = CSIRAC_STEP_OK;
    return state->step_state;
}

csirac_step_state_t csirac_run(csirac_state_t *state)
{
    csirac_step_state_t step;
    do {
        step = csirac_step(state);
    } while (step == CSIRAC_STEP_OK);
    return step;
}

int csirac_state_set_breakpoint
    (csirac_state_t *state, int n, csirac_half_word_t address)
{
    int m;
    if (n < 0 || n >= CSIRAC_NUM_BREAKPOINTS) {
        /* Invalid breakpoint number */
        return 0;
    }
    address &= CSIRAC_HALF_WORD_MASK;
    for (m = 0; m < CSIRAC_NUM_BREAKPOINTS; ++m) {
        /* Check for existing breakpoints at the address */
        if (state->breakpoint_addr[m] == (address + 1)) {
            return 0;
        }
    }
    if (state->breakpoint_addr[n] != 0) {
        /* Clear the breakpoint if it is already set */
        state->M[state->breakpoint_addr[n] - 1] = state->breakpoint_insn[n];
    }
    state->breakpoint_addr[n] = address + 1;
    state->breakpoint_insn[n] = state->M[address];

    /* Replace the instruction with "n+2 -> T" which halts the machine
     * with the breakpoint number encoded in the instruction. */
    state->M[address] =
        (((csirac_word_t)(n + 2)) << 10) | CSIRAC_DEST_STOP;
    return 1;
}

void csirac_state_clear_breakpoint(csirac_state_t *state, int n)
{
    if (n < 0 || n >= CSIRAC_NUM_BREAKPOINTS) {
        /* Invalid breakpoint number */
        return;
    }
    if (state->breakpoint_addr[n] == 0) {
        /* Breakpoint is not set */
        return;
    }
    state->M[state->breakpoint_addr[n] - 1] = state->breakpoint_insn[n];
    state->breakpoint_addr[n] = 0;
    state->breakpoint_insn[n] = 0;
}
