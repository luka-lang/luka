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
    t_vector *tokens;
    size_t index;
    const char *file_path;
    t_logger *logger;
} t_parser;

void PARSER_initialize(t_parser *parser, t_vector *tokens, const char *file_path, t_logger *logger);

t_vector *PARSER_parse_top_level(t_parser *parser);

t_ast_node *PARSER_parse_function(t_parser *parser);

void PARSER_print_parser_tokens(t_parser *parser);

#endif // __PARSER_H__
