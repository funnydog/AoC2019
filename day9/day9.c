#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

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
	WAITING = 0,
	HALTED = 1,
};

struct module
{
	int64_t *ram;
	size_t size;
	int64_t pc;		/* instruction/program counter */
	int64_t rbp;		/* relative base pointer */

	struct queue input;
	struct queue *output;
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
		m->pc = 0;
		m->input.r = m->input.w = 0;
		if (m->output)
		{
			m->output->r = m->output->w = 0;
		}
	}
	return m;
}

static void module_load(struct module *m, const int64_t *prog, size_t psize)
{
	assert(psize < m->size);
	memset(m->ram, 0, m->size * sizeof(m->ram[0]));
	memcpy(m->ram, prog, psize * sizeof(m->ram[0]));
	m->pc = 0;
	m->rbp = 0;
	queue_init(&m->input);
}

static void module_pipe(struct module *m, struct module *other)
{
	m->output = &other->input;
}

static void module_pushinput(struct module *m, int64_t value)
{
	queue_add(&m->input, value);
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
			a = address_of(m, m->pc+1, a_mode);
			assert(a_mode != IMODE);
			if (queue_empty(&m->input))
			{
				return WAITING;
			}
			m->ram[a] = queue_pop(&m->input);
			m->pc += 2;
			break;

		case OP_OUT:
			a = address_of(m, m->pc+1, a_mode);
			if (m->output && !queue_full(m->output))
			{
				queue_add(m->output, m->ram[a]);
			}
			else
			{
				fprintf(stderr, "output discarded %" SCNd64 "\n",
					m->ram[a]);
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

	struct module *m = module_new(4096);
	assert(m);
	module_pipe(m, m);

	/* self checks */
	{
		int64_t prog[] = {109,1,204,-1,1001,100,1,100,1008,100,16,101,1006,101,0,99};
		module_load(m, prog, sizeof(prog)/sizeof(prog[0]));
		module_execute(m);
		for (size_t i = 0; i < sizeof(prog)/sizeof(prog[0]); i++)
		{
			assert(prog[i] == queue_pop(&m->input));
		}
	}

	{
		int64_t prog[] = {1102,34915192LL,34915192LL,7,4,7,99,0};
		module_load(m, prog, sizeof(prog)/sizeof(prog[0]));
		module_execute(m);
		assert(snprintf(NULL, 0, "%" SCNd64, queue_pop(&m->input)) == 16);
	}

	{
		int64_t prog[] = {104,1125899906842624LL,99};
		module_load(m, prog, sizeof(prog)/sizeof(prog[0]));
		module_execute(m);
		assert(queue_pop(&m->input) == prog[1]);
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

	module_load(m, array, acount);
	module_pushinput(m, 1);
	module_execute(m);
	printf("part1: %" SCNd64 "\n", queue_pop(&m->input));

	module_load(m, array, acount);
	module_pushinput(m, 2);
	module_execute(m);
	printf("part2: %" SCNd64 "\n", queue_pop(&m->input));

	free(array);
	module_free(m);
	return 0;
}
