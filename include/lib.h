/** @file lib.h */
#ifndef __LIB_H__
#define __LIB_H__

#include "defs.h"
#include "logger.h"

/**
 * @brief Deallocates all memory allocated by the @p tokens vector.
 *
 * @param[in,out] tokens the tokens vector to free.
 */
void LIB_free_tokens_vector(t_vector *tokens);

/**
 * @brief Deallocates all memory allocated by the @p functions vector.
 *
 * @param[in,out] functions the functions vector to free.
 * @param[in] logger a logger that can be used to log messages.
 */
void LIB_free_functions_vector(t_vector *functions, t_logger *logger);

#endif // __LIB_H__
