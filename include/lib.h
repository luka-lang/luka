#ifndef __LIB_H__
#define __LIB_H__

#include "defs.h"
#include "logger.h"

void LIB_free_tokens_vector(t_vector *tokens);

void LIB_free_functions_vector(t_vector *functions, t_logger *logger);

#endif // __LIB_H__
