CSIRAC New Assembler
====================

To make it easier to create tapes, I wrote a modern assembler in C
that takes in a syntax similar to the original CSIRO notation and
produces 12-hole punch tape files in <tt>.cvt</tt> format.

## Hello CSIRAC

Here is a simple example that prints "HELLO WORLD" in original
teleprinter codes:

    start:
        7  -> Ot        ; Print 'H'
        4  -> Ot        ; Print 'E'
        11 -> Ot        ; Print 'L'
        11 -> Ot        ; Print 'L'
        14 -> Ot        ; Print 'O'
        31 -> Ot        ; Print ' '
        22 -> Ot        ; Print 'W'
        14 -> Ot        ; Print 'O'
        17 -> Ot        ; Print 'R'
        11 -> Ot        ; Print 'L'
        3  -> Ot        ; Print 'D'
        30 -> Ot        ; Print CR
        29 -> Ot        ; Print LF
        p20 -> T        ; Halt

        .entry start    ; Set the entry point

To assemble and run this program:

    csirac-assembler -o hello.cvt hello.cna
    csirac hello.cvt

The <tt>.cna</tt> extension means "CSIRAC New Assembly File".

The assembler automatically prepends the "primary routine" and the
"control routine" to the start of the output tape.  These routines can
be suppressed with command-line options to <tt>csirac-assembler</tt>.

The original CSIRAC eventually gained support for a Flexowriter teletype,
which used different character codes.  My emulator also supports ASCII
teletypes, which the original crew may have supported if the project
had continued.

It can be awkward to support multiple teletypes using numeric character codes.
I have added strings and automatic character conversion to the assembler:

    start:
        "H" -> Ot
        "E" -> Ot
        "L" -> Ot
        "L" -> Ot
        "O" -> Ot
        " " -> Ot
        "W" -> Ot
        "O" -> Ot
        "R" -> Ot
        "L" -> Ot
        "D" -> Ot
        "\r" -> Ot
        "\n" -> Ot
        p20 -> T

        .entry start

Now we can compile and run for any kind of teletype:

    # Original teletype
    csirac-assembler -o hello.cvt hello.cna
    csirac hello.cvt

    # Flexowriter teletype
    csirac-assembler -t flexowriter -o hello_f.cvt hello.cna
    csirac --tty-is-flexo hello_f.cvt

    # ASCII teletype
    csirac-assembler -t ascii -o hello_a.cvt hello.cna
    csirac --tty-is-ascii hello_a.cvt

The assembler will convert the input strings into the correct codes.
An error will be produced if the strings use characters that are not
supported in the selected character set.  Lowercase letters will be
converted to uppercase for non-ASCII teletypes.

Strings that are used as the source for an instruction must be a
single character.  Here is an example of converting the numbers
0-9 in the upper 10 bits of A into decimal for printing:

    "0" -> +A
    A   -> Ot

Note: This decimal conversion only works with the original teleprinter
and ASCII.  It does not work with the Flexowriter which does not encode
digits with consecutive character codes.

You can make the character set explicit in the code with directives if
you don't want to pass a command-line option:

        .ascii

    start:
        "H" -> Ot
        "i" -> Ot
        "!" -> Ot
        p20 -> T

        .entry start

The character set directives are <tt>.teleprinter</tt>, <tt>.flexowriter</tt>,
and <tt>.ascii</tt>.  You can even switch between character sets in different
parts of the code.

## Instruction Syntax

Instructions use the ASCII version of the CSIRO syntax described
[here](architecture.md).  Each instruction has a source and destination,
separated by <tt>-></tt>:

    (2, 18 M) -> C
    (C) -> +A

Source memory addresses can be specified in a number of ways:

* <tt>(n, m M)</tt> or <tt>(n, m)</tt> where the address is n * 32 + m.
* <tt>(n M)</tt> or <tt>(n)</tt> where the address is n (0-1023).
* <tt>(label M)</tt> or <tt>(label)</tt> where the address is the
value of the <tt>label</tt>.

Destination memory addresses omit the parentheses:

* <tt>n, m M</tt> or <tt>n, m</tt> where the address is n * 32 + m.
* <tt>n M</tt> or <tt>n</tt> where the address is n (0-1023).
* <tt>label M</tt> or <tt>label</tt> where the address is the
value of the <tt>label</tt>.

Magnetic drum addresses are similar except they use <tt>a</tt>, <tt>b</tt>,
<tt>c</tt>, or <tt>d</tt> in place of <tt>M</tt> for the 4 pages of drum
memory; for example, <tt>(20 a)</tt> or <tt>42 c</tt>.

Source constants use a similar syntax to memory addresses:

* <tt>n, m</tt> or <tt>(n, m K)</tt> where the constant is n * 32 + m.
* <tt>n</tt> or <tt>(n K)</tt> where the constant is n.
* <tt>label</tt> or <tt>(label K)</tt> where the constant is the
value of the <tt>label</tt>.

Numeric constants can be prefixed with '-' to negate them in two's
complement notation; for example <tt>-42 -> C</tt>.  Note however that
constants only operate on the top 10 bits of a word.  Negated constants
can be useful in loops where the instruction count to return to the
top of the loop is negative:

    1023 -> P
    -2 -> +S

## Jumps and Subroutine Calls

The simplest way to jump to an address is to give it a label and use the
<tt>S</tt> destination:

    loop:
        ...
        loop -> S

The usual method to call a subroutine in CSIRAC code is to save the
return address in a D register and then jump to the subroutine:

    (S) -> D15
    label -> S

This pattern is so common, that I have created a <tt>.call</tt>
psuedo-instruction for it that takes the label and the link register number:

    .call label,D15

If the function was already defined previously with a <tt>.function</tt>
directive, then the link register number can be omitted.  The link register
number from the function definition will be used implicitly:

    .call label

Inside the subroutine, the first instruction should add 1 to the
link register to point it at the correct return address:

    1 -> +D15

To return from the subroutine, copy the value of the link register back to S:

    (D15) -> S

## Definining Functions

Functions, or subroutines, are the most important part of the CSIRAC
tape library.  Library tapes define common functionality that can be
included into other programs so that the programmer doesn't need to
write the common code themselves.

Functions are a fundamental unit of relocatable tapes, so they are
quite important.  So important that I created a special syntax for
definining them:

    .function label,D15
        ...
        .return
    .endfunction

The <tt>.function</tt> directive declares a <tt>label</tt> to identify
the function, and the name of the link register to use (D15 in this case).
The <tt>.return</tt> psuedo-instruction copies the value of the link
register for the current function back to the S register.
The above code is equivalent to the following:

    label:
        1 -> +D15
        ...
        (D15) -> S

If you omit the `.return` instruction from the end of the function
definition, it will be added implicitly if the last instruction is
not of the form `... -> S`.

In the original CSIRAC system, link registers had to be allocated in
levels.  Usually D15 was used for functions at the lowest level,
D14 for the next level up, and so on.  This can make it difficult to
create recursive functions or functions that may be called from
different levels in the program.

If you do need to use recursion, then you can use a D register as a
stack pointer and save the return address on the stack:

    .function label,D15
        D15   -> A
        1     -> -D0
        (D0)  -> +K
        (A)   -> (0 M)
        ...
        (D0)  -> +K
        (0 M) -> A
        1     -> +D0
        (A)   -> D15
    .endfunction

While the use of D registers is traditional for link registers, the assembler
also allows A and C to be used.  This can reduce the overhead of stack
manipulation in recursive functions:

    .function label,A
        1     -> -D0
        (D0)  -> +K
        (A)   -> (0 M)
        ...
        (D0)  -> +K
        (0 M) -> A
        1     -> +D0
    .endfunction

    ...
    .call label,A

The relocation mechanism used by CSIRAC is strictly bottom-up.  It is not
possible to refer to a function or subroutine that is further down the
tape.  Function definitions must appear before the parent functions that
call them.  This makes it difficult to implement mutually-recursive functions.

## Data

Data in the form of words, characters, and strings is important in all
useful programs.

The <tt>.dd</tt> directive inserts 10-bit values into the program,
aligned in the top 10 bits of the 20-bit word just like a program
instruction constant or address:

    .dd 1023, 9, 64
    .dd label

The low 10 bits of the values will be set to zero.

If you need to store a full 20-bit value for an integer or fractional
constant, then use the <tt>.dw</tt> directive instead:

    .dw 1000000
    .dw 0.5, -0.33333333, 0.25, -0.2, 0.16666667
    .dw "HELLO"

Fractional constants must include a "." and must be between -1 (inclusive)
and 1 (exclusive).  They are converted into the equivalent 20-bit word value.

Strings are converted into the current character set and each character is
stored in an individual 20-bit word, with a terminating zero word on the
end.  The <tt>.dh</tt> and <tt>.dg</tt> directives provide better packing
for strings.

The <tt>.dh</tt> directive inserts half-sized 10-bit values into the program,
packed two per word:

    .dh 976, 576    ; 1000000 as two 10-bit halves.
    .dh 1, 2, 3     ; Two words: 1026 and 3072.
    .dh "HELLO"     ; Characters packed into 10-bit halves.

If the <tt>.dh</tt> directive has an odd number of arguments, then the
final 10-bit half will be set to zero.  Values are packed into the high
10 bits first and then the low 10 bits.

The <tt>.dg</tt> directive inserts group-sized 5-bit values into the
program, packed four per word:

    .dg 30, 16, 18, 0   ; 1000000 as four 5-bit values.
    .dg "HELLO"         ; Characters packed into 5-bit groups.

This provides the best packing for teleprinter and Flexowriter strings
but it is harder to extract the characters at runtime.  ASCII strings
should use <tt>.dh</tt> instead.

## Includes and Imports

TBD

## Example: Printing Strings

Bringing everything together, here is an example of printing two
NUL-terminated ASCII strings to a teleprinter where the strings are
packed two characters per word:

    .ascii

    ;
    ; Print the NUL-terminated string pointed to by C to the teleprinter.
    ;
    ; Destroys C and H.
    ;
    .function print,D15
    next_pair:
        (C) -> +K       ; Get the next character from the upper half
        (0 M) -> Hu     ; of the word pointed to by C.
        (Hl) -> cS      ; Skip next instruction if the character is not NUL.
        .return         ; Return from the subroutine if the character is NUL.
        (Hl) -> Ot      ; Output the character to the teleprinter.
        (C) -> +K       ; Get the next character from the lower half
        (0 M) -> Hl     ; of the word pointed to by C.
        (Hl) -> cS      ; Skip next instruction if the character is not NUL.
        .return         ; Return from the subroutine if the character is NUL.
        (Hl) -> Ot      ; Output the character to the teleprinter.
        1 -> +C         ; Increment C to point to the next string word.
        next_pair -> S  ; Jump back to the top of the loop.
    .endfunction

    string1:
        .dh "THIS IS A TEST\r\n",0
    string2:
        .dh "HELLO WORLD\r\n",0

    main:
        string1 -> C    ; Load a pointer to string1 into C.
        .call print     ; Print string1.
        string2 -> C    ; Load a pointer to string2 into C.
        .call print     ; Print string2.
        p20 -> T        ; Halt the machine.

    .entry main

## Directives

`.align` enables alignment of function definitions on 32-word boundaries.

`.ascii` or `.utf8` switches the character encoding in strings to ASCII/UTF-8.
Technically the encoding is always UTF-8, even with `.ascii`.

`.call LABEL[,R]` calls a function called `LABEL`, using `R` as the
link register.  If the function has already been declared, then `R` can
be omitted.  A, C, and D0 to D15 can be used as link registers.

`.dd DATA` writes literal 10-bit values to the output, aligned in the top
half of the 20-bit word.  The bottom half is set to zero bits.

`.dg DATA` writes literal 5-bit values to the output, alternating between
the four groups in the 20-bit words.  Unused groups are set to zero.

`.dh DATA` writes literal 10-bit values to the output, alternating between
the top and bottom halves of the 20-bit words.  Unused halves are
set to zero.

`.dw DATA` writes literal 20-bit values to the output.

`.entry LABEL` defines the entry point for the program.  The `LABEL` must
have already been defined previously.  Usually `.entry` is the last line
in the assembly code input.

`.flexowriter` switches the character encoding in strings to the Flexowriter.

`.function LABEL,R` declares a relocatable function in the program with
name `LABEL` and link register `R`.  The defintion ends with `.endfunction`.

`.noalign` disables alignment of function definitions on 32-word boundaries.

`.punch5` sets the output punch to 5-hole punch tape format (this is the
default).

`.punch8` sets the output punch to 8-hole punch tape format (binary).

`.punch12` sets the output punch to 12-hole punch tape format.

`.return` returns from the current function using its link register.

`.tape5` sets the input tape to 5-hole punch tape format (this is the default).

`.tape8` sets the input tape to 8-hole punch tape format (binary).

`.tape12` sets the input tape to 12-hole punch tape format.

`.teleprinter` switches the character encoding in strings to the old-style
teleprinter (this is the default encoding).

`.title "NAME"` sets the title for the program to `NAME`, which is output
as a comment in the final `.cvt` file.
