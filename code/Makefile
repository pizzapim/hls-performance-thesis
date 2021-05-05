SA_JUMPS ?= 1
RANK_JUMPS ?= 1

CC=gcc
CPPC=g++
CFLAGS=-I. -Wextra -Wall -g -DSA_JUMPS=$(SA_JUMPS) -DRANK_JUMPS=$(RANK_JUMPS)
DEPS = fmindex.h util.h
OBJ = fmindex.o util.o
EXES = program repl construct generate_test_data benchmark

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(EXES)

program: $(OBJ) program.o
	$(CC) -o $@ $^ $(CFLAGS)

repl: $(OBJ) repl.o
	$(CC) -o $@ $^ $(CFLAGS)

construct: $(OBJ) construct.o
	$(CC) -o $@ $^ $(CFLAGS)

generate_test_data: $(OBJ) generate_test_data.o
	$(CPPC) -o $@ $^ $(CFLAGS)

benchmark: $(OBJ) benchmark.o
	$(CPPC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ core $(EXES)
