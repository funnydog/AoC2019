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
    def __init__(self):
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
            val, op = divmod(self.ram[self.pc], 100)
            val, a_mode = divmod(val, 10)
            val, b_mode = divmod(val, 10)
            _, c_mode = divmod(val, 10)
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


def xmul(a, b):
    return (a[0]*b[0]-a[1]*b[1], a[0]*b[1]+a[1]*b[0])

def add(a, b):
    return (a[0]+b[0], a[1]+b[1])

class Hull(object):
    def __init__(self):
        self.clear()

    def clear(self):
        self.panels = {}
        self.x0 = 0
        self.x1 = 0
        self.y0 = 0
        self.y1 = 0

    def paint(self, ram, initial):
        self.clear()
        pos = (0,0)
        cdir = (0, -1)
        panel = {}
        robot = Module()
        robot.load(ram)
        robot.push_input(initial)
        while robot.execute() == WAITING:
            self.panels[pos] = robot.output.popleft()
            d = robot.output.popleft()
            if d == 0:              # rotate left
                cdir = xmul(cdir, (0, -1))
            elif d == 1:            # rotate right
                cdir = xmul(cdir, (0, 1))

            pos = add(pos, cdir)
            if pos[0] < self.x0:
                self.x0 = pos[0]
            if pos[0] > self.x1:
                self.x1 = pos[0]
            if pos[1] < self.y0:
                self.y0 = pos[1]
            if pos[1] > self.y1:
                self.y1 = pos[1]

            robot.push_input(self.panels.get(pos, 0))

    def count(self):
        return len(self.panels.keys())

    def __str__(self):
        lst = []
        for y in range(self.y0, self.y1-self.y0+1):
            for x in range(self.x0, self.x1-self.x0+1):
                if self.panels.get((x, y)):
                    lst.append("#")
                else:
                    lst.append(" ")
            lst.append("\n")
        return "".join(lst)

ram = []
with open("input", "rt") as file:
    ram = [ int(x) for x in file.read().split(",") ]

h = Hull()
h.paint(ram, 0)
print("part1:", h.count())
h.paint(ram, 1)
print("part2:")
print(h)
