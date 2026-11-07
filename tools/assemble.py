#!/usr/bin/env python3
"""Assembler for the VirtualMachine instruction set.

Usage:
    python tools/assemble.py program.asm program.bin

Syntax, one instruction per line:

    <unit> <op> <arg1>, <arg2> -> <dest>

where <unit> is `alu` or `cond`, each argument is either an immediate (`#42`,
0-255) or a location name, and <dest> is a location name.

Locations: ram, base, r0 .. r15, counter, in (source only), out (dest only).

ALU ops : or nand nor and add sub
Cond ops: never eq lt le always ne ge gt

A label is a line ending in `:`. `@label` is the instruction index that label
refers to, usable anywhere an immediate is, which is how jumps are written:

    loop:
        alu add r0, r1 -> r0
        alu add @loop, #0 -> counter

`#` and `;` start a comment.
"""

import re
import sys

LOCATIONS = {"ram": 0, "base": 1, "counter": 18, "in": 19, "out": 20}
for _i in range(16):
    LOCATIONS["r{}".format(_i)] = 2 + _i

ALU_OPS = {"or": 0, "nand": 1, "nor": 2, "and": 3, "add": 4, "sub": 5}
COND_OPS = {"never": 0, "eq": 1, "lt": 2, "le": 3,
            "always": 4, "ne": 5, "ge": 6, "gt": 7}

ARG1_IMMEDIATE_BIT = 1 << 31
ARG2_IMMEDIATE_BIT = 1 << 30
ALU_BIT = 1 << 27

LINE_RE = re.compile(
    r"^(?P<unit>alu|cond)\s+(?P<op>\w+)\s+"
    r"(?P<a1>[^,]+),\s*(?P<a2>.+?)\s*->\s*(?P<dest>\w+)$",
    re.IGNORECASE,
)


class AsmError(Exception):
    pass


def strip_comment(line):
    out = []
    for ch in line:
        if ch in "#;":
            # `#` also starts an immediate, so only treat it as a comment when
            # it is not immediately followed by a digit or a label reference.
            idx = len(out)
            rest = line[idx:]
            if ch == "#" and re.match(r"#\s*\d", rest):
                out.append(ch)
                continue
            break
        out.append(ch)
    return "".join(out).strip()


def parse_lines(text):
    """First pass: collect labels and the instruction lines they point at."""
    labels = {}
    instructions = []
    for lineno, raw in enumerate(text.splitlines(), start=1):
        line = strip_comment(raw)
        if not line:
            continue
        if line.endswith(":"):
            name = line[:-1].strip()
            if not name:
                raise AsmError("line {}: empty label".format(lineno))
            if name in labels:
                raise AsmError("line {}: duplicate label {!r}".format(lineno, name))
            labels[name] = len(instructions)
            continue
        instructions.append((lineno, line))
    return labels, instructions


def resolve_value(token, labels, lineno):
    """An immediate: `#N` or `@label`. Returns None if it is not one."""
    token = token.strip()
    if token.startswith("@"):
        name = token[1:].strip()
        if name not in labels:
            raise AsmError("line {}: unknown label {!r}".format(lineno, name))
        return labels[name]
    if token.startswith("#"):
        body = token[1:].strip()
        try:
            return int(body, 0)
        except ValueError:
            raise AsmError("line {}: bad immediate {!r}".format(lineno, token))
    return None


def resolve_location(token, lineno, role):
    name = token.strip().lower()
    if name not in LOCATIONS:
        raise AsmError("line {}: unknown {} {!r}".format(lineno, role, token.strip()))
    return LOCATIONS[name]


def assemble_line(line, lineno, labels):
    match = LINE_RE.match(line)
    if not match:
        raise AsmError("line {}: cannot parse {!r}".format(lineno, line))

    unit = match.group("unit").lower()
    op_name = match.group("op").lower()
    table = ALU_OPS if unit == "alu" else COND_OPS
    if op_name not in table:
        raise AsmError("line {}: {} has no operation {!r}".format(lineno, unit, op_name))

    word = table[op_name] << 24
    if unit == "alu":
        word |= ALU_BIT

    for which, token in ((1, match.group("a1")), (2, match.group("a2"))):
        shift = 16 if which == 1 else 8
        immediate_bit = ARG1_IMMEDIATE_BIT if which == 1 else ARG2_IMMEDIATE_BIT

        value = resolve_value(token, labels, lineno)
        if value is None:
            field = resolve_location(token, lineno, "source")
            if field == LOCATIONS["out"]:
                raise AsmError("line {}: `out` cannot be read".format(lineno))
        else:
            if not 0 <= value <= 255:
                raise AsmError(
                    "line {}: immediate {} does not fit in 8 bits".format(lineno, value))
            field = value
            word |= immediate_bit
        word |= field << shift

    dest = resolve_location(match.group("dest"), lineno, "destination")
    if dest == LOCATIONS["in"]:
        raise AsmError("line {}: `in` cannot be written".format(lineno))
    word |= dest

    return word & 0xFFFFFFFF


def assemble(text):
    labels, instructions = parse_lines(text)
    return [assemble_line(line, lineno, labels) for lineno, line in instructions]


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2

    with open(argv[1], "r", encoding="utf-8") as handle:
        text = handle.read()

    try:
        words = assemble(text)
    except AsmError as error:
        print("error: {}".format(error), file=sys.stderr)
        return 1

    with open(argv[2], "wb") as handle:
        for word in words:
            handle.write(bytes((
                (word >> 24) & 0xFF,
                (word >> 16) & 0xFF,
                (word >> 8) & 0xFF,
                word & 0xFF,
            )))

    print("{} instructions -> {}".format(len(words), argv[2]))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
