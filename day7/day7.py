#!/usr/bin/env python3

from collections import deque
from itertools import permutations

ADD  = 1                         # add [a], [b] into [c]
MUL  = 2                         # mul [a], [b] into [c]
IN   = 3                         # read stdin and store to a
OUT  = 4                         # write a to stdout
JNZ  = 5                         # jump to b if a != 0
JZ   = 6                         # jump to b if a == 0
TLT  = 7                         # c = a < b
TEQ  = 8                         # c = a == b
HALT = 99                        # halt

PMODE = 0                       # positional mode: index into memory
IMODE = 1                       # immediate mode: literal

WAITING = 0                     # the program is waiting for input
HALTED = 1                      # the program is halted

class Module(object):
    def __init__(self, ram):
        self.ram = ram[:]       # copy the contents
        self.pc = 0
        self.input = deque()
        self.output = deque()

    def push_input(self, data):
        self.input.append(data)

    def pipe(self, module):
        self.output = module.input

    def address_of(self, addr, mode):
        "Return the address of a location for the access mode"
        assert 0 <= addr and addr < len(self.ram)
        if mode == IMODE:
            return addr

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

                assert c_mode == PMODE
                self.ram[c] = self.ram[a] + self.ram[b]
                self.pc += 4
            elif op == MUL:
                a = self.address_of(self.pc+1, a_mode)
                b = self.address_of(self.pc+2, b_mode)
                c = self.address_of(self.pc+3, c_mode)

                assert c_mode == PMODE
                self.ram[c] = self.ram[a] * self.ram[b]
                self.pc += 4
            elif op == IN:
                a = self.address_of(self.pc+1, a_mode)

                assert a_mode == PMODE
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

                assert c_mode == PMODE
                self.ram[c] = self.ram[a] < self.ram[b] and 1 or 0
                self.pc += 4
            elif op == TEQ:
                a = self.address_of(self.pc+1, a_mode)
                b = self.address_of(self.pc+2, b_mode)
                c = self.address_of(self.pc+3, c_mode)

                assert c_mode == PMODE
                self.ram[c] = (self.ram[a] == self.ram[b]) and 1 or 0
                self.pc += 4
            elif op == HALT:
                return HALTED
            else:
                print("Unknown opcode:", op, self.pc)
                assert op, "Unknown opcode"

def signal(array, sequence):
    modules = []
    old = None
    for i in sequence:
        m = Module(array)
        modules.append(m)
        m.push_input(i)
        if old:
            old.pipe(m)
        old = m

    modules[0].push_input(0)

    while True:
        exit = True
        for m in modules:
            s = m.execute()
            exit = exit and (s != WAITING)

        if exit:
            break

    return modules[-1].output[0]

def signal_feedback(array, sequence):
    modules = []
    old = None
    for i in sequence:
        m = Module(array)
        m.push_input(i)
        modules.append(m)
        if old:
            old.pipe(m)
        old = m

    modules[-1].pipe(modules[0])
    modules[0].push_input(0)
    while True:
        exit = True
        for m in modules:
            s = m.execute()
            exit = exit and (s != WAITING)

        if exit:
            break

    return modules[0].input[0]

array = [3,15,3,16,1002,16,10,16,1,16,15,15,4,15,99,0,0]
datain = [4, 3, 2, 1, 0]
assert signal(array, datain) == 43210

array = [3,23,3,24,1002,24,10,24,1002,23,-1,23,
         101,5,23,23,1,24,23,23,4,23,99,0,0]
datain = [0, 1, 2, 3, 4]
assert signal(array, datain) == 54321

array = [3,31,3,32,1002,32,10,32,1001,31,-2,31,1007,31,0,33,
         1002,33,7,33,1,33,31,31,1,32,31,31,4,31,99,0,0,0]
datain = [1,0,4,3,2]
assert signal(array, datain) == 65210

array = [3,26,1001,26,-4,26,3,27,1002,27,2,27,1,27,26,
         27,4,27,1001,28,-1,28,1005,28,6,99,0,0,5]
sequence = (9,8,7,6,5)
assert signal_feedback(array,sequence) == 139629729

array = [3,52,1001,52,-5,52,3,53,1,52,56,54,1007,54,5,55,1005,55,26,1001,54,
         -5,54,1105,1,12,1,53,54,53,1008,54,0,55,1001,55,1,55,2,53,55,53,4,
         53,1001,56,-1,56,1005,56,6,99,0,0,0,0,10]
sequence = (9,7,8,5,6)
assert signal_feedback(array, sequence) == 18216

array = []
with open("input", "rt") as file:
    array = [ int(x) for x in file.read().split(",") ]

maxv = 0
for p in permutations(range(5)):
    v = signal(array, p)
    if v > maxv:
        maxv = v
print("part1", maxv)

maxv = 0
for p in permutations(range(5,10)):
    v = signal_feedback(array, p)
    if maxv < v:
        maxv = v
print("part2", maxv)

# intprogram([3,9,8,9,10,9,4,9,99,-1,8])
# intprogram([3,9,7,9,10,9,4,9,99,-1,8])
# intprogram([3,3,1108,-1,8,3,4,3,99])
# intprogram([3,3,1107,-1,8,3,4,3,99])
# intprogram([3,21,1008,21,8,20,1005,20,22,107,8,21,20,1006,20,31,
#             1106,0,36,98,0,0,1002,21,125,20,4,20,1105,1,46,104,
#             999,1105,1,46,1101,1000,1,20,4,20,1105,1,46,98,99])
