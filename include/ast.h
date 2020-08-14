#ifndef __AST_H__
#define __AST_H__

#include "defs.h"
#include <stdlib.h>

t_ast_node *new_ast_number(int value);
t_ast_node *new_ast_string(char *content);

t_ast_node *new_ast_binary_expr(t_ast_binop_type operator, t_ast_node * lhs,
                             t_ast_node * rhs);

t_ast_node *new_ast_prototype(char *name, char **args, t_type *types, int arity,
                           t_type return_type);
t_ast_node *new_ast_function(t_ast_node *prototype, t_vector *body);

t_ast_node *new_ast_return_stmt(t_ast_node *expr);
t_ast_node *new_ast_let_stmt(t_ast_node *var, t_ast_node *expr);

t_ast_node *new_ast_if_expr(t_ast_node *cond, t_vector *then_body, t_vector *else_body);

t_ast_node *new_ast_variable(char *name, t_type type);

t_ast_node *new_ast_call_expr(char *name, t_vector *args);

t_ast_node *new_ast_expression_stmt(t_ast_node *expr);

void free_ast_node(t_ast_node *node);

void print_statements_block(t_vector *statements, int offset);
void print_functions(t_vector *functions, int offset);
void print_ast(t_ast_node *node, int offset);

#endif // __AST_H__
