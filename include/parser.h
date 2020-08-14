#ifndef __PARSER_H__
#define __PARSER_H__

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "vector.h"

typedef struct
{
    t_vector *tokens;
    size_t index;
    const char *file_path;
} t_parser;

void initialize_parser(t_parser *parser, t_vector *tokens, const char *file_path);

t_vector *parse_top_level(t_parser *parser);

t_ast_node *parse_function(t_parser *parser);
t_ast_node *parse_binexpr(t_parser *parser, int ptp);

void print_parser_tokens(t_parser *parser);

#endif // __PARSER_H__
