#ifndef __GEN_H__
#define __GEN_H__

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>

#include "defs.h"
#include "logger.h"
#include "uthash.h"

typedef struct
{
    const char *name;
    LLVMValueRef alloca_inst;
    LLVMTypeRef type;
    t_type *ttype;
    bool mutable;
    UT_hash_handle hh;
} t_named_value;

typedef struct
{
    LLVMTypeRef struct_type;
    char *struct_name;
    char **struct_fields;
    size_t number_of_fields;
    UT_hash_handle hh;
} t_struct_info;

typedef struct
{
    char *enum_name;
    char **enum_field_names;
    int *enum_field_values;
    size_t number_of_fields;
    UT_hash_handle hh;
} t_enum_info;

LLVMValueRef GEN_codegen(t_ast_node *n, LLVMModuleRef module, LLVMBuilderRef builder, t_logger *logger);

void GEN_codegen_initialize();
void GEN_codegen_reset();

#endif // __GEN_H__
