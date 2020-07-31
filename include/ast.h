#ifndef __AST_H_
#define __AST_H_

#include "defs.h"
#include <stdlib.h>

ASTnode *new_ast_number(int value);

ASTnode *new_ast_binary_expr(AST_binop_type operator, ASTnode * lhs,
                             ASTnode * rhs);

ASTnode *new_ast_prototype(char *name, char **args, int arity);
ASTnode *new_ast_function(ASTnode *prototype, Vector *body);

ASTnode *new_ast_return_stmt(ASTnode *expr);
ASTnode *new_ast_let_stmt(ASTnode *var, ASTnode *expr);

ASTnode *new_ast_if_expr(ASTnode *cond, Vector *then_body, Vector *else_body);

ASTnode *new_ast_variable(char *name);

ASTnode *new_ast_call_expr(char *name, Vector *args);

void free_ast_node(ASTnode *node);

void print_statements_block(Vector *statements, int offset);
void print_functions(Vector *functions, int offset);
void print_ast(ASTnode *node, int offset);

#endif // __AST_H_
