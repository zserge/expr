# expr

[![Build Status](https://travis-ci.org/zserge/expr.svg?branch=master)](https://travis-ci.org/zserge/expr)

Expr is a mathematical expression evaluator written in C. It takes string as
input and returns floating-point number as a result.

## Features

- Supports arithmetic, bitwise and logical operators
- Supports variables
- Supports macros
- Can be extended with custom functions
- Error handling with error kind and position
- Simple evaluation takes ~50 nanoseconds on an average PC
- Low memory usage makes it suitable for embedded systems
- Pure C99 with no external dependencies
- Good test coverage
- Easy to understand (~600 LOC in a single header file)

## Example

```c
#include "expr.h"

int main(void) {
  printf("result: %f\n", expr_calc("2 + 3"));
  return 0;
}
```

Output: `result: 5.000000`

## API

```c
static struct expr *expr_create2(const char *s, size_t len,
                                 struct expr_var_list *vars,
                                 struct expr_func *funcs, int *near,
                                 int *error);

static struct expr *expr_create(const char *s, size_t len,
                                struct expr_var_list *vars,
                                struct expr_func *funcs);
```

Returns compiled expression from the given string. If expression uses
variables - they are bound to `vars`, so you can modify values before evaluation
or check the results after the evaluation. The `near` and `error` arguments are
used for error handling.

```c
static void expr_destroy(struct expr *e, struct expr_var_list *vars);
```

Cleans up. Parameters can be `NULL` (e.g. if you want to clean up expression,
but reuse variables for another expression).

```c
static expr_num_t expr_eval(struct expr *e);
```

Evaluates compiled expression.

```c
static struct expr_var *expr_var(struct expr_var_list *vars, const char *s,
                                 size_t len);
```

Returns/creates variable of the given name in the given list. This can be used
to get variable references to get/set them manually.

```c
static expr_num_t expr_calc(const char *s);
```

Takes an expression and immediately returns the result of it. If there is a parse error, `expr_calc()` returns `NAN`.

## Supported operators

- Arithmetics: `+`, `-`, `*`, `/`, `%` (remainder), `**` (power)
- Bitwise: `<<`, `>>`, `&`, `|`, `^` (xor or unary bitwise negation)
- Logical: `<`, `>`, `==`, `!=`, `<=`, `>=`, `&&`, `||`, `!` (unary not)
- Other: `=` (assignment, e.g. `x=y=5`), `,` (separates expressions or function parameters)

Only the following functions from libc are used to reduce the footprint and
make it easier to use:

- `calloc()`, `realloc()`, `free()` and `memcpy()` - memory management (all replaceable via macro)
- `isnan()`, `isinf()`, `fmod()`, `pow()` - math operations (`fmod()` and `pow()` replaceable via macro)
- `strlen()`, `strncmp()` and `snprintf()` - tokenizing and parsing (all replaceable via macro)

## Running tests

To run all the tests and benchmarks do `make test`. This will be using your
default compiler and will do no code coverage.

To see the code coverage you may either do `make llvm-cov` or `make gcov`
depending on whether you use GCC or LLVM/Clang.

Since people may have different compiler versions, one may specify a version
explicitly, e.g. `make llvm-cov LLVM_VER=-3.8` or `make gcov GCC_VER=-5`.

## Building with CMake

To build all the examples, tests and benchmarks using [CMake](https://cmake.org), do:

```bash
mkdir build && cd build/
cmake ..
make
```

## Running PVS Studio analysis

Download and install the PVS's command line tool [`how-to-use-pvs-studio-free`](https://github.com/viva64/how-to-use-pvs-studio-free) according its site instructions. After that, in the library root directory, perform the following commands:

```bash
how-to-use-pvs-studio-free -c 2 -m .
mkdir build && cd build/
cmake -DEXPR_PVS_STUDIO=ON ..
make pvs_studio_analysis
```

The full PVS report will be generated in the `build/pvs_studio_fullhtml` directory.

## License

Code is distributed under MIT license, feel free to use it in your proprietary
projects as well.
