#!/usr/bin/env python3

import sys
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

    def command(self, cmd):
        for c in cmd:
            m.push_input(ord(c))
        m.execute()
        while m.output:
            print(chr(m.pop_output()), end="")

def add(a, b):
    return (a[0]+b[0], a[1]+b[1])

def sub(a, b):
    return (a[0]-b[0], a[1]-b[1])

class Location(object):
    def __init__(self, name, doors):
        self.name = name
        self.doors = doors
        self.link = {}
        self.parent = None

class Map(object):
    DOORS = {
        "north": (0, -1),
        "south": (0, 1),
        "west": (-1, 0),
        "east": (1, 0),
    }
    BACK = {
        "north": "south",
        "south": "north",
        "west": "east",
        "east": "west",
    }
    FORBIDDEN = set([
        "photons",
        "escape pod",
        "giant electromagnet",
        "infinite loop",
        "molten lava",
    ])

    def __init__(self, program):
        self.module = Module()
        self.module.log(sys.stdout)
        self.module.load(program)
        self.locations = {}
        self.target = {}
        self.door = None
        self.collected = []

    def readline(self):
        lst = []
        status = WAITING
        while True:
            if not self.module.output:
                if status == HALTED:
                    return None
                status = self.module.execute()
            else:
                c = self.module.pop_output()
                if c == ord("\n"):
                    break
                else:
                    lst.append(chr(c))

        return "".join(lst)

    def write(self, cmd):
        for c in cmd:
            self.module.push_input(ord(c))

    def wait_for_prompt(self):
        while True:
            line = self.readline()
            if line is None or line == "Command?":
                break

    def parse_location(self, pick=True):
        state = 0
        name = None
        doors = []
        items = []
        while True:
            line = self.readline()
            if line is None or line == "Command?":
                break
            elif state == 0:
                if line and line[0] == "=":
                    name = line
                elif line == "Doors here lead:":
                    state = 1
                elif line == "Items here:":
                    state = 2
            elif state == 1:
                if line and line[0] == "-":
                    doors.append(line[2:])
                else:
                    state = 0
            elif state == 2:
                if line and line[0] == "-":
                    items.append(line[2:])
                else:
                    state = 0

        if pick:
            for item in items:
                if item in self.FORBIDDEN:
                    pass
                else:
                    self.write("take {}\n".format(item))
                    self.wait_for_prompt()
                    self.collected.append(item)

        return Location(name, doors)

    def dfs(self, loc):
        for door in loc.doors:
            if door not in loc.link:
                self.write("{}\n".format(door))
                newloc = self.parse_location()

                # if we are stuck ignore the door and continue
                if newloc.name == loc.name:
                    self.target = loc
                    self.door = door
                    continue

                # if we already visited the location go back and
                # try another door
                if newloc.name in self.locations:
                    self.write("{}\n".format(self.BACK[door]))
                    self.wait_for_prompt()
                    continue

                # add the location to the map
                self.locations[newloc.name] = newloc

                # add the direct and reverse route to both the locations
                # we assume we can always go back
                loc.link[door] = newloc
                newloc.link[self.BACK[door]] = loc
                # the parent is the door that led to this location
                newloc.parent = door

                # explore the new location
                self.dfs(newloc)

                # and go back
                self.write("{}\n".format(self.BACK[door]))
                self.wait_for_prompt()

    def discover(self):
        self.locations.clear()
        loc = self.parse_location()
        self.locations[loc.name] = loc
        self.dfs(loc)

    def dot(self, filename):
        with open(filename, "wt") as f:
            print("digraph G {", file = f)
            for loc in self.locations.values():
                for door in loc.doors:
                    print("\t\"{}\" -> \"{}\" [label=\"{}\"];".format(
                        loc.name, loc.link[door].name, door), file=f)
            print("}", file = f)

    def solve(self):
        # find the path to the target
        path = []
        loc = self.target
        while loc.parent:
            path.append(loc.parent)
            loc = loc.link[self.BACK[loc.parent]]

        for door in reversed(path):
            self.write("{}\n".format(door))
            self.wait_for_prompt()

        length = len(self.collected)
        mask = (1<<length) - 1
        for i in range(1<<length):
            gray = i ^ (i>>1)
            for j in range(length):
                if (gray & 1<<j) != 0 and (mask & 1<<j) == 0:
                    self.write("take {}\n".format(self.collected[j]))
                    self.wait_for_prompt()
                elif (gray & 1<<j) == 0 and (mask & 1<<j) != 0:
                    self.write("drop {}\n".format(self.collected[j]))
                    self.wait_for_prompt()
            mask = gray
            self.write("{}\n".format(self.door))
            loc = self.parse_location(pick = False)
            if loc.name != self.target.name:
                # complete the map
                self.locations[loc.name] = loc
                self.target.link[self.door] = loc
                loc.link[self.BACK[self.door]] = self.target
                break

program = []
with open("input", "rt") as file:
    program = [ int(x) for x in file.read().split(",") ]

m = Map(program)
m.discover()
m.solve()
if len(sys.argv) > 1:
    m.dot(sys.argv[1])
