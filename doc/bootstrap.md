Primary and Control Bootstrap Routines
======================================

CSIRAC programs are punched onto 12-hole tape and fed into the computer.
The first 40 or so punched symbols on the tape are usually the
"Primary Routine" and the "Control Routine".

These routines bootstrap the computer to load the rest of the program
and the libraries that the program uses.  The libraries are relocated
by the control routine to their final location in memory.

## Tape format

Each row on the 12-hole tape has 10 bits for the value (P1..P10),
an X punch bit (P19), and a Y punch bit (P20).  The X punch is used to
help assemble 20-bit words from two 10-bit fragments.  For example:

    31  8
    12 14X

This assembles (31, 8) into the top 10 bits of the word and (12, 14)
into the bottom 10 bits.  The resulting instruction is then written to
main memory.  If the top 10 bits are zero, they can be omitted to save
space on the tape:

    26 14X
    21 12X
    11 22X

The Y punch is used to issue commands to the control routine for
relocating the program in memory.  More on those later.

## Padding

Sections of the tape may be separated by unpunched tape rows, which
corresponds to the instruction word 0.

## Primary

The primary routine makes up the first 19 symbols on the tape.  The first
is a zero for padding, and the remaining 18 are the instructions.

When loading a tape, the instructions of the primary are "force fed"
into memory in a special single-stepping mode.  Then the "RUN"
button is pressed and the primary loads the rest of the tape automatically.

Here is the primary routine, with the disassembled form and notes on the right:

    Addr  Insn    Disassembly       Comment
    -------------------------------------------------------------------------
          0  0                      Padding on the tape, does nothing.
     0:  17 18     (D0) -> +D0      Add D0 to itself (shift left by 1 bit).
     1:  18 25    s(D0) -> cS       Skip next instruction if sign bit of D0 is 1.
     2:  22  5     (Hu) -> +A       Add H to the upper half of A.
     3:  18 25    s(D0) -> cS       Skip next instruction if sign bit of D0 is 1.
     4:  23 24     (S)  -> +S       Skips the next 5 instructions.
     5:  21  5     (Hl) -> +A       Add H to the lower half of A.
     6:  11 24     (B)  -> +S       Skip the next B instructions.
     7:  14 26     (C)  -> +K       Add C to the next instruction.
     8:   9  0     c(A) -> 0 M      Store A into address C and then clear A.
     9:  24 15     p11  -> +C       Advance C to the next address.
    10:   1 17     (I)  -> D0       Read the next from the input tape into D0.
    11:  17 21     (D0) -> Hl       Transfer the lower half of D0 to H.
    12:  18 25    s(D0) -> cS       Skip next instruction if sign bit of D0 is 1.
    13:  20 23     (Z)  -> S        Jump to address 0 (top of this routine).
    14:  22 14     (Hu) -> C        Transfer H to the upper half of C.
    15:  17 19     (D0) -> -D0      Subtract D0 from itself; e.g. set D0 to zero.
    16:  20 22     (Z)  -> Hu       Set H to zero.
    17:  20 23     (Z)  -> S        Jump to address 0 (top of this routine).

On startup, all machine registers are cleared to zero before loading the
primary.  The primary uses the following registers for housekeeping:

* D0 - Last word that was read from the 12-hole tape.
* H - Low 10 bits of the last word that was read from the 12-hole tape.
* A - Instruction word that is being assembled.
* C - Memory location to write the next instruction word once it is assembled.

This code above is still pretty mysterious.  But it becomes a littler
clearer when we write it in C:

    while (1) {
        // Shift D0 up by 1 bit, which has the effect of
        // putting the X punch into the sign bit of D0.
        D0 <<= 1;

        // Was X punched?
        if (sign(D0)) {
            // Yes it was.  Move the low 10 bits of the previous
            // tape word into the lower half of A.
            A += lower(H);

            // If B is 0, then we store the instruction to memory at C.
            // If B is 11, then we jump to the control routine at address 18
            // and perform a "DO" command with the instruction in A.
            if (B == 0) {
                memory[C] = A;
            } else {
                goto control_18;
            }

            // Set A back to 0 for the next instruction.
            A = 0;

            // Increment C to point at the next memory location to fill.
            ++C;
        } else {
            // Not an X punch, so move the low 10 bits of the
            // previous tape word into the upper half of A.
            A += upper(H);
        }

        // Read the next word from the input tape into D0.
        D0 = read_tape();

        // Transfer the low 10 bits of D0 into H.
        H = lower_bits(D0);

        // Was Y punched?  Y is currently the sign bit of D0.
        if (sign(D0)) {
            // Transfer H to the upper 10 bits of C, and zero the low bits.
            // C is now the address to store the next instruction to.
            C = upper(H);

            // Set D0 and H to zero.
            D = 0;
            H = 0;
        }
    }

## Loading the primary

The primary is loaded by force-feeding it into memory starting at address 0.
The machine is put into a special mode where the first 18 tape words are
single-stepped into memory.  Then the S register is cleared and the
"RUN" button pressed to start executing the primary routine.

## Transferring from primary to control

Looking at the above C code, it isn't clear how we get out of the primary
routine to the control routine.  And the primary does something different
when B is equal to 11.  But all registers including B are set to zero at
startup.  Huh?

There is a trick going on here.  The control routine starts with this
control command:

    0 14 Y

This control command is detected at address 12 which jumps to address 14
if Y has been punched.  The C register is then set to 14 for the next
iteration.

When we return to the main primary loop, the tape contents will overwrite
the primary routine from address 14 onwards with the control routine.

The next time we see a Y punch and execution falls through to address 14,
the control routine will take over.

Another trick is with B, which is set to 11 to mean "execute the next
instruction" instead of "store the next instruction".  It causes the
primary routine to jump to address 18 in the control routine and "DO"
the command in A.

## Control

Here is the control routine that is loaded over the top of the primary
routine starting at address 14 in memory.

    Addr    Insn      Disassembly       Comment
    -------------------------------------------------------------------------
    14:  0  0 22 24    (Hu) -> +S       Add H to the upper half of S.
    15:  0 24  0  5  (24 M) -> +A       Add contents of 24 in memory to A.
    16:  0 23  0  5  (23 M) -> +A       Add contents of 23 in memory to A.
    17:  0 15  0  5  (15 M) -> +A       Add contents of 15 in memory to A.
    18:  0 19  9  0    c(A) -> 19 M     Write A to next instruction and clear A.
    19:  0  0 31 31     p20 -> T        Halt (replaced with instruction in A).
    20: 31 21 26 26    1013 -> +K       Add -11 to the next instruction.
    21:  0 11 26 11      11 -> B        Load 11 (or 0) into B.
    22:  0 10 26 23      10 -> S        Jump to address 10 in primary routine.
    23:  0  0 13 27    r(B) -> (0 a)    Constant: 443
    24: 31  8 12 14     (R) -> C        Constant: -24178

This one is pretty weird.  It is constructing an instruction by adding
several constants together and then executing that instruction at address 19.

It helps to understand the Y-punch commands that the control routine
uses to load programs and relocate them in memory.

<table border="1">
<tr><td><b>Command</b></td><td><b>Symbol</b></td><td><b>Description</b></td></tr>
<tr><td><tt>m, n, 0, 0Y</tt></td><td><tt>m, nT</tt></td><td>Set the next address to (m, n)</td></tr>
<tr><td><tt>0, n, 0, 1Y</tt></td><td><tt>nS</tt></td><td>Store the current address in parameter n</td></tr>
<tr><td><tt>0, n, 0, 2Y</tt></td><td><tt>nA</tt></td><td>Add parameter n to the next instruction</td></tr>
<tr><td><tt>0, 0, 0, 4Y</tt></td><td><tt>R</tt></td><td>Repeat the last control command</td></tr>
<tr><td><tt>0, 0, 0, 6Y</tt></td><td><tt>D</tt></td><td>"DO" the next instruction instead of storing it</td></tr>
</table>

The first instruction of the control routine is <tt>(Hu) -> +S</tt>.  What
this does is add the command code in the low 5 bits to the program counter.
So command 0 adds 0, command 1 adds 1, and so on.  The first instruction
is essentially a "switch" on the type of command:

* 0 - Jump to address 15
* 1 - Jump to address 16
* 2 - Jump to address 17
* 4 - Jump to address 19
* 6 - Jump to address 21

Once fully constructed, the first three commands correspond to the
following instructions:

* 0 - (0, 0, 26, 14) - <tt>(A) -> C</tt> - Make A the new current address.
* 1 - (0, 24 + n, 14, 0) - <tt>(C) -> (24 + n) M</tt> - Store C at memory address 24 + n.
* 2 - (0, 24 + n, 0, 5) - <tt>((24 + n) M) -> +A</tt> - Add the value stored at memory address 24 + n to A.

What is going on here is dynamic relocation of code.  There are n parameters
stored in memory starting at address 25.  Whenever a <tt>0, n, 0, 1Y</tt>
command is encountered, the current address in C is stored to parameter n
in the table.  When a <tt>0, n, 0, 2Y</tt> command is encounted, the
contents of parameter n is added to the in-progress instruction in A.
This is how relocation is done!

In the description above, parameter numbers are shown as "0, n" which
implies that they must be between 1 and 31.  It is possible to have
up to 998 parameters using "m, n".  But that wouldn't leave any room
for the program code!

There may be a legitimate use for high parameter numbers if you wanted
to put the relocation table at the end of memory instead of at the start.
Set "m, n" to the desired relocation address minus 24.

## Structure of a program

The program is laid out on tape as follows:

* Primary routine.
* Control routine.
* Library routines that are used by the program.
* Main program routine.

If library routine A calls library routine B, then B must appear on the
tape before A so that the address of B can be stored in a parameter for
A to use when relocating itself.

This tends to preclude mutually-recursive library routines where A calls B
and B calls A.  However, it can be done if you manually place sections that
are mutually-recursive:

    20 0T       ; Set the current address for B to (20, 0)
    2S          ; Store it in parameter 2
    10 0T       ; Set the current address for A to (10, 0)
    3S          ; Store it in parameter 3
    1S          ; Relocate A at (10, 0) using parameter 1
    ...
    2A          ; Relocate a reference to routine B
    ...
    20 0T       ; Set the curent address for B to (20, 0)
    1S          ; Relocate B at (20, 0) using parameter 1
    ...
    3A          ; Relocate a reference to routine A
    ...
