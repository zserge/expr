#include "expr.h"

int main(void) {
  int near, error;
  printf("macro definition: %f\n",
         expr_calc("$(mysum, $1 + $2), mysum(2, 3)", &near, &error));
  printf("variable assignment: %f\n",
         expr_calc("a=2, b=3, a+b", &near, &error));
  printf("all together: %f\n",
         expr_calc("$(mysum, $1 + $2), a=2, b=3, mysum(a, b)", &near, &error));
  return 0;
}
