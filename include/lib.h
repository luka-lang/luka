#ifndef __LIB_H__
#define __LIB_H__

#include "defs.h"

typedef enum
{
    LUKA_UNINITIALIZED = -1,
    LUKA_SUCCESS = 0,
    LUKA_WRONG_PARAMETERS,
    LUKA_CANT_OPEN_FILE,
    LUKA_CANT_ALLOC_MEMORY,
} t_return_code;

void LIB_free_tokens_vector(t_vector *tokens);

void LIB_free_functions_vector(t_vector *functions);

#endif // __LIB_H__