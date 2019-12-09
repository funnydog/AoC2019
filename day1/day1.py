#!/usr/bin/env python3

import math

def fuel(mass):
    return math.floor(mass/3) - 2

def recursive_fuel(f):
    t = 0
    while f > 0:
        f = fuel(f)
        if f < 0:
            break
        t += f

    return t

array = []
with open("input", "rt") as file:
    array = [int(x) for x in file.read().split("\n") if x]

assert fuel(12) == 2, "fuel check failed for 12"
assert fuel(14) == 2, "fuel check failed for 14"
assert fuel(1969) == 654, "fuel check failed for 1969"
assert fuel(100756) == 33583, "fuel check failed for 100756"

total_fuel = 0
for mass in array:
    total_fuel += fuel(mass)

print("part 1:", total_fuel)

assert recursive_fuel(14) == 2, "recursive fuel check failed for 14"
assert recursive_fuel(1969) == 966, "recursive fuel check failed for 1969"
assert recursive_fuel(100756) == 50346, "recursive fuel check failed for 100756"

total_fuel = 0
for mass in array:
    total_fuel += recursive_fuel(mass)

print("part 2:", total_fuel)
