#!/usr/bin/env

from collections import deque
from heapq import heappush, heappop

def add(a, b):
    return (a[0]+b[0], a[1]+b[1])

class Map(object):
    ADJACENT = ((0,-1), (1,0), (0,1), (-1,0))

    def __init__(self):
        self.points = {}
        self.start = None
        self.keys = []

    def load(self, txt):
        self.points.clear()
        for y, row in enumerate(txt.split("\n")):
            for x, v in enumerate(row):
                self.points[x, y] = v
                if v == "@":
                    self.start = (x, y)

    def patch(self):
        for d in self.ADJACENT:
            self.points[add(self.start, d)] = "#"
        return [add(self.start, d) for d in ((-1,-1), (1, -1), (1, 1), (-1, 1))]

    def bfs(self, start):
        distance = {}
        parent = {}
        q = deque()

        distance[start] = 0
        parent[start] = None
        q.append(start)
        while q:
            pos = q.popleft()
            for d in self.ADJACENT:
                npos = add(pos, d)
                if npos in distance:
                    continue
                elif self.points[npos] == "#":
                    continue
                else:
                    distance[npos] = distance[pos]+1;
                    parent[npos] = pos
                    q.append(npos)

        return distance, parent

class Edge(object):
    def __init__(self, y, distance, needed):
        self.y = y
        self.distance = distance
        self.needed = needed

class Vertex(object):
    def __init__(self, key, pos):
        self.key = key
        self.pos = pos
        self.edges = []

class Graph(object):
    def __init__(self):
        self.vertices = []
        self.start = None
        self.goal = 0

    def add_edge(self, i, j, distance, needed):
        self.vertices[i].edges.append(Edge(j, distance, needed))

    def bitkey(self, v):
        i = "abcdefghijklmnopqrstuvwxyz".find(v)
        if i == -1:
            return 0
        else:
            return 1<<i

    def build(self, m, start = None):
        self.vertices.clear()
        self.start = None
        self.goal = 0

        # create the vertices
        start = start or m.start
        distance, parent = m.bfs(start)
        for pos in distance.keys():
            v = m.points[pos]
            i = self.bitkey(v)
            if pos == start:
                self.start = len(self.vertices)
            elif i != 0:
                self.goal |= i
            else:
                continue
            self.vertices.append(Vertex(v, pos))

        # create the edges
        for i, src in enumerate(self.vertices):
            distance, parent = m.bfs(src.pos)
            for j in range(i+1, len(self.vertices)):
                dst = self.vertices[j]
                needed = 0
                pos = parent[dst.pos]
                while pos != src.pos:
                    if m.points[pos] in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
                        needed |= self.bitkey(m.points[pos].lower())
                    pos = parent[pos]

                needed &= self.goal
                self.add_edge(i, j, distance[dst.pos], needed)
                self.add_edge(j, i, distance[dst.pos], needed)

    def search(self, m, start = None):
        self.build(m, start)
        lowest = 10000000000
        distance = {}
        q = []
        distance[self.start, 0] = 0
        heappush(q, (0, self.start, 0))
        while q:
            dist, pos, wallet = heappop(q)
            for e in self.vertices[pos].edges:
                if e.y == self.start:
                    continue

                newwallet = wallet | self.bitkey(self.vertices[e.y].key)
                newdist = dist + e.distance

                if newwallet == wallet:
                    continue

                if e.needed | wallet != wallet:
                    continue

                if (e.y, newwallet) not in distance or distance[e.y, newwallet] > newdist:
                    distance[e.y, newwallet] = newdist
                    heappush(q, (newdist, e.y, newwallet))
                    if newwallet == self.goal and lowest > newdist:
                        lowest = newdist

        return lowest

    def search_many(self, m):
        starts = m.patch()
        count = 0
        for s in starts:
            count += self.search(m, s)
        return count

g = Graph()
m = Map()
m.load("""########################
#f.D.E.e.C.b.A.@.a.B.c.#
######################.#
#d.....................#
########################""")
assert g.search(m) == 86

m.load("""########################
#...............b.C.D.f#
#.######################
#.....@.a.B.c.d.A.e.F.g#
########################""")
assert g.search(m) == 132

m.load("""#################
#i.G..c...e..H.p#
########.########
#j.A..b...f..D.o#
########@########
#k.E..a...g..B.n#
########.########
#l.F..d...h..C.m#
#################""")
assert g.search(m) == 136

m.load("""########################
#@..............ac.GI.b#
###d#e#f################
###A#B#C################
###g#h#i################
########################""")
assert g.search(m) == 81

m.load("""#######
#a.#Cd#
##...##
##.@.##
##...##
#cB#Ab#
#######""")
assert g.search_many(m) == 8

m.load("""###############
#d.ABC.#.....a#
######...######
######.@.######
######...######
#b.....#.....c#
###############""")
assert g.search_many(m) == 24

m.load("""#############
#DcBa.#.GhKl#
#.###...#I###
#e#d#.@.#j#k#
###C#...###J#
#fEbA.#.FgHi#
#############""")
assert g.search_many(m) == 32

with open("input", "rt") as f:
    m.load(f.read())

print("part1:", g.search(m))
print("part2:", g.search_many(m))
