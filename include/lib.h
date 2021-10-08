/** @file lib.h */
#ifndef LUKA_LIB_H
#define LUKA_LIB_H

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
 * @brief Check if a module was already codegened.
 *
 * @param[in] codegen_modules a vector of already codegened modules.
 * @param[in] module the module to check.
 *
 * @return boolean that represents if the module appears in the vector.
 */
bool LIB_module_in_list(t_vector *codegen_modules, const t_module *module);

/**
 * @brief Check if @p name is a name of a struct type in @p module or any of the
 * imports.
 *
 * @param[in] module the module to search in.
 * @param[in] name the name of the struct type.
 * @param[in] original_module used for solving pointless recursion, use NULL
 * when calling this function.
 *
 * @return boolean that represents if @p name is the name of a struct type in @p
 * module.
 */
bool LIB_is_struct_name(const t_module *module, const char *name,
                        const t_module *original_module);

/**
 * @brief Check if @p name is a name of a enum type in @p module or any of the
 * imports.
 *
 * @param[in] module the module to search in.
 * @param[in] name the name of the enum type.
 * @param[in] original_module used for solving pointless recursion, use NULL
 * when calling this function.
 *
 * @return boolean that represents if @p name is the name of a enum type in @p
 * module.
 */
bool LIB_is_enum_name(const t_module *module, const char *name,
                      const t_module *original_module);

/**
 * @brief Find a function inside a given @p module or any of its imported
 * modules.
 *
 * @param[in] module the module to search in.
 * @param[in] name the name of the function to look for.
 * @param[in] original_module the module that requested to resolve, used for
 * solving circular recursion, use NULL when calling this function by
 * yourself.
 *
 * @return NULL if not found or the t_ast_node of that function.
 */
t_ast_node *LIB_resolve_func_name(const t_module *module, const char *name,
                                  const t_module *original_module);

#endif // LUKA_LIB_H
