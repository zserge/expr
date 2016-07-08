CFLAGS := -std=c99 -g
LDFLAGS := -lm

all: expr_test

expr_test: expr_test.o
	$(CC) $^ $(LDFLAGS) -o $@

expr_test.o: expr_test.c expr.h expr_debug.h
