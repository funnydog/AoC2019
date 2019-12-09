#!/usr/bin/env python3

def rule2(s):
    p = None
    for a in s:
        if a == p:
            return True
        p = a
    return False

def rule3(s):
    p = "0"
    for a in s:
        if a < p:
            return False
        p = a
    return True

def rule4(s):
    t = len(s)
    p = None
    c = 0
    for a in s:
        if a == p:
            c += 1
        else:
            if c > 0 and c < t:
                t = c
            c = 0
        p = a

    if c > 0 and c < t:
        t = c

    return t == 1

x = 111111
assert rule2(str(x)) and rule3(str(x))
x = 223450
assert rule2(str(x)) and not rule3(str(x))
x = 123789
assert not rule2(str(x)) and rule3(str(x))
assert rule4("112233")
assert not rule4("123444")
assert rule4("11122")

p1 = 0
p2 = 0
for x in range(109165,576723+1):
    s = str(x)
    r3 = rule3(s)
    if rule2(s) and r3:
        p1 += 1
    if r3 and rule4(s):
        p2 += 1

print("part1", p1)
print("part2", p2)
