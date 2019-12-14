#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct chemical
{
	char name[10];
	size_t quantity;
	size_t available;
	struct chemical *next;
};

struct reaction
{
	struct chemical *output;
	struct chemical *input;
	struct reaction *next;
};

#define TABLE_SIZE 1024

struct reactions
{
	struct reaction *table[TABLE_SIZE];
};

static unsigned hashfn(const char *key)
{
	unsigned hash = 5381;
	for (; *key; key++)
	{
		hash += hash * 33 + *key;
	}
	return hash;
}

static void reactions_add(struct reactions *rs, struct chemical *in, struct chemical *out)
{
	struct reaction *r = calloc(1, sizeof(*r));
	if (r)
	{
		r->input = in;
		r->output = out;

		unsigned pos = hashfn(r->output->name) & (TABLE_SIZE-1);
		r->next = rs->table[pos];
		rs->table[pos] = r;
	}
}

static void reactions_reset(struct reactions *rs)
{
	for (int i = 0; i < TABLE_SIZE; i++)
	{
		for (struct reaction *r = rs->table[i];
		     r;
		     r = r->next)
		{
			r->output->available = 0;
		}
	}
}

static void reactions_destroy(struct reactions *rs)
{
	for (size_t i = 0; i < TABLE_SIZE; i++)
	{
		struct reaction *r = rs->table[i];
		while (r)
		{
			struct reaction *t = r;
			r = r->next;
			struct chemical *in = t->input;
			while (in)
			{
				struct chemical *ti = in;
				in = in->next;
				free(ti);
			}
			free(t->output);
			free(t);
		}
	}
}

static struct reaction *reactions_find(struct reactions *rs, const char *name)
{
	unsigned pos = hashfn(name) & (TABLE_SIZE -1);
	struct reaction *r = rs->table[pos];
	while (r && strcmp(r->output->name, name))
	{
		r = r->next;
	}
	return r;
}

static struct chemical *parse_chemical(char *str)
{
	struct chemical *c = calloc(1, sizeof(*c));
	if (c)
	{
		for (; !isdigit(*str); str++);
		c->quantity = strtoul(str, NULL, 0);
		for (; !isspace(*str); str++);
		for (; isspace(*str); str++);
		size_t i;
		for (i = 0; i < sizeof(c->name)-1 && *str != 0 && !isspace(*str); i++)
		{
			c->name[i] = *str++;
		}
		c->name[i] = 0;
	}
	return c;
}

static void parse_reactions(FILE *input, struct reactions *rs)
{
	char *buf = NULL;
	size_t sbuf = 0;
	while (getline(&buf, &sbuf, input) != -1)
	{
		char *line = buf;
		char *chems = strsep(&line, "=>\n");
		struct chemical *in = NULL;
		for (char *chem = strsep(&chems, ",");
		     chem;
		     chem = strsep(&chems, ","))
		{
			if (chem[0])
			{
				struct chemical *c = parse_chemical(chem);
				c->next = in;
				in = c;
			}
		}

		while ((chems = strsep(&line, "=>\n")) && chems[0] == 0);
		if (chems)
		{
			struct chemical *out = parse_chemical(chems);
			reactions_add(rs, in, out);
		}
	}
	free(buf);
}

static size_t find_ore(struct reactions *rs, int64_t fuel)
{
	reactions_reset(rs);
	struct chemical *need = NULL;

	size_t ore = 0;
	need = calloc(1, sizeof(*need));
	strcpy(need->name, "FUEL");
	need->quantity = fuel;
	while (need)
	{
		struct chemical *req = need;
		need = need->next;

		if (strcmp(req->name, "ORE") == 0)
		{
			ore += req->quantity;
		}
		else
		{
			struct reaction *r = reactions_find(rs, req->name);
			assert(r);
			size_t quantity = req->quantity - r->output->available;
			size_t m = (quantity + r->output->quantity - 1) / r->output->quantity;
			for (struct chemical *in = r->input;
			     in;
			     in = in->next)
			{
				struct chemical *c = calloc(1, sizeof(*c));
				c->quantity = in->quantity * m;
				strcpy(c->name, in->name);
				c->next = need;
				need = c;
			}
			r->output->available = m * r->output->quantity - quantity;
		}
		free(req);
	}
	return ore;
}

static size_t bisect(struct reactions *rs, size_t ore)
{
	size_t maxfuel = ore;
	size_t minfuel = 1;
	while (minfuel < maxfuel)
	{
		size_t midfuel = minfuel + (maxfuel - minfuel) / 2;
		size_t midore = find_ore(rs, midfuel);
		if (midore < ore)
		{
			minfuel = midfuel + 1;
		}
		else
		{
			maxfuel = midfuel;
		}
	}
	return minfuel -1;
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

	struct reactions rs = {};
	parse_reactions(input, &rs);
	printf("part1: %zu\n", find_ore(&rs, 1));
	printf("part2: %zu\n", bisect(&rs, 1000000000000));
	reactions_destroy(&rs);
	fclose(input);
	return 0;
}
