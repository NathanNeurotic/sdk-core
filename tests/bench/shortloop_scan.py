#!/usr/bin/env python3
"""Usage: shortloop_scan.py <objdump> <elf or .a>...

Lists R5900 short-loop erratum hazards in EE code: a conditional branch back over 6 or fewer instructions
(branch and delay slot included), no other branch or jump inside, and a delay slot that is not a nop."""

import re
import subprocess
import sys

OBJDUMP = sys.argv[1]
BRANCHES = {"beq", "bne", "blez", "bgtz", "bltz", "bgez", "beqz", "bnez", "bc1t", "bc1f", "beql", "bnel", "blezl",
              "bgtzl", "bltzl", "bgezl", "bc1tl", "bc1fl", "bltzal", "bgezal", "bc0t", "bc0f"}
JUMPS = {"j", "jal", "jr", "jalr", "b", "bal", "eret", "syscall", "break"}

for elf in sys.argv[2:]:
    out = subprocess.run([OBJDUMP, "-d", elf], capture_output=True, text=True).stdout
    sections, ins, func, member = [], [], "?", elf
    for line in out.splitlines():
        if "file format" in line or line.startswith("Disassembly of section"):
              if ins:
                  sections.append((member, ins))
              ins = []
              if "file format" in line:
                  member = line.split(":")[0]
              continue
        m = re.match(r"^([0-9a-f]+) <(.+)>:$", line)
        if m:
              func = m.group(2)
              continue
        m = re.match(r"^\s*([0-9a-f]+):\t[0-9a-f]+ \t(\S+)\s*(.*)$", line)
        if m:
              ins.append((int(m.group(1), 16), m.group(2), m.group(3), func))
    if ins:
        sections.append((member, ins))
    hazards = 0
    for member, ins in sections:
        index = {a: i for i, (a, _, _, _) in enumerate(ins)}
        for i, (addr, op, args, func) in enumerate(ins):
            if op not in BRANCHES:
                continue
            t = re.search(r"([0-9a-f]+) <", args)
            if not t:
                continue
            target = int(t.group(1), 16)
            if target > addr or target not in index:
                continue
            n = (addr - target) // 4 + 2
            if n > 6 or i + 1 >= len(ins):
                continue
            body = ins[index[target]:i]
            if any(o in BRANCHES or o in JUMPS for _, o, _, _ in body):
                continue
            slot = ins[i + 1]
            if slot[1] == "nop" or op.endswith("l"):
                continue
            hazards += 1
            print(f"{member}: {func} {addr:08x} {op} {args} | {n} instructions, slot: {slot[1]} {slot[2]}")
    print(f"{elf}: {hazards} hazards")
