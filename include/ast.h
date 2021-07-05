/** @file ast.h  */
#ifndef __AST_H__
#define __AST_H__

#include "defs.h"
#include "logger.h"

#include <stdlib.h>

/**
 * @brief Creates a new AST node of a number literal.
 *
 * @param[in] type the type of the passed in number.
 * @param[in] value a pointer to value of the number.
 * @return an AST node of a number literal with the passed in value and type.
 */
t_ast_node *AST_new_number(t_type *type, void *value);

/**
 * @brief Creates a new AST node of a string literal.
 *
 * @param[in] content the content of the string literal.
 * @return an AST node of a string literal with the passed in content.
 */
t_ast_node *AST_new_string(char *content);

/**
 * @brief Creates a new AST node of a unary expression.
 *
 * @param[in] operator the operator used in the unary expression.
 * @param[in] rhs the operand in the unary expression.
 *
 * @return an AST node of a unary expression with the passed in operator and
 * rhs.
 */
t_ast_node *AST_new_unary_expr(t_ast_unop_type operator, t_ast_node * rhs,
                               bool mutable);

/**
 * @brief Creates a new AST node of a binary expression.
 *
 * @param[in] operator the operator used in the binary expression.
 * @param[in] lhs the left hand side in the binary expression.
 * @param[in] rhs the right hand side in the binary expression.
 *
 * @return an AST node of a binary expression with the passed in operator, lhs
 * and rhs.
 */
t_ast_node *AST_new_binary_expr(t_ast_binop_type operator, t_ast_node * lhs,
                                t_ast_node *rhs);

/**
 * @brief Creates a new AST node of a function prototype.
 *
 * A function prototype describes a function.
 *
 * @param[in] name the name of the function.
 * @param[in] args the names of the function arguments.
 * @param[in] types the types of the function arguments.
 * @param[in] arity the arity of the function (how many arguments does it take).
 * @param[in] return_type the type of the expression this function returns.
 * @param[in] vararg whether or not this function is variadic.
 *
 * @return an AST node of a binary expression with the passed in name, args,
 * types, arity, return_type and vararg.
 */
t_ast_node *AST_new_prototype(char *name, char **args, t_type **types,
                              int arity, t_type *return_type, bool vararg);

/**
 * @brief Creates a new AST node of a function.
 *
 * @param[in] prototype a prototype node that describes the function.
 * @param[in] body a vector of statements, what happens in this function.
 *
 * @return an AST node of a function with the passed in prototype and body.
 */
t_ast_node *AST_new_function(t_ast_node *prototype, t_vector *body);

/**
 * @brief Creates a new AST node of a return statement.
 *
 * @param[in] expr the expression returned by this statement.
 *
 * @return an AST node of a return statement with the passed in expr.
 */
t_ast_node *AST_new_return_stmt(t_ast_node *expr);

/**
 * @brief Creates a new AST node of a let statement.
 *
 * @param[in] var an AST node of the variable the expression is bound to.
 * @param[in] expr the expression that the variable will be bound to.
 * @param[in] is_global whether the variable is global or not.
 *
 * @return an AST node of a let statement with the passed in var, expr and
 * is_global.
 */
t_ast_node *AST_new_let_stmt(t_ast_node *var, t_ast_node *expr, bool is_global);

/**
 * @brief Creates a new AST node of a let statement.
 *
 * @details An assignment target can be one of the following things:
 *
 * * A variable
 * * A get expression
 * * An array derefrence
 * * Any other node that has a memory address.
 *
 * @param[in] lhs an AST node of an assignment target the expression will be
 * assigned to.
 * @param[in] rhs the expression that the assignment target will be assigned to.
 *
 * @return an AST node of a assignment statement with the passed in lhs and rhs.
 */
t_ast_node *AST_new_assignment_expr(t_ast_node *lhs, t_ast_node *rhs);

/**
 * @brief Creates a new AST node of an if expression.
 *
 * @note This if is an expression and can return a value based on the executed
 * branch.
 *
 * @param[in] cond the expression to check for deciding which branch should be
 * taken.
 * @param[in] then_body a vector of statements that will be executed if the
 * condition is true.
 * @param[in] else_body a vector of statements that will be executed if the
 * condition is false.
 *
 * @return an AST node of an if expression with the passed in cond, then_body
 * and else_body.
 */
t_ast_node *AST_new_if_expr(t_ast_node *cond, t_vector *then_body,
                            t_vector *else_body);

/**
 * @brief Creates a new AST node of a while expression.
 *
 * @note This while is an expression and can return a value from its body.
 *
 * @param[in] cond the expression to check for deciding if the loop continues or
 * not.
 * @param[in] body a vector of statements that will be executed as long as the
 * condition is true.
 *
 * @return an AST node of a while expression with the passed in cond and body.
 */
t_ast_node *AST_new_while_expr(t_ast_node *cond, t_vector *body);

/**
 * @brief Creates a new AST node of a cast expression.
 *
 * @param[in] expr the expression to cast.
 * @param[in] type the type the expression will be casted to.
 *
 * @return an AST node of a cast expression with the passed in expr and type.
 */
t_ast_node *AST_new_cast_expr(t_ast_node *expr, t_type *type);

/**
 * @brief Creates a new AST node of a variable reference.
 *
 * @param[in] name the name of the variable.
 * @param[in] type the type of the variable.
 * @param[in] mutable whether the variable is mutable.
 *
 * @return an AST node of a variable reference with the passed in name, type and
 * mutable.
 */
t_ast_node *AST_new_variable(char *name, t_type *type, bool mutable);

/**
 * @brief Creates a new AST node of a call expression.
 *
 * @param[in] callable the callable that should be called.
 * @param[in] args a vector of AST nodes for the arguments that are passed to
 * function.
 *
 * @return an AST node of a call expression with the passed in name and args.
 */
t_ast_node *AST_new_call_expr(t_ast_node *callable, t_vector *args);

/**
 * @brief Creates a new AST node of an expression statement.
 *
 * @note Expression statements do not return the value of the expresions inside
 * them.
 *
 * @param[in] expr the expression performed in this statement.
 *
 * @return an AST node of a expression statement with the passed in expression.
 */
t_ast_node *AST_new_expression_stmt(t_ast_node *expr);

/**
 * @brief Creates a new AST node of a break statement.
 *
 * @note Break statements can only be used inside loops.
 *
 * @return an AST node of a break statement.
 */
t_ast_node *AST_new_break_stmt();

/**
 * @brief Creates a new AST node of a struct defintion.
 *
 * @details A struct defintion is used when defining a new struct type.
 *
 * @param[in] name the name of the struct.
 * @param[in] struct_fields a vector of t_struct_field* that contains the fields
 * of the struct.
 * @param[in] functions a vector of t_ast_node* that contains the functions
 * defined in the struct.
 *
 * @return an AST node of a struct definition with the passed in name and
 * struct_fields.
 */
t_ast_node *AST_new_struct_definition(char *name, t_vector *struct_fields,
                                      t_vector *functions);

/**
 * @brief Creates a new AST node of a struct value.
 *
 * @param[in] name the name of the struct.
 * @param[in] value_fields a vector of t_struct_value_field* that contains
 * values for fields of the struct.
 *
 * @return an AST node of a struct definition with the passed in name and
 * struct_fields.
 */
t_ast_node *AST_new_struct_value(char *name, t_vector *value_fields);

/**
 * @brief Creates a new AST node of a enum defintion.
 *
 * @details A enum defintion is used when defining a new enum type.
 *
 * @param[in] name the name of the enum.
 * @param[in] enum_fields a vector of t_enum_field* that contains the fields of
 * the enum.
 *
 * @return an AST node of a enum definition with the passed in name and
 * enum_fields.
 */
t_ast_node *AST_new_enum_definition(char *name, t_vector *enum_fields);

/**
 * @brief Creates a new AST node of a get expression.
 *
 * @details Get expressions are used in order to get a field of a struct or a
 * value from an enum.
 *
 * Enum get expressions look like `Enum::Value` while struct get expression look
 * like `Struct.Field`.
 *
 * @param[in] variable a reference to the variable used.
 * @param[in] key the field of the struct or the value of the enum requested.
 * @param[in] is_enum whether this is an enum get expression or not.
 *
 * @return an AST node of a get expression with the passed in name, key and
 * is_enum.
 */

t_ast_node *AST_new_get_expr(t_ast_node *variable, char *key, bool is_enum);

/**
 * @brief Creates a new AST node of an array derefrence.
 *
 * @details Array derefrences are used to get a value from an array or pointer
 * based on the offset notated by `index`.
 *
 * @param[in] variable a reference to the variable.
 * @param[in] index an expression that evalutes to the requested offset.
 *
 * @return an AST node of an array dereference with the passed in variable and
 * index.
 */
t_ast_node *AST_new_array_deref(t_ast_node *variable, t_ast_node *index);

/**
 * @brief Creates a new AST node of a literal.
 *
 * @details Literals are either `true`, `false` or `null`.
 *
 * @param[in] type the type of the literal.
 *
 * @return an AST node of a literal with the passed in type.
 */
t_ast_node *AST_new_literal(t_ast_literal_type type);

/**
 * @brief Creates a new AST node of an array literal.
 *
 * @param[in] exprs the elements of the array literal.
 * @param[in] type the type of the array literal's elements.
 *
 * @return an AST node of an array literal with the passed in exprs and type.
 */
t_ast_node *AST_new_array_literal(t_vector *exprs, t_type *type);

/**
 * @brief Creates a new AST node of a builtin.
 *
 * @param[in] name the name of the builtin.
 *
 * @return an AST node of a builtin with the passed in name.
 */
t_ast_node *AST_new_builtin(char *builtin);

/**
 * @brief Creates a new AST node of a type expr.
 *
 * @param[in] type the type of the type expr.
 *
 * @return an AST node of a type expr with the passed in type.
 */
t_ast_node *AST_new_type_expr(t_type *type);

/**
 * @brief Promotes the last expression statement to an expresion in a function
 * body.
 *
 * @details The expression statement is promoted if and only if it could also be
 * parsed as an expression without code changes.
 *
 * For example, if and while expressions do not require a semi-colon to become
 * expression statements so when they are the last expression in a function the
 * function won't return their value.
 *
 * @param[in] node the AST node that should be fixed.
 *
 * @return the same AST node with the expression statement promoted to
 * expression if applicable.
 */
t_ast_node *AST_fix_function_last_expression_stmt(t_ast_node *node);

/**
 * @brief Fix structs and enums that were classified as type aliases.
 *
 * @param[in] node the AST node that should be fixed.
 * @param[in] module the module that should be used to look up structs and enums.
 * @param[in] logger the logger to be used to log messages.
 *
 * @return the same AST node with the type possibly fixed.
 */
t_ast_node *AST_fix_types(t_ast_node *node, t_module *module, t_logger *logger);

/**
 * @brief Resolve all type aliases inside @p node using @p type_aliases.
 *
 * @param[in] node the node to resolve type aliases in.
 * @param[in] type_aliases a vector that contains all type_aliases in scope.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @returns @p node with resolved type aliases.
 */
t_ast_node *AST_resolve_type_aliases(t_ast_node *node, t_vector *type_aliases,
                                     t_logger *logger);

/**
 * @brief Populate types of variable reference to parameters of @p function with
 * the correct types.
 *
 * @param[in,out] function the function to fill types in.
 * @param[in] logger a logger that can be used to log messages.
 */
void AST_fill_parameter_types(t_ast_node *function, t_logger *logger,
                              const t_module *module);

/**
 * @brief Populate types of variable reference to variables declared in @p
 * function with the correct types.
 *
 * @param[in,out] function the function to fill types in.
 * @param[in] logger a logger that can be used to log messages.
 */
void AST_fill_variable_types(t_ast_node *function, t_logger *logger,
                             const t_module *module);

/**
 * @brief Helper function to know if a given node can be used as an
 * expression.
 *
 * @param[in] node the AST node to check.
 *
 * @return whether or not the node is an expression.
 */
bool AST_is_expression(t_ast_node *node);

/**
 * @brief Helper function to a free an AST node and all its resources.
 *
 * @param[in] node the AST node to free.
 * @param[in] logger a logger that can be used to log messages.
 */
void AST_free_node(t_ast_node *node, t_logger *logger);

/**
 * @brief Helper function to print multiple functions.
 *
 * @param[in] functions a vector of function AST nodes to print.
 * @param[in] offset the offset to print at.
 * @param[in] logger a logger that can be used to log messages.
 */
void AST_print_functions(t_vector *functions, int offset, t_logger *logger);

/**
 * @brief Helper function to print an AST node.
 *
 * @param[in] node the AST node to print.
 * @param[in] offset the offset to print at.
 * @param[in] logger a logger that can be used to log messages.
 */
void AST_print_ast(t_ast_node *node, int offset, t_logger *logger);

/**
 * @brief Checks if a binary operator is a condition operator.
 *
 * @param[in] op the binary operator.
 *
 * @return whether the binary operator is a condition operator.
 */
bool AST_is_cond_binop(t_ast_binop_type op);

#endif // __AST_H__
