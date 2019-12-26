#!/usr/bin/env

DELTA = ((0,-1), (1, 0), (0, 1), (-1, 0))

def evolve(bug):
    new = 0
    bit = 1
    for y in range(5):
        for x in range(5):
            count = 0
            for dx, dy in DELTA:
                ny = y + dy
                nx = x + dx
                if 0 <= nx < 5 and 0 <= ny < 5:
                    if bug & 1 << (ny * 5 + nx):
                        count += 1
            if count == 1 or (count == 2 and bug & bit == 0):
                new |= bit
            bit <<= 1

    return new

def first_of_cycle(original):
    stack = []
    bug = original
    while True:
        while stack and stack[-1] > bug:
            stack.pop()
        if stack and stack[-1] == bug:
            break
        stack.append(bug)
        bug = evolve(bug)

    cycleset = set()
    old = bug
    while True:
        bug = evolve(bug)
        cycleset.add(bug)
        if bug == old:
            break

    bug = original
    while bug not in cycleset:
        bug = evolve(bug)

    return bug

def parse(txt):
    bit = 1
    bug = 0
    for c in list(txt):
        if c == "." or c == "?":
            bit <<= 1
        elif c == "#":
            bug |= bit
            bit <<= 1
        else:
            pass
    return bug

def popcount(x):
    return bin(x).count("1")

class Bugmap(object):
    def __init__(self, bug):
        self.bugs = {0: bug}
        self.minlevel = 0
        self.maxlevel = 0

    def count_adjacent(self, level, x, y):
        if x == 2 and y == 2:
            return 0

        count = 0
        for i, (dx, dy) in enumerate(DELTA):
            nx = x + dx
            ny = y + dy
            if nx < 0:
                if self.bugs.get(level-1, 0) & 1<<11:
                    count += 1
            elif nx >= 5:
                if self.bugs.get(level-1, 0) & 1<<13:
                    count += 1
            elif ny < 0:
                if self.bugs.get(level-1, 0) & 1<<7:
                    count += 1
            elif ny >= 5:
                if self.bugs.get(level-1, 0) & 1<<17:
                    count += 1
            elif nx == 2 and ny == 2:
                bug = self.bugs.get(level+1, 0)
                if i == 0:
                    count += popcount(bug & 0x1f00000)
                elif i == 1:
                    count += popcount(bug & 0x108421)
                elif i == 2:
                    count += popcount(bug & 0x1f)
                else:
                    count += popcount(bug & 0x1084210)
            else:
                if self.bugs[level] & 1<<(ny * 5 + nx):
                    count += 1
        return count

    def evolve(self):
        new = {}
        minlevel, maxlevel = self.minlevel, self.maxlevel

        # create a new minlevel if needed
        bug = self.bugs[minlevel]
        newbug = 0
        if popcount(bug & 0x1f) in (1,2):
            newbug |= 1<<7
        if popcount(bug & 0x108421) in (1,2):
            newbug |= 1<<11
        if popcount(bug & 0x1084210) in (1,2):
            newbug |= 1<<13
        if popcount(bug & 0x1f00000) in (1,2):
            newbug |= 1<<17
        if newbug:
            minlevel -= 1
            new[minlevel] = newbug

        # update the existings levels
        for level, bug in self.bugs.items():
            bit = 1
            newbug = 0
            for y in range(5):
                for x in range(5):
                    count = self.count_adjacent(level, x, y)
                    if count == 1 or (count == 2 and bug & bit == 0):
                        newbug |= bit
                    bit <<= 1
            new[level] = newbug

        # create a new maxlevel if needed
        bug = self.bugs[maxlevel]
        newbug = 0
        if bug & 1<<7:
            newbug |= 0x1f
        if bug & 1<<11:
            newbug |= 0x108421
        if bug & 1<<13:
            newbug |= 0x1084210
        if bug & 1<<17:
            newbug |= 0x1f00000
        if newbug:
            maxlevel += 1
            new[maxlevel] = newbug

        self.bugs = new
        self.minlevel = minlevel
        self.maxlevel = maxlevel

    def count_bugs(self):
        return sum(popcount(bug) for bug in self.bugs.values())

with open("input", "rt") as f:
    bug = parse(f.read())

print("part1:", first_of_cycle(bug))

bugmap = Bugmap(bug)
for i in range(200):
    bugmap.evolve()

print("part2:", bugmap.count_bugs())
