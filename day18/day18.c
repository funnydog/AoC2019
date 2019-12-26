#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct edge
{
	size_t y;
	unsigned distance;
	unsigned needed;

	struct edge *next;
};

struct vertex
{
	int key;		/* key name */
	unsigned pos;		/* original position in the map */
	struct edge *edges;	/* edges to other keys */
	struct edge **tail;	/* to keep the order of the edges */
	unsigned deg;
};

struct graph
{
	struct vertex vertices[32];
	size_t vcount;
	size_t start;
	unsigned goal;
};

struct map
{
	char *points;
	unsigned pcount;
	unsigned height;
	unsigned width;
	unsigned keys;

	unsigned start;
};

void map_free(struct map *m)
{
	if (m)
	{
		free(m->points);
		free(m);
	}
}

static unsigned map_index(struct map *m, unsigned x, unsigned y)
{
	return y * m->width + x;
}

struct map *map_load(FILE *input)
{
	struct map *m = calloc(1, sizeof(*m));
	if (m)
	{
		size_t psize = 0;
		size_t lsize = 0;
		char *line = NULL;
		while (getline(&line, &lsize, input) != -1)
		{
			m->width = strlen(line)-1;

			/* chop the \n */
			if (line[m->width] == '\n')
			{
				line[m->width] = 0;
			}

			/* find the start position as offset */
			char *s = strchr(line, '@');
			if (s)
			{
				m->start = m->height * m->width + s-line;
			}

			if (m->pcount == psize)
			{
				size_t newsize = psize ? psize * 2 : m->width;
				char *newpoints = realloc(m->points, newsize);
				if (!newpoints)
				{
					map_free(m);
					return NULL;
				}
				psize = newsize;
				m->points = newpoints;
			}
			memcpy(m->points+m->pcount, line, m->width);
			m->pcount += m->width;
			m->height++;
		}
		free(line);
	}
	return m;
}

static void map_patch(struct map *m)
{
	const int dx[] = {0, 1, 0, -1};
	const int dy[] = {-1, 0, 1, 0};
	for (int i = 0; i < 4; i++)
	{
		m->points[m->start + map_index(m, dx[i], dy[i])] = '#';
	}
}

static void map_bfs(struct map *m, unsigned pos, unsigned *distance, unsigned *parent)
{
	for (unsigned i = 0; i < m->pcount; i++)
	{
		distance[i] = parent[i] = UINT_MAX;
	}
	distance[pos] = 0;

	const int dx[] = { 0, 1, 0, -1 };
	const int dy[] = { -1, 0, 1, 0 };
	unsigned queue[32];
	unsigned qr, qw;
	qr = qw = 0;
	queue[(qw++)&31] = pos;
	while (qr < qw)
	{
		pos = queue[(qr++)&31];
		for (int i = 0; i < 4; i++)
		{
			/*
			 * NOTE: here we should check if we are
			 * looking outside the map but we dont. We get
			 * away with it because the map has a border
			 * of walls.
			 */
			unsigned npos = pos + map_index(m, dx[i], dy[i]);
			assert(npos < m->pcount);
			if (m->points[npos] == '#')
			{
				continue;
			}
			if (distance[npos] != UINT_MAX)
			{
				continue;
			}
			distance[npos] = distance[pos]+1;
			parent[npos] = pos;
			assert(qw-qr <= 32);
			queue[(qw++)&31] = npos;
		}
	}
}

void graph_free(struct graph *g)
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
		free(g);
	}
}

void graph_add_edge(struct graph *g, size_t x, size_t y, int distance, unsigned needed)
{
	struct edge *e = calloc(1, sizeof(*e));
	if (e)
	{
		e->y = y;
		e->distance = distance;
		e->needed = needed;
		*g->vertices[x].tail = e;
		g->vertices[x].tail = &e->next;
		g->vertices[x].deg++;
	}
}

unsigned key_to_bit(int key)
{
	if ('a' <= key && key <= 'z')
	{
		return 1<<(key-'a');
	}
	return 0;
}

struct graph *graph_build(struct map *m, unsigned startpos)
{
	/* allocate space for the graph */
	struct graph *g = calloc(1, sizeof(*g));

	/* allocate space for distance and parents */
	unsigned *distance = calloc(2 * m->pcount, sizeof(*distance));
	unsigned *parent   = distance + m->pcount;

	/* create the vertices by looking at reachable keys */
	map_bfs(m, startpos, distance, parent);
	for (unsigned pos = 0; pos < m->pcount; pos++)
	{
		/* skip if the key is not reachable */
		if (distance[pos] == UINT_MAX)
		{
			continue;
		}

		char v = m->points[pos];
		if (('a' <= v && v <= 'z') || pos == startpos)
		{
			if (pos == startpos)
			{
				g->start = g->vcount;
			}
			else
			{
				g->goal |= key_to_bit(v);
			}
			g->vertices[g->vcount].key = v;
			g->vertices[g->vcount].pos = pos;
			g->vertices[g->vcount].tail = &g->vertices[g->vcount].edges;
			g->vcount++;
		}
	}

	/* create the edges */
	for (size_t i = 0; i < g->vcount; i++)
	{
		struct vertex *src = g->vertices + i;
		map_bfs(m, src->pos, distance, parent);
		for (size_t j = i+1; j < g->vcount; j++)
		{
			struct vertex *dst = g->vertices + j;

			/* compute the needed keys */
			unsigned needed = 0;
			unsigned pos = dst->pos;
			while (pos != src->pos)
			{
				char v = m->points[pos];
				if ('A' <= v && v <= 'Z')
				{
					needed |= key_to_bit(tolower(v));
				}
				pos = parent[pos];
			}

			/* ignore doors without keys */
			needed &= g->goal;

			/* add the edges */
			graph_add_edge(g, i, j, distance[dst->pos], needed);
			graph_add_edge(g, j, i, distance[dst->pos], needed);
		}
	}
	free(distance);
	return g;
}

struct state
{
	size_t x;		/* vertex index */
	unsigned keys;		/* collected keys */
	unsigned distance;	/* computed distance */

	struct state *next;	/* next state in hashtable */
};

#define TABLE_SIZE 4096

struct distance
{
	struct state *table[TABLE_SIZE];
};

unsigned hashfn(size_t x, unsigned keys)
{
	unsigned hash = 5381;
	while (keys)
	{
		hash = hash * 33 + (keys & 0x3f);
		keys >>= 6;
	}
	return hash * 33 + x;
}

struct state *distance_find(struct distance *d, size_t x, unsigned keys)
{
	unsigned pos = hashfn(x, keys) % TABLE_SIZE;
	struct state *s = d->table[pos];
	while (s && (s->x != x || s->keys != keys))
	{
		s = s->next;
	}
	return s;
}

struct state *distance_add(struct distance *d, size_t x, unsigned keys, unsigned distance)
{
	struct state *s = calloc(1, sizeof(*s));
	if (s)
	{
		s->x = x;
		s->keys = keys;
		s->distance = distance;

		unsigned pos = hashfn(x, keys) % TABLE_SIZE;
		s->next = d->table[pos];
		d->table[pos] = s;
	}
	return s;
}

void distance_destroy(struct distance *d)
{
	for (size_t i = 0; i < TABLE_SIZE; i++)
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

unsigned graph_mindistance(struct graph *g)
{
	unsigned lowest = UINT_MAX;
	struct distance distance = {};
	struct state *cur = distance_add(&distance, g->start, 0, 0);
	struct heap h = {};
	heap_push(&h, cur);
	while (h.count)
	{
		cur = heap_pop(&h);
		for (struct edge *e = g->vertices[cur->x].edges;
		     e;
		     e = e->next)
		{
			/* skip if we found @ */
			if (e->y == g->start)
			{
				continue;
			}

			unsigned newkeys = cur->keys | key_to_bit(g->vertices[e->y].key);

			/* skip if we already collected the key */
			if (newkeys == cur->keys)
			{
				continue;
			}

			/* skip if we require keys not collected yet */
			if (e->needed & ~cur->keys)
			{
				continue;
			}

			unsigned newdist = cur->distance + e->distance;
			struct state *s = distance_find(&distance, e->y, newkeys);
			if (s && s->distance > newdist)
			{
				s->distance = newdist;
				heap_push(&h, s);
				if (newkeys == g->goal && lowest > newdist)
				{
					lowest = newdist;
				}
			}
			else if (!s)
			{
				s = distance_add(&distance, e->y, newkeys, newdist);
				heap_push(&h, s);
				if (newkeys == g->goal && lowest > newdist)
				{
					lowest = newdist;
				}
			}
		}
	}

	heap_destroy(&h);
	distance_destroy(&distance);
	return lowest;
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

	struct map *m = map_load(input);
	fclose(input);
	if (!m)
	{
		fprintf(stderr, "Cannot load the map\n");
		return -1;
	}
	struct graph *g = graph_build(m, m->start);
	printf("part1: %u\n", graph_mindistance(g));
	graph_free(g);

	const int dx[] = {-1, 1, -1, 1};
	const int dy[] = {-1, -1, 1, 1};
	map_patch(m);
	unsigned count = 0;
	for (int i = 0; i < 4; i++)
	{
		g = graph_build(m, m->start + map_index(m, dx[i], dy[i]));
		count += graph_mindistance(g);
		graph_free(g);
	}
	map_free(m);

	printf("part2: %u\n", count);
	return 0;
}
