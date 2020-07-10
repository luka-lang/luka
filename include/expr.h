#ifndef __EXPR_H_
#define __EXPR_H_

#include "defs.h"

extern int OpPrec[];

int op_precedence(token_t *token);

#endif // __EXPR_H_
