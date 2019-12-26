#!/usr/bin/env python3

from collections import deque, defaultdict

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

    def load(self, program):
        self.ram.clear()
        for p,v in enumerate(program):
            self.ram[p] = v
        self.pc = 0
        self.rbp = 0
        self.input.clear()
        self.output.clear()

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

                if self.input:
                    self.ram[a] = self.input.popleft()
                    self.pc += 2
                else:
                    return WAITING
            elif op == OUT:
                a = self.address_of(self.pc+1, a_mode)
                self.pc += 2
                self.output.append(self.ram[a])
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


springscript = """NOT J T
AND B T
AND C T
NOT T T
AND D T
NOT T T
AND A T
NOT T J
WALK
"""
module = Module(4096)
module.load(program)
module.execute()
while module.output:
    print(chr(module.pop_output()), end="")

for s in springscript:
    module.push_input(ord(s))
while module.execute() != HALTED:
    while module.output:
        print(chr(module.pop_output()), end="")
print("part1:", module.pop_output())

module.load(program)
springscript = """NOT J T
AND B T
AND C T
NOT T T
AND D T
AND H T
NOT T T
AND A T
NOT T J
RUN
"""
for s in springscript:
    module.push_input(ord(s))
while module.execute() != HALTED:
    while module.output:
        print(chr(module.pop_output()), end="")
if module.output:
    print("part2:", module.pop_output())
