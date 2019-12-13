CFLAGS=-Wall -g -ggdb

.PHONY: clean all

all: day13

day13.c: queue.h

day13: day13.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@rm -f *.o day13
