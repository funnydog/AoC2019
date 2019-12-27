#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define TABLE_SIZE 1024

struct asteroid
{
	int x;
	int y;
	int count;
	int vaporized;

	struct asteroid *next;
	struct asteroid *nvap;
};

struct map
{
	struct asteroid *table[TABLE_SIZE];
	size_t count;
};

unsigned hashfn(int x, int y)
{
	return 5287 + x * 33 + y;
}

struct asteroid *map_find(struct map *m, int x, int y)
{
	unsigned pos = hashfn(x, y) & (TABLE_SIZE-1);
	struct asteroid *n = m->table[pos];
	while (n && !(n->x == x && n->y == y))
	{
		n = n->next;
	}
	return n;
}

struct asteroid *map_add(struct map *m, int x, int y)
{
	unsigned pos = hashfn(x, y) & (TABLE_SIZE-1);
	struct asteroid *n = calloc(1, sizeof(*n));
	if (n)
	{
		n->x = x;
		n->y = y;
		n->next = m->table[pos];
		m->table[pos] = n;
		m->count++;
	}
	return n;
}

void map_destroy(struct map *m)
{
	for (int i = 0; i < TABLE_SIZE; i++)
	{
		struct asteroid *a = m->table[i];
		while (a)
		{
			struct asteroid *t = a;
			a = a->next;
			free(t);
		}
	}
}

int gcd(int a, int b)
{
	if (a < 0)
	{
		a = -a;
	}
	if (b < 0)
	{
		b = -b;
	}
	while (b)
	{
		int t = b;
		b = a % b;
		a = t;
	}
	return a;
}

struct asteroid *line_of_sight(struct map *m, struct asteroid *p, struct asteroid *q)
{
	int dx = q->x - p->x;
	int dy = q->y - p->y;
	int g = gcd(dx, dy);
	if (g != 0)
	{
		dx /= g;
		dy /= g;
	}

	int x = p->x + dx;
	int y = p->y + dy;
	while (!(x == q->x && y == q->y))
	{
		struct asteroid *t = map_find(m, x, y);
		if (t)
		{
			return t;
		}
		x += dx;
		y += dy;
	}
	return q;
}

struct asteroid *best_position(struct map *m)
{
	int maxcount = 0;
	struct asteroid *best = NULL;
	for (int i = 0; i < TABLE_SIZE; i++)
	{
		for (struct asteroid *p = m->table[i]; p; p = p->next)
		{
			p->count = 0;
			for (int j = 0; j < TABLE_SIZE; j++)
			{
				for (struct asteroid *q = m->table[j]; q; q = q->next)
				{
					if (p != q && line_of_sight(m, p, q) == q)
					{
						p->count++;
					}
				}
			}
			if (maxcount < p->count)
			{
				maxcount = p->count;
				best = p;
			}
		}
	}
	return best;
}

int acmp(const void *a, const void *b)
{
	const struct asteroid *p = *(const struct asteroid **)a;
	const struct asteroid *q = *(const struct asteroid **)b;

	assert(p && q);

	double pr = -atan2(p->x, p->y);
	double qr = -atan2(q->x, q->y);
	if (pr < qr)
	{
		return -1;
	}
	else if (pr > qr)
	{
		return +1;
	}
	else
	{
		return p->x*p->x + p->y*p->y - q->x*q->x + q->y*q->y;
	}
}

struct asteroid **sorted_vectors(struct map *m, struct asteroid *q)
{
	struct asteroid **a = calloc(m->count, sizeof(*a));
	if (!a)
	{
		return NULL;
	}

	size_t i = 0;
	for (int j = 0; j < TABLE_SIZE; j++)
	{
		for (struct asteroid *p = m->table[j];
		     p;
		     p = p->next)
		{
			if (p != q)
			{
				p->x -= q->x;
				p->y -= q->y;
				a[i++] = p;
			}
		}
	}

	qsort(a, i, sizeof(*a), acmp);
	return a;
}

struct asteroid *vaporize(struct map *m, struct asteroid *p)
{
	struct asteroid **arr = sorted_vectors(m, p);
	size_t count = m->count-1;

	struct asteroid *head = NULL;
	struct asteroid **tail = &head;

	size_t i = 0;
	size_t left_intact = count;
	while (left_intact)
	{
		/* find the asteroid to vaporize */
		struct asteroid *q = arr[i];
		while (q->vaporized)
		{
			i = (i + 1) % count;
			q = arr[i];
		}

		/* vaporize it */
		q->vaporized = 1;
		q->nvap = NULL;
		left_intact--;

		*tail = q;
		tail = &q->nvap;

		/* skip the asteroids shadowed by the vaporized one */
		double angle = -atan2(q->x, q->y);
		while (angle == -atan2(arr[i]->x, arr[i]->y))
		{
			i = (i + 1) % count;
		}
	}
	free(arr);
	return head;
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
		fprintf(stderr, "Cannot open %s for reading\n", argv[1]);
		return -1;
	}

	struct map m = {};
	char *line = NULL;
	size_t size = 0;
	int y = 0;
	while (getline(&line, &size, input) != -1)
	{
		for (int x = 0; line[x]; x++)
		{
			if (line[x] == '#')
			{
				map_add(&m, x, y);
			}
		}
		y++;
	}
	free(line);
	fclose(input);

	struct asteroid *best = best_position(&m);
	printf("part1: %d\n", best->count);

	struct asteroid *lst = vaporize(&m, best);
	for (int i = 1; lst && i < 200; i++)
	{
		lst = lst->nvap;
	}
	printf("part2: %d\n", (lst->x + best->x) * 100 + lst->y + best->y);
	map_destroy(&m);

	return 0;
}
