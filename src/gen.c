#include "../include/gen.h"

#include <stdlib.h>

LLVMValueRef codegen_binexpr(ASTnode *n, LLVMModuleRef module,
                             LLVMBuilderRef builder) {
  LLVMValueRef lhs = NULL, rhs = NULL;
  if (n->binary_expr.lhs) {
    lhs = codegen(n->binary_expr.lhs, module, builder);
  }
  if (n->binary_expr.rhs) {
    rhs = codegen(n->binary_expr.rhs, module, builder);
  }

  if ((NULL == lhs) || (NULL == rhs)) {
    return NULL;
  }

  switch (n->binary_expr.operator) {
  case BINOP_ADD:
    return LLVMBuildAdd(builder, lhs, rhs, "addtmp");
  case BINOP_SUBTRACT:
    return LLVMBuildSub(builder, lhs, rhs, "subtmp");
  case BINOP_MULTIPLY:
    return LLVMBuildMul(builder, lhs, rhs, "multmp");
  case BINOP_DIVIDE:
    return LLVMBuildFDiv(builder, lhs, rhs, "divtmp");
  }

  return NULL;
}

LLVMValueRef codegen_return_stmt(ASTnode *n, LLVMModuleRef module,
                                 LLVMBuilderRef builder) {
  LLVMValueRef expr;
  if (n->return_stmt.expr) {
    expr = codegen(n->return_stmt.expr, module, builder);
  }

  LLVMBuildRet(builder, expr);
  return NULL;
}

LLVMValueRef codegen(ASTnode *node, LLVMModuleRef module,
                     LLVMBuilderRef builder) {
  switch (node->type) {
  case AST_TYPE_NUMBER:
    return LLVMConstInt(LLVMInt32Type(), node->number.value, 0);
  case AST_TYPE_BINARY_EXPR:
    return codegen_binexpr(node, module, builder);
  case AST_TYPE_RETURN_STMT:
    return codegen_return_stmt(node, module, builder);
  }

  return NULL;
}
