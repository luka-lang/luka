/** @file gen.h */
#ifndef __GEN_H__
#define __GEN_H__

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>

#include "defs.h"
#include "logger.h"
#include "uthash.h"

typedef struct
{
    const char *name;         /**< The name of the named value */
    LLVMValueRef alloca_inst; /**< The alloca instruction of the named value */
    LLVMTypeRef type;         /**< The LLVM type of the named value */
    t_type *ttype;            /**< The luka type of the named value */
    bool mutable;             /**< Whether the named value is mutable */
    UT_hash_handle hh;        /**< A handle for uthash */
} t_named_value;              /**< A struct for named values */

typedef struct
{
    LLVMTypeRef struct_type; /**< The LLVM type of the struct */
    char *struct_name;       /**< The name of the struct */
    char **struct_fields;    /**< The name of the struct fields */
    size_t number_of_fields; /**< The number of fields in the struct */
    UT_hash_handle hh;       /**< A handle for uthash */
} t_struct_info;             /**< A struct for keeping info about structs */

typedef struct
{
    char *enum_name;         /**< The name of the enum */
    char **enum_field_names; /**< The fields of the enum */
    int *enum_field_values;  /**< The values of the enum fields */
    size_t number_of_fields; /**< The number of enum fields */
    UT_hash_handle hh;       /**< A handle for uthash */
} t_enum_info;               /**< A struct for keeping info about enums */

/**
 * @brief Generating LLVM IR for a Luka AST node.
 *
 * @param[in] n the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM IR builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the generated LLVM IR code for the AST node.
 */
LLVMValueRef GEN_codegen(t_ast_node *n, LLVMModuleRef module,
                         LLVMBuilderRef builder, t_logger *logger);

/**
 * @brief Initializing the codegen environment.
 */
void GEN_codegen_initialize();

/**
 * @brief Resetting the codegen environment.
 */
void GEN_codegen_reset();

#endif // __GEN_H__
