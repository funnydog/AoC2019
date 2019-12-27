#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 1024

struct node
{
	char name[10];
	struct node *parent;
	struct node *next;
};

struct elem
{
	char name[10];
	struct elem *next;
};

struct node *table[1024];

static unsigned hashfn(const char *name)
{
	unsigned hash;
	for (hash = 5381; *name; name++)
	{
		hash = hash * 33 + *name;
	}
	return hash;
}

static struct node *find(const char *name)
{
	unsigned pos = hashfn(name) % TABLE_SIZE;
	struct node *n = table[pos];
	while (n && strcmp(n->name, name) != 0)
	{
		n = n->next;
	}
	if (!n)
	{
		n = calloc(1, sizeof(*n));
		strcpy(n->name, name);
		n->next = table[pos];
		table[pos] = n;
	}
	return n;
}

static void dep_add(const char *name, const char *parent)
{
	struct node *n = find(name);
	n->parent = find(parent);
}

static void clear(void)
{
	for (int i = 0; i < TABLE_SIZE; i++)
	{
		struct node *n = table[i];
		while (n)
		{
			struct node *t = n;
			n = n->next;
			free(t);
		}
		table[i] = NULL;
	}
}

static int count_orbits(void)
{
	int count = 0;
	for (int i = 0; i < TABLE_SIZE; i++)
	{
		for (struct node *n = table[i]; n; n = n->next)
		{
			for (struct node *t = n->parent; t; t = t->parent)
			{
				count++;
			}
		}
	}
	return count;
}

static struct elem *path_to_origin(const char *start)
{
	struct elem *e = NULL;
	struct node *n = find(start);
	for (n = n->parent; n; n = n->parent)
	{
		struct elem *t = calloc(1, sizeof(*t));
		strcpy(t->name, n->name);
		t->next = e;
		e = t;
	}
	return e;
}

static int pathlen(const char *elem1, const char *elem2)
{
	struct elem *e1 = path_to_origin(elem1);
	struct elem *e2 = path_to_origin(elem2);
	while (e1 && e2 && strcmp(e1->name, e2->name) == 0)
	{
		struct elem *t = e1;
		e1 = e1->next;
		free(t);

		t = e2;
		e2 = e2->next;
		free(t);
	}

	int count = 0;
	while (e1)
	{
		struct elem *t = e1;
		e1 = e1->next;
		free(t);
		count++;
	}
	while (e2)
	{
		struct elem *t = e2;
		e2 = e2->next;
		free(t);
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
		fprintf(stderr, "Cannot open %s for reading\n", argv[1]);
		return EXIT_FAILURE;
	}

	char *buf = NULL;
	size_t size = 0;
	while (getline(&buf, &size, input) != -1)
	{
		char *parent = strtok(buf, ")");
		if (parent)
		{
			char *child = strtok(NULL, "\n");
			if (child)
			{
				dep_add(child, parent);
			}
		}
	}
	free(buf);
	fclose(input);

	printf("part1: %d\n", count_orbits());
	printf("part2: %d\n", pathlen("YOU", "SAN"));
	clear();
	return 0;
}
