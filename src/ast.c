#include "ast.h"

#include <stdio.h>
#include <string.h>

const char *ast_type_to_string(t_type type)
{
    switch (type)
    {
    case TYPE_INT32:
        return "int32";
    case TYPE_STRING:
        return "str";
    case TYPE_VOID:
        return "void";
    case TYPE_DOUBLE:
        return "double";
    case TYPE_FLOAT:
        return "float";
    case TYPE_INT1:
        return "int1";
    case TYPE_INT8:
        return "int8";
    case TYPE_INT16:
        return "int16";
    case TYPE_INT64:
        return "int64";
    case TYPE_INT128:
        return "int128";
    default:
        fprintf(stderr, "I don't know how to translate type %d to LLVM types.\n",
                type);
        return "int32";
    }
}

t_ast_node *AST_new_number(int value)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_NUMBER;
    node->number.value = value;
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
                              t_type return_type)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_PROTOTYPE;
    node->prototype.name = strdup(name);
    node->prototype.args = calloc(arity, sizeof(char *));
    node->prototype.types = types;
    node->prototype.return_type = return_type;
    for (int i = 0; i < arity; ++i)
    {
        node->prototype.args[i] = strdup(args[i]);
    }
    node->prototype.arity = arity;
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

t_ast_node *AST_new_variable(char *name, t_type type)
{
    t_ast_node *node = calloc(1, sizeof(t_ast_node));
    node->type = AST_TYPE_VARIABLE;
    node->variable.name = name;
    node->variable.type = type;
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

void AST_free_node(t_ast_node *node)
{
    if (!node)
        return;

    switch (node->type)
    {
    case AST_TYPE_NUMBER:
        break;
    case AST_TYPE_STRING:
    {
        if (node->string.value)
        {
            free(node->string.value);
        }
        break;
    }
    case AST_TYPE_BINARY_EXPR:
    {
        if (node->binary_expr.lhs)
            AST_free_node(node->binary_expr.lhs);
        if (node->binary_expr.rhs)
            AST_free_node(node->binary_expr.rhs);
        break;
    }
    case AST_TYPE_PROTOTYPE:
    {
        if (node->prototype.name)
            free(node->prototype.name);
        for (int i = 0; i < node->prototype.arity; ++i)
        {
            free(node->prototype.args[i]);
        }
        free(node->prototype.args);
        break;
    }

    case AST_TYPE_FUNCTION:
    {
        if (node->function.prototype)
            AST_free_node(node->function.prototype);
        if (node->function.body)
        {
            t_ast_node *stmt = NULL;
            VECTOR_FOR_EACH(node->function.body, stmts)
            {
                stmt = ITERATOR_GET_AS(t_ast_node_ptr, &stmts);
                AST_free_node(stmt);
            }

            vector_clear(node->function.body);
            vector_destroy(node->function.body);
            free(node->function.body);
        }
        break;
    }
    case AST_TYPE_RETURN_STMT:
    {
        if (node->return_stmt.expr)
            AST_free_node(node->return_stmt.expr);
        break;
    }
    case AST_TYPE_IF_EXPR:
    {
        if (node->if_expr.cond)
            AST_free_node(node->if_expr.cond);
        if (node->if_expr.then_body)
        {
            t_ast_node *stmt = NULL;
            VECTOR_FOR_EACH(node->if_expr.then_body, stmts)
            {
                stmt = ITERATOR_GET_AS(t_ast_node_ptr, &stmts);
                AST_free_node(stmt);
            }

            vector_clear(node->if_expr.then_body);
            vector_destroy(node->if_expr.then_body);
            free(node->if_expr.then_body);
        }
        if (node->if_expr.else_body)
        {
            t_ast_node *stmt = NULL;
            VECTOR_FOR_EACH(node->if_expr.else_body, stmts)
            {
                stmt = ITERATOR_GET_AS(t_ast_node_ptr, &stmts);
                AST_free_node(stmt);
            }

            vector_clear(node->if_expr.else_body);
            vector_destroy(node->if_expr.else_body);
            free(node->if_expr.else_body);
        }
        break;
    }

    case AST_TYPE_VARIABLE:
    {
        if (node->variable.name)
        {
            free(node->variable.name);
        }
        break;
    }

    case AST_TYPE_LET_STMT:
    {
        if (node->let_stmt.var)
        {
            AST_free_node(node->let_stmt.var);
        }

        if (node->let_stmt.expr)
        {
            AST_free_node(node->let_stmt.expr);
        }
        break;
    }

    case AST_TYPE_CALL_EXPR:
    {
        if (node->call_expr.args)
        {
            t_ast_node *arg = NULL;
            VECTOR_FOR_EACH(node->call_expr.args, args)
            {
                arg = ITERATOR_GET_AS(t_ast_node_ptr, &args);
                AST_free_node(arg);
            }

            vector_clear(node->call_expr.args);
            vector_destroy(node->call_expr.args);
            free(node->call_expr.args);
        }
        break;
    }

    case AST_TYPE_EXPRESSION_STMT:
    {
        if (node->expression_stmt.expr)
            AST_free_node(node->expression_stmt.expr);
        break;
    }

    default:
    {
        printf("I don't now how to free type - %d\n", node->type);
        break;
    }
    }

    free(node);
}

void ast_print_statements_block(t_vector *statements, int offset)
{
    t_ast_node *stmt = NULL;

    printf("%*c\b Statements block\n", offset, ' ');
    VECTOR_FOR_EACH(statements, stmts)
    {
        stmt = ITERATOR_GET_AS(t_ast_node_ptr, &stmts);
        AST_print_ast(stmt, offset + 2);
    }
}

void AST_print_functions(t_vector *functions, int offset)
{
    t_ast_node *func = NULL;
    VECTOR_FOR_EACH(functions, funcs)
    {
        func = ITERATOR_GET_AS(t_ast_node_ptr, &funcs);
        AST_print_ast(func, offset);
    }
}

char *op_to_str(t_ast_binop_type op)
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
        fprintf(stderr, "Unknown operator: %d\n", op);
        exit(1);
    }
}

void AST_print_ast(t_ast_node *node, int offset)
{
    t_ast_node *arg = NULL;
    size_t i = 0;
    if (!node)
        return;

    if (0 == offset)
    {
        printf("Printing AST:\n");
    }

    switch (node->type)
    {
    case AST_TYPE_NUMBER:
        printf("%*c\b AST number %d\n", offset, ' ', node->number.value);
        break;
    case AST_TYPE_STRING:
        printf("%*c\b AST string \"%s\"\n", offset, ' ', node->string.value);
        break;
    case AST_TYPE_BINARY_EXPR:
    {
        printf("%*c\b Binary Expression\n", offset, ' ');
        printf("%*c\b Operator: %s\n", offset + 2, ' ',
               op_to_str(node->binary_expr.operator));
        if (node->binary_expr.lhs)
            AST_print_ast(node->binary_expr.lhs, offset + 4);
        if (node->binary_expr.rhs)
            AST_print_ast(node->binary_expr.rhs, offset + 4);
        break;
    }
    case AST_TYPE_PROTOTYPE:
    {
        printf("%*c\b %s - %d args\n", offset, '-', node->prototype.name,
               node->prototype.arity);
        for (i = 0; i < node->prototype.arity; ++i)
        {
            printf("%*c\b %zu: (%s) %s\n", offset, '-', i,
                   ast_type_to_string(node->prototype.types[i]), node->prototype.args[i]);
        }
        printf("%*c\b Return Type -> %s\n", offset, '-',
               ast_type_to_string(node->prototype.return_type));
        break;
    }

    case AST_TYPE_FUNCTION:
    {
        printf("%*c\b Function definition\n", offset, ' ');
        if (node->function.prototype)
        {
            printf("%*c\b Prototype\n", offset + 2, ' ');
            AST_print_ast(node->function.prototype, offset + 4);
        }

        if (node->function.body)
        {
            printf("%*c\b Body\n", offset + 2, ' ');
            ast_print_statements_block(node->function.body, offset + 4);
        }
        break;
    }
    case AST_TYPE_RETURN_STMT:
    {
        printf("%*c\b Return statement\n", offset, ' ');
        if (node->return_stmt.expr)
            AST_print_ast(node->return_stmt.expr, offset + 2);
        break;
    }
    case AST_TYPE_IF_EXPR:
    {
        printf("%*c\b If Expression\n", offset, ' ');
        if (node->if_expr.cond)
        {
            printf("%*c\b Condition\n", offset + 2, ' ');

            AST_print_ast(node->if_expr.cond, offset + 4);
        }
        if (node->if_expr.then_body)
        {
            printf("%*c\b Then Body\n", offset + 2, ' ');
            ast_print_statements_block(node->if_expr.then_body, offset + 4);
        }
        if (node->if_expr.else_body)
        {
            printf("%*c\b Else Body\n", offset + 2, ' ');
            ast_print_statements_block(node->if_expr.else_body, offset + 4);
        }
        break;
    }
    case AST_TYPE_VARIABLE:
    {
        printf("%*c\b Variable\n", offset, ' ');
        if (node->variable.name)
        {
            printf("%*c\b Name: %s\n", offset + 2, ' ', node->variable.name);
        }
        break;
    }
    case AST_TYPE_LET_STMT:
    {
        printf("%*c\b Let Statement\n", offset, ' ');
        if (node->let_stmt.var)
        {
            AST_print_ast(node->let_stmt.var, offset + 2);
        }

        if (node->let_stmt.expr)
        {
            printf("%*c\b Expression\n", offset + 2, ' ');
            AST_print_ast(node->let_stmt.expr, offset + 4);
        }
        break;
    }

    case AST_TYPE_CALL_EXPR:
    {
        printf("%*c\b Call Expression\n", offset, ' ');
        if (node->call_expr.name)
        {
            printf("%*c\b Name - %s\n", offset + 2, ' ', node->call_expr.name);
        }

        if (node->call_expr.args)
        {
            printf("%*c\b Arguments\n", offset + 2, ' ');
            printf("%*c\b Count - %d\n", offset + 4, ' ', node->call_expr.args->size);
            VECTOR_FOR_EACH(node->call_expr.args, args)
            {
                arg = ITERATOR_GET_AS(t_ast_node_ptr, &args);
                AST_print_ast(arg, offset + 4);
            }
        }

        break;
    }

    case AST_TYPE_EXPRESSION_STMT:
    {
        printf("%*c\b Expression statement\n", offset, ' ');
        if (node->expression_stmt.expr)
            AST_print_ast(node->expression_stmt.expr, offset + 2);
        break;
    }
    default:
    {
        printf("I don't know how to print type - %d\n", node->type);
        break;
    }
    }
}
