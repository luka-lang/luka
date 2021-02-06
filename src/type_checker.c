#include "type_checker.h"
#include "defs.h"
#include "io.h"
#include "lib.h"
#include "logger.h"
#include "type.h"
#include "vector.h"
#include <string.h>

bool check_expr(const t_module *module, const t_ast_node *expr,
                t_logger *logger);
bool check_stmt(const t_module *module, const t_ast_node *stmt,
                t_logger *logger);

bool check_expr(const t_module *module, const t_ast_node *expr,
                t_logger *logger)
{
    t_ast_node *func = NULL, *proto = NULL, *stmt = NULL, *node = NULL;
    bool vararg = false, success = false;
    size_t required_params_count = 0, i = 0;
    t_type *type1 = NULL, *type2 = NULL;
    char type1_str[1024], type2_str[1024];

    (void) LOGGER_log(logger, L_INFO, "check_expr: case %d\n", expr->type);

    switch (expr->type)
    {
        case AST_TYPE_NUMBER:
        case AST_TYPE_STRING:
        case AST_TYPE_LITERAL:
            return true;
        case AST_TYPE_CALL_EXPR:
            {
                if (NULL == expr->call_expr.args)
                {
                    return true;
                }

                func = LIB_resolve_func_name(module, expr->call_expr.name, NULL);
                if (NULL == func)
                {
                    LOGGER_LOG_LOC(logger, L_ERROR, expr->token,
                                   "Func %s not found in scope\n",
                                   expr->call_expr.name);
                    return false;
                }

                if (NULL == func->function.prototype)
                {
                    LOGGER_LOG_LOC(logger, L_ERROR, expr->token,
                                   "Func %s prototype is NULL\n",
                                   expr->call_expr.name);
                    return false;
                }

                proto = func->function.prototype;

                if (NULL == expr->call_expr.args)
                {
                    LOGGER_LOG_LOC(logger, L_ERROR, expr->token,
                                   "Call expr for func %s args are NULL\n",
                                   expr->call_expr.name);
                    return false;
                }

                vararg = proto->prototype.vararg;
                required_params_count = proto->prototype.arity;
                if (vararg)
                {
                    --required_params_count;
                }

                if (!vararg
                    && expr->call_expr.args->size != required_params_count)
                {
                    LOGGER_LOG_LOC(
                        logger, L_ERROR, expr->token,
                        "Function `%s` called with incorrect number of "
                        "arguments, "
                        "expected %d arguments but got %d arguments.\n",
                        expr->call_expr.name, required_params_count,
                        expr->call_expr.args->size);
                    return false;
                }

                if (vararg
                    && expr->call_expr.args->size < required_params_count)
                {
                    LOGGER_LOG_LOC(logger, L_ERROR, expr->token,
                                   "Function `%s` is variadic but not called "
                                   "with enough arguments, "
                                   "expected at least %d arguments but got "
                                   "%d arguments.\n",
                                   expr->call_expr.name,
                                   expr->call_expr.args->size,
                                   required_params_count);
                    return false;
                }

                for (i = 0; i < proto->prototype.arity
                            && i < expr->call_expr.args->size;
                     ++i)
                {
                    node = *(t_ast_node **) vector_get(expr->call_expr.args, i);
                    type1 = TYPE_get_type(node, logger, module);
                    type2 = proto->prototype.types[i];
                    if (!TYPE_equal(type1, type2))
                    {
                        (void) memset(type1_str, 0, 1024);
                        (void) memset(type2_str, 0, 1024);
                        (void) TYPE_to_string(type1, logger, type1_str, 1024);
                        (void) TYPE_to_string(type2, logger, type2_str, 1024);
                        LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                                       "Expected argument `%s` of function "
                                       "`%s` to be of type "
                                       "`%s` but got parameter of type `%s`\n",
                                       proto->prototype.args[i],
                                       expr->call_expr.name, type2_str,
                                       type1_str);

                        (void) TYPE_free_type(type1);
                        return false;
                    }

                    if (type2->mutable && !type1->mutable)
                    {
                        LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                                       "Expected argument `%s` of function "
                                       "`%s` to be mutable "
                                       "but got an immutable parameter\n",
                                       proto->prototype.args[i],
                                       expr->call_expr.name);
                        return false;
                    }
                }
                return true;
            }
        case AST_TYPE_WHILE_EXPR:
            {
                if (NULL != expr->while_expr.body)
                {
                    VECTOR_FOR_EACH(expr->while_expr.body, stmts)
                    {
                        stmt = ITERATOR_GET_AS(t_ast_node_ptr, &stmts);
                        success = check_stmt(module, stmt, logger);
                        if (!success)
                        {
                            return false;
                        }
                    }
                }

                return true;
            }
        case AST_TYPE_IF_EXPR:
            {
                if (NULL != expr->if_expr.then_body)
                {
                    VECTOR_FOR_EACH(expr->if_expr.then_body, stmts)
                    {
                        stmt = ITERATOR_GET_AS(t_ast_node_ptr, &stmts);
                        success = check_stmt(module, stmt, logger);
                        if (!success)
                        {
                            return false;
                        }
                    }
                }

                if (NULL != expr->if_expr.else_body)
                {
                    VECTOR_FOR_EACH(expr->if_expr.else_body, stmts)
                    {
                        stmt = ITERATOR_GET_AS(t_ast_node_ptr, &stmts);
                        success = check_stmt(module, stmt, logger);
                        if (!success)
                        {
                            return false;
                        }
                    }
                }

                return true;
            }
        case AST_TYPE_ASSIGNMENT_EXPR:
            if (!check_expr(module, expr->assignment_expr.rhs, logger))
            {
                return false;
            }
            type1 = TYPE_get_type(expr->assignment_expr.lhs, logger, module);
            type2 = TYPE_get_type(expr->assignment_expr.rhs, logger, module);
            if (!TYPE_equal(type2, type1))
            {
                (void) memset(type1_str, 0, 1024);
                (void) memset(type2_str, 0, 1024);
                (void) TYPE_to_string(type1, logger, type1_str, 1024);
                (void) TYPE_to_string(type2, logger, type2_str, 1024);
                LOGGER_LOG_LOC(logger, L_ERROR, expr->token,
                               "Assignment expr type checking failed: "
                               "lhs is of type `%s` but rhs is of type `%s`\n",
                               type1_str, type2_str);

                (void) TYPE_free_type(type1);
                (void) TYPE_free_type(type2);
                return false;
            }

            if (!type1->mutable)
            {
                (void) TYPE_to_string(type1, logger, type1_str, 1024);
                LOGGER_LOG_LOC(
                    logger, L_ERROR, expr->token,
                    "Assignment expr type checking failed: "
                    "Tried to assign to immutable lhs of type `%s`\n",
                    type1_str);
                (void) TYPE_free_type(type1);
                (void) TYPE_free_type(type2);
                return false;
            }

            (void) TYPE_free_type(type1);
            (void) TYPE_free_type(type2);
            return true;
        case AST_TYPE_GET_EXPR:
            if (NULL == expr->get_expr.variable)
            {
                LOGGER_LOG_LOC(logger, L_ERROR, expr->token,
                               "Get expr variable is NULL\n", NULL);
                return false;
            }
            type1 = expr->get_expr.variable->variable.type;
            if (NULL == type1)
            {
                LOGGER_LOG_LOC(logger, L_ERROR, expr->token,
                               "Get expr variable type is NULL\n", NULL);
                return false;
            }
            return (expr->get_expr.is_enum ? TYPE_ENUM == type1->type
                                           : TYPE_STRUCT == type1->type);
        case AST_TYPE_BINARY_EXPR:
            if (!check_expr(module, expr->binary_expr.rhs, logger))
            {
                return false;
            }

            if (!check_expr(module, expr->binary_expr.rhs, logger))
            {
                return false;
            }

            type1 = TYPE_get_type(expr->binary_expr.lhs, logger, module);
            type2 = TYPE_get_type(expr->binary_expr.rhs, logger, module);
            if (!TYPE_equal(type1, type2))
            {
                (void) memset(type1_str, 0, 1024);
                (void) memset(type2_str, 0, 1024);
                (void) TYPE_to_string(type1, logger, type1_str, 1024);
                (void) TYPE_to_string(type2, logger, type2_str, 1024);
                LOGGER_LOG_LOC(logger, L_ERROR, expr->token,
                               "Binary expr type checking failed: "
                               "lhs is of type `%s` but rhs is of type `%s`\n",
                               type1_str, type2_str);

                (void) TYPE_free_type(type1);
                (void) TYPE_free_type(type2);
                return false;
            }
            return true;
        default:
            (void) LOGGER_log(logger, L_INFO, "check_expr: default case %d\n",
                              expr->type);
            return true;
    }
}

bool check_stmt(const t_module *module, const t_ast_node *stmt,
                t_logger *logger)
{
    t_type *type1 = NULL, *type2 = NULL;
    char type1_str[1024], type2_str[1024];

    (void) LOGGER_log(logger, L_INFO, "check_stmt: case %d\n", stmt->type);
    switch (stmt->type)
    {
        case AST_TYPE_EXPRESSION_STMT:
            return check_expr(module, stmt->expression_stmt.expr, logger);
        case AST_TYPE_LET_STMT:
            if (!check_expr(module, stmt->let_stmt.expr, logger))
            {
                return false;
            }

            type1 = TYPE_get_type(stmt->let_stmt.var, logger, module);
            type2 = TYPE_get_type(stmt->let_stmt.expr, logger, module);
            if (!TYPE_equal(type2, type1))
            {
                (void) memset(type1_str, 0, 1024);
                (void) memset(type2_str, 0, 1024);
                (void) TYPE_to_string(type1, logger, type1_str, 1024);
                (void) TYPE_to_string(type2, logger, type2_str, 1024);
                LOGGER_LOG_LOC(logger, L_ERROR, stmt->token,
                               "Let stmt type checking failed: "
                               "lhs is of type `%s` but rhs is of type `%s`\n",
                               type1_str, type2_str);

                (void) TYPE_free_type(type1);
                (void) TYPE_free_type(type2);
                return false;
            }
            return true;
        default:
            (void) LOGGER_log(logger, L_INFO, "check_stmt: default case %d\n",
                              stmt->type);
            return check_expr(module, stmt, logger);
    }
}

bool CHECK_function(const t_module *module, const t_ast_node *function,
                    t_logger *logger)
{
    t_ast_node *stmt = NULL;
    bool success = false;

    if (NULL == function->function.body)
    {
        return true;
    }

    VECTOR_FOR_EACH(function->function.body, statments)
    {
        stmt = ITERATOR_GET_AS(t_ast_node_ptr, &statments);

        success = check_stmt(module, stmt, logger);
        if (!success)
        {
            return false;
        }
    }

    return true;
}
