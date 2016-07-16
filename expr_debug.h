#ifndef EXPR_DEBUG_H
#define EXPR_DEBUG_H

#include <stdio.h>

static void expr_print(struct expr *e) {
  switch (e->type) {
  case OP_UNKNOWN:
    break;
  case OP_UNARY_MINUS:
    printf("-(");
    expr_print(&e->param.op.args.buf[0]);
    printf(")");
    break;
  case OP_UNARY_LOGICAL_NOT:
    printf("!(");
    expr_print(&e->param.op.args.buf[0]);
    printf(")");
    break;
  case OP_UNARY_BITWISE_NOT:
    printf("^(");
    expr_print(&e->param.op.args.buf[0]);
    printf(")");
    break;
  case OP_POWER:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf("**");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_MULTIPLY:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf("*");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_DIVIDE:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf("/");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_REMAINDER:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf("%%");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_PLUS:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf("+");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_MINUS:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf("-");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_SHL:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf("<<");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_SHR:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf(">>");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_LT:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf("<");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_LE:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf("<=");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_GT:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf(">");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_GE:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf(">=");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_EQ:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf("==");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_NE:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf("!=");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_BITWISE_AND:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf("&");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_BITWISE_OR:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf("|");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_BITWISE_XOR:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf("^");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_LOGICAL_AND:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf("&&");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_LOGICAL_OR:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf("||");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_ASSIGN:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf(":=");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_COMMA:
    printf("(");
    expr_print(&e->param.op.args.buf[0]);
    printf(",");
    expr_print(&e->param.op.args.buf[1]);
    printf(")");
    break;
  case OP_CONST:
    printf("%.2f", e->param.num.value);
    break;
  case OP_VAR:
    printf("[%.2f@%p]", *e->param.var.value, (void *)e->param.var.value);
    break;
  case OP_FUNC:
    printf("func(todo)");
    break;
  }
}

static void expr_println(struct expr *e) {
  expr_print(e);
  printf("\n");
}

#endif /* EXPR_DEBUG_H */
