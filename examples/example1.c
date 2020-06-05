#include "expr.h"

int main(void) {
  int near, error;
  printf("result: %f\n", expr_calc("2 + 3", &near, &error));
  return 0;
}
