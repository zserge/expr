-include config.mk

CC ?= clang
COV ?= llvm-cov

CFLAGS_COV ?= -fprofile-arcs -ftest-coverage
LDFLAGS_COV ?= $(CFLAGS_COV)

CFLAGS := -std=c99 -g -pedantic -Wall -Wextra -Wno-missing-field-initializers $(CFLAGS_COV)
LDFLAGS := -lm -g $(LDFLAGS_COV)

all:
	@echo make test - run tests
	@echo make cov  - report test coverage

test: expr_test
	./expr_test

expr_test: expr_test.o
	$(CC) $^ $(LDFLAGS) -o $@

expr_test.o: expr_test.c expr.h expr_debug.h

cov: test
	$(COV) -gcda=expr_test.gcda -gcno=expr_test.gcno | sed -ne '0,/expr_test\.c/p' | grep "#####:"

clean:
	rm -f expr_test
	rm -f *.o
	rm -f *.gcda *.gcno

.PHONY: clean all test cov
