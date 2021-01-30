/** @file type_checker.h */
#ifndef __TYPE_CHECKER_H__
#define __TYPE_CHECKER_H__

#include "defs.h"
#include "logger.h"
#include <stdbool.h>

/**
 * @brief Type check the given @p function.
 *
 * @param[in] module the currently checked module.
 * @param[in] function the function to check.
 * @param[in] logger the logger to use to log messages.
 *
 * @returns true if the function is valid or false otherwise.
 */
bool CHECK_function(const t_module *module, const t_ast_node *function, t_logger *logger);

#endif
