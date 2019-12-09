#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
	OP_ADD = 1,
	OP_MUL = 2,
	OP_HALT = 99,
};

void execute(int *array, size_t size)
{
	int pc = 0;
	for (;;)
	{
		size_t a, b, c;
		switch (array[pc])
		{
		case OP_ADD:
			a = array[pc+1];
			b = array[pc+2];
			c = array[pc+3];
			if (a >= size || b >= size || c >= size)
			{
				fprintf(stderr, "out of bounds\n");
			}
			else
			{
				array[c] = array[a] + array[b];
			}
			break;

		case OP_MUL:
			a = array[pc+1];
			b = array[pc+2];
			c = array[pc+3];
			if (a >= size || b >= size || c >= size)
			{
				fprintf(stderr, "out of bounds\n");
			}
			else
			{
				array[c] = array[a] * array[b];
			}
			break;

		case OP_HALT:
			return;

		default:
			fprintf(stderr, "Unkown opcode %d\n", array[pc]);
			return;
		}
		pc += 4;
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

	int *test = malloc(acount * sizeof(*test));
	if (!test)
	{
		abort();
	}
	memcpy(test, array, acount * sizeof(*test));
	test[1] = 12;
	test[2] = 2;
	execute(test, acount);
	printf("part 1: %d\n", test[0]);

	for (int i = 0; i < 100; i++)
	{
		for (int j = 0; j < 100; j++)
		{
			memcpy(test, array, acount * sizeof(*test));
			test[1] = i;
			test[2] = j;
			execute(test, acount);

			if (test[0] == 19690720)
			{
				printf("part 2: 100 * %d + %d = %d\n", i, j, i*100+j);
				i = j = 100;
				break;
			}
		}
	}

	free(test);
	free(array);

	return 0;
}
