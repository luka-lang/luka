#include "../include/expr.h"
#include <stdio.h>
#include <stdlib.h>

// PLUS MINUS MULT DIV INTLIT EOF
int OpPrec[] = {10, 10, 20, 20, 0, 0};

int op_precedence(token_t *token) {
  int prec = OpPrec[token->type - T_PLUS];
  if (prec == 0) {
    fprintf(stderr, "op_precedence: Syntax error at %ld:%ld, token %d\n",
            token->line, token->offset, token->type);
    exit(1);
  }
  return prec;
}
