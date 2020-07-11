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

LLVMValueRef codegen_prototype(ASTnode *n, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  LLVMTypeRef func_type = NULL;
  int arity = n->prototype.arity;
  LLVMTypeRef params[arity];
  for (int i = 0; i < arity; ++i) {
    // int32_t by default
    params[i] = LLVMInt32Type();
  }
  func_type = LLVMFunctionType(LLVMInt32Type(), params, arity, 0);

  return LLVMAddFunction(module, n->function.prototype->prototype.name,
                         func_type);
}

LLVMValueRef codegen_function(ASTnode *n, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  LLVMValueRef func = NULL, tmp = NULL;
  LLVMBasicBlockRef entry = NULL;
  ASTnode *stmt = NULL;
  bool has_return_value = false;

  func = codegen_prototype(n, module, builder);

  entry = LLVMAppendBasicBlock(func, "entry");
  builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(builder, entry);

  VECTOR_FOR_EACH(n->function.body, stmts) {
    stmt = ITERATOR_GET_AS(ast_node_ptr_t, &stmts);
    if (AST_TYPE_RETURN_STMT == stmt->type) {
      if (!has_return_value) {
        codegen(stmt, module, builder);
      }
      has_return_value = true;
    }
  }

  if (!has_return_value) {
    tmp = LLVMConstInt(LLVMInt32Type(), 0, 0);
    LLVMBuildRet(builder, tmp);
  }

  return func;
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
  case AST_TYPE_PROTOTYPE:
    return codegen_prototype(node, module, builder);
  case AST_TYPE_FUNCTION:
    return codegen_function(node, module, builder);
  case AST_TYPE_RETURN_STMT:
    return codegen_return_stmt(node, module, builder);
  }

  return NULL;
}
