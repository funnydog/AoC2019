#!/usr/bin/env python3

import math

def asteroids(table):
    asteroids = set()
    for py, y in enumerate(table):
        for px, x in enumerate(y):
            if x == "#":
                asteroids.add((px, py))

    return asteroids

def line_of_sight(asteroids, p0, p1):
    x0, y0 = p0
    x1, y1 = p1
    dx = x1-x0
    dy = y1-y0
    g = math.gcd(dx, dy)
    if g:
        dx //= g
        dy //= g

    x0 += dx
    y0 += dy
    while x0 != x1 or y0 != y1:
        if (x0, y0) in asteroids:
            return (x0, y0)
        x0 += dx
        y0 += dy

    return p1

def best(table):
    aster = asteroids(table)
    maxcount = 0
    bestpoint = None
    for a1 in aster:
        count = 0
        for a2 in aster:
            if a1 != a2:
                if line_of_sight(aster, a1, a2) == a2:
                    count += 1

        if maxcount < count:
            maxcount = count
            bestpoint = a1

    return bestpoint, maxcount

def vaporize(asteroids, width, height, p):
    asteroids.remove(p)

    dist = lambda p: p[0]*p[0]+p[1]*p[1]
    angle = lambda p: -math.atan2(p[0], p[1])
    vectors = sorted(
        [(x[0]-p[0], x[1]-p[1]) for x in asteroids],
        key = lambda p: (angle(p), dist(p))
    )
    lst = []
    i = 0
    while vectors:
        v = vectors.pop(i)
        lst.append((v[0]+p[0], v[1]+p[1]))
        av = angle(v)
        while i < len(vectors) and angle(vectors[i]) == av:
            i += 1
        if i >= len(vectors):
            i = 0
    return lst

table = """.#..#
.....
#####
....#
...##""".split("\n")
assert best(table) == ((3,4), 8)
table ="""......#.#.
#..#.#....
..#######.
.#.#.###..
.#..#.....
..#....#.#
#..#....#.
.##.#..###
##...#..#.
.#....####""".split("\n")
assert best(table) == ((5, 8), 33)
table = """#.#...#.#.
.###....#.
.#....#...
##.#.#.#.#
....#.#.#.
.##..###.#
..#...##..
..##....##
......#...
.####.###.""".split("\n")
assert best(table) == ((1,2), 35)
table = """.#..#..###
####.###.#
....###.#.
..###.##.#
##.##.#.#.
....###..#
..#.#..#.#
#..#.#.###
.##...##.#
.....#.#..""".split("\n")
assert best(table) == ((6,3),41)
table = """.#..##.###...#######
##.############..##.
.#.######.########.#
.###.#######.####.#.
#####.##.#.##.###.##
..#####..#.#########
####################
#.####....###.#.#.##
##.#################
#####.##.###..####..
..######..##.#######
####.##.####...##..#
.#####..#.######.###
##...#.##########...
#.##########.#######
.####.#.###.###.#.##
....##.##.###..#####
.#.#.###########.###
#.#.#.#####.####.###
###.##.####.##.#..##""".split("\n")
assert best(table) == ((11,13), 210)
arr = vaporize(asteroids(table), len(table[0]), len(table), (11, 13))
assert arr[0] == (11,12), arr[0]
assert arr[1] == (12, 1), arr[1]
assert arr[2] == (12, 2), arr[2]
assert arr[9] == (12,8)
assert arr[19] == (16,0)
assert arr[49] == (16,9)
assert arr[99] == (10,16)
assert arr[198] == (9, 6)
assert arr[199] == (8,2)
assert arr[200] == (10,9)
assert arr[298] == (11,1)

with open("input", "rt") as file:
    table = file.read().split("\n")

p, count = best(table)
print("part1:", count)
arr = vaporize(asteroids(table), len(table[0]), len(table), p)
print("part2:", arr[199][0]*100 + arr[199][1])
