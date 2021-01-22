/** @file lexer.h */
#ifndef __LEXER_H__
#define __LEXER_H__

#include "defs.h"
#include "logger.h"
#include "vector.h"

/**
 * @brief tokenize Luka source code.
 *
 * @param[out] tokens the vector of tokens.
 * @param[in] source the source code.
 * @param[in] logger a logger that can be used to log messages.
 * @param[in] file_path the path of the file from which the @p source came.
 *
 * @return
 * * LUKA_SUCCESS on success,
 * * LUKA_CANT_ALLOC_MEMORY on memory allocation failure,
 * * LUKA_VECTOR_FAILURE when failed to add a token to the vector
 * * LUKA_LEXER_FAILED if failed to lex identifier or string.
 */
t_return_code LEXER_tokenize_source(t_vector *tokens, const char *source,
                                    t_logger *logger, const char *file_path);

#endif // __LEXER_H__
