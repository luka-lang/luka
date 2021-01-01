/** @file parser.h */
#ifndef __PARSER_H__
#define __PARSER_H__

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "logger.h"
#include "vector.h"

typedef struct
{
    t_vector
        *tokens;  /**< The vector of tokens that the parser will operate on */
    size_t index; /**< The current index in the tokens vector */
    t_vector *struct_names; /**< A vector of currently defined struct names */
    t_vector *enum_names;   /**< A vector of currently defined enum names*/
    const char *file_path;  /**< The path of parsed file */
    t_logger *logger;       /**< A logger the parser will log messages to */
} t_parser;                 /**< A struct for a parser */

/**
 * @brief Initializes a new parser based on the given parameters.
 *
 * @note @p parser should already be allocated.
 *
 * @param[in,out] parser the parser to initialize.
 * @param[in] tokens the tokens vector that the parser should operate on.
 * @param[in] file_path the path of the file the tokens came from.
 * @param[in] logger the logger that the parser should log messages to.
 */
void PARSER_initialize(t_parser *parser, t_vector *tokens,
                       const char *file_path, t_logger *logger);

/**
 * @brief Parse top level declarations and definitions.
 *
 * @param[in,out] parser the parser to parse with.
 *
 * @return a luka module built from the parsed tokens.
 */
t_module *PARSER_parse_file(t_parser *parser);

/**
 * @brief Print all tokens of a parser.
 *
 * @param[in] parser the parser to print its tokens.
 */
void PARSER_print_parser_tokens(t_parser *parser);

/**
 * @brief Deallocate all memory allocated by @p parser.
 *
 * @param[in,out] parser the parser to free.
 */
void PARSER_free(t_parser *parser);

#endif // __PARSER_H__
