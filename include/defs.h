#ifndef __DEFS_H_
#define __DEFS_H_

#include "vector.h"

#define NUMBER_OF_KEYWORDS 4
extern const char *keywords[NUMBER_OF_KEYWORDS];

typedef enum {
  T_UNKNOWN = -1,
  T_FN = 0,
  T_RETURN,
  T_IF,
  T_ELSE,
  T_IDENTIFIER = NUMBER_OF_KEYWORDS,
  T_OPEN_PAREN,
  T_CLOSE_PAREN,
  T_OPEN_BRACKET,
  T_CLOSE_BRACKET,
  T_SEMI_COLON,
  T_COMMA,
  T_NUMBER,
  T_STRING,
  T_PLUS,
  T_MINUS,
  T_STAR,
  T_SLASH,
  T_INTLIT,

  T_EQUALS,
  T_BANG,
  T_OPEN_ANG,
  T_CLOSE_ANG,
  T_EQEQ,
  T_NEQ,
  T_LEQ,
  T_GEQ,

  T_EOF,
} toktype_t;

typedef struct {
  long line;
  long offset;
  toktype_t type;
  char *content;
} token_t;

typedef token_t *token_ptr_t;

typedef enum {
  AST_TYPE_NUMBER,
  AST_TYPE_BINARY_EXPR,
  AST_TYPE_PROTOTYPE,
  AST_TYPE_FUNCTION,
  AST_TYPE_RETURN_STMT,
  AST_TYPE_IF_EXPR
} AST_node_type;

typedef enum {
  BINOP_ADD,
  BINOP_SUBTRACT,
  BINOP_MULTIPLY,
  BINOP_DIVIDE,
  BINOP_NOT,
  BINOP_LESSER,
  BINOP_GREATER,
  BINOP_EQUALS,
  BINOP_NEQ,
  BINOP_LEQ,
  BINOP_GEQ,
} AST_binop_type;

typedef struct ASTnode_s ASTnode;

typedef struct {
  int value;
} AST_number;

typedef struct {
  AST_binop_type operator;
  ASTnode *lhs;
  ASTnode *rhs;
} AST_binary_expr;

typedef struct {
  char *name;
  char **args;
  unsigned int arity;
} AST_prototype;

typedef struct {
  ASTnode *prototype;
  Vector *body;
} AST_function;

typedef struct {
  ASTnode *expr;
} AST_return_stmt;

typedef struct {
  ASTnode *cond;
  Vector *then_body;
  Vector *else_body;
} AST_if_expr;

typedef struct ASTnode_s {
  AST_node_type type;
  union {
    AST_number number;
    AST_binary_expr binary_expr;
    AST_prototype prototype;
    AST_function function;
    AST_return_stmt return_stmt;
    AST_if_expr if_expr;
  };
} ASTnode;

typedef ASTnode *ast_node_ptr_t;

#endif // __DEFS_H_
