#!/usr/bin/env python3

from collections import defaultdict, deque
import heapq

def add(a, b):
    return (a[0]+b[0], a[1]+b[1])

def sub(a, b):
    return (a[0]-b[0], a[1]-b[1])

def xmul(a, b):
    return (a[0]*b[0]-a[1]*b[1], a[0]*b[1] + a[1]*b[0])

class Map(object):
    DIRS = ((0, -1), (1, 0), (0, 1), (-1, 0))
    PASSAGE = 0
    PORTAL = 1

    def __init__(self):
        self.points = defaultdict(lambda: [])
        self.portals = {}
        self.inner = {}

    def load(self, txt):
        self.points.clear()
        upper_left_corner = None
        lower_right_corner = None
        dct = defaultdict(lambda: [])
        grid = {(x,y): v for y, row in enumerate(txt.split("\n")) for x,v in enumerate(row)}
        for p, v in grid.items():
            if v == ".":
                lst = []
                for d in self.DIRS:
                    np = add(p, d)
                    if grid.get(np) == ".":
                        lst.append((self.PASSAGE, np))
                self.points[p] = lst
            elif v == "#":
                if upper_left_corner is None:
                    upper_left_corner = p
                lower_right_corner = p
            elif "A" <= v <= "Z":
                for d in self.DIRS:
                    np = add(p, d)
                    nv = grid.get(np)
                    if nv == ".":
                        if p[0] < np[0] or p[1] < np[1]:
                            portal = grid.get(sub(p, d)) + v
                        else:
                            portal = v + grid.get(sub(p, d))
                        dct[portal].append(np)

        self.portals.clear()
        self.inner.clear()
        for gate, links in dct.items():
            # cache the inner/outer state of the portal
            for p in links:
                if p[0] == upper_left_corner[0] or p[0] == lower_right_corner[0] \
                   or p[1] == upper_left_corner[1] or p[1] == lower_right_corner[1]:
                    self.inner[p] = False
                else:
                    self.inner[p] = True

            # add the portal passages
            if len(links) == 2:
                self.points[links[0]].append((self.PORTAL, links[1]))
                self.points[links[1]].append((self.PORTAL, links[0]))
            elif gate == "AA":
                self.start = links[0]
            elif gate == "ZZ":
                self.end = links[0]

    def bfs(self, start):
        distance = {start: 0}
        q = deque()
        q.append(start)
        while q:
            pos = q.popleft()
            for t, npos in self.points[pos]:
                if npos in distance:
                    continue
                if npos == self.end:
                    return distance[pos] + 1

                distance[npos] = distance[pos] + 1
                q.append(npos)

    def dijkstra(self, start):
        distance = {(0, start): 0}
        q = []
        heapq.heappush(q, (0, start))
        while q:
            level, pos = heapq.heappop(q)
            for t, npos in self.points[pos]:
                if t == self.PASSAGE:
                    nlevel = level
                elif self.inner[pos]:
                    nlevel = level + 1
                elif level > 0:
                    nlevel = level - 1
                else:
                    continue

                nstate = (nlevel, npos)
                ndist = distance[(level, pos)] + 1
                if nstate not in distance or ndist < distance[nstate]:
                    if npos == self.end and level == 0:
                        return ndist
                    distance[nstate] = ndist
                    heapq.heappush(q, nstate)


txt="""
         A           
         A           
  #######.#########  
  #######.........#  
  #######.#######.#  
  #######.#######.#  
  #######.#######.#  
  #####  B    ###.#  
BC...##  C    ###.#  
  ##.##       ###.#  
  ##...DE  F  ###.#  
  #####    G  ###.#  
  #########.#####.#  
DE..#######...###.#  
  #.#########.###.#  
FG..#########.....#  
  ###########.#####  
             Z       
             Z       
"""
m = Map()
m.load(txt)
assert m.bfs(m.start) == 23
assert m.dijkstra(m.start) == 26

txt="""
                   A               
                   A               
  #################.#############  
  #.#...#...................#.#.#  
  #.#.#.###.###.###.#########.#.#  
  #.#.#.......#...#.....#.#.#...#  
  #.#########.###.#####.#.#.###.#  
  #.............#.#.....#.......#  
  ###.###########.###.#####.#.#.#  
  #.....#        A   C    #.#.#.#  
  #######        S   P    #####.#  
  #.#...#                 #......VT
  #.#.#.#                 #.#####  
  #...#.#               YN....#.#  
  #.###.#                 #####.#  
DI....#.#                 #.....#  
  #####.#                 #.###.#  
ZZ......#               QG....#..AS
  ###.###                 #######  
JO..#.#.#                 #.....#  
  #.#.#.#                 ###.#.#  
  #...#..DI             BU....#..LF
  #####.#                 #.#####  
YN......#               VT..#....QG
  #.###.#                 #.###.#  
  #.#...#                 #.....#  
  ###.###    J L     J    #.#.###  
  #.....#    O F     P    #.#...#  
  #.###.#####.#.#####.#####.###.#  
  #...#.#.#...#.....#.....#.#...#  
  #.#####.###.###.#.#.#########.#  
  #...#.#.....#...#.#.#.#.....#.#  
  #.###.#####.###.###.#.#.#######  
  #.#.........#...#.............#  
  #########.###.###.#############  
           B   J   C               
           U   P   P               
"""
m.load(txt)
assert m.bfs(m.start) == 58


txt="""
             Z L X W       C                 
             Z P Q B       K                 
  ###########.#.#.#.#######.###############  
  #...#.......#.#.......#.#.......#.#.#...#  
  ###.#.#.#.#.#.#.#.###.#.#.#######.#.#.###  
  #.#...#.#.#...#.#.#...#...#...#.#.......#  
  #.###.#######.###.###.#.###.###.#.#######  
  #...#.......#.#...#...#.............#...#  
  #.#########.#######.#.#######.#######.###  
  #...#.#    F       R I       Z    #.#.#.#  
  #.###.#    D       E C       H    #.#.#.#  
  #.#...#                           #...#.#  
  #.###.#                           #.###.#  
  #.#....OA                       WB..#.#..ZH
  #.###.#                           #.#.#.#  
CJ......#                           #.....#  
  #######                           #######  
  #.#....CK                         #......IC
  #.###.#                           #.###.#  
  #.....#                           #...#.#  
  ###.###                           #.#.#.#  
XF....#.#                         RF..#.#.#  
  #####.#                           #######  
  #......CJ                       NM..#...#  
  ###.#.#                           #.###.#  
RE....#.#                           #......RF
  ###.###        X   X       L      #.#.#.#  
  #.....#        F   Q       P      #.#.#.#  
  ###.###########.###.#######.#########.###  
  #.....#...#.....#.......#...#.....#.#...#  
  #####.#.###.#######.#######.###.###.#.#.#  
  #.......#.......#.#.#.#.#...#...#...#.#.#  
  #####.###.#####.#.#.#.#.###.###.#.###.###  
  #.......#.....#.#...#...............#...#  
  #############.#.#.###.###################  
               A O F   N                     
               A A D   M                     
"""
m.load(txt)
assert m.dijkstra(m.start) == 396

with open("input", "rt") as f:
    txt = f.read()

m.load(txt)
print("part1:", m.bfs(m.start))
print("part2:", m.dijkstra(m.start))
