#!/usr/bin/env python3

import re
import math

pattern = re.compile(r"<x=(-?[0-9]+), y=(-?[0-9]+), z=(-?[0-9]+)>")
def load_moons(text):
    moons = []
    for line in text.split("\n"):
        m = pattern.search(line)
        if m:
            moons.append([int(x) for x in m.groups()])
    return moons

def update_axis(pos, vel):
    # update the velocity
    for i in range(len(pos)):
        for j in range(i+1, len(pos)):
            if pos[i] < pos[j]:
                vel[i] += 1
                vel[j] -= 1
            elif pos[i] > pos[j]:
                vel[i] -= 1
                vel[j] += 1
            else:
                pass

    # update the gravity
    for i in range(len(pos)):
        pos[i] += vel[i]

def energy_after_steps(moons, steps):
    pos = []
    vel = []
    for i in range(3):
        p = [m[i] for m in moons]
        v = [0 for m in moons]
        for i in range(steps):
            update_axis(p, v)

        pos.append(p)
        vel.append(v)

    energy = 0
    for j in range(len(moons)):
        k = sum(abs(vel[i][j]) for i in range(3))
        p = sum(abs(pos[i][j]) for i in range(3))
        energy += k*p

    return energy

def simulate_axis(pos, vel):
    oldpos = tuple(pos)
    oldvel = tuple(vel)
    count = 0
    while True:
        update_axis(pos, vel)
        count += 1
        if tuple(pos) == oldpos and tuple(vel) == oldvel:
            break

    return count

def lcm(a, b):
    return a * b // math.gcd(a, b)

def steps_to_state(moons):
    xcount = simulate_axis(
        [p[0] for p in moons],
        [0 for p in moons]
    )

    ycount = simulate_axis(
        [p[1] for p in moons],
        [0 for p in moons]
    )

    zcount = simulate_axis(
        [p[2] for p in moons],
        [0 for p in moons]
    )
    return lcm(lcm(xcount, ycount), zcount)

txt = """
<x=-1, y=0, z=2>
<x=2, y=-10, z=-7>
<x=4, y=-8, z=8>
<x=3, y=5, z=-1>
"""
assert energy_after_steps(load_moons(txt), 10) == 179
assert steps_to_state(load_moons(txt)) == 2772

txt = """
<x=-8, y=-10, z=0>
<x=5, y=5, z=10>
<x=2, y=-7, z=3>
<x=9, y=-8, z=-3>
"""
assert energy_after_steps(load_moons(txt), 100) == 1940
assert steps_to_state(load_moons(txt)) == 4686774924

with open("input", "rt") as f:
    txt = f.read()

print("part1:", energy_after_steps(load_moons(txt), 1000))
print("part2:", steps_to_state(load_moons(txt)))
