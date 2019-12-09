CFLAGS = -O0 -g -ggdb -Wall -Wextra -Wno-unused-parameter

.PHONY: clean all

all: day1

day1: day1.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@rm -f *.o day1
