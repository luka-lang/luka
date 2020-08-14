#ifndef __EXPR_H__
#define __EXPR_H__

#include "defs.h"

extern int g_operator_precedence[];

int op_precedence(t_token *token);

#endif // __EXPR_H__
