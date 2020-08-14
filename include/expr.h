#ifndef __EXPR_H__
#define __EXPR_H__

#include "defs.h"

extern int OpPrec[];

int op_precedence(token_t *token);

#endif // __EXPR_H__
