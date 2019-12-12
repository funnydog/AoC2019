#!/usr/bin/env python3

import re
import math

class Moon(object):
    def __init__(self, pos):
        self.pos = pos
        self.vel = [0, 0, 0]

pattern = re.compile(r"<x=(-?[0-9]+), y=(-?[0-9]+), z=(-?[0-9]+)>")
def load_moons(text):
    moons = []
    for line in text.split("\n"):
        m = pattern.search(line)
        if m:
            moons.append(Moon([int(x) for x in m.groups()]))
    return moons

def update_step(moons, axis):
    # update the velocity
    for i in range(len(moons)):
        for j in range(i+1, len(moons)):
            if moons[i].pos[axis] < moons[j].pos[axis]:
                moons[i].vel[axis] += 1
                moons[j].vel[axis] -= 1
            elif moons[i].pos[axis] > moons[j].pos[axis]:
                moons[i].vel[axis] -= 1
                moons[j].vel[axis] += 1
            else:
                pass

    # update the position
    for m in moons:
        m.pos[axis] += m.vel[axis]

def energy(moons, steps):
    for axis in range(3):
        for i in range(steps):
            update_step(moons, axis)

    energy = 0
    for m in moons:
        p = sum(abs(i) for i in m.pos)
        k = sum(abs(i) for i in m.vel)
        energy += k*p

    return energy

def repeat_axis(moons, axis):
    pos = [m.pos[axis] for m in moons]
    vel = [m.vel[axis] for m in moons]

    count = 0
    while True:
        update_step(moons, axis)
        count += 1

        exit = True
        for i, m in enumerate(moons):
            if m.pos[axis] != pos[i] or m.vel[axis] != vel[i]:
                exit = False
                break
        if exit:
            break

    return count

def lcm(a, b):
    return a * b // math.gcd(a, b)

def repeat(moons):
    x = repeat_axis(moons, 0)
    y = repeat_axis(moons, 1)
    z = repeat_axis(moons, 2)
    return lcm(lcm(x, y), z)

moons = load_moons("""
<x=-1, y=0, z=2>
<x=2, y=-10, z=-7>
<x=4, y=-8, z=8>
<x=3, y=5, z=-1>
""")
assert energy(moons, 10) == 179
assert repeat(moons) == 2772

moons = load_moons("""
<x=-8, y=-10, z=0>
<x=5, y=5, z=10>
<x=2, y=-7, z=3>
<x=9, y=-8, z=-3>
""")
assert energy(moons, 100) == 1940
assert repeat(moons) == 4686774924

with open("input", "rt") as f:
    moons = load_moons(f.read())

print("part1:", energy(moons, 1000))
print("part2:", repeat(moons))
