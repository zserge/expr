CC := clang
CFLAGS := -std=c11 -g -pedantic -fprofile-arcs -ftest-coverage
LDFLAGS := -lm -g -fprofile-arcs -ftest-coverage

all: expr_test

expr_test: expr_test.o
	$(CC) $^ $(LDFLAGS) -o $@

expr_test.o: expr_test.c expr.h expr_debug.h

test: expr_test
	./expr_test
	llvm-cov -gcda=expr_test.gcda -gcno=expr_test.gcno | sed -ne '0,/expr_test\.c/p' | grep "#####:"

clean:
	rm -f expr_test
	rm -f *.o

.PHONY: clean all
