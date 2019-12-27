#include <assert.h>
#include <inttypes.h>
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
	return calloc(1, sizeof(struct module));
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
	memset(m->ram+psize, 0, (m->size-psize) * sizeof(m->ram[0]));
	m->pc = 0;
	m->rbp = 0;
	m->ri = m->wi = m->ro = m->wo = 0;
}

static void module_push_input(struct module *m, int64_t value)
{
	assert(m->wi - m->ri < 32);
	m->inq[(m->wi++) & 31] = value;
}

static int64_t module_pop_output(struct module *m)
{
	assert(m->wo != m->ro);
	return m->outq[(m->ro++) & 31];
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
			break;

		case OP_OUT:
			a = address_of(m, m->pc+1, a_mode);
			if (m->wo - m->ro < 32)
			{
				m->outq[(m->wo++) & 31] = m->ram[a];
			}
			else
			{
				return OUTPUT_FULL;
			}
			m->pc += 2;
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

#define TABLE_SIZE 1024

enum
{
	WALL = 0,
	FREE = 1,
	OXYGEN = 2,
};

struct point
{
	int x;
	int y;
	int v;
	int d;

	struct point *next;
};

struct map
{
	struct point *table[TABLE_SIZE];
	int xmin, xmax;
	int ymin, ymax;
	int maxd;
	struct point *start;
	struct point *oxygen;
	struct module *m;
};

static void map_destroy(struct map *m)
{
	for (int i = 0; i < TABLE_SIZE; i++)
	{
		struct point *p = m->table[i];
		while (p)
		{
			struct point *t = p;
			p = p->next;
			free(t);
		}
	}
}

static unsigned hashfn(int x, int y)
{
	return 5381 + 33 * x + y;
}

static struct point *map_find(struct map *m, int x, int y)
{
	unsigned pos = hashfn(x, y) & (TABLE_SIZE-1);
	struct point *p = m->table[pos];
	while (p && !(p->x == x && p->y == y))
	{
		p = p->next;
	}
	return p;
}

static struct point *map_add(struct map *m, int x, int y, int v)
{
	unsigned pos = hashfn(x, y) & (TABLE_SIZE-1);
	struct point *p = calloc(1, sizeof(*p));
	if (p)
	{
		if (m->xmin > x) m->xmin = x;
		if (m->xmax < x) m->xmax = x;
		if (m->ymin > y) m->ymin = y;
		if (m->ymax < y) m->ymax = y;
		p->x = x;
		p->y = y;
		p->v = v;
		p->next = m->table[pos];
		m->table[pos] = p;
	}
	return p;
}

static void map_print(struct map *m)
{
	for (int y = m->ymin-1; y <= m->ymax+1; y++)
	{
		for (int x = m->xmin-1; x <= m->xmax+1; x++)
		{
			struct point *p = map_find(m, x, y);
			if (!p || p->v == WALL)
			{
				putchar('#');
			}
			else if (p == m->start)
			{
				putchar('S');
			}
			else if (p->v == FREE)
			{
				putchar('.');
			}
			else if (p->v == OXYGEN)
			{
				putchar('O');
			}
		}
		putchar('\n');
	}
}

static const int dx[] = {0, 0, -1, +1};
static const int dy[] = {-1, +1, 0, 0};

static void map_dfs(struct map *map, int x, int y, int value)
{
	static const int back[] = {2, 1, 4, 3};
	struct point *p = map_add(map, x, y, value);
	if (value == OXYGEN)
	{
		map->oxygen = p;
	}
	for (int i = 0; i < 4; i++)
	{
		module_push_input(map->m, i+1);
		module_execute(map->m);
		int r = module_pop_output(map->m);
		if (r == WALL)
		{
			/* nothing to do */
		}
		else
		{
			if (!map_find(map, x + dx[i], y + dy[i]))
			{
				map_dfs(map, x+dx[i], y + dy[i], r);
			}
			module_push_input(map->m, back[i]);
			module_execute(map->m);
			module_pop_output(map->m);
		}
	}

}

static void map_discover(struct map *map, const int64_t *program, size_t size)
{
	map->m = module_new();
	module_load(map->m, program, size);
	map_dfs(map, 0, 0, FREE);
	map->start = map_find(map, 0, 0);
	module_free(map->m);
	map->m = NULL;
}

static void map_bfs(struct map *map, struct point *start)
{
	struct point *fifo[64];
	size_t fr = 0, fw = 0;
	map->maxd = start->d = 0;
	fifo[(fw++) & 63] = start;

	while (fr != fw)
	{
		struct point *p = fifo[(fr++) & 63];
		for (int i = 0; i < 4; i++)
		{
			struct point *np = map_find(map, p->x + dx[i], p->y + dy[i]);
			if (np && np != start && np->d == 0)
			{
				assert(fw-fr<64);
				fifo[(fw++) & 63] = np;
				np->d = p->d + 1;
				if (map->maxd < np->d)
				{
					map->maxd = np->d;
				}
			}
		}
	}
}

static int map_oxyfill(struct map *m, struct point *start)
{
	for (int i = 0; i < TABLE_SIZE; i++)
	{
		for (struct point *p = m->table[i];
		     p;
		     p = p->next)
		{
			p->d = 0;
		}
	}
	map_bfs(m, start);
	return m->maxd;
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
				abort();
			}
			asize = newsize;
			array = newarray;
		}
		array[acount++] = value;
	}
	fclose(input);

	struct map m = {};
	map_discover(&m, array, acount);
	free(array);
	map_print(&m);
	map_bfs(&m, m.start);
	printf("part1: %d\n", m.oxygen->d);
	printf("part2: %d\n", map_oxyfill(&m, m.oxygen));
	map_destroy(&m);
	return 0;
}
