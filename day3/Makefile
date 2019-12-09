CFLAGS=-g -ggdb -Wall

.PHONY: clean all

all: day3

day3: day3.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	@rm -f *.o day3
