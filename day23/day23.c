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

static int module_output_len(struct module *m)
{
	return m->wo - m->ro;
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
			if  (m->output && 0 <= m->ram[a] && m->ram[a] < 256)
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
			if  (m->output && 0 <= m->ram[a] && m->ram[a] < 256)
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

int main(int argc, char *argv[])
{
	(void)module_input_full;
	(void)module_output_empty;
	(void)module_log;

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

	size_t pcount = 0;
	int64_t *program = program_load(input, &pcount);
	fclose(input);
	if (!program)
	{
		fprintf(stderr, "Cannot load the intcode program\n");
		return -1;
	}

	struct module *comp[50];
	for (int i = 0; i < 50; i++)
	{
		comp[i] = module_new();
		module_load(comp[i], program, pcount);
		module_push_input(comp[i], i);
	}
	free(program);

	int64_t natx = INT64_MAX;
	int64_t naty = INT64_MAX;
	int64_t lasty = INT64_MAX;
	int last_running = -1;
	int i = 0;
	for (;;)
	{
		struct module *c = comp[i];
		module_execute(c);
		if (module_output_len(c) >= 3)
		{
			last_running = i;
			int dst = module_pop_output(c);
			int64_t x = module_pop_output(c);
			int64_t y = module_pop_output(c);
			if (dst == 255)
			{
				if (naty == INT64_MAX)
				{
					printf("part1: %ld\n", y);
				}
				natx = x;
				naty = y;
				i = (i + 1) % 50;
			}
			else
			{
				last_running = -1;
				i = dst;
				module_push_input(comp[i], x);
				module_push_input(comp[i], y);
			}
		}
		else
		{
			module_push_input(c, -1);
			i = (i + 1) % 50;
		}

		if (i == last_running)
		{
			if (lasty == naty)
			{
				printf("part2: %ld\n", naty);
				break;
			}
			lasty = naty;
			i = 0;
			module_push_input(comp[i], natx);
			module_push_input(comp[i], naty);
		}
	}

	for (i = 0; i < 50; i++)
	{
		module_free(comp[i]);
	}
	return 0;
}
