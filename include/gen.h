#ifndef __GEN_H_
#define __GEN_H_

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>

#include "defs.h"
#include "uthash.h"

typedef struct {
    const char *name;
    LLVMValueRef value;
    UT_hash_handle hh;
} named_value_t;

LLVMValueRef codegen(ASTnode *n, LLVMModuleRef module, LLVMBuilderRef builder);

void codegen_reset();

#endif // __GEN_H_
