#!/usr/bin/env python3

ADD  = 1                         # add [a], [b] into [c]
MUL  = 2                         # mul [a], [b] into [c]
IN   = 3                         # read stdin and store to a
OUT  = 4                         # write a to stdout
JNZ  = 5                         # jump to b if a != 0
JZ   = 6                         # jump to b if a == 0
TLT  = 7                         # c = a < b
TEQ  = 8                         # c = a == b
HALT = 99                        # halt

PMODE = 0                       # positional mode: index into memory
IMODE = 1                       # immediate mode: literal

def intprogram(array):

    def address_of(addr, mode):
        "Return the address of a location for a given access mode"
        assert 0 <= addr & addr < len(array)
        if mode == IMODE:
            return addr
        else:
            return address_of(array[addr], IMODE)

    pc = 0
    while True:
        op = array[pc] % 100
        a_mode = (array[pc] // 100) % 10
        b_mode = (array[pc] // 1000) % 10
        c_mode = (array[pc] // 10000) % 10
        if op == ADD:
            a = address_of(pc+1, a_mode)
            b = address_of(pc+2, b_mode)
            c = address_of(pc+3, c_mode)

            assert c_mode == PMODE
            array[c] = array[a] + array[b]
            pc += 4
        elif op == MUL:
            a = address_of(pc+1, a_mode)
            b = address_of(pc+2, b_mode)
            c = address_of(pc+3, c_mode)

            assert c_mode == PMODE
            array[c] = array[a] * array[b]
            pc += 4
        elif op == IN:
            a = address_of(pc+1, a_mode)

            assert a_mode == PMODE
            array[a] = int(input("Enter value: "))
            pc += 2
        elif op == OUT:
            a = address_of(pc+1, a_mode)

            print("Value:", array[a])
            pc += 2
        elif op == JNZ:
            a = address_of(pc+1, a_mode)
            b = address_of(pc+2, b_mode)

            if array[a] != 0:
                pc = array[b]
            else:
                pc += 3
        elif op == JZ:
            a = address_of(pc+1, a_mode)
            b = address_of(pc+2, b_mode)

            if array[a] == 0:
                pc = array[b]
            else:
                pc += 3
        elif op == TLT:
            a = address_of(pc+1, a_mode)
            b = address_of(pc+2, b_mode)
            c = address_of(pc+3, c_mode)

            assert c_mode == PMODE
            array[c] = array[a] < array[b] and 1 or 0
            pc += 4
        elif op == TEQ:
            a = address_of(pc+1, a_mode)
            b = address_of(pc+2, b_mode)
            c = address_of(pc+3, c_mode)

            assert c_mode == PMODE
            array[c] = (array[a] == array[b]) and 1 or 0
            pc += 4
        elif op == HALT:
            break
        else:
            print("Unknown opcode:", op, pc)
            assert op, "Unknown opcode"

    return array

array = []
with open("input", "rt") as file:
    array = [ int(x) for x in file.read().split(",") ]

intprogram(array)
# intprogram([3,9,8,9,10,9,4,9,99,-1,8])
# intprogram([3,9,7,9,10,9,4,9,99,-1,8])
# intprogram([3,3,1108,-1,8,3,4,3,99])
# intprogram([3,3,1107,-1,8,3,4,3,99])
# intprogram([3,21,1008,21,8,20,1005,20,22,107,8,21,20,1006,20,31,
#             1106,0,36,98,0,0,1002,21,125,20,4,20,1105,1,46,104,
#             999,1105,1,46,1101,1000,1,20,4,20,1105,1,46,98,99])
