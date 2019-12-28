CFLAGS=-Wall -g -ggdb $(shell pkg-config --cflags libbsd-overlay)
LDFLAGS=$(shell pkg-config --libs libbsd-overlay)

.PHONY: all clean

all: day25

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $< $(LDFLAGS)

%.png: %.dot
	dot -Tpng -o $@ $<

day25: day25.o

clean:
	rm -f *.o *.dot *.png day25
