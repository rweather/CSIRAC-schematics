CSIRAC Examples
===============

This directory contains examples that can be run with the CSIRAC emulator.

Each of the examples is provided as a CVT file ("CSIRAC Virtual Tape")
which contains the 12-hole punch tape data for the program.

## hoot.cvt

This example "hoots" the loudspeaker at 500Hz.  Back in the day, the operators
would put the hooter sequence in the code to (loudly!) notify them that the
program had finished or it had reached an invalid state.

At the moment, the hoot isn't hooked up to sound so the emulator just prints
"HOOT!" on the standard error output.  This is the expected behaviour:

    $ csirac hoot.cvt
    HOOT!

This is the code that it is running (excluding the primary and control
routines that load the program):

    31, 31 -> P
    31, 30 -> +S

## hello.cvt

This is the classic "Hello World" example, writing to the teleprinter:

    $ csirac hello.cvt
    HELLO WORLD

The code is pretty boring, printing one character at a time to the
teleprinter and then halting:

    0,  7 -> Ot     ; Print 'H'
    0,  4 -> Ot     ; Print 'E'
    0, 11 -> Ot     ; Print 'L'
    0, 11 -> Ot     ; Print 'L'
    0, 14 -> Ot     ; Print 'O'
    0, 31 -> Ot     ; Print ' '
    0, 22 -> Ot     ; Print 'W'
    0, 14 -> Ot     ; Print 'O'
    0, 17 -> Ot     ; Print 'R'
    0, 11 -> Ot     ; Print 'L'
    0,  3 -> Ot     ; Print 'D'
    0, 30 -> Ot     ; Print CR
    0, 29 -> Ot     ; Print LF
      p20 -> T      ; Halt

## hello2.cvt

This is another example of printing strings to the teleprinter in ASCII:

    $ csirac -a hello2.cvt
    THIS IS A TEST
    HELLO WORLD

The two strings are printed using a common `print` function.  See the
`hello2.cna` file for the assembly source code.
