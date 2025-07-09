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

#include <libcsirac/csirac.h>
#include <string.h>

static csirac_state_t machine;

static void disassemble(csirac_state_t *machine, uint16_t address);
static void disassemble_mnemonic(csirac_state_t *machine, int mode);

int main(int argc, char *argv[])
{
    int exitval = 0;
    int multi_files = (argc > 3);
    uint16_t address = 0;
    int mnemonic_mode = 0;

    /* Initialise the machine state */
    csirac_init(&machine);

    /* "-m" changes to the old-style mnemonic mode */
    if (argc > 1 && !strcmp(argv[1], "-m")) {
        mnemonic_mode = 1;
        ++argv;
        --argc;
    }

    /* "-c" uses mnemonic mode but decodes the instructions into the
     * classic CSIRAC "X -> Y" syntax instead of "Melbourne Form" */
    if (argc > 1 && !strcmp(argv[1], "-c")) {
        mnemonic_mode = 2;
        ++argv;
        --argc;
    }

    /* Dump all incoming tapes */
    while (argc > 1) {
        if (csirac_open_input_tape
                (&machine, CSIRAC_MEDIA_12HOLE_TAPE, argv[1])) {
            if (multi_files) {
                printf("%s:\n", argv[1]);
            }
            if (mnemonic_mode) {
                disassemble_mnemonic(&machine, mnemonic_mode);
            } else {
                disassemble(&machine, address);
            }
            csirac_close_input_tape(&machine);
            if (multi_files) {
                putc('\n', stdout);
                address = 0;
            }
        } else {
            perror(argv[1]);
        }
        ++argv;
        --argc;
    }

    /* Free the machine state */
    csirac_free(&machine);
    return exitval;
}

static void decode_insn
    (uint16_t address, uint16_t relative_base, csirac_word_t insn,
     int perform, int param)
{
    csirac_half_word_t iaddr = (insn >> 10);
    char srcbuf[64];
    char destbuf[64];
    char scommentbuf[64];
    char dcommentbuf[64];
    const char *src;
    const char *dest;
    const char *scomment;
    const char *dcomment;
    int const_load;

    /* Print the address we are currently disassembling */
    if (address != 0xFFFF) {
        if (perform) {
            /* Performing a command under control - doesn't have an address */
            printf("Do           : ");
        } else {
            printf("%4d [%2d, %2d]: ",
                   (int)address,
                   (int)(address >> 5), (int)(address & 0x1F));
        }
    }

    /* Decode the source */
    scomment = NULL;
    const_load = 0;
    switch ((insn >> 5) & 0x1F) {
    case CSIRAC_SRC_MAIN_MEMORY:
        if (param != -1) {
            snprintf(srcbuf, sizeof(srcbuf), "(R%d + %d, %d M)",
                     param, (int)(iaddr >> 5), (int)(iaddr & 0x1F));
            snprintf(scommentbuf, sizeof(scommentbuf),
                     "Read Memory at R%d + %d", param, iaddr);
        } else {
            snprintf(srcbuf, sizeof(srcbuf), "(%d, %d M)",
                     (int)(iaddr >> 5), (int)(iaddr & 0x1F));
            snprintf(scommentbuf, sizeof(scommentbuf),
                     "Read Memory at %d", iaddr);
        }
        src = srcbuf;
        scomment = scommentbuf;
        break;

    case CSIRAC_SRC_INPUT_TAPE:
        src = "(I)";
        scomment = "Read Input Tape";
        break;

    case CSIRAC_SRC_N1:
        src = "(N1)";
        scomment = "Read N1";
        break;

    case CSIRAC_SRC_N2:
        src = "(N2)";
        scomment = "Read N2";
        break;

    case CSIRAC_SRC_A:
        src = "(A)";
        scomment = "Read A";
        break;

    case CSIRAC_SRC_A_MSB:
        src = "s(A)";
        scomment = "Read MSB of A";
        break;

    case CSIRAC_SRC_A_RIGHT_SHIFT:
        src = "r(A)";
        scomment = "Shift A Right";
        break;

    case CSIRAC_SRC_A_LEFT_SHIFT:
        src = "2(A)";
        scomment = "Shift A Left";
        break;

    case CSIRAC_SRC_A_LSB:
        src = "p1(A)";
        scomment = "Read LSB of A";
        break;

    case CSIRAC_SRC_A_CLEAR:
        src = "c(A)";
        scomment = "Read A and Clear";
        break;

    case CSIRAC_SRC_A_TEST:
        src = "z(A)";
        scomment = "Test A";
        break;

    case CSIRAC_SRC_B:
        src = "(B)";
        scomment = "Read B";
        break;

    case CSIRAC_SRC_B_MSB:
        src = "(R)";
        scomment = "Read MSB of B";
        break;

    case CSIRAC_SRC_B_RIGHT_SHIFT:
        src = "r(B)";
        scomment = "Shift B Right";
        break;

    case CSIRAC_SRC_C:
        src = "(C)";
        scomment = "Read C";
        break;

    case CSIRAC_SRC_C_MSB:
        src = "s(C)";
        scomment = "Read MSB of C";
        break;

    case CSIRAC_SRC_C_RIGHT_SHIFT:
        src = "r(C)";
        scomment = "Shift C Right";
        break;

    case CSIRAC_SRC_D:
        snprintf(srcbuf, sizeof(srcbuf), "(D%d)", (int)(iaddr & 0x0F));
        snprintf(scommentbuf, sizeof(scommentbuf), "Read D%d",
                (int)(iaddr & 0x0F));
        src = srcbuf;
        scomment = scommentbuf;
        break;

    case CSIRAC_SRC_D_MSB:
        snprintf(srcbuf, sizeof(srcbuf), "s(D%d)", (int)(iaddr & 0x0F));
        snprintf(scommentbuf, sizeof(scommentbuf), "Read MSB of D%d",
                (int)(iaddr & 0x0F));
        src = srcbuf;
        scomment = scommentbuf;
        break;

    case CSIRAC_SRC_D_RIGHT_SHIFT:
        snprintf(srcbuf, sizeof(srcbuf), "r(D%d)", (int)(iaddr & 0x0F));
        snprintf(scommentbuf, sizeof(scommentbuf), "Shift D%d Right",
                (int)(iaddr & 0x0F));
        src = srcbuf;
        scomment = scommentbuf;
        break;

    case CSIRAC_SRC_ZERO:
        src = "(Z)";
        scomment = "Load Zero";
        const_load = 1;
        break;

    case CSIRAC_SRC_H_LOWER:
        src = "(Hl)";
        scomment = "Read Lower H";
        break;

    case CSIRAC_SRC_H_UPPER:
        src = "(Hu)";
        scomment = "Read Upper H";
        break;

    case CSIRAC_SRC_S:
        src = "(S)";
        scomment = "Read S";
        break;

    case CSIRAC_SRC_P11:
        src = "p11";
        scomment = "Load P11";
        const_load = 1;
        break;

    case CSIRAC_SRC_P1:
        src = "p1";
        scomment = "Load P1";
        break;

    case CSIRAC_SRC_I:
        if (param != -1) {
            snprintf(srcbuf, sizeof(srcbuf), "R%d + %d, %d",
                     param, (int)(iaddr >> 5), (int)(iaddr & 0x1F));
            snprintf(scommentbuf, sizeof(scommentbuf),
                     "Load R%d + (%d, %d)",
                     param, (int)(iaddr >> 5), (int)(iaddr & 0x1F));
        } else {
            snprintf(srcbuf, sizeof(srcbuf), "%d, %d",
                     (int)(iaddr >> 5), (int)(iaddr & 0x1F));
            snprintf(scommentbuf, sizeof(scommentbuf), "Load %d", (int)iaddr);
        }
        src = srcbuf;
        scomment = scommentbuf;
        const_load = 1;
        break;

    case CSIRAC_SRC_DRUM_MEMORY_1:
        snprintf(srcbuf, sizeof(srcbuf), "(%d, %d a)",
                 (int)(iaddr >> 5), (int)(iaddr & 0x1F));
        snprintf(scommentbuf, sizeof(scommentbuf), "Read Drum 1 at %d", iaddr);
        src = srcbuf;
        scomment = scommentbuf;
        break;

    case CSIRAC_SRC_DRUM_MEMORY_2:
        snprintf(srcbuf, sizeof(srcbuf), "(%d, %d b)",
                 (int)(iaddr >> 5), (int)(iaddr & 0x1F));
        snprintf(scommentbuf, sizeof(scommentbuf), "Read Drum 2 at %d", iaddr);
        src = srcbuf;
        scomment = scommentbuf;
        break;

    case CSIRAC_SRC_DRUM_MEMORY_3:
        snprintf(srcbuf, sizeof(srcbuf), "(%d, %d c)",
                 (int)(iaddr >> 5), (int)(iaddr & 0x1F));
        snprintf(scommentbuf, sizeof(scommentbuf), "Read Drum 3 at %d", iaddr);
        src = srcbuf;
        scomment = scommentbuf;
        break;

    case CSIRAC_SRC_DRUM_MEMORY_4:
        snprintf(srcbuf, sizeof(srcbuf), "(%d, %d d)",
                 (int)(iaddr >> 5), (int)(iaddr & 0x1F));
        snprintf(scommentbuf, sizeof(scommentbuf), "Read Drum 4 at %d", iaddr);
        src = srcbuf;
        scomment = scommentbuf;
        break;

    case CSIRAC_SRC_P20:
        src = "p20";
        scomment = "Load P20";
        const_load = 1;
        break;

    default:
        src = "?";
        break;
    }

    /* Decode the destination */
    dcomment = NULL;
    switch (insn & 0x1F) {
    case CSIRAC_DEST_MAIN_MEMORY:
        if (param != -1) {
            snprintf(destbuf, sizeof(destbuf), "R%d + %d, %d M",
                     param, (int)(iaddr >> 5), (int)(iaddr & 0x1F));
            snprintf(dcommentbuf, sizeof(dcommentbuf),
                     "Write to Memory at R%d + %d", param, iaddr);
        } else {
            snprintf(destbuf, sizeof(destbuf), "%d, %d M",
                     (int)(iaddr >> 5), (int)(iaddr & 0x1F));
            snprintf(dcommentbuf, sizeof(dcommentbuf),
                     "Write to Memory at %d", iaddr);
        }
        dest = destbuf;
        dcomment = dcommentbuf;
        break;

    case CSIRAC_DEST_NOP_I:
        dest = "I";
        dcomment = "No Operation";
        break;

    case CSIRAC_DEST_TELEPRINTER:
        dest = "Ot";
        dcomment = "Write to Teleprinter";
        break;

    case CSIRAC_DEST_PUNCH_TAPE:
        dest = "Op";
        dcomment = "Punch to Output Tape";
        break;

    case CSIRAC_DEST_A:
        dest = "A";
        dcomment = "Write to A";
        break;

    case CSIRAC_DEST_A_PLUS:
        dest = "+A";
        dcomment = "Add to A";
        break;

    case CSIRAC_DEST_A_MINUS:
        dest = "-A";
        dcomment = "Subtract from A";
        break;

    case CSIRAC_DEST_A_AND:
        dest = ".A";
        dcomment = "AND with A";
        break;

    case CSIRAC_DEST_A_OR:
        dest = "vA";
        dcomment = "OR with A";
        break;

    case CSIRAC_DEST_A_XOR:
        dest = "~A";
        dcomment = "XOR with A";
        break;

    case CSIRAC_DEST_LOUDSPEAKER:
        dest = "P";
        dcomment = "Write to Loudspeaker";
        break;

    case CSIRAC_DEST_B:
        dest = "B";
        dcomment = "Write to B";
        break;

    case CSIRAC_DEST_B_TIMES:
        dest = "xB";
        dcomment = "Multiply";
        break;

    case CSIRAC_DEST_CYCLIC_SHIFT:
        dest = "L";
        dcomment = "Left Cyclic Shift";
        break;

    case CSIRAC_DEST_C:
        dest = "C";
        dcomment = "Write to C";
        break;

    case CSIRAC_DEST_C_PLUS:
        dest = "+C";
        dcomment = "Add to C";
        break;

    case CSIRAC_DEST_C_MINUS:
        dest = "-C";
        dcomment = "Subtract from C";
        break;

    case CSIRAC_DEST_D:
        snprintf(destbuf, sizeof(destbuf), "D%d", (int)(iaddr & 0x0F));
        snprintf(dcommentbuf, sizeof(dcommentbuf), "Write to D%d",
                (int)(iaddr & 0x0F));
        dest = destbuf;
        dcomment = dcommentbuf;
        break;

    case CSIRAC_DEST_D_PLUS:
        snprintf(destbuf, sizeof(destbuf), "+D%d", (int)(iaddr & 0x0F));
        snprintf(dcommentbuf, sizeof(dcommentbuf), "Add to D%d",
                (int)(iaddr & 0x0F));
        dest = destbuf;
        dcomment = dcommentbuf;
        break;

    case CSIRAC_DEST_D_MINUS:
        snprintf(destbuf, sizeof(destbuf), "-D%d", (int)(iaddr & 0x0F));
        snprintf(dcommentbuf, sizeof(dcommentbuf), "Subtract from D%d",
                (int)(iaddr & 0x0F));
        dest = destbuf;
        dcomment = dcommentbuf;
        break;

    case CSIRAC_DEST_NOP_Z:
        dest = "Z";
        dcomment = "No Operation";
        break;

    case CSIRAC_DEST_H_LOWER:
        dest = "Hl";
        dcomment = "Write to Lower H";
        break;

    case CSIRAC_DEST_H_UPPER:
        dest = "Hu";
        dcomment = "Write to Upper H";
        break;

    case CSIRAC_DEST_S:
        dest = "S";
        if (const_load) {
            /* If we had a constant load, then this is an absolute jump */
            scomment = NULL;
            if (address != 0xFFFF) {
                snprintf(dcommentbuf, sizeof(dcommentbuf),
                        "Jump to %d", (int)iaddr);
            } else if (param != -1 && iaddr == 0) {
                snprintf(dcommentbuf, sizeof(dcommentbuf),
                        "Jump to R%d", param);
            } else if (param != -1) {
                snprintf(dcommentbuf, sizeof(dcommentbuf),
                        "Jump to R%d + (%d, %d)",
                        param,
                        (int)((iaddr >> 5) & 0x1F),
                        (int)(iaddr & 0x1F));
            } else {
                snprintf(dcommentbuf, sizeof(dcommentbuf),
                        "Jump to (%d, %d)",
                        (int)((iaddr >> 5) & 0x1F),
                        (int)(iaddr & 0x1F));
            }
            dcomment = dcommentbuf;
        } else {
            dcomment = "Jump Indirect";
        }
        break;

    case CSIRAC_DEST_S_PLUS:
        dest = "+S";
        if (const_load) {
            /* If we had a constant load, then this is a relative jump */
            scomment = NULL;
            if (address != 0xFFFF) {
                snprintf(dcommentbuf, sizeof(dcommentbuf),
                        "Jump Relative to %d",
                        (int)((relative_base + iaddr + 1) & 0x03FF));
            } else {
                uint16_t target = relative_base + iaddr + 1;
                snprintf(dcommentbuf, sizeof(dcommentbuf),
                        "Jump Relative to (%d, %d)",
                        (int)((target >> 5) & 0x1F),
                        (int)(target & 0x1F));
            }
            dcomment = dcommentbuf;
        } else {
            dcomment = "Jump Relative Indirect";
        }
        break;

    case CSIRAC_DEST_S_SKIP:
        dest = "cS";
        dcomment = "Skip if Non-Zero";
        break;

    case CSIRAC_DEST_INSTRUCTION_ADD:
        dest = "+K";
        dcomment = "Add to Next Instruction";
        break;

    case CSIRAC_DEST_DRUM_MEMORY_1:
        snprintf(destbuf, sizeof(destbuf), "(%d, %d a)",
                 (int)(iaddr >> 5), (int)(iaddr & 0x1F));
        snprintf(dcommentbuf, sizeof(dcommentbuf),
                 "Write to Drum 1 at %d", iaddr);
        dest = destbuf;
        dcomment = dcommentbuf;
        break;

    case CSIRAC_DEST_DRUM_MEMORY_2:
        snprintf(destbuf, sizeof(destbuf), "(%d, %d b)",
                 (int)(iaddr >> 5), (int)(iaddr & 0x1F));
        snprintf(dcommentbuf, sizeof(dcommentbuf),
                 "Write to Drum 2 at %d", iaddr);
        dest = destbuf;
        dcomment = dcommentbuf;
        break;

    case CSIRAC_DEST_DRUM_MEMORY_3:
        snprintf(destbuf, sizeof(destbuf), "(%d, %d c)",
                 (int)(iaddr >> 5), (int)(iaddr & 0x1F));
        snprintf(dcommentbuf, sizeof(dcommentbuf),
                 "Write to Drum 3 at %d", iaddr);
        dest = destbuf;
        dcomment = dcommentbuf;
        break;

    case CSIRAC_DEST_DRUM_MEMORY_4:
        snprintf(destbuf, sizeof(destbuf), "(%d, %d d)",
                 (int)(iaddr >> 5), (int)(iaddr & 0x1F));
        snprintf(dcommentbuf, sizeof(dcommentbuf),
                 "Write to Drum 4 at %d", iaddr);
        dest = destbuf;
        dcomment = dcommentbuf;
        break;

    case CSIRAC_DEST_STOP:
        dest = "T";
        if (const_load && iaddr != 0) {
            scomment = NULL;
            dcomment = "Halt";
        } else {
            dcomment = "Halt if Non-Zero";
        }
        break;

    default:
        dest = "?";
        break;
    }

    /* Print the instruction */
    if (address != 0xFFFF) {
        printf("%10s -> %-10s", src, dest);
    } else {
        printf("%-15s -> %-15s", src, dest);
    }
    if (scomment) {
        printf(" ; %s", scomment);
        if (dcomment) {
            printf(", %s", dcomment);
        }
    } else if (dcomment) {
        printf(" ; %s", dcomment);
    }
    printf("\n");
}

static void disassemble(csirac_state_t *machine, uint16_t address)
{
    csirac_word_t P[1024] = {0}; /* Parameter values for relocations */
    csirac_word_t insn = 0;
    csirac_word_t control = 0;
    csirac_word_t next;
    csirac_word_t word;
    int primary = 1;
    int perform = 0;

    /* We are done once we run out of tapes */
    while (machine->input_tape != NULL) {
        if (!csirac_media_get(machine, &word)) {
            break;
        }
        if (primary) {
            /* Words from the Primary are not X-punched, so add the X */
            word |= CSIRAC_WORD_X;
        }
        if ((word & CSIRAC_WORD_Y) != 0) {
            /* Control command */
            insn += (word & CSIRAC_HALF_WORD_MASK);
            if (primary) {
                /* "0, 0, m, n Y" is the first Y-punched command.  It resets
                 * the origin to the lower 10-bits and exits the primary. */
                address = insn & CSIRAC_HALF_WORD_MASK;
                printf("\nChange Origin to %d [%d, %d]:\n",
                       (int)address, (int)((address >> 5) & 0x1F),
                       (int)(address & 0x1F));
                primary = 0;
                insn = 0;
                continue;
            }
            next = 0;
            if ((insn & 0x1F) == 4) {
                /* "0, 0, 0, 4 Y" repeats the previous control command */
                insn = control;
            } else {
                /* Save this control instruction for the next "4Y" */
                control = insn;
            }
            switch (insn & 0x1F) {
            case 0:
                /* "m, n, 0, 0 Y" changes the current address to m, n */
                address = insn >> 10;
                printf("\nChange Origin to %d [%d, %d]:\n",
                       (int)address, (int)((address >> 5) & 0x1F),
                       (int)(address & 0x1F));
                break;

            case 1:
                /* "0, n, 0, 1 Y" stores the current address as the
                 * relocation parameter n */
                P[(insn >> 10) & 0x03FF] = ((csirac_word_t)address) << 10;
                break;

            case 2:
                /* "0, n, 0, 2 Y" adds the relocation parameter n to the
                 * next instruction we encounter */
                next = P[(insn >> 10) & 0x03FF];
                break;

            case 6:
                /* "0, 0, 0, 6 Y" performs the next command rather than
                 * stores it into the program.  Normally used for jump
                 * instructions to start running the code or to jump to a
                 * second-stage bootstrap. */
                perform = 1;
                printf("\n");
                break;

            default: break;
            }
            insn = next;
        } else if ((word & CSIRAC_WORD_X) != 0) {
            /* Ignore the word if it is zero and we are in the primary.
             * Zero words in the primary are padding. */
            if (primary && (word & ~CSIRAC_WORD_X) == 0) {
                insn = 0;
                continue;
            }

            /* We now have a full instruction to be decoded */
            insn += (word & CSIRAC_HALF_WORD_MASK);
            decode_insn(address, address, insn & CSIRAC_WORD_MASK, perform, -1);
            if (perform) {
                perform = 0;
            } else {
                ++address;
            }

            /* Clear the in-progress instruction for the next one */
            insn = 0;
        } else {
            /* This is the top part of the current instruction.
             * The next word on the tape will be the bottom part. */
            insn += (word << 10);
        }
    }
}

static void dump_flexowriter_char(unsigned char ch, int *letter_shift)
{
    static const char * const flexowriter_to_string[64] = {
        /* Letter shift symbols */
        "<NUL>",    /* Blank */
        "Q", "W", "C", "R", "K", "L", "U", "I", "D", "V",
        "A", "F", "M", "G", "N", "P", "J", "H", "E", "B",
        "T", "Y", "S", "X", "O", "Z",
        "<FS>",     /* Figure shift */
        " ",        /* Space */
        "<CR>",     /* Carriage return */
        "<LS>",     /* Letter shift */
        "<LF>",     /* Line feed */

        /* Figure shift symbols */
        "<NUL>",    /* Blank */
        "1", "2", "*", "4", "(", ")", "7", "8", "#", "=",
        "-", "&", ".",
        "<TAB>",    /* TAB */
        ",", "0",
        "s",        /* STOP */
        "\xC2\xA3", /* U+00A3 - POUND SIGN */
        "3", "'", "5", "6", "/", "x", "9", "+",
        "<FS>",     /* Figure shift */
        " ",        /* Space */
        "<CR>",     /* Carriage return */
        "<LS>",     /* Letter shift */
        "<LF>"      /* Line feed */
    };
    if (*letter_shift) {
        printf("%s", flexowriter_to_string[ch]);
    } else {
        printf("%s", flexowriter_to_string[ch + 32]);
    }
    if (ch == 0x1E) {
        *letter_shift = 1;
    } else if (ch == 0x1B) {
        *letter_shift = 0;
    }
}

static void disassemble_mnemonic(csirac_state_t *machine, int mode)
{
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
    csirac_word_t insn = 0;
    csirac_word_t word;
    int primary = 1;
    int param = -1;
    int perform = 0;
    int literals = 0;
    int letter_shift = 1;
    uint16_t address = 0;
    uint16_t real_address = 0;
    int field;

    /* We are done once we run out of tapes */
    while (machine->input_tape != NULL) {
        if (!csirac_media_get(machine, &word)) {
            break;
        }
        if (primary) {
            /* Words from the Primary are not X-punched, so add the X */
            word |= CSIRAC_WORD_X;
        }
        if ((word & CSIRAC_WORD_Y) != 0) {
            /* Control command */
            insn += (word & CSIRAC_HALF_WORD_MASK);
            if (primary) {
                /* "0, 0, m, n Y" is the first Y-punched command.  It resets
                 * the origin to the lower 10-bits and exits the primary. */
                if (mode > 1) {
                    printf("        .org (%d, %d)\n",
                           (int)((insn >> 5) & 0x1F), (int)(insn & 0x1F));
                } else {
                    printf("          %2d %2dY\n",
                           (int)((insn >> 5) & 0x1F), (int)(insn & 0x1F));
                }
                primary = 0;
                insn = 0;
                address = 0;
                real_address = (insn & 0x3FF);
                continue;
            }
            switch (insn & 0x1F) {
            case 0:
                /* "m, n, 0, 0 Y" changes the current address to m, n */
                if (mode > 1) {
                    printf("        .org (%d, %d)\n",
                           (int)((insn >> 15) & 0x1F),
                           (int)((insn >> 10) & 0x1F));
                } else {
                    printf("          %2d %2dT\n",
                           (int)((insn >> 15) & 0x1F),
                           (int)((insn >> 10) & 0x1F));
                }
                real_address = ((insn >> 10) & 0x3FF);
                address = real_address;
                break;

            case 1:
                /* "0, n, 0, 1 Y" stores the current address as the
                 * relocation parameter n */
                if (mode > 1) {
                    printf("        .set R%-2d = (%d, %d)\n",
                           (int)((insn >> 10) & 0x03FF),
                           (int)((real_address >> 5) & 0x1F),
                           (int)(real_address & 0x1F));
                } else {
                    printf("             %2dS\n", (int)((insn >> 10) & 0x03FF));
                }

                /* Also reset the instruction address to zero.  We are
                 * starting a new relocatable subroutine. */
                if (mode < 2) {
                    address = 0;
                } else {
                    address = real_address;
                }
                break;

            case 2:
                /* "0, n, 0, 2 Y" adds the relocation parameter n to the
                 * next instruction we encounter */
                param = (int)((insn >> 10) & 0x03FF);
                break;

            case 3:
                /* "0, 0, 0, 3 Y" starts a literal data block */
                if (mode > 1) {
                    printf("        .literal start\n");
                    literals = 1;
                    letter_shift = 1;
                } else {
                    printf("          %2d %2dY\n",
                           (int)((insn >> 5) & 0x1F), (int)(insn & 0x1F));
                }
                break;

            case 4:
                /* "0, 0, 0, 4 Y" repeats the previous control command */
                if (mode > 1) {
                    printf("        .repeat\n");
                } else {
                    printf("               R\n");
                }
                break;

            case 6:
                /* "0, 0, 0, 6 Y" performs the next command rather than
                 * stores it into the program.  Normally used for jump
                 * instructions to start running the code or to jump to a
                 * second-stage bootstrap. */
                perform = 1;
                break;

            case 7:
                /* "0, 0, 0, 7 Y" ends a literal data block */
                if (mode > 1) {
                    printf("        .literal end\n");
                    literals = 0;
                } else {
                    printf("          %2d %2dY\n",
                           (int)((insn >> 5) & 0x1F), (int)(insn & 0x1F));
                }
                break;

            default:
                /* Some other command - print it as a generic Y-punch */
                if (mode > 1) {
                    printf("        .command %d, %d\n",
                           (int)((insn >> 5) & 0x1F), (int)(insn & 0x1F));
                } else {
                    printf("          %2d %2dY\n",
                           (int)((insn >> 5) & 0x1F), (int)(insn & 0x1F));
                }
                break;
            }
            insn = 0;
        } else if ((word & CSIRAC_WORD_X) != 0) {
            /* Ignore the word if it is zero - such words are tape padding */
            if ((word & ~CSIRAC_WORD_X) == 0) {
                if (mode <= 1) {
                    printf("\n");
                }
                insn = 0;
                continue;
            }

            /* We now have a full instruction to be decoded */
            insn += (word & CSIRAC_HALF_WORD_MASK);

            /* Print the address of the instruction if not performing it */
            if (!perform) {
                printf("%2d %2d ",
                       (int)((address >> 5) & 0x1F), (int)(address & 0x1F));
            } else {
                printf("      ");
            }

            /* What mode are we printing in? */
            if (mode <= 1) {
                /* Print the relocation parameter, if any */
                if (param >= 0) {
                    printf("%2dA", param);
                } else {
                    printf("   ");
                }

                /* Print the address part of the instruction, skipping zeroes */
                field = (int)((insn >> 15) & 0x1F);
                if (field != 0) {
                    printf(" %2d", field);
                } else {
                    printf("   ");
                }
                field = (int)((insn >> 10) & 0x1F);
                if (field != 0) {
                    printf(" %2d", field);
                } else {
                    printf("   ");
                }

                /* If we are performing this instruction, output a D */
                if (perform) {
                    putc('D', stdout);
                } else {
                    putc(' ', stdout);
                }

                /* Print the source field */
                field = (int)((insn >> 5) & 0x1F);
                printf("%2s ", sources[field]);

                /* Print the destination field */
                field = (int)(insn & 0x1F);
                printf("%2s\n", destinations[field]);
            } else {
                /* Decode the instruction using old-style CSIRAC syntax */
                if (literals) {
                    /* Dump a block of literals, with Flexowriter decoding */
                    unsigned char codes[4] = {
                        (insn >> 15) & 0x1F,
                        (insn >> 10) & 0x1F,
                        (insn >>  5) & 0x1F,
                        (insn & 0x1F)
                    };
                    printf("  .literal %2d, %2d, %2d, %2d  ; \"",
                           codes[0], codes[1], codes[2], codes[3]);
                    dump_flexowriter_char(codes[0], &letter_shift);
                    dump_flexowriter_char(codes[1], &letter_shift);
                    dump_flexowriter_char(codes[2], &letter_shift);
                    dump_flexowriter_char(codes[3], &letter_shift);
                    printf("\"\n");
                } else {
                    if (perform) {
                        printf("  .perform\n        ");
                    } else {
                        printf("  ");
                    }
                    decode_insn(0xFFFF, address, insn, 0, param);
                }
            }
            perform = 0;
            param = -1;

            /* Increment the address of the instruction */
            ++address;
            ++real_address;

            /* Clear the in-progress instruction for the next one */
            insn = 0;
        } else {
            /* This is the top part of the current instruction.
             * The next word on the tape will be the bottom part. */
            insn += (word << 10);
        }
    }
}
