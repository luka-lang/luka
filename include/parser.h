#ifndef __PARSER_H_
#define __PARSER_H_

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "vector.h"

typedef struct {
  Vector *tokens;
  size_t index; // tokens index
  const char *file_path;
} parser_t;

void initialize_parser(parser_t *parser, Vector *tokens, const char *file_path);

void start_parsing(parser_t *parser);

void parse_function(parser_t *parser);
ASTnode *parse_binexpr(parser_t *parser, int ptp);

void print_parser_tokens(parser_t *parser);

#endif // __PARSER_H_
