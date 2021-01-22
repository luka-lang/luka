/** @file type.c */
#include "type.h"
#include "defs.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>

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
                    && (strncmp(type1->payload, type2->payload,
                                strlen(type1->payload)))
              : equal;
    equal = equal || ((type1->type == TYPE_ANY) || (type2->type == TYPE_ANY));
    equal = equal && type1->mutable == type2->mutable;
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

t_type *TYPE_get_type(const t_ast_node *node, t_logger *logger)
{
    t_type *type = NULL;
    switch (node->type)
    {
        case AST_TYPE_NUMBER:
            return node->number.type;
        case AST_TYPE_STRING:
            type = TYPE_initialize_type(TYPE_PTR);
            type->inner_type = TYPE_initialize_type(TYPE_UINT8);
            return type;
        case AST_TYPE_VARIABLE:
            return node->variable.type;
        case AST_TYPE_CAST_EXPR:
            return node->cast_expr.type;
        case AST_TYPE_RETURN_STMT:
            return TYPE_get_type(node->return_stmt.expr, logger);
        default:
            LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                           "TYPE_get_type: Unhandled node type %d\n",
                           node->type);
            return TYPE_initialize_type(TYPE_ANY);
    }
}
