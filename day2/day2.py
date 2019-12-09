#!/usr/bin/env python3

ADD = 1                         # add [a], [b] into [c]
MUL = 2                         # mul [a], [b] into [c]
HALT = 99                       # halt

def intprogram(array):
    pc = 0
    while True:
        op = array[pc]
        if op == ADD:
            a = array[pc+1]
            b = array[pc+2]
            c = array[pc+3]
            array[c] = array[a] + array[b]
        elif op == MUL:
            a = array[pc+1]
            b = array[pc+2]
            c = array[pc+3]
            array[c] = array[a] * array[b]
        elif op == HALT:
            break
        else:
            assert op, "Unknown opcode"
        pc += 4
    return array


assert intprogram([1,9,10,3,2,3,11,0,99,30,40,50]) == [3500,9,10,70,2,3,11,0,99,30,40,50], "Program 1 failed"
assert intprogram([1,0,0,0,99]) == [2,0,0,0,99], "Program 2 failed"
assert intprogram([2,3,0,3,99]) == [2,3,0,6,99], "Program 3 failed"
assert intprogram([2,4,4,5,99,0]) == [2,4,4,5,99,9801], "Program 4 failed"
assert intprogram([1,1,1,4,99,5,6,0,99]) == [30,1,1,4,2,5,6,0,99], "Program 5 failed"

array = []
with open("input", "rt") as file:
    array = [ int(x) for x in file.read().split(",") ]

# restore the state
array[1] = 12
array[2] = 2
out = intprogram(array[:])

print("value at position 0:", out[0])

# brute force take
for i in range(0,100):
    for j in range(0, 100):
        array[1] = i
        array[2] = j
        out = intprogram(array[:])
        if out[0] == 19690720:
            print("100 * {} + {} = {}".format(i, j, i * 100 + j))
            exit(0)
