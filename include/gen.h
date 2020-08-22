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
    LLVMValueRef value;
    LLVMTypeRef type;
    UT_hash_handle hh;
} t_named_value;

LLVMValueRef GEN_codegen(t_ast_node *n, LLVMModuleRef module, LLVMBuilderRef builder, t_logger *logger);

void GEN_codegen_reset();

#endif // __GEN_H__
