#include "expr.h"

#if 0
/* This can be useful for debugging */
#include "expr_debug.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <sys/time.h>

int status = 0;

/*
 * VECTOR TESTS
 */
typedef vec(int) test_vec_int_t;
typedef vec(char *) test_vec_str_t;

static void test_vector() {
  test_vec_int_t ints = vec_init();
  test_vec_str_t strings = vec_init();

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

  expr_var(&vars, "b", 1);
  expr_var(&vars, "ab", 2);

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
      (char *[]){"", NULL},
      (char *[]){"1", "1", NULL},
      (char *[]){"1+11", "1", "+", "11", NULL},
      (char *[]){"1*11", "1", "*", "11", NULL},
      (char *[]){"1**11", "1", "**", "11", NULL},
      (char *[]){"1**-11", "1", "**", "-", "11", NULL},
  };
  for (unsigned int i = 0; i < sizeof(TESTS) / sizeof(TESTS[0]); i++) {
    assert_tokens(TESTS[i][0], TESTS[i] + 1);
  }
}

/*
 * PARSER TESTS
 */
struct nop_context {
  void *p;
};
static void user_func_nop_cleanup(struct expr_func *f, void *c) {
  (void)f;
  struct nop_context *nop = (struct nop_context *)c;
  free(nop->p);
}
static float user_func_nop(struct expr_func *f, vec_expr_t *args, void *c) {
  (void)args;
  struct nop_context *nop = (struct nop_context *)c;
  if (f->ctxsz == 0) {
    free(nop->p);
    return 0;
  }
  if (nop->p == NULL) {
    nop->p = malloc(10000);
  }
  return 0;
}

static float user_func_add(struct expr_func *f, vec_expr_t *args, void *c) {
  (void)f, (void)c;
  float a = expr_eval(&vec_nth(args, 0));
  float b = expr_eval(&vec_nth(args, 1));
  return a + b;
}

static float user_func_next(struct expr_func *f, vec_expr_t *args, void *c) {
  (void)f, (void)c;
  float a = expr_eval(&vec_nth(args, 0));
  return a + 1;
}

static float user_func_print(struct expr_func *f, vec_expr_t *args, void *c) {
  (void)f, (void)c;
  int i;
  struct expr e;
  fprintf(stderr, ">> ");
  vec_foreach(args, e, i) { fprintf(stderr, "%f ", expr_eval(&e)); }
  fprintf(stderr, "\n");
  return 0;
}

static struct expr_func user_funcs[] = {
    {"nop", user_func_nop, user_func_nop_cleanup, sizeof(struct nop_context)},
    {"add", user_func_add, NULL, 0},
    {"next", user_func_next, NULL, 0},
    {"print", user_func_print, NULL, 0},
    {NULL, NULL, NULL, 0},
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

  char *p = (char *)malloc(strlen(s) + 1);
  strncpy(p, s, strlen(s) + 1);
  for (char *it = p; *it; it++) {
    if (*it == '\n') {
      *it = '\\';
    }
  }

  if ((isnan(result) && !isnan(expected)) ||
      fabs(result - expected) > 0.00001f) {
    printf("FAIL: %s: %f != %f\n", p, result, expected);
    status = 1;
  } else {
    printf("OK: %s == %f\n", p, expected);
  }
  expr_destroy(e, &vars);
  free(p);
}

static void test_expr_error(char *s) {
  struct expr_var_list vars = {0};
  struct expr *e = expr_create(s, strlen(s), &vars, user_funcs);
  if (e != NULL) {
    printf("FAIL: %s should return error\n", s);
    status = 1;
  }
  expr_destroy(e, &vars);
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
  test_expr("1/3*6/4*2", 1.0 / 3 * 6 / 4.0 * 2);
  test_expr("1*3/6*4/2", 1.0 * 3 / 6 * 4.0 / 2.0);
  test_expr("6/2+8*4/2", 19);
  test_expr("3/2", 3.0 / 2.0);
  test_expr("(3/2)|0", 3 / 2);
  test_expr("(3/0)", INFINITY);
  test_expr("(3/0)|0", INT_MAX);
  test_expr("(3%0)", NAN);
  test_expr("(3%0)|0", 0);
  test_expr("2**3", 8);
  test_expr("9**(1/2)", 3);
  test_expr("1+2<<3", (1 + 2) << 3);
  test_expr("2<<3", 2 << 3);
  test_expr("12>>2", 12 >> 2);
  test_expr("1<2", 1 < 2);
  test_expr("2<2", 2 < 2);
  test_expr("3<2", 3 < 2);
  test_expr("1>2", 1 > 2);
  test_expr("2>2", 2 > 2);
  test_expr("3>2", 3 > 2);
  test_expr("1==2", 1 == 2);
  test_expr("2==2", 2 == 2);
  test_expr("3==2", 3 == 2);
  test_expr("3.2==3.1", 3.2f == 3.1f);
  test_expr("1<=2", 1 <= 2);
  test_expr("2<=2", 2 <= 2);
  test_expr("3<=2", 3 <= 2);
  test_expr("1>=2", 1 >= 2);
  test_expr("2>=2", 2 >= 2);
  test_expr("3>=2", 3 >= 2);
  test_expr("123&42", 123 & 42);
  test_expr("123^42", 123 ^ 42);

  test_expr("1-1+1+1", 1 - 1 + 1 + 1);
  test_expr("2**2**3", 256); /* 2^(2^3), not (2^2)^3 */
}

static void test_logical() {
  test_expr("2&&3", 3);
  test_expr("0&&3", 0);
  test_expr("3&&0", 0);
  test_expr("2||3", 2);
  test_expr("0||3", 3);
  test_expr("2||0", 2);
  test_expr("0||0", 0);

  test_expr("1&&(3%0)", NAN);
  test_expr("(3%0)&&1", NAN);
  test_expr("1||(3%0)", 1);
  test_expr("(3%0)||1", 1);
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
  test_expr("x=5, x", 5);
  test_expr("x=5, y = 3, x+y", 8);
  test_expr("x=5, x=(x!=0)", 1);
  test_expr("x=5, x = x+1", 6);
}

static void test_funcs() {
  test_expr("add(1,2) + next(3)", 7);
  test_expr("add(1,next(2))", 4);
  test_expr("add(1,1+1) + add(2*2+1,2)", 10);
  test_expr("nop()", 0);
  test_expr("x=2,add(1, next(x))", 4);
  test_expr("$(zero), zero()", 0);
  test_expr("$(zero), zero(1, 2, 3)", 0);
  test_expr("$(one, 1), one()+one(1)+one(1, 2, 4)", 3);
  test_expr("$(number, 1), $(number, 2+3), number()", 5);
  test_expr("$(triw, ($1 * 256) & 255), triw(0.5, 2)", 128);
  test_expr("$(triw, ($1 * 256) & 255), triw(0.1)+triw(0.7)+triw(0.2)", 255);
}

static void test_name_collision() {
  test_expr("next=5", 5);
  test_expr("next=2,next(5)+next", 8);
}

static void test_fancy_variable_names() {
  test_expr("one=1", 1);
  test_expr("один=1", 1);
  test_expr("six=6, seven=7, six*seven", 42);
  test_expr("шість=6, сім=7, шість*сім", 42);
  test_expr("六=6, 七=7, 六*七", 42);
  test_expr("ταῦ=1.618, 3*ταῦ", 3 * 1.618);
  test_expr("$(ταῦ, 1.618), 3*ταῦ()", 3 * 1.618);
  test_expr("x#4=12, x#3=3, x#4+x#3", 15);
}

static void test_auto_comma() {
  test_expr("a=3\na+2\n", 5);
  test_expr("a=3\n\n\na+2\n", 5);
  test_expr("\n\na=\n3\n\n\na+2\n", 5);
  test_expr("\n\n3\n\n", 3);
  test_expr("\n\n\n\n", 0);
  test_expr("3\n\n\n\n", 3);
  test_expr("a=3\nb=4\na", 3);
  test_expr("(\n2+3\n)\n", 5);
  test_expr("a=\n3*\n(4+\n3)\na+\na\n", 42);
}

static void test_benchmark(const char *s) {
  struct timeval t;
  gettimeofday(&t, NULL);
  double start = t.tv_sec + t.tv_usec * 1e-6;
  struct expr_var_list vars = {0};
  struct expr *e = expr_create(s, strlen(s), &vars, user_funcs);
  if (e == NULL) {
    printf("FAIL: %s can't be compiled\n", s);
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
  printf("BENCH %40s:\t%f ns/op (%dM op/sec)\n", s, ns, (int)(1000 / ns));
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
  test_expr_error("nop=");
  test_expr_error("nop(");
  test_expr_error("unknownfunc()");
  test_expr_error("$(recurse, recurse()), recurse()");
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
  test_expr_error("$($())");
  test_expr_error("$(1)");
  test_expr_error("$()");
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

  test_name_collision();
  test_fancy_variable_names();

  test_auto_comma();

  test_bad_syntax();

  test_benchmark("5");
  test_benchmark("5+5+5+5+5+5+5+5+5+5");
  test_benchmark("5*5*5*5*5*5*5*5*5*5");
  test_benchmark("5,5,5,5,5,5,5,5,5,5");
  test_benchmark("((5+5)+(5+5))+((5+5)+(5+5))+(5+5)");
  test_benchmark("x=5");
  test_benchmark("x=5,x+x+x+x+x+x+x+x+x+x");
  test_benchmark("x=5,((x+x)+(x+x))+((x+x)+(x+x))+(x+x)");
  test_benchmark("a=1,b=2,c=3,d=4,e=5,f=6,g=7,h=8,i=9,j=10");
  test_benchmark("a=1,a=2,a=3,a=4,a=5,a=6,a=7,a=8,a=9,a=10");
  test_benchmark("$(sqr,$1*$1),5*5");
  test_benchmark("$(sqr,$1*$1),sqr(5)");
  test_benchmark("x=2+3*(x/(42+next(x))),x");
  test_benchmark("add(next(x), next(next(x)))");
  test_benchmark("a,b,c,d,e,d,e,f,g,h,i,j,k");
  test_benchmark("$(a,1),$(b,2),$(c,3),$(d,4),5");

  return status;
}
