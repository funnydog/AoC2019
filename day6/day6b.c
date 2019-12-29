#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 1024

struct edge
{
	struct vertex *target;
	struct edge *next;
};

struct vertex
{
	char name[10];
	int degree;
	int processed;
	int discovered;
	struct vertex *parent;

	struct edge *edges;

	/* for the hashtable */
	struct vertex *next;
};

struct graph
{
	struct vertex *vertices[1024];
};

/* djb hash function */
static unsigned hashfn(const char *key)
{
	unsigned hash;
	for (hash = 5381; *key; key++)
	{
		hash = hash * 33 + *key;
	}
	return hash;
}

static struct vertex *graph_find(struct graph *g, const char *name)
{
	unsigned pos = hashfn(name) % TABLE_SIZE;
	struct vertex *v = g->vertices[pos];
	while (v != NULL && strcmp(v->name, name) != 0)
	{
		v = v->next;
	}
	if (v == NULL)
	{
		v = calloc(1, sizeof(*v));
		strncpy(v->name, name, sizeof(v->name));
		v->next = g->vertices[pos];
		g->vertices[pos] = v;
	}
	return v;
}

static void graph_add_edge(struct graph *g, const char *a, const char *b)
{
	struct vertex *va = graph_find(g, a);
	struct vertex *vb = graph_find(g, b);
	struct edge *e = calloc(1, sizeof(*e));
	e->target = vb;
	e->next = va->edges;
	va->edges = e;
}

static void graph_read(struct graph *g, FILE *input)
{
	char *buffer = NULL;
	size_t bsize = 0;
	while (getline(&buffer, &bsize, input) != -1)
	{
		char *a = strtok(buffer, ")");
		char *b = strtok(NULL, "\n");
		if (a && b)
		{
			graph_add_edge(g, a, b);
			graph_add_edge(g, b, a);
		}
	}
	free(buffer);
}

static void graph_destroy(struct graph *g)
{
	for (int i = 0; i < TABLE_SIZE; i++)
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

static void graph_print(struct graph *g)
{
	for (int i = 0; i < TABLE_SIZE; i++)
	{
		for (struct vertex *v = g->vertices[i];
		     v;
		     v = v->next)
		{
			printf("%s: ", v->name);

			for (struct edge *e = v->edges;
			     e;
			     e = e->next)
			{
				printf("%s ", e->target->name);
			}
			printf("\n");
		}
	}
}

static void graph_bfs(struct graph *g, const char *start)
{
	for (int i = 0; i < TABLE_SIZE; i++)
	{
		for (struct vertex *v = g->vertices[i];
		     v;
		     v = v->next)
		{
			v->processed = v->discovered = 0;
			v->parent = NULL;
		}
	}

	struct vertex *queue[32];
	unsigned qr = 0, qw = 0;

	struct vertex *v = graph_find(g, start);
	queue[(qw++)&31] = v;
	v->discovered = 1;
	while (qr != qw)
	{
		v = queue[(qr++)&31];
		/* process early */
		v->processed = 1;
		for (struct edge *e = v->edges; e; e = e->next)
		{
			struct vertex *t = e->target;
			if (!t->processed)
			{
				/* process edge */
			}
			if (!t->discovered)
			{
				t->discovered = 1;
				if (qr+32 == qw)
				{
					fprintf(stderr, "queue full\n");
					abort();
				}
				queue[(qw++)&31] = t;
				t->parent = v;
			}
		}
		/* process late */
	}
}

static int graph_count_paths(struct graph *g)
{
	int count = 0;
	for (int i = 0; i < TABLE_SIZE; i++)
	{
		for (struct vertex *v = g->vertices[i];
		     v;
		     v = v->next)
		{
			for (struct vertex *p = v->parent;
			     p;
			     p = p->parent)
			{
				count++;
			}
		}
	}
	return count;
}

static struct vertex *graph_find_parent(struct graph *g, const char *src)
{
	struct vertex *v = graph_find(g, src);
	if (v)
	{
		v = v->parent;
	}
	return v;
}

static int graph_path_len(struct graph *g, const char *src)
{
	int count = 0;
	for (struct vertex *v = graph_find(g, src)->parent;
	     v;
	     v = v->parent)
	{
		count++;
	}
	return count;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <input>\n", argv[0]);
		return EXIT_FAILURE;
	}

	FILE *input = fopen(argv[1], "rb");
	if (!input)
	{
		fprintf(stderr, "Cannot open file %s\n", argv[1]);
		return EXIT_FAILURE;
	}

	struct graph g = {};
	graph_read(&g, input);
	fclose(input);

	(void)graph_print;
	graph_bfs(&g, "COM");
	printf("part1: %d\n", graph_count_paths(&g));
	graph_bfs(&g, graph_find_parent(&g, "SAN")->name);
	printf("part2: %d\n", graph_path_len(&g, graph_find_parent(&g, "YOU")->name));
	graph_destroy(&g);
	return 0;
}
