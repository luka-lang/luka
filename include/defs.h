/** @file defs.h */
#ifndef __DEFS_H__
#define __DEFS_H__

#include <stdbool.h>
#include <stdint.h>

#include "uthash.h"
#include "vector.h"

/* Taken from
 * https://stackoverflow.com/questions/3599160/how-to-suppress-unused-parameter-warnings-in-c
 */
#ifdef __GNUC__
#define UNUSED(x) UNUSED_##x __attribute__((__unused__))
#else
#define UNUSED(x) UNUSED_##x
#endif

#ifdef __GNUC__
#define UNUSED_FUNCTION(x) __attribute__((__unused__)) UNUSED_##x
#else
#define UNUSED_FUNCTION(x) UNUSED_##x
#endif

typedef Vector t_vector;     /**< Type alias to conform to luka's type naming */
typedef Iterator t_iterator; /**< Type alias to conform to luka's type naming */

typedef enum
{
    LUKA_UNINITIALIZED
        = -1, /**< A value for the return code when the function starts */
    LUKA_SUCCESS = 0,       /**< The function completed without errors */
    LUKA_GENERAL_ERROR,     /**< The function completed with an error */
    LUKA_WRONG_PARAMETERS,  /**< The executable received wrong paramters */
    LUKA_CANT_OPEN_FILE,    /**< Cannot open the file given to compile */
    LUKA_CANT_ALLOC_MEMORY, /**< Cannot allocate dynamic memory */
    LUKA_LEXER_FAILED,      /**< The lexer failed */
    LUKA_PARSER_FAILED,     /**< The parser failed */
    LUKA_CODEGEN_ERROR,     /**< There was a problem in the codegen phase */
    LUKA_TYPE_CHECK_ERROR,  /**< There was a problem in the type check phase */
    LUKA_VECTOR_FAILURE,    /**< There was a problem in the vector library */
    LUKA_IO_ERROR,   /**< There was a problem with input output operations */
    LUKA_LLVM_ERROR, /**< Signifies that a problem occured when calling an LLVM
                        function */
} t_return_code;     /**< An enum of possible luka return codes */

#define NUMBER_OF_KEYWORDS                                                     \
    35 /**< Number of keywords in the luka programming language */
extern const char *
    keywords[NUMBER_OF_KEYWORDS]; /**< string representations of the keywords */

typedef enum
{
    T_UNKNOWN = -1, /**< The token type is unknown */
    T_FN = 0,       /**< A "fn" token */
    T_RETURN,       /**< A "return" token */
    T_IF,           /**< A "if" token */
    T_ELSE,         /**< A "else" token */
    T_LET,          /**< A "let" token */
    T_MUT,          /**< A "mut" token */
    T_EXTERN,       /**< A "extern" token */
    T_WHILE,        /**< A "while" token */
    T_BREAK,        /**< A "break" token */
    T_AS,           /**< A "as" token */
    T_STRUCT,       /**< A "struct" token */
    T_ENUM,         /**< A "enum" token */
    T_IMPORT,       /**< A "import" token */
    T_TYPE,         /**< A "type" token */

    T_NULL,  /**< A "null" token */
    T_TRUE,  /**< A "true" token */
    T_FALSE, /**< A "false" token */

    T_INT_TYPE,    /**< A "int" token */
    T_CHAR_TYPE,   /**< A "char" token */
    T_STR_TYPE,    /**< A "string" token */
    T_VOID_TYPE,   /**< A "void" token */
    T_FLOAT_TYPE,  /**< A "float" token */
    T_DOUBLE_TYPE, /**< A "double" token */
    T_ANY_TYPE,    /**< A "any" token */
    T_BOOL_TYPE,   /**< A "bool" token */
    T_U8_TYPE,     /**< A "u8" token */
    T_U16_TYPE,    /**< A "u16" token */
    T_U32_TYPE,    /**< A "u32" token */
    T_U64_TYPE,    /**< A "u64" token */
    T_S8_TYPE,     /**< A "s8" token */
    T_S16_TYPE,    /**< A "s16" token */
    T_S32_TYPE,    /**< A "s32" token */
    T_S64_TYPE,    /**< A "s64" token */
    T_F32_TYPE,    /**< A "f32" token */
    T_F64_TYPE,    /**< A "f64" token */

    T_IDENTIFIER = NUMBER_OF_KEYWORDS, /**< An identifer token */
    T_OPEN_PAREN,                      /**< A "(" token */
    T_CLOSE_PAREN,                     /**< A ")" token */
    T_OPEN_BRACE,                      /**< A "{" token */
    T_CLOSE_BRACE,                     /**< A "}" token */
    T_OPEN_BRACKET,                    /**< A "[" token */
    T_CLOSE_BRACKET,                   /**< A "]" token */
    T_SEMI_COLON,                      /**< A ";" token */
    T_COMMA,                           /**< A "," token */
    T_NUMBER,                          /**< A number literal token */
    T_STRING,                          /**< A string literal token */
    T_PLUS,                            /**< A "+" token */
    T_MINUS,                           /**< A "-" token */
    T_STAR,                            /**< A "*" token */
    T_SLASH,                           /**< A "/" token */
    T_PERCENT,                         /**< A "%" token */
    T_INTLIT,                          /**< An int literal token */

    T_EQUALS,    /**< A "=" token */
    T_BANG,      /**< A "!" token */
    T_OPEN_ANG,  /**< A "<" token */
    T_CLOSE_ANG, /**< A ">" token */
    T_EQEQ,      /**< A "==" token */
    T_NEQ,       /**< A "!=" token */
    T_LEQ,       /**< A "<=" token */
    T_GEQ,       /**< A ">=" token */

    T_AMPERCENT, /**< A "&" token */

    T_COLON,        /**< A ":" token */
    T_DOUBLE_COLON, /**< A "::" token */
    T_DOT,          /**< A "." token */
    T_THREE_DOTS,   /**< A "..." token */

    T_EOF,   /**< A token for the end of the file */
} t_toktype; /**< An enum for the type of the token */

typedef struct
{
    long line;             /**< The line of the token*/
    long offset;           /**< The offset of the token inside the line */
    t_toktype type;        /**< The type of the token */
    char *content;         /**< The string representation of the token */
    const char *file_path; /**< The file path the token came from */
} t_token;                 /**< A struct that represents a token */

typedef t_token
    *t_token_ptr; /**< A type alias for getting this type from a vector */

typedef enum
{
    AST_TYPE_NUMBER,            /**< An AST node for number literals */
    AST_TYPE_STRING,            /**< An AST node for string literals */
    AST_TYPE_UNARY_EXPR,        /**< An AST node for unary expressions */
    AST_TYPE_BINARY_EXPR,       /**< An AST node for binary expressions */
    AST_TYPE_PROTOTYPE,         /**< An AST node for function prototypes */
    AST_TYPE_FUNCTION,          /**< An AST node for functions */
    AST_TYPE_RETURN_STMT,       /**< An AST node for return statements */
    AST_TYPE_IF_EXPR,           /**< An AST node for if expressions */
    AST_TYPE_WHILE_EXPR,        /**< An AST node for while expressions */
    AST_TYPE_CAST_EXPR,         /**< An AST node for cast expresions */
    AST_TYPE_VARIABLE,          /**< An AST node for variable references */
    AST_TYPE_LET_STMT,          /**< An AST node for let statements */
    AST_TYPE_ASSIGNMENT_EXPR,   /**< An AST node for assignment expressions */
    AST_TYPE_CALL_EXPR,         /**< An AST node for call expressions */
    AST_TYPE_EXPRESSION_STMT,   /**< An AST node for expresion statements */
    AST_TYPE_BREAK_STMT,        /**< An AST node for break statements */
    AST_TYPE_STRUCT_DEFINITION, /**< An AST node for struct defintions */
    AST_TYPE_STRUCT_VALUE,      /**< An AST node for struct values */
    AST_TYPE_ENUM_DEFINITION,   /**< An AST node for enum definitions */
    AST_TYPE_ENUM_VALUE,        /**< An AST node for enum values */
    AST_TYPE_GET_EXPR,          /**< An AST node for get expressions */
    AST_TYPE_ARRAY_DEREF,       /**< An AST node for array dereferences */
    AST_TYPE_LITERAL,           /**< An AST node for literals */
} t_ast_node_type; /**< An enum for different types of an AST node */

typedef enum
{
    BINOP_ADD,      /**< A binary operator for addition */
    BINOP_SUBTRACT, /**< A binary operator for subtraction */
    BINOP_MULTIPLY, /**< A binary operator for multiplication */
    BINOP_DIVIDE,   /**< A binary operator for division */
    BINOP_MODULOS,  /**< A binary operator for modulos */
    BINOP_LESSER,   /**< A binary operator for lesser comparison */
    BINOP_GREATER,  /**< A binary operator for greater comparison */
    BINOP_EQUALS,   /**< A binary operator for equals comparison */
    BINOP_NEQ,      /**< A binary operator for not equals comparison */
    BINOP_LEQ,      /**< A binary operator for lesser or equal comparison */
    BINOP_GEQ,      /**< A binary operator for greater or equal comparison */
} t_ast_binop_type; /**< An enum for differnt binary operators */

typedef enum
{
    UNOP_NOT,      /**< A unary operator for logical not */
    UNOP_MINUS,    /**< A unary operator for negation */
    UNOP_PLUS,     /**< A unary operator for plus */
    UNOP_DEREF,    /**< A unary operator for dereferencing */
    UNOP_REF,      /**< A unary operator for referencing */
} t_ast_unop_type; /**< An enum for different unary operators */

typedef enum
{
    TYPE_ANY,    /**< Any type */
    TYPE_BOOL,   /**< Boolean type */
    TYPE_SINT8,  /**< Signed integer 8 bit type */
    TYPE_SINT16, /**< Signed integer 16 bit type */
    TYPE_SINT32, /**< Signed integer 32 bit type */
    TYPE_SINT64, /**< Signed integer 64 bit type */
    TYPE_UINT8,  /**< Unsigned integer 8 bit type */
    TYPE_UINT16, /**< Unsigned integer 16 bit type */
    TYPE_UINT32, /**< Unsigned integer 32 bit type */
    TYPE_UINT64, /**< Unsigned integer 64 bit type */
    TYPE_F32,    /**< Floating point 32 bit type */
    TYPE_F64,    /**< Floating point 64 bit type */
    TYPE_STRING, /**< String type */
    TYPE_VOID,   /**< Void type */
    TYPE_PTR,    /**< Pointer type */
    TYPE_STRUCT, /**< Struct type */
    TYPE_ENUM,   /**< Enum type */
    TYPE_ARRAY,  /**< Array type */
    TYPE_ALIAS,  /**< Alias type */
} t_base_type;   /**< An enum for different value types */

typedef struct s_type
{
    t_base_type type;          /**< Base type */
    struct s_type *inner_type; /**< Inner type, used in pointers and arrays */
    void *payload;             /**< Used for the name of structs and enums */
    bool mutable;              /**< Whether the value is mutable or not */
} t_type;                      /**< Struct for describing a type */

typedef struct s_ast_node
    t_ast_node; /**< Forward declaration for the ast node struct */

typedef struct
{
    t_type *type; /**< The type of the number */
    union
    {
        int8_t s8;    /**< Signed integer 8 bit value */
        uint8_t u8;   /**< Signed integer 16 bit value */
        int16_t s16;  /**< Signed integer 32 bit value */
        uint16_t u16; /**< Signed integer 64 bit value */
        int32_t s32;  /**< Unsigned integer 8 bit value */
        uint32_t u32; /**< Unsigned integer 16 bit value */
        int64_t s64;  /**< Unsigned integer 32 bit value */
        uint64_t u64; /**< Unsigned integer 64 bit value */
        float f32;    /**< Floating point 32 bit value */
        double f64;   /**< Floating point 64 bit value */
    } value;    /**< A union for holding the value of the number literal */
} t_ast_number; /**< An AST node for number literals */

typedef struct
{
    char *value;   /**< The value of the string literal */
    size_t length; /**< The length of the string literal */
} t_ast_string;    /**< An AST node for string literals */

typedef struct
{
    t_ast_unop_type operator; /**< The operator of the unary expression */
    t_ast_node *rhs;          /**< The operand of the unary expression */
} t_ast_unary_expr;           /**< An AST node for unary expression */

typedef struct
{
    t_ast_binop_type operator; /**< The operator of the binary expression */
    t_ast_node *lhs; /**< The left hand side of the binary expression */
    t_ast_node *rhs; /**< The right hand side of the binary expression */
} t_ast_binary_expr; /**< An AST node for binary expressions */

typedef struct
{
    t_ast_node *lhs;     /**< The assignee */
    t_ast_node *rhs;     /**< The expression assigned to the assignee */
} t_ast_assignment_expr; /**< An AST node for assignment expressions */

typedef struct
{
    char *name;          /**< The name of the function */
    char **args;         /**< The names of the function arguments */
    t_type **types;      /**< The types of the function arguments */
    t_type *return_type; /**< The return type of the function */
    unsigned int
        arity;   /**< The arity of the function (how many arguments it takes) */
    bool vararg; /**< Whether the function is variadic */
} t_ast_prototype; /**< An AST node for function prototypes */

typedef struct
{
    t_ast_node *prototype; /**< The prototype of the function */
    t_vector *body;        /**< The body of the function */
} t_ast_function;          /**< An AST node for functions */

typedef struct
{
    t_ast_node *expr; /**< The expression returned in the return statement */
} t_ast_return_stmt;  /**< An AST node for return statements */

typedef struct
{
    t_ast_node *cond; /**< The condition of the if expression */
    t_vector
        *then_body; /**< The statements executed if the condition is true */
    t_vector
        *else_body; /**< The statements executed it the condition is false */
} t_ast_if_expr;    /**< An AST node for if expressions */

typedef struct
{
    t_ast_node *cond; /**< The condition of the while expression */
    t_vector
        *body; /**< The statements executed as long as the condition is true */
} t_ast_while_expr; /**< An AST node for while expressions */

typedef struct
{
    t_ast_node *expr; /**< The expression to cast */
    t_type *type;     /**< The type to cast to */
} t_ast_cast_expr;    /**< An AST node for cast expressions */

typedef struct
{
    t_ast_node *var;  /**< The declared variable */
    t_ast_node *expr; /**< The expression to bind to the variable */
    bool is_global;   /**< Whether the variable is a global variable or not */
} t_ast_let_stmt;     /**< An AST node for let statements */

typedef struct
{
    char *name;   /**< The name of the variable */
    t_type *type; /**< The type of the variable */
    bool mutable; /**< Whether the variable is mutable */
} t_ast_variable; /**< An AST node for variable references */

typedef struct
{
    char *name;     /**< The name of the called function */
    t_vector *args; /**< The arguments passed to the function */
} t_ast_call_expr;  /**< An AST node for call expressions */

typedef struct
{
    t_ast_node *expr; /**< The expression evaluated inside the statement */
} t_ast_expr_stmt;    /**< An AST node for expression statements */

typedef struct
{
    char *name;              /**< The name of the struct */
    t_vector *struct_fields; /**< The fields of the struct */
} t_ast_struct_definition;   /**< An AST node for struct definitions */

typedef struct
{
    char *name; /**< The name of the struct */
    t_vector
        *struct_values; /**< The values passed to the fields in the struct */
} t_ast_struct_value;   /**< An AST node for struct values */

typedef struct
{
    char *name;            /**< The name of the enum */
    t_vector *enum_fields; /**< The fields of the enum */
} t_ast_enum_definition;   /**< An AST node for enum definitions */

typedef struct
{
    char *variable; /**< The name of the variable */
    char *key;      /**< The requested field/enumerated value */
    bool is_enum;   /**< Whether the get is an enum or struct get */
} t_ast_get_expr;   /**< An AST node for get expressions */

typedef struct
{
    char *variable;    /**< The name of the variable */
    t_ast_node *index; /**< The index to dereference at */
} t_ast_array_deref;   /**< An AST node for array dereferences */

typedef enum
{
    AST_LITERAL_NULL,  /**< Literal "null" */
    AST_LITERAL_TRUE,  /**< Literal "true" */
    AST_LITERAL_FALSE, /**< Literal "false" */
} t_ast_literal_type;

typedef struct
{
    t_ast_literal_type type; /**< The type of the literal */
} t_ast_literal;             /**< An AST node for literals */

typedef struct s_ast_node
{
    t_ast_node_type type; /**< The type of the AST node */
    union
    {
        t_ast_number number;           /**< Number literal AST node value */
        t_ast_string string;           /**< String literal AST node value */
        t_ast_unary_expr unary_expr;   /**< Unary expression AST node value */
        t_ast_binary_expr binary_expr; /**< Binary expression AST node value */
        t_ast_prototype prototype;     /**< Function prototype AST node value */
        t_ast_function function;       /**< Function AST node value */
        t_ast_return_stmt return_stmt; /**< Return statement AST node value */
        t_ast_if_expr if_expr;         /**< If expression AST node value */
        t_ast_while_expr while_expr;   /**< While expression AST node value */
        t_ast_cast_expr cast_expr;     /**< Cast expression AST node value */
        t_ast_assignment_expr
            assignment_expr;       /**< Assignement expression AST node value */
        t_ast_variable variable;   /**< Variable reference AST node value */
        t_ast_let_stmt let_stmt;   /**< Let statement AST node value */
        t_ast_call_expr call_expr; /**< Call expression AST node value */
        t_ast_expr_stmt
            expression_stmt; /**< Expression statement AST node value */
        t_ast_struct_definition
            struct_definition; /**< Struct definition AST node value */
        t_ast_struct_value struct_value; /**< Struct value AST node value */
        t_ast_enum_definition
            enum_definition;           /**< Enum definition AST node value */
        t_ast_get_expr get_expr;       /**< Get expression AST node value */
        t_ast_array_deref array_deref; /**< Array dereference AST node value */
        t_ast_literal literal;         /**< Literal AST node value */
    };                                 /**< All possible AST node values */
    t_token *token;                    /**< The origin token of the node */
} t_ast_node;                          /**< A struct for AST nodes */

typedef t_ast_node
    *t_ast_node_ptr; /**< A type alias for getting this type from a vector */

typedef struct
{
    char *name;        /**< The name of the struct field */
    t_type *type;      /**< The type of the struct field */
    UT_hash_handle hh; /**< A handle for uthash */
} t_struct_field;      /**< A struct for fields of a struct */

typedef struct
{
    char *name;         /**< The name of the struct value field */
    t_ast_node *expr;   /**< The value of the struct value field */
    UT_hash_handle hh;  /**< A handle for uthash */
} t_struct_value_field; /**< A struct for field values in struct values */

typedef t_struct_field *
    t_struct_field_ptr; /**< A type alias for getting this type from a vector */
typedef t_struct_value_field
    *t_struct_value_field_ptr; /**< A type alias for getting this type from a
                                  vector */

typedef struct
{
    char *name;        /**< The name of the enum field */
    t_ast_node *expr;  /**< The value of the enum field */
    UT_hash_handle hh; /**< A handle for uthash */
} t_enum_field;

typedef t_enum_field
    *t_enum_field_ptr; /**< A type alias for getting this type from a vector */

#define RAISE_STATUS_ON_ERROR_INTERNAL(expr, status_variable, status_type,     \
                                       success_status, label)                  \
    do                                                                         \
    {                                                                          \
        status_type _internal_status_ = expr;                                  \
        if (_internal_status_ != success_status)                               \
        {                                                                      \
            status_variable = _internal_status_;                               \
            goto label;                                                        \
        }                                                                      \
    } while (0)

#define RAISE_STATUS_ON_ERROR(expr, status_variable, label)                    \
    RAISE_STATUS_ON_ERROR_INTERNAL(expr, status_variable, int, 0, label)
#define RAISE_LUKA_STATUS_ON_ERROR(expr, status_variable, label)               \
    RAISE_STATUS_ON_ERROR_INTERNAL(expr, status_variable, t_return_code,       \
                                   LUKA_SUCCESS, label)
#define ON_ERROR(expr) if (0 != (expr))

typedef struct
{
    t_vector *enums;
    t_vector *functions;
    t_vector *import_paths;
    t_vector *imports;
    t_vector *structs;
    t_vector *variables;
} t_module;

typedef struct
{
    char *name;
    t_type *type;
} t_type_alias;

typedef char
    *t_char_ptr; /**< A type alias for getting this type from a vector */

#endif // __DEFS_H__
