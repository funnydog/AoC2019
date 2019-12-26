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

	FILE *output;		/* echoes the input and output */
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
	struct module *m = calloc(1, sizeof(*m));
	if (m)
	{
		m->ram = NULL;
		m->size = 0;
		m->pc = 0;
		m->ri = m->wi = m->ro = m->wo = 0;
	}
	return m;
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

static int module_input_full(struct module *m)
{
	return m->ri + 32 == m->wi;
}

static void module_push_input(struct module *m, int64_t value)
{
	assert(m->wi - m->ri < 32);
	if (m->output && 0 <= value && value < 256)
	{
		putc(value, m->output);
	}
	m->inq[(m->wi++) & 31] = value;
}

static int module_output_empty(struct module *m)
{
	return m->wo == m->ro;
}

static int64_t module_pop_output(struct module *m)
{
	assert(m->wo != m->ro);
	int64_t value = m->outq[(m->ro++) & 31];
	if (m->output && 0 <= value && value < 256)
	{
		putc(value, m->output);
	}
	return value;
}

static void module_dump(struct module *m, FILE *out)
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

static void module_print(struct module *m)
{
	int status;
	do
	{
		status = module_execute(m);
		while (!module_output_empty(m))
		{
			if (m->outq[m->ro & 31] > 256)
			{
				break;
			}
			module_pop_output(m);
		}
	} while (status == OUTPUT_FULL);
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

struct map
{
	char *points;
	size_t size;
	size_t count;
	unsigned width;
	unsigned height;
	unsigned startx;
	unsigned starty;
};

static void map_free(struct map *m)
{
	if (m)
	{
		free(m->points);
		free(m);
	}
}

static struct map *map_discover(struct module *mod)
{
	struct map *m = calloc(1, sizeof(*m));
	assert(m);

	int status;
	do
	{
		int64_t old = 0;
		status = module_execute(mod);
		while (!module_output_empty(mod))
		{
			int64_t v = module_pop_output(mod);
			if (v == '\n' && old != v)
			{
				old = v;
				m->height++;
				continue;
			}
			else if (m->height == 0)
			{
				m->width++;
			}

			/* take note of the current bot position */
			if (strchr("^>v<", v))
			{
				m->startx = m->count % m->width;
				m->starty = m->count / m->width;
			}

			if (m->count == m->size)
			{
				size_t nsize = m->size ? m->size * 2 : 64;
				char *npoints = realloc(m->points, nsize);
				if (!npoints)
				{
					map_free(m);
					return NULL;
				}
				m->size = nsize;
				m->points = npoints;
			}
			m->points[m->count++] = v;
		}
	} while (status == OUTPUT_FULL);
	return m;
}

static char map_get(struct map *m, unsigned x, unsigned y)
{
	if (x < m->width && y < m->height)
	{
		return m->points[y * m->width + x];
	}
	return 0;
}

static unsigned map_alignment(struct map *m)
{
	const int dx[] = {0,  0, 1, 0, -1};
	const int dy[] = {0, -1, 0, 1,  0};
	unsigned alignment = 0;
	for (unsigned y = 0; y < m->height; y++)
	{
		for (unsigned x = 0; x < m->width; x++)
		{
			int i;
			for (i = 0; i < 5; i++)
			{
				if (map_get(m, x + dx[i], y + dy[i]) != '#')
				{
					break;
				}
			}

			if (i == 5)
			{
				alignment += x*y;
			}
		}
	}
	return alignment;
}

static char *map_path(struct map *m, size_t *cmdcount)
{
	int dx, dy;
	unsigned x = m->startx;
	unsigned y = m->starty;

	switch (map_get(m, x, y))
	{
	case '^': dx = 0; dy = -1; break;
	case '>': dx = 1; dy = 0; break;
	case 'v': dx = 0; dy = 1; break;
	case '<': dx = -1; dy = 0; break;
	default:
		abort();
	}

	char *path = NULL;
	size_t psize = 0;
	size_t pcount = 0;
	cmdcount[0] = 0;
	for (;;)
	{
		/* rotate left */
		unsigned lx = dy;
		unsigned ly = -dx;

		/* rotate right */
		unsigned rx = -dy;
		unsigned ry = dx;

		/* command */
		int cmd;
		int len;

		if (map_get(m, x + dx, y + dy) == '#')
		{
			cmd = 0;
			len = 0;
			unsigned nx, ny;
			nx = x + dx;
			ny = y + dy;
			while (map_get(m, nx, ny) == '#')
			{
				x = nx;
				y = ny;
				nx += dx;
				ny += dy;
				len++;
			}
		}
		else if (map_get(m, x + lx, y + ly) == '#')
		{
			cmd = 1;
			dx = lx;
			dy = ly;
		}
		else if (map_get(m, x + rx, y + ry) == '#')
		{
			cmd = 2;
			dx = rx;
			dy = ry;
		}
		else
		{
			break;
		}

		cmdcount[0]++;
		int required;
		switch(cmd)
		{
		case 0: required = snprintf(NULL, 0, "%d,", len); break;
		case 1:
		case 2: required = 2; break;
		}
		while (pcount+required >= psize)
		{
			size_t nsize = psize ? psize * 2 : (required + 1);
			char *npath = realloc(path, nsize);
			if (!npath)
			{
				free(path);
				return NULL;
			}
			psize = nsize;
			path = npath;
		}
		switch(cmd)
		{
		case 0: snprintf(path+pcount, psize-pcount, "%d,", len); break;
		case 1: snprintf(path+pcount, psize-pcount, "L,"); break;
		case 2: snprintf(path+pcount, psize-pcount, "R,"); break;
		}
		pcount += required;
	}

	return path;
}

static int check_equal(const char **ptr, int s, int e, int pos)
{
	const char *pp;
	const char *ss;
	for (ss = ptr[s], pp = ptr[pos];
	     ss < ptr[e];
	     ss++, pp++)
	{
		if (*ss != *pp)
		{
			return 0;
		}
	}
	return 1;
}

static int skip_equal(const char **ptrs, int s, int e, int off)
{
	if (check_equal(ptrs, s, e, off))
	{
		off += e - s;
	}
	return off;
}

struct interval
{
	const char *str;
	int len;
};

static int decompose(const char *path, struct interval *inter, size_t cmdcount)
{
	/* find the start of each parameter */
	const char **ptrs = malloc((cmdcount+1) * sizeof(*ptrs));
	assert(ptrs);
	ptrs[0] = path;
	for (size_t i = 1; i < cmdcount+1; i++)
	{
		ptrs[i] = strchr(ptrs[i-1], ',')+1;
		assert(ptrs[i] != ptrs[i-1]);
	}

	/* brute force the search */
	size_t sa = 0;
	for (size_t ea = sa+2; ea < sa+20; ea += 2)
	{
		size_t sb = ea, osb = 0;
		while (sb != osb)
		{
			osb = sb;
			sb = skip_equal(ptrs, sa, ea, sb);
		}
		for (size_t eb = sb+2; eb < sb+20; eb += 2)
		{
			size_t sc = eb, osc = 0;
			while (sc != osc)
			{
				osc = sc;
				sc = skip_equal(ptrs, sa, ea, sc);
				sc = skip_equal(ptrs, sb, eb, sc);
			}
			for (size_t ec = sc+2; ec < sc+20; ec += 2)
			{
				size_t sd = ec, osd = 0;
				while (sd != osd)
				{
					osd = sd;
					sd = skip_equal(ptrs, sa, ea, sd);
					sd = skip_equal(ptrs, sb, eb, sd);
					sd = skip_equal(ptrs, sc, ec, sd);
				}
				if (sd == cmdcount)
				{
					inter[0].str = ptrs[sa];
					inter[1].str = ptrs[sb];
					inter[2].str = ptrs[sc];
					inter[0].len = ptrs[ea] - ptrs[sa];
					inter[1].len = ptrs[eb] - ptrs[sb];
					inter[2].len = ptrs[ec] - ptrs[sc];
					free(ptrs);
					return 0;
				}
			}
		}
	}
	free(ptrs);
	return -1;
}

static int64_t program_robot(struct module *mod, struct map *m)
{
	size_t cmdcount;
	char *path = map_path(m, &cmdcount);

	struct interval intervals[3];
	if (decompose(path, intervals, cmdcount) < 0)
	{
		fprintf(stderr, "Cannot decompose the command stream\n");
		free(path);
		return -1;
	}

	/* Main routine */
	module_print(mod);
	const char *t = path;
	while (*t)
	{
		int c;
		int i;
		for (i = 0; i < 3; i++)
		{
			if (strncmp(t, intervals[i].str, intervals[i].len) == 0)
			{
				t += intervals[i].len;
				c = 'A' + i;
				break;
			}
		}
		if (i < 3)
		{
			module_push_input(mod, c);
			if (*t)
			{
				module_push_input(mod, ',');
			}
		}
	}
	module_push_input(mod, '\n');

	/* A, B, C function definitions */
	module_print(mod);
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < intervals[i].len-1; j++)
		{
			while (module_input_full(mod))
			{
				module_execute(mod);
			}
			module_push_input(mod, intervals[i].str[j]);
		}
		while (module_input_full(mod))
		{
			module_execute(mod);
		}
		module_push_input(mod, 10);
		module_print(mod);
	}

	/* video feed */
	module_push_input(mod, 'n');
	module_push_input(mod, '\n');
	module_print(mod);
	free(path);
	return module_pop_output(mod);
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

	size_t pcount;
	int64_t *program = program_load(input, &pcount);
	fclose(input);
	if (!program)
	{
		fprintf(stderr, "Cannot load the program\n");
		return -1;
	}

	struct module *mod = module_new();
#ifdef DUMP_OUTPUT
	module_dump(mod, stdout);
#else
	(void)module_dump;
#endif

	module_load(mod, program, pcount);
	struct map *m = map_discover(mod);
	printf("part1: %u\n", map_alignment(m));

	/* patch the program */
	program[0] = 2;
	module_load(mod, program, pcount);
	printf("part2: %ld\n", program_robot(mod, m));

	map_free(m);
	module_free(mod);
	free(program);
	return 0;
}
