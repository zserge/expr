#ifndef EXPR_H
#define EXPR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h> /* for isspace */
#include <limits.h>
#include <math.h> /* for pow */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NAN
#define NAN (0.0 / 0.0)
#endif /* NAN */

#ifndef INFINITY
#define INFINITY (1.0 / 0.0)
#endif /* INFINITY */

#ifdef __GNUC__
#define EXPR_UNUSED __attribute__((used))
#else /* __GNUC__ */
#define EXPR_UNUSED
#endif /* __GNUC__ */

/*
 * Expression number type
 */
#ifndef expr_num_t
#define expr_num_t double
#endif /* expr_num_t */

/*
 * Memory management
 */
#ifndef expr_alloc
#define expr_alloc(sz) calloc(1, (sz))
#endif /* expr_alloc */
#ifndef expr_realloc
#define expr_realloc realloc
#endif /* expr_realloc */
#ifndef expr_free
#define expr_free free
#endif /* expr_free */
#ifndef expr_memcpy
#define expr_memcpy memcpy
#endif /* expr_memcpy */

/*
 * String handling
 */
#ifndef expr_strlen
#define expr_strlen strlen
#endif /* expr_strlen */
#ifndef expr_strncmp
#define expr_strncmp strncmp
#endif /* expr_strncmp */
#ifndef expr_snprintf
#define expr_snprintf snprintf
#endif /* expr_snprintf */

/*
 * Math
 */
#ifndef expr_pow
#define expr_pow pow
#endif /* expr_powf */
#ifndef expr_fmod
#define expr_fmod fmod
#endif /* expr_fmodf */

/*
 * Simple expandable vector implementation
 */
static int vec_expand(char **buf, int *length, int *cap, int memsz) {
  if (*length + 1 > *cap) {
    void *ptr;
    int n = (*cap == 0) ? 1 : *cap << 1;
    ptr = expr_realloc(*buf, n * memsz);
    if (ptr == NULL) {
      return -1; /* allocation failed */
    }
    *buf = (char *) ptr;
    *cap = n;
  }
  return 0;
}
#define vec(T)                                                                 \
  struct {                                                                     \
    T *buf;                                                                    \
    int len;                                                                   \
    int cap;                                                                   \
  }
#define vec_init()                                                             \
  { NULL, 0, 0 }
#define vec_len(v) ((v)->len)
#define vec_unpack(v)                                                          \
  (char **) &(v)->buf, &(v)->len, &(v)->cap, sizeof(*(v)->buf)
#define vec_push(v, val)                                                       \
  vec_expand(vec_unpack(v)) ? -1 : ((v)->buf[(v)->len++] = (val), 0)
#define vec_nth(v, i) (v)->buf[i]
#define vec_peek(v) (v)->buf[(v)->len - 1]
#define vec_pop(v) (v)->buf[--(v)->len]
#define vec_free(v)                                                            \
  (expr_free((v)->buf), (v)->buf = NULL, (v)->len = (v)->cap = 0)
#define vec_foreach(v, var, iter)                                              \
  if ((v)->len > 0)                                                            \
    for ((iter) = 0; (iter) < (v)->len && (((var) = (v)->buf[(iter)]), 1);     \
         ++(iter))

/*
 * Expression data types
 */
struct expr;
struct expr_func;

enum expr_type {
  OP_UNKNOWN,
  OP_UNARY_MINUS,
  OP_UNARY_LOGICAL_NOT,
  OP_UNARY_BITWISE_NOT,

  OP_POWER,
  OP_DIVIDE,
  OP_MULTIPLY,
  OP_REMAINDER,

  OP_PLUS,
  OP_MINUS,

  OP_SHL,
  OP_SHR,

  OP_LT,
  OP_LE,
  OP_GT,
  OP_GE,
  OP_EQ,
  OP_NE,

  OP_BITWISE_AND,
  OP_BITWISE_OR,
  OP_BITWISE_XOR,

  OP_LOGICAL_AND,
  OP_LOGICAL_OR,

  OP_ASSIGN,
  OP_COMMA,

  OP_CONST,
  OP_VAR,
  OP_FUNC,
};

static int prec[] = {0, 1, 1, 1, 2, 2, 2, 2, 3,  3,  4,  4, 5, 5,
                     5, 5, 5, 5, 6, 7, 8, 9, 10, 11, 12, 0, 0, 0};

typedef vec(struct expr) vec_expr_t;
typedef void (*exprfn_cleanup_t)(struct expr_func *f, void *context);
typedef expr_num_t (*exprfn_t)(struct expr_func *f, vec_expr_t *args,
                               void *context);

struct expr {
  enum expr_type type;
  union {
    struct {
      expr_num_t value;
    } num;
    struct {
      expr_num_t *value;
    } var;
    struct {
      vec_expr_t args;
    } op;
    struct {
      struct expr_func *f;
      vec_expr_t args;
      void *context;
    } func;
  } param;
};

#define expr_init()                                                            \
  { .type = (enum expr_type) 0 }

struct expr_string {
  const char *s;
  int n;
};
struct expr_arg {
  int oslen;
  int eslen;
  vec_expr_t args;
};

typedef vec(struct expr_string) vec_str_t;
typedef vec(struct expr_arg) vec_arg_t;

static int expr_is_unary(enum expr_type op) {
  return op == OP_UNARY_MINUS || op == OP_UNARY_LOGICAL_NOT ||
         op == OP_UNARY_BITWISE_NOT;
}

static int expr_is_binary(enum expr_type op) {
  return !expr_is_unary(op) && op != OP_CONST && op != OP_VAR &&
         op != OP_FUNC && op != OP_UNKNOWN;
}

static int expr_prec(enum expr_type a, enum expr_type b) {
  int left =
    expr_is_binary(a) && a != OP_ASSIGN && a != OP_POWER && a != OP_COMMA;
  return (left && prec[a] >= prec[b]) || (prec[a] > prec[b]);
}

#define isfirstvarchr(c)                                                       \
  (((unsigned char) c >= '@' && c != '^' && c != '|') || c == '$')
#define isvarchr(c)                                                            \
  (((unsigned char) c >= '@' && c != '^' && c != '|') || c == '$' ||           \
   c == '#' || (c >= '0' && c <= '9'))

static struct {
  const char *s;
  const enum expr_type op;
} OPS[] = {
  {"-u", OP_UNARY_MINUS},
  {"!u", OP_UNARY_LOGICAL_NOT},
  {"^u", OP_UNARY_BITWISE_NOT},
  {"**", OP_POWER},
  {"*", OP_MULTIPLY},
  {"/", OP_DIVIDE},
  {"%", OP_REMAINDER},
  {"+", OP_PLUS},
  {"-", OP_MINUS},
  {"<<", OP_SHL},
  {">>", OP_SHR},
  {"<", OP_LT},
  {"<=", OP_LE},
  {">", OP_GT},
  {">=", OP_GE},
  {"==", OP_EQ},
  {"!=", OP_NE},
  {"&", OP_BITWISE_AND},
  {"|", OP_BITWISE_OR},
  {"^", OP_BITWISE_XOR},
  {"&&", OP_LOGICAL_AND},
  {"||", OP_LOGICAL_OR},
  {"=", OP_ASSIGN},
  {",", OP_COMMA},

  /* These are used by lexer and must be ignored by parser, so we put
       them at the end */
  {"-", OP_UNARY_MINUS},
  {"!", OP_UNARY_LOGICAL_NOT},
  {"^", OP_UNARY_BITWISE_NOT},
};

static enum expr_type expr_op(const char *s, size_t len, int unary) {
  unsigned int i;
  for (i = 0; i < sizeof(OPS) / sizeof(OPS[0]); i++) {
    if (expr_strlen(OPS[i].s) == len && expr_strncmp(OPS[i].s, s, len) == 0 &&
        (unary == -1 || expr_is_unary(OPS[i].op) == unary)) {
      return OPS[i].op;
    }
  }
  return OP_UNKNOWN;
}

static expr_num_t expr_parse_number(const char *s, size_t len) {
  expr_num_t num = 0;
  unsigned int frac = 0;
  unsigned int digits = 0;
  unsigned int i;
  for (i = 0; i < len; i++) {
    if (s[i] == '.' && frac == 0) {
      frac++;
      continue;
    }
    if (isdigit(s[i])) {
      digits++;
      if (frac > 0) {
        frac++;
      }
      num = num * 10 + (s[i] - '0');
    } else {
      return NAN;
    }
  }
  while (frac > 1) {
    num = num / 10;
    frac--;
  }
  return (digits > 0 ? num : NAN);
}

/*
 * Functions
 */
struct expr_func {
  const char *name;
  exprfn_t f;
  exprfn_cleanup_t cleanup;
  size_t ctxsz;
};

static struct expr_func *expr_func(struct expr_func *funcs, const char *s,
                                   size_t len) {
  struct expr_func *f;
  for (f = funcs; f->name; f++) {
    if (expr_strlen(f->name) == len && expr_strncmp(f->name, s, len) == 0) {
      return f;
    }
  }
  return NULL;
}

/*
 * Variables
 */
struct expr_var {
  expr_num_t value;
  struct expr_var *next;
  char name[];
};

struct expr_var_list {
  struct expr_var *head;
};

static struct expr_var *expr_var(struct expr_var_list *vars, const char *s,
                                 size_t len) {
  struct expr_var *v = NULL;
  if (len == 0 || !isfirstvarchr(*s)) {
    return NULL;
  }
  for (v = vars->head; v; v = v->next) {
    if (expr_strlen(v->name) == len && expr_strncmp(v->name, s, len) == 0) {
      return v;
    }
  }
  v = expr_alloc(sizeof(struct expr_var) + len + 1);
  if (v == NULL) {
    return NULL; /* allocation failed */
  }
  v->next = vars->head;
  v->value = 0;
  expr_memcpy(v->name, s, len);
  v->name[len] = '\0';
  vars->head = v;
  return v;
}

static int to_int(expr_num_t x) {
  if (isnan(x)) {
    return 0;
  } else if (isinf(x) != 0) {
    return INT_MAX * isinf(x);
  } else {
    return (int) x;
  }
}

static expr_num_t expr_eval(struct expr *e) {
  expr_num_t n;
  switch (e->type) {
    case OP_UNARY_MINUS:
      return -(expr_eval(&e->param.op.args.buf[0]));
    case OP_UNARY_LOGICAL_NOT:
      return !(expr_eval(&e->param.op.args.buf[0]));
    case OP_UNARY_BITWISE_NOT:
      return ~(to_int(expr_eval(&e->param.op.args.buf[0])));
    case OP_POWER:
      return expr_pow(expr_eval(&e->param.op.args.buf[0]),
                      expr_eval(&e->param.op.args.buf[1]));
    case OP_MULTIPLY:
      return expr_eval(&e->param.op.args.buf[0]) *
             expr_eval(&e->param.op.args.buf[1]);
    case OP_DIVIDE:
      return expr_eval(&e->param.op.args.buf[0]) /
             expr_eval(&e->param.op.args.buf[1]);
    case OP_REMAINDER:
      return expr_fmod(expr_eval(&e->param.op.args.buf[0]),
                       expr_eval(&e->param.op.args.buf[1]));
    case OP_PLUS:
      return expr_eval(&e->param.op.args.buf[0]) +
             expr_eval(&e->param.op.args.buf[1]);
    case OP_MINUS:
      return expr_eval(&e->param.op.args.buf[0]) -
             expr_eval(&e->param.op.args.buf[1]);
    case OP_SHL:
      return to_int(expr_eval(&e->param.op.args.buf[0]))
             << to_int(expr_eval(&e->param.op.args.buf[1]));
    case OP_SHR:
      return to_int(expr_eval(&e->param.op.args.buf[0])) >>
             to_int(expr_eval(&e->param.op.args.buf[1]));
    case OP_LT:
      return expr_eval(&e->param.op.args.buf[0]) <
             expr_eval(&e->param.op.args.buf[1]);
    case OP_LE:
      return expr_eval(&e->param.op.args.buf[0]) <=
             expr_eval(&e->param.op.args.buf[1]);
    case OP_GT:
      return expr_eval(&e->param.op.args.buf[0]) >
             expr_eval(&e->param.op.args.buf[1]);
    case OP_GE:
      return expr_eval(&e->param.op.args.buf[0]) >=
             expr_eval(&e->param.op.args.buf[1]);
    case OP_EQ:
      return expr_eval(&e->param.op.args.buf[0]) ==
             expr_eval(&e->param.op.args.buf[1]);
    case OP_NE:
      return expr_eval(&e->param.op.args.buf[0]) !=
             expr_eval(&e->param.op.args.buf[1]);
    case OP_BITWISE_AND:
      return to_int(expr_eval(&e->param.op.args.buf[0])) &
             to_int(expr_eval(&e->param.op.args.buf[1]));
    case OP_BITWISE_OR:
      return to_int(expr_eval(&e->param.op.args.buf[0])) |
             to_int(expr_eval(&e->param.op.args.buf[1]));
    case OP_BITWISE_XOR:
      return to_int(expr_eval(&e->param.op.args.buf[0])) ^
             to_int(expr_eval(&e->param.op.args.buf[1]));
    case OP_LOGICAL_AND:
      n = expr_eval(&e->param.op.args.buf[0]);
      if (n != 0) {
        n = expr_eval(&e->param.op.args.buf[1]);
        if (n != 0) {
          return n;
        }
      }
      return 0;
    case OP_LOGICAL_OR:
      n = expr_eval(&e->param.op.args.buf[0]);
      if (n != 0 && !isnan(n)) {
        return n;
      } else {
        n = expr_eval(&e->param.op.args.buf[1]);
        if (n != 0) {
          return n;
        }
      }
      return 0;
    case OP_ASSIGN:
      n = expr_eval(&e->param.op.args.buf[1]);
      if (vec_nth(&e->param.op.args, 0).type == OP_VAR) {
        *e->param.op.args.buf[0].param.var.value = n;
      }
      return n;
    case OP_COMMA:
      expr_eval(&e->param.op.args.buf[0]);
      return expr_eval(&e->param.op.args.buf[1]);
    case OP_CONST:
      return e->param.num.value;
    case OP_VAR:
      return *e->param.var.value;
    case OP_FUNC:
      return e->param.func.f->f(e->param.func.f, &e->param.func.args,
                                e->param.func.context);
    default:
      return NAN;
  }
}

#define EXPR_TOP (1 << 0)
#define EXPR_TOPEN (1 << 1)
#define EXPR_TCLOSE (1 << 2)
#define EXPR_TNUMBER (1 << 3)
#define EXPR_TWORD (1 << 4)
#define EXPR_TDEFAULT (EXPR_TOPEN | EXPR_TNUMBER | EXPR_TWORD)

#define EXPR_UNARY (1 << 5)
#define EXPR_COMMA (1 << 6)

#define EXPR_ERR_UNKNOWN (0)
#define EXPR_ERR_UNEXPECTED_NUMBER (-1)
#define EXPR_ERR_UNEXPECTED_WORD (-2)
#define EXPR_ERR_UNEXPECTED_PARENS (-3)
#define EXPR_ERR_MISS_EXPECTED_OPERAND (-4)
#define EXPR_ERR_UNKNOWN_OPERATOR (-5)
#define EXPR_ERR_INVALID_FUNC_NAME (-6)
#define EXPR_ERR_BAD_CALL (-7)
#define EXPR_ERR_BAD_PARENS (-8)
#define EXPR_ERR_TOO_FEW_FUNC_ARGS (-9)
#define EXPR_ERR_FIRST_ARG_IS_NOT_VAR (-10)
#define EXPR_ERR_ALLOCATION_FAILED (-11)
#define EXPR_ERR_BAD_VARIABLE_NAME (-12)
#define EXPR_ERR_BAD_ASSIGNMENT (-13)

static int expr_next_token(const char *s, size_t len, int *flags) {
  unsigned int i = 0;
  char c;
  if (len == 0) {
    return 0;
  }
  c = s[0];
  if (c == '#') {
    for (; i < len && s[i] != '\n'; i++)
      ;
    return i;
  } else if (c == '\n') {
    for (; i < len && isspace(s[i]); i++)
      ;
    if (*flags & EXPR_TOP) {
      if (i == len || s[i] == ')') {
        *flags = *flags & (~EXPR_COMMA);
      } else {
        *flags = EXPR_TNUMBER | EXPR_TWORD | EXPR_TOPEN | EXPR_COMMA;
      }
    }
    return i;
  } else if (isspace(c)) {
    while (i < len && isspace(s[i]) && s[i] != '\n') {
      i++;
    }
    return i;
  } else if (isdigit(c)) {
    if ((*flags & EXPR_TNUMBER) == 0) {
      return EXPR_ERR_UNEXPECTED_NUMBER; /* unexpected number */
    }
    *flags = EXPR_TOP | EXPR_TCLOSE;
    while ((c == '.' || isdigit(c)) && i < len) {
      i++;
      c = s[i];
    }
    return i;
  } else if (isfirstvarchr(c)) {
    if ((*flags & EXPR_TWORD) == 0) {
      return EXPR_ERR_UNEXPECTED_WORD; /* unexpected word */
    }
    *flags = EXPR_TOP | EXPR_TOPEN | EXPR_TCLOSE;
    while ((isvarchr(c)) && i < len) {
      i++;
      c = s[i];
    }
    return i;
  } else if (c == '(' || c == ')') {
    if (c == '(' && (*flags & EXPR_TOPEN) != 0) {
      *flags = EXPR_TNUMBER | EXPR_TWORD | EXPR_TOPEN | EXPR_TCLOSE;
    } else if (c == ')' && (*flags & EXPR_TCLOSE) != 0) {
      *flags = EXPR_TOP | EXPR_TCLOSE;
    } else {
      return EXPR_ERR_UNEXPECTED_PARENS; /* unexpected parenthesis */
    }
    return 1;
  } else {
    if ((*flags & EXPR_TOP) == 0) {
      if (expr_op(&c, 1, 1) == OP_UNKNOWN) {
        return EXPR_ERR_MISS_EXPECTED_OPERAND; /* missing expected operand */
      }
      *flags = EXPR_TNUMBER | EXPR_TWORD | EXPR_TOPEN | EXPR_UNARY;
      return 1;
    } else {
      int found = 0;
      while (!isvarchr(c) && !isspace(c) && c != '(' && c != ')' && i < len) {
        if (expr_op(s, i + 1, 0) != OP_UNKNOWN) {
          found = 1;
        } else if (found) {
          break;
        }
        i++;
        c = s[i];
      }
      if (!found) {
        return EXPR_ERR_UNKNOWN_OPERATOR; /* unknown operator */
      }
      *flags = EXPR_TNUMBER | EXPR_TWORD | EXPR_TOPEN;
      return i;
    }
  }
}

#define EXPR_PAREN_ALLOWED 0
#define EXPR_PAREN_EXPECTED 1
#define EXPR_PAREN_FORBIDDEN 2

static int expr_bind(const char *s, size_t len, vec_expr_t *es) {
  enum expr_type op = expr_op(s, len, -1);
  if (op == OP_UNKNOWN) {
    return -1;
  }

  if (expr_is_unary(op)) {
    if (vec_len(es) < 1) {
      return -1;
    }
    {
      struct expr arg = vec_pop(es);
      struct expr unary = expr_init();
      unary.type = op;
      vec_push(&unary.param.op.args, arg);
      vec_push(es, unary);
    }
  } else {
    if (vec_len(es) < 2) {
      return -1;
    }
    {
      struct expr b = vec_pop(es);
      struct expr a = vec_pop(es);
      struct expr binary = expr_init();
      binary.type = op;
      if (op == OP_ASSIGN && a.type != OP_VAR) {
        return -1; /* Bad assignment */
      }
      vec_push(&binary.param.op.args, a);
      vec_push(&binary.param.op.args, b);
      vec_push(es, binary);
    }
  }
  return 0;
}

static struct expr expr_const(expr_num_t value) {
  struct expr e = expr_init();
  e.type = OP_CONST;
  e.param.num.value = value;
  return e;
}

static struct expr expr_varref(struct expr_var *v) {
  struct expr e = expr_init();
  e.type = OP_VAR;
  e.param.var.value = &v->value;
  return e;
}

static struct expr expr_binary(enum expr_type type, struct expr a,
                               struct expr b) {
  struct expr e = expr_init();
  e.type = type;
  vec_push(&e.param.op.args, a);
  vec_push(&e.param.op.args, b);
  return e;
}

static inline void expr_copy(struct expr *dst, struct expr *src) {
  int i;
  struct expr arg;
  dst->type = src->type;
  if (src->type == OP_FUNC) {
    dst->param.func.f = src->param.func.f;
    vec_foreach(&src->param.func.args, arg, i) {
      struct expr tmp = expr_init();
      expr_copy(&tmp, &arg);
      vec_push(&dst->param.func.args, tmp);
    }
    if (src->param.func.f->ctxsz > 0) {
      dst->param.func.context = expr_alloc(src->param.func.f->ctxsz);
    }
  } else if (src->type == OP_CONST) {
    dst->param.num.value = src->param.num.value;
  } else if (src->type == OP_VAR) {
    dst->param.var.value = src->param.var.value;
  } else {
    vec_foreach(&src->param.op.args, arg, i) {
      struct expr tmp = expr_init();
      expr_copy(&tmp, &arg);
      vec_push(&dst->param.op.args, tmp);
    }
  }
}

static void expr_destroy_args(struct expr *e);

static struct expr *expr_create2(const char *s, size_t len,
                                 struct expr_var_list *vars,
                                 struct expr_func *funcs, int *near,
                                 int *error) {
  expr_num_t num;
  struct expr_var *v;
  const char *id = NULL;
  size_t idn = 0;

  struct expr *result = NULL;

  vec_expr_t es = vec_init();
  vec_str_t os = vec_init();
  vec_arg_t as = vec_init();

  struct macro {
    char *name;
    vec_expr_t body;
  };
  vec(struct macro) macros = vec_init();

  int flags = EXPR_TDEFAULT;
  int paren = EXPR_PAREN_ALLOWED;
  *near = 0;
  *error = EXPR_ERR_UNKNOWN;
  for (;;) {
    int n = expr_next_token(s, len, &flags);
    const char *tok;
    int paren_next;
    if (n == 0) {
      break;
    } else if (n < 0) {
      *error = n;
      goto cleanup;
    }
    *near += n;
    tok = s;
    s = s + n;
    len = len - n;
    if (*tok == '#') {
      continue;
    }
    if (flags & EXPR_UNARY) {
      if (n == 1) {
        switch (*tok) {
          case '-':
            tok = "-u";
            break;
          case '^':
            tok = "^u";
            break;
          case '!':
            tok = "!u";
            break;
          default:
            goto cleanup;
        }
        n = 2;
      }
    }
    if (*tok == '\n' && (flags & EXPR_COMMA)) {
      flags = flags & (~EXPR_COMMA);
      n = 1;
      tok = ",";
    }
    if (isspace(*tok)) {
      continue;
    }
    paren_next = EXPR_PAREN_ALLOWED;

    if (idn > 0) {
      if (n == 1 && *tok == '(') {
        int i;
        int has_macro = 0;
        struct macro m;
        vec_foreach(&macros, m, i) {
          if (expr_strlen(m.name) == idn &&
              expr_strncmp(m.name, id, idn) == 0) {
            has_macro = 1;
            break;
          }
        }
        if ((idn == 1 && id[0] == '$') || has_macro ||
            expr_func(funcs, id, idn) != NULL) {
          struct expr_string str = {id, (int) idn};
          vec_push(&os, str);
          paren = EXPR_PAREN_EXPECTED;
        } else {
          *error = EXPR_ERR_INVALID_FUNC_NAME;
          goto cleanup; /* invalid function name */
        }
      } else if ((v = expr_var(vars, id, idn)) != NULL) {
        vec_push(&es, expr_varref(v));
        paren = EXPR_PAREN_FORBIDDEN;
      }
      id = NULL;
      idn = 0;
    }

    if (n == 1 && *tok == '(') {
      if (paren == EXPR_PAREN_EXPECTED) {
        struct expr_string str = {"{", 1};
        vec_push(&os, str);
        {
          struct expr_arg arg = {vec_len(&os), vec_len(&es), vec_init()};
          vec_push(&as, arg);
        }
      } else if (paren == EXPR_PAREN_ALLOWED) {
        struct expr_string str = {"(", 1};
        vec_push(&os, str);
      } else {
        *error = EXPR_ERR_BAD_CALL;
        goto cleanup; /* bad call */
      }
    } else if (paren == EXPR_PAREN_EXPECTED) {
      *error = EXPR_ERR_BAD_CALL;
      goto cleanup; /* bad call */
    } else if (n == 1 && *tok == ')') {
      int minlen = (vec_len(&as) > 0 ? vec_peek(&as).oslen : 0);
      while (vec_len(&os) > minlen && *vec_peek(&os).s != '(' &&
             *vec_peek(&os).s != '{') {
        struct expr_string str = vec_pop(&os);
        if (expr_bind(str.s, str.n, &es) == -1) {
          goto cleanup;
        }
      }
      if (vec_len(&os) == 0) {
        *error = EXPR_ERR_BAD_PARENS;
        goto cleanup; /* bad parens */
      }
      {
        struct expr_string str = vec_pop(&os);
        if (str.n == 1 && *str.s == '{') {
          struct expr_arg arg;
          str = vec_pop(&os);
          arg = vec_pop(&as);
          if (vec_len(&es) > arg.eslen) {
            vec_push(&arg.args, vec_pop(&es));
          }
          if (str.n == 1 && str.s[0] == '$') {
            struct expr *u;
            struct expr_var *v;
            if (vec_len(&arg.args) < 1) {
              vec_free(&arg.args);
              *error = EXPR_ERR_TOO_FEW_FUNC_ARGS;
              goto cleanup; /* too few arguments for $() function */
            }
            u = &vec_nth(&arg.args, 0);
            if (u->type != OP_VAR) {
              vec_free(&arg.args);
              *error = EXPR_ERR_FIRST_ARG_IS_NOT_VAR;
              goto cleanup; /* first argument is not a variable */
            }
            for (v = vars->head; v; v = v->next) {
              if (&v->value == u->param.var.value) {
                struct macro m = {v->name, arg.args};
                vec_push(&macros, m);
                break;
              }
            }
            vec_push(&es, expr_const(0));
          } else {
            int i = 0;
            int found = -1;
            struct macro m;
            vec_foreach(&macros, m, i) {
              if (expr_strlen(m.name) == (size_t) str.n &&
                  expr_strncmp(m.name, str.s, str.n) == 0) {
                found = i;
              }
            }
            if (found != -1) {
              struct expr root;
              struct expr *p;
              int j;
              m = vec_nth(&macros, found);
              root = expr_const(0);
              p = &root;
              /* Assign macro parameters */
              for (j = 0; j < vec_len(&arg.args); j++) {
                char varname[4];
                expr_snprintf(varname, sizeof(varname) - 1, "$%d", (j + 1));
                {
                  struct expr_var *v =
                    expr_var(vars, varname, expr_strlen(varname));
                  struct expr ev = expr_varref(v);
                  struct expr assign =
                    expr_binary(OP_ASSIGN, ev, vec_nth(&arg.args, j));
                  *p = expr_binary(OP_COMMA, assign, expr_const(0));
                }
                p = &vec_nth(&p->param.op.args, 1);
              }
              /* Expand macro body */
              for (j = 1; j < vec_len(&m.body); j++) {
                if (j < vec_len(&m.body) - 1) {
                  *p = expr_binary(OP_COMMA, expr_const(0), expr_const(0));
                  expr_copy(&vec_nth(&p->param.op.args, 0),
                            &vec_nth(&m.body, j));
                } else {
                  expr_copy(p, &vec_nth(&m.body, j));
                }
                p = &vec_nth(&p->param.op.args, 1);
              }
              vec_push(&es, root);
              vec_free(&arg.args);
            } else {
              struct expr_func *f = expr_func(funcs, str.s, str.n);
              struct expr bound_func = expr_init();
              bound_func.type = OP_FUNC;
              bound_func.param.func.f = f;
              bound_func.param.func.args = arg.args;
              if (f->ctxsz > 0) {
                void *p = expr_alloc(f->ctxsz);
                if (p == NULL) {
                  *error = EXPR_ERR_ALLOCATION_FAILED;
                  goto cleanup; /* allocation failed */
                }
                bound_func.param.func.context = p;
              }
              vec_push(&es, bound_func);
            }
          }
        }
      }
      paren_next = EXPR_PAREN_FORBIDDEN;
    } else if (!isnan(num = expr_parse_number(tok, n))) {
      vec_push(&es, expr_const(num));
      paren_next = EXPR_PAREN_FORBIDDEN;
    } else if (expr_op(tok, n, -1) != OP_UNKNOWN) {
      enum expr_type op = expr_op(tok, n, -1);
      struct expr_string o2 = {NULL, 0};
      if (vec_len(&os) > 0) {
        o2 = vec_peek(&os);
      }
      for (;;) {
        enum expr_type type2;
        if (n == 1 && *tok == ',' && vec_len(&os) > 0) {
          struct expr_string str = vec_peek(&os);
          if (str.n == 1 && *str.s == '{') {
            struct expr e = vec_pop(&es);
            vec_push(&vec_peek(&as).args, e);
            break;
          }
        }
        type2 = expr_op(o2.s, o2.n, -1);
        if (!(type2 != OP_UNKNOWN && expr_prec(op, type2))) {
          struct expr_string str = {tok, n};
          vec_push(&os, str);
          break;
        }

        if (expr_bind(o2.s, o2.n, &es) == -1) {
          goto cleanup;
        }
        (void) vec_pop(&os);
        if (vec_len(&os) > 0) {
          o2 = vec_peek(&os);
        } else {
          o2.n = 0;
        }
      }
    } else {
      if (/*n > 0 &&*/ !isdigit(*tok)) {
        /* Valid identifier, a variable or a function */
        id = tok;
        idn = n;
      } else {
        *error = EXPR_ERR_BAD_VARIABLE_NAME;
        goto cleanup; /* bad variable name, e.g. '2.3.4' or '4ever' */
      }
    }
    paren = paren_next;
  }

  if (idn > 0) {
    vec_push(&es, expr_varref(expr_var(vars, id, idn)));
  }

  while (vec_len(&os) > 0) {
    struct expr_string rest = vec_pop(&os);
    if (rest.n == 1 && (*rest.s == '(' || *rest.s == ')')) {
      *error = EXPR_ERR_BAD_PARENS;
      goto cleanup; /* bad paren */
    }
    if (expr_bind(rest.s, rest.n, &es) == -1) {
      *error = (*rest.s == '=') ? EXPR_ERR_BAD_ASSIGNMENT : EXPR_ERR_BAD_PARENS;
      goto cleanup;
    }
  }

  result = (struct expr *) expr_alloc(sizeof(struct expr));
  if (result != NULL) {
    if (vec_len(&es) == 0) {
      result->type = OP_CONST;
    } else {
      *result = vec_pop(&es);
    }
  }

cleanup : {
  int i, j;
  struct macro m;
  struct expr e;
  struct expr_arg a;
  vec_foreach(&macros, m, i) {
    struct expr e;
    vec_foreach(&m.body, e, j) {
      expr_destroy_args(&e);
    }
    vec_free(&m.body);
  }
  vec_free(&macros);

  vec_foreach(&es, e, i) {
    expr_destroy_args(&e);
  }
  vec_free(&es);

  vec_foreach(&as, a, i) {
    vec_foreach(&a.args, e, j) {
      expr_destroy_args(&e);
    }
    vec_free(&a.args);
  }
}
  vec_free(&as);

  /*vec_foreach(&os, o, i) {vec_free(&m.body);}*/
  vec_free(&os);

  if (*near == 0) {
    *near = 1;
  }
  return result;
}

static struct expr *expr_create(const char *s, size_t len,
                                struct expr_var_list *vars,
                                struct expr_func *funcs) {
  int near;
  int error;
  return expr_create2(s, len, vars, funcs, &near, &error);
}

static void expr_destroy_args(struct expr *e) {
  int i;
  struct expr arg;
  if (e->type == OP_FUNC) {
    vec_foreach(&e->param.func.args, arg, i) {
      expr_destroy_args(&arg);
    }
    vec_free(&e->param.func.args);
    if (e->param.func.context != NULL) {
      if (e->param.func.f->cleanup != NULL) {
        e->param.func.f->cleanup(e->param.func.f, e->param.func.context);
      }
      expr_free(e->param.func.context);
    }
  } else if (e->type != OP_CONST && e->type != OP_VAR) {
    vec_foreach(&e->param.op.args, arg, i) {
      expr_destroy_args(&arg);
    }
    vec_free(&e->param.op.args);
  }
}

static void expr_destroy(struct expr *e, struct expr_var_list *vars) {
  if (e != NULL) {
    expr_destroy_args(e);
    expr_free(e);
  }
  if (vars != NULL) {
    struct expr_var *v;
    for (v = vars->head; v;) {
      struct expr_var *next = v->next;
      expr_free(v);
      v = next;
    }
  }
}

EXPR_UNUSED static expr_num_t expr_calc2(const char *s, size_t len) {
  struct expr_var_list vars = {0};
  struct expr_func funcs[] = {{NULL, NULL, NULL, 0}};
  struct expr *e;
  expr_num_t r;
  e = expr_create(s, len, &vars, funcs);
  if (e == NULL) {
    return NAN;
  }
  r = expr_eval(e);
  expr_destroy(e, &vars);
  return r;
}

EXPR_UNUSED static expr_num_t expr_calc(const char *s) {
  return expr_calc2(s, expr_strlen(s));
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* EXPR_H */
