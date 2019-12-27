#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 4096

struct cell
{
	int x, y;
	int flags;
	int steps;

	struct cell *next;
};

struct map
{
	struct cell *table[SIZE];
	struct cell *free;
};

static unsigned hash(int x, int y)
{
	return (x*33 + y + 5189);
}

static void map_clear(struct map *m)
{
	for (int i = 0; i < SIZE; i++)
	{
		struct cell *c = m->table[i];
		while (c)
		{
			struct cell *t = c;
			c = c->next;

			t->next = m->free;
			m->free = t;
		}
		m->table[i] = NULL;
	}
}

static void map_destroy(struct map *m)
{
	map_clear(m);
	while (m->free)
	{
		struct cell *c = m->free;
		m->free = m->free->next;
		free(c);
	}
}

static struct cell *map_get(struct map *m, int x, int y)
{
	unsigned idx = hash(x, y) % SIZE;
	struct cell *c = m->table[idx];
	while (c && !(c->x == x && c->y == y))
	{
		c = c->next;
	}
	if (c == NULL)
	{
		if (m->free)
		{
			c = m->free;
			m->free = c->next;
			memset(c, 0, sizeof(*c));
		}
		else
		{
			c = calloc(1, sizeof(*c));
		}
		c->x = x;
		c->y = y;
		c->next = m->table[idx];
		m->table[idx] = c;
	}
	return c;
}

static void map_walk(struct map *m, char *str, int flags)
{
	char *tok = strtok(str, ",\n");
	int x = 0;
	int y = 0;
	int steps = 0;
	while (tok && tok[0])
	{
		int action = tok[0];
		int len = atoi(tok+1);

		int dx = 0;
		int dy = 0;
		switch (action)
		{
		case 'R': dx = 1; break;
		case 'L': dx = -1; break;
		case 'U': dy = 1; break;
		case 'D': dy = -1; break;
		default:
			fprintf(stderr, "unknown direction %c\n", action);
			abort();
			break;
		}

		for (int i = 0; i < len; i++)
		{
			x += dx;
			y += dy;
			steps++;
			struct cell *c = map_get(m, x, y);
			c->flags |= flags;
			c->steps += steps;
		}

		tok = strtok(NULL, ",\n");
	}
}

static int manhattan(int x, int y)
{
	return ((x>0)?(x):(-x)) + ((y>0)?(y):(-y));
}

static int closest(struct map *m)
{
	int dist = INT_MAX;
	for (int i = 0; i < SIZE; i++)
	{
		for (struct cell *c = m->table[i]; c; c = c->next)
		{
			if (c->flags == 3)
			{
				int md = manhattan(c->x, c->y);
				if (md < dist)
				{
					dist = md;
				}
			}
		}
	}
	return dist;
}

static int minsteps(struct map *m)
{
	int dist = INT_MAX;
	for (int i = 0; i < SIZE; i++)
	{
		for (struct cell *c = m->table[i]; c; c = c->next)
		{
			if (c->flags == 3 && c->steps < dist)
			{
				dist = c->steps;
			}
		}
	}
	return dist;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <input>\n", argv[0]);
		return -1;
	}

	FILE *input = fopen(argv[1], "rb");
	if (!input)
	{
		fprintf(stderr, "File %s not found\n", argv[1]);
		return -1;
	}

	struct map m = {};
	char buffer[255];
	strcpy(buffer, "R8,U5,L5,D3");
	map_walk(&m, buffer, 1);
	strcpy(buffer, "U7,R6,D4,L4");
	map_walk(&m, buffer, 2);
	assert(closest(&m) == 6);
	assert(minsteps(&m) == 30);

	map_clear(&m);
	strcpy(buffer, "R75,D30,R83,U83,L12,D49,R71,U7,L72");
	map_walk(&m, buffer, 1);
	strcpy(buffer, "U62,R66,U55,R34,D71,R55,D58,R83");
	map_walk(&m, buffer, 2);
	assert(closest(&m) == 159);
	assert(minsteps(&m) == 610);

	map_clear(&m);
	strcpy(buffer, "R98,U47,R26,D63,R33,U87,L62,D20,R33,U53,R51");
	map_walk(&m, buffer, 1);
	strcpy(buffer, "U98,R91,D20,R16,D67,R40,U7,R15,U6,R7");
	map_walk(&m, buffer, 2);
	assert(closest(&m) == 135);
	assert(minsteps(&m) == 410);

	map_clear(&m);
	char *line = NULL;
	size_t lsize = 0;
	int i = 0;
	while (getline(&line, &lsize, input) != -1)
	{
		map_walk(&m, line, ++i);
	}
	free(line);
	fclose(input);

	printf("part1: %d\n", closest(&m));
	printf("part2: %d\n", minsteps(&m));

	map_destroy(&m);

	return 0;
}
