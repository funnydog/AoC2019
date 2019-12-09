#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
};

static int address_of(int pos, int mode, int *array, size_t size)
{
	assert(pos >= 0 && (size_t)pos < size);
	if (mode == PMODE)
	{
		pos = address_of(array[pos], IMODE, array, size);
	}
	return pos;
}

static void execute(int *array, size_t size)
{
	int pc = 0;
	for (;;)
	{
		int op, a, b, c;
		div_t d = div(array[pc], 100);
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
			a = address_of(pc+1, a_mode, array, size);
			b = address_of(pc+2, b_mode, array, size);
			c = address_of(pc+3, c_mode, array, size);
			assert(c_mode == PMODE);
			array[c] = array[a] + array[b];
			pc += 4;
			break;

		case OP_MUL:
			a = address_of(pc+1, a_mode, array, size);
			b = address_of(pc+2, b_mode, array, size);
			c = address_of(pc+3, c_mode, array, size);
			assert(c_mode == PMODE);
			array[c] = array[a] * array[b];
			pc += 4;
			break;

		case OP_IN:
			a = address_of(pc+1, a_mode, array, size);
			assert(a_mode == PMODE);
			printf("Enter value: ");
			scanf("%d", array + a);
			pc += 2;
			break;

		case OP_OUT:
			a = address_of(pc+1, a_mode, array, size);
			printf("Value: %d\n", array[a]);
			pc += 2;
			break;

		case OP_JNZ:
			a = address_of(pc+1, a_mode, array, size);
			b = address_of(pc+2, b_mode, array, size);
			if (array[a] != 0)
			{
				pc = array[b];
			}
			else
			{
				pc += 3;
			}
			break;

		case OP_JZ:
			a = address_of(pc+1, a_mode, array, size);
			b = address_of(pc+2, b_mode, array, size);
			if (array[a] == 0)
			{
				pc = array[b];
			}
			else
			{
				pc += 3;
			}
			break;

		case OP_TLT:
			a = address_of(pc+1, a_mode, array, size);
			b = address_of(pc+2, b_mode, array, size);
			c = address_of(pc+3, c_mode, array, size);
			assert(c_mode == PMODE);
			array[c] = array[a] < array[b] ? 1 : 0;
			pc += 4;
			break;

		case OP_TEQ:
			a = address_of(pc+1, a_mode, array, size);
			b = address_of(pc+2, b_mode, array, size);
			c = address_of(pc+3, c_mode, array, size);
			assert(c_mode == PMODE);
			array[c] = array[a] == array[b] ? 1 : 0;
			pc += 4;
			break;

		case OP_HALT:
			return;

		default:
			fprintf(stderr, "Unkown opcode %d at %d\n", array[pc], pc);
			return;
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

	execute(array, acount);

	free(array);

	return 0;
}
