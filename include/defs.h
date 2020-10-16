#ifndef __DEFS_H__
#define __DEFS_H__

#include <stdbool.h>
#include <stdint.h>

#include "vector.h"

/* Taken from https://stackoverflow.com/questions/3599160/how-to-suppress-unused-parameter-warnings-in-c */
#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif

#ifdef __GNUC__
#  define UNUSED_FUNCTION(x) __attribute__((__unused__)) UNUSED_ ## x
#else
#  define UNUSED_FUNCTION(x) UNUSED_ ## x
#endif


typedef Vector t_vector;
typedef Iterator t_iterator;

typedef enum
{
    LUKA_UNINITIALIZED = -1,
    LUKA_SUCCESS = 0,
    LUKA_GENERAL_ERROR,
    LUKA_WRONG_PARAMETERS,
    LUKA_CANT_OPEN_FILE,
    LUKA_CANT_ALLOC_MEMORY,
    LUKA_LEXER_FAILED,
    LUKA_VECTOR_FAILURE,
    LUKA_CODEGEN_ERROR,
} t_return_code;

#define NUMBER_OF_KEYWORDS 27
extern const char *keywords[NUMBER_OF_KEYWORDS];

typedef enum
{
    T_UNKNOWN = -1,
    T_FN = 0,
    T_RETURN,
    T_IF,
    T_ELSE,
    T_LET,
    T_MUT,
    T_EXTERN,
    T_WHILE,
    T_BREAK,

    T_INT_TYPE,
    T_CHAR_TYPE,
    T_STR_TYPE,
    T_VOID_TYPE,
    T_FLOAT_TYPE,
    T_DOUBLE_TYPE,
    T_ANY_TYPE,
    T_BOOL_TYPE,
    T_U8_TYPE,
    T_U16_TYPE,
    T_U32_TYPE,
    T_U64_TYPE,
    T_S8_TYPE,
    T_S16_TYPE,
    T_S32_TYPE,
    T_S64_TYPE,
    T_F32_TYPE,
    T_F64_TYPE,

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
    T_PERCENT,
    T_INTLIT,

    T_EQUALS,
    T_BANG,
    T_OPEN_ANG,
    T_CLOSE_ANG,
    T_EQEQ,
    T_NEQ,
    T_LEQ,
    T_GEQ,

    T_AMPERCENT,

    T_COLON,
    T_DOT,
    T_THREE_DOTS,

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
    AST_TYPE_UNARY_EXPR,
    AST_TYPE_BINARY_EXPR,
    AST_TYPE_PROTOTYPE,
    AST_TYPE_FUNCTION,
    AST_TYPE_RETURN_STMT,
    AST_TYPE_IF_EXPR,
    AST_TYPE_WHILE_EXPR,
    AST_TYPE_VARIABLE,
    AST_TYPE_LET_STMT,
    AST_TYPE_ASSIGNMENT_EXPR,
    AST_TYPE_CALL_EXPR,
    AST_TYPE_EXPRESSION_STMT,
    AST_TYPE_BREAK_STMT
} t_ast_node_type;

typedef enum
{
    BINOP_ADD,
    BINOP_SUBTRACT,
    BINOP_MULTIPLY,
    BINOP_DIVIDE,
    BINOP_MODULOS,
    BINOP_LESSER,
    BINOP_GREATER,
    BINOP_EQUALS,
    BINOP_NEQ,
    BINOP_LEQ,
    BINOP_GEQ,
} t_ast_binop_type;

typedef enum
{
    UNOP_NOT,
    UNOP_MINUS,
    UNOP_PLUS,
    UNOP_DEREF,
    UNOP_REF,
} t_ast_unop_type;

typedef enum
{
    TYPE_ANY,
    TYPE_BOOL,
    TYPE_SINT8,
    TYPE_SINT16,
    TYPE_SINT32,
    TYPE_SINT64,
    TYPE_UINT8,
    TYPE_UINT16,
    TYPE_UINT32,
    TYPE_UINT64,
    TYPE_F32,
    TYPE_F64,
    TYPE_STRING,
    TYPE_VOID,
    TYPE_PTR
} t_base_type;

typedef struct s_type
{
    t_base_type type;
    struct s_type *inner_type;
} t_type;

typedef struct s_ast_node t_ast_node;

typedef struct
{
    t_type type;
    union {
        int8_t s8;
        uint8_t u8;
        int16_t s16;
        uint16_t u16;
        int32_t s32;
        uint32_t u32;
        int64_t s64;
        uint64_t u64;
        float f32;
        double f64;
    } value;
} t_ast_number;

typedef struct
{
    char *value;
    size_t length;
} t_ast_string;

typedef struct
{
    t_ast_unop_type operator;
    t_ast_node *rhs;
} t_ast_unary_expr;

typedef struct
{
    t_ast_binop_type operator;
    t_ast_node *lhs;
    t_ast_node *rhs;
} t_ast_binary_expr;

typedef struct
{
    t_ast_node *lhs;
    t_ast_node *rhs;
} t_ast_assignment_expr;

typedef struct
{
    char *name;
    char **args;
    t_type **types;
    t_type *return_type;
    unsigned int arity;
    bool vararg;
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
    t_ast_node *cond;
    t_vector *body;
} t_ast_while_expr;

typedef struct
{
    t_ast_node *var;
    t_ast_node *expr;
} t_ast_let_stmt;


typedef struct
{
    char *name;
    t_type *type;
    bool mutable;
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
        t_ast_unary_expr unary_expr;
        t_ast_binary_expr binary_expr;
        t_ast_prototype prototype;
        t_ast_function function;
        t_ast_return_stmt return_stmt;
        t_ast_if_expr if_expr;
        t_ast_while_expr while_expr;
        t_ast_assignment_expr assignment_expr;
        t_ast_variable variable;
        t_ast_let_stmt let_stmt;
        t_ast_call_expr call_expr;
        t_ast_expr_stmt expression_stmt;
    };
} t_ast_node;

typedef t_ast_node *t_ast_node_ptr;

#endif // __DEFS_H__
