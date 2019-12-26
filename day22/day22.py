#!/usr/bin/env python

import re
cmd_cut = re.compile(r"cut (-?\d+)")
cmd_inc = re.compile(r"deal with increment (-?\d+)")
cmd_new = re.compile(r"deal into new stack")

def parse(txt):
    cmds = []
    for cmd in txt.split("\n"):
        m = cmd_cut.match(cmd)
        if m:
            v = int(m.group(1))
            cmds.append((0, v))
            continue
        m = cmd_inc.match(cmd)
        if m:
            v = int(m.group(1))
            cmds.append((1, v))
            continue
        m = cmd_new.match(cmd)
        if m:
            cmds.append((2, 0))
            continue
    return cmds

def get_period(dlen, inc):
    n = dlen + 1
    while n % inc != 0:
        n += dlen
    return (n // inc) % dlen

def shuffle(cmds, decksize, times):
    start, period = 0, 1
    for cmd, val in cmds:
        if cmd == 0:
            # cut
            start = (start + val * period) % decksize
        elif cmd == 1:
            # inc
            period = (period * get_period(decksize, val)) % decksize
        else:
            # new
            period = decksize - period
            start = (start + period) % decksize

    start = start * (1 - pow(period, times, decksize)) * pow(1-period, decksize-2, decksize) % decksize
    period = pow(period, times, decksize)

    return decksize, start, period

def index_of(deck, value):
    decksize, start, period = deck
    assert value < decksize
    i = 0
    while start != value:
        start = (start + period) % decksize
        i += 1
    return i

def value_of(deck, index):
    decksize, start, period = deck
    for i in range(index):
        start = (start + period) % decksize

    return start

with open("input", "rt") as f:
    commands = parse(f.read())

deck = shuffle(commands, 10007, 1)
print("part1:", index_of(deck, 2019))

deck = shuffle(commands, 119315717514047, 101741582076661)
print("part2:", value_of(deck, 2020))
