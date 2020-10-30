#include "parser.h"
#include "ast.h"
#include "type.h"

#include <string.h>

t_ast_node *parser_parse_expression(t_parser *parser);
t_ast_node *parser_parse_equality(t_parser *parser);
t_ast_node *parser_parse_comparison(t_parser *parser);
t_ast_node *parser_parse_term(t_parser *parser);
t_ast_node *parser_parse_factor(t_parser *parser);
t_ast_node *parser_parse_unary(t_parser *parser);
t_ast_node *parser_parse_primary(t_parser *parser);

t_vector *parse_statements(t_parser *parser);
t_vector *parser_parse_struct_fields(t_parser *parser);
t_struct_field *parser_parse_struct_field(t_parser *parser);

t_ast_node *parse_prototype(t_parser *parser);
t_ast_node *parser_parse_assignment(t_parser *parser);
t_ast_node *parse_unary_expr(t_parser *parser);

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

t_type *parse_type(t_parser *parser, bool parse_prefix)
{
    t_token *token = NULL;
    t_type *type = NULL;
    t_type *inner_type = NULL;
    type = calloc(1, sizeof(t_type));
    if (NULL == type)
    {
        (void) exit(LUKA_CANT_ALLOC_MEMORY);
    }
    type->inner_type = NULL;
    type->payload = NULL;

    if (parse_prefix)
    {
        token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index + 1);
        if (T_COLON != token->type)
        {
            type->type = TYPE_SINT32;
            return type;
        }

        EXPECT_ADVANCE(parser, T_COLON, "Expected a `:` before type.");

    }

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index + 1);

    ADVANCE(parser);

    switch (token->type)
    {
    case T_ANY_TYPE:
        type->type = TYPE_ANY;
        break;
    case T_BOOL_TYPE:
        type->type = TYPE_BOOL;
        break;
    case T_S8_TYPE:
        type->type = TYPE_SINT8;
        break;
    case T_S16_TYPE:
        type->type = TYPE_SINT16;
        break;
    case T_S32_TYPE:
    case T_INT_TYPE:
        type->type = TYPE_SINT32;
        break;
    case T_S64_TYPE:
        type->type = TYPE_SINT64;
        break;
    case T_U8_TYPE:
    case T_CHAR_TYPE:
        type->type = TYPE_UINT8;
        break;
    case T_U16_TYPE:
        type->type = TYPE_UINT16;
        break;
    case T_U32_TYPE:
        type->type = TYPE_UINT32;
        break;
    case T_U64_TYPE:
        type->type = TYPE_UINT64;
        break;
    case T_F32_TYPE:
    case T_FLOAT_TYPE:
        type->type = TYPE_F32;
        break;
    case T_F64_TYPE:
    case T_DOUBLE_TYPE:
        type->type = TYPE_F64;
        break;
    case T_STR_TYPE:
        type->type = TYPE_STRING;
        break;
    case T_VOID_TYPE:
        type->type = TYPE_VOID;
        break;
    case T_STRUCT:
        type->type = TYPE_STRUCT;
        ADVANCE(parser);
        token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
        type->payload = (void *)strdup(token->content);
        break;
    default:
        (void) LOGGER_log(parser->logger, L_ERROR, "Unknown type %d %s. Fallbacking to s32.\n", token->type,
                        token->content);
        type->type = TYPE_SINT32;
    }

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index + 1);
    while (T_STAR == token->type)
    {
        inner_type = type;
        type = calloc(1, sizeof(t_type));
        if (NULL == type)
        {
            (void) exit(LUKA_CANT_ALLOC_MEMORY);
        }
        type->type = TYPE_PTR;
        type->inner_type = inner_type;
        ADVANCE(parser);
        token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index + 1);
    }

    return type;
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
    parser->struct_names = calloc(1, sizeof(t_vector));
    if (NULL == parser->struct_names)
    {
        (void) LOGGER_log(parser->logger, L_ERROR, "Cannot allocate memory for struct names vector.\n");
        (void) exit(LUKA_CANT_ALLOC_MEMORY);
    }

    (void) vector_setup(parser->struct_names, 1, sizeof(char *));
}

void PARSER_free(t_parser *parser)
{
    (void) assert(parser != NULL);
    (void) assert(parser->struct_names != NULL);
    (void) assert(vector_is_initialized(parser->struct_names));

    (void) vector_clear(parser->struct_names);
    (void) vector_destroy(parser->struct_names);
    (void) free(parser->struct_names);
    parser->struct_names = NULL;
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

t_ast_unop_type parse_unop(t_token *token, t_logger *logger)
{
    switch (token->type)
    {
    case T_PLUS:
        return UNOP_PLUS;
    case T_MINUS:
        return UNOP_MINUS;
    case T_STAR:
        return UNOP_DEREF;
    case T_AMPERCENT:
        return UNOP_REF;
    case T_BANG:
        return UNOP_NOT;
    default:
        (void) LOGGER_log(logger, L_ERROR, "Unknown token in parse_unop at %ld:%ld\n", token->line,
                          token->offset);
        (void) exit(1);
    }
}

t_ast_binop_type parse_binop(t_token *token, t_logger *logger)
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
    case T_EQEQ:
        return BINOP_EQUALS;
    case T_NEQ:
        return BINOP_NEQ;
    case T_LEQ:
        return BINOP_LEQ;
    case T_GEQ:
        return BINOP_GEQ;
    default:
        (void) LOGGER_log(logger, L_ERROR, "Unknown token in parse_binop at %ld:%ld\n", token->line,
                          token->offset);
        (void) exit(1);
    }
}

t_ast_node *parse_paren_expr(t_parser *parser)
{
    t_ast_node *expr;
    MATCH_ADVANCE(parser, T_OPEN_PAREN, "Expected '('");
    expr = parser_parse_expression(parser);
    MATCH_ADVANCE(parser, T_CLOSE_PAREN, "Expected ')'");
    return expr;
}

bool parser_is_struct_name(t_parser *parser, const char *ident_name)
{
    char *value = NULL;
    VECTOR_FOR_EACH(parser->struct_names, iterator)
    {
        value = ITERATOR_GET_AS(char *, &iterator);
        if (0 == strcmp(value, ident_name))
        {
            return true;
        }
    }

    return false;
}

t_ast_node *parse_ident_expr(t_parser *parser)
{
    t_token *token;
    t_ast_node *expr = NULL;
    char *ident_name = NULL;
    t_vector *args = NULL;
    bool mutable = false;
    t_vector *struct_value_fields = NULL;
    t_struct_value_field *struct_value_field = NULL;

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    ident_name = strdup(token->content);

    ADVANCE(parser);

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    if (MATCH(parser, T_DOT))
    {
        ADVANCE(parser);
        token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
        ADVANCE(parser);

        return AST_new_get_expr(ident_name, strdup(token->content));
    }
    else if (MATCH(parser , T_OPEN_BRACKET) && parser_is_struct_name(parser, ident_name))
    {
        ADVANCE(parser);
        struct_value_fields = calloc(1, sizeof(t_vector));
        if (NULL == struct_value_fields)
        {
            (void) LOGGER_log(parser->logger, L_ERROR, "Couldn't allocate memory for struct_value_fields.\n");
            goto cleanup;
        }

        (void) vector_setup(struct_value_fields, 5, sizeof(t_struct_value_field_ptr));


        while (true)
        {
            struct_value_field = calloc(1, sizeof(t_struct_value_field));
            if (NULL == struct_value_field)
            {
                (void) LOGGER_log(parser->logger, L_ERROR, "Couldn't allocate memory for struct_value_field.\n");
                goto cleanup;
            }

            token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
            struct_value_field->name = strdup(token->content);

            EXPECT_ADVANCE(parser, T_COLON, "Expected ':' after field name in struct value.\n");
            ADVANCE(parser);
            struct_value_field->expr = parser_parse_expression(parser);

            vector_push_back(struct_value_fields, &struct_value_field);

            if (MATCH(parser, T_CLOSE_BRACKET))
            {
                ADVANCE(parser);
                break;
            }

            MATCH_ADVANCE(parser, T_COMMA, "Expected '}' or ',' after struct value field.\n");
        }

        return AST_new_struct_value(ident_name, struct_value_fields);
    }

    if (T_OPEN_PAREN != token->type)
    {
        if (MATCH(parser, T_MUT))
        {
            ADVANCE(parser);
            mutable = true;
        }
        return AST_new_variable(ident_name, parse_type(parser, true), mutable);
    }

    ADVANCE(parser);
    args = calloc(1, sizeof(t_vector));
    if (NULL == args)
    {
        (void) LOGGER_log(parser->logger, L_ERROR, "Couldn't allocate memory for args.\n");
        goto cleanup;
    }
    vector_setup(args, 10, sizeof(t_ast_node));
    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    if (T_CLOSE_PAREN != token->type)
    {
        while (true)
        {
            expr = parser_parse_expression(parser);
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
        t_ast_node *node = NULL;
        t_iterator iterator = vector_begin(args);
        t_iterator last = vector_end(args);

        for (; !iterator_equals(&iterator, &last); iterator_increment(&iterator))
        {
            node = *(t_ast_node_ptr *)iterator_get(&iterator);
            AST_free_node(node, parser->logger);
        }

        (void) vector_clear(args);
        (void) vector_destroy(args);
        (void) free(args);
        args = NULL;
    }

    if (NULL != struct_value_fields)
    {
        t_struct_value_field *struct_value_field = NULL;
        t_iterator iterator = vector_begin(struct_value_fields);
        t_iterator last = vector_end(struct_value_fields);

        for (; !iterator_equals(&iterator, &last); iterator_increment(&iterator))
        {
            struct_value_field = ITERATOR_GET_AS(t_struct_value_field_ptr, &iterator);
            if (NULL == struct_value_field)
            {
                continue;
            }

            if (NULL != struct_value_field->name)
            {
                (void) free(struct_value_field->name);
                struct_value_field->name = NULL;
            }

            if (NULL != struct_value_field->expr)
            {
                (void) AST_free_node(struct_value_field->expr, parser->logger);
                struct_value_field->expr = NULL;
            }

            (void) free(struct_value_field);
            struct_value_field = NULL;
        }

        (void) vector_clear(struct_value_fields);
        (void) vector_destroy(struct_value_fields);
        (void) free(struct_value_fields);
        struct_value_fields = NULL;
    }

    if (NULL != struct_value_field)
    {
        if (NULL != struct_value_field->name)
        {
            (void) free(struct_value_field->name);
            struct_value_field->name = NULL;
        }

        if (NULL != struct_value_field->expr)
        {
            (void) AST_free_node(struct_value_field->expr, parser->logger);
            struct_value_field->expr = NULL;
        }

        (void) free(struct_value_field);
        struct_value_field = NULL;
    }

    if (NULL != expr)
    {
        (void) AST_free_node(expr, parser->logger);
        expr = NULL;
    }

    if (NULL != ident_name)
    {
        (void) free(ident_name);
        ident_name = NULL;
    }

    return NULL;

}

t_ast_node *parser_parse_equality(t_parser *parser)
{
    t_ast_node *lhs = NULL, *rhs = NULL;
    t_token *token = NULL;
    t_ast_binop_type operator = BINOP_NEQ;

    lhs = parser_parse_comparison(parser);
    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    switch (token->type)
    {
        case T_NEQ:
        {
            operator = BINOP_NEQ;
            break;
        }
        case T_EQEQ:
        {
            operator = BINOP_EQUALS;
            break;
        }
        default:
        {
            return lhs;
        }
    }

    ADVANCE(parser);
    rhs = parser_parse_comparison(parser);
    return AST_new_binary_expr(operator, lhs, rhs);
}

t_ast_node *parser_parse_comparison(t_parser *parser)
{
    t_ast_node *lhs = NULL, *rhs = NULL;
    t_token *token = NULL;
    t_ast_binop_type operator = BINOP_GREATER;

    lhs = parser_parse_term(parser);
    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    switch (token->type)
    {
        case T_CLOSE_ANG:
        {
            operator = BINOP_GREATER;
            break;
        }
        case T_GEQ:
        {
            operator = BINOP_GEQ;
            break;
        }
        case T_OPEN_ANG:
        {
            operator = BINOP_LESSER;
            break;
        }
        case T_LEQ:
        {
            operator = BINOP_LEQ;
            break;
        }
        default:
        {
            return lhs;
        }
    }

    ADVANCE(parser);
    rhs = parser_parse_term(parser);
    return AST_new_binary_expr(operator, lhs, rhs);
}

t_ast_node *parser_parse_term(t_parser *parser)
{
    t_ast_node *lhs = NULL, *rhs = NULL;
    t_token *token = NULL;
    t_ast_binop_type operator = BINOP_SUBTRACT;

    lhs = parser_parse_factor(parser);
    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    switch (token->type)
    {
        case T_MINUS:
        {
            operator = BINOP_SUBTRACT;
            break;
        }
        case T_PLUS:
        {
            operator = BINOP_ADD;
            break;
        }
        default:
        {
            return lhs;
        }
    }

    ADVANCE(parser);
    rhs = parser_parse_factor(parser);
    return AST_new_binary_expr(operator, lhs, rhs);
}


t_ast_node *parser_parse_factor(t_parser *parser)
{
    t_ast_node *lhs = NULL, *rhs = NULL;
    t_token *token = NULL;
    t_ast_binop_type operator = BINOP_DIVIDE;

    lhs = parser_parse_unary(parser);
    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    switch (token->type)
    {
        case T_SLASH:
        {
            operator = BINOP_DIVIDE;
            break;
        }
        case T_STAR:
        {
            operator = BINOP_MULTIPLY;
            break;
        }
        case T_PERCENT:
        {
            operator = BINOP_MODULOS;
            break;
        }
        default:
        {
            return lhs;
        }
    }

    ADVANCE(parser);
    rhs = parser_parse_unary(parser);
    return AST_new_binary_expr(operator, lhs, rhs);
}

t_ast_node *parser_parse_unary(t_parser *parser)
{
    t_token *token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    t_ast_node *unary = NULL;

    switch (token->type)
    {
        case T_BANG:
        {
            ADVANCE(parser);
            unary = parser_parse_unary(parser);
            return AST_new_unary_expr(UNOP_NOT, unary);
        }
        case T_MINUS:
        {
            ADVANCE(parser);
            unary = parser_parse_unary(parser);
            return AST_new_unary_expr(UNOP_MINUS, unary);
        }
        case T_AMPERCENT:
        {
            ADVANCE(parser);
            unary = parser_parse_unary(parser);
            return AST_new_unary_expr(UNOP_REF, unary);
        }
        case T_STAR:
        {
            ADVANCE(parser);
            unary = parser_parse_unary(parser);
            return AST_new_unary_expr(UNOP_DEREF, unary);
        }
        default:
        {
            return parser_parse_primary(parser);
        }
    }
}

t_ast_node *parser_parse_primary(t_parser *parser)
{
    t_ast_node *n;
    t_token *token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    t_type *type = NULL;
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
        type = TYPE_initialize_type(TYPE_SINT32);
        if (TYPE_is_floating_point(token->content))
        {
            if ('f' == token->content[strlen(token->content) - 1])
            {
                type->type = TYPE_F32;
                f32 = strtof(token->content, NULL);
                n = AST_new_number(type, &f32);
            }
            else
            {
                type->type = TYPE_F64;
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
        n = AST_new_string(strdup(token->content));
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

    if (T_AS == token->type)
    {
        return true;
    }

    return false;
}

t_ast_node *parser_parse_expression(t_parser *parser)
{
    t_ast_node *node = NULL, *cond = false;
    t_vector *then_body = NULL, *else_body = NULL, *body = NULL;
    t_token *token = NULL;
    t_type *type = NULL;

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);

    switch (token->type)
    {
    case T_IF:
    {
        ADVANCE(parser);
        cond = parser_parse_expression(parser);
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
        --parser->index;
        node = AST_new_if_expr(cond, then_body, else_body);
        ADVANCE(parser);
        break;
    };
    case T_WHILE:
    {
        ADVANCE(parser);
        cond = parser_parse_expression(parser);
        --parser->index;
        body = parse_statements(parser);
        node = AST_new_while_expr(cond, body);
        break;
    }
    default:
    {
        node = parser_parse_assignment(parser);
        break;
    }
    }

    if (MATCH(parser, T_AS))
    {
        type = parse_type(parser, false);
        ADVANCE(parser);
        return AST_new_cast_expr(node, type);
    }

    return node;
}

t_ast_node *parser_parse_assignment(t_parser *parser)
{
    t_ast_node *lhs = NULL, *rhs = NULL;

    lhs = parser_parse_equality(parser);

    if (MATCH(parser, T_EQUALS))
    {
        ADVANCE(parser);
        rhs = parser_parse_assignment(parser);

        if ((AST_TYPE_VARIABLE == lhs->type) || ((AST_TYPE_UNARY_EXPR == lhs->type) && (UNOP_DEREF == lhs->unary_expr.operator)))
        {
            return AST_new_assignment_expr(lhs, rhs);
        }

        (void) LOGGER_log(parser->logger, L_ERROR, "Invalid assignment target.");
        (void) exit(LUKA_GENERAL_ERROR);
    }

    return lhs;
}

t_ast_node *parse_statement(t_parser *parser)
{
    t_ast_node *node = NULL, *expr = NULL, *var = NULL;
    t_token *token = NULL;
    bool mutable = false;
    t_type *type = NULL;
    char *name = NULL;
    t_vector *fields = NULL;

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    switch (token->type)
    {
    case T_RETURN:
    {
        ADVANCE(parser);
        expr = parser_parse_expression(parser);
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
            type = parse_type(parser, true);
        }
        EXPECT_ADVANCE(parser, T_EQUALS,
                       "Expected a '=' after ident in variable declaration");
        ADVANCE(parser);
        expr = parser_parse_expression(parser);
        var = AST_new_variable(strdup(token->content), type, mutable);
        node = AST_new_let_stmt(var, expr);
        MATCH_ADVANCE(parser, T_SEMI_COLON, "Expected a ';' after let statement");
        return node;
    }
    case T_BREAK:
    {
        EXPECT_ADVANCE(parser, T_SEMI_COLON, "Expected a ';' after 'break'");
        ADVANCE(parser);
        return AST_new_break_stmt();
    }
    case T_STRUCT:
    {
        EXPECT_ADVANCE(parser, T_IDENTIFIER, "Expected an identifier after keyword 'struct'");
        token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
        name = strdup(token->content);
        EXPECT_ADVANCE(parser, T_OPEN_BRACKET, "Expected a '{' after identifier in struct definition");
        ADVANCE(parser);
        fields = parser_parse_struct_fields(parser);
        MATCH_ADVANCE(parser, T_CLOSE_BRACKET, "Expected a '}' after struct fields in struct definition");
        node = AST_new_struct_definition(name, fields);
        (void) vector_push_front(parser->struct_names, &name);
        return node;
    }
    default:
    {
        expr = parser_parse_expression(parser);
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
    t_type **types = NULL, **new_types = NULL, *return_type = NULL;
    size_t arity = 0;
    size_t allocated = 6;
    bool vararg = false;

    t_token *token = NULL;

    EXPECT_ADVANCE(parser, T_IDENTIFIER,
                   "Expected an identifier after 'fn' keyword");
    name = strdup((VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index))->content);

    EXPECT_ADVANCE(parser, T_OPEN_PAREN, "Expected a '('");

    if (T_CLOSE_PAREN ==
        (VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index + 1))->type)
    {
        // No args
        ADVANCE(parser);
        return_type = parse_type(parser, true);
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
        types[0] = parse_type(parser, true);
        if (TYPE_ANY != types[0])
        {
            types[0]->type = TYPE_ANY;
            types[0]->inner_type = NULL;
            types[0]->payload = NULL;
        }
        vararg = true;
    }
    else
    {
        types[0] = parse_type(parser, true);
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
            types[arity - 1] = parse_type(parser, true);
            if (TYPE_ANY != types[arity - 1])
            {
                types[arity - 1]->type = TYPE_ANY;
                types[arity - 1]->inner_type = NULL;
                types[arity - 1]->payload = NULL;
            }
            vararg = true;
        }
        else
        {
            types[arity - 1] = parse_type(parser, true);
        }
        args[arity - 1] = strdup(token->content);
    }

    EXPECT_ADVANCE(parser, T_CLOSE_PAREN, "Expected a ')'");

    return_type = parse_type(parser, true);

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


t_vector *parser_parse_struct_fields(t_parser *parser)
{
    t_vector *fields = NULL;
    t_struct_field *struct_field = NULL;
    t_token *token = NULL;

    fields = calloc(1, sizeof(t_vector));
    if (NULL == fields)
    {
        (void) LOGGER_log(parser->logger, L_ERROR, "Couldn't allocate memory for struct fields vector.\n");
        goto cleanup;
    }

    vector_setup(fields, 5, sizeof(t_struct_field_ptr));

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    if (T_CLOSE_BRACKET != token->type)
    {
        while (true)
        {
            struct_field = parser_parse_struct_field(parser);
            if (NULL == struct_field)
            {
                goto cleanup;
            }
            vector_push_back(fields, &struct_field);

            token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
            if (T_CLOSE_BRACKET == token->type)
            {
                break;
            }

            MATCH_ADVANCE(parser, T_COMMA, "Expected '}' or ',' after a struct field.");

        }
    }

    vector_shrink_to_fit(fields);
    return fields;

cleanup:

    VECTOR_FOR_EACH(fields, field) {
        struct_field = ITERATOR_GET_AS(t_struct_field_ptr, &field);
        if (NULL != struct_field)
        {
            if (NULL != struct_field->name)
            {
                (void) free(struct_field->name);
                struct_field->name = NULL;
            }

            if (NULL != struct_field->type)
            {
                (void) TYPE_free_type(struct_field->type);
                struct_field->type = NULL;
            }
            (void) free(struct_field);
            struct_field = NULL;
        }

    }
    (void) vector_clear(fields);
    (void) vector_destroy(fields);
    (void) free(fields);
    fields = NULL;
    return NULL;
}

t_struct_field *parser_parse_struct_field(t_parser *parser)
{
    t_token *token = NULL;
    t_struct_field *struct_field = calloc(1, sizeof(t_struct_field));
    if (NULL == struct_field)
    {
        goto cleanup;
    }

    token = VECTOR_GET_AS(t_token_ptr, parser->tokens, parser->index);
    MATCH_ADVANCE(parser, T_IDENTIFIER, "Expected an identifier as a struct field name");
    --parser->index;
    struct_field->name = strdup(token->content);
    struct_field->type = parse_type(parser, true);
    ADVANCE(parser);

    return struct_field;

cleanup:
    if (NULL != struct_field)
    {
        if (NULL != struct_field->name)
        {
            (void) free(struct_field->name);
            struct_field->name = NULL;
        }

        if (NULL != struct_field->type)
        {
            (void) TYPE_free_type(struct_field->type);
            struct_field->type = NULL;
        }
        (void) free(struct_field);
        struct_field = NULL;
    }

    return NULL;
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
