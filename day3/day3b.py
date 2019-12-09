#!/usr/bin/env python3

from collections import defaultdict

def manhattan(p):
    return abs(p[0]) + abs(p[1])

class Cell(object):
    def __init__(self):
        self.flags = 0
        self.steps = 0

    def merge(self, flags, steps):
        self.flags |= flags
        self.steps += steps

class Map(object):
    def __init__(self):
        self.map = defaultdict(lambda: Cell())

    def clear(self):
        self.map.clear()

    def walk(self, actions, flags):
        x = 0
        y = 0
        step = 0
        for a in actions:
            dx = 0
            dy = 0
            d = a[0]
            if d == "R":
                dx = 1
            elif d == "L":
                dx = -1
            elif d == "U":
                dy = 1
            elif d == "D":
                dy = -1
            else:
                pass

            for z in range(int(a[1:])):
                x += dx
                y += dy
                step += 1
                self.map[(x, y)].merge(flags, step)

    def closest(self):
        dist = 100000000
        for k, v in self.map.items():
            if v.flags == 3:
                m = manhattan(k)
                if m < dist:
                    dist = m
        return dist

    def minsteps(self):
        dist = 100000000
        for k, v in self.map.items():
            if v.flags == 3 and v.steps < dist:
                dist = v.steps
        return dist

m = Map()
m.walk("R8,U5,L5,D3".split(","), 1)
m.walk("U7,R6,D4,L4".split(","), 2)
assert m.closest() == 6
assert m.minsteps() == 30

m.clear()
m.walk("R75,D30,R83,U83,L12,D49,R71,U7,L72".split(","), 1)
m.walk("U62,R66,U55,R34,D71,R55,D58,R83".split(","), 2)
assert m.closest() == 159
assert m.minsteps() == 610

m.clear()
m.walk("R98,U47,R26,D63,R33,U87,L62,D20,R33,U53,R51".split(","), 1)
m.walk("U98,R91,D20,R16,D67,R40,U7,R15,U6,R7".split(","), 2)
assert m.closest() == 135
assert m.minsteps() == 410

with open("input", "rt") as file:
    line1, line2 = file.read().split("\n")[:2]
    wire1 = line1.split(",")
    wire2 = line2.split(",")

m.clear()
m.walk(wire1, 1)
m.walk(wire2, 2)
print("part1:", m.closest())
print("part2:", m.minsteps())
