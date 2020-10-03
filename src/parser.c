#include "parser.h"
#include "ast.h"
#include "expr.h"
#include "type.h"

#include <string.h>

t_ast_node *parse_expression(t_parser *parser);

t_vector *parse_statements(t_parser *parser);

t_ast_node *parse_prototype(t_parser *parser);
t_ast_node *parse_assignment_stmt(t_parser *parser, char *var_name);

void ERR(t_parser *parser, const char *message)
{
    t_token *token = NULL;
    token = *(t_token_ptr *)vector_get(parser->tokens, parser->index + 1);

    (void) LOGGER_log(parser->logger, L_ERROR, "%s\n", message);

    (void) LOGGER_log(parser->logger, L_ERROR, "Error at %s %ld:%ld - %s\n", parser->file_path, token->line,
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

bool parser_is_compound_expr(t_ast_node *node)
{
    return (node->type == AST_TYPE_WHILE_EXPR) || (node->type == AST_TYPE_IF_EXPR);
}

t_type parse_type(t_parser *parser)
{
    t_token *token = NULL;

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index + 1);
    if (T_COLON != token->type)
    {
        return TYPE_SINT32;
    }

    EXPECT_ADVANCE(parser, T_COLON, "Expected a `:` before type.");

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index + 1);

    ADVANCE(parser);

    switch (token->type)
    {
    case T_ANY_TYPE:
        return TYPE_ANY;
    case T_BOOL_TYPE:
        return TYPE_BOOL;
    case T_S8_TYPE:
        return TYPE_SINT8;
    case T_S16_TYPE:
        return TYPE_SINT16;
    case T_S32_TYPE:
    case T_INT_TYPE:
        return TYPE_SINT32;
    case T_S64_TYPE:
        return TYPE_SINT64;
    case T_U8_TYPE:
    case T_CHAR_TYPE:
        return TYPE_UINT8;
    case T_U16_TYPE:
        return TYPE_UINT16;
    case T_U64_TYPE:
        return TYPE_UINT64;
    case T_F32_TYPE:
    case T_FLOAT_TYPE:
        return TYPE_F32;
    case T_F64_TYPE:
    case T_DOUBLE_TYPE:
        return TYPE_F64;
    case T_STR_TYPE:
        return TYPE_STRING;
    case T_VOID_TYPE:
        return TYPE_VOID;
    default:
        (void) LOGGER_log(parser->logger, L_ERROR, "Unknown type %d %s. Fallbacking to s32.\n", token->type,
                        token->content);
        return TYPE_SINT32;
    }
}

void PARSER_initialize(t_parser *parser,
                       t_vector *tokens,
                       const char *file_path,
                       t_logger* logger)
{
    (void) assert(parser != NULL);

    parser->tokens = tokens;
    parser->index = 0;
    parser->file_path = file_path;
    parser->logger = logger;
}

t_vector *PARSER_parse_top_level(t_parser *parser)
{
    t_vector *functions = NULL;
    t_token *token = NULL;
    t_ast_node *function, *prototype;

    functions = calloc(1, sizeof(t_vector));
    if (NULL == functions)
    {
        (void) LOGGER_log(parser->logger, L_ERROR, "Couldn't allocate memory for functions");
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
            (void) LOGGER_log(parser->logger, L_ERROR, "Syntax error at %s %ld:%ld - %s\n", parser->file_path,
                              token->line, token->offset, token->content);
        }
        }
        parser->index += 1;
    }

    (void) vector_shrink_to_fit(functions);

cleanup:
    return functions;
}

int parse_op(t_token *token, t_logger *logger)
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
        (void) LOGGER_log(logger, L_ERROR, "Unknown token in parse_op at %ld:%ld\n", token->line,
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
    bool mutable = false;

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    ident_name = token->content;

    ADVANCE(parser);
    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    if (T_EQUALS == token->type)
    {
        return parse_assignment_stmt(parser, ident_name);
    }

    if (T_OPEN_PAREN != token->type)
    {
        if (MATCH(parser, T_MUT))
        {
            ADVANCE(parser);
            mutable = true;
        }
        return AST_new_variable(ident_name, parse_type(parser), mutable);
    }

    ADVANCE(parser);
    args = calloc(1, sizeof(t_vector));
    if (NULL == args)
    {
        (void) LOGGER_log(parser->logger, L_ERROR, "Couldn't allocate memory for args.\n");
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
    t_type type = TYPE_SINT32;
    int32_t s32;
    double f64;
    float f32;


    switch (token->type)
    {
    case T_IDENTIFIER:
    {
        return parse_ident_expr(parser);
    }
    case T_NUMBER:
    {
        if (TYPE_is_floating_point(token->content))
        {
            if ('f' == token->content[strlen(token->content) - 1])
            {
                type = TYPE_F32;
                f32 = strtof(token->content, NULL);
                n = AST_new_number(type, &f32);
            }
            else
            {
                type = TYPE_F64;
                f64 = strtod(token->content, NULL);
                n = AST_new_number(type, &f64);
            }
        }
        else
        {
            s32 = strtol(token->content, NULL, 10);
            n = AST_new_number(type, &s32);
        }
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
        (void) LOGGER_log(parser->logger, L_ERROR, "parse_primary: Syntax error at %ld:%ld - %s\n",
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

    while (EXPR_get_op_precedence(token, parser->logger) > ptp)
    {
        ADVANCE(parser);

        right = PARSER_parse_binexpr(parser, g_operator_precedence[token->type - T_PLUS]);
        left = AST_new_binary_expr(parse_op(token, parser->logger), left, right);

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
    t_vector *then_body = NULL, *else_body = NULL, *body = NULL;
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
        node = AST_new_if_expr(cond, then_body, else_body);
        ADVANCE(parser);
        return node;
    };
    case T_WHILE:
    {
        ADVANCE(parser);
        cond = parse_expression(parser);
        --parser->index;
        body = parse_statements(parser);
        node = AST_new_while_expr(cond, body);
        return node;
    }
    default:
    {
        return parser_parse_binexpr(parser, 0);
    }
    }
}

t_ast_node *parse_assignment_stmt(t_parser *parser, char *var_name)
{
    t_ast_node *node = NULL, *expr = NULL;
    MATCH_ADVANCE(parser, T_EQUALS, "Expected a '=' after identifier in assignment");
    expr = parse_expression(parser);
    node = AST_new_assignment_stmt(var_name, expr);
    MATCH_ADVANCE(parser, T_SEMI_COLON, "Expected a ';' after let statement");
    --parser->index;
    return node;
}

t_ast_node *parse_statement(t_parser *parser)
{
    t_ast_node *node = NULL, *expr = NULL, *var = NULL;
    t_token *token = NULL;
    char *var_name = NULL;
    bool mutable = false;
    t_type type = TYPE_SINT32;

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
        if (EXPECT(parser, T_MUT))
        {
            ADVANCE(parser);
            mutable = true;
        }
        EXPECT_ADVANCE(parser, T_IDENTIFIER,
                       "Expected an identifier after a 'let'");
        token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
        if (EXPECT(parser, T_COLON))
        {
            type = parse_type(parser);
        }
        EXPECT_ADVANCE(parser, T_EQUALS,
                       "Expected a '=' after ident in variable declaration");
        ADVANCE(parser);
        expr = parse_expression(parser);
        var = AST_new_variable(token->content, type, mutable);
        node = AST_new_let_stmt(var, expr);
        MATCH_ADVANCE(parser, T_SEMI_COLON, "Expected a ';' after let statement");
        return node;
    }
    default:
    {
        expr = parse_expression(parser);
        token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);

        if ((T_SEMI_COLON == token->type) || (parser_is_compound_expr(expr)))
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
        (void) LOGGER_log(parser->logger, L_ERROR, "Not a statement: %ld:%ld - %s\n", token->line,
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
        (void) LOGGER_log(parser->logger, L_ERROR, "Couldn't allocate memory for statments.\n");
        goto cleanup;
    }

    (void) vector_setup(stmts, 10, sizeof(t_ast_node_ptr));

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index + 1);

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
    bool vararg = false;

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
        return AST_new_prototype(name, NULL, NULL, 0, return_type, vararg);
    }

    ADVANCE(parser);

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    args = calloc(allocated, sizeof(char *));
    if (NULL == args)
    {
        (void) LOGGER_log(parser->logger, L_ERROR, "Couldn't allocate memory for args.\n");
        goto cleanup;
    }
    types = calloc(allocated, sizeof(t_type));
    if (NULL == types)
    {
        (void) LOGGER_log(parser->logger, L_ERROR, "Couldn't allocate memory for types.\n");
        goto cleanup;
    }

    if (T_THREE_DOTS == token->type)
    {
        types[0] = parse_type(parser);
        if (TYPE_ANY != types[0])
        {
            types[0] = TYPE_ANY;
        }
        vararg = true;
    }
    else
    {
        types[0] = parse_type(parser);
    }
    args[0] = strdup(token->content);
    arity = 1;

    while ((T_CLOSE_PAREN !=
           (token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index + 1))
               ->type) && !vararg)
    {
        EXPECT_ADVANCE(parser, T_COMMA, "Expected ',' after arg");
        if (!(EXPECT(parser, T_IDENTIFIER) || EXPECT(parser, T_THREE_DOTS)))
        {
            ERR(parser, "Expected another arg after ','");
        }
        ADVANCE(parser);
        token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
        ++arity;

        if (arity > allocated)
        {
            allocated *= 2;
            new_args = realloc(args, sizeof(char *) * allocated);
            if (NULL == new_args)
            {
                (void) LOGGER_log(parser->logger, L_ERROR, "Couldn't allocate memory for arguments.\n");
                goto cleanup;
            }
            args = new_args;

            new_types = realloc(types, sizeof(t_type) * allocated);
            if (NULL == new_types)
            {
                (void) LOGGER_log(parser->logger, L_ERROR, "Couldn't allocate memory for types.\n");
                goto cleanup;
            }
            types = new_types;
        }

        if (T_THREE_DOTS == token->type)
        {
            types[arity - 1] = parse_type(parser);
            if (TYPE_ANY != types[arity - 1])
            {
                types[arity - 1] = TYPE_ANY;
            }
            vararg = true;
        }
        else
        {
            types[arity - 1] = parse_type(parser);
        }
        args[arity - 1] = strdup(token->content);
    }

    EXPECT_ADVANCE(parser, T_CLOSE_PAREN, "Expected a ')'");

    return_type = parse_type(parser);

    if (arity != allocated)
    {
        new_args = realloc(args, sizeof(char *) * arity);
        if (NULL == new_args)
        {
            (void) LOGGER_log(parser->logger, L_ERROR, "Couldn't allocate memory for arguments.\n");
            goto cleanup;
        }
        args = new_args;

        new_types = realloc(types, sizeof(t_type) * arity);
        if (NULL == new_types)
        {
            (void) LOGGER_log(parser->logger, L_ERROR, "Couldn't allocate memory for types.\n");
            goto cleanup;
        }
        types = new_types;
    }

    return AST_new_prototype(name, args, types, arity, return_type, vararg);

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

        (void) LOGGER_log(parser->logger,
                          L_DEBUG,
                          "%ld:%ld - %d - %s\n",
                          token->line,
                          token->offset,
                          token->type,
                          token->content);
    }
}
