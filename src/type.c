/** @file type.c */
#include "type.h"
#include "defs.h"
#include "lib.h"
#include "logger.h"
#include "vector.h"

#include <stdlib.h>
#include <string.h>

bool type_can_cast(const t_type *type1, const t_type *type2);

bool TYPE_is_floating_point(const char *s)
{
    size_t i = 0, len = strlen(s);
    for (i = 0; i < len; ++i)
    {
        if ('.' == s[i])
        {
            return true;
        }
    }
    return false;
}

bool TYPE_is_floating_type(t_type *type)
{
    return (type->type == TYPE_F32) || (type->type == TYPE_F64);
}

t_type *TYPE_initialize_type(t_base_type type)
{
    t_type *ttype = calloc(1, sizeof(t_type));
    if (NULL == ttype)
    {
        (void) exit(LUKA_CANT_ALLOC_MEMORY);
    }

    ttype->type = type;
    ttype->inner_type = NULL;
    ttype->payload = NULL;
    ttype->mutable = false;
    return ttype;
}

bool type_is_payload_type(const t_type *type)
{
    switch (type->type)
    {
        case TYPE_STRUCT:
        case TYPE_ENUM:
        case TYPE_ALIAS:
            return true;
        default:
            return false;
    }
}

/** can cast type1 to type2 */
bool type_can_cast(const t_type *type1, const t_type *type2)
{
    bool result = false;

    switch (type1->type)
    {
        case TYPE_ANY:
            result = true;
            break;
        case TYPE_BOOL:
            switch (type2->type)
            {
                case TYPE_SINT8:
                case TYPE_SINT16:
                case TYPE_SINT32:
                case TYPE_SINT64:
                case TYPE_UINT8:
                case TYPE_UINT16:
                case TYPE_UINT32:
                case TYPE_UINT64:
                case TYPE_F32:
                case TYPE_F64:
                    result = true;
                    break;
                default:
                    break;
            }
            break;
        case TYPE_SINT8:
        case TYPE_SINT16:
        case TYPE_SINT32:
        case TYPE_SINT64:
            switch (type2->type)
            {
                case TYPE_BOOL:
                case TYPE_UINT8:
                case TYPE_UINT16:
                case TYPE_UINT32:
                case TYPE_UINT64:
                case TYPE_F32:
                case TYPE_F64:
                    result = true;
                    break;
                default:
                    break;
            }
            break;
        case TYPE_UINT8:
        case TYPE_UINT16:
        case TYPE_UINT32:
        case TYPE_UINT64:
            switch (type2->type)
            {
                case TYPE_BOOL:
                case TYPE_SINT8:
                case TYPE_SINT16:
                case TYPE_SINT32:
                case TYPE_SINT64:
                case TYPE_F32:
                case TYPE_F64:
                    result = true;
                    break;
                default:
                    break;
            }
            break;
        case TYPE_F32:
            switch (type2->type)
            {
                case TYPE_BOOL:
                case TYPE_F64:
                    result = true;
                    break;
                default:
                    break;
            }
            break;
        case TYPE_F64:
            result = TYPE_BOOL == type2->type;
            break;
        case TYPE_STRING:
            result = ((TYPE_PTR == type2->type)
                      && ((NULL != type2->inner_type)
                          && ((TYPE_UINT8 == type2->inner_type->type)
                              || (TYPE_SINT8 == type2->inner_type->type))));
            break;
        case TYPE_PTR:
        case TYPE_ARRAY:
            switch (type2->type)
            {
                case TYPE_ARRAY:
                case TYPE_PTR:
                    result = TYPE_equal(type1->inner_type, type2->inner_type);
                    result = result
                          && (type1->inner_type->mutable
                                  ? true
                                  : !type2->inner_type->mutable);
                    break;
                default:
                    break;
            }
            break;
        case TYPE_ALIAS:
            /* TODO: Check aliases match */
            result = true;
        default:
            break;
    }

    if (TYPE_ANY == type2->type)
    {
        result = true;
    }

    return result;
}

bool TYPE_equal(const t_type *type1, const t_type *type2)
{
    bool equal = false;

    if ((NULL == type1) && (NULL == type2))
    {
        return true;
    }
    else if (NULL == type1)
    {
        return false;
    }
    else if (NULL == type2)
    {
        return false;
    }

    equal = type1->type == type2->type;
    equal = equal && TYPE_equal(type1->inner_type, type2->inner_type);
    equal
        = equal && (type_is_payload_type(type1) == type_is_payload_type(type2));
    equal = type_is_payload_type(type1)
              ? equal
                    && (0
                        == strncmp(type1->payload, type2->payload,
                                   strlen(type1->payload)))
              : equal;
    equal = equal || type_can_cast(type1, type2);
    return equal;
}

t_type *TYPE_dup_type(t_type *type)
{
    t_type *res = NULL;

    if (NULL != type)
    {
        res = TYPE_initialize_type(type->type);
        res->inner_type = TYPE_dup_type(type->inner_type);
        res->payload = NULL;
        res->mutable = type->mutable;
        if (NULL != type->payload)
        {
            res->payload = (void *) strdup(type->payload);
        }
    }

    return res;
}

void TYPE_free_type(t_type *type)
{
    if (NULL != type)
    {
        if (NULL != type->payload)
        {
            (void) free(type->payload);
            type->payload = NULL;
        }

        if (NULL != type->inner_type)
        {
            (void) TYPE_free_type(type->inner_type);
            type->inner_type = NULL;
        }

        (void) free(type);
        type = NULL;
    }
}

size_t TYPE_sizeof(t_type *type)
{
    switch (type->type)
    {
        case TYPE_ANY:
        case TYPE_VOID:
        case TYPE_STRUCT:
            return 0;
        case TYPE_BOOL:
            return 1;
        case TYPE_SINT8:
        case TYPE_UINT8:
            return 8;
        case TYPE_SINT16:
        case TYPE_UINT16:
            return 16;
        case TYPE_ENUM:
        case TYPE_SINT32:
        case TYPE_UINT32:
        case TYPE_F32:
            return 32;
        case TYPE_SINT64:
        case TYPE_UINT64:
        case TYPE_F64:
            return 64;
        case TYPE_PTR:
        case TYPE_ARRAY:
            return sizeof(void *);
        case TYPE_STRING:
            return sizeof(char *);
        default:
            return 0;
    }
}

bool TYPE_is_signed(t_type *type)
{
    switch (type->type)
    {
        case TYPE_SINT8:
        case TYPE_SINT16:
        case TYPE_SINT32:
        case TYPE_SINT64:
            return true;
        default:
            return false;
    }
}

const char *TYPE_to_string(t_type *type, t_logger *logger, char *buffer,
                           size_t buffer_size)
{
    if (NULL == type)
    {
        (void) snprintf(buffer, buffer_size, "(unknown type - null)");
        return buffer;
    }

    if (type->mutable)
    {
        (void) snprintf(buffer, buffer_size, "mut ");
        buffer = buffer + strlen("mut ");
    }

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
            (void) snprintf(buffer, buffer_size, "f32");
            break;
        case TYPE_F64:
            (void) snprintf(buffer, buffer_size, "f64");
            break;
        case TYPE_STRING:
            (void) snprintf(buffer, buffer_size, "string");
            break;
        case TYPE_VOID:
            (void) snprintf(buffer, buffer_size, "void");
            break;
        case TYPE_PTR:
            (void) TYPE_to_string(type->inner_type, logger, buffer,
                                  buffer_size);
            (void) snprintf(buffer + strlen(buffer), buffer_size, "*");
            break;
        case TYPE_ARRAY:
            (void) TYPE_to_string(type->inner_type, logger, buffer,
                                  buffer_size);
            (void) snprintf(buffer + strlen(buffer), buffer_size, "[]");
            break;
        case TYPE_ENUM:
        case TYPE_STRUCT:
        case TYPE_ALIAS:
            (void) snprintf(buffer + strlen(buffer), buffer_size, "%s",
                            (char *) type->payload);
            break;
        default:
            (void) LOGGER_log(logger, L_ERROR,
                              "TYPE_to_string: I don't know how to "
                              "translate type %d to string.\n",
                              type);
            (void) snprintf(buffer, buffer_size, "s32");
            break;
    }

    if (type->mutable)
    {
        buffer = buffer - strlen("mut ");
    }

    return buffer;
}

t_type *type_last_stmt_type(t_vector *body, t_logger *logger,
                            const t_module *module)
{
    t_ast_node *last_stmt = NULL;

    last_stmt = *(t_ast_node **) vector_get(body, body->size - 1);
    return TYPE_get_type(last_stmt, logger, module);
}

t_type *TYPE_get_type(const t_ast_node *node, t_logger *logger,
                      const t_module *module)
{
    t_type *type = NULL, *inner = NULL;
    t_ast_node *func = NULL, *struct_definition = NULL;
    t_vector *struct_fields = NULL;
    t_struct_field *struct_field = NULL;

    switch (node->type)
    {
        case AST_TYPE_STRUCT_DEFINITION:
        case AST_TYPE_BREAK_STMT:
        case AST_TYPE_EXPRESSION_STMT:
        case AST_TYPE_LET_STMT:
            return TYPE_initialize_type(TYPE_VOID);
        case AST_TYPE_LITERAL:
            switch (node->literal.type)
            {
                case AST_LITERAL_FALSE:
                case AST_LITERAL_TRUE:
                    return TYPE_initialize_type(TYPE_BOOL);
                case AST_LITERAL_NULL:
                    type = TYPE_initialize_type(TYPE_PTR);
                    type->inner_type = TYPE_initialize_type(TYPE_ANY);
                    return type;
            }
            return TYPE_initialize_type(TYPE_ANY);
        case AST_TYPE_ASSIGNMENT_EXPR:
            return TYPE_get_type(node->assignment_expr.lhs, logger, module);
        case AST_TYPE_FUNCTION:
            if (node->function.prototype)
            {
                return TYPE_get_type(node->function.prototype, logger, module);
            }
            return TYPE_initialize_type(TYPE_ANY);
        case AST_TYPE_PROTOTYPE:
            return TYPE_dup_type(node->prototype.return_type);
        case AST_TYPE_IF_EXPR:
            if (NULL != node->if_expr.then_body)
            {
                return type_last_stmt_type(node->if_expr.then_body, logger,
                                           module);
            }
            else if (NULL != node->if_expr.else_body)
            {
                return type_last_stmt_type(node->if_expr.else_body, logger,
                                           module);
            }
            return TYPE_initialize_type(TYPE_VOID);
        case AST_TYPE_WHILE_EXPR:
            if (NULL != node->while_expr.body)
            {
                return type_last_stmt_type(node->while_expr.body, logger,
                                           module);
            }
            return TYPE_initialize_type(TYPE_VOID);
        case AST_TYPE_NUMBER:
            type = TYPE_dup_type(node->number.type);
            type->mutable = true;
            return type;
        case AST_TYPE_STRING:
            type = TYPE_initialize_type(TYPE_PTR);
            type->inner_type = TYPE_initialize_type(TYPE_UINT8);
            return type;
        case AST_TYPE_VARIABLE:
            return TYPE_dup_type(node->variable.type);
        case AST_TYPE_CAST_EXPR:
            return TYPE_dup_type(node->cast_expr.type);
        case AST_TYPE_RETURN_STMT:
            return TYPE_get_type(node->return_stmt.expr, logger, module);
        case AST_TYPE_ARRAY_DEREF:
            type = TYPE_get_type(node->array_deref.variable, logger, module);
            inner = TYPE_dup_type(type->inner_type);
            (void) TYPE_free_type(type);
            return inner;
        case AST_TYPE_GET_EXPR:
            if (node->get_expr.is_enum)
            {
                return TYPE_initialize_type(TYPE_SINT32);
            }

            type = TYPE_get_type(node->get_expr.variable, logger, module);
            if (NULL == type->payload)
            {
                LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                               "get expr variable type payload is NULL, "
                               "assuming return type is any\n",
                               NULL);
                return TYPE_initialize_type(TYPE_ANY);
            }
            VECTOR_FOR_EACH(module->structs, structs)
            {
                struct_definition = ITERATOR_GET_AS(t_ast_node_ptr, &structs);
                if (0
                    == strcmp(type->payload,
                              struct_definition->struct_definition.name))
                {
                    struct_fields
                        = struct_definition->struct_definition.struct_fields;
                    break;
                }
            }

            if (NULL == struct_fields)
            {
                LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                               "get expr variable type struct %s not found in "
                               "module, assuming return type is any\n",
                               type->payload);
                return TYPE_initialize_type(TYPE_ANY);
            }

            VECTOR_FOR_EACH(struct_fields, struct_fields_it)
            {
                struct_field
                    = *(t_struct_field **) iterator_get(&struct_fields_it);
                if (0 == strcmp(struct_field->name, node->get_expr.key))
                {
                    return TYPE_dup_type(struct_field->type);
                }
            }

            LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                           "get expr key not found in struct %s, assuming "
                           "return type is any\n",
                           type->payload);
            return TYPE_initialize_type(TYPE_ANY);
        case AST_TYPE_UNARY_EXPR:
            switch (node->unary_expr.operator)
            {
                case UNOP_MINUS:
                case UNOP_PLUS:
                    return TYPE_get_type(node->unary_expr.rhs, logger, module);
                case UNOP_NOT:
                    return TYPE_initialize_type(TYPE_BOOL);
                case UNOP_REF:
                    type = TYPE_initialize_type(TYPE_PTR);
                    type->inner_type
                        = TYPE_get_type(node->unary_expr.rhs, logger, module);
                    type->mutable = node->unary_expr.mutable;
                    return type;
                case UNOP_DEREF:
                    type = TYPE_get_type(node->unary_expr.rhs, logger, module);
                    inner = TYPE_dup_type(type->inner_type);
                    (void) TYPE_free_type(type);
                    return inner;
                default:
                    return TYPE_initialize_type(TYPE_ANY);
            }
        case AST_TYPE_BINARY_EXPR:
            return TYPE_get_type(node->binary_expr.rhs, logger, module);
        case AST_TYPE_CALL_EXPR:
            if (NULL == module)
            {
                LOGGER_LOG_LOC(
                    logger, L_ERROR, node->token,
                    "TYPE_get_type: module is NULL, cannot use it "
                    "to lookup function %s, assuming return type is any\n",
                    node->call_expr.name);
                return TYPE_initialize_type(TYPE_ANY);
            }
            func = LIB_resolve_func_name(module, node->call_expr.name, NULL);
            if (NULL == func)
            {
                LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                               "TYPE_get_type: Couldn't find function %s "
                               "inside module, assuming return type is any\n",
                               node->call_expr.name);
                return TYPE_initialize_type(TYPE_ANY);
            }
            if (NULL == func->function.prototype)
            {
                LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                               "TYPE_get_type: function %s prototype is NULL\n",
                               node->call_expr.name);
                return TYPE_initialize_type(TYPE_ANY);
            }
            return TYPE_dup_type(
                func->function.prototype->prototype.return_type);
        case AST_TYPE_STRUCT_VALUE:
            type = TYPE_initialize_type(TYPE_STRUCT);
            type->payload = strdup(node->struct_value.name);
            type->mutable = true;
            return type;
        case AST_TYPE_ENUM_DEFINITION:
            type = TYPE_initialize_type(TYPE_ENUM);
            type->payload = strdup(node->enum_definition.name);
            return type;
        case AST_TYPE_SIZEOF_EXPR:
            return TYPE_initialize_type(TYPE_UINT64);
        default:
            LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                           "TYPE_get_type: Unhandled node type %d\n",
                           node->type);
            return TYPE_initialize_type(TYPE_ANY);
    }
}
