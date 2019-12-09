#!/usr/bin/env python3

import sys

BLACK = 0
WHITE = 1
TRANS = 2

def layers(seq, width, height):
    layers = []
    i = 0
    while i < len(seq):
        layer = []
        for y in range(height):
            row = []
            for x in range(width):
                row.append(int(seq[i]))
                i += 1
            layer.append(row)
        layers.append(layer)
    return layers

def count_digits(layer, digit):
    count = 0
    for row in layer:
        for x in row:
            if x == digit:
                count += 1
    return count

def minimal_layer(layers, digit):
    height, width = len(layers[0]), len(layers[0][0])
    mind = height * width
    minl = -1
    for i, l in enumerate(layers):
        c = count_digits(l, digit)
        if mind > c:
            mind = c
            minl = i

    if minl != -1:
        return layers[minl]

    return None

def stack_layers(layers):
    height, width = len(layers[0]), len(layers[0][0])
    output = [[TRANS for x in range(width)] for y in range(height)]
    for l in layers:
        for y in range(height):
            for x in range(width):
                if output[y][x] == TRANS:
                    output[y][x] = l[y][x]
    return output

def print_layer(layer):
    for row in layer:
        for x in row:
            if x == BLACK:
                sys.stdout.write(" ")
            elif x == WHITE:
                sys.stdout.write("#")
            else:
                sys.stdout.write(".")
        sys.stdout.write("\n")

with open("input", "rt") as file:
    seq = file.read().split("\n")[0]

l = layers(seq, 25, 6)
m = minimal_layer(l, BLACK)
if m:
    print("part1:", count_digits(m, WHITE) * count_digits(m, TRANS))

print("part2:")
print_layer(stack_layers(l))
