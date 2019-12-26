#!/usr/bin/env python3

from collections import deque

def add(a, b):
    return (a[0]+b[0], a[1]+b[1])

def xmul(a, b):
    return (a[0]*b[0]-a[1]*b[1], a[0]*b[1] + a[1]*b[0])

key_labels = set("abcdefghijklmnopqrstuvwxyz")
door_labels = set("ABCDEFGHIJKLMNOPQRSTUWVXYZ")

class Map(object):
    def __init__(self, txt):
        self.grid = [[x for x in row] for row in txt.split("\n")]
        self.start = []
        self.height = 0
        self.width = 0
        self.cache = {}
        self.robot_positions = {}
        self.keys = {}
        self.path_cache = {}
        self.build_map(txt)

    def build_map(self, txt):
        start = []
        for y, row in enumerate(self.grid):
            for x, v in enumerate(row):
                if v == "@":
                    start.append((x, y))
                    self.robot_positions[(x, y)] = v
                elif v in key_labels:
                    self.robot_positions[(x, y)] = v
                    self.keys[v] = (x, y)

        if len(start) == 1:
            self.start = start[0]
        else:
            self.start = tuple(start)

        self.width = x+1
        self.height = y+1

    def patch_map(self):
        x, y = self.start
        self.grid[y][x]="#"
        self.grid[y][x+1]="#"
        self.grid[y][x-1]="#"
        self.grid[y+1][x]="#"
        self.grid[y-1][x]="#"
        self.grid[y+1][x+1]="@"
        self.grid[y+1][x-1]="@"
        self.grid[y-1][x+1]="@"
        self.grid[y-1][x-1]="@"
        del self.robot_positions[self.start]
        self.start = (
            (x-1, y-1),
            (x+1, y-1),
            (x-1, y+1),
            (x+1, y+1),
        )
        for s in self.start:
            self.robot_positions[s] = "@"

    def get(self, pos):
        return self.grid[pos[1]][pos[0]]

    def bfs(self, start):
        # Breadth First Search
        distance = {start: 0}
        parent = {start: None}
        q = deque()
        q.append(start)
        while q:
            p = q.popleft()
            for dd in ((0,-1),(1,0),(0,1),(-1,0)):
                np = add(p, dd)
                v = self.get(np)
                if v != "#" and np not in distance:
                    distance[np] = distance[p] + 1
                    parent[np] = p
                    q.append(np)

        return distance, parent

    def build_path_cache(self):
        self.path_cache.clear()
        for rp1, rv1 in self.robot_positions.items():
            distance, parent = self.bfs(rp1)
            for rp2, rv2 in self.robot_positions.items():
                if rp1 != rp2 and rp2 in distance:
                    doors = []
                    keys = []
                    par = parent[rp2]
                    while par:
                        v = self.get(par)
                        if v in door_labels:
                            doors.append(v)
                        par = parent[par]
                    self.path_cache[rp1, rp2] = (distance[rp2], doors)

    def unlock(self, doors, collected_keys):
        for d in doors:
            if d.lower() not in collected_keys:
                return False
        return True

    def minsteps(self, p1, collected_keys=""):
        if not self.path_cache:
            self.build_path_cache()

        kstr = "".join(sorted(collected_keys))
        if (p1, kstr) in self.cache:
            return self.cache[p1, kstr]

        new_keys = []
        for k2, p2 in self.keys.items():
            if k2 not in collected_keys:
                if (p1, p2) in self.path_cache:
                    dist, doors = self.path_cache[p1, p2]
                    if self.unlock(doors, collected_keys):
                        new_keys.append((k2, p2, dist))

        rv = 0
        if new_keys:
            rv = None
            for nkey, npos, ndist in new_keys:
                newdist = ndist + self.minsteps(npos, collected_keys + nkey)
                if rv is None or rv > newdist:
                    rv = newdist

        self.cache[p1, kstr] = rv
        return rv

    def manysteps(self, p1s, collected_keys=""):
        if not self.path_cache:
            self.build_path_cache()

        kstr = "".join(sorted(collected_keys))
        if (p1s, kstr) in self.cache:
            return self.cache[p1s, kstr]

        newkeys = []
        for k2, p2 in self.keys.items():
            if k2 not in collected_keys:
                for i, p1 in enumerate(p1s):
                    if (p1, p2) in self.path_cache:
                        dist, doors = self.path_cache[p1, p2]
                        if self.unlock(doors, collected_keys):
                            newkeys.append((k2, p2, dist, i))

        rv = 0
        if newkeys:
            rv = 100000
            for nk, pos, dist, robot in newkeys:
                np1s = tuple(pos if i == robot else oldp for i, oldp in enumerate(p1s))
                newd = dist + self.manysteps(np1s, collected_keys + nk)
                if rv > newd:
                    rv = newd
        self.cache[p1s, kstr] = rv
        return rv

txt = """#########
#b.A.@.a#
#########"""
# txt = """########################
# #f.D.E.e.C.b.A.@.a.B.c.#
# ######################.#
# #d.....................#
# ########################"""
# txt = """########################
# #...............b.C.D.f#
# #.######################
# #.....@.a.B.c.d.A.e.F.g#
# ########################"""
txt="""#################
#i.G..c...e..H.p#
########.########
#j.A..b...f..D.o#
########@########
#k.E..a...g..B.n#
########.########
#l.F..d...h..C.m#
#################"""

m = Map(txt)
assert m.minsteps(m.start, "") == 136

txt="""###############
#d.ABC.#.....a#
######@#@######
###############
######@#@######
#b.....#.....c#
###############"""
m = Map(txt)
assert m.manysteps(m.start, "") == 24

txt="""#############
#g#f.D#..h#l#
#F###e#E###.#
#dCba@#@BcIJ#
#############
#nK.L@#@G...#
#M###N#H###.#
#o#m..#i#jk.#
#############"""
m = Map(txt)
assert m.manysteps(m.start, "") == 72

with open("input", "rt") as f:
    txt = f.read().strip()

m = Map(txt)
print("part1:", m.minsteps(m.start, ""))

m = Map(txt)
m.patch_map()
print("part2:", m.manysteps(m.start, ""))
