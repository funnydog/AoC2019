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
	unsigned ri, wi;

	int64_t outq[32];
	unsigned ro, wo;
};

static void module_free(struct module *m)
{
	if (m)
	{
		free(m->ram);
		free(m);
	}
}

static struct module *module_new(size_t msize)
{
	struct module *m = calloc(1, sizeof(*m));
	if (m)
	{
		m->ram = calloc(msize, sizeof(m->ram[0]));
		m->size = msize;
	}
	return m;
}

static void module_load(struct module *m, const int64_t *prog, size_t psize)
{
	assert(psize < m->size);
	memset(m->ram, 0, m->size * sizeof(m->ram[0]));
	memcpy(m->ram, prog, psize * sizeof(m->ram[0]));
	m->pc = m->rbp = m->ri = m->wi = m->ro = m->wo = 0;
}

static void module_push_input(struct module *m, int64_t value)
{
	assert(m->ri+32 != m->wi);
	m->inq[(m->wi++)&31] = value;
}

static int module_input_full(struct module *m)
{
	return m->ri + 32 == m->wi;
}

static int64_t module_pop_output(struct module *m)
{
	assert(m->ro != m->wo);
	return m->outq[(m->ro++)&31];
}

static int module_output_empty(struct module *m)
{
	return m->ro == m->wo;
}

static int64_t address_of(struct module *m, int64_t pos, int mode)
{
	assert(pos >= 0 && (size_t)pos < m->size);
	switch(mode)
	{
	case IMODE: return pos;
	case PMODE: return m->ram[pos];
	case RMODE: return m->rbp + m->ram[pos];
	default:
		fprintf(stderr, "Unknown addressing mode: %d\n", mode);
		abort();
	}
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
			if (m->ri == m->wi)
			{
				return INPUT_EMPTY;
			}
			a = address_of(m, m->pc+1, a_mode);
			assert(a_mode != IMODE);
			m->ram[a] = m->inq[(m->ri++)&31];
			m->pc += 2;
			break;

		case OP_OUT:
			if (m->ro + 32 == m->wo)
			{
				return OUTPUT_FULL;
			}
			a = address_of(m, m->pc+1, a_mode);
			m->outq[(m->wo++)&31] = m->ram[a];
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

struct panel
{
	int x;
	int y;
	int value;

	struct panel *next;
};

struct hull
{
	struct panel *table[TABLE_SIZE];
	size_t tcount;
	int x0, y0, x1, y1;
	struct module *m;
};

static void hull_init(struct hull *h)
{
	memset(h, 0, sizeof(*h));
	h->m = module_new(4096);
}

static void hull_reset(struct hull *h)
{
	for (int i = 0; i < TABLE_SIZE; i++)
	{
		struct panel *p = h->table[i];
		while (p)
		{
			struct panel *t = p;
			p = p->next;
			free(t);
		}
		h->table[i] = NULL;
	}
	h->tcount = 0;
	h->x0 = h->x1 = h->y0 = h->y1 = 0;
}

static void hull_destroy(struct hull *h)
{
	module_free(h->m);
	hull_reset(h);
}

static unsigned hashfn(int x, int y)
{
	return 5381 + x*33 + y;
}

static int hull_get(struct hull *h, int x, int y, int *value)
{
	unsigned pos = hashfn(x, y) & (TABLE_SIZE-1);
	struct panel *p = h->table[pos];
	while (p && !(p->x == x && p->y == y))
	{
		p = p->next;
	}
	if (p)
	{
		if (value)
		{
			*value = p->value;
		}
		return 1;
	}
	return 0;
}

static void hull_set(struct hull *h, int x, int y, int value)
{
	unsigned pos = hashfn(x, y) & (TABLE_SIZE-1);
	struct panel *p = h->table[pos];
	while (p && !(p->x == x && p->y == y))
	{
		p = p->next;
	}
	if (!p)
	{
		p = calloc(1, sizeof *p);
		p->x = x;
		p->y = y;
		p->next = h->table[pos];
		h->table[pos] = p;
		h->tcount++;
	}
	p->value = value;
}

static void hull_paint(struct hull *h, const int64_t *prog, size_t size, int start)
{
	hull_reset(h);
	module_load(h->m, prog, size);
	module_push_input(h->m, start);
	int x = 0;
	int y = 0;
	int dx = 0;
	int dy = -1;
	while (module_execute(h->m) == INPUT_EMPTY)
	{
		hull_set(h, x, y, module_pop_output(h->m));
		int d = module_pop_output(h->m);
		if (d == 0)
		{
			/* rotate left */
			int t = dx;
			dx = dy;
			dy = -t;
		}
		else if (d == 1)
		{
			/* rotate right */
			int t = dx;
			dx = -dy;
			dy = t;
		}
		x += dx;
		y += dy;
		if (x < h->x0) h->x0 = x;
		if (x > h->x1) h->x1 = x;
		if (y < h->y0) h->y0 = y;
		if (y > h->y1) h->y1 = y;

		int value = 0;
		hull_get(h, x, y, &value);
		module_push_input(h->m, value);
	}
}

int main(int argc, char *argv[])
{
	(void)module_input_full;
	(void)module_output_empty;
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

	struct hull h = {};
	hull_init(&h);
	hull_paint(&h, array, acount, 0);
	printf("part1: %zu\n", h.tcount);

	hull_paint(&h, array, acount, 1);
	printf("part2:\n");
	for (int y = h.y0; y <= h.y1; y++)
	{
		for (int x = h.x0; x <= h.x1; x++)
		{
			int v;
			putchar(hull_get(&h, x, y, &v) && v ? '#' : ' ');
		}
		putchar('\n');
	}

	free(array);
	hull_destroy(&h);
	return 0;
}
