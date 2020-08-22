#include "parser.h"
#include "ast.h"
#include "expr.h"

#include <string.h>

t_ast_node *parse_expression(t_parser *parser);

t_vector *parse_statements(t_parser *parser);

t_ast_node *parse_prototype(t_parser *parser);

void ERR(t_parser *parser, const char *message)
{
    t_token *token = NULL;
    token = *(t_token_ptr *)vector_get(parser->tokens, parser->index + 1);

    (void) fprintf(stderr, "%s\n", message);

    (void) fprintf(stderr, "Error at %s %ld:%ld - %s\n", parser->file_path, token->line,
                   token->offset, token->content);
    (void) exit(1);
}

bool EXPECT(t_parser *parser, t_toktype type)
{
    t_token *token = NULL;

    (void) assert(parser->index + 1 < parser->tokens->size);

    token = *(t_token_ptr *)vector_get(parser->tokens, parser->index + 1);

    return token->type == type;
}

bool MATCH(t_parser *parser, t_toktype type)
{
    t_token *token = NULL;

    (void) assert(parser->index < parser->tokens->size);

    token = *(t_token_ptr *)vector_get(parser->tokens, parser->index);

    return token->type == type;
}

void ADVANCE(t_parser *parser)
{
    ++parser->index;
    (void) assert(parser->index < parser->tokens->size);
}

void EXPECT_ADVANCE(t_parser *parser, t_toktype type, const char *message)
{
    if (!EXPECT(parser, type))
    {
        ERR(parser, message);
    }

    ADVANCE(parser);
}

void MATCH_ADVANCE(t_parser *parser, t_toktype type, const char *message)
{
    if (!MATCH(parser, type))
    {
        ERR(parser, message);
    }

    ADVANCE(parser);
}

t_type parse_type(t_parser *parser)
{
    t_token *token = NULL;

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index + 1);
    if (T_COLON != token->type)
    {
        return TYPE_INT32;
    }

    EXPECT_ADVANCE(parser, T_COLON, "Expected a `:` before type.");

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index + 1);

    ADVANCE(parser);

    switch (token->type)
    {
    case T_INT_TYPE:
        return TYPE_INT32;
    case T_STR_TYPE:
        return TYPE_STRING;
    case T_VOID_TYPE:
        return TYPE_VOID;
    case T_FLOAT_TYPE:
        return TYPE_FLOAT;
    case T_DOUBLE_TYPE:
        return TYPE_DOUBLE;
    default:
        (void) fprintf(stderr, "Unknown type %d %s. Fallbacking to int32.\n", token->type,
                token->content);
        return TYPE_INT32;
    }
}

void PARSER_initialize_parser(t_parser *parser, t_vector *tokens,
                       const char *file_path)
{
    (void) assert(parser != NULL);

    parser->tokens = tokens;
    parser->index = 0;
    parser->file_path = file_path;
}

t_vector *PARSER_parse_top_level(t_parser *parser)
{
    t_vector *functions = NULL;
    t_token *token = NULL;
    t_ast_node *function, *prototype;

    functions = calloc(1, sizeof(t_vector));
    if (NULL == functions)
    {
        (void) fprintf(stderr, "Couldn't allocate memory for functions");
        goto cleanup;
    }

    (void) vector_setup(functions, 5, sizeof(t_ast_node_ptr));

    while (parser->index < parser->tokens->size)
    {
        token = *(t_token_ptr *)vector_get(parser->tokens, parser->index);

        switch (token->type)
        {
        case T_FN:
        {
            function = PARSER_parse_function(parser);
            (void) vector_push_back(functions, &function);
            break;
        }
        case T_EXTERN:
        {
            prototype = parse_prototype(parser);
            function = AST_new_function(prototype, NULL);
            (void) vector_push_back(functions, &function);
            EXPECT_ADVANCE(parser, T_SEMI_COLON,
                           "Expected a `;` at the end of an extern statement.");
            break;
        }
        case T_EOF:
            break;
        default:
        {
            (void) fprintf(stderr, "Syntax error at %s %ld:%ld - %s\n", parser->file_path,
                           token->line, token->offset, token->content);
        }
        }
        parser->index += 1;
    }

    (void) vector_shrink_to_fit(functions);

cleanup:
    return functions;
}

int parse_op(t_token *token)
{
    switch (token->type)
    {
    case T_PLUS:
        return BINOP_ADD;
    case T_MINUS:
        return BINOP_SUBTRACT;
    case T_STAR:
        return BINOP_MULTIPLY;
    case T_SLASH:
        return BINOP_DIVIDE;
    case T_OPEN_ANG:
        return BINOP_LESSER;
    case T_CLOSE_ANG:
        return BINOP_GREATER;
    case T_BANG:
        return BINOP_NOT;
    case T_EQEQ:
        return BINOP_EQUALS;
    case T_NEQ:
        return BINOP_NEQ;
    case T_LEQ:
        return BINOP_LEQ;
    case T_GEQ:
        return BINOP_GEQ;
    default:
        (void) fprintf(stderr, "unknown token in parse_op at %ld:%ld\n", token->line,
                       token->offset);
        (void) exit(1);
    }
}

t_ast_node *parse_paren_expr(t_parser *parser)
{
    t_ast_node *expr;
    ADVANCE(parser);
    expr = parse_expression(parser);
    EXPECT_ADVANCE(parser, T_CLOSE_PAREN, "Expected ')'");
    ADVANCE(parser);
    return expr;
}

t_ast_node *parse_ident_expr(t_parser *parser)
{
    t_token *token;
    t_ast_node *node, *expr;
    char *ident_name;
    t_vector *args;

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    ident_name = token->content;

    ADVANCE(parser);
    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    if (T_OPEN_PAREN != token->type)
    {
        return AST_new_variable(ident_name, parse_type(parser));
    }

    ADVANCE(parser);
    args = calloc(1, sizeof(t_vector));
    if (NULL == args)
    {
        (void) fprintf(stderr, "Couldn't allocate memory for args.\n");
    }
    vector_setup(args, 10, sizeof(t_ast_node));
    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    if (T_CLOSE_PAREN != token->type)
    {
        while (true)
        {
            expr = parse_expression(parser);
            vector_push_back(args, &expr);

            token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
            if (T_CLOSE_PAREN == token->type)
            {
                break;
            }

            MATCH_ADVANCE(parser, T_COMMA, "Expected ')' or ',' in argument list.");
        }
    }

    ADVANCE(parser);
    (void) vector_shrink_to_fit(args);

    return AST_new_call_expr(ident_name, args);

cleanup:
    if (NULL != args)
    {
        (void) free(args);
        args = NULL;
    }

    return NULL;

}

t_ast_node *parse_primary(t_parser *parser)
{
    t_ast_node *n;
    t_token *token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);

    switch (token->type)
    {
    case T_IDENTIFIER:
    {
        return parse_ident_expr(parser);
    }
    case T_NUMBER:
    {
        n = AST_new_number(atoi(token->content));
        ADVANCE(parser);
        return n;
    }
    case T_OPEN_PAREN:
    {
        return parse_paren_expr(parser);
    }
    case T_STRING:
    {
        n = AST_new_string(token->content);
        ADVANCE(parser);
        return n;
    }

    default:
        (void) fprintf(stderr, "parse_primary: Syntax error at %ld:%ld - %s\n",
                       token->line, token->offset, token->content);
        (void) exit(1);
    }
}

bool should_finish_expression(t_token *token)
{
    if (T_OPEN_BRACKET == token->type)
    {
        return true;
    }

    if (T_CLOSE_BRACKET == token->type)
    {
        return true;
    }

    if (T_SEMI_COLON == token->type)
    {
        return true;
    }

    if (T_EOF == token->type)
    {
        return true;
    }

    if (T_COMMA == token->type)
    {
        return true;
    }

    if (T_CLOSE_PAREN == token->type)
    {
        return true;
    }

    return false;
}

t_ast_node *PARSER_parse_binexpr(t_parser *parser, int ptp)
{
    t_ast_node *left, *right;
    int nodetype;
    t_token *token = NULL;

    left = parse_primary(parser);

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);

    if (should_finish_expression(token))
    {
        return left;
    }

    while (EXPR_get_op_precedence(token) > ptp)
    {
        ADVANCE(parser);

        right = PARSER_parse_binexpr(parser, g_operator_precedence[token->type - T_PLUS]);
        left = AST_new_binary_expr(parse_op(token), left, right);

        token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);

        if (should_finish_expression(token))
        {
            return left;
        }
    }

    return left;
}

t_ast_node *parse_expression(t_parser *parser)
{
    t_ast_node *node = NULL, *expr = NULL, *cond = false;
    t_vector *then_body = NULL, *else_body = NULL;
    t_token *token = NULL;

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);

    switch (token->type)
    {
    case T_IF:
    {
        ADVANCE(parser);
        cond = parse_expression(parser);
        --parser->index;
        then_body = parse_statements(parser);
        if (EXPECT(parser, T_ELSE))
        {
            ADVANCE(parser);
            else_body = parse_statements(parser);
        }
        else
        {
            else_body = NULL;
        }
        ADVANCE(parser);

        node = AST_new_if_expr(cond, then_body, else_body);
        return node;
    };
    default:
    {
        return PARSER_parse_binexpr(parser, 0);
    }
    }
}

t_ast_node *parse_statement(t_parser *parser)
{
    t_ast_node *node = NULL, *expr = NULL, *var = NULL;
    t_token *token = NULL;

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    switch (token->type)
    {
    case T_RETURN:
    {
        ADVANCE(parser);
        expr = parse_expression(parser);
        MATCH_ADVANCE(parser, T_SEMI_COLON,
                      "Expected a ';' at the end of a return statement");
        node = AST_new_return_stmt(expr);
        return node;
    }
    case T_LET:
    {
        EXPECT_ADVANCE(parser, T_IDENTIFIER,
                       "Expected an identifier after a 'let'");
        token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
        var = AST_new_variable(token->content, TYPE_INT32);
        EXPECT_ADVANCE(parser, T_EQUALS,
                       "Expected a '=' after ident in variable declaration");
        ADVANCE(parser);
        expr = parse_expression(parser);
        node = AST_new_let_stmt(var, expr);
        MATCH_ADVANCE(parser, T_SEMI_COLON, "Expected a ';' after let statement");
        return node;
    }
    default:
    {
        expr = parse_expression(parser);
        token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);

        if (T_SEMI_COLON == token->type)
        {
            /* Expression Statement */
            ADVANCE(parser);
            node = AST_new_expression_stmt(expr);
            return node;
        }

        if (should_finish_expression(token))
        {
            return expr;
        }
        (void) fprintf(stderr, "Not a statement: %ld:%ld - %s\n", token->line,
                       token->offset, token->content);
        (void) exit(1);
    }
    }

    return NULL;
}

t_vector *parse_statements(t_parser *parser)
{
    t_vector *stmts = NULL;
    t_ast_node *stmt = NULL;
    t_token *token = NULL;
    stmts = calloc(1, sizeof(t_vector));
    if (NULL == stmts)
    {
        (void) fprintf(stderr, "Couldn't allocate memory for statments.\n");
        goto cleanup;
    }

    (void) vector_setup(stmts, 10, sizeof(t_ast_node_ptr));

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index + 1);

    if (T_OPEN_BRACKET != token->type)
    {
        ADVANCE(parser);
        stmt = parse_statement(parser);
        vector_push_back(stmts, &stmt);
        --parser->index;
    }
    else
    {
        EXPECT_ADVANCE(parser, T_OPEN_BRACKET,
                       "Expected '{' to open a body of statements");

        while (
            T_CLOSE_BRACKET !=
            (token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index + 1))
                ->type)
        {
            ADVANCE(parser);
            stmt = parse_statement(parser);
            (void) vector_push_back(stmts, &stmt);
            --parser->index;
        }

        ADVANCE(parser);
    }
    (void) vector_shrink_to_fit(stmts);

cleanup:
    return stmts;
}

t_ast_node *parse_prototype(t_parser *parser)
{
    char *name = NULL;
    char **args = NULL, **new_args = NULL;
    t_type *types = NULL, *new_types = NULL, return_type;
    int arity = 0;
    size_t allocated = 6;

    t_token *token = NULL;

    EXPECT_ADVANCE(parser, T_IDENTIFIER,
                   "Expected an identifier after 'fn' keyword");
    name = (VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index))->content;

    EXPECT_ADVANCE(parser, T_OPEN_PAREN, "Expected a '('");

    if (T_CLOSE_PAREN ==
        (VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index + 1))->type)
    {
        // No args
        ADVANCE(parser);
        return_type = parse_type(parser);
        return AST_new_prototype(name, NULL, NULL, 0, return_type);
    }

    ADVANCE(parser);

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    args = calloc(allocated, sizeof(char *));
    if (NULL == args)
    {
        (void) fprintf(stderr, "Couldn't allocate memory for args.\n");
        goto cleanup;
    }
    types = calloc(allocated, sizeof(t_type));
    if (NULL == types)
    {
        (void) fprintf(stderr, "Couldn't allocate memory for types.\n");
        goto cleanup;
    }
    args[0] = strdup(token->content);
    types[0] = parse_type(parser);
    arity = 1;

    while (T_CLOSE_PAREN !=
           (token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index + 1))
               ->type)
    {
        EXPECT_ADVANCE(parser, T_COMMA, "Expected ',' after arg");
        EXPECT_ADVANCE(parser, T_IDENTIFIER, "Expected another arg after ','");
        token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
        ++arity;

        if (arity > allocated)
        {
            allocated *= 2;
            new_args = realloc(args, sizeof(char *) * allocated);
            if (NULL == new_args)
            {
                (void) fprintf(stderr, "Couldn't allocate memory for arguments.\n");
                goto cleanup;
            }
            args = new_args;

            new_types = realloc(types, sizeof(t_type) * allocated);
            if (NULL == new_types)
            {
                (void) fprintf(stderr, "Couldn't allocate memory for types.\n");
                goto cleanup;
            }
            types = new_types;
        }

        args[arity - 1] = strdup(token->content);
        types[arity - 1] = parse_type(parser);
    }
    EXPECT_ADVANCE(parser, T_CLOSE_PAREN, "Expected a ')'");

    return_type = parse_type(parser);

    if (arity != allocated)
    {
        new_args = realloc(args, sizeof(char *) * arity);
        if (NULL == new_args)
        {
            (void) fprintf(stderr, "Couldn't allocate memory for arguments.\n");
            goto cleanup;
        }
        args = new_args;

        new_types = realloc(types, sizeof(t_type) * arity);
        if (NULL == new_types)
        {
            (void) fprintf(stderr, "Couldn't allocate memory for types.\n");
            goto cleanup;
        }
        types = new_types;
    }

    return AST_new_prototype(name, args, types, arity, return_type);

cleanup:
    if (NULL != args)
    {
        (void) free(args);
        args = NULL;
    }

    if (NULL != types)
    {
        (void) free(types);
        types = NULL;
    }

    (void) exit(1);
}

t_ast_node *PARSER_parse_function(t_parser *parser)
{
    int number = 0;
    t_ast_node *prototype = NULL;
    t_vector *body = NULL;
    prototype = parse_prototype(parser);
    body = parse_statements(parser);
    return AST_new_function(prototype, body);
}

void PARSER_print_parser_tokens(t_parser *parser)
{

    t_token *token = NULL;
    t_iterator iterator = vector_begin(parser->tokens);
    t_iterator last = vector_end(parser->tokens);
    for (; !iterator_equals(&iterator, &last); iterator_increment(&iterator))
    {
        token = *(t_token_ptr *)iterator_get(&iterator);

        (void) printf("%ld:%ld - %d - %s\n", token->line, token->offset, token->type,
                      token->content);
    }
}
