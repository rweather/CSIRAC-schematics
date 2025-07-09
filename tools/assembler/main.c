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
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define short_options "o:L:pPnNaAt:"
static struct option long_options[] = {
    {"output",          required_argument, 0, 'o'},
    {"library-dir",     required_argument, 0, 'L'},

    {"compile-library", no_argument,       0, 'c'},

    {"primary",         no_argument,       0, 'p'},
    {"no-primary",      no_argument,       0, 'P'},

    {"control",         no_argument,       0, 'n'},
    {"no-control",      no_argument,       0, 'N'},

    {"align",           no_argument,       0, 'a'},
    {"no-align",        no_argument,       0, 'A'},

    {"teleprinter",     required_argument, 0, 't'},

    {"help",            no_argument,       0, '?'},
    {0,                 0,                 0,   0}
};

static void usage(const char *progname);

static parser_t parser;

int main(int argc, char *argv[])
{
    const char *progname = argv[0];
    int primary = -1;
    int control = -1;
    int compile_library = 0;
    const char *output = "-";
    const char *library_dir = NULL;
    csirac_media_type_t teleprinter = CSIRAC_MEDIA_TELEPRINTER;
    int opt, index;
    int exitval = 0;
    int flags = CODE_FLAG_ALIGN;

    /* Parse the command-line arguments */
    while ((opt = getopt_long
                (argc, argv, short_options, long_options, &index)) != -1) {
        switch (opt) {
        case 'o':
            /* Set the output file */
            output = optarg;
            break;

        case 'L':
            /* Set the location of the CSIRAC library */
            library_dir = optarg;
            break;

        case 'c':
            /* Compile to a relocatable library instead of an executable */
            compile_library = 1;
            break;

        case 'p':
            /* Enable the generation of the primary routine code */
            primary = 1;
            break;

        case 'P':
            /* Disable the generation of the primary routine code */
            primary = 0;
            break;

        case 'n':
            /* Enable the generation of the control routine code */
            control = 1;
            break;

        case 'N':
            /* Disable the generation of the control routine code */
            control = 0;
            break;

        case 'a':
            /* Align functions on a group boundary */
            flags |= CODE_FLAG_ALIGN;
            break;

        case 'A':
            /* Do not align functions on a group boundary */
            flags &= ~CODE_FLAG_ALIGN;
            break;

        case 't':
            /* Set the default kind of teleprinter to use */
            if (!strcmp(optarg, "original") || !strcmp(optarg, "teleprinter")) {
                teleprinter = CSIRAC_MEDIA_TELEPRINTER;
            } else if (!strcmp(optarg, "flexowriter")) {
                teleprinter = CSIRAC_MEDIA_FLEXOWRITER;
            } else if (!strcmp(optarg, "ascii")) {
                teleprinter = CSIRAC_MEDIA_ASCII;
            } else {
                fprintf(stderr, "%s: unknown teleprinter type '%s'\n",
                        progname, optarg);
                return 1;
            }
            break;

        default:
            usage(progname);
            return 1;
        }
    }
    if (primary < 0) {
        /* Set the default for the primary option */
        primary = !compile_library;
    }
    if (control < 0) {
        /* Set the default for the control option */
        control = primary;
    } else if (!primary) {
        /* Cannot use the control routine unless we also have the primary */
        control = 0;
    }
    if (optind >= argc) {
        usage(progname);
        return 1;
    }

    /* Determine the type of bootstrap code to use in the output */
    if (primary) {
        flags |= CODE_FLAG_PRIMARY;
    }
    if (control) {
        flags |= CODE_FLAG_CONTROL;
    }

    /* Parse the input files */
    parser_init(&parser, flags);
    parser.code.teleprinter = teleprinter;
    for (index = optind; index < argc; ++index) {
        if (!parser_parse(&parser, argv[index])) {
            exitval = 1;
        }
    }

    /* Write the output file */
    if (exitval == 0) {
        if (!code_write(&(parser.code), output)) {
            exitval = 1;
        }
    }

    /* Clean up and exit */
    if (exitval != 0 && strcmp(output, "-") != 0) {
        /* Delete the output file if an error occurred */
        unlink(output);
    }
    parser_free(&parser);
    return exitval;
}

static void usage(const char *progname)
{
    printf("Usage: %s [options] input ...\n\n", progname);

    printf("    --output FILE, -o FILE\n");
    printf("        Set the location of the output FILE (default is standard output).\n\n");

    printf("    --library-dir DIR, -L DIR\n");
    printf("        Set the directory where the CSIRAC library can be found.\n\n");

    printf("    --compile-library, -c\n");
    printf("        Compile to a relocatable library instead of an executable.\n\n");

    printf("    --primary, -p\n");
    printf("        Add the primary routine before the program (default if not -c).\n\n");

    printf("    --no-primary, -P\n");
    printf("        No primary routine before the program (default if -c).\n\n");

    printf("    --control, -n\n");
    printf("        Add the control routine before the program (default if -p).\n\n");

    printf("    --no-control, -N\n");
    printf("        No control routine before the program (default if -P).\n\n");

    printf("    --align, -a\n");
    printf("        Align functions on a group boundary (default).\n\n");

    printf("    --no-align, -A\n");
    printf("        Do not align functions on a group boundary.\n\n");

    printf("    --teleprinter TYPE, -t TYPE\n");
    printf("        Type of teleprinter: 'flexowriter', 'ascii', or 'original' (default).\n\n");
}
