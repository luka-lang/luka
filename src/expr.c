#include "expr.h"
#include <stdio.h>
#include <stdlib.h>

// PLUS MINUS MULT DIV INTLIT EQUALS BANG OPEN_ANG CLOSE_ANG EQEQ NEQ LEQ GEQ
// EOF
int g_operator_precedence[] = {
    12, /* PLUS */
    12, /* MINUS */
    13, /* MULT */
    13, /* DIV */
    0, /*  INTLIT */
    2, /*  EQUALS */
    14, /* BANG */
    9, /*  OPEN_ANG */
    9, /*  CLOSE_ANG */
    8, /*  EQEQ */
    8, /*  NEQ */
    9, /*  LEQ */
    9, /*  GEQ */
    14, /* AMPERCENT */
    0, /*  COLON */
    0, /*  DOT */
    0, /*  THREE_DOTS */
    0 /*   EOF */
};

int EXPR_get_op_precedence(t_token *token, t_logger *logger)
{
    int prec = g_operator_precedence[token->type - T_PLUS];
    if (prec == 0)
    {
        (void) LOGGER_log(logger,
                          L_ERROR,
                          "expr_get_op_precedence: Syntax error at %ld:%ld, token %d\n",
                          token->line,
                          token->offset,
                          token->type);
        (void) exit(1);
    }
    return prec;
}
