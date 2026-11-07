# Virtual Machine

A small register machine with its own instruction set, an interpreter for it in
C++, and an assembler so you can write programs for it.

Every instruction is exactly 32 bits and does the same thing: take two
operands, feed them to either the arithmetic unit or the condition unit, and
write the result somewhere. There is no branch instruction — the program
counter is just another destination, so a program jumps by writing to it.

![tests](https://github.com/sahakyan-tigran/virtual-machine/actions/workflows/ci.yml/badge.svg)

## Quick start

```sh
make                                                   # build the interpreter
python tools/assemble.py examples/sum_to_ten.asm sum.bin
./virtualmachine sum.bin                               # prints 55
```

or in one step:

```sh
make example
```

Run the tests with `make test`.

## Machine state

| | |
| --- | --- |
| `r0` … `r15` | 16 general-purpose 32-bit registers |
| base register | indexes into RAM |
| RAM | 1024 words of 32 bits |
| counter | the program counter |

## Instruction format

32 bits, stored big-endian in the program file:

```
 31  30  29 28  27   26 24   23      16   15       8   7     0
┌───┬───┬───────┬────┬───────┬───────────┬───────────┬─────────┐
│ I1│ I2│ unused│unit│  op   │   arg1    │   arg2    │  dest   │
└───┴───┴───────┴────┴───────┴───────────┴───────────┴─────────┘
```

- **I1 / I2** — when set, the corresponding argument field is an 8-bit
  immediate (0–255). When clear, it is a 5-bit source id in the low bits.
- **unit** — `1` selects the ALU, `0` selects the condition unit.
- **op** — three bits, interpreted by the selected unit.
- **dest** — a 5-bit destination id.

### Sources and destinations

| Id | Binary | Name | Notes |
| --- | --- | --- | --- |
| 0 | `00000` | RAM | the word at the base register |
| 1 | `00001` | base register | |
| 2–17 | `00010`–`10001` | `r0` … `r15` | |
| 18 | `10010` | counter | writing it jumps |
| 19 | `10011` | in | source only; reads a number from stdin |
| 20 | `10100` | out | destination only; writes a number to stdout |

### ALU operations

| Op | Name | Assembler |
| --- | --- | --- |
| 0 | OR | `or` |
| 1 | NAND | `nand` |
| 2 | NOR | `nor` |
| 3 | AND | `and` |
| 4 | ADD | `add` |
| 5 | SUB | `sub` |

Results are 32-bit and wrap on overflow.

### Condition operations

Each yields `1` or `0`, written to the destination like any other result.

| Op | Meaning | Assembler |
| --- | --- | --- |
| 0 | never | `never` |
| 1 | `==` | `eq` |
| 2 | `<` | `lt` |
| 3 | `<=` | `le` |
| 4 | always | `always` |
| 5 | `!=` | `ne` |
| 6 | `>=` | `ge` |
| 7 | `>` | `gt` |

## Writing programs

The assembler takes one instruction per line:

```
<unit> <op> <arg1>, <arg2> -> <dest>
```

Arguments are either an immediate (`#42`) or a location name. A line ending in
`:` is a label, and `@label` is that label's instruction index — which is how
you write a jump. `;` starts a comment.

```
        alu add #2, #3 -> r0       ; r0 = 5
        alu add r0, #1 -> out      ; print 6
```

### Branching

Since the counter is a destination, an unconditional jump is just:

```
        alu add @target, #0 -> counter
```

A conditional jump needs a little arithmetic, because the condition unit
produces `1`/`0` rather than an address. Subtracting it from zero gives a mask
of all ones or all zeros, and ANDing each candidate target with the right mask
leaves exactly one of them:

```
        cond le  r1, #10   -> r2        ; r2 = (i <= 10)
        alu  sub #0, r2    -> r3        ; r3 = -r2      -1 if true, 0 if false
        alu  sub r2, #1    -> r4        ; r4 = r2 - 1    0 if true, -1 if false
        alu  and r3, @loop -> r3        ; loop target, or 0
        alu  and r4, @done -> r4        ; done target, or 0
        alu  or  r3, r4    -> counter   ; jump to whichever survived
```

[`examples/sum_to_ten.asm`](examples/sum_to_ten.asm) is that loop in full: it
sums 1..10 and prints 55.

Execution stops when the counter moves outside the program. There is also a
step limit (10 million by default) so a runaway loop terminates instead of
hanging.

## Layout

| File | What it is |
| --- | --- |
| [`Machine.h`](Machine.h) / [`Machine.cpp`](Machine.cpp) | machine state, instruction decoding, the run loop |
| [`ALU.h`](ALU.h) / [`ALU.cpp`](ALU.cpp) | arithmetic and logic |
| [`Cond.h`](Cond.h) / [`Cond.cpp`](Cond.cpp) | comparisons |
| [`VirtualMachine.cpp`](VirtualMachine.cpp) | command-line entry point |
| [`tools/assemble.py`](tools/assemble.py) | assembler |
| [`examples/`](examples) | worked example programs |
| [`tests/`](tests) | test suite |

## Limitations

- No subtraction-free multiply or divide; those would be the next opcodes.
- Immediates are 8 bits, so addresses beyond 255 cannot be written literally.
  A program that big would need to build the address in a register first.
- The assembler is deliberately minimal: no macros, no expressions, no data
  directives.

## License

MIT
