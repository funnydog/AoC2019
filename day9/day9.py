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
        self.pc = 0             # instruction pointer
        self.rbp = 0            # relative base
        self.input = deque()
        self.output = deque()

    def load(self, program):
        self.ram.clear()
        for i, v in enumerate(program):
            self.ram[i] = v
        self.pc = 0
        self.rbp = 0
        self.input.clear()
        self.output.clear()

    def push_input(self, data):
        self.input.append(data)

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


m = Module(4096)
ram = [109,1,204,-1,1001,100,1,100,1008,100,16,101,1006,101,0,99]
m.load(ram)
m.execute()
assert list(m.output) == ram

ram = [1102,34915192,34915192,7,4,7,99,0]
m.load(ram)
m.execute()
assert len(str(m.output.popleft())) == 16

ram = [104,1125899906842624,99]
m.load(ram)
m.execute()
assert m.output.popleft() == ram[1]

array = []
with open("input", "rt") as file:
    array = [ int(x) for x in file.read().split(",") ]

m.load(array)
m.push_input(1)
m.execute()
print("part1:", m.output.popleft())
m.load(array)
m.push_input(2)
m.execute()
print("part2:", m.output.popleft())
