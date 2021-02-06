/** @file lib.c */
#include "lib.h"

#include <stdlib.h>

#include "ast.h"
#include "defs.h"
#include "type.h"

void LIB_free_tokens_vector(t_vector *tokens)
{
    t_token *token = NULL;
    t_iterator iterator = vector_begin(tokens);
    t_iterator last = vector_end(tokens);
    for (; !iterator_equals(&iterator, &last); iterator_increment(&iterator))
    {
        token = *(t_token_ptr *) iterator_get(&iterator);
        if ((token->type == T_NUMBER) || (token->type == T_STRING)
            || ((token->type <= T_IDENTIFIER) && (token->type > T_UNKNOWN)))
        {
            if (NULL != token->content)
            {
                (void) free(token->content);
                token->content = NULL;
            }
        }
        (void) free(token);
        token = NULL;
    }
    (void) vector_clear(tokens);
    (void) vector_destroy(tokens);
    (void) free(tokens);
    tokens = NULL;
}

void lib_free_nodes_vector(t_vector *nodes, t_logger *logger)
{
    t_ast_node *node = NULL;
    t_iterator iterator = vector_begin(nodes);
    t_iterator last = vector_end(nodes);

    for (; !iterator_equals(&iterator, &last); iterator_increment(&iterator))
    {
        node = *(t_ast_node_ptr *) iterator_get(&iterator);
        (void) AST_free_node(node, logger);
    }

    (void) vector_clear(nodes);
    (void) vector_destroy(nodes);
    (void) free(nodes);
    nodes = NULL;
}

void lib_free_strings_vector(t_vector *strings)
{
    char *string = NULL;
    t_iterator iterator = vector_begin(strings);
    t_iterator last = vector_end(strings);

    for (; !iterator_equals(&iterator, &last); iterator_increment(&iterator))
    {
        string = *(char **) iterator_get(&iterator);
        (void) free(string);
    }

    (void) vector_clear(strings);
    (void) vector_destroy(strings);
    (void) free(strings);
    strings = NULL;
}

void lib_free_modules_vector(t_vector *modules, t_logger *logger)
{
    t_module *module = NULL;
    t_iterator iterator = vector_begin(modules);
    t_iterator last = vector_end(modules);

    for (; !iterator_equals(&iterator, &last); iterator_increment(&iterator))
    {
        module = *(t_module **) iterator_get(&iterator);
        (void)logger;
        (void) module;
        // (void) LIB_free_module(module, logger);
    }

    (void) vector_clear(modules);
    (void) vector_destroy(modules);
    (void) free(modules);
    modules = NULL;
}

void LIB_free_type_aliases_vector(t_vector *type_alises)
{
    t_type_alias *type_alias = NULL;
    t_iterator iterator = vector_begin(type_alises);
    t_iterator last = vector_end(type_alises);

    for (; !iterator_equals(&iterator, &last); iterator_increment(&iterator))
    {
        type_alias = *(t_type_alias **) iterator_get(&iterator);
        if (NULL != type_alias)
        {
            if (NULL != type_alias->name)
            {
                (void) free(type_alias->name);
                type_alias->name = NULL;
            }

            if (NULL != type_alias->type)
            {
                (void) TYPE_free_type(type_alias->type);
                type_alias->type = NULL;
            }

            (void) free(type_alias);
            type_alias = NULL;
        }
    }

    (void) vector_clear(type_alises);
    (void) vector_destroy(type_alises);
    (void) free(type_alises);
    type_alises = NULL;
}

t_return_code LIB_intialize_list(t_vector **items, size_t item_size,
                                 t_logger *logger)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    *items = calloc(1, sizeof(t_vector));
    if (NULL == *items)
    {
        (void) LOGGER_log(logger, L_ERROR,
                          "Couldn't allocate memory for items");
        status_code = LUKA_CANT_ALLOC_MEMORY;
        goto l_cleanup;
    }

    if (vector_setup(*items, 5, item_size))
    {
        status_code = LUKA_VECTOR_FAILURE;
        goto l_cleanup;
    }

    return LUKA_SUCCESS;

l_cleanup:
    if (NULL != *items)
    {
        (void) free(*items);
    }

    return status_code;
}

t_return_code LIB_initialize_module(t_module **module, t_logger *logger)
{
    t_return_code status_code = LUKA_UNINITIALIZED;

    *module = calloc(1, sizeof(t_module));
    if (NULL == *module)
    {
        status_code = LUKA_CANT_ALLOC_MEMORY;
        goto l_cleanup;
    }

    (*module)->enums = NULL;
    (*module)->functions = NULL;
    (*module)->import_paths = NULL;
    (*module)->imports = NULL;
    (*module)->structs = NULL;
    (*module)->variables = NULL;
    (*module)->file_path = NULL;

    RAISE_LUKA_STATUS_ON_ERROR(
        LIB_intialize_list(&(*module)->enums, sizeof(t_ast_node_ptr), logger),
        status_code, l_cleanup);

    RAISE_LUKA_STATUS_ON_ERROR(LIB_intialize_list(&(*module)->functions,
                                                  sizeof(t_ast_node_ptr),
                                                  logger),
                               status_code, l_cleanup);

    RAISE_LUKA_STATUS_ON_ERROR(
        LIB_intialize_list(&(*module)->import_paths, sizeof(char *), logger),
        status_code, l_cleanup);

    RAISE_LUKA_STATUS_ON_ERROR(
        LIB_intialize_list(&(*module)->imports, sizeof(t_module *), logger),
        status_code, l_cleanup);

    RAISE_LUKA_STATUS_ON_ERROR(
        LIB_intialize_list(&(*module)->structs, sizeof(t_ast_node_ptr), logger),
        status_code, l_cleanup);

    RAISE_LUKA_STATUS_ON_ERROR(LIB_intialize_list(&(*module)->variables,
                                                  sizeof(t_ast_node_ptr),
                                                  logger),
                               status_code, l_cleanup);

    status_code = LUKA_SUCCESS;
    return status_code;
l_cleanup:
    (void) LIB_free_module(*module, logger);
    return status_code;
}

void LIB_free_module(t_module *module, t_logger *logger)
{
    if (NULL != module)
    {
        if (NULL != module->functions)
        {
            (void) lib_free_nodes_vector(module->functions, logger);
            module->functions = NULL;
        }

        if (NULL != module->import_paths)
        {
            (void) lib_free_strings_vector(module->import_paths);
            module->import_paths = NULL;
        }

        if (NULL != module->imports)
        {
            (void) lib_free_modules_vector(module->imports, logger);
            module->imports = NULL;
        }

        if (NULL != module->structs)
        {
            (void) lib_free_nodes_vector(module->structs, logger);
            module->structs = NULL;
        }

        if (NULL != module->variables)
        {
            (void) lib_free_nodes_vector(module->variables, logger);
            module->variables = NULL;
        }

        if (NULL != module->enums)
        {
            (void) lib_free_nodes_vector(module->enums, logger);
            module->enums = NULL;
        }

        module->file_path = NULL;

        (void) free(module);
        module = NULL;
    }
}

char *LIB_stringify(const char *source, size_t source_length, t_logger *logger)
{
    size_t i = 0;
    size_t char_count = source_length;
    size_t off = 0;
    for (i = 0; i < source_length; ++i)
    {
        switch (source[i])
        {
            case '\n':
            case '\t':
            case '\\':
            case '\"':
                ++char_count;
                break;
            default:
                break;
        }
    }

    ++i;

    char *str = calloc(sizeof(char), char_count + 1);
    if (NULL == str)
    {
        (void) LOGGER_log(
            logger, L_ERROR,
            "Couldn't allocate memory for string in ast_stringify.\n");
        return NULL;
    }

    for (i = 0; i < source_length && i + off < char_count; ++i)
    {
        switch (source[i])
        {
            case '\n':
                str[i + off] = '\\';
                str[i + off + 1] = 'n';
                ++off;
                break;
            case '\t':
                str[i + off] = '\\';
                str[i + off + 1] = 't';
                ++off;
                break;
            case '\\':
                str[i + off] = '\\';
                str[i + off + 1] = '\\';
                ++off;
                break;
            case '\"':
                str[i + off] = '\\';
                str[i + off + 1] = '"';
                ++off;
                break;
            default:
                str[i + off] = source[i];
        }
    }

    str[char_count] = '\0';
    return str;
}

t_ast_node *LIB_resolve_func_name(const t_module *module, const char *name,
                                  const t_module *original_module)
{
    t_ast_node *func = NULL;
    t_module *imported_module = NULL;

    if (NULL == module)
    {
        return NULL;
    }

    if (NULL == module->functions)
    {
        return NULL;
    }

    VECTOR_FOR_EACH(module->functions, functions)
    {
        func = ITERATOR_GET_AS(t_ast_node_ptr, &functions);
        if (NULL == func->function.prototype)
        {
            continue;
        }

        if (NULL == func->function.prototype->prototype.name)
        {
            continue;
        }

        if (0 != strcmp(name, func->function.prototype->prototype.name))
        {
            continue;
        }

        return func;
    }

    func = NULL;
    VECTOR_FOR_EACH(module->imports, imports)
    {
        imported_module = *(t_module **) iterator_get(&imports);
        if (NULL == original_module)
        {
            func = LIB_resolve_func_name(imported_module, name, module);
        }
        else if (0
                 != strcmp(imported_module->file_path,
                           original_module->file_path))
        {
            func
                = LIB_resolve_func_name(imported_module, name, original_module);
        }

        if (NULL == func)
        {
            continue;
        }

        return func;
    }

    return NULL;
}
