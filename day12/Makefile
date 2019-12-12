CFLAGS=-Wall -g -ggdb

.PHONY: clean all

all: day12

day12: day12.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@rm -f *.o day12
