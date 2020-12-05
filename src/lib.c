/** @file lib.c */
#include "lib.h"

#include <stdlib.h>

#include "ast.h"

void LIB_free_tokens_vector(t_vector *tokens)
{
    t_token *token = NULL;
    t_iterator iterator = vector_begin(tokens);
    t_iterator last = vector_end(tokens);
    for (; !iterator_equals(&iterator, &last); iterator_increment(&iterator))
    {
        token = *(t_token_ptr *)iterator_get(&iterator);
        if ((token->type == T_NUMBER) || (token->type == T_STRING) ||
            ((token->type <= T_IDENTIFIER) && (token->type > T_UNKNOWN)))
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
}

void LIB_free_functions_vector(t_vector *functions, t_logger *logger)
{
    t_ast_node *function = NULL;
    t_iterator iterator = vector_begin(functions);
    t_iterator last = vector_end(functions);
    for (; !iterator_equals(&iterator, &last); iterator_increment(&iterator))
    {
        function = *(t_ast_node_ptr *)iterator_get(&iterator);
        AST_free_node(function, logger);
    }

    (void) vector_clear(functions);
    (void) vector_destroy(functions);
    (void) free(functions);
    functions = NULL;
}

char *LIB_stringify(const char* source, size_t source_length, t_logger *logger)
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
        (void) LOGGER_log(logger, L_ERROR, "Couldn't allocate memory for string in ast_stringify.\n");
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
