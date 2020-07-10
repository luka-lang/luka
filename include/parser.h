#ifndef __PARSER_H_
#define __PARSER_H_

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "tree.h"
#include "vector.h"

typedef struct ASTNode_s ASTNode;
union SyntaxNode {
  int64_t llVal;
  uint64_t ulVal;
  struct {
    ASTNode *left, *right;
  } BinaryExpr;
  struct {
    char *name;
    Vector *stmts;
  } Function;
};

enum SyntaxNodeType {
  AST_IntVal,
  AST_Add,
  AST_Sub,
  AST_Mul,
  AST_Div,
  AST_Mod,
  AST_ReturnStmt,
};

struct ASTNode_s {
  union SyntaxNode *Data;
  enum SyntaxNodeType Type;
};

typedef struct {
  Vector *tokens;
  size_t index; // tokens index
  const char *file_path;
  ASTNode *tree;
} parser_t;

void initialize_parser(parser_t *parser, Vector *tokens, const char *file_path);

void start_parsing(parser_t *parser);

void parse_function(parser_t *parser);
ASTnode *parse_binexpr(parser_t *parser, int ptp);

void print_parser_tokens(parser_t *parser);

#endif // __PARSER_H_
