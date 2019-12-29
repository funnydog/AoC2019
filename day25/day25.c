#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
	/* opcodes */
	OP_ADD	= 1,		/* [c] = [a] + [b]        */
	OP_MUL	= 2,		/* [c] = [a] * [b]        */
	OP_IN	= 3,		/* [a] = <input>          */
	OP_OUT	= 4,		/* output([a])            */
	OP_JNZ	= 5,		/* if ([a] != 0) goto [b] */
	OP_JZ	= 6,		/* if ([a] == 0) goto [b] */
	OP_TLT	= 7,		/* [c] = [a] < [b]        */
	OP_TEQ	= 8,		/* [c] = [a] == [b]       */
	OP_ARB  = 9,		/* rbp += [a]             */
	OP_HALT = 99,

	/* addressing modes */
	PMODE = 0,		/* absolute index */
	IMODE = 1,		/* immediate literal */
	RMODE = 2,		/* relative index (to rbp) */

	/* state of the execution */
	INPUT_EMPTY = 0,
	OUTPUT_FULL = 1,
	HALTED = 2,
};

struct module
{
	int64_t *ram;
	size_t size;
	int64_t pc;		/* instruction/program counter */
	int64_t rbp;		/* relative base pointer */

	int64_t inq[32];
	size_t ri, wi;

	int64_t outq[32];
	size_t ro, wo;

	FILE *output;
};

static void module_free(struct module *m)
{
	if (m)
	{
		free(m->ram);
		free(m);
	}
}

static struct module *module_new(void)
{
	return 	calloc(1, sizeof(struct module));
}

static int64_t address_of(struct module *m, int64_t pos, int mode)
{
	while ((size_t)pos >= m->size)
	{
		size_t nsize = m->size ? m->size * 2 : 1024;
		int64_t *nram = realloc(m->ram, nsize * sizeof(*nram));
		assert(nram);

		/* NOTE: realloc() doesn't guarantee that the added
		 * memory is zeroed. */
		memset(nram+m->size, 0, nsize - m->size);
		m->size = nsize;
		m->ram = nram;
	}

	switch(mode)
	{
	case IMODE: return pos;
	case PMODE: return address_of(m, m->ram[pos], IMODE);
	case RMODE: return address_of(m, m->rbp + m->ram[pos], IMODE);
	default:
		fprintf(stderr, "Unknown addressing mode: %d\n", mode);
		abort();
	}
}

static void module_load(struct module *m, const int64_t *prog, size_t psize)
{
	/* NOTE: forces a reallocation */
	address_of(m, psize, IMODE);

	/* reset the memory and copy the program */
	memcpy(m->ram, prog, psize * sizeof(m->ram[0]));
	memset(m->ram+psize, 0, (m->size - psize) * sizeof(m->ram[0]));

	m->pc = 0;
	m->rbp = 0;
	m->ri = m->wi = m->ro = m->wo = 0;
}

static void module_push_input(struct module *m, int64_t value)
{
	assert(m->wi - m->ri < 32);
	m->inq[(m->wi++) & 31] = value;
}

static int module_input_full(struct module *m)
{
	return m->ri + 32 == m->wi;
}

static int64_t module_pop_output(struct module *m)
{
	assert(m->wo != m->ro);
	return m->outq[(m->ro++) & 31];
}

static int module_output_empty(struct module *m)
{
	return m->ro == m->wo;
}

static void module_log(struct module *m, FILE *out)
{
	m->output = out;
}

static int module_execute(struct module *m)
{
	for (;;)
	{
		int op;
		div_t d = div(m->ram[m->pc], 100);
		op = d.rem;
		d = div(d.quot, 10);
		int a_mode = d.rem;
		d = div(d.quot, 10);
		int b_mode = d.rem;
		d = div(d.quot, 10);
		int c_mode = d.rem;

		int64_t a, b, c;
		switch (op)
		{
		case OP_ADD:
			a = address_of(m, m->pc+1, a_mode);
			b = address_of(m, m->pc+2, b_mode);
			c = address_of(m, m->pc+3, c_mode);
			assert(c_mode != IMODE);
			m->ram[c] = m->ram[a] + m->ram[b];
			m->pc += 4;
			break;

		case OP_MUL:
			a = address_of(m, m->pc+1, a_mode);
			b = address_of(m, m->pc+2, b_mode);
			c = address_of(m, m->pc+3, c_mode);
			assert(c_mode != IMODE);
			m->ram[c] = m->ram[a] * m->ram[b];
			m->pc += 4;
			break;

		case OP_IN:
			a = address_of(m, m->pc+1, a_mode);
			assert(a_mode != IMODE);
			if (m->wi == m->ri)
			{
				return INPUT_EMPTY;
			}
			m->ram[a] = m->inq[(m->ri++) & 31];
			m->pc += 2;
			if (m->output && 0 <= m->ram[a] && m->ram[a] < 256)
			{
				putc(m->ram[a], m->output);
			}
			break;

		case OP_OUT:
			a = address_of(m, m->pc+1, a_mode);
			if (m->wo - m->ro == 32)
			{
				return OUTPUT_FULL;
			}
			m->outq[(m->wo++) & 31] = m->ram[a];
			m->pc += 2;
			if (m->output && 0 <= m->ram[a] && m->ram[a] < 256)
			{
				putc(m->ram[a], m->output);
			}
			break;

		case OP_JNZ:
			a = address_of(m, m->pc+1, a_mode);
			b = address_of(m, m->pc+2, b_mode);
			if (m->ram[a] != 0)
			{
				m->pc = m->ram[b];
			}
			else
			{
				m->pc += 3;
			}
			break;

		case OP_JZ:
			a = address_of(m, m->pc+1, a_mode);
			b = address_of(m, m->pc+2, b_mode);
			if (m->ram[a] == 0)
			{
				m->pc = m->ram[b];
			}
			else
			{
				m->pc += 3;
			}
			break;

		case OP_TLT:
			a = address_of(m, m->pc+1, a_mode);
			b = address_of(m, m->pc+2, b_mode);
			c = address_of(m, m->pc+3, c_mode);
			assert(c_mode != IMODE);
			m->ram[c] = m->ram[a] < m->ram[b] ? 1 : 0;
			m->pc += 4;
			break;

		case OP_TEQ:
			a = address_of(m, m->pc+1, a_mode);
			b = address_of(m, m->pc+2, b_mode);
			c = address_of(m, m->pc+3, c_mode);
			assert(c_mode != IMODE);
			m->ram[c] = m->ram[a] == m->ram[b] ? 1 : 0;
			m->pc += 4;
			break;

		case OP_ARB:
			a = address_of(m, m->pc+1, a_mode);
			m->rbp += m->ram[a];
			m->pc += 2;
			break;

		case OP_HALT:
			return HALTED;

		default:
			fprintf(stderr, "Unknown opcode %" SCNd64 " at %" SCNd64 "\n",
				m->ram[m->pc], m->pc);
			abort();
		}
	}
}

static int64_t *program_load(FILE *input, size_t *count)
{
	int64_t *array = NULL;
	size_t acount = 0;
	size_t asize = 0;

	int64_t value;
	while (fscanf(input, "%" SCNd64 ",", &value) == 1)
	{
		if (acount == asize)
		{
			size_t newsize = asize ? asize * 2 : 32;
			int64_t *newarray = realloc(array, newsize * sizeof(*newarray));
			if (!newarray)
			{
				free(array);
				return NULL;
			}
			asize = newsize;
			array = newarray;
		}
		array[acount++] = value;
	}
	*count = acount;
	return array;
}

#define TABLE_SIZE 64
#define ITEMS_SIZE 16

enum
{
	DOOR_NORTH,
	DOOR_EAST,
	DOOR_SOUTH,
	DOOR_WEST,
};

static const char *DOOR_NAME[] = {
	"north",
	"east",
	"south",
	"west",
};

static int DOOR_BACK[] = {
	DOOR_SOUTH,
	DOOR_WEST,
	DOOR_NORTH,
	DOOR_EAST,
};

static const char *dangerous_items[] = {
	"photons",
	"escape pod",
	"giant electromagnet",
	"infinite loop",
	"molten lava",
	NULL,
};

struct location
{
	char name[80];
	int parent;
	unsigned exits;
	struct location *doors[4];

	struct location *next;
};

static void location_connect(struct location *src, struct location *dst, int door)
{
	src->doors[door] = dst;
	dst->doors[DOOR_BACK[door]] = src;
}

struct map
{
	struct module *mod;
	struct location *table[TABLE_SIZE];
	size_t tcount;

	struct location *target;
	int missing_door;

	char items[ITEMS_SIZE][80];
	size_t icount;
};

unsigned hashfn(const char *name)
{
	unsigned hash;
	for (hash = 5183; *name; name++)
	{
		hash = hash * 33 + *name;
	}
	return hash;
}

static void map_free(struct map *m)
{
	if (m)
	{
		module_free(m->mod);
		for (size_t i = 0; i < TABLE_SIZE; i++)
		{
			struct location *l = m->table[i];
			while (l)
			{
				struct location *t = l;
				l = l->next;
				free(t);
			}
		}
		free(m);
	}
}

static struct map *map_new(int64_t *program, size_t pcount)
{
	struct map *map = calloc(1, sizeof(*map));
	if (map)
	{
		map->mod = module_new();
		assert(map->mod);
		module_log(map->mod, stdout);
		module_load(map->mod, program, pcount);
	}
	return map;
}

static struct location *map_find_location(struct map *m, const char *name)
{
	unsigned pos = hashfn(name) % TABLE_SIZE;
	struct location *l = m->table[pos];
	while (l && strcmp(l->name, name))
	{
		l = l->next;
	}
	return l;
}

static struct location *map_add_location(struct map *m, const char *name, unsigned exits, int parent)
{
	struct location *l = calloc(1, sizeof(*l));
	if (l)
	{
		strlcpy(l->name, name, sizeof(l->name));
		l->exits = exits;
		l->parent = parent;

		unsigned pos = hashfn(name) % TABLE_SIZE;
		l->next = m->table[pos];
		m->table[pos] = l;
		m->tcount++;
	}
	return l;
}

static int map_readline(struct map *m, char *buf, size_t buflen)
{
	buflen--;
	size_t len = 0;
	int status = OUTPUT_FULL;
	for (;;)
	{
		int c;
		if (module_output_empty(m->mod))
		{
			if (status == HALTED)
			{
				break;
			}
			status = module_execute(m->mod);
		}
		else if ((c = module_pop_output(m->mod)) == '\n')
		{
			break;
		}
		else if (len < buflen)
		{
			*buf++ = c;
		}
		else
		{
			break;
		}
	}
	*buf = 0;
	return status == HALTED ? -1 : 0;
}

static void map_printf(struct map *m, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	char line[256];
	vsnprintf(line, sizeof(line), fmt, ap);

	for (char *s = line; *s; s++)
	{
		if (module_input_full(m->mod) && module_execute(m->mod) != INPUT_EMPTY)
		{
			break;
		}
		else
		{
			module_push_input(m->mod, *s);
		}
	}

	va_end(ap);
}

static void map_wait_prompt(struct map *m)
{
	char line[256];
	do
	{
		map_readline(m, line, sizeof(line));
	} while (strcmp(line, "Command?"));
}

static void map_pick_up(struct map *m, const char *item_name)
{
	for (const char **item = dangerous_items;
	     *item;
	     item++)
	{
		if (strcmp(*item, item_name) == 0)
		{
			return;
		}
	}
	if (m->icount < ITEMS_SIZE)
	{
		map_printf(m, "take %s\n", item_name);
		map_wait_prompt(m);

		strlcpy(m->items[m->icount], item_name, sizeof(m->items[0]));
		m->icount++;
	}
}

static int map_parse_location(struct map *m, struct location *l, int pickup)
{
	char line[256];
	memset(l, 0, sizeof(*l));
	enum
	{
		READ_NAME,
		READ_DOORS,
		READ_ITEMS
	} state = READ_NAME;
	do
	{
		if (map_readline(m, line, sizeof(line)) < 0)
		{
			return -1;
		}
		else if (state == READ_NAME)
		{
			if (line[0] == '=')
			{
				strlcpy(l->name, line, sizeof(l->name));
			}
			else if (strcmp(line, "Doors here lead:") == 0)
			{
				state = READ_DOORS;
			}
			else if (strcmp(line, "Items here:") == 0)
			{
				state = READ_ITEMS;
			}
		}
		else if (state == READ_DOORS)
		{
			if (line[0] != '-')
			{
				state = READ_NAME;
			}
			else
			{
				for (int i = 0; i < 4; i++)
				{
					if (strcmp(line+2, DOOR_NAME[i]) == 0)
					{
						l->exits |= 1<<i;
					}
				}
			}
		}
		else if (state == READ_ITEMS)
		{
			if (line[0] != '-')
			{
				state = READ_NAME;
			}
			else if (pickup)
			{
				map_pick_up(m, line+2);
			}
		}
	} while (strcmp(line, "Command?"));
	return 0;
}

static void map_dfs(struct map *m, struct location *loc)
{
	for (int i = 0; i < 4; i++)
	{
		if ((loc->exits & 1<<i) && loc->doors[i] == NULL)
		{
			struct location tmp;
			map_printf(m, "%s\n", DOOR_NAME[i]);
			if (map_parse_location(m, &tmp, 1) < 0)
			{
				/* the program halted... */
				return;
			}

			/*
			 * if we are stuck ignore the door and
			 * continue
			 */
			if (strcmp(tmp.name, loc->name) == 0)
			{
				m->target = loc;
				m->missing_door = i;
				continue;
			}

			/*
			 * if we already visited the location go back
			 * and try another door
			 */
			if (map_find_location(m, tmp.name))
			{
				map_printf(m, "%s\n", DOOR_NAME[DOOR_BACK[i]]);
				map_wait_prompt(m);
				continue;
			}

			/*
			 * add the location to the map, parent is the
			 * door that led to the current location
			 */
			struct location *newloc = map_add_location(m, tmp.name, tmp.exits, i);

			/* connect the two locations */
			location_connect(loc, newloc, i);

			/* explore the new location */
			map_dfs(m, newloc);

			/* go back */
			map_printf(m, "%s\n", DOOR_NAME[DOOR_BACK[i]]);
			map_wait_prompt(m);
		}
	}
}

static void map_discover(struct map *m, int64_t *program, size_t pcount)
{
	module_load(m->mod, program, pcount);
	struct location tmp;
	map_parse_location(m, &tmp, 1);

	struct location *loc = map_add_location(m, tmp.name, tmp.exits, -1);
	map_dfs(m, loc);
}

static void map_go_to(struct map *m, struct location *l)
{
	if (l->parent != -1)
	{
		map_go_to(m, l->doors[DOOR_BACK[l->parent]]);
		map_printf(m, "%s\n", DOOR_NAME[l->parent]);
		map_wait_prompt(m);
	}
}

static void map_solve_quest(struct map *m)
{
	map_go_to(m, m->target);

	/* lazy approach, try every combination */
	unsigned collected = (1<<m->icount) - 1;
	for (unsigned i = 0; i < 1<<m->icount; i++)
	{
		unsigned gray = i ^ (i >> 1);
		for (size_t  j = 0; j < m->icount; j++)
		{
			if ((gray & 1<<j) && !(collected & 1<<j))
			{
				map_printf(m, "take %s\n", m->items[j]);
				map_wait_prompt(m);
			}
			else if (!(gray & 1<<j) && (collected & 1<<j))
			{
				map_printf(m, "drop %s\n", m->items[j]);
				map_wait_prompt(m);
			}
		}
		collected = gray;

		/* now try to go in the restricted area */
		map_printf(m, "%s\n", DOOR_NAME[m->missing_door]);
		struct location tmp;
		map_parse_location(m, &tmp, 0);
		if (strcmp(tmp.name, m->target->name))
		{
			/* complete the map */
			struct location *l = map_add_location(m, tmp.name, tmp.exits, m->missing_door);
			location_connect(m->target, l, m->missing_door);
			break;
		}
	}
}

static void map_make_dot(struct map *m, FILE *output)
{
	fprintf(output, "digraph G {\n");
	for (size_t i = 0; i < TABLE_SIZE; i++)
	{
		for (struct location *l = m->table[i];
		     l;
		     l = l->next)
		{
			for (int j = 0; j < 4; j++)
			{
				if (!l->doors[j])
				{
					continue;
				}
				fprintf(output, "\t\"%s\" -> \"%s\" [label=\"%s\"];\n",
					l->name,
					l->doors[j]->name,
					DOOR_NAME[j]);
			}
		}
	}
	fprintf(output, "}\n");
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <input> <dotfilename>\n", argv[0]);
		return -1;
	}

	FILE *input = fopen(argv[1], "rb");
	if (!input)
	{
		fprintf(stderr, "File %s not found\n", argv[1]);
		return -1;
	}

	size_t pcount = 0;
	int64_t *program = program_load(input, &pcount);
	fclose(input);
	if (!program)
	{
		fprintf(stderr, "Cannot load the intcode program\n");
		return -1;
	}
	struct map *m = map_new(program, pcount);
	free(program);
	if (m)
	{
		map_discover(m, program, pcount);
		map_solve_quest(m);
		if (argc > 2)
		{
			FILE *output = fopen(argv[2], "wb");
			if (!output)
			{
				fprintf(stderr, "Cannot open %s for writing\n", argv[2]);
			}
			else
			{
				map_make_dot(m, output);
				fclose(output);
			}
		}
		map_free(m);
	}
	return 0;
}
