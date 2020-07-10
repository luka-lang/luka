#ifndef __GEN_H_
#define __GEN_H_

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>

#include "defs.h"

LLVMValueRef codegen(ASTnode *n, LLVMModuleRef module, LLVMBuilderRef builder);

#endif // __GEN_H_
