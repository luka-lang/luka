/** @file io.h */
#ifndef __IO_H__
#define __IO_H__

#include <stdio.h>
#include <stdlib.h>

#include "defs.h"

/**
 * @brief Get the contents of a file.
 *
 * @param[in] file_path the path to the file.
 *
 * @return the contents of the file.
 */
char *IO_get_file_contents(const char *file_path);

/**
 * @brief Print the source of @p file_path at @p line with caret on @p offset.
 *
 * @param[in] file_path the path of the file that should be read.
 * @param[in] line the line that should be printed.
 * @param[in] offset the offset of token that starts the error.
 */
void IO_print_error(const char *file_path, int line, int offset);

/**
 * @brief Copy the contents from the file at @p original_file_path to @p
 * new_file_path.
 *
 * @param[in] original_file_path the path of the file that should be read from.
 * @param[in] new_file_path the path of the file that should be written to.
 *
 * @returns LUKA_IO_ERROR if cannot open or close files, LUKA_SUCCESS on
 * success.
 */
t_return_code IO_copy(const char *original_file_path,
                      const char *new_file_path);

/**
 * @brief Get a path for @p requested_path from @p current_path.
 *
 * @param[in] requested_path the path of the requeste file to be imported.
 * @param[in] current_path the path of the current processed file.
 * @param[in] in_import this function was called when resolving an import,
 * relevant for knowing whether the dirname should be removed from the
 * current_path.
 *
 * @returns an absolute path that matches @p requested_path.
 */
char *IO_resolve_path(const char *requested_path, const char *current_path,
                      bool in_import);

#endif // __IO_H__
