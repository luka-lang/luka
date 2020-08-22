#ifndef __EXPR_H__
#define __EXPR_H__

#include "defs.h"
#include "logger.h"

extern int g_operator_precedence[];

int EXPR_get_op_precedence(t_token *token, t_logger *logger);

#endif // __EXPR_H__
