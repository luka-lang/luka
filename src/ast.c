#include "ast.h"

#include <stdio.h>
#include <string.h>

#include "type.h"

const char *ast_type_to_string(t_type type, t_logger *logger)
{
    switch (type)
    {
    case TYPE_ANY:
        return "any";
    case TYPE_BOOL:
        return "bool";
    case TYPE_SINT8:
        return "s8";
    case TYPE_SINT16:
        return "s16";
    case TYPE_SINT32:
        return "s32";
    case TYPE_SINT64:
        return "s64";
    case TYPE_UINT8:
        return "u8";
    case TYPE_UINT16:
        return "u16";
    case TYPE_UINT32:
        return "u32";
    case TYPE_UINT64:
        return "u64";
    case TYPE_F32:
        return "float";
    case TYPE_F64:
        return "double";
    case TYPE_STRING:
        return "str";
    case TYPE_VOID:
        return "void";
    default:
        (void) LOGGER_log(logger, L_ERROR, "I don't know how to translate type %d to LLVM types.\n",
                type);
        return "int32";
    }
}

t_ast_node *AST_new_number(t_type type, void *value)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_NUMBER;
    node->number.type = type;
    switch (type)
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
            (void) fprintf(stderr, "%d is not a number type.\n", type);
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

t_ast_node *AST_new_prototype(char *name, char **args, t_type *types, int arity,
                              t_type return_type, bool vararg)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_PROTOTYPE;
    node->prototype.name = strdup(name);
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

t_ast_node *AST_new_variable(char *name, t_type type, bool mutable)
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

t_ast_node *AST_new_assignment_stmt(char *var_name, t_ast_node *expr)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_ASSIGNMENT_STMT;
    node->assignment_stmt.var_name = var_name;
    node->assignment_stmt.expr = expr;
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

void AST_free_node(t_ast_node *node, t_logger *logger)
{
    if (NULL == node)
        return;

    switch (node->type)
    {
    case AST_TYPE_NUMBER:
    case AST_TYPE_STRING:
    case AST_TYPE_VARIABLE:
        break;
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
            for (int i = 0; i < node->prototype.arity; ++i)
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
            (void) free(node->prototype.types);
            node->prototype.types = NULL;
        }

        break;
    }

    case AST_TYPE_FUNCTION:
    {
        if (NULL != node->function.prototype)
            (void) AST_free_node(node->function.prototype, logger);

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
            (void) AST_free_node(node->if_expr.cond, logger);
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
    case AST_TYPE_ASSIGNMENT_STMT:
    {
        if (NULL != node->assignment_stmt.expr)
        {
            (void) AST_free_node(node->assignment_stmt.expr, logger);
        }
        break;
    }

    case AST_TYPE_CALL_EXPR:
    {
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

char *op_to_str(t_ast_binop_type op, t_logger *logger)
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
    case BINOP_NOT:
        return "!";
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
        (void) LOGGER_log(logger, L_ERROR, "Unknown operator: %d\n", op);
        (void) exit(1);
    }
}

void AST_print_ast(t_ast_node *node, int offset, t_logger *logger)
{
    t_ast_node *arg = NULL;
    size_t i = 0;
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
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b AST string \"%s\"\n", offset, ' ', node->string.value);
        break;
    case AST_TYPE_BINARY_EXPR:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Binary Expression\n", offset, ' ');
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Operator: %s\n", offset + 2, ' ',
                          op_to_str(node->binary_expr.operator, logger));
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
                (void) LOGGER_log(logger, L_DEBUG, "%*c\b %zu: (%s) %s\n", offset, '-', i,
                    ast_type_to_string(node->prototype.types[i], logger), node->prototype.args[i]);
            }
        }
        else
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b %s - %d args\n", offset, '-', node->prototype.name,
                node->prototype.arity);
            for (i = 0; i < node->prototype.arity; ++i)
            {
                (void) LOGGER_log(logger, L_DEBUG, "%*c\b %zu: (%s) %s\n", offset, '-', i,
                    ast_type_to_string(node->prototype.types[i], logger), node->prototype.args[i]);
            }
        }
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Return Type -> %s\n", offset, '-',
               ast_type_to_string(node->prototype.return_type, logger));
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

    case AST_TYPE_ASSIGNMENT_STMT:
    {
        (void) LOGGER_log(logger, L_DEBUG, "%*c\b Assignment Statement\n", offset, ' ');
        if (NULL != node->assignment_stmt.var_name)
        {
            (void) LOGGER_log(logger,
                              L_DEBUG,
                              "%*c\b Variable Name: %s\n",
                              offset,
                              ' ',
                              node->assignment_stmt.var_name);
        }

        if (NULL != node->assignment_stmt.expr)
        {
            (void) LOGGER_log(logger, L_DEBUG, "%*c\b Expression\n", offset + 2, ' ');
            (void) AST_print_ast(node->assignment_stmt.expr, offset + 4, logger);
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
    default:
    {
        (void) LOGGER_log(logger, L_DEBUG, "I don't know how to print type - %d\n", node->type);
        break;
    }
    }
}
