#include "core.h"
#include "ast.h"
#include "type.h"

#define NUMBER_OF_BUILTINS 1

#define ALLOC_GENERIC(amount, var_name, type)                                  \
    do                                                                         \
    {                                                                          \
        var_name = calloc((amount), sizeof(type));                             \
        if (NULL == var_name)                                                  \
        {                                                                      \
            goto l_cleanup;                                                    \
        }                                                                      \
    } while (0)

#define ALLOC_ARGS(amount)  ALLOC_GENERIC((amount), args, char *)
#define ALLOC_TYPES(amount) ALLOC_GENERIC((amount), types, t_type *)

t_ast_node *g_builtins[NUMBER_OF_BUILTINS] = {0};

bool CORE_initialize_builtins(t_logger *logger)
{
    int i = 0;
    t_ast_node *prototype = NULL;
    t_type **types = NULL, *return_type = NULL;
    char **args = NULL;

    return_type = TYPE_initialize_type(TYPE_UINT64);
    ALLOC_ARGS(1);
    args[0] = "expr_type";

    ALLOC_TYPES(1);
    types[0] = TYPE_initialize_type(TYPE_TYPE);
    prototype
        = AST_new_prototype("@sizeOf", args, types, 1, return_type, false);
    g_builtins[i++] = prototype;

    return true;

l_cleanup:
    for (i = 0; i < NUMBER_OF_BUILTINS; ++i)
    {
        if (NULL != g_builtins[i])
        {
            AST_free_node(g_builtins[i], logger);
        }
    }
    return false;
}

t_ast_node *CORE_lookup_builtin(const t_ast_node *builtin)
{
    size_t i = 0;

    if (NULL == builtin)
    {
        return NULL;
    }

    if (NULL == builtin->builtin.name)
    {
        return NULL;
    }

    for (i = 0; i < NUMBER_OF_BUILTINS; ++i)
    {
        if (0 == strcmp(builtin->builtin.name, g_builtins[i]->prototype.name))
        {
            return g_builtins[i];
        }
    }

    return NULL;
}
