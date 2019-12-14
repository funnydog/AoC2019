#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 1024
#define NAME_LEN 10

struct edge
{
	size_t quantity;
	struct vertex *target;

	struct edge *next;
};

struct vertex
{
	char name[NAME_LEN];
	size_t quantity;
	size_t computed;
	int discovered;

	struct edge *edges;

	struct vertex *next;	/* for the hashtable */
};

struct graph
{
	struct vertex *vertices[TABLE_SIZE];
	size_t vcount;

	struct vertex **stack;
	size_t scount;
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

static struct vertex *graph_find(struct graph *g, const char *name)
{
	unsigned pos = hashfn(name) & (TABLE_SIZE-1);
	struct vertex *v = g->vertices[pos];
	while (v && strcmp(v->name, name))
	{
		v = v->next;
	}
	if (!v)
	{
		v = calloc(1, sizeof(*v));
		assert(v);
		strcpy(v->name, name);
		v->next = g->vertices[pos];
		g->vertices[pos] = v;
		g->vcount++;
	}
	return v;
}

static void parse_chemical(const char *str, char *out, size_t outlen, size_t *q)
{
	for (; !isdigit(*str); str++);
	*q = strtoul(str, NULL, 0);
	for (; *str && !isspace(*str); str++);
	for (; isspace(*str); str++);
	size_t i;
	outlen--;
	for (i = 0; i < outlen && *str && !isspace(*str); i++)
	{
		*out++ = *str++;
	}
	*out = 0;
}

static void graph_read(struct graph *g, FILE *input)
{
	char *buf  = NULL;
	size_t sbuf = 0;
	while (getline(&buf, &sbuf, input) != -1)
	{
		char name[NAME_LEN];
		size_t quantity;
		char *line = buf;
		char *chems = strsep(&line, "=>\n");
		struct edge *edges = NULL;
		for (char *chem = strsep(&chems, ",");
		     chem;
		     chem = strsep(&chems, ","))
		{
			if (chem[0])
			{
				parse_chemical(chem, name, sizeof(name), &quantity);
				struct edge *e = calloc(1, sizeof(*e));
				assert(e);
				e->target = graph_find(g, name);
				e->quantity = quantity;
				e->next = edges;
				edges = e;
			}
		}

		while ((chems = strsep(&line, "=>\n")) && chems[0] == 0);
		if (chems)
		{
			size_t q;
			parse_chemical(chems, name, sizeof(name), &q);
			struct vertex *v = graph_find(g, name);
			v->quantity = q;
			v->edges = edges;
		}
	}
	free(buf);
}

static void graph_dfs(struct graph *g, struct vertex *v)
{
	v->discovered = 1;
	for (struct edge *e = v->edges;
	     e;
	     e = e->next)
	{
		struct vertex *y = e->target;
		if (!y->discovered)
		{
			graph_dfs(g, y);
		}
	}
	g->stack[g->scount++] = v;
}

static void graph_sort(struct graph *g)
{
	if (!g->stack)
	{
		g->stack = calloc(g->vcount, sizeof(*g->stack));
		assert(g->stack);
	}
	g->scount = 0;
	graph_dfs(g, graph_find(g, "FUEL"));
}

static size_t graph_ore(struct graph *g, size_t fuel)
{
	g->stack[g->scount-1]->computed = fuel;
	for (int i = g->scount-2; i >= 0; i--)
	{
		g->stack[i]->computed = 0;
	}
	for (size_t i = g->scount-1; i > 0; i--)
	{
		struct vertex *v = g->stack[i];
		size_t m = (v->computed  + v->quantity - 1) / v->quantity;
		for (struct edge *e = v->edges;
		     e;
		     e = e->next)
		{
			e->target->computed += m * e->quantity;
		}
	}
	return g->stack[0]->computed;
}

static size_t graph_minfuel(struct graph *g, size_t ore)
{
	size_t minfuel = ore / graph_ore(g, 1);
	size_t maxfuel = 2 * minfuel;
	while (minfuel < maxfuel)
	{
		size_t midfuel = minfuel + (maxfuel-minfuel)/2;
		if (graph_ore(g, midfuel) < ore)
		{
			minfuel = midfuel + 1;
		}
		else
		{
			maxfuel = midfuel;
		}
	}
	return minfuel-1;
}

static void graph_destroy(struct graph *g)
{
	free(g->stack);
	for (size_t i = 0; i < TABLE_SIZE; i++)
	{
		struct vertex *v = g->vertices[i];
		while (v)
		{
			struct vertex *tv = v;
			v = v->next;
			struct edge *e = tv->edges;
			while (e)
			{
				struct edge *te = e;
				e = e->next;
				free(te);
			}
			free(tv);
		}
	}
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

	struct graph g = {};
	graph_read(&g, input);
	fclose(input);

	graph_sort(&g);
	printf("part1: %zu\n", graph_ore(&g, 1));
	printf("part2: %zu\n", graph_minfuel(&g, 1000000000000));
	graph_destroy(&g);
	return 0;
}
