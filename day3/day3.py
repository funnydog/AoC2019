#!/usr/bin/env python3

from collections import defaultdict

def end(p, action):
    d = action[0]
    l = int(action[1:])
    if d == "R":
        return (p[0]+l, p[1])
    elif d == "L":
        return (p[0]-l, p[1])
    elif d == "U":
        return (p[0], p[1]+l)
    elif d == "D":
        return (p[0], p[1]-l)
    else:
        print("ERROR")
        exit(-1)

def segments(actions):
    segments = []
    p = (0,0)
    for a in actions:
        e = end(p, a)
        segments.append((p, e))
        p = e

    return segments

def intersect(s1, s2):
    vertical = lambda s: s[0][0] == s[1][0]

    v1 = vertical(s1)
    v2 = vertical(s2)
    if v1 == v2:
        # colinear
        return None
    elif v1:
        s = s1
        s1 = s2
        s2 = s
    else:
        pass

    # s1 horizontal, s2 vertical
    if s2[0][0] > max(s1[0][0], s1[1][0]) or \
       s2[0][0] < min(s1[0][0], s1[1][0]) or \
       s1[0][1] > max(s2[0][1], s2[1][1]) or \
       s1[0][1] < min(s2[0][1], s2[1][1]):
        return None

    return (s2[0][0], s1[0][1])

def manhattan(p):
    return abs(p[0]) + abs(p[1])

def intersections(wire1, wire2):
    inter = []
    segs1 = segments(wire1)
    segs2 = segments(wire2)
    for s1 in segs1:
        for s2 in segs2:
            p = intersect(s1, s2)
            if p:
                inter.append(p)

    return inter

def closest(inters):
    mindist = 1000000000
    for p in inters:
        d = manhattan(p)
        if d != 0 and d < mindist:
            mindist = d

    return mindist

def add_steps(interset, actions, steps):
    x = 0
    y = 0
    step = 0
    for w in actions:
        dx = 0
        dy = 0
        if w[0] == "R":
            dx = 1
        elif w[0] == "L":
            dx = -1
        elif w[0] == "U":
            dy = 1
        elif w[0] == "D":
            dy = -1
        else:
            print("ERROR")

        for l in range(int(w[1:])):
            x += dx
            y += dy
            step += 1
            if (x,y) in interset:
                steps[(x,y)] += step

def min_steps(inters, wire1, wire2):
    steps = defaultdict(lambda: 0)
    interset = set(inters)
    add_steps(interset, wire1, steps)
    add_steps(interset, wire2, steps)
    minsteps = 100000
    for v in steps.values():
        if v < minsteps:
            minsteps = v

    return minsteps

w1 = "R8,U5,L5,D3".split(",")
w2 = "U7,R6,D4,L4".split(",")
inters = intersections(w1, w2)
assert closest(inters) == 6
assert min_steps(inters, w1, w2) == 30

w1 = "R75,D30,R83,U83,L12,D49,R71,U7,L72".split(",")
w2 = "U62,R66,U55,R34,D71,R55,D58,R83".split(",")
inters = intersections(w1, w2)
assert closest(inters) == 159
assert min_steps(inters, w1, w2) == 610

w1 = "R98,U47,R26,D63,R33,U87,L62,D20,R33,U53,R51".split(",")
w2 = "U98,R91,D20,R16,D67,R40,U7,R15,U6,R7".split(",")
inters = intersections(w1, w2)
assert closest(inters) == 135
assert min_steps(inters, w1, w2) == 410

with open("input", "rt") as file:
    line1, line2 = file.read().split("\n")[:2]
    wire1 = line1.split(",")
    wire2 = line2.split(",")

inters = intersections(wire1, wire2)
print("part1:", closest(inters))
print("part2:", min_steps(inters, wire1, wire2))
