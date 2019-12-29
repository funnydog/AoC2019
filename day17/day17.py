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
    def __init__(self):
        self.ram = defaultdict(lambda: 0)
        self.pc = 0             # instruction pointer
        self.rbp = 0            # relative base
        self.input = deque()
        self.output = deque()
        self.snapshot = None
        self.logfile = None

    def load(self, program):
        self.ram.clear()
        for i, v in enumerate(program):
            self.ram[i] = v
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
                if self.ram[a] == 10:
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


def xmul(a, b):
    return (a[0]*b[0]-a[1]*b[1], a[0]*b[1]+a[1]*b[0])

def add(a, b):
    return (a[0]+b[0], a[1]+b[1])

def sub(a, b):
    return (a[0]-b[0], a[1]-b[1])

class Map(object):
    directions = ((0, -1), (0, +1), (-1, 0), (+1, 0))

    def __init__(self, echo = False):
        self.points = {}
        self.distances = {}
        self.max_distance = 0
        self.startpos = None
        self.startdir = None
        self.height = None
        self.module = Module()
        if echo:
            self.module.log(sys.stdout)

    def discover(self, program):
        self.module.load(program)
        while self.module.execute() != HALTED:
            pass
        x, y = 0, 0
        while self.module.output:
            v = chr(self.module.pop_output())
            if v == "\n":
                y += 1
                x = 0
            else:
                self.points[(x, y)] = v
                x += 1
        self.height = y

    def is_intersection(self, point):
        for d in self.directions:
            np = add(point, d)
            if self.points.get(np) != "#":
                return False

        return True

    def alignment(self):
        align = 0
        for point, value in self.points.items():
            if value in "#<^>v" and self.is_intersection(point):
                align += point[0] * point[1]

            p = "^>v<".find(value)
            if p != -1:
                self.startpos = point
                self.startdir = self.directions[p]

        return align

    def path(self):
        mypath = []
        curpos = self.startpos
        curdir = self.startdir
        while True:
            command = None
            leftdir = xmul(curdir, (0, -1))
            rightdir = xmul(curdir, (0, 1))
            if self.points.get(add(curpos, curdir)) == "#":
                length = 0
                npos = add(curpos, curdir)
                while self.points.get(npos) == "#":
                    curpos = npos
                    npos = add(npos, curdir)
                    length += 1
                command = length
            elif self.points.get(add(curpos, leftdir)) == "#":
                curdir = leftdir
                command = "L"
            elif self.points.get(add(curpos, rightdir)) == "#":
                curdir = rightdir
                command = "R"
            else:
                break

            mypath.append("{}".format(command))

        return mypath

    def reprogram(self, program, command, patterns):
        self.module.load(program)
        self.module.ram[0] = 2
        self.module.execute()
        while self.module.output:
            v = self.module.pop_output()

        # NOTE: main movement routine i.e. commands
        cmd = ",".join(command)+"\n"
        for v in cmd:
            self.module.push_input(ord(v))
        self.module.execute()
        while self.module.output:
            v = self.module.output.popleft()

        # NOTE: movements functions i.e. patterns
        for p in "ABC":
            cmd = ",".join(patterns[p]) + "\n"
            for v in cmd:
                self.module.push_input(ord(v))
            self.module.execute()
            while self.module.output:
                v = self.module.pop_output()

        # NOTE: disable continuous video feed
        cmd = "n\n"
        for v in cmd:
            self.module.push_input(ord(v))

        y = 0
        while True:
            self.module.execute()
            while self.module.output:
                v = self.module.pop_output()
                if v >= 256:
                    return v

        return 0

def subequal(sample, off1, off2, width):
    for i in range(width):
        sa = sample[off1+i]
        sb = sample[off2+i]
        if sa in "ABC" or sb in "ABC" or sa != sb:
            return False

    return True

def find_longest_pattern(sample, pos):
    for width in range(20, 1, -2):
        for i in range(pos+width, len(sample)-width):
            if subequal(sample, pos, i, width):
                return width

    return None

def replace(sample, src, dst):
    newsample = []
    srclen = len(src)
    j = 0
    for s in sample:
        # not quite correct but happens to work
        if s == src[j]:
            j += 1
            if j == srclen:
                newsample.append(dst)
                j = 0
        else:
            for k in range(j):
                newsample.append(src[k])
            newsample.append(s)
            j = 0

    return newsample

def build_patterns(sample):
    patterns = {}
    name = 65
    pos = 0
    worksample = sample
    while pos < len(worksample):
        if worksample[pos] in "ABC":
            pos += 1
        else:
            size = find_longest_pattern(worksample, pos)
            assert size, "Pattern not found"
            pattern = worksample[pos:pos+size]
            patterns[chr(name)] = pattern
            worksample = replace(worksample, pattern, chr(name))
            name += 1

    return worksample, patterns

program = []
with open("input", "rt") as file:
    program = [ int(x) for x in file.read().split(",") ]

m = Map(False)
m.discover(program)
print("part1:", m.alignment())
mypath = m.path()

command, patterns = build_patterns(mypath)
print("part2:", m.reprogram(program, command, patterns))
