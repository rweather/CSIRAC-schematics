CSIRAC Library
==============

The CSIRAC Programming Manual contains a list of the routines that
were in the CSIRAC Library as of June 1958 in an Appendix.

It is hard to find the library.  Much of it was digitised in the 1990's as
".cvt" (CSIRAC Virtual Tape) and ".tsp" (Tape Symbol Print) files but the
collection isn't online anywhere I could find.  Let me know if you find it.
Some fragments came with the
[Windows 98 emulator](https://cis.unimelb.edu.au/about/csirac/emulator).

I have attempted to reproduce some of the library where the code is
obvious from the description.

Each directory contains a ".cvt" file for the loadable 12-hole punch
tape image, a ".tsp" disassembly of the ".cvt" file, and often a
".cna" file for source code that is designed for my "CSIRAC New Assembler".

## T000 - Executive and Tests

### T001 - Primary and Control

This is the code for the "Primary and Control" routine that is placed
at the beginning of program tapes.  Together they take care of loading
the rest of the tape into memory and relocating it.

This is the original code which was stored in box B001.

### T002 - Tape Symbol Print

Reads a program tape and dumps it in "Melbourne Mnemonic Form" to the
teleprinter.  The data tape is the program to print in 12-hole punched form.
Using the emulator, T002 can be invoked as follows:

    csirac T002 program.cvt

Stored in box B001.

### T003 - Punch Store in Binary

Reads the contents of main memory between N1 and (N1 + N2 + 1) inclusive.
The instructions are punched onto 12-hole punch tape.  This is how you
would save a program that was already in memory.

On the original machine, the range was set using the N1 and N2 toggle
switches on the operator's console.  The starting address must be in
the upper 10 bits of N1, and the number of words to punch (minus 1)
must be in the lower 10 bits of N2.  All other bits of N1 and N2
should be zero.

The manual refers to three versions of this program, "T003.1", "T003.2",
and "T003.3":

* T003.1 dumps main memory to tape with two tape punches per memory word.
* T003.2 dumps main memory to tape with either one or two tape punches
per memory word.  If the upper half of a memory word is zero, that tape
punch will be suppressed.  This is the recommended version from the manual.
* T003.3 dumps the contents of the magnetic drum.  N1 must be in a
special form for this to work.  This version is not implemented yet.

The first 32 main memory locations are used by "T003.1" and the
first 34 main memory locations are used by "T003.2".  These locations
cannot be dumped.

Uses A, B, C, D0, and H.

Stored in box B001.

### T004 - Primary and Control located by N2

Not reproduced yet.

Stored in box B001.

### T005 - Arithmetic Test

Not reproduced yet.

Stored in box B110.

### T006 - Reader Test

Not reproduced yet.

Stored in box B110.

### T007 - Teleprinter Test

Not sure what the original did.  This library provides three versions
called "T007", "T007f", and "T007a" which dump the visible characters
of the teleprinter, Flexowriter, and ASCII character sets respectively.
They can be invoked using the emulator as follows:

    csirac T007
    csirac T007f
    csirac T007a

Stored in box B110.

See also T712A below.

### T008 - Test of Primary Facilities

Not reproduced yet.

Stored in box B110.

### T009 - Store Test

Not reproduced yet.

Stored in box B110.

### T712A - Old Teleprinter Test

This tape came from the examples for the
[Windows 98 emulator](https://cis.unimelb.edu.au/about/csirac/emulator).
It prints the full set of teleprinter characters once only.
