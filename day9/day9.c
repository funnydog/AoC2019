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
	m->pc = m->rbp = m->ri = m->ro = m->wi = m->wo = 0;
}

static void module_push_input(struct module *m, int64_t value)
{
	assert(m->ri + 32 != m->wi);
	m->inq[(m->wi++)&31] = value;
}

static int module_input_full(struct module *m)
{
	return m->ri+32 == m->wi;
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

int main(int argc, char *argv[])
{
	(void)module_output_empty;
	(void)module_input_full;
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

	struct module *m = module_new(4096);
	assert(m);

	/* self checks */
	{
		int64_t prog[] = {109,1,204,-1,1001,100,1,100,1008,100,16,101,1006,101,0,99};
		module_load(m, prog, sizeof(prog)/sizeof(prog[0]));
		module_execute(m);
		for (size_t i = 0; i < sizeof(prog)/sizeof(prog[0]); i++)
		{
			int64_t v = module_pop_output(m);
			assert(prog[i] == v);
		}
	}

	{
		int64_t prog[] = {1102,34915192LL,34915192LL,7,4,7,99,0};
		module_load(m, prog, sizeof(prog)/sizeof(prog[0]));
		module_execute(m);
		int len = snprintf(NULL, 0, "%" SCNd64, module_pop_output(m));
		assert(len == 16);
	}

	{
		int64_t prog[] = {104,1125899906842624LL,99};
		module_load(m, prog, sizeof(prog)/sizeof(prog[0]));
		module_execute(m);
		int64_t v = module_pop_output(m);
		assert(prog[1] == v);
	}

	int64_t *program = NULL;
	size_t pcount = 0;
	size_t psize = 0;

	int64_t value;
	while (fscanf(input, "%" SCNd64 ",", &value) == 1)
	{
		if (pcount == psize)
		{
			size_t newsize = psize ? psize * 2 : 32;
			int64_t *newprogram = realloc(program, newsize * sizeof(*newprogram));
			if (!newprogram)
			{
				abort();
			}
			psize = newsize;
			program = newprogram;
		}
		program[pcount++] = value;
	}
	fclose(input);

	module_load(m, program, pcount);
	module_push_input(m, 1);
	module_execute(m);
	printf("part1: %" SCNd64 "\n", module_pop_output(m));

	module_load(m, program, pcount);
	module_push_input(m, 2);
	module_execute(m);
	printf("part2: %" SCNd64 "\n", module_pop_output(m));

	free(program);
	module_free(m);
	return 0;
}
