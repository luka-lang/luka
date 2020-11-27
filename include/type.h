/** @file type.h */
#ifndef __TYPE_H_
#define __TYPE_H_

#include "defs.h"

/**
 * @brief Checks if the given string representation of a number will be stored in a floating point type.
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
 * @brief Duplicate a Luka type.
 *
 * @param[in] type the Luka type to duplicate.
 *
 * @return a Luka type that is identical to @p type but resides in new memory.
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

#endif // __TYPE_H_
