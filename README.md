# expr

[![Build Status](https://travis-ci.org/zserge/expr.svg?branch=master)](https://travis-ci.org/zserge/expr)

Expr is a mathematical expression evaluator written in C. It takes string as
input and returns floating-point number as a result.

## Features

* Supports arithmetic, bitwise and logical operators
* Supports variables
* Can be extended with custom functions
* Simple evaluation takes ~50 nanoseconds on an average PC
* Low memory usage makes it suitable for embedded systems
* Pure C99 with no external dependencies
* Good test coverage
* Easy to understand (~600 LOC in a single header file)

## Example

```c
#include "expr.h"

// Custom function that returns the sum of its two arguments
static float add(struct expr_func *f, vec_expr_t *args, void *c) {
  float a = expr_eval(&vec_nth(args, 0));
  float b = expr_eval(&vec_nth(args, 1));
  return a + b;
}

static struct expr_func user_funcs[] = {
    {"add", add, NULL, 0},
    {NULL, NULL, NULL, 0},
};

int main() {
  const char *s = "x = 5, add(2, x)";
  struct expr_var_list vars = {0};
  struct expr *e = expr_create(s, strlen(s), &vars, user_funcs);
  if (e == NULL) {
    printf("Syntax error");
    return 1;
  }

  float result = expr_eval(e);
  printf("result: %f\n", result);

  expr_destroy(e, &vars);
  return 0;
}
```

Output: `result: 7.000000`

## API

`struct expr *expr_create(const char *s, size_t len, struct expr_var_list
*vars, struct expr_func *funcs)` - returns compiled expression from the given
string. If expression uses variables - they are bound to `vars`, so you can
modify values before evaluation or check the results after the evaluation.

`float expr_eval(struct expr *e)` - evaluates compiled expression.

`void expr_destroy(struct expr *e, struct expr_var_list *vars)` - cleans up
memory. Parameters can be NULL (e.g. if you want to clean up expression, but
reuse variables for another expression).

`struct expr_var *expr_var(struct expr_var *vars, const char *s, size_t len)` -
returns/creates variable of the given name in the given list. This can be used
to get variable references to get/set them manually.

## Supported operators

* Arithmetics: `+`, `-`, `*`, `/`, `%` (remainder), `**` (power)
* Bitwise: `<<`, `>>`, `&`, `|`, `^` (xor or unary bitwise negation)
* Logical: `<`, `>`, `==`, `!=`, `<=`, `>=`, `&&`, `||`, `!` (unary not)
* Other: `=` (assignment, e.g. `x=y=5`), `,` (separates expressions or function parameters)

Only the following functions from libc are used to reduce the footprint and
make it easier to use:

* calloc, realloc and free - memory management
* isnan, isinf, fmodf, powf - math operations
* strlen, strncmp, strncpy, strtof - tokenizing and parsing

## Running tests

To run all the tests and benchmarks do `make test`. This will be using your
default compiler and will do no code coverage.

To see the code coverage you may either do `make llvm-cov` or `make gcov`
depending on whether you use GCC or LLVM/Clang.

Since people may have different compiler versions, one may specify a version
explicitly, e.g. `make llvm-cov LLVM_VER=-3.8` or `make gcov GCC_VER=-5`.

## License

Code is distributed under MIT license, feel free to use it in your proprietary
projects as well.

