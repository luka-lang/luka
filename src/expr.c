#include "expr.h"
#include <stdio.h>
#include <stdlib.h>

// PLUS MINUS MULT DIV INTLIT EQUALS BANG OPEN_ANG CLOSE_ANG EQEQ NEQ LEQ GEQ
// EOF
int g_operator_precedence[] = {12, 12, 13, 13, 0, 2, 15, 9, 9, 8, 8, 9, 9, 0};

int op_precedence(t_token *token)
{
    int prec = g_operator_precedence[token->type - T_PLUS];
    if (prec == 0)
    {
        fprintf(stderr, "op_precedence: Syntax error at %ld:%ld, token %d\n",
                token->line, token->offset, token->type);
        exit(1);
    }
    return prec;
}
