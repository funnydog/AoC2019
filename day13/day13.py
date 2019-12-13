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


def xmul(a, b):
    return (a[0]*b[0]-a[1]*b[1], a[0]*b[1]+a[1]*b[0])

def add(a, b):
    return (a[0]+b[0], a[1]+b[1])

class Screen(object):
    def __init__(self):
        self.reset()
        self.game = Module(4096)

    def reset(self):
        self.screen = {}
        self.score = 0
        self.ball = 0
        self.paddle = 0

    def update(self):
        while self.game.output:
            x = self.game.output.popleft()
            y = self.game.output.popleft()
            tile_id = self.game.output.popleft()
            if x == -1 and y == 0:
                self.score = tile_id
            else:
                self.screen[(x,y)] = tile_id
                if tile_id == 3:
                    self.paddle = x
                elif tile_id == 4:
                    self.ball = x

    def run(self, ram):
        self.reset()
        self.game.load(ram)
        height = 0
        while self.game.execute() != HALTED:
            self.update()
            height = self.paint(height)
            if self.paddle < self.ball:
                self.game.push_input(1)
            elif self.paddle > self.ball:
                self.game.push_input(-1)
            else:
                self.game.push_input(0)

        self.update()
        self.paint(height)

    def paint(self, up):
        if up:
            print("\033[{}A".format(up), end="")
        x0 = min(x for x,y in self.screen.keys())
        x1 = max(x for x,y in self.screen.keys())
        y0 = min(y for x,y in self.screen.keys())
        y1 = max(y for x,y in self.screen.keys())
        for y in range(y0, y1+1):
            for x in range(x0, x1+1):
                v = self.screen.get((x,y))
                c = " "
                if v == 1:
                    c = "*"
                elif v == 2:
                    c = "#"
                elif v == 3:
                    c = "="
                elif v == 4:
                    c = "o"
                print(c, end="")
            print("")
        return y1-y0+1

    def count(self, tile_id):
        return len([x for x in self.screen.values() if x == tile_id])

ram = []
with open("input", "rt") as file:
    ram = [ int(x) for x in file.read().split(",") ]

s = Screen()
s.run(ram)
print("part1:", s.count(2))

ram[0] = 2
s.run(ram)
print("part2:", s.score)
