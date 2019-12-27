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

static void module_pipe(struct module *m, struct queue *queue)
{
	m->output = queue;
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
				return INPUT_EMPTY;
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
			else if (m->output)
			{
				return OUTPUT_FULL;
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

struct game
{
	char screen[100][100];
	unsigned width, height;
	struct module *m;
	struct queue out;
	int64_t paddle_x;
	int64_t ball_x;
	int64_t score;
};

static void game_init(struct game *g)
{
	memset(g, 0, sizeof(*g));
	g->m = module_new(4096);
	module_pipe(g->m, &g->out);
}

static void game_reset(struct game *g)
{
	memset(g->screen, 0, sizeof(g->screen));
	g->width = g->height = 0;
	g->paddle_x = g->ball_x = g->score = 0;
	memset(&g->out, 0, sizeof(g->out));
}

static void game_destroy(struct game *g)
{
	module_free(g->m);
}

static void update_screen(struct game *g)
{
	while (queue_len(&g->out) >= 3)
	{
		int64_t x = queue_pop(&g->out);
		int64_t y = queue_pop(&g->out);
		int64_t id = queue_pop(&g->out);
		if (x == -1 && y == 0)
		{
			g->score = id;
			continue;
		}

		assert(0 <= x && (size_t)x < sizeof(g->screen[0]) / sizeof(g->screen[0][0]));
		assert(0 <= y && (size_t)y < sizeof(g->screen) / sizeof(g->screen[0]));
		if (g->width <= x)
		{
			g->width = x+1;
		}
		if (g->height <= y)
		{
			g->height = y+1;
		}
		g->screen[y][x] = id;
		if (id == 3)
		{
			g->paddle_x = x;
		}
		else if (id == 4)
		{
			g->ball_x = x;
		}
	}
}

static void game_paint(struct game *g, int up)
{
	if (up)
	{
		printf("\033[%dA", up);
	}
	for (unsigned y = 0; y < g->height; y++)
	{
		for (unsigned x = 0; x < g->width; x++)
		{
			char c;
			switch(g->screen[y][x])
			{
			case 1: c = '*'; break;
			case 2: c = '#'; break;
			case 3: c = '='; break;
			case 4: c = 'o'; break;
			default:
				c = ' ';
				break;
			}
			putc(c, stdout);
		}
		putc('\n', stdout);
	}
}

static void game_run(struct game *g, const int64_t *program, size_t count)
{
	module_load(g->m, program, count);
	int height = 0;
	int rv;
	while ((rv = module_execute(g->m)) != HALTED)
	{
		update_screen(g);
		if (rv == INPUT_EMPTY)
		{
			game_paint(g, height);
			height = g->height;
			if (g->paddle_x < g->ball_x)
			{
				module_pushinput(g->m, 1);
			}
			else if (g->paddle_x > g->ball_x)
			{
				module_pushinput(g->m, -1);
			}
			else
			{
				module_pushinput(g->m, 0);
			}
		}
	}
	update_screen(g);
	game_paint(g, height);
}

static size_t count_blocks(struct game *g, int64_t type)
{

	size_t blocks = 0;
	for (unsigned y = 0; y < g->height; y++)
	{
		for (unsigned x = 0; x < g->width; x++)
		{
			if (g->screen[y][x] == type)
			{
				blocks++;
			}
		}
	}
	return blocks;
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

	struct game h = {};
	game_init(&h);
	game_run(&h, array, acount);
	printf("part1: %zu\n", count_blocks(&h, 2));
	array[0] = 2;
	game_reset(&h);
	game_run(&h, array, acount);
	printf("part2: %ld\n", h.score);
	free(array);
	game_destroy(&h);
	return 0;
}
