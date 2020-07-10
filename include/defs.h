#ifndef __DEFS_H_
#define __DEFS_H_

#define NUMBER_OF_KEYWORDS 2
extern const char *keywords[NUMBER_OF_KEYWORDS];

typedef enum {
  T_UNKNOWN = -1,
  T_FN = 0,
  T_RETURN,
  T_IDENTIFIER = NUMBER_OF_KEYWORDS,
  T_OPEN_PAREN,
  T_CLOSE_PAREN,
  T_OPEN_BRACKET,
  T_CLOSE_BRACKET,
  T_SEMI_COLON,
  T_NUMBER,
  T_STRING,
  T_PLUS,
  T_MINUS,
  T_STAR,
  T_SLASH,
  T_INTLIT,

  T_EOF,
} toktype_t;

typedef struct {
  long line;
  long offset;
  toktype_t type;
  char *content;
} token_t;

typedef token_t *token_ptr_t;
typedef token_t **vectok_t;

enum { A_ADD, A_SUBTRACT, A_MULTIPLY, A_DIVIDE, A_INT_LITERAL };

typedef struct ASTnode_s {
  int op;
  struct ASTnode_s *left;
  struct ASTnode_s *right;
  int intvalue;
} ASTnode;

#endif // __DEFS_H_
