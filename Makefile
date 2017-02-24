CFLAGS ?= -std=c99 -g -O0 -pedantic -Wall -Wextra
LDFLAGS ?= -lm -O0 -g

TESTBIN := expr_test

all:
	@echo make test      - run tests
	@echo make llvm-cov  - report test coverage using LLVM (set LLVM_VER if needed)
	@echo make gcov  - report test coverage (set GCC_VER if needed)

test: $(TESTBIN)
	./$(TESTBIN)

$(TESTBIN): expr_test.o
	$(CC) $^ $(LDFLAGS) -o $@

expr_test.o: expr_test.c expr.h expr_debug.h

llvm-cov: CC := clang$(LLVM_VER)
llvm-cov: CFLAGS += -fprofile-instr-generate -fcoverage-mapping
llvm-cov: LDFLAGS += -fprofile-instr-generate -fcoverage-mapping
llvm-cov: clean test
	llvm-profdata$(LLVM_VER) merge -o $(TESTBIN).profdata default.profraw
	llvm-cov$(LLVM_VER) show ./$(TESTBIN) -instr-profile=$(TESTBIN).profdata -name-regex='expr.*'

gcov: CC := gcc$(GCC_VER)
gcov: CFLAGS += -fprofile-arcs -ftest-coverage
gcov: LDFLAGS += -fprofile-arcs -ftest-coverage
gcov: clean test
	gcov$(GCC_VER) $(TESTBIN)
	cat expr.h.gcov

clean:
	rm -f $(TESTBIN) *.o *.profraw *.profdata *.gcov *.gcda *.gcno

.PHONY: clean all test gcov llvm-cov
