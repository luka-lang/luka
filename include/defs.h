#ifndef __DEFS_H__
#define __DEFS_H__

#include "vector.h"

typedef Vector t_vector;

#define NUMBER_OF_KEYWORDS 11
extern const char *keywords[NUMBER_OF_KEYWORDS];

typedef enum
{
    T_UNKNOWN = -1,
    T_FN = 0,
    T_RETURN,
    T_IF,
    T_ELSE,
    T_LET,
    T_EXTERN,

    T_INT_TYPE,
    T_STR_TYPE,
    T_VOID_TYPE,
    T_FLOAT_TYPE,
    T_DOUBLE_TYPE,

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

    T_COLON,

    T_EOF,
} t_toktype;

typedef struct
{
    long line;
    long offset;
    t_toktype type;
    char *content;
} t_token;

typedef t_token *t_token_ptr;

typedef enum
{
    AST_TYPE_NUMBER,
    AST_TYPE_STRING,
    AST_TYPE_BINARY_EXPR,
    AST_TYPE_PROTOTYPE,
    AST_TYPE_FUNCTION,
    AST_TYPE_RETURN_STMT,
    AST_TYPE_IF_EXPR,
    AST_TYPE_VARIABLE,
    AST_TYPE_LET_STMT,
    AST_TYPE_CALL_EXPR,
    AST_TYPE_EXPRESSION_STMT
} t_ast_node_type;

typedef enum
{
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
} t_ast_binop_type;

typedef enum
{
    TYPE_INT1,
    TYPE_INT8,
    TYPE_INT16,
    TYPE_INT32,
    TYPE_INT64,
    TYPE_INT128,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_VOID
} t_type;

typedef struct s_ast_node t_ast_node;

typedef struct
{
    int value;
} t_ast_number;

typedef struct
{
    char *value;
    size_t length;
} t_ast_string;

typedef struct
{
    t_ast_binop_type operator;
    t_ast_node *lhs;
    t_ast_node *rhs;
} t_ast_binary_expr;

typedef struct
{
    char *name;
    char **args;
    t_type *types;
    t_type return_type;
    unsigned int arity;
} t_ast_prototype;

typedef struct
{
    t_ast_node *prototype;
    t_vector *body;
} t_ast_function;

typedef struct
{
    t_ast_node *expr;
} t_ast_return_stmt;

typedef struct
{
    t_ast_node *cond;
    t_vector *then_body;
    t_vector *else_body;
} t_ast_if_expr;

typedef struct
{
    t_ast_node *var;
    t_ast_node *expr;
} t_ast_let_stmt;

typedef struct
{
    char *name;
    t_type type;
} t_ast_variable;

typedef struct
{
    char *name;
    t_vector *args;
} t_ast_call_expr;

typedef struct
{
    t_ast_node *expr;
} t_ast_expr_stmt;

typedef struct s_ast_node
{
    t_ast_node_type type;
    union
    {
        t_ast_number number;
        t_ast_string string;
        t_ast_binary_expr binary_expr;
        t_ast_prototype prototype;
        t_ast_function function;
        t_ast_return_stmt return_stmt;
        t_ast_if_expr if_expr;
        t_ast_variable variable;
        t_ast_let_stmt let_stmt;
        t_ast_call_expr call_expr;
        t_ast_expr_stmt expression_stmt;
    };
} t_ast_node;

typedef t_ast_node *t_ast_node_ptr;

#endif // __DEFS_H__
