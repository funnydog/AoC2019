#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

#define TABLE_SIZE 64

enum
{
	EDGE_COMMON,		/* doesn't increment the level */
	EDGE_INNER,		/* increments the level */
	EDGE_OUTER,		/* decrements the level */
};

struct edge
{
	size_t y;
	int type;
	struct edge *next;
};

struct vertex
{
	struct edge *edges;
	unsigned deg;
};

struct graph
{
	struct vertex *vertices;
	size_t vcount;
	size_t start;
	size_t end;
};

struct passage
{
	char name[3];
	size_t positions[2];
	struct passage *next;
};

struct passages
{
	struct passage *table[TABLE_SIZE];
};

static unsigned hashfn(const char *name)
{
	unsigned hash = 5381;
	for (; *name; name++)
	{
		hash = hash * 33 + *name;
	}
	return hash;
}

static void passages_add(struct passages *pas, const char *name, size_t pos)
{
	unsigned i = hashfn(name) % TABLE_SIZE;
	struct passage *p = pas->table[i];
	while (p && strcmp(p->name, name))
	{
		p = p->next;
	}
	if (!p)
	{
		p = calloc(1, sizeof(*p));
		strcpy(p->name, name);
		p->positions[0] = p->positions[1] = SIZE_MAX;
		p->next = pas->table[i];
		pas->table[i] = p;
	}
	for (i = 0; i < 2; i++)
	{
		if (p->positions[i] == SIZE_MAX)
		{
			p->positions[i] = pos;
			break;
		}
	}
}

static void passages_destroy(struct passages *pas)
{
	for (size_t i = 0; i < TABLE_SIZE; i++)
	{
		struct passage *p = pas->table[i];
		while (p)
		{
			struct passage *t = p;
			p = p->next;
			free(t);
		}
	}
}

static void graph_free(struct graph *g)
{
	if (g)
	{
		for (size_t i = 0; i < g->vcount; i++)
		{
			struct edge *e = g->vertices[i].edges;
			while (e)
			{
				struct edge *t = e;
				e = e->next;
				free(t);
			}
		}
		free(g->vertices);
		free(g);
	}
}

static void graph_add_edge(struct graph *g, size_t x, size_t y, int type)
{
	assert(x < g->vcount && y < g->vcount);
	struct edge *e = malloc(sizeof(*e));
	if (e)
	{
		e->y = y;
		e->type = type;
		e->next = g->vertices[x].edges;
		g->vertices[x].edges = e;
		g->vertices[x].deg++;
	}
}

struct graph *graph_load(FILE *input)
{
	/* first load the map as a matrix of chars */
	char *points = NULL;
	size_t psize = 0;
	size_t pcount = 0;
	size_t width = 0;
	size_t height = 0;

	size_t lsize = 0;
	char *line = NULL;
	while (getline(&line, &lsize, input) != -1)
	{
		width = strlen(line);

		/* chop the \n */
		if (line[width-1] == '\n')
		{
			line[--width] = 0;
		}
		if (pcount == psize)
		{
			psize = psize ? psize * 2 : width;
			char *newpoints = realloc(points, psize);
			assert(newpoints);
			points = newpoints;
		}

		memmove(points + pcount, line, width);
		pcount += width;
		height++;
	}
	free(line);

	/* set the map from point to vertex */
	size_t *offsets = malloc(pcount * sizeof(offsets[0]));
	assert(offsets);
	for (size_t i = 0; i < pcount; i++)
	{
		offsets[i] = SIZE_MAX;
	}

	/* build the graph */
	struct graph *g = calloc(1, sizeof(*g));
	assert(g);
	g->start = g->end = SIZE_MAX;

	/* add the vertices */
	struct passages pas = {};
	char name[3];
	const int dx[] = {0, 1, 0, -1};
	const int dy[] = {-1, 0, 1, 0};
	unsigned up_left = UINT_MAX;
	unsigned low_right = UINT_MAX;
	size_t vsize = 0;
	for (size_t pos = 0; pos < pcount; pos++)
	{
		if (points[pos] == '.')
		{
			/* look for passages around us */
			for (int i = 0; i < 4; i++)
			{
				size_t np = pos + dx[i] + dy[i] * width;
				assert(np < pcount);
				if ('A' <= points[np] && points[np] <= 'Z')
				{
					size_t np2 = np + dx[i] + dy[i] * width;
					assert(np2 < pcount);
					if (np2 < np)
					{
						name[0] = points[np2];
						name[1] = points[np];
					}
					else
					{
						name[0] = points[np];
						name[1] = points[np2];
					}
					name[2] = 0;
					if (g->start == SIZE_MAX && strcmp(name, "AA") == 0)
					{
						g->start = g->vcount;
					}
					else if (g->end == SIZE_MAX && strcmp(name, "ZZ") == 0)
					{
						g->end = g->vcount;
					}
					else
					{
						passages_add(&pas, name, pos);
					}
				}
			}

			/* add the vertex */
			offsets[pos] = g->vcount;
			while (g->vcount == vsize)
			{
				vsize = vsize ? vsize * 2 : 8;
				struct vertex *nv = realloc(
					g->vertices, vsize*sizeof(*nv));
				assert(nv);
				g->vertices = nv;
			}
			struct vertex *v = g->vertices + g->vcount;
			v->edges = NULL;
			v->deg = 0;
			g->vcount++;
		}
		else if (points[pos] == '#')
		{
			if (up_left == UINT_MAX)
			{
				up_left = pos;
			}
			low_right = pos;
		}
	}

	/* add the passages */
	for (size_t i = 0; i < TABLE_SIZE; i++)
	{
		for (struct passage *p = pas.table[i];
		     p;
		     p = p->next)
		{
			for (int j = 0; j < 2; j++)
			{
				assert(p->positions[j] != SIZE_MAX);
				size_t x = p->positions[j] % width;
				size_t y = p->positions[j] / width;
				int outer = x == up_left % width
					|| x == low_right % width
					|| y == up_left / width
					|| y == low_right / width;
				graph_add_edge(
					g,
					offsets[p->positions[j]],
					offsets[p->positions[1-j]],
					outer ? EDGE_OUTER : EDGE_INNER);
			}
		}
	}
	passages_destroy(&pas);

	/* add the edges */
	for (unsigned pos = 0; pos < pcount; pos++)
	{
		if (points[pos] == '.')
		{
			for (int i = 1; i < 3; i++)
			{
				unsigned npos = pos + dx[i] + dy[i] * width;
				if (npos >= pcount)
				{
					continue;
				}
				if (points[npos] == '.')
				{
					size_t x = offsets[pos];
					size_t y = offsets[npos];
					graph_add_edge(g, x, y, EDGE_COMMON);
					graph_add_edge(g, y, x, EDGE_COMMON);
				}
			}
		}
	}

	free(offsets);
	free(points);
	return g;
}

static size_t graph_steps(struct graph *g)
{
	size_t *distance = calloc(g->vcount, sizeof(*distance));
	assert(distance);
	for (size_t i = 0; i < g->vcount; i++)
	{
		distance[i] = SIZE_MAX;
	}

	size_t queue[32];
	size_t qr = 0, qw = 0;

	distance[g->start] = 0;
	queue[(qw++) & 31] = g->start;
	while (qr != qw)
	{
		size_t pos = queue[(qr++) & 31];
		for (struct edge *e = g->vertices[pos].edges;
		     e;
		     e = e->next)
		{
			if (distance[e->y] != SIZE_MAX)
			{
				continue;
			}

			distance[e->y] = distance[pos] + 1;
			assert(qr+32 != qw);
			queue[(qw++) & 31] = e->y;
		}
	}

	size_t d = distance[g->end];
	free(distance);
	return d;
}

struct state
{
	size_t x;		/* vertex index */
	size_t level;		/* current level */
	size_t distance;	/* current distance */

	struct state *next;	/* next state in hashtable */
};

#define TABLE_SIZE2 65537		   /* too large? */

struct distance
{
	struct state *table[TABLE_SIZE2];
};

unsigned distance_hashfn(size_t x, size_t level)
{
	return level * 6353 + x;
}

struct state *distance_find(struct distance *d, size_t x, size_t level)
{
	unsigned pos = distance_hashfn(x, level) % TABLE_SIZE2;
	struct state *s = d->table[pos];
	while (s && !(s->x == x && s->level == level))
	{
		s = s->next;
	}
	return s;
}

struct state *distance_add(struct distance *d, size_t x, size_t level, unsigned distance)
{
	struct state *s = calloc(1, sizeof(*s));
	if (s)
	{
		s->x = x;
		s->level = level;
		s->distance = distance;

		unsigned pos = distance_hashfn(x, level) % TABLE_SIZE2;
		s->next = d->table[pos];
		d->table[pos] = s;
	}
	return s;
}

void distance_destroy(struct distance *d)
{
	for (size_t i = 0; i < TABLE_SIZE2; i++)
	{
		struct state *s = d->table[i];
		while (s)
		{
			struct state *t = s;
			s = s->next;
			free(t);
		}
	}
}

struct heap
{
	struct state **a;
	size_t size;
	size_t count;
};

void bubble_up(struct heap *h, size_t pos)
{
	while (pos > 0)
	{
		size_t parent = (pos - 1) / 2;

		/* check if the dominance relation is satisfied */
		if (h->a[pos]->distance > h->a[parent]->distance)
		{
			break;
		}

		struct state *t = h->a[pos];
		h->a[pos] = h->a[parent];
		h->a[parent] = t;

		pos = parent;
	}
}

void bubble_down(struct heap *h, size_t pos)
{
	for (;;)
	{
		/* find the extreme between parent and children */
		size_t max = pos;
		for (size_t j = 2 * pos + 1; j <= 2 * pos + 2; j++)
		{
			if (j < h->count && h->a[max]->distance > h->a[j]->distance)
			{
				max = j;
			}
		}

		/* exit if the extreme is the parent, i.e. the
		 * dominance relation is satisfied */
		if (max == pos)
		{
			break;
		}

		/* else swap the parent with the bigger child and
		 * repeat */
		struct state *t = h->a[pos];
		h->a[pos] = h->a[max];
		h->a[max] = t;

		pos = max;
	}
}

void heap_push(struct heap *h, struct state *s)
{
	if (h->count == h->size)
	{
		size_t newsize = h->size ? h->size * 2 : 64;
		struct state **na = realloc(h->a, newsize * sizeof(*na));
		assert(na);
		h->size = newsize;
		h->a = na;
	}
	h->a[h->count] = s;
	bubble_up(h, h->count);
	h->count++;
}

struct state *heap_pop(struct heap *h)
{
	assert(h->count > 0);
	struct state *r = h->a[0];
	h->a[0] = h->a[--h->count];
	bubble_down(h, 0);
	return r;
}

void heap_destroy(struct heap *h)
{
	free(h->a);
	h->a = NULL;
	h->count = h->size = 0;
}

static size_t graph_steps_recursive(struct graph *g)
{
	struct distance distance = {};
	struct heap h = {};
	struct state *cur = distance_add(&distance, g->start, 0, 0);
	heap_push(&h, cur);
	while (h.count)
	{
		cur = heap_pop(&h);
		for (struct edge *e = g->vertices[cur->x].edges;
		     e;
		     e = e->next)
		{
			size_t level = cur->level;
			if (e->type == EDGE_COMMON)
			{
				/* do nothing */
			}
			else if (e->type == EDGE_INNER)
			{
				level++;
			}
			else if (e->type == EDGE_OUTER && level > 0)
			{
				level--;
			}
			else
			{
				continue;
			}

			size_t newdist = cur->distance + 1;
			struct state *s = distance_find(&distance, e->y, level);
			if (!s || s->distance > newdist)
			{
				if (!s)
				{
					s = distance_add(&distance, e->y, level, newdist);
				}
				else
				{
					s->distance = newdist;
				}
				heap_push(&h, s);
				if (e->y == g->end && level == 0)
				{
					h.count = 0;
					break;
				}
			}
		}
	}

	size_t dist = SIZE_MAX;
	cur = distance_find(&distance, g->end, 0);
	if (cur)
	{
		dist = cur->distance;
	}
	heap_destroy(&h);
	distance_destroy(&distance);
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
		fprintf(stderr, "Cannot open %s for reading\n", argv[1]);
		return -1;
	}
	struct graph *g = graph_load(input);
	fclose(input);
	printf("part1: %zu\n", graph_steps(g));
	printf("part2: %zu\n", graph_steps_recursive(g));
	graph_free(g);
	return 0;
}
