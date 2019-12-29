#!/usr/bin/env python3

from collections import deque, defaultdict

import sys

ADD  = 1                        # add [a], [b] into [c]
MUL  = 2                        # mul [a], [b] into [c]
IN   = 3                        # read stdin and store to a
OUT  = 4                        # write a to stdout
JNZ  = 5                        # jump to b if a != 0
JZ   = 6                        # jump to b if a == 0
TLT  = 7                        # c = a < b
TEQ  = 8                        # c = a == b
ARB  = 9                        # add to relative base (base pointer
HALT = 99                       # halt

PMODE = 0                       # absolute index into memory
IMODE = 1                       # immediate literal
RMODE = 2                       # relative mode: offset from rbp

WAITING = 0                     # the program is waiting for input
HALTED = 1                      # the program is halted

class Module(object):
    def __init__(self, size):
        self.ram = defaultdict(lambda: 0)
        self.size = size
        self.pc = 0             # instruction pointer
        self.rbp = 0            # relative base
        self.input = deque()
        self.output = deque()
        self.snapshot = None
        self.logfile = None

    def load(self, program):
        self.ram.clear()
        for p,v in enumerate(program):
            self.ram[p] = v
        self.pc = 0
        self.rbp = 0
        self.input.clear()
        self.output.clear()

    def log(self, f):
        self.logfile = f

    def push_input(self, data):
        self.input.append(data)

    def pop_output(self):
        return self.output.popleft()

    def pipe(self, module):
        self.output = module.input

    def address_of(self, addr, mode):
        "Return the address of a location for the access mode"
        if mode == IMODE:
            return addr
        elif mode == RMODE:
            return self.rbp + self.ram[addr]
        else:
            return self.address_of(self.ram[addr], IMODE)

    def execute(self):
        while True:
            op = self.ram[self.pc] % 100
            a_mode = (self.ram[self.pc] // 100) % 10
            b_mode = (self.ram[self.pc] // 1000) % 10
            c_mode = (self.ram[self.pc] // 10000) % 10
            if op == ADD:
                a = self.address_of(self.pc+1, a_mode)
                b = self.address_of(self.pc+2, b_mode)
                c = self.address_of(self.pc+3, c_mode)

                self.ram[c] = self.ram[a] + self.ram[b]
                self.pc += 4
            elif op == MUL:
                a = self.address_of(self.pc+1, a_mode)
                b = self.address_of(self.pc+2, b_mode)
                c = self.address_of(self.pc+3, c_mode)

                self.ram[c] = self.ram[a] * self.ram[b]
                self.pc += 4
            elif op == IN:
                a = self.address_of(self.pc+1, a_mode)

                if not self.input:
                    return WAITING

                self.ram[a] = self.input.popleft()
                self.pc += 2
                if self.logfile and 0 <= self.ram[a] < 256:
                    self.logfile.write(chr(self.ram[a]))
            elif op == OUT:
                a = self.address_of(self.pc+1, a_mode)
                self.pc += 2
                self.output.append(self.ram[a])
                if self.logfile and 0 <= self.ram[a] < 256:
                    self.logfile.write(chr(self.ram[a]))
                if self.ram[a] == ord("\n"):
                    return WAITING
            elif op == JNZ:
                a = self.address_of(self.pc+1, a_mode)
                b = self.address_of(self.pc+2, b_mode)

                if self.ram[a] != 0:
                    self.pc = self.ram[b]
                else:
                    self.pc += 3
            elif op == JZ:
                a = self.address_of(self.pc+1, a_mode)
                b = self.address_of(self.pc+2, b_mode)

                if self.ram[a] == 0:
                    self.pc = self.ram[b]
                else:
                    self.pc += 3
            elif op == TLT:
                a = self.address_of(self.pc+1, a_mode)
                b = self.address_of(self.pc+2, b_mode)
                c = self.address_of(self.pc+3, c_mode)

                self.ram[c] = self.ram[a] < self.ram[b] and 1 or 0
                self.pc += 4
            elif op == TEQ:
                a = self.address_of(self.pc+1, a_mode)
                b = self.address_of(self.pc+2, b_mode)
                c = self.address_of(self.pc+3, c_mode)

                self.ram[c] = (self.ram[a] == self.ram[b]) and 1 or 0
                self.pc += 4
            elif op == ARB:
                a = self.address_of(self.pc+1, a_mode)
                self.rbp += self.ram[a]
                self.pc += 2
            elif op == HALT:
                return HALTED
            else:
                print("Unknown opcode:", op, self.pc)
                assert op, "Unknown opcode"

program = []
with open("input", "rt") as file:
    program = [ int(x) for x in file.read().split(",") ]

computers = []
for i in range(50):
    c = Module(4096)
    c.load(program)
    c.push_input(i)
    computers.append(c)

natx = None
naty = None
lasty = None
i = 0
last_running = None
while True:
    c = computers[i]
    v = c.execute()
    if len(c.output) >= 3:
        last_running = i
        dst = c.pop_output()
        x = c.pop_output()
        y = c.pop_output()
        if dst == 255:
            if naty is None:
                print("part1:", y)
            natx = x
            naty = y
            i = (i + 1) % 50
        else:
            last_running = None
            i = dst
            computers[i].push_input(x)
            computers[i].push_input(y)
    else:
        c.push_input(-1)
        i = (i + 1) % 50

    if i == last_running:
        if lasty and lasty == naty:
            print("part2:", naty)
            break
        lasty = naty
        i = 0
        computers[i].push_input(natx)
        computers[i].push_input(naty)
