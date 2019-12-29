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
        self.snapshot = None

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

class Map(object):
    backwards = [ 2, 1, 4, 3 ]
    directions = [(0, -1), (0, +1), (-1, 0), (+1, 0)]

    def __init__(self):
        self.points = {}
        self.distances = {}
        self.max_distance = 0
        self.start = (0,0)
        self.oxygen = None
        self.module = Module()

    def dfs(self, point, value):
        if value == 2:
            self.oxygen = point

        self.points[point] = value
        for i in range(4):
            self.module.push_input(i+1)
            self.module.execute()
            v = self.module.pop_output()
            if v == 0:
                # WALL
                pass
            else:
                newpoint = add(point, self.directions[i])
                if not newpoint in self.points:
                    self.dfs(newpoint, v)

                # backtracking
                self.module.push_input(self.backwards[i])
                self.module.execute()
                self.module.pop_output()

    def discover(self, program):
        self.module.load(program)
        self.start = (0, 0)
        self.dfs(self.start, 1)

    def bfs(self, start):
        self.max_distance = 0
        self.distances.clear()
        self.distances[start] = 0
        q = deque()
        q.append(start)
        while q:
            point = q.popleft()
            for delta in self.directions:
                new_point = add(point, delta)
                if new_point in self.points and new_point not in self.distances:
                    q.append(new_point)
                    new_distance = self.distances[point] + 1
                    self.distances[new_point] = new_distance
                    if self.max_distance < new_distance:
                        self.max_distance = new_distance

    def __str__(self):
        rows = []
        miny = min(y for x,y in self.points.keys())-1
        maxy = max(y for x,y in self.points.keys())+1
        minx = min(x for x,y in self.points.keys())-1
        maxx = max(x for x,y in self.points.keys())+1
        for y in range(miny, maxy+1):
            row = []
            for x in range(minx, maxx+1):
                v = self.points.get((x, y))
                if x == 0 and y == 0:
                    row.append("*")
                elif v == 1:
                    row.append(".")
                elif v == 2:
                    row.append("O")
                else:
                    row.append("#")
            rows.append("".join(row))
        return "\n".join(rows)

program = []
with open("input", "rt") as file:
    program = [ int(x) for x in file.read().split(",") ]

m = Map()
m.discover(program)
print(m)
m.bfs((0,0))
print("part1:", m.distances[m.oxygen])
m.bfs(m.oxygen)
print("part2:", m.max_distance)
