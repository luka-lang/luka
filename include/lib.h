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
 * @brief Deallocates all memory allocated by the @p type_alises vector.
 *
 * @param[in,out] type_alises the type aliases vector to free.
 */
void LIB_free_type_aliases_vector(t_vector *type_alises);

t_return_code LIB_intialize_list(t_vector **items, size_t item_size,
                                 t_logger *logger);

/**
 * @brief Initializes a module.
 *
 * @param[in,out] module the module to free.
 * @param[in] logger a logger that can be used to log messages.
 */
t_return_code LIB_initialize_module(t_module **module, t_logger *logger);

/**
 * @brief Deallocates all memory allocated by the @p module.
 *
 * @param[in,out] module the module to free.
 * @param[in] logger a logger that can be used to log messages.
 */
void LIB_free_module(t_module *module, t_logger *logger);

/**
 * @brief Stringifying a string value.
 *
 * @param[in] source the string value.
 * @param[in] source_length the length of the source string.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return an escaped string.
 */
char *LIB_stringify(const char *source, size_t source_length, t_logger *logger);

/**
 * @brief Find a function inside a given @p module or any of its imported modules.
 *
 * @param[in] module the module to search in.
 * @param[in] name the name of the function to look for.
 *
 * @return NULL if not found or the t_ast_node of that function.
 */
t_ast_node *LIB_resolve_func_name(const t_module *module, const char *name);

#endif // __LIB_H__
