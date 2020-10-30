#ifndef __TYPE_H_
#define __TYPE_H_

#include "defs.h"

bool TYPE_is_floating_point(const char *s);
bool TYPE_is_floating_type(t_type *type);
t_type *TYPE_initialize_type(t_base_type type);
void TYPE_free_type(t_type *type);

#endif // __TYPE_H_
