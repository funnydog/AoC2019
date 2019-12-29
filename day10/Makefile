CFLAGS=-Wall -g -ggdb
LDFLAGS=-lm

.PHONY: clean all

all: day10

day10: day10.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o day10
