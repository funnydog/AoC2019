#!/usr/bin/env python3

pattern = [0, 1, 0, -1]

def get_sum(signal, slen, i):
    global pattern

    start = 0
    left = slen
    v = 0
    while start < slen:
        val = pattern[(start+1)//(i+1) % 4]
        size = min(left, i+1 - (start+1) % (i+1))
        if val == 0:
            pass
        elif val == 1:
            v += sum(signal[start:start+size])
        elif val == -1:
            v -= sum(signal[start:start+size])

        start += size
        left -= size

    return abs(v) % 10

def part1(txt, n):
    signal = [int(x) for x in txt.strip()]
    slen = len(signal)
    for phase in range(n):
        for j in range(slen):
            signal[j] = get_sum(signal, slen, j)

    return "".join(str(x) for x in signal)[:8]

def part2(txt, n):
    signal = [int(x) for x in txt.strip()]
    offset = int(txt[:7])
    assert(offset > len(signal) * 10000 //2)

    # For i > len//2 the s[i] = sum(s_old[i..len])
    # If we start from the last element (that never changes)
    # we can build the previous elements with a running sum
    # and the value of the old element.
    slen = len(signal)
    signal = [signal[pos % slen] for pos in range(offset, slen*10000)]
    slen = len(signal)
    for i in range(n):
        running_sum = 0
        for j in range(slen-1, -1, -1):
            running_sum += signal[j]
            signal[j] = abs(running_sum) % 10

    return "".join(str(signal[i]) for i in range(8))

assert part1("12345678", 4) == "01029498"
assert part1("80871224585914546619083218645595", 100) == "24176176"
assert part1("19617804207202209144916044189917", 100) == "73745418"
assert part1("69317163492948606335995924319873", 100) == "52432133"

assert part2("03036732577212944063491565474664", 100) == "84462026"
assert part2("02935109699940807407585447034323", 100) == "78725270"
assert part2("03081770884921959731165446850517", 100) == "53553731"

with open("input", "rt") as f:
    txt = f.read()

print("part1:", part1(txt, 100))
print("part2:", part2(txt, 100))
