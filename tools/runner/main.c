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
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define short_options "pxaf:t:h:e:w:s:F:T:H:E:W:S:n:m:d:cL:v"
static struct option long_options[] = {
    {"bootstrap",       no_argument,       0, 'p'},
    {"tty-is-flexo",    no_argument,       0, 'x'},
    {"tty-is-ascii",    no_argument,       0, 'a'},

    {"flexo-punch",     required_argument, 0, 'f'},
    {"tty-punch",       required_argument, 0, 't'},
    {"5-hole-punch",    required_argument, 0, 'h'},
    {"8-hole-punch",    required_argument, 0, 'e'},
    {"12-hole-output",  required_argument, 0, 'w'},
    {"ascii-punch",     required_argument, 0, 's'},

    {"flexo-tape",      required_argument, 0, 'F'},
    {"tty-tape",        required_argument, 0, 'T'},
    {"5-hole-tape",     required_argument, 0, 'H'},
    {"8-hole-tape",     required_argument, 0, 'E'},
    {"12-hole-tape",    required_argument, 0, 'W'},
    {"ascii-tape",      required_argument, 0, 'S'},

    {"n1",              required_argument, 0, 'n'},
    {"n2",              required_argument, 0, 'm'},

    {"drum",            required_argument, 0, 'd'},
    {"clear-drum",      no_argument,       0, 'c'},

    {"library-dir",     required_argument, 0, 'L'},

    {"verbose",         no_argument,       0, 'v'},

    {"help",            no_argument,       0, '?'},

    {0,                 0,                 0,   0}
};

static void usage(const char *progname);

static csirac_state_t machine;

int main(int argc, char *argv[])
{
    const char *progname = argv[0];
    int bootstrap = 0;
    int clear_drum = 0;
    const char *drum_file = NULL;
    const char *library_dir = NULL;
    int opt, index, ok;
    int exitval = 0;

    /* Initialise the machine state */
    csirac_init(&machine);

    /* Add a teleprinter on stdout as the default output device */
    csirac_open_teleprinter(&machine, CSIRAC_MEDIA_TELEPRINTER, NULL);

    /* Parse the command-line arguments */
    while ((opt = getopt_long
                (argc, argv, short_options, long_options, &index)) != -1) {
        switch (opt) {
        case 'p':
            /* Insert the primary and control routines at the start */
            bootstrap = 1;
            ok = 1;
            break;

        case 'x':
            /* Teleprinter output is actually to a Flexowriter */
            optarg = "Flexowriter";
            ok = csirac_open_teleprinter
                (&machine, CSIRAC_MEDIA_FLEXOWRITER, NULL);
            break;

        case 'a':
            /* Teleprinter output is actually to an ASCII teletype */
            optarg = "ASCII";
            ok = csirac_open_teleprinter
                (&machine, CSIRAC_MEDIA_ASCII, NULL);
            break;

        case 'f':
            /* Output punch is a Flexowriter */
            ok = csirac_open_output_punch
                (&machine, CSIRAC_MEDIA_FLEXOWRITER, optarg);
            break;

        case 't':
            /* Output punch is a teleprinter */
            ok = csirac_open_output_punch
                (&machine, CSIRAC_MEDIA_TELEPRINTER, optarg);
            break;

        case 'h':
            /* Output punch is 5-hole punch tape */
            ok = csirac_open_output_punch
                (&machine, CSIRAC_MEDIA_5HOLE_TAPE, optarg);
            break;

        case 'e':
            /* Output punch is 8-hole punch tape */
            ok = csirac_open_output_punch
                (&machine, CSIRAC_MEDIA_8HOLE_TAPE, optarg);
            break;

        case 'w':
            /* Output punch is 12-hole punch tape */
            ok = csirac_open_output_punch
                (&machine, CSIRAC_MEDIA_12HOLE_TAPE, optarg);
            break;

        case 'F':
            /* Input tape is a Flexowriter */
            ok = csirac_open_input_tape
                (&machine, CSIRAC_MEDIA_FLEXOWRITER, optarg);
            break;

        case 'T':
            /* Input tape is a teleprinter */
            ok = csirac_open_input_tape
                (&machine, CSIRAC_MEDIA_TELEPRINTER, optarg);
            break;

        case 'H':
            /* Input tape is 5-hole punch tape */
            ok = csirac_open_input_tape
                (&machine, CSIRAC_MEDIA_5HOLE_TAPE, optarg);
            break;

        case 'E':
            /* Input tape is 8-hole punch tape */
            ok = csirac_open_input_tape
                (&machine, CSIRAC_MEDIA_8HOLE_TAPE, optarg);
            break;

        case 'W':
            /* Input tape is 12-hole punch tape */
            ok = csirac_open_input_tape
                (&machine, CSIRAC_MEDIA_12HOLE_TAPE, optarg);
            break;

        case 'n':
            /* Set the value of N1 */
            machine.N1 = (csirac_word_t)(strtol(optarg, NULL, 0));
            machine.N1 &= CSIRAC_WORD_MASK;
            ok = 1;
            break;

        case 'm':
            /* Set the value of N2 */
            machine.N2 = (csirac_word_t)(strtol(optarg, NULL, 0));
            machine.N2 &= CSIRAC_WORD_MASK;
            ok = 1;
            break;

        case 'd':
            /* Specify the name of the file to persist the magnetic drum */
            drum_file = optarg;
            ok = 1;
            break;

        case 'c':
            /* Clear the magnetic drum's contents at startup */
            clear_drum = 1;
            ok = 1;
            break;

        case 'L':
            /* Set the location of the CSIRAC library */
            library_dir = optarg;
            ok = 1;
            break;

        case 'v':
            /* Activate verbose mode */
            machine.dump_instructions = 1;
            ok = 1;
            break;

        default:
            usage(progname);
            csirac_free(&machine);
            return 1;
        }
        if (!ok) {
            /* Could not open one of the input/output files */
            perror(optarg);
            csirac_free(&machine);
            return 1;
        }
    }

    /* If there is no output punch, then add a default 5-hole punch on stdout */
    if (machine.output_punch == NULL) {
        ok = csirac_open_output_punch
            (&machine, CSIRAC_MEDIA_5HOLE_TAPE, NULL);
        if (!ok) {
            perror("stdout");
            csirac_free(&machine);
            return 1;
        }
    }

    /* Add the program tape sources in non-option arguments.
     * They are all assumed to be 12-hole punch tapes.  Data files
     * should be provided via the regular options. */
    for (index = optind; index < argc; ++index) {
        ok = csirac_open_input_tape
            (&machine, CSIRAC_MEDIA_12HOLE_TAPE, argv[index]);
        if (!ok && errno == ENOENT && argv[index][0] == 'T' &&
                argv[index][1] >= '0' && argv[index][1] <= '9' &&
                argv[index][2] >= '0' && argv[index][2] <= '9' &&
                argv[index][3] >= '0' && argv[index][3] <= '9') {
            /* Not found, but it may be a file from the CSIRAC library */
            char filename[BUFSIZ];
            if (!library_dir) {
                library_dir = getenv("CSIRAC_LIBRARY_DIR");
                if (!library_dir) {
#if defined(CSIRAC_LIBRARY_DIR)
                    library_dir = CSIRAC_LIBRARY_DIR;
#else
                    fprintf(stderr, "%s: not found, try setting CSIRAC_LIBRARY_DIR\n",
                            argv[index]);
                    csirac_free(&machine);
                    return 1;
#endif
                }
            }
            snprintf(filename, sizeof(filename), "%s/%s/%s.cvt",
                     library_dir, argv[index], argv[index]);
            ok = csirac_open_input_tape
                (&machine, CSIRAC_MEDIA_12HOLE_TAPE, filename);
        }
        if (!ok) {
            perror(argv[index]);
            csirac_free(&machine);
            return 1;
        }
    }

    /* Move the non-option tape sources to the front of the list
     * because they need to be loaded first. */
    for (index = optind; index < argc; ++index) {
        csirac_move_input_tape_to_front(&machine);
    }

    /* If there are no input tapes, or the first tape is not 12-hole,
     * then we do not have a program that we can run. */
    if (machine.input_tape == NULL ||
            machine.input_tape->type != CSIRAC_MEDIA_12HOLE_TAPE) {
        fprintf(stderr, "%s: no program to run\n", progname);
        csirac_free(&machine);
        return 1;
    }

    /* Load the contents of the magnetic drum file */
    if (drum_file != NULL) {
        if (!csirac_load_drum(&machine, drum_file)) {
            /* If the file does not exist, treat it as an empty drum
             * if the "--clear-drum" option was supplied. */
            if (errno != ENOENT || !clear_drum) {
                perror(drum_file);
                csirac_free(&machine);
                return 1;
            }
        }
    }
    if (clear_drum) {
        memset(machine.MD, 0, sizeof(machine.MD));
    }

    /* Load the primary routine into memory */
    if (bootstrap) {
        /* Bootstrap the primary ourselves */
        size_t posn;
        index = 0;
        for (posn = 0; posn < csirac_short_primary_routine_len; ++posn, ++index) {
            machine.M[posn] = csirac_primary_routine[posn];
        }
        index = (int)posn;
        for (posn = 0; posn < csirac_control_routine_len; ++posn, ++index) {
            machine.M[index] = csirac_control_routine[posn];
        }

        /* Set the next address for the primary to populate */
        machine.C = index;
    } else {
        /* Load the primary from the tape.  We need the first 18
         * non-zero words from the tape.  After those are loaded,
         * the primary can take care of loading the rest.  Stop
         * after 100 words for safety in case it is all zeroes. */
        for (index = 0, opt = 100; index < 18 && opt > 0; --opt) {
            csirac_word_t word;
            if (!csirac_media_get(&machine, &word)) {
                /* There are less than 18 words on the tape */
                opt = 0;
                break;
            }
            if (word != 0) {
                machine.M[index++] = word;
            }
        }

        /* Did we find something that looks like the primary routine? */
        if (opt <= 0 || machine.M[0] != CSIRAC_WORD(0, 0, 17, 18)) {
            fprintf(stderr, "%s: no primary routine found\n", progname);
            csirac_free(&machine);
            return 1;
        }
    }

    /* Run the program until we reach halt or hoot */
    switch (csirac_run(&machine)) {
    case CSIRAC_STEP_HOOT:
        fprintf(stderr, "HOOT!\n");
        break;

    case CSIRAC_STEP_TAPE_EXHAUSTED:
        fprintf(stderr, "The input tape has been exhausted.\n");
        exitval = 1;
        break;

    case CSIRAC_STEP_ILLEGAL_INSTRUCTION:
        fprintf(stderr, "Illegal instruction at address %d\n",
                (int)(((machine.S - 1) >> 10) & CSIRAC_HALF_WORD_MASK));
        exitval = 1;
        break;

    case CSIRAC_STEP_BREAKPOINT:
        fprintf(stderr, "Breakpoint at address %d\n",
                (int)(((machine.S - 1) >> 10) & CSIRAC_HALF_WORD_MASK));
        exitval = 1;
        break;

    default:
        break;
    }

    /* Persist the magnetic drum if necessary */
    if (drum_file != NULL) {
        if (!csirac_save_drum(&machine, drum_file)) {
            perror(drum_file);
            exitval = 1;
        }
    }

    /* Free the machine state */
    csirac_free(&machine);
    return exitval;
}

static void usage(const char *progname)
{
    printf("Usage: %s [options] program-tape ...\n\n", progname);

    printf("    --bootstrap, -p\n");
    printf("        Load the primary and control routines before the program.\n\n");

    printf("    --tty-is-flexo, -x\n");
    printf("        The teleprinter output should use Flexowriter codes.\n\n");

    printf("    --tty-is-ascii, -a\n");
    printf("        The teleprinter output should use ASCII codes.\n\n");

    printf("    --flexo-punch FILE, -f FILE\n");
    printf("        Output punch FILE is a Flexowriter.\n\n");

    printf("    --tty-punch FILE, -t FILE\n");
    printf("        Output punch FILE is a teleprinter.\n\n");

    printf("    --5-hole-punch FILE, -h FILE\n");
    printf("        Output punch FILE is a 5-hole punch.\n\n");

    printf("    --8-hole-punch FILE, -e FILE\n");
    printf("        Output punch FILE is a 8-hole punch (binary output mode).\n\n");

    printf("    --12-hole-punch FILE, -w FILE\n");
    printf("        Output punch FILE is a 12-hole punch (.cvt format).\n\n");

    printf("    --ascii-punch FILE, -s FILE\n");
    printf("        Output punch FILE is an ASCII (or binary) data stream.\n\n");

    printf("    --flexo-tape FILE, -F FILE\n");
    printf("        Add FILE as an input tape that uses Flexowriter codes.\n\n");

    printf("    --tty-tape FILE, -T FILE\n");
    printf("        Add FILE as an input tape that uses teleprinter codes.\n\n");

    printf("    --5-hole-tape FILE, -H FILE\n");
    printf("        Add FILE as an input tape that uses 5-hole punch codes.\n\n");

    printf("    --8-hole-tape FILE, -E FILE\n");
    printf("        Add FILE as an input tape that uses 8-hole punch codes.\n\n");

    printf("    --12-hole-tape FILE, -W FILE\n");
    printf("        Add FILE as an input tape that uses 12-hole punch codes (.cvt format).\n\n");

    printf("    --n1 VALUE, -n VALUE\n");
    printf("        Set the integer VALUE as the N1 input switches.\n\n");

    printf("    --n2 VALUE, -m VALUE\n");
    printf("        Set the integer VALUE as the N2 input switches.\n\n");

    printf("    --drum FILE, -d FILE\n");
    printf("        Specify a FILE that persists the magnetic drum's contents.\n");
    printf("        FILE is read at startup and overwritten at shutdown.\n\n");

    printf("    --clear-drum, -c\n");
    printf("        Clear the magnetic drum's contents at startup.\n\n");

    printf("    --library-dir DIR, -L DIR\n");
    printf("        Set the directory where the CSIRAC library can be found.\n\n");

    printf("    --verbose, -v\n");
    printf("        Dump instructions as they are being executed.\n\n");

    printf("Program tapes that are provided as regular arguments must be in\n");
    printf("12-hole punch tape format (.cvt format).\n");
}
