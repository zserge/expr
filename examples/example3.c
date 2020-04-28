#include "expr.h"

// Custom function that returns the sum of its two arguments
static expr_num_t sum(struct expr_func *f, vec_expr_t *args, void *c) {
  expr_num_t a = expr_eval(&vec_nth(args, 0));
  expr_num_t b = expr_eval(&vec_nth(args, 1));
  (void) f, (void) c;
  return a + b;
}

static struct expr_func user_funcs[] = {
  {"sum", sum, NULL, 0},
  {NULL, NULL, NULL, 0},
};

int main(void) {
  const char *s = "x = 5, sum(2, x)";
  struct expr_var_list vars = {0};
  struct expr *e = expr_create(s, strlen(s), &vars, user_funcs);
  if (e == NULL) {
    printf("Syntax error");
    return 1;
  }
  printf("result: %f\n", expr_eval(e));
  expr_destroy(e, &vars);
  return 0;
}
