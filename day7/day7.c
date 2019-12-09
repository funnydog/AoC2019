#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "perm.h"
#include "queue.h"

enum
{
	OP_ADD	= 1,
	OP_MUL	= 2,
	OP_IN	= 3,
	OP_OUT	= 4,
	OP_JNZ	= 5,
	OP_JZ	= 6,
	OP_TLT	= 7,
	OP_TEQ	= 8,
	OP_HALT = 99,

	PMODE = 0,
	IMODE = 1,

	WAITING = 0,
	HALTED = 1,
};

struct module
{
	int *ram;
	size_t size;
	int pc;

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

static struct module *module_new(const int *ram, size_t size)
{
	struct module *m = calloc(1, sizeof(*m));
	if (m)
	{
		if (ram && size)
		{
			m->ram = malloc(size * sizeof(*ram));
			memcpy(m->ram, ram, size * sizeof(*ram));
		}
		m->size = size;
		m->pc = 0;
		m->input.r = m->input.w = 0;
		if (m->output)
		{
			m->output->r = m->output->w = 0;
		}
	}
	return m;
}

static void module_reset(struct module *m, const int *ram, size_t size)
{
	int *newarr = realloc(m->ram, size * sizeof(*m->ram));
	if (newarr)
	{
		m->ram = newarr;
		m->size = size;
		memcpy(m->ram, ram, size * sizeof(*ram));
		m->pc = 0;
	}
}

static void module_pipe(struct module *m, struct module *other)
{
	m->output = &other->input;
}

static void module_pushinput(struct module *m, int value)
{
	queue_add(&m->input, value);
}

static int address_of(struct module *m, int pos, int mode)
{
	assert(pos >= 0 && (size_t)pos < m->size);
	if (mode == PMODE)
	{
		pos = address_of(m, m->ram[pos], IMODE);
	}
	return pos;
}

static int module_execute(struct module *m)
{
	for (;;)
	{
		int op, a, b, c;
		div_t d = div(m->ram[m->pc], 100);
		op = d.rem;
		d = div(d.quot, 10);
		int a_mode = d.rem;
		d = div(d.quot, 10);
		int b_mode = d.rem;
		d = div(d.quot, 10);
		int c_mode = d.rem;

		switch (op)
		{
		case OP_ADD:
			a = address_of(m, m->pc+1, a_mode);
			b = address_of(m, m->pc+2, b_mode);
			c = address_of(m, m->pc+3, c_mode);
			assert(c_mode == PMODE);
			m->ram[c] = m->ram[a] + m->ram[b];
			m->pc += 4;
			break;

		case OP_MUL:
			a = address_of(m, m->pc+1, a_mode);
			b = address_of(m, m->pc+2, b_mode);
			c = address_of(m, m->pc+3, c_mode);
			assert(c_mode == PMODE);
			m->ram[c] = m->ram[a] * m->ram[b];
			m->pc += 4;
			break;

		case OP_IN:
			a = address_of(m, m->pc+1, a_mode);
			assert(a_mode == PMODE);
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
				fprintf(stderr, "output discarded %d\n", m->ram[a]);
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
			assert(c_mode == PMODE);
			m->ram[c] = m->ram[a] < m->ram[b] ? 1 : 0;
			m->pc += 4;
			break;

		case OP_TEQ:
			a = address_of(m, m->pc+1, a_mode);
			b = address_of(m, m->pc+2, b_mode);
			c = address_of(m, m->pc+3, c_mode);
			assert(c_mode == PMODE);
			m->ram[c] = m->ram[a] == m->ram[b] ? 1 : 0;
			m->pc += 4;
			break;

		case OP_HALT:
			return HALTED;

		default:
			fprintf(stderr, "Unknown opcode %d at %d\n", m->ram[m->pc], m->pc);
			abort();
		}
	}
}

static struct module **wire_modules(const int *ram, size_t size)
{
	struct module **m = calloc(5, sizeof(*m));
	for (int i = 0; i < 5; i++)
	{
		m[i] = module_new(ram, size);
		if (i > 0)
		{
			module_pipe(m[i-1], m[i]);
		}
	}
	module_pipe(m[4], m[0]);
	return m;
}

static void reset_modules(struct module **ma, const int *ram, size_t size)
{
	for (int i = 0; i < 5; i++)
	{
		module_reset(ma[i], ram, size);
	}
}

static void destroy_modules(struct module **marr)
{
	for (int i = 0; i < 5; i++)
	{
		module_free(marr[i]);
	}
	free(marr);
}

static int signal(struct module **marr, int *sarr, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		module_pushinput(marr[i], sarr[i]);
	}
	module_pushinput(marr[0], 0);

	int exit;
	do
	{
		exit = 1;
		for (size_t i = 0; i < count; i++)
		{
			exit &= module_execute(marr[i]) != WAITING;
		}
	} while (!exit);

	return queue_pop(marr[count-1]->output);
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

	struct module **ma = wire_modules(NULL, 0);
	{
		int ram[] = {3,15,3,16,1002,16,10,16,1,16,15,15,4,15,99,0,0};
		int sequence[] = {4, 3, 2, 1, 0};
		reset_modules(ma, ram, sizeof(ram)/sizeof(ram[0]));
		assert(signal(ma, sequence, 5) == 43210);
	}

	{
		int ram[] = {
			3,23,3,24,1002,24,10,24,1002,23,-1,23,
			101,5,23,23,1,24,23,23,4,23,99,0,0
		};
		int sequence[] = {0, 1, 2, 3, 4};
		reset_modules(ma, ram, sizeof(ram)/sizeof(ram[0]));
		assert(signal(ma, sequence, 5) == 54321);
	}

	{
		int ram[] = {
			3,31,3,32,1002,32,10,32,1001,31,-2,31,1007,31,0,33,
			1002,33,7,33,1,33,31,31,1,32,31,31,4,31,99,0,0,0
		};
		int sequence[] = {1, 0, 4, 3, 2};
		reset_modules(ma, ram, sizeof(ram)/sizeof(ram[0]));
		assert(signal(ma, sequence, 5) == 65210);
	}

	{
		int ram[] = {
			3,26,1001,26,-4,26,3,27,1002,27,2,27,1,27,26,
			27,4,27,1001,28,-1,28,1005,28,6,99,0,0,5
		};
		int sequence[] = {9,8,7,6,5};
		reset_modules(ma, ram, sizeof(ram)/sizeof(ram[0]));
		assert(signal(ma, sequence, 5) == 139629729);
	}

	{
		int ram[] = {
			3,52,1001,52,-5,52,3,53,1,52,56,54,1007,54,5,55,1005,55,26,1001,54,
			-5,54,1105,1,12,1,53,54,53,1008,54,0,55,1001,55,1,55,2,53,55,53,4,
			53,1001,56,-1,56,1005,56,6,99,0,0,0,0,10
		};
		int sequence[] = {9,7,8,5,6};
		reset_modules(ma, ram, sizeof(ram)/sizeof(ram[0]));
		assert(signal(ma, sequence, 5) == 18216);
	}

	int *array = NULL;
	size_t acount = 0;
	size_t asize = 0;

	int value;
	while (fscanf(input, "%d,", &value) == 1)
	{
		if (acount == asize)
		{
			size_t newsize = asize ? asize * 2 : 32;
			int *newarray = realloc(array, newsize * sizeof(*newarray));
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

	int maxv = 0;
	for (struct piterator *p = perm_init((int[]){0,1,2,3,4}, 5);
	     p;
	     p = perm_next(p))
	{
		reset_modules(ma, array, acount);
		int s = signal(ma, p->a, p->size);
		if (maxv < s)
		{
			maxv = s;
		}
	}
	printf("part1: %d\n", maxv);

	maxv = 0;
	for (struct piterator *p = perm_init((int[]){5,6,7,8,9}, 5);
	     p;
	     p = perm_next(p))
	{
		reset_modules(ma, array, acount);
		int s = signal(ma, p->a, p->size);
		if (maxv < s)
		{
			maxv = s;
		}
	}
	printf("part2: %d\n", maxv);

	free(array);
	destroy_modules(ma);
	return 0;
}
