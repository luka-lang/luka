#ifndef __AST_H__
#define __AST_H__

#include "defs.h"
#include "logger.h"

#include <stdlib.h>

t_ast_node *AST_new_number(t_type *type, void *value);
t_ast_node *AST_new_string(char *content);

t_ast_node *AST_new_unary_expr(t_ast_unop_type operator, t_ast_node *rhs);
t_ast_node *AST_new_binary_expr(t_ast_binop_type operator, t_ast_node *lhs,
                                t_ast_node *rhs);

t_ast_node *AST_new_prototype(char *name, char **args, t_type **types, int arity,
                              t_type *return_type, bool vararg);
t_ast_node *AST_new_function(t_ast_node *prototype, t_vector *body);

t_ast_node *AST_new_return_stmt(t_ast_node *expr);
t_ast_node *AST_new_let_stmt(t_ast_node *var, t_ast_node *expr);
t_ast_node *AST_new_assignment_expr(t_ast_node *lhs, t_ast_node *rhs);

t_ast_node *AST_new_if_expr(t_ast_node *cond, t_vector *then_body, t_vector *else_body);
t_ast_node *AST_new_while_expr(t_ast_node *cond, t_vector *body);
t_ast_node *AST_new_cast_expr(t_ast_node *expr, t_type *type);

t_ast_node *AST_new_variable(char *name, t_type *type, bool mutable);

t_ast_node *AST_new_call_expr(char *name, t_vector *args);

t_ast_node *AST_new_expression_stmt(t_ast_node *expr);

t_ast_node *AST_new_break_stmt();

t_ast_node *AST_new_struct_definition(char *name, t_vector *struct_fields);
t_ast_node *AST_new_struct_value(char *name, t_vector *value_fields);

t_ast_node *AST_new_enum_definition(char *name, t_vector *enum_fields);

t_ast_node *AST_new_get_expr(char *variable, char *key, bool is_enum);

void AST_free_node(t_ast_node *node, t_logger *logger);

void AST_print_functions(t_vector *functions, int offset, t_logger *logger);
void AST_print_ast(t_ast_node *node, int offset, t_logger *logger);

#endif // __AST_H__
