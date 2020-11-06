#include "gen.h"

#include <stdio.h>
#include <stdlib.h>

#include "type.h"

t_named_value *named_values = NULL;
t_struct_info *struct_infos = NULL;
Vector *loop_blocks = NULL;

LLVMValueRef gen_get_struct_field_pointer(t_named_value *variable,
                                          char *key,
                                          LLVMBuilderRef builder,
                                          t_logger *logger);
bool gen_llvm_cast_sizes_if_needed(LLVMValueRef *lhs, LLVMValueRef *rhs, LLVMBuilderRef builder, t_logger *logger);

LLVMTypeRef gen_type_to_llvm_type(t_type *type, t_logger *logger)
{
    t_struct_info *struct_info = NULL;

    switch (type->type)
    {
    case TYPE_ANY:
        return LLVMInt8Type();
    case TYPE_BOOL:
        return LLVMInt1Type();
    case TYPE_SINT8:
        return LLVMInt8Type();
    case TYPE_SINT16:
        return LLVMInt16Type();
    case TYPE_SINT32:
        return LLVMInt32Type();
    case TYPE_SINT64:
        return LLVMInt64Type();
    case TYPE_UINT8:
        return LLVMInt8Type();
    case TYPE_UINT16:
        return LLVMInt16Type();
    case TYPE_UINT32:
        return LLVMInt32Type();
    case TYPE_UINT64:
        return LLVMInt64Type();
    case TYPE_F32:
        return LLVMFloatType();
    case TYPE_F64:
        return LLVMDoubleType();
    case TYPE_STRING:
        return LLVMPointerType(LLVMInt8Type(), 0);
    case TYPE_VOID:
        return LLVMVoidType();
    case TYPE_PTR:
        return LLVMPointerType(gen_type_to_llvm_type(type->inner_type, logger), 0);
    case TYPE_STRUCT:
        HASH_FIND_STR(struct_infos, (char*)type->payload, struct_info);
        if (NULL != struct_info)
        {
            return struct_info->struct_type;
        }

        (void) LOGGER_log(logger, L_ERROR, "I don't know how to translate struct named %s to LLVM types without a previous definition.\n",
                          type);
        (void) exit(LUKA_CODEGEN_ERROR);

    default:
        (void) LOGGER_log(logger, L_ERROR, "I don't know how to translate type %d to LLVM types.\n",
                          type);
        (void) exit(LUKA_CODEGEN_ERROR);
    }
}

t_type *gen_llvm_type_to_ttype(LLVMTypeRef type, t_logger *logger)
{
    t_type *ttype = calloc(1, sizeof(t_type));
    if (NULL == type)
    {
        (void) exit(LUKA_CANT_ALLOC_MEMORY);
    }

    ttype->payload = NULL;
    ttype->inner_type = NULL;
    if (type == LLVMPointerType(LLVMVoidType(), 0))
    {
        ttype->type = TYPE_ANY;
    }
    else if (type == LLVMInt1Type())
    {
        ttype->type = TYPE_BOOL;
    }
    else if (type == LLVMInt8Type())
    {
        ttype->type = TYPE_SINT8;
    }
    else if (type == LLVMInt16Type())
    {
        ttype->type = TYPE_SINT16;
    }
    else if (type == LLVMInt32Type())
    {
        ttype->type = TYPE_SINT32;
    }
    else if (type == LLVMInt64Type())
    {
        ttype->type = TYPE_SINT64;
    }
    else if (type == LLVMInt8Type())
    {
        ttype->type = TYPE_UINT8;
    }
    else if (type == LLVMInt16Type())
    {
        ttype->type = TYPE_UINT16;
    }
    else if (type == LLVMInt32Type())
    {
        ttype->type = TYPE_UINT32;
    }
    else if (type == LLVMInt64Type())
    {
        ttype->type = TYPE_UINT64;
    }
    else if (type == LLVMFloatType())
    {
        ttype->type = TYPE_F32;
    }
    else if (type == LLVMDoubleType())
    {
        ttype->type = TYPE_F64;
    }
    else if (type == LLVMPointerType(LLVMInt8Type(), 0))
    {
        ttype->type = TYPE_STRING;
    }
    else if (type == LLVMVoidType())
    {
        ttype->type = TYPE_VOID;
    }
    else if (LLVMGetTypeKind(type) == LLVMPointerTypeKind)
    {
        ttype->type = TYPE_PTR;
        ttype->inner_type = gen_llvm_type_to_ttype(LLVMGetElementType(type), logger);
    }
    else if (LLVMStructTypeKind == LLVMGetTypeKind(type))
    {
        ttype->type = TYPE_STRUCT;
        ttype->payload = (void *)LLVMGetStructName(type);
    }
    else
    {
        (void) LLVMDumpType(type);
        (void) LOGGER_log(logger, L_ERROR, "I don't know how to translate LLVM type %d to t_type.\n",
                            type);
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    return ttype;
}

LLVMValueRef gen_create_entry_block_allca(LLVMValueRef function,
                                          LLVMTypeRef type,
                                          const char *var_name)
{
    LLVMBuilderRef builder = NULL;
    LLVMValueRef alloca_inst = NULL;
    LLVMBasicBlockRef entry_block = NULL;
    LLVMValueRef inst = NULL;
    builder = LLVMCreateBuilder();
    entry_block = LLVMGetEntryBasicBlock(function);
    inst = LLVMGetFirstInstruction(entry_block);
    LLVMPositionBuilderAtEnd(builder, entry_block);
    if (inst != NULL) {
        LLVMPositionBuilderBefore(builder, inst);
    } else {
        LLVMPositionBuilderAtEnd(builder, entry_block);
    }
    alloca_inst = LLVMBuildAlloca(builder, type, var_name);
    LLVMDisposeBuilder(builder);
    return alloca_inst;
}

bool ttype_is_signed(t_type *type)
{
    switch (type->type)
    {
    case TYPE_SINT8:
    case TYPE_SINT16:
    case TYPE_SINT32:
    case TYPE_SINT64:
        return true;
    default:
        return false;
    }
}

LLVMOpcode gen_llvm_get_cast_op(LLVMTypeRef type, LLVMTypeRef dest_type, t_logger *logger)
{
    t_type *ttype = gen_llvm_type_to_ttype(type, logger), *dest_ttype = gen_llvm_type_to_ttype(dest_type, logger);
    LLVMTypeKind type_kind = LLVMGetTypeKind(type), dtype_kind = LLVMGetTypeKind(dest_type);
    LLVMOpcode opcode = LLVMBitCast;

    if ((LLVMDoubleTypeKind == type_kind) || (LLVMFloatTypeKind == type_kind))
    {
        if (LLVMDoubleTypeKind == dtype_kind)
        {
            opcode = LLVMFPExt;
        }
        else if (LLVMFloatTypeKind == dtype_kind)
        {
            opcode = LLVMFPTrunc;
        }
        else if (ttype_is_signed(dest_ttype))
        {
            opcode = LLVMFPToSI;
        }
        else
        {
            opcode = LLVMFPToUI;
        }
    }
    else if (LLVMIntegerTypeKind == dtype_kind)
    {
        if ((LLVMDoubleTypeKind == type_kind) || (LLVMFloatTypeKind == type_kind))
        {
            if (ttype_is_signed(ttype))
            {
                opcode = LLVMSIToFP;
            }

            opcode = LLVMUIToFP;
        }

        else if (LLVMGetIntTypeWidth(dest_type) > LLVMGetIntTypeWidth(type))
        {
            if (ttype_is_signed(dest_ttype))
            {
                opcode = LLVMSExt;
            }

            opcode = LLVMZExt;
        }
        else
        {
            opcode = LLVMTrunc;
        }
    }

    if (NULL != ttype)
    {
        (void) free(ttype);
        ttype = NULL;
    }

    if (NULL != dest_ttype)
    {
        (void) free(dest_ttype);
        dest_ttype = NULL;
    }

    return opcode;
}

LLVMValueRef gen_codegen_cast(LLVMBuilderRef builder,
                              LLVMValueRef original_value,
                              LLVMTypeRef dest_type,
                              t_logger *logger)
{
    LLVMTypeRef type = LLVMTypeOf(original_value);

    if ((LLVMPointerTypeKind == LLVMGetTypeKind(type)) && (LLVMPointerTypeKind == LLVMGetTypeKind(dest_type)))
    {
        return LLVMBuildPointerCast(builder, original_value, dest_type, "ptrcasttmp");
    }

    return LLVMBuildCast(builder, gen_llvm_get_cast_op(type, dest_type, logger), original_value, dest_type, "casttmp");
}

bool gen_is_cond_op(t_ast_binop_type op) {
    switch (op) {
    case BINOP_LESSER:
    case BINOP_GREATER:
    case BINOP_EQUALS:
    case BINOP_NEQ:
    case BINOP_LEQ:
    case BINOP_GEQ:
        return true;
    default:
    {
        return false;
    }
    }
}

bool gen_llvm_cast_to_fp_if_needed(LLVMValueRef *lhs, LLVMValueRef *rhs, LLVMBuilderRef builder, t_logger *logger) {
    t_type *lhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(*lhs), logger);
    t_type *rhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(*rhs), logger);

    if (TYPE_is_floating_type(lhs_t) || TYPE_is_floating_type(rhs_t)) {
        if (TYPE_is_floating_type(lhs_t) && !TYPE_is_floating_type(rhs_t)) {
            if (ttype_is_signed(rhs_t)) {
                *rhs = LLVMBuildSIToFP(builder, *rhs, LLVMTypeOf(*lhs), "sitofpcasttmp");
            } else {
                *rhs = LLVMBuildUIToFP(builder, *rhs, LLVMTypeOf(*lhs), "uitofpcasttmp");
            }
        } else if (!TYPE_is_floating_type(lhs_t) && TYPE_is_floating_type(rhs_t)) {
            if (ttype_is_signed(lhs_t)) {
                *lhs = LLVMBuildSIToFP(builder, *lhs, LLVMTypeOf(*rhs), "sitofpcasttmp");
            } else {
                *lhs = LLVMBuildUIToFP(builder, *lhs, LLVMTypeOf(*rhs), "uitofpcasttmp");
            }
        }

        (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
        return true;
    }

    return false;
}

bool gen_llvm_cast_to_signed_if_needed(LLVMValueRef *lhs, LLVMValueRef *rhs, LLVMBuilderRef builder, t_logger *logger) {
    t_type *lhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(*lhs), logger);
    t_type *rhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(*rhs), logger);

    if (ttype_is_signed(lhs_t) || ttype_is_signed(rhs_t)) {
        if (ttype_is_signed(lhs_t) && !ttype_is_signed(rhs_t)) {
            *rhs = LLVMBuildIntCast2(builder, *rhs, LLVMTypeOf(*lhs), true, "signedcasttmp");
        } else if (!ttype_is_signed(lhs_t) && ttype_is_signed(rhs_t)) {
            *lhs = LLVMBuildIntCast2(builder, *lhs, LLVMTypeOf(*rhs), true, "signedcasttmp");
        }

        (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
        return true;
    }

    return false;
}

bool gen_llvm_cast_sizes_if_needed(LLVMValueRef *lhs, LLVMValueRef *rhs, LLVMBuilderRef builder, t_logger *logger) {
    t_type *lhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(*lhs), logger);
    t_type *rhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(*rhs), logger);

    if (lhs_t->type != rhs_t->type)
    {
        if (TYPE_is_floating_type(lhs_t))
        {
            if (TYPE_F32 == lhs_t->type)
            {
                *lhs = LLVMBuildFPExt(builder, *lhs, LLVMTypeOf(*rhs), "fpexttmp");
            }
            else
            {

                *rhs = LLVMBuildFPExt(builder, *rhs, LLVMTypeOf(*lhs), "fpexttmp");
            }

        }
        else if (LLVMGetIntTypeWidth(LLVMTypeOf(*lhs)) > LLVMGetIntTypeWidth(LLVMTypeOf(*rhs)))
        {
            *lhs = LLVMBuildIntCast2(builder, *lhs, LLVMTypeOf(*rhs), ttype_is_signed(rhs_t), "intcasttmp");
        }
        else
        {
            *rhs = LLVMBuildIntCast2(builder, *rhs, LLVMTypeOf(*lhs), ttype_is_signed(lhs_t), "intcasttmp");
        }


        return true;
    }

    return false;
}

LLVMOpcode gen_get_llvm_opcode(t_ast_binop_type op, LLVMValueRef *lhs, LLVMValueRef *rhs, LLVMBuilderRef builder, t_logger *logger) {
    switch (op)
    {
    case BINOP_ADD:
        if (gen_llvm_cast_to_fp_if_needed(lhs, rhs, builder, logger)) {
           return LLVMFAdd;
        }

        (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
        return LLVMAdd;
    case BINOP_SUBTRACT:
        if (gen_llvm_cast_to_fp_if_needed(lhs, rhs, builder, logger)) {
           return LLVMFSub;
        }

        (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
        return LLVMSub;
    case BINOP_MULTIPLY:
        if (gen_llvm_cast_to_fp_if_needed(lhs, rhs, builder, logger)) {
           return LLVMFMul;
        }

        (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
        return LLVMMul;
    case BINOP_DIVIDE:
        if (gen_llvm_cast_to_fp_if_needed(lhs, rhs, builder, logger)) {
           return LLVMFDiv;
        }

        if (gen_llvm_cast_to_signed_if_needed(lhs, rhs, builder, logger)) {
            return LLVMSDiv;
        }

        (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
        return LLVMUDiv;
    case BINOP_MODULOS:
        if (gen_llvm_cast_to_fp_if_needed(lhs, rhs, builder, logger)) {
           return LLVMFRem;
        }

        if (gen_llvm_cast_to_signed_if_needed(lhs, rhs, builder, logger)) {
            return LLVMSRem;
        }

        (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
        return LLVMURem;
    default:
    {
        (void) LOGGER_log(logger, L_ERROR, "No handler found for op: %d\n", op);
        (void) exit(LUKA_CODEGEN_ERROR);
    }
    }
}

bool gen_is_icmp(LLVMValueRef lhs, LLVMValueRef rhs, t_logger *logger) {
    t_type *lhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(lhs), logger);
    t_type *rhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(rhs), logger);
    return !TYPE_is_floating_type(lhs_t) && !TYPE_is_floating_type(rhs_t);
}

LLVMIntPredicate gen_llvm_get_int_predicate(t_ast_binop_type op, LLVMValueRef *lhs, LLVMValueRef *rhs, LLVMBuilderRef builder, t_logger *logger) {
    switch (op) {
    case BINOP_LESSER:
        if (gen_llvm_cast_to_signed_if_needed(lhs, rhs, builder, logger)) {
            return LLVMIntSLE;
        }

        return LLVMIntULE;
    case BINOP_GREATER:
        if (gen_llvm_cast_to_signed_if_needed(lhs, rhs, builder, logger)) {
            return LLVMIntSGT;
        }

        return LLVMIntUGT;
    case BINOP_EQUALS:
        return LLVMIntEQ;
    case BINOP_NEQ:
        return LLVMIntNE;
    case BINOP_LEQ:
        if (gen_llvm_cast_to_signed_if_needed(lhs, rhs, builder, logger)) {
            return LLVMIntSLE;
        }

        return LLVMIntULE;
    case BINOP_GEQ:
        if (gen_llvm_cast_to_signed_if_needed(lhs, rhs, builder, logger)) {
            return LLVMIntSGE;
        }

        return LLVMIntUGE;
    default:
    {
        (void) LOGGER_log(logger, L_ERROR, "Op %d is not a int comparison operator.\n", op);
        (void) exit(LUKA_CODEGEN_ERROR);
    }
    }
}

LLVMRealPredicate gen_llvm_get_real_predicate(t_ast_binop_type op, LLVMValueRef *lhs, LLVMValueRef *rhs, LLVMBuilderRef builder, t_logger *logger) {
    (void) gen_llvm_cast_to_fp_if_needed(lhs, rhs, builder, logger);
    switch (op) {
    case BINOP_LESSER:
        return LLVMRealOLE;
    case BINOP_GREATER:
        return LLVMRealOGT;
    case BINOP_EQUALS:
        return LLVMRealOEQ;
    case BINOP_NEQ:
        return LLVMRealONE;
    case BINOP_LEQ:
        return LLVMRealOLE;
    case BINOP_GEQ:
        return LLVMRealOGE;
    default:
    {
        (void) LOGGER_log(logger, L_ERROR, "Op %d is not a real comparison operator.\n", op);
        (void) exit(LUKA_CODEGEN_ERROR);
    }
    }
}

LLVMValueRef gen_get_address(t_ast_node *node, LLVMBuilderRef builder, t_logger *logger)
{
    switch (node->type)
    {
        case AST_TYPE_VARIABLE:
        {
            t_named_value *val = NULL;

            HASH_FIND_STR(named_values, node->variable.name, val);

            if (NULL != val)
            {
                return val->alloca_inst;
            }

            (void) LOGGER_log(logger, L_ERROR, "Variable %s is undefined.\n", node->variable.name);
            (void) exit(LUKA_CODEGEN_ERROR);
        }
        case AST_TYPE_GET_EXPR:
        {
            t_named_value *variable = NULL;

            if (NULL == node->get_expr.variable)
            {
                (void) LOGGER_log(logger, L_WARNING, "Get expr variable name is null.\n");
                (void) exit(LUKA_CODEGEN_ERROR);
            }

            HASH_FIND_STR(named_values, node->get_expr.variable, variable);
            if (NULL == variable)
            {
                (void) LOGGER_log(logger, L_ERROR, "Couldn't find a variable named `%s`.\n", node->get_expr.variable);
                (void) exit(LUKA_CODEGEN_ERROR);
            }

            return gen_get_struct_field_pointer(variable, node->get_expr.key, builder, logger);
        }
        default:
        {
            (void) LOGGER_log(logger, L_ERROR, "Can't get address of %d.\n", node->type);
            (void) exit(LUKA_CODEGEN_ERROR);
        }
    }
}

LLVMValueRef gen_codegen_unexpr(t_ast_node *n,
                                LLVMModuleRef module,
                                LLVMBuilderRef builder,
                                t_logger *logger)
{
    LLVMValueRef rhs = NULL;
    if (NULL != n->unary_expr.rhs)
    {
        rhs = GEN_codegen(n->unary_expr.rhs, module, builder, logger);
    }

    if (NULL == rhs)
    {
        (void) LOGGER_log(logger, L_ERROR, "Couldn't codegen rhs for unary expression.\n");
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    switch (n->unary_expr.operator)
    {
        case UNOP_NOT:
        {
            return LLVMBuildNot(builder, rhs, "nottmp");
        }
        case UNOP_MINUS:
        {
            if (TYPE_is_floating_type(gen_llvm_type_to_ttype(LLVMTypeOf(rhs), logger)))
            {
                return LLVMBuildFNeg(builder, rhs, "negtmp");
            }

            return LLVMBuildNeg(builder, rhs, "negtmp");
        }
        case UNOP_REF:
        {
            return gen_get_address(n->unary_expr.rhs, builder, logger);
        }
        case UNOP_DEREF:
        {
            return LLVMBuildLoad(builder, gen_get_address(n->unary_expr.rhs, builder, logger), "loadtmp");
        }
        default:
        {
            (void) LOGGER_log(logger, L_ERROR, "Currently not supporting %d operator in unary expression.\n", n->unary_expr.operator);
            (void) exit(LUKA_CODEGEN_ERROR);
        }

    }
}

LLVMValueRef gen_codegen_binexpr(t_ast_node *n,
                                 LLVMModuleRef module,
                                 LLVMBuilderRef builder,
                                 t_logger *logger)
{
    LLVMValueRef lhs = NULL, rhs = NULL;
    LLVMOpcode opcode = LLVMAdd;
    LLVMIntPredicate int_predicate = LLVMIntEQ;
    LLVMRealPredicate real_predicate = LLVMRealOEQ;

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

    if (gen_is_cond_op(n->binary_expr.operator))
    {
        if (gen_is_icmp(lhs, rhs, logger))
        {
            int_predicate = gen_llvm_get_int_predicate(n->binary_expr.operator, &lhs, &rhs, builder, logger);
            return LLVMBuildICmp(builder, int_predicate, lhs, rhs, "icmptmp");
        }
        real_predicate = gen_llvm_get_real_predicate(n->binary_expr.operator, &lhs, &rhs, builder, logger);
        return LLVMBuildFCmp(builder, real_predicate, lhs, rhs, "fcmptmp");
    }

    opcode = gen_get_llvm_opcode(n->binary_expr.operator, &lhs, &rhs, builder, logger);
    return LLVMBuildBinOp(builder, opcode, lhs, rhs, "binoptmp");
}

LLVMValueRef gen_codegen_prototype(t_ast_node *n,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef UNUSED(builder),
                                   t_logger *logger)
{
    LLVMValueRef func = NULL;
    LLVMTypeRef func_type = NULL;
    size_t i = 0, arity = n->prototype.arity;
    bool vararg = n->prototype.vararg;
    LLVMTypeRef *params = NULL;
    if (vararg)
    {
        --arity;
    }
    params = calloc(arity, sizeof(LLVMTypeRef));
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
                                 params, arity, vararg);

    func = LLVMAddFunction(module, n->prototype.name, func_type);
    (void) LLVMSetLinkage(func, LLVMExternalLinkage);

    for (i = 0; i < arity; ++i)
    {
        LLVMValueRef param = LLVMGetParam(func, i);
        (void) LLVMSetValueName(param, n->prototype.args[i]);

        t_named_value *val = malloc(sizeof(t_named_value));
        val->name = strdup(n->prototype.args[i]);
        val->alloca_inst = NULL;
        val->type = params[i];
        val->ttype = n->prototype.types[i];
        HASH_ADD_KEYPTR(hh, named_values, val->name, strlen(val->name), val);
    }

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
    LLVMValueRef func = NULL, ret_val = NULL;
    LLVMBasicBlockRef block = NULL;
    t_ast_node *proto = NULL;
    bool has_return_stmt = false;
    t_type *return_type;
    size_t i = 0, arity = 0;
    t_named_value *val = NULL;
    char **args = NULL;

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

    proto = n->function.prototype;
    arity = proto->prototype.arity;
    args = proto->prototype.args;

    block = LLVMAppendBasicBlock(func, "entry");
    (void) LLVMPositionBuilderAtEnd(builder, block);

    for (i = 0; i < arity; ++i)
    {
        HASH_FIND_STR(named_values, args[i], val);
        if (NULL != val)
        {
            val->alloca_inst = gen_create_entry_block_allca(func, val->type, val->name);
            LLVMBuildStore(builder, LLVMGetParam(func, i), val->alloca_inst);
        }
        else
        {
            (void) LOGGER_log(logger, L_ERROR, "There is not named value with name %s\n");
            (void) exit(LUKA_CODEGEN_ERROR);
        }

    }

    ret_val = gen_codegen_stmts(n->function.body, module, builder, &has_return_stmt, logger);
    return_type = n->function.prototype->prototype.return_type;

    if (false == has_return_stmt)
    {
        switch (return_type->type)
        {
        case TYPE_VOID:
            ret_val = NULL;
            break;
        case TYPE_F32:
        case TYPE_F64:
            ret_val = LLVMConstReal(gen_type_to_llvm_type(return_type, logger), 0.0);
            break;
        case TYPE_ANY:
        case TYPE_STRING:
            ret_val = LLVMConstPointerNull(gen_type_to_llvm_type(return_type, logger));
            break;
        case TYPE_SINT8:
        case TYPE_SINT16:
        case TYPE_SINT32:
        case TYPE_SINT64:
            ret_val = LLVMConstInt(gen_type_to_llvm_type(return_type, logger), 0, true);
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
                 func = NULL, incoming_values[2] = {NULL, NULL};
    LLVMBasicBlockRef cond_block = NULL, then_block = NULL, else_block = NULL,
                      merge_block = NULL;

    func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

    cond_block = LLVMAppendBasicBlock(func, "if_cond");
    then_block = LLVMCreateBasicBlockInContext(LLVMGetGlobalContext(), "then");
    if(NULL != n->if_expr.else_body)
    {
        else_block = LLVMCreateBasicBlockInContext(LLVMGetGlobalContext(), "else");
    }
    merge_block = LLVMCreateBasicBlockInContext(LLVMGetGlobalContext(), "if_merge");

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

    (void) LLVMAppendExistingBasicBlock(func, then_block);
    (void) LLVMPositionBuilderAtEnd(builder, then_block);

    then_value = gen_codegen_stmts(n->if_expr.then_body, module, builder, NULL, logger);

    (void) LLVMBuildBr(builder, merge_block);

    then_block = LLVMGetInsertBlock(builder);

    if (NULL != n->if_expr.else_body)
    {
        (void) LLVMAppendExistingBasicBlock(func, else_block);
        (void) LLVMPositionBuilderAtEnd(builder, else_block);
        else_value = gen_codegen_stmts(n->if_expr.else_body, module, builder, NULL, logger);
        (void) LLVMBuildBr(builder, merge_block);
    }

    else_block = LLVMGetInsertBlock(builder);

    (void) LLVMAppendExistingBasicBlock(func, merge_block);
    (void) LLVMPositionBuilderAtEnd(builder, merge_block);

    if ((NULL == then_value) && !((NULL != n->if_expr.else_body) && (NULL != else_value)))
    {
        return NULL;
    }

    if (((NULL == then_value) && (NULL != else_value)) || ((NULL != then_value) && (NULL == else_value)))
    {
        (void) LOGGER_log(logger, L_ERROR, "If one branch returns a values, both must return a value.\n");
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    if ((NULL != n->if_expr.else_body) && (LLVMTypeOf(then_value) != LLVMTypeOf(else_value)))
    {
        (void) LOGGER_log(logger, L_ERROR, "Values of then and else branches must be of the same type in if expr.\n");
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    phi = LLVMBuildPhi(builder, LLVMTypeOf(then_value), "phi");

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
    end_block = LLVMCreateBasicBlockInContext(LLVMGetGlobalContext(), "while_end");

    if (NULL != loop_blocks)
    {
        (void) vector_push_front(loop_blocks, &end_block);
    }

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

    if (NULL != loop_blocks)
    {
        (void) vector_pop_front(loop_blocks);
    }

    (void) LLVMAppendExistingBasicBlock(func, end_block);
    (void) LLVMPositionBuilderAtEnd(builder, end_block);

    return body_value;
}

LLVMValueRef gen_codegen_cast_expr(t_ast_node *node,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder,
                                   t_logger *logger)
{

    LLVMValueRef expr = NULL;
    LLVMTypeRef dest_type = NULL;

    expr = GEN_codegen(node->cast_expr.expr, module, builder, logger);
    dest_type = gen_type_to_llvm_type(node->cast_expr.type, logger);
    return gen_codegen_cast(builder, expr, dest_type, logger);
}

LLVMValueRef gen_codegen_variable(t_ast_node *node,
                                  LLVMModuleRef UNUSED(module),
                                  LLVMBuilderRef builder,
                                  t_logger *logger)
{
    t_named_value *val = NULL;

    HASH_FIND_STR(named_values, node->variable.name, val);

    if (NULL != val)
    {
        return LLVMBuildLoad2(builder, val->type, val->alloca_inst, val->name);
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
    if (NULL == variable.type)
    {
        val->type = LLVMTypeOf(expr);
        val->ttype = gen_llvm_type_to_ttype(val->type, logger);
    }
    else
    {
        val->ttype = variable.type;
        val->type = gen_type_to_llvm_type(val->ttype, logger);
    }
    val->alloca_inst = gen_create_entry_block_allca(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), val->type, val->name);
    if (LLVMTypeOf(expr) != val->type)
    {
        expr = gen_codegen_cast(builder, expr, val->type, logger);
    }
    LLVMSetAlignment(LLVMBuildStore(builder, expr, val->alloca_inst), LLVMGetAlignment(val->alloca_inst) ? LLVMGetAlignment(val->alloca_inst) : 8);
    val->mutable = variable.mutable;
    HASH_ADD_KEYPTR(hh, named_values, val->name, strlen(val->name), val);

    return NULL;
}

LLVMValueRef gen_codegen_assignment_expr(t_ast_node *node,
                                         LLVMModuleRef module,
                                         LLVMBuilderRef builder,
                                         t_logger *logger)
{
    LLVMValueRef lhs = NULL, rhs = NULL;
    t_ast_node *variable = NULL;
    t_named_value *val = NULL;

    if (NULL != node->assignment_expr.lhs)
    {
        if (AST_TYPE_VARIABLE == node->assignment_expr.lhs->type)
        {
            variable = node->assignment_expr.lhs;
            HASH_FIND_STR(named_values, variable->variable.name, val);
            if (NULL == val)
            {
                (void) LOGGER_log(logger, L_ERROR, "Cannot assign to undeclared variable '%s'.\n", val->name);
                (void) exit(LUKA_CODEGEN_ERROR);
            }

            if (!val->mutable)
            {
                (void) LOGGER_log(logger, L_ERROR, "Trying to assign to immutable variable '%s'.\n", val->name);
                (void) exit(LUKA_CODEGEN_ERROR);
            }

            lhs = gen_get_address(node->assignment_expr.lhs, builder, logger);
        }
        else if (AST_TYPE_GET_EXPR == node->assignment_expr.lhs->type)
        {

            variable = node->assignment_expr.lhs;
            HASH_FIND_STR(named_values, variable->get_expr.variable, val);
            if (NULL == val)
            {
                (void) LOGGER_log(logger, L_ERROR, "Cannot assign to undeclared variable '%s'.\n", val->name);
                (void) exit(LUKA_CODEGEN_ERROR);
            }

            if (!val->mutable)
            {
                (void) LOGGER_log(logger, L_ERROR, "Trying to assign to immutable variable '%s'.\n", val->name);
                (void) exit(LUKA_CODEGEN_ERROR);
            }

            lhs = gen_get_address(node->assignment_expr.lhs, builder, logger);
        }
        else
        {
            lhs = GEN_codegen(node->assignment_expr.lhs, module, builder, logger);
        }
    }

    if (NULL != node->assignment_expr.rhs)
    {
        rhs = GEN_codegen(node->assignment_expr.rhs, module, builder, logger);
    }

    if ((NULL == lhs ) || (NULL == rhs))
    {
        (void) LOGGER_log(logger, L_ERROR, "Expression generation in assignment expr failed.\n");
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    (void) LLVMBuildStore(builder, rhs, lhs);
    return rhs;
}

LLVMValueRef gen_codegen_call(t_ast_node *node,
                              LLVMModuleRef module,
                              LLVMBuilderRef builder,
                              t_logger *logger)
{
    LLVMValueRef call = NULL;
    LLVMValueRef func = NULL;
    LLVMValueRef *args = NULL;
    LLVMValueRef *varargs = NULL;
    LLVMTypeRef func_type = NULL, type = NULL, dest_type = NULL;
    t_ast_node *arg = NULL;
    size_t i = 0;
    bool vararg = NULL;
    size_t required_params_count = 0;

    func = LLVMGetNamedFunction(module, node->call_expr.name);
    if (NULL == func)
    {
        (void) LOGGER_log(logger, L_WARNING, "Couldn't find a function named `%s`, are you sure you defined it or wrote a proper extern line for it?\n", node->call_expr.name);
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    func_type = LLVMGetElementType(LLVMTypeOf(func));
    vararg = LLVMIsFunctionVarArg(func_type);
    required_params_count = LLVMCountParams(func);

    if (!vararg && node->call_expr.args->size != required_params_count)
    {
        goto cleanup;
    }

    if (vararg && node->call_expr.args->size < required_params_count)
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

        if (i < required_params_count)
        {
            type = LLVMTypeOf(args[i]);
            dest_type = LLVMTypeOf(LLVMGetParam(func, i));
            if (type != dest_type)
            {
                args[i] = gen_codegen_cast(builder, args[i], dest_type, logger);
            }
        }
        ++i;
    }

    call = LLVMBuildCall2(builder,
                          func_type,
                          func,
                          args,
                          node->call_expr.args->size,
                          LLVMGetReturnType(func_type) != LLVMVoidType() ? "calltmp" : "");

cleanup:
    if (NULL != varargs)
    {
        (void) free(varargs);
        varargs = NULL;
    }

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
    if (NULL != n->expression_stmt.expr)
    {
        (void) GEN_codegen(n->expression_stmt.expr, module, builder, logger);

    }

    return NULL;
}

LLVMValueRef gen_codegen_break_stmt(t_ast_node *UNUSED(n),
                                    LLVMModuleRef UNUSED(module),
                                    LLVMBuilderRef builder,
                                    t_logger *logger)
{
    LLVMBasicBlockRef dest_block = NULL;

    if ((NULL == loop_blocks) || (0 == loop_blocks->size))
    {
        (void) LOGGER_log(logger, L_WARNING, "Cannot break when not inside a loop.\n");
        return NULL;
    }

    dest_block = VECTOR_GET_AS(LLVMBasicBlockRef, loop_blocks, 0);

    (void) LLVMBuildBr(builder, dest_block);
    // TODO: Find if there's a better way to supress "Terminator found in the middle of a basic block"
    dest_block = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "unused_block");
    (void) LLVMPositionBuilderAtEnd(builder, dest_block);
    return NULL;
}

LLVMValueRef gen_codegen_number(t_ast_node *node, t_logger *logger)
{
    LLVMTypeRef type = gen_type_to_llvm_type(node->number.type, logger);
    switch (node->number.type->type)
    {
        case TYPE_F32:
            return LLVMConstReal(type, node->number.value.f32);
        case TYPE_F64:
            return LLVMConstReal(type, node->number.value.f64);
        case TYPE_SINT8:
            return LLVMConstInt(type, node->number.value.s8, true);
        case TYPE_SINT16:
            return LLVMConstInt(type, node->number.value.s16, true);
        case TYPE_SINT32:
            return LLVMConstInt(type, node->number.value.s32, true);
        case TYPE_SINT64:
            return LLVMConstInt(type, node->number.value.s64, true);
        case TYPE_UINT8:
            return LLVMConstInt(type, node->number.value.u8, false);
        case TYPE_UINT16:
            return LLVMConstInt(type, node->number.value.u16, false);
        case TYPE_UINT32:
            return LLVMConstInt(type, node->number.value.u32, false);
        case TYPE_UINT64:
            return LLVMConstInt(type, node->number.value.u64, false);
        default:
            (void) fprintf(stderr, "%d is not a number type.\n", node->number.type->type);
            (void) exit(LUKA_GENERAL_ERROR);
    }
}

LLVMValueRef gen_codegen_struct_definition(t_ast_node *node,
                                           LLVMModuleRef UNUSED(module),
                                           LLVMBuilderRef UNUSED(builder),
                                           t_logger *logger)
{
    size_t elements_count = node->struct_definition.struct_fields->size;
    LLVMTypeRef struct_type = NULL;
    LLVMTypeRef *element_types = NULL;
    t_struct_info *struct_info = NULL;
    bool error = true;

    element_types = calloc(elements_count, sizeof(LLVMTypeRef));
    if (NULL == element_types)
    {
        goto cleanup;
    }

    struct_info = calloc(1, sizeof(t_struct_info));
    if (NULL == struct_info)
    {
        goto cleanup;
    }

    struct_info->struct_name = strdup(node->struct_definition.name);
    struct_info->number_of_fields = elements_count;
    struct_info->struct_fields = calloc(elements_count, sizeof(char**));
    if (NULL == struct_info->struct_fields)
    {
        goto cleanup;
    }

    for (size_t i = 0; i < elements_count; ++i)
    {
        struct_info->struct_fields[i] = NULL;
    }

    struct_type = LLVMStructCreateNamed(LLVMGetGlobalContext(), node->struct_definition.name);

    for (size_t i = 0; i < elements_count; ++i)
    {
        element_types[i] = gen_type_to_llvm_type((VECTOR_GET_AS(t_struct_field_ptr, node->struct_definition.struct_fields, i))->type, logger);
        struct_info->struct_fields[i] = strdup((VECTOR_GET_AS(t_struct_field_ptr, node->struct_definition.struct_fields, i))->name);
    }

    LLVMStructSetBody(struct_type, element_types, elements_count, false);

    struct_info->struct_type = struct_type;
    HASH_ADD_KEYPTR(hh, struct_infos, struct_info->struct_name, strlen(struct_info->struct_name), struct_info);

    error = false;

cleanup:
    if (NULL != element_types)
    {
        (void) free(element_types);
        element_types = NULL;
    }

    if (!error)
    {
        return NULL;
    }

    if (NULL != struct_info)
    {
        if (NULL != struct_info->struct_fields)
        {
            (void) free(struct_info->struct_fields);
            struct_info->struct_fields = NULL;
        }

        (void) free(struct_info);
        struct_info = NULL;
    }

    return NULL;
}

LLVMValueRef gen_codegen_struct_value(t_ast_node *node,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder,
                                      t_logger *logger)
{
    size_t elements_count = node->struct_value.struct_values->size;
    LLVMValueRef struct_value = NULL, *element_values = calloc(elements_count, sizeof(LLVMValueRef));
    if (NULL == element_values)
    {
        return NULL;
    }

    for (size_t i = 0; i < elements_count; ++i)
    {
        element_values[i] = GEN_codegen((VECTOR_GET_AS(t_struct_value_field_ptr, node->struct_value.struct_values, i))->expr, module, builder, logger);
    }

    struct_value = LLVMConstStruct(element_values, elements_count, false);

    (void) free(element_values);

    return struct_value;
}


LLVMValueRef gen_get_struct_field_pointer(t_named_value *variable,
                                          char *key,
                                          LLVMBuilderRef builder,
                                          t_logger *logger)
{
    LLVMValueRef indices[2] = { LLVMConstInt(LLVMInt32Type(), 0, 0), NULL };
    t_struct_info *struct_info = NULL;

    HASH_FIND_STR(struct_infos, (char*)variable->ttype->payload, struct_info);

    if (NULL == struct_info)
    {
        (void) LOGGER_log(logger, L_ERROR, "Couldn't find struct info.\n");
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    for (size_t i = 0; i < struct_info->number_of_fields; ++i)
    {
        if ((NULL != struct_info->struct_fields) && (0 == strcmp(key, struct_info->struct_fields[i])))
        {
            indices[1] = LLVMConstInt(LLVMInt32Type(), i, 0);
        }
    }

    if (NULL == indices[1])
    {
        (void) LOGGER_log(logger, L_ERROR, "`%s` is not a field in struct `%s`.\n", key, struct_info->struct_name);
        (void) exit(LUKA_CODEGEN_ERROR);
    }

    return LLVMBuildGEP(builder, variable->alloca_inst, indices, 2, "geptmp");
}

LLVMValueRef gen_codegen_get_expr(t_ast_node *node,
                                  LLVMModuleRef UNUSED(module),
                                  LLVMBuilderRef builder,
                                  t_logger *logger)
{
    LLVMValueRef field_pointer = NULL;

    field_pointer = gen_get_address(node, builder, logger);
    return LLVMBuildLoad2(builder, LLVMGetElementType(LLVMTypeOf(field_pointer)), field_pointer, "loadtmp"); // LLVMBuildStructGEP(builder, field_pointer, 0, "getexprtmp");
}

LLVMValueRef GEN_codegen(t_ast_node *node,
                         LLVMModuleRef module,
                         LLVMBuilderRef builder,
                         t_logger *logger)
{
    switch (node->type)
    {
    case AST_TYPE_NUMBER:
        return gen_codegen_number(node, logger);
    case AST_TYPE_STRING:
        return LLVMBuildGlobalStringPtr(builder, node->string.value, "str");
    case AST_TYPE_UNARY_EXPR:
        return gen_codegen_unexpr(node, module, builder, logger);
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
    case AST_TYPE_CAST_EXPR:
        return gen_codegen_cast_expr(node, module, builder, logger);
    case AST_TYPE_VARIABLE:
        return gen_codegen_variable(node, module, builder, logger);
    case AST_TYPE_LET_STMT:
        return gen_codegen_let_stmt(node, module, builder, logger);
    case AST_TYPE_ASSIGNMENT_EXPR:
        return gen_codegen_assignment_expr(node, module, builder, logger);
    case AST_TYPE_CALL_EXPR:
        return gen_codegen_call(node, module, builder, logger);
    case AST_TYPE_EXPRESSION_STMT:
        return gen_codegen_expression_stmt(node, module, builder, logger);
    case AST_TYPE_BREAK_STMT:
        return gen_codegen_break_stmt(node, module, builder, logger);
    case AST_TYPE_STRUCT_DEFINITION:
        return gen_codegen_struct_definition(node, module, builder, logger);
    case AST_TYPE_STRUCT_VALUE:
        return gen_codegen_struct_value(node, module, builder, logger);
    case AST_TYPE_GET_EXPR:
        return gen_codegen_get_expr(node, module, builder, logger);
    default:
    {
        (void) LOGGER_log(logger, L_ERROR, "No codegen function was found for type - %d\n", node->type);
        (void) exit(LUKA_CODEGEN_ERROR);
    }
    }
}

void GEN_codegen_initialize()
{
    loop_blocks = calloc(1, sizeof(t_vector));
    (void) vector_setup(loop_blocks, 6 , sizeof(LLVMBasicBlockRef));
}

void GEN_codegen_reset()
{
    t_named_value *named_value = NULL, *named_value_iter = NULL;
    t_struct_info *struct_info = NULL, *struct_info_iter = NULL;

    HASH_ITER(hh, named_values, named_value, named_value_iter) {
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

    HASH_ITER(hh, struct_infos, struct_info, struct_info_iter) {
        HASH_DEL(struct_infos, struct_info);
        if (NULL != struct_info)
        {
            if (NULL != struct_info->struct_name)
            {
                (void) free((char *) struct_info->struct_name);
                struct_info->struct_name = NULL;
            }

            if (NULL != struct_info->struct_fields)
            {
                for (size_t i = 0; i < struct_info->number_of_fields; ++i)
                {
                    if (NULL != struct_info->struct_fields[i])
                    {
                        (void) free(struct_info->struct_fields[i]);
                        struct_info->struct_fields[i] = NULL;
                    }
                }
                (void) free(struct_info->struct_fields);
                struct_info->struct_fields = NULL;
            }
            (void) free(struct_info);
            struct_info = NULL;
        }
    }

    if (NULL != loop_blocks)
    {
        (void) vector_clear(loop_blocks);
        (void) vector_destroy(loop_blocks);
        (void) free(loop_blocks);
    }
}
