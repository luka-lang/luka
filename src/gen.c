#include "gen.h"

#include <stdio.h>
#include <stdlib.h>

t_named_value *named_values = NULL;

LLVMTypeRef gen_type_to_llvm_type(t_type type, t_logger *logger)
{
    switch (type)
    {
    case TYPE_INT32:
        return LLVMInt32Type();
    case TYPE_STRING:
        return LLVMPointerType(LLVMInt8Type(), 0);
    case TYPE_VOID:
        return LLVMVoidType();
    case TYPE_DOUBLE:
        return LLVMDoubleType();
    case TYPE_FLOAT:
        return LLVMFloatType();
    case TYPE_INT1:
        return LLVMInt1Type();
    case TYPE_INT8:
        return LLVMInt8Type();
    case TYPE_INT16:
        return LLVMInt16Type();
    case TYPE_INT64:
        return LLVMInt64Type();
    case TYPE_INT128:
        return LLVMInt128Type();

    default:
        (void) LOGGER_log(logger, L_ERROR, "I don't know how to translate type %d to LLVM types.\n",
                          type);
        (void) exit(LUKA_CODEGEN_ERROR);
    }
}

LLVMValueRef gen_create_entry_block_allca(LLVMValueRef function,
                                          LLVMTypeRef type,
                                          const char *var_name)
{
    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, LLVMGetEntryBasicBlock(function));
    LLVMBuildAlloca(builder, type, var_name);
    LLVMDisposeBuilder(builder);
}

LLVMValueRef gen_codegen_binexpr(t_ast_node *n,
                                 LLVMModuleRef module,
                                 LLVMBuilderRef builder,
                                 t_logger *logger)
{
    LLVMValueRef lhs = NULL, rhs = NULL;

    if (NULL != n->binary_expr.lhs)
    {
        lhs = GEN_codegen(n->binary_expr.lhs, module, builder, logger);
    }

    if (NULL != n->binary_expr.rhs)
    {
        rhs = GEN_codegen(n->binary_expr.rhs, module, builder, logger);
    }

    if ((NULL == lhs) || (NULL == rhs))
    {
        (void) LOGGER_log(logger, L_ERROR, "Binexpr lhs or rhs is null.\n");
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    switch (n->binary_expr.operator)
    {
    case BINOP_ADD:
        return LLVMBuildAdd(builder, lhs, rhs, "addtmp");
    case BINOP_SUBTRACT:
        return LLVMBuildSub(builder, lhs, rhs, "subtmp");
    case BINOP_MULTIPLY:
        return LLVMBuildMul(builder, lhs, rhs, "multmp");
    case BINOP_DIVIDE:
        return LLVMBuildExactSDiv(builder, lhs, rhs, "divtmp");
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
    default:
    {
        (void) LOGGER_log(logger, L_ERROR, "No handler found for op: %d\n", n->binary_expr.operator);
        (void) exit(LUKA_CODEGEN_ERROR);
    }
    }

    return NULL;
}

LLVMValueRef gen_codegen_prototype(t_ast_node *n,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder,
                                   t_logger *logger)
{
    LLVMValueRef func = NULL;
    LLVMTypeRef func_type = NULL;
    size_t i = 0, arity = n->prototype.arity;
    LLVMTypeRef *params = calloc(arity, sizeof(LLVMTypeRef));
    if (NULL == params)
    {
        (void) LOGGER_log(logger, L_ERROR, "Failed to assign memory for params in prototype generaion.\n");
        (void) exit(LUKA_CANT_ALLOC_MEMORY);
    }

    for (i = 0; i < arity; ++i)
    {
        params[i] = gen_type_to_llvm_type(n->prototype.types[i], logger);
    }
    func_type = LLVMFunctionType(gen_type_to_llvm_type(n->prototype.return_type, logger),
                                 params, arity, 0);

    func = LLVMAddFunction(module, n->prototype.name, func_type);
    (void) LLVMSetLinkage(func, LLVMExternalLinkage);

    for (i = 0; i < arity; ++i)
    {
        LLVMValueRef param = LLVMGetParam(func, i);
        (void) LLVMSetValueName(param, n->prototype.args[i]);

        t_named_value *val = malloc(sizeof(t_named_value));
        val->name = strdup(n->prototype.args[i]);
        val->value = param;
        val->type = params[i];
        HASH_ADD_KEYPTR(hh, named_values, val->name, strlen(val->name), val);
    }

cleanup:
    if (NULL != params)
    {
        (void) free(params);
    }

    return func;

}

LLVMValueRef gen_codegen_stmts(t_vector *statements,
                               LLVMModuleRef module,
                               LLVMBuilderRef builder,
                               bool *has_return_stmt,
                               t_logger *logger)
{
    t_ast_node *stmt = NULL;
    LLVMValueRef ret_val = NULL;
    int i = 0;

    VECTOR_FOR_EACH(statements, stmts)
    {
        stmt = ITERATOR_GET_AS(t_ast_node_ptr, &stmts);
        if (AST_TYPE_RETURN_STMT == stmt->type)
        {
            ret_val = GEN_codegen(stmt, module, builder, logger);

            if (NULL != has_return_stmt)
            {
                *has_return_stmt = true;
            }
            break;
        }
        ret_val = GEN_codegen(stmt, module, builder, logger);
    }


    return ret_val;
}

LLVMValueRef gen_codegen_function(t_ast_node *n,
                                  LLVMModuleRef module,
                                  LLVMBuilderRef builder,
                                  t_logger *logger)
{
    LLVMValueRef func = NULL, tmp = NULL, ret_val = NULL;
    LLVMBasicBlockRef block = NULL;
    t_ast_node *stmt = NULL;
    bool has_return_stmt = false;
    t_type return_type;

    func = GEN_codegen(n->function.prototype, module, builder, logger);
    if (NULL == func)
    {
        (void) LOGGER_log(logger, L_ERROR, "Prototype generation failed in function generation\n");
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    if (NULL == n->function.body)
    {
        /* Extern definition */
        return func;
    }

    block = LLVMAppendBasicBlock(func, "entry");
    (void) LLVMPositionBuilderAtEnd(builder, block);

    ret_val = gen_codegen_stmts(n->function.body, module, builder, &has_return_stmt, logger);
    return_type = n->function.prototype->prototype.return_type;

    if (false == has_return_stmt)
    {
        switch (return_type)
        {
        case TYPE_VOID:
            ret_val = NULL;
            break;
        case TYPE_FLOAT:
        case TYPE_DOUBLE:
            ret_val = LLVMConstReal(gen_type_to_llvm_type(return_type, logger), 0.0);
            break;
        default:
            ret_val = LLVMConstInt(gen_type_to_llvm_type(return_type, logger), 0, false);
            break;
        }
        (void) LLVMBuildRet(builder, ret_val);
    }

    if (1 == LLVMVerifyFunction(func, LLVMPrintMessageAction))
    {
        (void) LOGGER_log(logger, L_ERROR, "Invalid function.\n");
        (void) LLVMDeleteFunction(func);
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    return func;
}

LLVMValueRef gen_codegen_return_stmt(t_ast_node *n,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder,
                                     t_logger *logger)
{
    LLVMValueRef expr;
    if(NULL != n->return_stmt.expr)
    {
        expr = GEN_codegen(n->return_stmt.expr, module, builder, logger);
        if (NULL == expr)
        {
            (void) LOGGER_log(logger, L_ERROR, "Expression generation failed in return stmt\n");
            (void) exit(LUKA_CODEGEN_ERROR);
        }
    }

    (void) LLVMBuildRet(builder, expr);
    return NULL;
}

LLVMValueRef gen_codegen_if_expr(t_ast_node *n,
                                 LLVMModuleRef module,
                                 LLVMBuilderRef builder,
                                 t_logger *logger)
{
    LLVMValueRef cond = NULL, then_value = NULL, else_value = NULL, phi = NULL,
                 zero = NULL, func = NULL, incoming_values[2] = {NULL, NULL};
    LLVMBasicBlockRef cond_block = NULL, then_block = NULL, else_block = NULL,
                      merge_block = NULL, incoming_blocks[2] = {NULL, NULL};

    func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

    cond_block = LLVMAppendBasicBlock(func, "if_cond");
    then_block = LLVMAppendBasicBlock(func, "then");
    if(NULL != n->if_expr.else_body)
    {
        else_block = LLVMAppendBasicBlock(func, "else");
    }
    merge_block = LLVMAppendBasicBlock(func, "if_merge");

    (void) LLVMBuildBr(builder, cond_block);
    (void) LLVMPositionBuilderAtEnd(builder, cond_block);

    cond = GEN_codegen(n->if_expr.cond, module, builder, logger);
    if (NULL == cond)
    {
        (void) LOGGER_log(logger, L_ERROR, "Condition generation failed in if expr\n");
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    if (NULL != n->if_expr.else_body)
    {
        (void) LLVMBuildCondBr(builder, cond, then_block, else_block);
    }
    else
    {
        (void) LLVMBuildCondBr(builder, cond, then_block, merge_block);
    }

    (void) LLVMPositionBuilderAtEnd(builder, then_block);

    then_value = gen_codegen_stmts(n->if_expr.then_body, module, builder, NULL, logger);
    if (NULL == then_value)
    {
        (void) LOGGER_log(logger, L_ERROR, "Then value generation failed in if expr\n");
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    (void) LLVMBuildBr(builder, merge_block);

    then_block = LLVMGetInsertBlock(builder);

    if (NULL != n->if_expr.else_body)
    {
        (void) LLVMPositionBuilderAtEnd(builder, else_block);
        else_value = gen_codegen_stmts(n->if_expr.else_body, module, builder, NULL, logger);
        if (NULL == else_value)
        {
            (void) LOGGER_log(logger, L_ERROR, "Else value generation failed in if expr\n");
            (void) exit(LUKA_CODEGEN_ERROR);
        }
        (void) LLVMBuildBr(builder, merge_block);
    }

    else_block = LLVMGetInsertBlock(builder);

    (void) LLVMPositionBuilderAtEnd(builder, merge_block);
    phi = LLVMBuildPhi(builder, LLVMInt32Type(), "phi");

    (void) LLVMAddIncoming(phi, &then_value, &then_block, 1);

    if (NULL != n->if_expr.else_body)
    {
        (void) LLVMAddIncoming(phi, &else_value, &else_block, 1);
    }
    else
    {
        incoming_values[0] = LLVMConstInt(LLVMInt32Type(), 0, 0);
        (void) LLVMAddIncoming(phi, incoming_values, &cond_block, 1);
    }

    return phi;
}

LLVMValueRef gen_codegen_while_expr(t_ast_node *n,
                                 LLVMModuleRef module,
                                 LLVMBuilderRef builder,
                                 t_logger *logger)
{
    LLVMValueRef func = NULL, cond = NULL, body_value = NULL;
    LLVMBasicBlockRef cond_block = NULL, body_block = NULL, end_block = NULL;

    func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

    cond_block = LLVMAppendBasicBlock(func, "while_cond");
    body_block = LLVMAppendBasicBlock(func, "while_body");
    end_block = LLVMAppendBasicBlock(func, "while_end");

    (void) LLVMBuildBr(builder, cond_block);
    (void) LLVMPositionBuilderAtEnd(builder, cond_block);

    cond = GEN_codegen(n->while_expr.cond, module, builder, logger);
    if (NULL == cond)
    {
        (void) LOGGER_log(logger, L_ERROR, "Condition generation failed in while expr\n");
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    (void) LLVMBuildCondBr(builder, cond, body_block, end_block);
    (void) LLVMPositionBuilderAtEnd(builder, body_block);

    body_value = gen_codegen_stmts(n->while_expr.body, module, builder, NULL, logger);
    cond = GEN_codegen(n->while_expr.cond, module, builder, logger);
    if (NULL == cond)
    {
        (void) LOGGER_log(logger, L_ERROR, "Condition generation failed in while expr\n");
        (void) exit(LUKA_CODEGEN_ERROR);
    }
    (void) LLVMBuildCondBr(builder, cond, body_block, end_block);
    (void) LLVMPositionBuilderAtEnd(builder, end_block);

    return body_value;
}

LLVMValueRef gen_codegen_variable(t_ast_node *node,
                                  LLVMModuleRef module,
                                  LLVMBuilderRef builder,
                                  t_logger *logger)
{
    t_named_value *val = NULL;

    HASH_FIND_STR(named_values, node->variable.name, val);

    if (NULL != val)
    {
        return val->value;
    }

    (void) LOGGER_log(logger, L_ERROR, "Variable %s is undefined.\n", node->variable.name);
    (void) exit(LUKA_CODEGEN_ERROR);
}

LLVMValueRef gen_codegen_let_stmt(t_ast_node *node,
                                  LLVMModuleRef module,
                                  LLVMBuilderRef builder,
                                  t_logger *logger)
{
    LLVMValueRef expr = NULL;
    t_ast_variable variable;
    t_named_value *val = NULL;

    variable = node->let_stmt.var->variable;
    expr = GEN_codegen(node->let_stmt.expr, module, builder, logger);
    if (NULL == expr)
    {
        (void) LOGGER_log(logger, L_ERROR, "Expression generation in let stmt failed.\n");
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    val = malloc(sizeof(t_named_value));
    val->name = strdup(variable.name);
    val->value = expr;
    val->type = gen_type_to_llvm_type(variable.type, logger);
    val->mutable = variable.mutable;
    HASH_ADD_KEYPTR(hh, named_values, val->name, strlen(val->name), val);

    return NULL;
}

LLVMValueRef gen_codegen_assignment_stmt(t_ast_node *node,
                                         LLVMModuleRef module,
                                         LLVMBuilderRef builder,
                                         t_logger *logger)
{
    LLVMValueRef expr = NULL;
    char *var_name = NULL;
    t_named_value *val = NULL;

    var_name = node->assignment_stmt.var_name;
    expr = GEN_codegen(node->assignment_stmt.expr, module, builder, logger);
    if (NULL == expr)
    {
        (void) LOGGER_log(logger, L_ERROR, "Expression generation in assignment stmt failed.\n");
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    HASH_FIND_STR(named_values, var_name, val);
    if (NULL == val)
    {
        (void) LOGGER_log(logger, L_ERROR, "Cannot assign to undeclared variable.\n");
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    if (!val->mutable)
    {
        (void) LOGGER_log(logger, L_ERROR, "Tried to assign to an immutable variable.\n");
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    val->value = expr;
    return expr;
}

LLVMValueRef gen_codegen_call(t_ast_node *node,
                              LLVMModuleRef module,
                              LLVMBuilderRef builder,
                              t_logger *logger)
{
    LLVMValueRef call = NULL;
    LLVMValueRef func = NULL;
    LLVMValueRef *args = NULL;
    t_ast_node *arg = NULL;
    size_t i = 0;

    func = LLVMGetNamedFunction(module, node->call_expr.name);
    if (NULL == func)
    {
        goto cleanup;
    }

    if (node->call_expr.args->size != LLVMCountParams(func))
    {
        goto cleanup;
    }

    args = calloc(node->call_expr.args->size, sizeof(LLVMValueRef));
    if (NULL == args)
    {
        goto cleanup;
    }

    VECTOR_FOR_EACH(node->call_expr.args, args_iter)
    {
        arg = ITERATOR_GET_AS(t_ast_node_ptr, &args_iter);
        args[i] = GEN_codegen(arg, module, builder, logger);

        if (NULL == args[i])
        {
            goto cleanup;
        }

        ++i;
    }

    call = LLVMBuildCall(builder, func, args, node->call_expr.args->size,
                         "calltmp");

cleanup:
    if (NULL != args)
    {
        (void) free(args);
        args = NULL;
    }

    return call;
}

LLVMValueRef gen_codegen_expression_stmt(t_ast_node *n,
                                         LLVMModuleRef module,
                                         LLVMBuilderRef builder,
                                         t_logger *logger)
{
    LLVMValueRef expr;
    if (NULL != n->expression_stmt.expr)
    {
        (void) GEN_codegen(n->expression_stmt.expr, module, builder, logger);

    }

    return NULL;
}

LLVMValueRef GEN_codegen(t_ast_node *node,
                         LLVMModuleRef module,
                         LLVMBuilderRef builder,
                         t_logger *logger)
{
    switch (node->type)
    {
    case AST_TYPE_NUMBER:
        return LLVMConstInt(LLVMInt32Type(), node->number.value, 0);
    case AST_TYPE_STRING:
        return LLVMBuildGlobalStringPtr(builder, node->string.value, "str");
    case AST_TYPE_BINARY_EXPR:
        return gen_codegen_binexpr(node, module, builder, logger);
    case AST_TYPE_PROTOTYPE:
        return gen_codegen_prototype(node, module, builder, logger);
    case AST_TYPE_FUNCTION:
        return gen_codegen_function(node, module, builder, logger);
    case AST_TYPE_RETURN_STMT:
        return gen_codegen_return_stmt(node, module, builder, logger);
    case AST_TYPE_IF_EXPR:
        return gen_codegen_if_expr(node, module, builder, logger);
    case AST_TYPE_WHILE_EXPR:
        return gen_codegen_while_expr(node, module, builder, logger);
    case AST_TYPE_VARIABLE:
        return gen_codegen_variable(node, module, builder, logger);
    case AST_TYPE_LET_STMT:
        return gen_codegen_let_stmt(node, module, builder, logger);
    case AST_TYPE_ASSIGNMENT_STMT:
        return gen_codegen_assignment_stmt(node, module, builder, logger);
    case AST_TYPE_CALL_EXPR:
        return gen_codegen_call(node, module, builder, logger);
    case AST_TYPE_EXPRESSION_STMT:
        return gen_codegen_expression_stmt(node, module, builder, logger);
    default:
    {
        (void) LOGGER_log(logger, L_ERROR, "No codegen function was found for type - %d\n", node->type);
        (void) exit(LUKA_CODEGEN_ERROR);
    }
    }
}

void GEN_codegen_reset()
{
    t_named_value *named_value, *tmp;

    HASH_ITER(hh, named_values, named_value, tmp) {
        HASH_DEL(named_values, named_value);
        if (NULL != named_value)
        {
            if (NULL != named_value->name)
            {
                (void) free((char *) named_value->name);
                named_value->name = NULL;
            }
            (void) free(named_value);
            named_value = NULL;
        }
    }
}
