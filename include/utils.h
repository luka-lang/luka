/** @file utils.h */
#ifndef __UTILS_H__
#define __UTILS_H__

#include "defs.h"
#include "logger.h"
#include <stdbool.h>
#include <stddef.h>

char *UTILS_fill_function_name(char *function_name_buffer, size_t buffer_length,
                               t_ast_node *node, bool *pushed_first_arg,
                               bool *builtin, t_logger *logger);

void UTILS_pop_first_arg(t_ast_node *node, t_logger *logger);

#endif /* __UTILS_H__ */
