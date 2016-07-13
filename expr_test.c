#include "expr.h"

#if !defined(NDEBUG)
#include "expr_debug.h"
#endif

#include <sys/time.h>
#include <assert.h>

int status = 0;

/*
 * VECTOR TESTS
 */
typedef vec(int) test_vec_int_t;
typedef vec(char *) test_vec_str_t;

static void test_vector() {
  test_vec_int_t ints = {0};
  test_vec_str_t strings = {0};

  vec_push(&ints, 3);
  assert(vec_len(&ints) == 1);
  assert(vec_peek(&ints) == 3);
  assert(vec_pop(&ints) == 3);
  assert(vec_len(&ints) == 0);
  vec_free(&ints);

  vec_push(&strings, "hello");
  vec_push(&strings, "world");
  vec_push(&strings, "foo");
  assert(vec_len(&strings) == 3);
  int i;
  char *el;
  vec_foreach(&strings, el, i) { printf("%s %d\n", el, i); }
  vec_free(&strings);
}

/*
 * VARIABLES VECTOR TEST
 */
static void test_vars() {
  struct expr_var_list vars = {0};

  struct expr_var *a = expr_var(&vars, "a", 1);
  a->value = 4;
  struct expr_var *b = expr_var(&vars, "b", 1);

  struct expr_var *again = expr_var(&vars, "a", 1);
  assert(again == a);
  assert(again->value == 4);
  expr_destroy(NULL, &vars);
}

/*
 * LEXER TESTS
 */
static int assert_tokens(char *s, char **expected) {
  int len = strlen(s);
  int flags = EXPR_TDEFAULT;
  char *test = s;
  for (;;) {
    int n = expr_next_token(s, len, &flags);
    if (n == 0) {
      if (*expected == NULL) {
        printf("OK '%s'\n", test);
        return 0;
      } else {
        printf("FAIL '%s': not enough tokens\n", test);
        status = 1;
      }
    } else if (n < 0) {
      printf("FAIL '%s': error %d\n", test, n);
      status = 1;
      return 0;
    }
    if (strncmp(*expected, s, n) != 0) {
      printf("FAIL '%s': token mismatch %.*s %s\n", test, n, s, *expected);
      status = 1;
      return 0;
    }
    expected++;
    s = s + n;
    len = len - n;
  }
}

static void test_tokizer() {
  char **TESTS[] = {
      (char *[]){"", NULL}, (char *[]){"1", "1", NULL},
      (char *[]){"1+11", "1", "+", "11", NULL},
      (char *[]){"1*11", "1", "*", "11", NULL},
      (char *[]){"1**11", "1", "**", "11", NULL},
      (char *[]){"1**-11", "1", "**", "-", "11", NULL},
  };
  for (int i = 0; i < sizeof(TESTS) / sizeof(TESTS[0]); i++) {
    assert_tokens(TESTS[i][0], TESTS[i] + 1);
  }
}

/*
 * PARSER TESTS
 */
static float user_func_nop(struct expr_func *f, vec_expr_t args, void *c) {
  return 0;
}

static float user_func_add(struct expr_func *f, vec_expr_t args, void *c) {
  float a = expr_eval(&vec_nth(&args, 0));
  float b = expr_eval(&vec_nth(&args, 1));
  return a + b;
}

static float user_func_next(struct expr_func *f, vec_expr_t args, void *c) {
  float a = expr_eval(&vec_nth(&args, 0));
  return a + 1;
}

static struct expr_func user_funcs[] = {
    {"nop", user_func_nop, 100},
    {"add", user_func_add, 0},
    {"next", user_func_next, 0},
    {NULL, NULL, 0},
};

static void test_expr(char *s, float expected) {
  struct expr_var_list vars = {0};
  struct expr *e = expr_create(s, strlen(s), &vars, user_funcs);
  if (e == NULL) {
    printf("FAIL: %s returned NULL\n", s);
    status = 1;
    return;
  }
  float result = expr_eval(e);
  if (fabs(result - expected) > 0.00001f) {
    printf("FAIL: %s: %f != %f\n", s, result, expected);
    status = 1;
  } else {
    printf("OK: %s == %f\n", s, expected);
  }
  expr_destroy(e, &vars);
}

static void test_expr_error(char *s) {
  struct expr_var_list vars = {0};
  struct expr *e = expr_create(s, strlen(s), &vars, user_funcs);
  if (e != NULL) {
    printf("FAIL: %s should return error\n", s);
    status = 1;
    expr_destroy(e, &vars);
    return;
  }
}

static void test_empty() {
  test_expr("", 0);
  test_expr("  ", 0);
  test_expr("  \t \n ", 0);
}

static void test_const() {
  test_expr("1", 1);
  test_expr(" 1 ", 1);
  test_expr("12", 12);
  test_expr("12.3", 12.3);
}

static void test_unary() {
  test_expr("-1", -1);
  test_expr("--1", -(-1));
  test_expr("!0 ", !0);
  test_expr("!2 ", !2);
  test_expr("^3", ~3);
}

static void test_binary() {
  test_expr("1+2", 1 + 2);
  test_expr("10-2", 10 - 2);
  test_expr("2*3", 2 * 3);
  test_expr("2+3*4", 2 + 3 * 4);
  test_expr("2*3+4", 2 * 3 + 4);
  test_expr("2+3/2", 2 + 3.0 / 2.0);
  test_expr("6/2+8*4/2", 19);
  test_expr("3/2", 3.0 / 2.0);
  test_expr("(3/2)|0", 3 / 2);
  test_expr("(3/0)", INFINITY);
  test_expr("(3/0)|0", INT_MAX);
  test_expr("(3%0)", NAN);
  test_expr("(3%0)|0", 0);
  test_expr("2**3", 8);
  test_expr("2<<3", 2 << 3);
  test_expr("12>>2", 12 >> 2);
  test_expr("2<2", 3 < 2);
  test_expr("2<=2", 2 <= 2);
  test_expr("2==2", 2 == 2);
  test_expr("2!=2", 2 != 2);
  test_expr("3>2", 3 > 2);
  test_expr("3>=2", 3 >= 2);
  test_expr("123&42", 123 & 42);
  test_expr("123^42", 123 ^ 42);
}

static void test_logical() {
  test_expr("2&&3", 3);
  test_expr("0&&3", 0);
  test_expr("3&&0", 0);
  test_expr("2||3", 2);
  test_expr("0||3", 3);
  test_expr("2||0", 2);
  test_expr("0||0", 0);
}

static void test_parens() {
  test_expr("(1+2)*3", (1 + 2) * 3);
  test_expr("(1)", 1);
  test_expr("(2.4)", 2.4);
  test_expr("((2))", 2);
  test_expr("(((3)))", 3);
  test_expr("(((3)))*(1+(2))", 9);
}

static void test_assign() {
  test_expr("x=5", 5);
  test_expr("x=y=3", 3);
}

static void test_comma() {
  test_expr("2,3,4", 4);
  test_expr("2+3,4*5", 4 * 5);
  test_expr("x=5, y = 3, x+y", 8);
  test_expr("x=5, x=(x!=0)", 1);
  test_expr("x=5, x = x+1", 6);
}

static void test_funcs() {
  test_expr("add(1,2) + next(3)", 7);
  test_expr("add(1,1+1) + add(2*2+1,2)", 10);
  test_expr("nop()", 0);
}

static void test_benchmark() {
  struct timeval t;
  gettimeofday(&t, NULL);
  double start = t.tv_sec + t.tv_usec * 1e-6;
  struct expr_var_list vars = {0};
  char *s = "x=2+3*(x/(42+next(x))),x";
  struct expr *e = expr_create(s, strlen(s), &vars, user_funcs);
  if (e == NULL) {
    printf("FAIL: %s returned NULL\n", s);
    status = 1;
    return;
  }
  long N = 1000000L;
  for (long i = 0; i < N; i++) {
    expr_eval(e);
  }
  gettimeofday(&t, NULL);
  double end = t.tv_sec + t.tv_usec * 1e-6;
  expr_destroy(e, &vars);
  double ns = 1000000000 * (end - start) / N;
  printf("TIME: %f ns/op (%dM op/sec)\n", ns, (int)(1000 / ns));
}

static void test_bad_syntax() {
  test_expr_error("(");
  test_expr_error(")");
  test_expr_error("()3");
  test_expr_error("()x");
  test_expr_error("0^+1");
  test_expr_error("()\\");
  test_expr_error("().");
  test_expr_error("4ever");
  test_expr_error("(2+3");
  test_expr_error("(-2");
  test_expr_error("*2");
  test_expr_error("nop");
  test_expr_error("nop(");
  test_expr_error("),");
  test_expr_error("+(");
  test_expr_error("2=3");
  test_expr_error("2.3.4");
  test_expr_error("1()");
  test_expr_error("x()");
  test_expr_error(",");
  test_expr_error("1,,2");
  test_expr_error("nop(,x)");
  test_expr_error("nop(x=)>1");
  test_expr_error("1 x");
  test_expr_error("1++");
  test_expr_error("foo((x))");
  test_expr_error("nop(x))");
  test_expr_error("nop((x)");
}

int main() {
  test_vector();
  test_vars();

  test_tokizer();

  test_empty();
  test_const();
  test_unary();
  test_binary();
  test_logical();
  test_parens();
  test_assign();
  test_comma();
  test_funcs();

  test_bad_syntax();

  test_benchmark();

  return status;
}
