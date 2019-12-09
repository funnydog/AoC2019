#!/usr/bin/env

def make_deps(s):
    couples = [x.split(")") for x in s.split("\n") if x]

    nodes = {}
    for a, b in couples:
        nodes[b] = a

    return nodes

def count_orbits(deps):
    count = 0
    for k, n in deps.items():
        while n:
            n = deps.get(n)
            count += 1

    return count

def path_to_origin(deps, start):
    d = []
    n = deps.get(start)
    while n:
        d.append(n)
        n = deps.get(n)

    return d

def path(deps, start, end):
    path1 = path_to_origin(deps, start)
    path2 = path_to_origin(deps, end)

    while path1 and path2 and path1[-1] == path2[-1]:
        path1.pop()
        path2.pop()

    return len(path1) + len(path2)

test = """COM)B
B)C
C)D
D)E
E)F
B)G
G)H
D)I
E)J
J)K
K)L"""
assert count_orbits(make_deps(test)) == 42
test = """COM)B
B)C
C)D
D)E
E)F
B)G
G)H
D)I
E)J
J)K
K)L
K)YOU
I)SAN"""
assert path(make_deps(test), "SAN", "YOU") == 4

with open("input", "rt") as file:
    st = file.read()

deps = make_deps(st)
print("part1", count_orbits(deps))
print("part2", path(deps, "YOU", "SAN"))
