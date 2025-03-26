Legend for CSIRAC logic symbols
===============================

## AND gates

AND gates are represented by a circle with a 2, 3, or 4 in it,
depending upon the number of inputs to the AND gate:

<img src="and-2-gate-symbol.png"/>

<img src="and-3-gate-symbol.png"/>

<img src="and-4-gate-symbol.png"/>

It is common to list the diodes that act as inputs to a gate.  The gate
itself is a valve hooked up in either non-inverting or inverting mode
depending upon the needs of the downstream circuit.

## OR gates

OR gates are rarer than AND gates and are represented by a circle
with a 1 in it.  This one also has an inverted input designated by the
small circle:

<img src="or-n-gate-symbol.png"/>

This OR gate is implemented by the "b" half of valve V4 (which is a
dual triode).

## Buffers

Buffers are shown as a circle on a line.  This buffer is implemented by the
"a" half of valve V4:

<img src="buffer-symbol.png"/>

## SR Flip-Flops

SR flip-flops are represented by a rounded rectangle:

<img src="flip-flop-1-symbol.png"/>

<img src="flip-flop-2-symbol.png"/>

<img src="flip-flop-3-symbol.png"/>

<img src="flip-flop-4-symbol.png"/>

Sometimes the set (S) input is labeled, but usually you have to figure out
which input is S and which input is R from context.

## Terminals

Terminals are the interconnects between major components.  They are
represented by two concentric circles, accompanied by the label:

<img src="terminal-1-symbol.png"/>

The labels are very unhelpful at indicating what they mean: P2-1, P13, etc.

A label with a dash in it indicates that the terminal has multiple lines;
usually a bus.  The diagram above uses lines 1 and 3 of bus P2.

There are times where an entire bus is represented by a single terminal.
Such cases have a rectangle attached to the outer circle:

<img src="terminal-2-symbol.png"/>

P3 and P4 are buses for the multi-bit source and destination opcodes.
