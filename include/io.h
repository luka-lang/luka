/** @file io.h */
#ifndef __IO_H__
#define __IO_H__

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Get the contents of a file.
 *
 * @param[in] file_path the path to the file.
 *
 * @return the contents of the file.
 */
char *IO_get_file_contents(const char *file_path);

#endif // __IO_H__
