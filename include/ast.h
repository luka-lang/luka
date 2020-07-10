#ifndef __AST_H_
#define __AST_H_

#include "defs.h"
#include <stdlib.h>

ASTnode *new_ast_number(int value);

ASTnode *new_ast_binary_expr(AST_binop_type operator, ASTnode * lhs,
                             ASTnode * rhs);

ASTnode *new_ast_prototype(char *name, char **args, int arity);
ASTnode *new_ast_function(ASTnode *prototype, ASTnode *body);

void free_ast_node(ASTnode *node);

#endif // __AST_H_
