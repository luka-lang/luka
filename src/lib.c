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
            free(token->content);
            token->content = NULL;
        }
        free(token);
        token = NULL;
    }
    vector_clear(tokens);
    vector_destroy(tokens);
}

void LIB_free_functions_vector(t_vector *functions)
{
    t_ast_node *function = NULL;
    t_iterator iterator = vector_begin(functions);
    t_iterator last = vector_end(functions);
    for (; !iterator_equals(&iterator, &last); iterator_increment(&iterator))
    {
        function = *(t_ast_node_ptr *)iterator_get(&iterator);
        AST_free_node(function);
    }

    vector_clear(functions);
    vector_destroy(functions);
    free(functions);
}