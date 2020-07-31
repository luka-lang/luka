#include "gen.h"

#include <stdio.h>
#include <stdlib.h>

named_value_t *named_values = NULL;

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
  case BINOP_NOT:
    return LLVMBuildNot(builder, lhs, "nottmp");
  case BINOP_LESSER:
    return LLVMBuildICmp(builder, LLVMIntSLT, lhs, rhs, "lessertmp");
  case BINOP_GREATER:
    return LLVMBuildICmp(builder, LLVMIntSGT, lhs, rhs, "greatertmp");
  case BINOP_EQUALS:
    return LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "eqeqtmp");
  case BINOP_NEQ:
    return LLVMBuildICmp(builder, LLVMIntNE, lhs, rhs, "neqtmp");
  case BINOP_LEQ:
    return LLVMBuildICmp(builder, LLVMIntSLE, lhs, rhs, "leqtmp");
  case BINOP_GEQ:
    return LLVMBuildICmp(builder, LLVMIntSGE, lhs, rhs, "geqtmp");
  default: {
    fprintf(stderr, "No handler found for op: %d\n", n->binary_expr.operator);
    exit(1);
  }
  }

  return NULL;
}

LLVMValueRef codegen_prototype(ASTnode *n, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  LLVMValueRef func = NULL;
  LLVMTypeRef func_type = NULL;
  size_t i = 0, arity = n->prototype.arity;
  LLVMTypeRef *params = calloc(arity, sizeof(LLVMTypeRef));
  for (i = 0; i < arity; ++i) {
    // int32_t by default
    params[i] = LLVMInt32Type();
  }
  func_type = LLVMFunctionType(LLVMInt32Type(), params, arity, 0);

  func = LLVMAddFunction(module, n->prototype.name, func_type);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  for (i = 0; i < arity; ++i) {
    LLVMValueRef param = LLVMGetParam(func, i);
    LLVMSetValueName(param, n->prototype.args[i]);

    named_value_t *val = malloc(sizeof(named_value_t));
    val->name = strdup(n->prototype.args[i]);
    val->value = param;
    HASH_ADD_KEYPTR(hh, named_values, val->name, strlen(val->name), val);
  }

  return func;
}

LLVMValueRef codegen_stmts(Vector *statements, LLVMModuleRef module,
                           LLVMBuilderRef builder, bool *has_return_stmt) {
  ASTnode *stmt = NULL;
  LLVMValueRef ret_val = NULL;
  int i = 0;

  VECTOR_FOR_EACH(statements, stmts) {
    stmt = ITERATOR_GET_AS(ast_node_ptr_t, &stmts);
    if (AST_TYPE_RETURN_STMT == stmt->type) {
      ret_val = codegen(stmt, module, builder);
      if (NULL != has_return_stmt) {
        *has_return_stmt = true;
      }
      break;
    }
    ret_val = codegen(stmt, module, builder);
  }

  return ret_val;
}

LLVMValueRef codegen_function(ASTnode *n, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  LLVMValueRef func = NULL, tmp = NULL, ret_val = NULL;
  LLVMBasicBlockRef block = NULL;
  ASTnode *stmt = NULL;
  bool has_return_stmt = false;

  func = codegen(n->function.prototype, module, builder);
  if (NULL == func) {
    return NULL;
  }

  if (NULL == n->function.body) {
    /* Extern definition */
    return func;
  }

  block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(builder, block);

  ret_val = codegen_stmts(n->function.body, module, builder, &has_return_stmt);

  if (false == has_return_stmt) {
    ret_val = LLVMConstInt(LLVMInt32Type(), 0, false);
    LLVMBuildRet(builder, ret_val);
  }

  if (1 == LLVMVerifyFunction(func, LLVMPrintMessageAction)) {
    fprintf(stderr, "Invalid function");
    LLVMDeleteFunction(func);
    return NULL;
  }

  LLVMDumpModule(module);

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

LLVMValueRef codegen_if_expr(ASTnode *n, LLVMModuleRef module,
                             LLVMBuilderRef builder) {
  LLVMValueRef cond = NULL, then_value = NULL, else_value = NULL, phi = NULL,
               zero = NULL, func = NULL, incoming_values[2] = {NULL, NULL};
  LLVMBasicBlockRef cond_block = NULL, then_block = NULL, else_block = NULL,
                    merge_block = NULL, incoming_blocks[2] = {NULL, NULL};

  func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

  cond_block = LLVMAppendBasicBlock(func, "if_cond");
  then_block = LLVMAppendBasicBlock(func, "then");
  if (n->if_expr.else_body) {
    else_block = LLVMAppendBasicBlock(func, "else");
  }
  merge_block = LLVMAppendBasicBlock(func, "if_merge");

  LLVMBuildBr(builder, cond_block);
  LLVMPositionBuilderAtEnd(builder, cond_block);

  cond = codegen(n->if_expr.cond, module, builder);
  if (NULL == cond) {
    return NULL;
  }

  if (NULL != n->if_expr.else_body) {
    LLVMBuildCondBr(builder, cond, then_block, else_block);
  } else {
    LLVMBuildCondBr(builder, cond, then_block, merge_block);
  }

  LLVMPositionBuilderAtEnd(builder, then_block);

  then_value = codegen_stmts(n->if_expr.then_body, module, builder, NULL);
  if (NULL == then_value) {
    return NULL;
  }

  LLVMBuildBr(builder, merge_block);

  then_block = LLVMGetInsertBlock(builder);

  if (NULL != n->if_expr.else_body) {
    LLVMPositionBuilderAtEnd(builder, else_block);
    else_value = codegen_stmts(n->if_expr.else_body, module, builder, NULL);
    if (NULL == else_value) {
      return NULL;
    }
    LLVMBuildBr(builder, merge_block);
  }

  else_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, merge_block);
  phi = LLVMBuildPhi(builder, LLVMInt32Type(), "phi");

  LLVMAddIncoming(phi, &then_value, &then_block, 1);

  if (NULL != n->if_expr.else_body) {
    LLVMAddIncoming(phi, &else_value, &else_block, 1);
  } else {
    incoming_values[0] = LLVMConstInt(LLVMInt32Type(), 0, 0);
    LLVMAddIncoming(phi, incoming_values, &cond_block, 1);
  }

  return phi;
}

LLVMValueRef codegen_variable(ASTnode *node, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  named_value_t *val = NULL;

  HASH_FIND_STR(named_values, node->variable.name, val);

  if (NULL != val) {
    return val->value;
  }
  return NULL;
}

LLVMValueRef codegen_let_stmt(ASTnode *node, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  LLVMValueRef expr;
  AST_variable variable;
  named_value_t *val = NULL;

  variable = node->let_stmt.var->variable;
  expr = codegen(node->let_stmt.expr, module, builder);

  val = malloc(sizeof(named_value_t));
  val->name = strdup(variable.name);
  val->value = expr;
  HASH_ADD_KEYPTR(hh, named_values, val->name, strlen(val->name), val);

  return NULL;
}

LLVMValueRef codegen_call(ASTnode *node, LLVMModuleRef module,
                          LLVMBuilderRef builder) {
  LLVMValueRef func = NULL;
  LLVMValueRef *args = NULL;
  ASTnode *arg = NULL;
  size_t i = 0;

  func = LLVMGetNamedFunction(module, node->call_expr.name);
  if (NULL == func) {
    return NULL;
  }

  if (node->call_expr.args->size != LLVMCountParams(func)) {
    return NULL;
  }

  args = calloc(node->call_expr.args->size, sizeof(LLVMValueRef));
  VECTOR_FOR_EACH(node->call_expr.args, args_iter) {
    arg = ITERATOR_GET_AS(ast_node_ptr_t, &args_iter);
    args[i] = codegen(arg, module, builder);

    if (NULL == args[i]) {
      free(args);
      return NULL;
    }

    ++i;
  }

  return LLVMBuildCall(builder, func, args, node->call_expr.args->size,
                       "calltmp");
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
  case AST_TYPE_IF_EXPR:
    return codegen_if_expr(node, module, builder);
  case AST_TYPE_VARIABLE:
    return codegen_variable(node, module, builder);
  case AST_TYPE_LET_STMT:
    return codegen_let_stmt(node, module, builder);
  case AST_TYPE_CALL_EXPR:
    return codegen_call(node, module, builder);
  default: {
    printf("No codegen function was found for type - %d\n", node->type);
    return NULL;
  }
  }
}

void codegen_reset() { HASH_CLEAR(hh, named_values); }
