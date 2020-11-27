#include "type.h"

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
            res->payload = (void *)strdup(type->payload);
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
