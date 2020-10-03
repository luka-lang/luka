#include "type.h"

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

bool TYPE_is_floating_type(t_type type)
{
    return (type == TYPE_F32) || (type == TYPE_F64);
}
