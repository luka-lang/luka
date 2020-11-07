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
