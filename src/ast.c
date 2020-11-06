#include "ast.h"

#include <stdio.h>
#include <string.h>

#include "type.h"

const char *ast_type_to_string(t_type *type, t_logger *logger, char *buffer, size_t buffer_size)
{
    switch (type->type)
    {
    case TYPE_ANY:
        (void) snprintf(buffer, buffer_size, "any");
        break;
    case TYPE_BOOL:
        (void) snprintf(buffer, buffer_size, "bool");
        break;
    case TYPE_SINT8:
        (void) snprintf(buffer, buffer_size, "s8");
        break;
    case TYPE_SINT16:
        (void) snprintf(buffer, buffer_size, "s16");
        break;
    case TYPE_SINT32:
        (void) snprintf(buffer, buffer_size, "s32");
        break;
    case TYPE_SINT64:
        (void) snprintf(buffer, buffer_size, "s64");
        break;
    case TYPE_UINT8:
        (void) snprintf(buffer, buffer_size, "u8");
        break;
    case TYPE_UINT16:
        (void) snprintf(buffer, buffer_size, "u16");
        break;
    case TYPE_UINT32:
        (void) snprintf(buffer, buffer_size, "u32");
        break;
    case TYPE_UINT64:
        (void) snprintf(buffer, buffer_size, "u64");
        break;
    case TYPE_F32:
        (void) snprintf(buffer, buffer_size, "float");
        break;
    case TYPE_F64:
        (void) snprintf(buffer, buffer_size, "double");
        break;
    case TYPE_STRING:
        (void) snprintf(buffer, buffer_size, "str");
        break;
    case TYPE_VOID:
        (void) snprintf(buffer, buffer_size, "void");
        break;
    case TYPE_PTR:
        (void) ast_type_to_string(type->inner_type, logger, buffer, buffer_size);
        (void) snprintf(buffer + strlen(buffer), buffer_size, "*");
        break;
    default:
        (void) LOGGER_log(logger, L_ERROR, "I don't know how to translate type %d to LLVM types.\n",
                type);
        (void) snprintf(buffer, buffer_size, "s32");
        break;
    }

    return buffer;
}

t_ast_node *AST_new_number(t_type *type, void *value)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_NUMBER;
    node->number.type = type;
    switch (node->number.type->type)
    {
        case TYPE_F32:
            node->number.value.f32 = *(float *)value;
            break;
        case TYPE_F64:
            node->number.value.f64 = *(double *)value;
            break;
        case TYPE_SINT8:
            node->number.value.s8 = *(int8_t *)value;
            break;
        case TYPE_SINT16:
            node->number.value.s16 = *(int16_t *)value;
            break;
        case TYPE_SINT32:
            node->number.value.s32 = *(int32_t *)value;
            break;
        case TYPE_SINT64:
            node->number.value.s64 = *(int64_t *)value;
            break;
        case TYPE_UINT8:
            node->number.value.u8 = *(uint8_t *)value;
            break;
        case TYPE_UINT16:
            node->number.value.u16 = *(uint16_t *)value;
            break;
        case TYPE_UINT32:
            node->number.value.u32 = *(uint32_t *)value;
            break;
        case TYPE_UINT64:
            node->number.value.u64 = *(uint64_t *)value;
            break;
        default:
            (void) fprintf(stderr, "%d is not a number type.\n", type->type);
            (void) exit(LUKA_GENERAL_ERROR);
    }
    return node;
}

t_ast_node *AST_new_string(char *value)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_STRING;
    node->string.value = value;
    node->string.length = strlen(value);
    return node;
}

t_ast_node *AST_new_unary_expr(t_ast_unop_type operator, t_ast_node *rhs)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_UNARY_EXPR;
    node->unary_expr.operator= operator;
    node->unary_expr.rhs = rhs;
    return node;
}

t_ast_node *AST_new_binary_expr(t_ast_binop_type operator, t_ast_node * lhs,
                                t_ast_node * rhs)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_BINARY_EXPR;
    node->binary_expr.operator= operator;
    node->binary_expr.lhs = lhs;
    node->binary_expr.rhs = rhs;
    return node;
}

t_ast_node *AST_new_prototype(char *name, char **args, t_type **types, int arity,
                              t_type *return_type, bool vararg)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_PROTOTYPE;
    node->prototype.name = name;
    node->prototype.args = args;
    node->prototype.types = types;
    node->prototype.return_type = return_type;
    node->prototype.arity = arity;
    node->prototype.vararg = vararg;

    return node;
}

t_ast_node *AST_new_function(t_ast_node *prototype, t_vector *body)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_FUNCTION;
    node->function.prototype = prototype;
    node->function.body = body;
    return node;
}

t_ast_node *AST_new_return_stmt(t_ast_node *expr)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_RETURN_STMT;
    node->return_stmt.expr = expr;
    return node;
}

t_ast_node *AST_new_if_expr(t_ast_node *cond, t_vector *then_body, t_vector *else_body)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_IF_EXPR;
    node->if_expr.cond = cond;
    node->if_expr.then_body = then_body;
    node->if_expr.else_body = else_body;
    return node;
}

t_ast_node *AST_new_while_expr(t_ast_node *cond, t_vector *body)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_WHILE_EXPR;
    node->while_expr.cond = cond;
    node->while_expr.body = body;
    return node;
}

t_ast_node *AST_new_cast_expr(t_ast_node *expr, t_type *type)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_CAST_EXPR;
    node->cast_expr.expr = expr;
    node->cast_expr.type = type;
    return node;
}

t_ast_node *AST_new_variable(char *name, t_type *type, bool mutable)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_VARIABLE;
    node->variable.name = name;
    node->variable.type = type;
    node->variable.mutable = mutable;
    return node;
}

t_ast_node *AST_new_let_stmt(t_ast_node *var, t_ast_node *expr)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_LET_STMT;
    node->let_stmt.var = var;
    node->let_stmt.expr = expr;
    return node;
}

t_ast_node *AST_new_assignment_expr(t_ast_node *lhs, t_ast_node *rhs)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_ASSIGNMENT_EXPR;
    node->assignment_expr.lhs = lhs;
    node->assignment_expr.rhs = rhs;
    return node;
}

t_ast_node *AST_new_call_expr(char *name, t_vector *args)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_CALL_EXPR;
    node->call_expr.name = name;
    node->call_expr.args = args;
    return node;
}

t_ast_node *AST_new_expression_stmt(t_ast_node *expr)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_EXPRESSION_STMT;
    node->expression_stmt.expr = expr;
    return node;
}

t_ast_node *AST_new_break_stmt()
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_BREAK_STMT;
    return node;
}

t_ast_node *AST_new_struct_definition(char *name, t_vector *struct_fields)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_STRUCT_DEFINITION;
    node->struct_definition.name = name;
    node->struct_definition.struct_fields = struct_fields;
    return node;
}

t_ast_node *AST_new_struct_value(char *name, t_vector *struct_values)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_STRUCT_VALUE;
    node->struct_value.name = name;
    node->struct_value.struct_values = struct_values;
    return node;
}

t_ast_node *AST_new_enum_definition(char *name, t_vector *enum_fields)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_ENUM_DEFINITION;
    node->enum_definition.name = name;
    node->enum_definition.enum_fields = enum_fields;
    return node;
}

t_ast_node *AST_new_get_expr(char *variable, char *key, bool is_enum)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_GET_EXPR;
    node->get_expr.variable = variable;
    node->get_expr.key = key;
    node->get_expr.is_enum = is_enum;
    return node;
}

void AST_free_node(t_ast_node *node, t_logger *logger)
{
    if (NULL == node)
        return;

    switch (node->type)
    {
    case AST_TYPE_BREAK_STMT:
        break;
    case AST_TYPE_STRING:
    {
        if (NULL != node->string.value)
        {
            (void) free(node->string.value);
            node->string.value = NULL;
        }
        break;
    }
    case AST_TYPE_NUMBER:
    {
        if (NULL != node->number.type)
        {
            (void) TYPE_free_type(node->number.type);
            node->number.type = NULL;
        }
        break;
    }
    case AST_TYPE_VARIABLE:
    {
        if (NULL != node->variable.name)
        {
            (void) free(node->variable.name);
            node->variable.name = NULL;
        }

        if (NULL != node->variable.type)
        {
            (void) TYPE_free_type(node->variable.type);
            node->variable.type = NULL;
        }
        break;
    }
    case AST_TYPE_UNARY_EXPR:
    {
        if (NULL != node->unary_expr.rhs)
            (void) AST_free_node(node->unary_expr.rhs, logger);
        break;
    }
    case AST_TYPE_BINARY_EXPR:
    {
        if (NULL != node->binary_expr.lhs)
            (void) AST_free_node(node->binary_expr.lhs, logger);
        if (NULL != node->binary_expr.rhs)
            (void) AST_free_node(node->binary_expr.rhs, logger);
        break;
    }
    case AST_TYPE_PROTOTYPE:
    {
        if (NULL != node->prototype.name)
        {
            (void) free(node->prototype.name);
            node->prototype.name = NULL;
        }

        if (NULL != node->prototype.args)
        {
            for (size_t i = 0; i < node->prototype.arity; ++i)
            {
                if (NULL != node->prototype.args[i])
                {
                    if (NULL != node->prototype.args[i])
                    {
                        (void) free(node->prototype.args[i]);
                        node->prototype.args[i] = NULL;
                    }
                }
            }
            (void) free(node->prototype.args);
            node->prototype.args = NULL;
        }

        if (NULL != node->prototype.types)
        {
            for (size_t i = 0; i < node->prototype.arity; ++i)
            {
                if (NULL != node->prototype.types[i])
                {
                    if (NULL != node->prototype.types[i])
                    {
                        (void) TYPE_free_type(node->prototype.types[i]);
                        node->prototype.types[i] = NULL;
                    }
                }
            }
            (void) free(node->prototype.types);
            node->prototype.types = NULL;
        }

        if (NULL != node->prototype.return_type)
        {
            (void) TYPE_free_type(node->prototype.return_type);
            node->prototype.return_type = NULL;
        }

        break;
    }

    case AST_TYPE_FUNCTION:
    {
        if (NULL != node->function.prototype)
        {
            (void) AST_free_node(node->function.prototype, logger);
        }

        if (NULL != node->function.body)
        {
            t_ast_node *stmt = NULL;
            VECTOR_FOR_EACH(node->function.body, stmts)
            {
                stmt = ITERATOR_GET_AS(t_ast_node_ptr, &stmts);
                (void) AST_free_node(stmt, logger);
            }

            (void) vector_clear(node->function.body);
            (void) vector_destroy(node->function.body);
            (void) free(node->function.body);
            node->function.body = NULL;
        }
        break;
    }
    case AST_TYPE_RETURN_STMT:
    {
        if (NULL != node->return_stmt.expr)
            (void) AST_free_node(node->return_stmt.expr, logger);
        break;
    }
    case AST_TYPE_IF_EXPR:
    {
        if (NULL != node->if_expr.cond)
        {
            (void) AST_free_node(node->if_expr.cond, logger);
        }

        if (node->if_expr.then_body)
        {
            t_ast_node *stmt = NULL;
            VECTOR_FOR_EACH(node->if_expr.then_body, stmts)
            {
                stmt = ITERATOR_GET_AS(t_ast_node_ptr, &stmts);
                (void) AST_free_node(stmt, logger);
            }

            (void) vector_clear(node->if_expr.then_body);
            (void) vector_destroy(node->if_expr.then_body);
            (void) free(node->if_expr.then_body);
            node->if_expr.then_body = NULL;
        }
        if (NULL != node->if_expr.else_body)
        {
            t_ast_node *stmt = NULL;
            VECTOR_FOR_EACH(node->if_expr.else_body, stmts)
            {
                stmt = ITERATOR_GET_AS(t_ast_node_ptr, &stmts);
                (void) AST_free_node(stmt, logger);
            }

            (void) vector_clear(node->if_expr.else_body);
            (void) vector_destroy(node->if_expr.else_body);
            (void) free(node->if_expr.else_body);
            node->if_expr.else_body = NULL;
        }
        break;
    }
    case AST_TYPE_WHILE_EXPR:
    {
        if (NULL != node->while_expr.cond)
        {
            (void) AST_free_node(node->while_expr.cond, logger);
        }

        if (node->while_expr.body)
        {
            t_ast_node *stmt = NULL;
            VECTOR_FOR_EACH(node->while_expr.body, stmts)
            {
                stmt = ITERATOR_GET_AS(t_ast_node_ptr, &stmts);
                (void) AST_free_node(stmt, logger);
            }

            (void) vector_clear(node->while_expr.body);
            (void) vector_destroy(node->while_expr.body);
            (void) free(node->while_expr.body);
            node->while_expr.body = NULL;
        }
        break;
    }
    case AST_TYPE_CAST_EXPR:
    {
        if (NULL != node->cast_expr.expr)
        {
            (void) AST_free_node(node->cast_expr.expr, logger);
        }

        if (NULL != node->cast_expr.type)
        {
            (void) free(node->cast_expr.type);
            node->cast_expr.type = NULL;
        }
        break;
    }
    case AST_TYPE_LET_STMT:
    {
        if (NULL != node->let_stmt.var)
        {
            (void) AST_free_node(node->let_stmt.var, logger);
        }

        if (NULL != node->let_stmt.expr)
        {
            (void) AST_free_node(node->let_stmt.expr, logger);
        }
        break;
    }
    case AST_TYPE_ASSIGNMENT_EXPR:
    {
        if (NULL != node->assignment_expr.lhs)
        {
            (void) AST_free_node(node->assignment_expr.lhs, logger);
        }

        if (NULL != node->assignment_expr.rhs)
        {
            (void) AST_free_node(node->assignment_expr.rhs, logger);
        }
        break;
    }

    case AST_TYPE_CALL_EXPR:
    {
        if (NULL != node->call_expr.name)
        {
            (void) free(node->call_expr.name);
            node->call_expr.name = NULL;
        }

        if (NULL != node->call_expr.args)
        {
            t_ast_node *arg = NULL;
            VECTOR_FOR_EACH(node->call_expr.args, args)
            {
                arg = ITERATOR_GET_AS(t_ast_node_ptr, &args);
                (void) AST_free_node(arg, logger);
            }

            (void) vector_clear(node->call_expr.args);
            (void) vector_destroy(node->call_expr.args);
            (void) free(node->call_expr.args);
            node->call_expr.args = NULL;
        }
        break;
    }

    case AST_TYPE_EXPRESSION_STMT:
    {
        if (NULL != node->expression_stmt.expr)
            (void) AST_free_node(node->expression_stmt.expr, logger);
        break;
    }

    case AST_TYPE_STRUCT_DEFINITION:
    {
        if (NULL != node->struct_definition.name)
        {
            (void) free(node->struct_definition.name);
            node->struct_definition.name = NULL;
        }
        if (NULL != node->struct_definition.struct_fields)
        {
            t_struct_field *struct_field = NULL;
            VECTOR_FOR_EACH(node->struct_definition.struct_fields, struct_fields)
            {
                struct_field = ITERATOR_GET_AS(t_struct_field_ptr, &struct_fields);
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

            (void) vector_clear(node->struct_definition.struct_fields);
            (void) vector_destroy(node->struct_definition.struct_fields);
            (void) free(node->struct_definition.struct_fields);
            node->struct_definition.struct_fields = NULL;
        }
        break;
    }

    case AST_TYPE_STRUCT_VALUE:
    {
        if (NULL != node->struct_value.name)
        {
            (void) free(node->struct_value.name);
            node->struct_value.name = NULL;
        }
        if (NULL != node->struct_value.struct_values)
        {
            t_struct_value_field *struct_value = NULL;
            VECTOR_FOR_EACH(node->struct_value.struct_values, struct_values)
            {
                struct_value = ITERATOR_GET_AS(t_struct_value_field_ptr, &struct_values);
                if (NULL != struct_value)
                {
                    if (NULL != struct_value->name)
                    {
                        (void) free(struct_value->name);
                        struct_value->name = NULL;
                    }

                    if (NULL != struct_value->expr)
                    {
                        (void) AST_free_node(struct_value->expr, logger);
                        struct_value->expr = NULL;
                    }

                    (void) free(struct_value);
                    struct_value = NULL;
                }
            }

            (void) vector_clear(node->struct_value.struct_values);
            (void) vector_destroy(node->struct_value.struct_values);
            (void) free(node->struct_value.struct_values);
            node->struct_value.struct_values = NULL;
        }
        break;
    }

    case AST_TYPE_ENUM_DEFINITION:
    {
        if (NULL != node->enum_definition.name)
        {
            (void) free(node->enum_definition.name);
            node->enum_definition.name = NULL;
        }
        if (NULL != node->enum_definition.enum_fields)
        {
            t_enum_field *enum_field = NULL;
            VECTOR_FOR_EACH(node->enum_definition.enum_fields, enum_fields)
            {
                enum_field = ITERATOR_GET_AS(t_enum_field_ptr, &enum_fields);
                if (NULL != enum_field)
                {
                    if (NULL != enum_field->name)
                    {
                        (void) free(enum_field->name);
                        enum_field->name = NULL;
                    }

                    if (NULL != enum_field->expr)
                    {
                        (void) AST_free_node(enum_field->expr, logger);
                        enum_field->expr = NULL;
                    }

                    (void) free(enum_field);
                    enum_field = NULL;
                }
            }

            (void) vector_clear(node->enum_definition.enum_fields);
            (void) vector_destroy(node->enum_definition.enum_fields);
            (void) free(node->enum_definition.enum_fields);
            node->enum_definition.enum_fields = NULL;
        }
        break;
    }

    case AST_TYPE_GET_EXPR:
    {
        if (NULL != node->get_expr.variable)
        {
            (void) free(node->get_expr.variable);
            node->get_expr.variable = NULL;
        }

        if (NULL != node->get_expr.key)
        {
            (void) free(node->get_expr.key);
            node->get_expr.key = NULL;
        }
        break;
    }

    default:
    {
        (void) LOGGER_log(logger, L_ERROR, "I don't now how to free type - %d\n", node->type);
        break;
    }
    }

    (void) free(node);
    node = NULL;
}

void ast_print_statements_block(t_vector *statements, int offset, t_logger *logger)
{
    t_ast_node *stmt = NULL;

    (void) LOGGER_log(logger, L_DEBUG, "%*c\b Statements block\n", offset, ' ');
    VECTOR_FOR_EACH(statements, stmts)
    {
        stmt = ITERATOR_GET_AS(t_ast_node_ptr, &stmts);
        (void) AST_print_ast(stmt, offset + 2, logger);
    }
}

void AST_print_functions(t_vector *functions, int offset, t_logger *logger)
{
    t_ast_node *func = NULL;
    VECTOR_FOR_EACH(functions, funcs)
    {
        func = ITERATOR_GET_AS(t_ast_node_ptr, &funcs);
        (void) AST_print_ast(func, offset, logger);
    }
}

char *unop_to_str(t_ast_unop_type op, t_logger *logger)
{
    switch (op)
    {
    case UNOP_NOT:
        return "!";
    case UNOP_MINUS:
        return "-";
    case UNOP_PLUS:
        return "+";
    case UNOP_DEREF:
        return "*";
    case UNOP_REF:
        return "&";
    default:
        (void) LOGGER_log(logger, L_ERROR, "Unknown unary operator: %d\n", op);
        (void) exit(1);
    }
}

char *binop_to_str(t_ast_binop_type op, t_logger *logger)
{
    switch (op)
    {
    case BINOP_ADD:
        return "+";
    case BINOP_SUBTRACT:
        return "-";
    case BINOP_MULTIPLY:
        return "*";
    case BINOP_DIVIDE:
        return "/";
    case BINOP_MODULOS:
        return "%";
    case BINOP_LESSER:
        return "<";
    case BINOP_GREATER:
        return ">";
    case BINOP_EQUALS:
        return "==";
    case BINOP_NEQ:
        return "!=";
    case BINOP_LEQ:
        return "<=";
    case BINOP_GEQ:
        return ">=";
    default:
        (void) LOGGER_log(logger, L_ERROR, "Unknown binary operator: %d\n", op);
        (void) exit(1);
    }
}

char *ast_stringify(const char* source, size_t source_length, t_logger *logger)
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

void AST_print_ast(t_ast_node *node, int offset, t_logger *logger)
{
    t_ast_node *arg = NULL;
    t_struct_field *struct_field = NULL;
    t_struct_value_field *value_field = NULL;
    size_t i = 0;
    char type_str[256] = {0};
    t_enum_field *enum_field = NULL;

    if (NULL == node)
        return;

    if (0 == offset)
    {
        (void) LOGGER_log(logger, L_DEBUG, "Printing AST:\n");
    }

    switch (node->type)
    {
    case AST_TYPE_NUMBER:
        if (TYPE_is_floating_type(node->number.type))
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b AST number %lf\n", offset, ' ', node->number.value.f64);
        }
        else
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b AST number %ld\n", offset, ' ', node->number.value.u64);
        }
        break;
    case AST_TYPE_STRING:
    {
        char *stringified = ast_stringify(node->string.value, node->string.length, logger);
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b AST string \"%s\"\n", offset, ' ', stringified);
        (void) free(stringified);
        break;
    }
    case AST_TYPE_UNARY_EXPR:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Unary Expression\n", offset, ' ');
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Operator: %s\n", offset + 2, ' ',
                          unop_to_str(node->unary_expr.operator, logger));

        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Expression:\n", offset + 2, ' ');
        if (NULL != node->unary_expr.rhs)
        {
            (void) AST_print_ast(node->unary_expr.rhs, offset + 4, logger);
        }
        break;
    }
    case AST_TYPE_BINARY_EXPR:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Binary Expression\n", offset, ' ');
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Operator: %s\n", offset + 2, ' ',
                          binop_to_str(node->binary_expr.operator, logger));
        if (NULL != node->binary_expr.lhs)
            (void) AST_print_ast(node->binary_expr.lhs, offset + 4, logger);
        if (NULL != node->binary_expr.rhs)
            (void) AST_print_ast(node->binary_expr.rhs, offset + 4, logger);
        break;
    }
    case AST_TYPE_PROTOTYPE:
    {
        if (node->prototype.vararg)
        {
            (void) LOGGER_log(logger,
                              L_DEBUG,
                              "%*c\b %s - %d required args (VarArg)\n",
                              offset,
                              '-',
                              node->prototype.name,
                              node->prototype.arity - 1);
            for (i = 0; i < node->prototype.arity - 1; ++i)
            {
                (void) memset(type_str, 0, sizeof(type_str));
                (void) ast_type_to_string(node->prototype.types[i], logger, type_str, sizeof(type_str));
                (void) LOGGER_log(logger, L_DEBUG, "%*c\b %zu: (%s) %s\n", offset, '-', i, type_str, node->prototype.args[i]);
            }
        }
        else
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b %s - %d args\n", offset, '-', node->prototype.name,
                node->prototype.arity);
            for (i = 0; i < node->prototype.arity; ++i)
            {
                (void) memset(type_str, 0, sizeof(type_str));
                (void) ast_type_to_string(node->prototype.types[i], logger, type_str, sizeof(type_str));
                (void) LOGGER_log(logger, L_DEBUG, "%*c\b %zu: (%s) %s\n", offset, '-', i,
                    type_str, node->prototype.args[i]);
            }
        }
        (void) memset(type_str, 0, sizeof(type_str));
        (void) ast_type_to_string(node->prototype.return_type, logger, type_str, sizeof(type_str));
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Return Type -> %s\n", offset, '-',
               type_str);
        break;
    }

    case AST_TYPE_FUNCTION:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Function definition\n", offset, ' ');
        if (NULL != node->function.prototype)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Prototype\n", offset + 2, ' ');
            (void) AST_print_ast(node->function.prototype, offset + 4, logger);
        }

        if (NULL != node->function.body)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Body\n", offset + 2, ' ');
            (void) ast_print_statements_block(node->function.body, offset + 4, logger);
        }
        break;
    }
    case AST_TYPE_RETURN_STMT:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Return statement\n", offset, ' ');
        if (NULL != node->return_stmt.expr)
            (void) AST_print_ast(node->return_stmt.expr, offset + 2, logger);
        break;
    }
    case AST_TYPE_IF_EXPR:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b If Expression\n", offset, ' ');
        if (NULL != node->if_expr.cond)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Condition\n", offset + 2, ' ');
            (void) AST_print_ast(node->if_expr.cond, offset + 4, logger);
        }
        if (NULL != node->if_expr.then_body)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Then Body\n", offset + 2, ' ');
            (void) ast_print_statements_block(node->if_expr.then_body, offset + 4, logger);
        }
        if (NULL != node->if_expr.else_body)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Else Body\n", offset + 2, ' ');
            (void) ast_print_statements_block(node->if_expr.else_body, offset + 4, logger);
        }
        break;
    }
    case AST_TYPE_WHILE_EXPR:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b While Expression\n", offset, ' ');
        if (NULL != node->while_expr.cond)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Condition\n", offset + 2, ' ');
            (void) AST_print_ast(node->while_expr.cond, offset + 4, logger);
        }
        if (NULL != node->while_expr.body)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Body\n", offset + 2, ' ');
            (void) ast_print_statements_block(node->while_expr.body, offset + 4, logger);
        }
        break;
    }
    case AST_TYPE_CAST_EXPR:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Cast Expression\n", offset, ' ');
        if (NULL != node->cast_expr.expr)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Expression\n", offset + 2);
            (void) AST_print_ast(node->cast_expr.expr, offset + 4, logger);
        }
        if (NULL != node->cast_expr.type)
        {
            (void) memset(type_str, 0, sizeof(type_str));
            ast_type_to_string(node->cast_expr.type, logger, type_str, sizeof(type_str));
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Type: %s\n", offset + 2, ' ', type_str);
        }
        break;
    }
    case AST_TYPE_VARIABLE:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Variable\n", offset, ' ');
        if (NULL != node->variable.name)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Name: %s\n", offset + 2, ' ', node->variable.name);
        }
        break;
    }
    case AST_TYPE_LET_STMT:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Let Statement\n", offset, ' ');
        if (NULL != node->let_stmt.var)
        {
            (void) AST_print_ast(node->let_stmt.var, offset + 2, logger);
        }

        if (NULL != node->let_stmt.expr)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Expression\n", offset + 2, ' ');
            (void) AST_print_ast(node->let_stmt.expr, offset + 4, logger);
        }
        break;
    }

    case AST_TYPE_ASSIGNMENT_EXPR:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Assignment Expression\n", offset, ' ');
        if (NULL != node->assignment_expr.lhs)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Left hand side\n", offset + 2, ' ');
            (void) AST_print_ast(node->assignment_expr.lhs, offset + 4, logger);
        }

        if (NULL != node->assignment_expr.rhs)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Right hand side\n", offset + 2, ' ');
            (void) AST_print_ast(node->assignment_expr.rhs, offset + 4, logger);
        }
        break;
    }

    case AST_TYPE_CALL_EXPR:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Call Expression\n", offset, ' ');
        if (NULL != node->call_expr.name)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Name - %s\n", offset + 2, ' ', node->call_expr.name);
        }

        if (NULL != node->call_expr.args)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Arguments\n", offset + 2, ' ');
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Count - %d\n", offset + 4, ' ', node->call_expr.args->size);
            VECTOR_FOR_EACH(node->call_expr.args, args)
            {
                arg = ITERATOR_GET_AS(t_ast_node_ptr, &args);
                (void) AST_print_ast(arg, offset + 4, logger);
            }
        }

        break;
    }

    case AST_TYPE_EXPRESSION_STMT:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Expression statement\n", offset, ' ');
        if (NULL != node->expression_stmt.expr)
            (void) AST_print_ast(node->expression_stmt.expr, offset + 2, logger);
        break;
    }
    case AST_TYPE_BREAK_STMT:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Break statement\n", offset, ' ');
        break;
    }
    case AST_TYPE_STRUCT_DEFINITION:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Struct Definition\n", offset, ' ');
        if (NULL != node->struct_definition.name)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Name - %s\n", offset + 2, ' ', node->struct_definition.name);
        }

        if (NULL != node->struct_definition.struct_fields)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Fields\n", offset + 2, ' ', node->struct_definition.name);
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Count - %d\n", offset + 4, ' ', node->struct_definition.struct_fields->size);

            VECTOR_FOR_EACH(node->struct_definition.struct_fields, struct_fields)
            {
                struct_field = ITERATOR_GET_AS(t_struct_field_ptr, &struct_fields);
                if (NULL != struct_field)
                {
                    (void) LOGGER_log(logger, L_DEBUG, "%*c\b Struct Field\n", offset + 4, ' ');

                    if (NULL != struct_field->name)
                    {
                        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Name - %s\n", offset + 6, ' ', struct_field->name);
                    }

                    if (NULL != struct_field->type)
                    {
                        (void) memset(type_str, 0, sizeof(type_str));
                        (void) ast_type_to_string(struct_field->type, logger, type_str, sizeof(type_str));
                        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Type - %s\n", offset + 6, ' ', type_str);
                    }
                }

            }
        }
        break;
    }
    case AST_TYPE_STRUCT_VALUE:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Struct Value\n", offset, ' ');
        if (NULL != node->struct_value.name)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Name - %s\n", offset + 2, ' ', node->struct_value.name);
        }

        if (NULL != node->struct_value.struct_values)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Fields\n", offset + 2, ' ', node->struct_value.name);
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Count - %d\n", offset + 4, ' ', node->struct_value.struct_values->size);

            VECTOR_FOR_EACH(node->struct_value.struct_values, value_fields)
            {
                value_field = ITERATOR_GET_AS(t_struct_value_field_ptr, &value_fields);
                if (NULL != value_field)
                {
                    (void) LOGGER_log(logger, L_DEBUG, "%*c\b Struct Field\n", offset + 4, ' ');

                    if (NULL != value_field->name)
                    {
                        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Name - %s\n", offset + 6, ' ', value_field->name);
                    }

                    if (NULL != value_field->expr)
                    {
                        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Expr\n", offset + 6, ' ');
                        (void) AST_print_ast(value_field->expr, offset + 8, logger);
                    }
                }

            }
        }
        break;
    }

    case AST_TYPE_ENUM_DEFINITION:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Enum Definition\n", offset, ' ');
        if (NULL != node->enum_definition.name)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Name - %s\n", offset + 2, ' ', node->enum_definition.name);
        }

        if (NULL != node->enum_definition.enum_fields)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Fields\n", offset + 2, ' ', node->enum_definition.name);
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Count - %d\n", offset + 4, ' ', node->enum_definition.enum_fields->size);

            VECTOR_FOR_EACH(node->enum_definition.enum_fields, enum_fields)
            {
                enum_field = ITERATOR_GET_AS(t_enum_field_ptr, &enum_fields);
                if (NULL != enum_field)
                {
                    (void) LOGGER_log(logger, L_DEBUG, "%*c\b Enum Field\n", offset + 4, ' ');

                    if (NULL != enum_field->name)
                    {
                        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Name - %s\n", offset + 6, ' ', enum_field->name);
                    }

                    if (NULL != enum_field->expr)
                    {
                        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Expr\n", offset + 6, ' ');
                        (void) AST_print_ast(enum_field->expr, offset + 8, logger);
                    }
                }

            }
        }
        break;
    }

    case AST_TYPE_GET_EXPR:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Get expr\n", offset, ' ');
        if (NULL != node->get_expr.variable)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b %s - %s\n", offset + 2, ' ', node->get_expr.is_enum ? "Enum" : "Variable", node->get_expr.variable);
        }

        if (NULL != node->get_expr.key)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Key - %s\n", offset + 2, ' ', node->get_expr.key);
        }
        break;
    }

    default:
    {
        (void) LOGGER_log(logger, L_DEBUG, "I don't know how to print type - %d\n", node->type);
        break;
    }
    }
}
