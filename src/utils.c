#include "utils.h"
#include "ast.h"
#include "type.h"
#include "vector.h"
#include <stdio.h>

char *UTILS_fill_function_name(char *function_name_buffer, size_t buffer_length,
                               t_ast_node *node, bool *pushed_first_arg,
                               bool *builtin, t_logger *logger)
{
    t_ast_node_type callable_type = node->call_expr.callable->type;

    if (callable_type == AST_TYPE_VARIABLE)
    {
        (void) snprintf(function_name_buffer, buffer_length, "%s",
                        node->call_expr.callable->variable.name);
    }
    else if (callable_type == AST_TYPE_GET_EXPR)
    {
        t_ast_node *variable = node->call_expr.callable->get_expr.variable,
                   *arg = NULL;
        t_type *type = variable->variable.type;
        char *name = variable->variable.name;

        /* Check if using syntactic sugar only if */
        if ((type->type == TYPE_PTR) && (type->inner_type->type == TYPE_STRUCT))
        {
            name = (char *) type->inner_type->payload;
            /* Push first arg only if relevant */
            if (NULL != pushed_first_arg)
            {
                arg = AST_new_variable(strdup(variable->variable.name),
                                       TYPE_dup_type(variable->variable.type),
                                       variable->variable.mutable);
                vector_push_front(node->call_expr.args, &arg);
                *pushed_first_arg = true;
            }
        }

        (void) snprintf(function_name_buffer, buffer_length, "%s.%s", name,
                        node->call_expr.callable->get_expr.key);
    }
    else if (callable_type == AST_TYPE_BUILTIN)
    {
        (void) snprintf(function_name_buffer, buffer_length, "%s",
                        node->call_expr.callable->builtin.name);
        if (NULL != builtin)
        {
            *builtin = true;
        }
    }
    else
    {
        LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                       "utils: Unknown callable type - %d\n", callable_type);
        (void) exit(LUKA_GENERAL_ERROR);
    }

    return function_name_buffer;
}

void UTILS_pop_first_arg(t_ast_node *node, t_logger *logger)
{
    t_ast_node *arg = NULL;
    if (node->type != AST_TYPE_CALL_EXPR)
    {
        return;
    }

    if (NULL == node->call_expr.args)
    {
        return;
    }

    if (node->call_expr.args->size < 1)
    {
        return;
    }

    arg = VECTOR_GET_AS(t_ast_node_ptr, node->call_expr.args, 0);
    (void) vector_pop_front(node->call_expr.args);
    (void) AST_free_node(arg, logger);
    arg = NULL;
}
