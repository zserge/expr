#include "expr.h"

int main(void) {
  printf("macro definition: %f\n", expr_calc("$(mysum, $1 + $2), mysum(2, 3)"));
  printf("variable assignment: %f\n", expr_calc("a=2, b=3, a+b"));
  printf("all together: %f\n",
         expr_calc("$(mysum, $1 + $2), a=2, b=3, mysum(a, b)"));
  return 0;
}
