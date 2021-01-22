/** @file type.h */
#ifndef __TYPE_H_
#define __TYPE_H_

#include "defs.h"
#include "logger.h"

/**
 * @brief Checks if the given string representation of a number will be stored
 * in a floating point type.
 *
 * @param[in] s the string representation of the number.
 *
 * @return whether the number is a floating point number.
 */
bool TYPE_is_floating_point(const char *s);

/**
 * @brief Checks if the given Luka type is a floating point type.
 *
 * @param[in] type the Luka type to check.
 *
 * @return whether the Luka type is a floating point type.
 */
bool TYPE_is_floating_type(t_type *type);

/**
 * @brief Initialize a Luka type with the base type @p type.
 *
 * @param[in] type the base type of the Luka type.
 *
 * @return a Luka type with the base type @p type.
 */
t_type *TYPE_initialize_type(t_base_type type);

/**
 * @brief Checks if two types are equal based on all of their properties.
 *
 * @param[in] type1 the first type.
 * @param[in] type2 the second type.
 *
 * @return a boolean that represents if the types are equal.
 */
bool TYPE_equal(const t_type *type1, const t_type *type2);

/**
 * @brief Duplicate a Luka type.
 *
 * @param[in] type the Luka type to duplicate.
 *
 * @return a Luka type that is identical to @p type but resides in new
 * memory.
 */
t_type *TYPE_dup_type(t_type *type);

/**
 * @brief Free a Luka type.
 *
 * @param[in] type the Luka type to free.
 */
void TYPE_free_type(t_type *type);

/**
 * @brief The size of a Luka type.
 *
 * @param[in] type the Luka type to get its size.
 */
size_t TYPE_sizeof(t_type *type);

/**
 * @brief Check if a Luka type is signed.
 *
 * @param[in] type the type to check.
 *
 * @return whether the type is signed.
 */
bool TYPE_is_signed(t_type *type);

/**
 * @brief Dumps a type into a string representation.
 *
 * @param[in] type the type to dump.
 * @param[in] logger a logger that can be used to log messages.
 * @param[out] buffer the buffer to which the string representation will be
 * written
 * @param[in] buffer_size the size of the buffer.
 *
 * @return the buffer with the written string representation of the type.
 */
const char *TYPE_to_string(t_type *type, t_logger *logger, char *buffer,
                           size_t buffer_size);

/**
 * @brief Get the type of @p node.
 *
 * @param[in] node the node of which the type should be extracted.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @returns TYPE_ANY type if the type is any or not uniquly handled or the type of the handled node.
 */
t_type *TYPE_get_type(const t_ast_node *node, t_logger *logger);

#endif // __TYPE_H_
