#ifndef __GEN_H__
#define __GEN_H__

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>

#include "defs.h"
#include "uthash.h"

typedef struct
{
    const char *name;
    LLVMValueRef value;
    LLVMTypeRef type;
    UT_hash_handle hh;
} named_value_t;

LLVMValueRef codegen(ASTnode *n, LLVMModuleRef module, LLVMBuilderRef builder);

void codegen_reset();

#endif // __GEN_H__
