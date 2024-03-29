/** @file gen.c */
#include "gen.h"

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "core.h"
#include "defs.h"
#include "logger.h"
#include "type.h"
#include "uthash.h"
#include "utils.h"
#include "vector.h"

static t_named_value *named_values = NULL;
static t_struct_info *struct_infos = NULL;
static t_enum_info *enum_infos = NULL;
static t_vector *loop_blocks = NULL;
static t_vector *defer_blocks = NULL;

static LLVMValueRef gen_codegen_sizeof(t_ast_node *node, t_type *type,
                                       t_logger *logger);

static LLVMValueRef gen_codegen_stmts(t_vector *statements,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder,
                                      bool *has_return_stmt, t_logger *logger);

/**
 * @brief Generate LLVM IR for all currently defined defer blocks.
 *
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 */
static void gen_codegen_defer_blocks(LLVMModuleRef module,
                                     LLVMBuilderRef builder, t_logger *logger)
{
    size_t i = 0;
    t_ast_node *node = NULL;

    if (NULL != defer_blocks)
    {
        for (i = 0; i < defer_blocks->size; ++i)
        {
            node = *(t_ast_node **) vector_get(defer_blocks, i);
            (void) gen_codegen_stmts(node->defer_stmt.body, module, builder,
                                     NULL, logger);
        }
    }
}

/**
 * @brief Converting a LLVM type to Luka type.
 *
 * @param[in] type the LLVM type.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the Luka type.
 */
static t_type *gen_llvm_type_to_ttype(LLVMTypeRef type, t_logger *logger)
{
    t_type *ttype = calloc(1, sizeof(t_type));
    if (NULL == type)
    {
        exit(LUKA_CANT_ALLOC_MEMORY);
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
        ttype->inner_type
            = gen_llvm_type_to_ttype(LLVMGetElementType(type), logger);
    }
    else if (LLVMStructTypeKind == LLVMGetTypeKind(type))
    {
        ttype->type = TYPE_STRUCT;
    }
    else if (LLVMArrayTypeKind == LLVMGetTypeKind(type))
    {
        ttype->type = TYPE_ARRAY;
        ttype->inner_type
            = gen_llvm_type_to_ttype(LLVMGetElementType(type), logger);
        ttype->payload = (void *) ((size_t) LLVMGetArrayLength(type));
    }
    else
    {
        (void) LLVMDumpType(type);
        (void) LOGGER_log(
            logger, L_ERROR,
            "\nI don't know how to translate LLVM type %d to t_type.\n", type);
        exit(LUKA_CODEGEN_ERROR);
    }

    return ttype;
}

/**
 * @brief Getting a field out of a struct.
 *
 * @param[in] variable the named value.
 * @param[in] key the field to get.
 * @param[in] builder the LLVM IR builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return a GEP instruction to the struct field.
 */
static LLVMValueRef gen_get_struct_field_pointer(t_named_value *variable,
                                                 char *key,
                                                 LLVMBuilderRef builder,
                                                 t_logger *logger)
{
    unsigned int index = 0;
    bool found_field = false;
    t_struct_info *struct_info = NULL;
    t_type *type = NULL;
    LLVMValueRef var = NULL;
    bool should_deref = false;

    if (NULL == variable)
    {
        (void) LOGGER_log(logger, L_ERROR, "Struct variable is NULL.\n",
                          variable->name);
        exit(LUKA_CODEGEN_ERROR);
    }

    if (NULL == variable->ttype)
    {
        (void) LOGGER_log(logger, L_ERROR, "Struct %s ttype is NULL.\n",
                          variable->name);
        exit(LUKA_CODEGEN_ERROR);
    }

    type = variable->ttype;
    if (type->type == TYPE_PTR)
    {
        type = type->inner_type;
        if (NULL == type)
        {
            (void) LOGGER_log(logger, L_ERROR,
                              "Struct %s ttype after dereference is NULL.\n",
                              variable->name);
            exit(LUKA_CODEGEN_ERROR);
        }
        should_deref = true;
    }

    if (NULL == type->payload)
    {
        (void) LOGGER_log(logger, L_ERROR, "Struct %s ttype payload is NULL.\n",
                          variable->name);
        exit(LUKA_CODEGEN_ERROR);
    }

    HASH_FIND_STR(struct_infos, (char *) type->payload, struct_info);

    if (NULL == struct_info)
    {
        (void) LOGGER_log(logger, L_ERROR, "Couldn't find struct info.\n");
        exit(LUKA_CODEGEN_ERROR);
    }

    for (unsigned int i = 0; i < struct_info->number_of_fields && !found_field;
         ++i)
    {
        if ((NULL != struct_info->struct_fields)
            && (0 == strcmp(key, struct_info->struct_fields[i])))
        {
            index = i;
            found_field = true;
        }
    }

    if (!found_field)
    {
        (void) LOGGER_log(logger, L_ERROR,
                          "`%s` is not a field in struct `%s`.\n", key,
                          struct_info->struct_name);
        exit(LUKA_CODEGEN_ERROR);
    }

    var = variable->alloca_inst;
    if (should_deref)
    {
        var = LLVMBuildLoad(builder, var, "loadtmp");
    }

    return LLVMBuildStructGEP(builder, var, index, key);
}

/**
 * @brief Cast lhs and rhs to be the same size if they are of different types.
 *
 * @details The size of the smaller value is always extended to be as large as
 * the larger value size.
 *
 * @param[in,out] lhs the left hand side value.
 * @param[in,out] rhs the right hand side value.
 * @param[in] builder the LLVM IR builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return whether a cast has happend.
 */
static bool gen_llvm_cast_sizes_if_needed(LLVMValueRef *lhs, LLVMValueRef *rhs,
                                          LLVMBuilderRef builder,
                                          t_logger *logger)
{
    t_type *lhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(*lhs), logger);
    t_type *rhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(*rhs), logger);

    if (lhs_t->type != rhs_t->type)
    {
        if (TYPE_is_floating_type(lhs_t))
        {
            if (TYPE_F32 == lhs_t->type)
            {
                *lhs = LLVMBuildFPExt(builder, *lhs, LLVMTypeOf(*rhs),
                                      "fpexttmp");
            }
            else
            {
                *rhs = LLVMBuildFPExt(builder, *rhs, LLVMTypeOf(*lhs),
                                      "fpexttmp");
            }
        }
        else if (LLVMGetIntTypeWidth(LLVMTypeOf(*lhs))
                 < LLVMGetIntTypeWidth(LLVMTypeOf(*rhs)))
        {
            *lhs = LLVMBuildIntCast2(builder, *lhs, LLVMTypeOf(*rhs),
                                     TYPE_is_signed(rhs_t), "intcasttmp");
        }
        else
        {
            *rhs = LLVMBuildIntCast2(builder, *rhs, LLVMTypeOf(*lhs),
                                     TYPE_is_signed(lhs_t), "intcasttmp");
        }

        (void) TYPE_free_type(lhs_t);
        lhs_t = NULL;
        (void) TYPE_free_type(rhs_t);
        rhs_t = NULL;

        return true;
    }

    (void) TYPE_free_type(lhs_t);
    lhs_t = NULL;
    (void) TYPE_free_type(rhs_t);
    rhs_t = NULL;
    return false;
}

/**
 * @brief Clearing currently defined named values.
 */
static void gen_named_values_clear(void)
{
    t_named_value *named_value = NULL, *named_value_iter = NULL;

    HASH_ITER(hh, named_values, named_value, named_value_iter)
    {
        HASH_DEL(named_values, named_value);
        if (NULL != named_value)
        {
            if (NULL != named_value->name)
            {
                (void) free(named_value->name);
                named_value->name = NULL;
            }

            if (NULL != named_value->ttype)
            {
                (void) TYPE_free_type(named_value->ttype);
                named_value->ttype = NULL;
            }

            (void) free(named_value);
            named_value = NULL;
        }
    }
}

/**
 * @brief Convert Luka type to LLVM type.
 *
 * @param[in] type the Luka type to convert.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the LLVM type.
 */
static LLVMTypeRef gen_type_to_llvm_type(t_type *type, t_logger *logger)
{
    t_struct_info *struct_info = NULL;

    switch (type->type)
    {
        case TYPE_ANY:
        case TYPE_TYPE:
            return LLVMInt8Type();
        case TYPE_BOOL:
            return LLVMInt1Type();
        case TYPE_SINT8:
            return LLVMInt8Type();
        case TYPE_SINT16:
            return LLVMInt16Type();
        case TYPE_ENUM:
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
            return LLVMPointerType(
                gen_type_to_llvm_type(type->inner_type, logger), 0);
        case TYPE_ARRAY:
            if (NULL != type->payload)
            {
                return LLVMArrayType(
                    gen_type_to_llvm_type(type->inner_type, logger),
                    (unsigned int) (size_t) type->payload);
            }
            return LLVMPointerType(
                gen_type_to_llvm_type(type->inner_type, logger), 0);
        case TYPE_STRUCT:
            HASH_FIND_STR(struct_infos, (char *) type->payload, struct_info);
            if ((NULL != struct_info) && (NULL != struct_info->struct_type))
            {
                return struct_info->struct_type;
            }

            (void) LOGGER_log(
                logger, L_ERROR,
                "gen_type_to_llvm_type: I don't know how to translate struct "
                "named %s to LLVM types without a previous definition.\n",
                (char *) type->payload);
            exit(LUKA_CODEGEN_ERROR);

        case TYPE_ALIAS:
            (void) LOGGER_log(
                logger, L_ERROR,
                "Unresolved alias %s got to gen_type_to_llvm_type.\n",
                (char *) type->payload);
            exit(LUKA_CODEGEN_ERROR);
    }
}

/**
 * @brief Create an alloca in the entry block of a function for a new
 * named_value.
 *
 * @param[in] function the function to create the alloca inside.
 * @param[in] type the type that should be allocated for.
 * @param[in] var_name the name of the variable.
 *
 * @return an alloca instruction.
 */
static LLVMValueRef gen_create_entry_block_allca(LLVMValueRef function,
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
    if (inst != NULL)
    {
        LLVMPositionBuilderBefore(builder, inst);
    }
    else
    {
        LLVMPositionBuilderAtEnd(builder, entry_block);
    }
    alloca_inst = LLVMBuildAlloca(builder, type, var_name);
    LLVMDisposeBuilder(builder);
    return alloca_inst;
}

/**
 * @brief Get a LLVM cast operator that converts from `type` to `dest_type`.
 *
 * @param[in] type the type to convert from.
 * @param[in] dest_type the type to convert to.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the opcode that should be used to cast from `type` to `dest_type`.
 */
static LLVMOpcode gen_llvm_get_cast_op(LLVMTypeRef type, LLVMTypeRef dest_type,
                                       t_logger *logger)
{
    t_type *ttype = gen_llvm_type_to_ttype(type, logger),
           *dest_ttype = gen_llvm_type_to_ttype(dest_type, logger);
    LLVMTypeKind type_kind = LLVMGetTypeKind(type),
                 dtype_kind = LLVMGetTypeKind(dest_type);
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
        else if (TYPE_is_signed(dest_ttype))
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
        if ((LLVMDoubleTypeKind == type_kind)
            || (LLVMFloatTypeKind == type_kind))
        {
            if (TYPE_is_signed(ttype))
            {
                opcode = LLVMSIToFP;
            }

            opcode = LLVMUIToFP;
        }

        else if (LLVMGetIntTypeWidth(dest_type) > LLVMGetIntTypeWidth(type))
        {
            if (TYPE_is_signed(dest_ttype))
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
        (void) TYPE_free_type(ttype);
        ttype = NULL;
    }

    if (NULL != dest_ttype)
    {
        (void) free(dest_ttype);
        dest_ttype = NULL;
    }

    return opcode;
}

/**
 * @brief Cast a LLVM value to a new type.
 *
 * @param[in] builder the LLVM IR builder.
 * @param[in] original_value the value that should be casted.
 * @param[in] dest_type the type the value should be casted to.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the value casted to the `dest_type`.
 */
static LLVMValueRef gen_codegen_cast(LLVMBuilderRef builder,
                                     LLVMValueRef original_value,
                                     LLVMTypeRef dest_type, t_logger *logger)
{
    LLVMTypeRef type = LLVMTypeOf(original_value);

    if ((LLVMArrayTypeKind == LLVMGetTypeKind(type))
        && (LLVMPointerTypeKind == LLVMGetTypeKind(dest_type)))
    {
        return original_value;
    }

    if ((LLVMPointerTypeKind == LLVMGetTypeKind(type))
        && (LLVMPointerTypeKind == LLVMGetTypeKind(dest_type)))
    {
        return LLVMBuildPointerCast(builder, original_value, dest_type,
                                    "ptrcasttmp");
    }

    return LLVMBuildCast(builder, gen_llvm_get_cast_op(type, dest_type, logger),
                         original_value, dest_type, "casttmp");
}

/**
 * @brief Cast both hand sides to floating point if one of them is a floating
 * point type.
 *
 * @param[in,out] lhs the left hand side.
 * @param[in,out] rhs the right hand side.
 * @param[in] builder the LLVM IR builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return whether a cast has happend.
 */
static bool gen_llvm_cast_to_fp_if_needed(LLVMValueRef *lhs, LLVMValueRef *rhs,
                                          LLVMBuilderRef builder,
                                          t_logger *logger)
{
    t_type *lhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(*lhs), logger);
    t_type *rhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(*rhs), logger);

    if (TYPE_is_floating_type(lhs_t) || TYPE_is_floating_type(rhs_t))
    {
        if (TYPE_is_floating_type(lhs_t) && !TYPE_is_floating_type(rhs_t))
        {
            if (TYPE_is_signed(rhs_t))
            {
                *rhs = LLVMBuildSIToFP(builder, *rhs, LLVMTypeOf(*lhs),
                                       "sitofpcasttmp");
            }
            else
            {
                *rhs = LLVMBuildUIToFP(builder, *rhs, LLVMTypeOf(*lhs),
                                       "uitofpcasttmp");
            }
        }
        else if (!TYPE_is_floating_type(lhs_t) && TYPE_is_floating_type(rhs_t))
        {
            if (TYPE_is_signed(lhs_t))
            {
                *lhs = LLVMBuildSIToFP(builder, *lhs, LLVMTypeOf(*rhs),
                                       "sitofpcasttmp");
            }
            else
            {
                *lhs = LLVMBuildUIToFP(builder, *lhs, LLVMTypeOf(*rhs),
                                       "uitofpcasttmp");
            }
        }

        (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);

        (void) TYPE_free_type(lhs_t);
        lhs_t = NULL;
        (void) TYPE_free_type(rhs_t);
        rhs_t = NULL;

        return true;
    }

    (void) TYPE_free_type(lhs_t);
    lhs_t = NULL;
    (void) TYPE_free_type(rhs_t);
    rhs_t = NULL;
    return false;
}

/**
 * @brief Cast both hand sides to signed if one of them is a signed type.
 *
 * @param[in,out] lhs the left hand side.
 * @param[in,out] rhs the right hand side.
 * @param[in] builder the LLVM IR builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return whether a cast has happend.
 */
static bool gen_llvm_cast_to_signed_if_needed(LLVMValueRef *lhs,
                                              LLVMValueRef *rhs,
                                              LLVMBuilderRef builder,
                                              t_logger *logger)
{
    t_type *lhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(*lhs), logger);
    t_type *rhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(*rhs), logger);

    if (TYPE_is_signed(lhs_t) || TYPE_is_signed(rhs_t))
    {
        if (TYPE_is_signed(lhs_t) && !TYPE_is_signed(rhs_t))
        {
            *rhs = LLVMBuildIntCast2(builder, *rhs, LLVMTypeOf(*lhs), true,
                                     "signedcasttmp");
        }
        else if (!TYPE_is_signed(lhs_t) && TYPE_is_signed(rhs_t))
        {
            *lhs = LLVMBuildIntCast2(builder, *lhs, LLVMTypeOf(*rhs), true,
                                     "signedcasttmp");
        }

        (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);

        (void) TYPE_free_type(lhs_t);
        lhs_t = NULL;
        (void) TYPE_free_type(rhs_t);
        rhs_t = NULL;

        return true;
    }

    (void) TYPE_free_type(lhs_t);
    lhs_t = NULL;
    (void) TYPE_free_type(rhs_t);
    rhs_t = NULL;
    return false;
}

/**
 * @brief Cast a null to be a null of the other type if one of the hand sides is
 * null.
 *
 * @param[in,out] lhs the left hand side.
 * @param[in,out] rhs the right hand side.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return whether a cast has happend.
 */
static bool gen_llvm_cast_null_if_needed(LLVMValueRef *lhs, LLVMValueRef *rhs,
                                         t_logger *logger)
{
    t_type *lhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(*lhs), logger);
    t_type *rhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(*rhs), logger);
    bool lhs_null = LLVMIsAConstantPointerNull(*lhs);
    bool rhs_null = LLVMIsAConstantPointerNull(*rhs);

    if (lhs_null || rhs_null)
    {
        if (lhs_null && !rhs_null)
        {
            *lhs = LLVMConstPointerNull(LLVMTypeOf(*rhs));
        }
        else if (!lhs_null && rhs_null)
        {
            *rhs = LLVMConstPointerNull(LLVMTypeOf(*lhs));
        }
        else
        {
            *lhs = LLVMConstPointerNull(LLVMTypeOf(*rhs));
        }

        (void) TYPE_free_type(lhs_t);
        lhs_t = NULL;
        (void) TYPE_free_type(rhs_t);
        rhs_t = NULL;

        return true;
    }

    (void) TYPE_free_type(lhs_t);
    lhs_t = NULL;
    (void) TYPE_free_type(rhs_t);
    rhs_t = NULL;
    return false;
}

/**
 * @brief Get LLVM opcode for the binary operator `op` and cast `lhs` and `rhs`
 * if needed.
 *
 * @param[in] op the binary operator.
 * @param[in,out] lhs a pointer to the left operand of the binary expression.
 * @param[in,out] rhs a pointer to the right operand of the binary expression.
 * @param[in] builder the LLVM IR builder.
 * @param[in] logger a logger that can be used to log messages.
 */
static LLVMOpcode gen_get_llvm_opcode(t_ast_binop_type op, LLVMValueRef *lhs,
                                      LLVMValueRef *rhs, LLVMBuilderRef builder,
                                      t_logger *logger)
{
    switch (op)
    {
        case BINOP_ADD:
            if (gen_llvm_cast_to_fp_if_needed(lhs, rhs, builder, logger))
            {
                return LLVMFAdd;
            }

            (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
            return LLVMAdd;
        case BINOP_SUBTRACT:
            if (gen_llvm_cast_to_fp_if_needed(lhs, rhs, builder, logger))
            {
                return LLVMFSub;
            }

            (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
            return LLVMSub;
        case BINOP_MULTIPLY:
            if (gen_llvm_cast_to_fp_if_needed(lhs, rhs, builder, logger))
            {
                return LLVMFMul;
            }

            (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
            return LLVMMul;
        case BINOP_DIVIDE:
            if (gen_llvm_cast_to_fp_if_needed(lhs, rhs, builder, logger))
            {
                return LLVMFDiv;
            }

            if (gen_llvm_cast_to_signed_if_needed(lhs, rhs, builder, logger))
            {
                return LLVMSDiv;
            }

            (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
            return LLVMUDiv;
        case BINOP_MODULOS:
            if (gen_llvm_cast_to_fp_if_needed(lhs, rhs, builder, logger))
            {
                return LLVMFRem;
            }

            if (gen_llvm_cast_to_signed_if_needed(lhs, rhs, builder, logger))
            {
                return LLVMSRem;
            }

            (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
            return LLVMURem;
        case BINOP_BAND:
            (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
            return LLVMAnd;
        case BINOP_BOR:
            (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
            return LLVMOr;
        case BINOP_BXOR:
            (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
            return LLVMXor;
        case BINOP_SHL:
            (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
            return LLVMShl;
        case BINOP_SHR:
            if (gen_llvm_cast_to_signed_if_needed(lhs, rhs, builder, logger))
            {
                return LLVMAShr;
            }

            (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
            return LLVMLShr;
        case BINOP_EQUALS:
        case BINOP_GEQ:
        case BINOP_GREATER:
        case BINOP_LEQ:
        case BINOP_LESSER:
        case BINOP_NEQ:
            {
                (void) LOGGER_log(logger, L_ERROR,
                                  "No handler found for op: %d\n", op);
                exit(LUKA_CODEGEN_ERROR);
            }
    }
}

/**
 * @brief Checks if the comparison between `lhs` and `rhs` will be an integer
 * comparison.
 *
 * @param[in] lhs the left operand of the comparison.
 * @param[in] rhs the right operand of the comparison.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return whether the comparison will be an integer comparison.
 */
static bool gen_is_icmp(LLVMValueRef lhs, LLVMValueRef rhs, t_logger *logger)
{
    t_type *lhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(lhs), logger);
    t_type *rhs_t = gen_llvm_type_to_ttype(LLVMTypeOf(rhs), logger);
    bool is_icmp
        = !TYPE_is_floating_type(lhs_t) && !TYPE_is_floating_type(rhs_t);

    (void) TYPE_free_type(lhs_t);
    lhs_t = NULL;
    (void) TYPE_free_type(rhs_t);
    rhs_t = NULL;

    return is_icmp;
}

/**
 * @brief Get a LLVM integer comparison opcode for `op`.
 *
 * @param[in] op the binary operator.
 * @param[in,out] lhs the left operand of the comparison.
 * @param[in,out] rhs the right operand of the comparison.
 * @param[in] builder the LLVM IR builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return an integer predicate for the comparison.
 */
static LLVMIntPredicate gen_llvm_get_int_predicate(t_ast_binop_type op,
                                                   LLVMValueRef *lhs,
                                                   LLVMValueRef *rhs,
                                                   LLVMBuilderRef builder,
                                                   t_logger *logger)
{
    switch (op)
    {
        case BINOP_LESSER:
            if (gen_llvm_cast_to_signed_if_needed(lhs, rhs, builder, logger))
            {
                (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
                return LLVMIntSLT;
            }

            (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
            return LLVMIntULT;
        case BINOP_GREATER:
            if (gen_llvm_cast_to_signed_if_needed(lhs, rhs, builder, logger))
            {
                (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
                return LLVMIntSGT;
            }

            (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
            return LLVMIntUGT;
        case BINOP_EQUALS:
            (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
            return LLVMIntEQ;
        case BINOP_NEQ:
            (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
            return LLVMIntNE;
        case BINOP_LEQ:
            if (gen_llvm_cast_to_signed_if_needed(lhs, rhs, builder, logger))
            {
                (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
                return LLVMIntSLE;
            }

            (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
            return LLVMIntULE;
        case BINOP_GEQ:
            if (gen_llvm_cast_to_signed_if_needed(lhs, rhs, builder, logger))
            {
                (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
                return LLVMIntSGE;
            }

            (void) gen_llvm_cast_sizes_if_needed(lhs, rhs, builder, logger);
            return LLVMIntUGE;
        case BINOP_ADD:
        case BINOP_DIVIDE:
        case BINOP_MODULOS:
        case BINOP_MULTIPLY:
        case BINOP_SUBTRACT:
        case BINOP_BAND:
        case BINOP_BOR:
        case BINOP_BXOR:
        case BINOP_SHL:
        case BINOP_SHR:
            {
                (void) LOGGER_log(logger, L_ERROR,
                                  "Op %d is not a int comparison operator.\n",
                                  op);
                exit(LUKA_CODEGEN_ERROR);
            }
    }
}

/**
 * @brief Get a LLVM real comparison opcode for `op`.
 *
 * @param[in] op the binary operator.
 * @param[in,out] lhs the left operand of the comparison.
 * @param[in,out] rhs the right operand of the comparison.
 * @param[in] builder the LLVM IR builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return a real predicate for the comparison.
 */
static LLVMRealPredicate gen_llvm_get_real_predicate(t_ast_binop_type op,
                                                     LLVMValueRef *lhs,
                                                     LLVMValueRef *rhs,
                                                     LLVMBuilderRef builder,
                                                     t_logger *logger)
{
    (void) gen_llvm_cast_to_fp_if_needed(lhs, rhs, builder, logger);
    switch (op)
    {
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
        case BINOP_ADD:
        case BINOP_DIVIDE:
        case BINOP_MODULOS:
        case BINOP_MULTIPLY:
        case BINOP_SUBTRACT:
        case BINOP_BAND:
        case BINOP_BOR:
        case BINOP_BXOR:
        case BINOP_SHL:
        case BINOP_SHR:
            {
                (void) LOGGER_log(logger, L_ERROR,
                                  "Op %d is not a real comparison operator.\n",
                                  op);
                exit(LUKA_CODEGEN_ERROR);
            }
    }
}

/**
 * @brief Get the address of the expression of `node`.
 *
 * @param[in] node the AST node.
 */
static LLVMValueRef gen_get_address(t_ast_node *node, LLVMModuleRef module,
                                    LLVMBuilderRef builder, t_logger *logger)
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

                LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                               "Variable %s is undefined.\n",
                               node->variable.name);
                exit(LUKA_CODEGEN_ERROR);
            }
        case AST_TYPE_GET_EXPR:
            {
                t_named_value *variable = NULL;

                if (NULL == node->get_expr.variable)
                {
                    LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                                   "Get expr variable name is null.\n", NULL);
                    exit(LUKA_CODEGEN_ERROR);
                }

                HASH_FIND_STR(named_values,
                              node->get_expr.variable->variable.name, variable);
                if (NULL == variable)
                {
                    LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                                   "Couldn't find a variable named `%s`.\n",
                                   node->get_expr.variable->variable.name);
                    exit(LUKA_CODEGEN_ERROR);
                }

                return gen_get_struct_field_pointer(
                    variable, node->get_expr.key, builder, logger);
            }
        case AST_TYPE_ARRAY_DEREF:
            {
                t_named_value *val = NULL;
                LLVMTypeKind val_type_kind = LLVMIntegerTypeKind;
                LLVMValueRef index = NULL;
                LLVMValueRef ptr = NULL;

                HASH_FIND_STR(named_values,
                              node->array_deref.variable->variable.name, val);

                if (NULL == val)
                {
                    LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                                   "Variable %s is undefined.\n",
                                   node->array_deref.variable->variable.name);
                    exit(LUKA_CODEGEN_ERROR);
                }

                val_type_kind = LLVMGetTypeKind(val->type);

                if ((LLVMArrayTypeKind != val_type_kind)
                    && (LLVMPointerTypeKind != val_type_kind))
                {
                    LOGGER_LOG_LOC(
                        logger, L_ERROR, node->token,
                        "Variable %s is not an array or a pointer.\n",
                        node->array_deref.variable->variable.name);
                    exit(LUKA_CODEGEN_ERROR);
                }

                index = GEN_codegen(node->array_deref.index, module, builder,
                                    logger);
                if (NULL == index)
                {
                    LOGGER_LOG_LOC(
                        logger, L_ERROR, node->array_deref.index->token,
                        "Couldn't generate index in array dereference.\n",
                        NULL);
                    exit(LUKA_CODEGEN_ERROR);
                }

                if (LLVMIntegerTypeKind != LLVMGetTypeKind(LLVMTypeOf(index)))
                {
                    LOGGER_LOG_LOC(logger, L_ERROR,
                                   node->array_deref.index->token,
                                   "Index in array dereference should "
                                   "resolve to an integer.\n",
                                   NULL);
                    exit(LUKA_CODEGEN_ERROR);
                }

                ptr = val->alloca_inst;
                if (LLVMArrayTypeKind != LLVMGetTypeKind(val->type))
                {
                    ptr = LLVMBuildLoad(builder, ptr, "loadtmp");
                }

                return LLVMBuildGEP2(builder, LLVMGetElementType(val->type),
                                     ptr, &index, 1, "arrdereftmp");
            }
        case AST_TYPE_UNARY_EXPR:
            {
                if (node->unary_expr.operator!= UNOP_DEREF)
                {
                    LOGGER_LOG_LOC(
                        logger, L_ERROR, node->token,
                        "Can't assign to unary expr not of type deref %d.\n",
                        node->unary_expr.operator);
                    exit(LUKA_CODEGEN_ERROR);
                }

                return LLVMBuildLoad(builder,
                                     gen_get_address(node->unary_expr.rhs,
                                                     module, builder, logger),
                                     "loadtmp");
            }
        case AST_TYPE_ARRAY_LITERAL:
        case AST_TYPE_ASSIGNMENT_EXPR:
        case AST_TYPE_BINARY_EXPR:
        case AST_TYPE_BREAK_STMT:
        case AST_TYPE_BUILTIN:
        case AST_TYPE_CALL_EXPR:
        case AST_TYPE_CAST_EXPR:
        case AST_TYPE_DEFER_STMT:
        case AST_TYPE_ENUM_DEFINITION:
        case AST_TYPE_EXPRESSION_STMT:
        case AST_TYPE_FUNCTION:
        case AST_TYPE_IF_EXPR:
        case AST_TYPE_LET_STMT:
        case AST_TYPE_LITERAL:
        case AST_TYPE_NUMBER:
        case AST_TYPE_PROTOTYPE:
        case AST_TYPE_RETURN_STMT:
        case AST_TYPE_STRING:
        case AST_TYPE_STRUCT_DEFINITION:
        case AST_TYPE_STRUCT_VALUE:
        case AST_TYPE_TYPE_EXPR:
        case AST_TYPE_WHILE_EXPR:
            {
                LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                               "Can't get address of %d.\n", node->type);
                exit(LUKA_CODEGEN_ERROR);
            }
    }
}

/**
 * @brief Generate LLVM IR for an unary expression.
 *
 * @param[in] n the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the unary expression.
 */
static LLVMValueRef gen_codegen_unexpr(t_ast_node *n, LLVMModuleRef module,
                                       LLVMBuilderRef builder, t_logger *logger)
{
    LLVMValueRef rhs = NULL;
    t_type *type = NULL;
    if (NULL != n->unary_expr.rhs)
    {
        rhs = GEN_codegen(n->unary_expr.rhs, module, builder, logger);
    }

    if (NULL == rhs)
    {
        LOGGER_LOG_LOC(logger, L_ERROR, n->unary_expr.rhs->token,
                       "Couldn't codegen rhs for unary expression.\n", NULL);
        exit(LUKA_CODEGEN_ERROR);
    }

    switch (n->unary_expr.operator)
    {
        case UNOP_NOT:
            {
                return LLVMBuildNot(builder, rhs, "nottmp");
            }
        case UNOP_MINUS:
            {
                type = gen_llvm_type_to_ttype(LLVMTypeOf(rhs), logger);
                if (TYPE_is_floating_type(type))
                {
                    (void) TYPE_free_type(type);
                    type = NULL;
                    return LLVMBuildFNeg(builder, rhs, "negtmp");
                }

                (void) TYPE_free_type(type);
                type = NULL;
                return LLVMBuildNeg(builder, rhs, "negtmp");
            }
        case UNOP_REF:
            {
                return gen_get_address(n->unary_expr.rhs, module, builder,
                                       logger);
            }
        case UNOP_DEREF:
            {
                return LLVMBuildLoad(builder, rhs, "loadtmp");
            }
        case UNOP_PLUS:
            {
                LOGGER_LOG_LOC(logger, L_ERROR, n->token,
                               "Currently not supporting %d operator in "
                               "unary expression.\n",
                               n->unary_expr.operator);
                exit(LUKA_CODEGEN_ERROR);
            }
        case UNOP_BNOT:
            {
                return LLVMBuildXor(
                    builder,
                    LLVMConstInt(LLVMTypeOf(rhs), (unsigned long) -1, true),
                    rhs, "bnottmp");
            }
    }
}

/**
 * @brief Generate LLVM IR for a binary expression.
 *
 * @param[in] n the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the binary expression.
 */
static LLVMValueRef gen_codegen_binexpr(t_ast_node *n, LLVMModuleRef module,
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
        LOGGER_LOG_LOC(logger, L_ERROR, n->token,
                       "Binexpr lhs or rhs is null.\n", NULL);
        exit(LUKA_CODEGEN_ERROR);
    }

    if (AST_is_cond_binop(n->binary_expr.operator))
    {
        (void) gen_llvm_cast_null_if_needed(&lhs, &rhs, logger);
        if (gen_is_icmp(lhs, rhs, logger))
        {
            int_predicate = gen_llvm_get_int_predicate(
                n->binary_expr.operator, & lhs, &rhs, builder, logger);
            return LLVMBuildICmp(builder, int_predicate, lhs, rhs, "icmptmp");
        }
        real_predicate = gen_llvm_get_real_predicate(
            n->binary_expr.operator, & lhs, &rhs, builder, logger);
        return LLVMBuildFCmp(builder, real_predicate, lhs, rhs, "fcmptmp");
    }

    opcode = gen_get_llvm_opcode(n->binary_expr.operator, & lhs, &rhs, builder,
                                 logger);
    return LLVMBuildBinOp(builder, opcode, lhs, rhs, "binoptmp");
}

static LLVMTypeRef gen_function_type(t_ast_node *prototype, t_logger *logger)
{
    size_t i = 0, arity = prototype->prototype.arity;
    bool vararg = prototype->prototype.vararg;
    LLVMTypeRef *params = NULL;

    if (vararg)
    {
        --arity;
    }
    params = calloc(arity, sizeof(LLVMTypeRef));
    if (NULL == params)
    {
        (void) LOGGER_log(
            logger, L_ERROR,
            "Failed to assign memory for params in prototype generaion.\n");
        exit(LUKA_CANT_ALLOC_MEMORY);
    }

    for (i = 0; i < arity; ++i)
    {
        params[i]
            = gen_type_to_llvm_type(prototype->prototype.types[i], logger);
    }

    return LLVMFunctionType(
        gen_type_to_llvm_type(prototype->prototype.return_type, logger), params,
        (unsigned int) arity, vararg);
}

/**
 * @brief Generate LLVM IR for a function prototype.
 *
 * @param[in] n the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the function prototype.
 */
static LLVMValueRef gen_codegen_prototype(t_ast_node *n, LLVMModuleRef module,
                                          LLVMBuilderRef UNUSED(builder),
                                          t_logger *logger)
{
    LLVMValueRef func = NULL, module_func = NULL, param = NULL;
    LLVMTypeRef func_type = NULL;
    unsigned int i = 0;
    size_t arity = n->prototype.arity;
    bool vararg = n->prototype.vararg;
    LLVMTypeRef *params = NULL;
    char *func_type_str = NULL, *module_func_str = NULL;
    if (vararg)
    {
        --arity;
    }

    func_type = gen_function_type(n, logger);

    module_func = LLVMGetNamedFunction(module, n->prototype.name);
    if (NULL != module_func)
    {
        func_type_str = LLVMPrintTypeToString(func_type);
        module_func_str = LLVMPrintTypeToString(LLVMTypeOf(module_func));
        if ((LLVMExternalLinkage == LLVMGetLinkage(module_func))
            && (0 == strcmp(func_type_str, module_func_str)))
        {
            func = module_func;
            goto l_cleanup;
        }
    }
    func = LLVMAddFunction(module, n->prototype.name, func_type);
    (void) LLVMSetLinkage(func, LLVMExternalLinkage);

    for (i = 0; i < arity; ++i)
    {
        param = LLVMGetParam(func, i);
        (void) LLVMSetValueName(param, n->prototype.args[i]);
    }

l_cleanup:
    if (NULL != params)
    {
        (void) free(params);
    }

    if (NULL != func_type_str)
    {
        (void) LLVMDisposeMessage(func_type_str);
    }

    if (NULL != module_func_str)
    {
        (void) LLVMDisposeMessage(module_func_str);
    }

    return func;
}

/**
 * @brief Generate LLVM IR for a vector of statements.
 *
 * @param[in] statements a vector of AST nodes for the statements.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[out] has_return_stmt whether there's a return statement in one of
 * these statements.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the statements.
 */
static LLVMValueRef gen_codegen_stmts(t_vector *statements,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder,
                                      bool *has_return_stmt, t_logger *logger)
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

/**
 * @brief Generate LLVM IR for a function.
 *
 * @param[in] n the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the function.
 */
static LLVMValueRef gen_codegen_function(t_ast_node *n, LLVMModuleRef module,
                                         LLVMBuilderRef builder,
                                         t_logger *logger)
{
    LLVMValueRef func = NULL, ret_val = NULL;
    LLVMBasicBlockRef block = NULL;
    t_ast_node *proto = NULL;
    bool has_return_stmt = false;
    t_type *return_ttype;
    LLVMTypeRef return_type = NULL, expected_func_type = NULL, func_type = NULL;
    unsigned int i = 0;
    size_t arity = 0;
    t_named_value *val = NULL;
    char **args = NULL;

    expected_func_type = gen_function_type(n->function.prototype, logger);
    func = LLVMGetNamedFunction(module, n->function.prototype->prototype.name);
    if (NULL == func)
    {
        func = GEN_codegen(n->function.prototype, module, builder, logger);
    }
    else if (0 != LLVMCountBasicBlocks(func))
    {
        LOGGER_LOG_LOC(logger, L_ERROR, n->token,
                       "Cannot redefine function %s\n",
                       n->function.prototype->prototype.name);
        exit(LUKA_CODEGEN_ERROR);
    }
    else if ((func_type = LLVMGetElementType(LLVMTypeOf(func)))
             != expected_func_type)
    {
        LOGGER_LOG_LOC(logger, L_ERROR, n->token,
                       "Previous declaration of function %s does not match "
                       "current declaration, previous: %s, current: %s\n",
                       n->function.prototype->prototype.name,
                       LLVMPrintTypeToString(func_type),
                       LLVMPrintTypeToString(expected_func_type));
        exit(LUKA_CODEGEN_ERROR);
    }

    if (NULL == func)
    {
        LOGGER_LOG_LOC(logger, L_ERROR, n->token,
                       "Prototype generation failed in function generation\n",
                       NULL);
        exit(LUKA_CODEGEN_ERROR);
    }

    if (NULL == n->function.body)
    {
        /* Extern definition */
        return func;
    }

    proto = n->function.prototype;
    arity = proto->prototype.arity;
    args = proto->prototype.args;

    (void) vector_clear(defer_blocks);

    block = LLVMAppendBasicBlock(func, "entry");
    (void) LLVMPositionBuilderAtEnd(builder, block);

    for (i = 0; i < arity; ++i)
    {
        val = malloc(sizeof(t_named_value));
        if (NULL == val)
        {
            LOGGER_LOG_LOC(logger, L_ERROR, n->token,
                           "Couldn't allocate memory for named value in "
                           "gen_codegen_function.",
                           NULL);
            exit(LUKA_CODEGEN_ERROR);
        }

        val->name = strdup(args[i]);
        val->type = LLVMTypeOf(LLVMGetParam(func, i));
        val->ttype = TYPE_dup_type(proto->prototype.types[i]);
        val->mutable = val->ttype->mutable;
        val->alloca_inst
            = gen_create_entry_block_allca(func, val->type, val->name);

        (void) LLVMBuildStore(builder, LLVMGetParam(func, i), val->alloca_inst);

        HASH_ADD_KEYPTR(hh, named_values, val->name, strlen(val->name), val);

        val = NULL;
    }

    ret_val = gen_codegen_stmts(n->function.body, module, builder,
                                &has_return_stmt, logger);
    return_ttype = n->function.prototype->prototype.return_type;
    return_type = gen_type_to_llvm_type(return_ttype, logger);

    if ((NULL == ret_val)
        || !AST_is_expression(VECTOR_GET_AS(t_ast_node_ptr, n->function.body,
                                            n->function.body->size - 1)))
    {
        if ((false == has_return_stmt))
        {
            switch (return_ttype->type)
            {
                case TYPE_VOID:
                    ret_val = NULL;
                    break;
                case TYPE_F32:
                case TYPE_F64:
                    ret_val = LLVMConstReal(return_type, 0.0);
                    break;
                case TYPE_ANY:
                case TYPE_STRING:
                    ret_val = LLVMConstPointerNull(return_type);
                    break;
                case TYPE_SINT8:
                case TYPE_SINT16:
                case TYPE_SINT32:
                case TYPE_SINT64:
                    ret_val = LLVMConstInt(return_type, 0, true);
                    break;
                case TYPE_UINT8:
                case TYPE_UINT16:
                case TYPE_UINT32:
                case TYPE_UINT64:
                    ret_val = LLVMConstInt(return_type, 0, false);
                    break;
                case TYPE_ALIAS:
                case TYPE_ARRAY:
                case TYPE_BOOL:
                case TYPE_ENUM:
                case TYPE_PTR:
                case TYPE_STRUCT:
                case TYPE_TYPE:
                    ret_val = LLVMConstInt(return_type, 0, false);
                    break;
            }
        }
    }
    else if ((NULL != ret_val) && (LLVMTypeOf(ret_val) != return_type))
    {
        ret_val = gen_codegen_cast(builder, ret_val, return_type, logger);
    }

    (void) gen_codegen_defer_blocks(module, builder, logger);

    if (!has_return_stmt)
    {
        (void) LLVMBuildRet(builder, ret_val);
    }

    if (1 == LLVMVerifyFunction(func, LLVMReturnStatusAction))
    {
        (void) LLVMDumpModule(module);
        LOGGER_LOG_LOC(logger, L_ERROR, n->token, "Invalid function %s\n",
                       n->function.prototype->prototype.name);
        (void) LLVMVerifyFunction(func, LLVMPrintMessageAction);
        (void) LLVMDeleteFunction(func);
        exit(LUKA_CODEGEN_ERROR);
    }

    return func;
}

/**
 * @brief Generate LLVM IR for a return statement.
 *
 * @param[in] n the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the return statement.
 */
static LLVMValueRef gen_codegen_return_stmt(t_ast_node *n, LLVMModuleRef module,
                                            LLVMBuilderRef builder,
                                            t_logger *logger)
{
    LLVMValueRef expr = NULL;

    if (NULL == n->return_stmt.expr)
    {
        LOGGER_LOG_LOC(logger, L_ERROR, n->token,
                       "Return statement has no expr.\n", NULL);
        exit(LUKA_CODEGEN_ERROR);
    }

    (void) gen_codegen_defer_blocks(module, builder, logger);

    expr = GEN_codegen(n->return_stmt.expr, module, builder, logger);
    if (NULL == expr)
    {
        LOGGER_LOG_LOC(logger, L_ERROR, n->token,
                       "Expression generation failed in return stmt\n", NULL);
        exit(LUKA_CODEGEN_ERROR);
    }
    (void) LLVMBuildRet(builder, expr);
    return NULL;
}

/**
 * @brief Generate LLVM IR for an if expression.
 *
 * @param[in] n the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the if expression.
 */
static LLVMValueRef gen_codegen_if_expr(t_ast_node *n, LLVMModuleRef module,
                                        LLVMBuilderRef builder,
                                        t_logger *logger)
{
    LLVMValueRef cond = NULL, then_value = NULL, else_value = NULL, phi = NULL,
                 func = NULL, incoming_values[2] = {NULL, NULL};
    LLVMBasicBlockRef cond_block = NULL, then_block = NULL, else_block = NULL,
                      merge_block = NULL;
    bool has_return_stmt = false;

    func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

    cond_block = LLVMAppendBasicBlock(func, "if_cond");
    then_block = LLVMCreateBasicBlockInContext(LLVMGetGlobalContext(), "then");
    if (NULL != n->if_expr.else_body)
    {
        else_block
            = LLVMCreateBasicBlockInContext(LLVMGetGlobalContext(), "else");
    }
    merge_block
        = LLVMCreateBasicBlockInContext(LLVMGetGlobalContext(), "if_merge");

    (void) LLVMBuildBr(builder, cond_block);
    (void) LLVMPositionBuilderAtEnd(builder, cond_block);

    cond = GEN_codegen(n->if_expr.cond, module, builder, logger);
    if (NULL == cond)
    {
        LOGGER_LOG_LOC(logger, L_ERROR, n->token,
                       "Condition generation failed in if expr\n", NULL);
        exit(LUKA_CODEGEN_ERROR);
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

    then_value = gen_codegen_stmts(n->if_expr.then_body, module, builder,
                                   &has_return_stmt, logger);

    if (!has_return_stmt)
    {
        (void) LLVMBuildBr(builder, merge_block);
    }

    then_block = LLVMGetInsertBlock(builder);

    if (NULL != n->if_expr.else_body)
    {
        (void) LLVMAppendExistingBasicBlock(func, else_block);
        (void) LLVMPositionBuilderAtEnd(builder, else_block);
        has_return_stmt = false;
        else_value = gen_codegen_stmts(n->if_expr.else_body, module, builder,
                                       &has_return_stmt, logger);

        if (!has_return_stmt)
        {
            (void) LLVMBuildBr(builder, merge_block);
        }
    }

    else_block = LLVMGetInsertBlock(builder);

    (void) LLVMAppendExistingBasicBlock(func, merge_block);
    (void) LLVMPositionBuilderAtEnd(builder, merge_block);

    if ((NULL == then_value)
        && !((NULL != n->if_expr.else_body) && (NULL != else_value)))
    {
        return NULL;
    }

    if (((NULL == then_value) && (NULL != else_value))
        || ((NULL != then_value) && (NULL == else_value)))
    {
        LOGGER_LOG_LOC(
            logger, L_ERROR, n->token,
            "If one branch returns a values, both must return a value.\n",
            NULL);
        exit(LUKA_CODEGEN_ERROR);
    }

    if ((NULL != n->if_expr.else_body)
        && (LLVMTypeOf(then_value) != LLVMTypeOf(else_value)))
    {
        LOGGER_LOG_LOC(logger, L_ERROR, n->token,
                       "Values of then and else branches must be of the "
                       "same type in if expr.\n",
                       NULL);
        exit(LUKA_CODEGEN_ERROR);
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

/**
 * @brief Generate LLVM IR for a while expression.
 *
 * @param[in] n the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the while expression.
 */
static LLVMValueRef gen_codegen_while_expr(t_ast_node *n, LLVMModuleRef module,
                                           LLVMBuilderRef builder,
                                           t_logger *logger)
{
    LLVMValueRef func = NULL, cond = NULL, body_value = NULL;
    LLVMBasicBlockRef cond_block = NULL, body_block = NULL, end_block = NULL;

    func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

    cond_block = LLVMAppendBasicBlock(func, "while_cond");
    body_block = LLVMAppendBasicBlock(func, "while_body");
    end_block
        = LLVMCreateBasicBlockInContext(LLVMGetGlobalContext(), "while_end");

    if (NULL != loop_blocks)
    {
        (void) vector_push_front(loop_blocks, &end_block);
    }

    (void) LLVMBuildBr(builder, cond_block);
    (void) LLVMPositionBuilderAtEnd(builder, cond_block);

    cond = GEN_codegen(n->while_expr.cond, module, builder, logger);
    if (NULL == cond)
    {
        LOGGER_LOG_LOC(logger, L_ERROR, n->token,
                       "Condition generation failed in while expr\n", NULL);
        exit(LUKA_CODEGEN_ERROR);
    }

    (void) LLVMBuildCondBr(builder, cond, body_block, end_block);
    (void) LLVMPositionBuilderAtEnd(builder, body_block);

    body_value
        = gen_codegen_stmts(n->while_expr.body, module, builder, NULL, logger);
    cond = GEN_codegen(n->while_expr.cond, module, builder, logger);
    if (NULL == cond)
    {
        LOGGER_LOG_LOC(logger, L_ERROR, n->token,
                       "Condition generation failed in while expr\n", NULL);
        exit(LUKA_CODEGEN_ERROR);
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

/**
 * @brief Generate LLVM IR for a cast expression.
 *
 * @param[in] node the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the cast expression.
 */
static LLVMValueRef gen_codegen_cast_expr(t_ast_node *node,
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

/**
 * @brief Generate LLVM IR for a variable reference.
 *
 * @param[in] node the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the variable reference.
 */
static LLVMValueRef gen_codegen_variable(t_ast_node *node,
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

    LOGGER_LOG_LOC(logger, L_ERROR, node->token, "Variable %s is undefined.\n",
                   node->variable.name);
    exit(LUKA_CODEGEN_ERROR);
}

/**
 * @brief Generate LLVM IR for a let statement.
 *
 * @param[in] node the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the let statement.
 */
static LLVMValueRef gen_codegen_let_stmt(t_ast_node *node, LLVMModuleRef module,
                                         LLVMBuilderRef builder,
                                         t_logger *logger)
{
    LLVMValueRef expr = NULL;
    t_ast_variable variable;
    t_named_value *val = NULL;
    bool is_global = node->let_stmt.is_global;
    bool extern_var = false;

    variable = node->let_stmt.var->variable;

    if (!is_global && (NULL == node->let_stmt.expr))
    {
        LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                       "Non global let statements must have an expression.\n",
                       NULL);
    }

    if (NULL != node->let_stmt.expr)
    {
        expr = GEN_codegen(node->let_stmt.expr, module, builder, logger);
        if (NULL == expr)
        {
            LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                           "Expression generation in let stmt failed.\n", NULL);
            exit(LUKA_CODEGEN_ERROR);
        }
    }
    else
    {
        extern_var = true;
    }

    val = malloc(sizeof(t_named_value));
    val->name = strdup(variable.name);
    if ((NULL == variable.type) && !extern_var)
    {
        val->type = LLVMTypeOf(expr);
        val->ttype = gen_llvm_type_to_ttype(val->type, logger);
        if (TYPE_STRUCT == val->ttype->type)
        {
            val->ttype->payload
                = strdup(node->let_stmt.expr->struct_value.name);
        }
    }
    else
    {
        val->ttype = TYPE_dup_type(variable.type);
        val->type = gen_type_to_llvm_type(val->ttype, logger);
    }

    if (!extern_var && (LLVMTypeOf(expr) != val->type))
    {
        expr = gen_codegen_cast(builder, expr, val->type, logger);
    }

    if (is_global)
    {
        val->alloca_inst = LLVMAddGlobal(module, val->type, val->name);
        if (!extern_var)
        {
            (void) LLVMSetInitializer(val->alloca_inst, expr);
        }
    }
    else
    {
        val->alloca_inst = gen_create_entry_block_allca(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), val->type,
            val->name);
        if ((TYPE_STRUCT != val->ttype->type)
            && (TYPE_ARRAY != val->ttype->type))
        {
            (void) LLVMSetAlignment(
                LLVMBuildStore(builder, expr, val->alloca_inst),
                LLVMGetAlignment(val->alloca_inst)
                    ? LLVMGetAlignment(val->alloca_inst)
                    : 8);
        }
        else
        {
            LLVMBuildMemCpy(
                builder,
                LLVMBuildBitCast(builder, val->alloca_inst,
                                 LLVMPointerType(LLVMInt8Type(), 0), ""),
                8,
                LLVMBuildBitCast(builder, expr,
                                 LLVMPointerType(LLVMInt8Type(), 0), ""),
                8, LLVMSizeOf(val->type));
        }
    }

    val->mutable = variable.mutable || variable.type->mutable;
    HASH_ADD_KEYPTR(hh, named_values, val->name, strlen(val->name), val);

    return NULL;
}

/**
 * @brief Generate LLVM IR for an assignment expression.
 *
 * @param[in] node the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the assignment expression.
 */
static LLVMValueRef gen_codegen_assignment_expr(t_ast_node *node,
                                                LLVMModuleRef module,
                                                LLVMBuilderRef builder,
                                                t_logger *logger)
{
    LLVMValueRef lhs = NULL, rhs = NULL, store = NULL;
    LLVMTypeRef dest_type = NULL;
    ssize_t alignment = 0;
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
                LOGGER_LOG_LOC(
                    logger, L_ERROR, node->token,
                    "variable: Cannot assign to undeclared variable '%s'.\n",
                    variable->variable.name);
                exit(LUKA_CODEGEN_ERROR);
            }

            lhs = gen_get_address(node->assignment_expr.lhs, module, builder,
                                  logger);
        }
        else if (AST_TYPE_GET_EXPR == node->assignment_expr.lhs->type)
        {
            variable = node->assignment_expr.lhs;
            HASH_FIND_STR(named_values,
                          variable->get_expr.variable->variable.name, val);
            if (NULL == val)
            {
                LOGGER_LOG_LOC(
                    logger, L_ERROR, node->token,
                    "get_expr: Cannot assign to undeclared variable '%s'.\n",
                    variable->get_expr.variable->variable.name);
                exit(LUKA_CODEGEN_ERROR);
            }

            lhs = gen_get_address(node->assignment_expr.lhs, module, builder,
                                  logger);
        }
        else if (AST_TYPE_ARRAY_DEREF == node->assignment_expr.lhs->type)
        {
            variable = node->assignment_expr.lhs;
            HASH_FIND_STR(named_values,
                          variable->array_deref.variable->variable.name, val);
            if (NULL == val)
            {
                LOGGER_LOG_LOC(
                    logger, L_ERROR, node->token,
                    "array_deref: Cannot assign to undeclared variable '%s'.\n",
                    variable->array_deref.variable->variable.name);
                exit(LUKA_CODEGEN_ERROR);
            }

            lhs = gen_get_address(node->assignment_expr.lhs, module, builder,
                                  logger);
        }
        else
        {
            lhs = gen_get_address(node->assignment_expr.lhs, module, builder,
                                  logger);
        }
    }

    if (NULL != node->assignment_expr.rhs)
    {
        rhs = GEN_codegen(node->assignment_expr.rhs, module, builder, logger);
    }

    if ((NULL == lhs) || (NULL == rhs))
    {
        LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                       "Expression generation in assignment expr failed.\n",
                       NULL);
        exit(LUKA_CODEGEN_ERROR);
    }

    dest_type = LLVMGetElementType(LLVMTypeOf(lhs));
    rhs = LLVMBuildCast(
        builder, gen_llvm_get_cast_op(LLVMTypeOf(rhs), dest_type, logger), rhs,
        dest_type, "casttmp");
    store = LLVMBuildStore(builder, rhs, lhs);
    alignment = TYPE_sizeof(gen_llvm_type_to_ttype(LLVMTypeOf(lhs), logger));
    if (0 != alignment)
    {
        (void) LLVMSetAlignment(store, (unsigned int) alignment);
    }
    return rhs;
}

static LLVMValueRef gen_codegen_builtin_call(t_ast_node *node,
                                             LLVMModuleRef UNUSED(module),
                                             LLVMBuilderRef UNUSED(builder),
                                             t_logger *logger)
{
    switch (node->call_expr.callable->builtin.id)
    {
        case BUILTIN_ID_SIZEOF:
            {
                t_ast_node *arg0_node
                    = VECTOR_GET_AS(t_ast_node_ptr, node->call_expr.args, 0);
                t_type *type = arg0_node->type_expr.type;
                return gen_codegen_sizeof(node, type, logger);
            }
        case BUILTIN_ID_INVALID:
            return NULL;
    }
}

/**
 * @brief Generate LLVM IR for a call expression.
 *
 * @param[in] node the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the call expression.
 */
static LLVMValueRef gen_codegen_call(t_ast_node *node, LLVMModuleRef module,
                                     LLVMBuilderRef builder, t_logger *logger)
{
    LLVMValueRef call = NULL;
    LLVMValueRef func = NULL;
    LLVMValueRef *args = NULL;
    LLVMValueRef *varargs = NULL;
    LLVMTypeRef func_type = NULL, type = NULL, dest_type = NULL;
    t_ast_node *arg = NULL;
    unsigned int i = 0;
    bool vararg = NULL, builtin = false, pushed_first_arg = false;
    size_t required_params_count = 0;
    char function_name_buffer[1024] = {0};

    (void) UTILS_fill_function_name(function_name_buffer,
                                    sizeof(function_name_buffer), node,
                                    &pushed_first_arg, &builtin, logger);

    if (builtin)
    {
        func = gen_codegen_prototype(
            CORE_lookup_builtin(node->call_expr.callable), module, builder,
            logger);
    }
    else
    {
        func = LLVMGetNamedFunction(module, function_name_buffer);
    }
    if (NULL == func)
    {
        LOGGER_LOG_LOC(
            logger, L_ERROR, node->token,
            "Couldn't find a function named `%s`, are you sure you defined it "
            "or wrote a proper extern line for it?\n",
            function_name_buffer);
        exit(LUKA_CODEGEN_ERROR);
    }

    func_type = LLVMGetElementType(LLVMTypeOf(func));
    vararg = LLVMIsFunctionVarArg(func_type);
    required_params_count = LLVMCountParams(func);

    if (!vararg && node->call_expr.args->size != required_params_count)
    {
        LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                       "Function %s called with incorrect number of arguments, "
                       "expected %d arguments but got %d arguments.\n",
                       function_name_buffer, required_params_count,
                       node->call_expr.args->size);
        exit(LUKA_CODEGEN_ERROR);
    }

    if (vararg && node->call_expr.args->size < required_params_count)
    {
        LOGGER_LOG_LOC(
            logger, L_ERROR, node->token,
            "Function %s is variadic but not called with enough arguments, "
            "expected at least %d arguments but got %d arguments.\n",
            function_name_buffer, node->call_expr.args->size,
            required_params_count);
        exit(LUKA_CODEGEN_ERROR);
    }

    if (!builtin)
    {
        args = calloc(node->call_expr.args->size, sizeof(LLVMValueRef));
        if (NULL == args)
        {
            goto l_cleanup;
        }

        VECTOR_FOR_EACH(node->call_expr.args, args_iter)
        {
            arg = ITERATOR_GET_AS(t_ast_node_ptr, &args_iter);
            if ((arg->type == AST_TYPE_VARIABLE)
                && (arg->variable.type->type == TYPE_ARRAY)
                && (((size_t) arg->variable.type->payload) != 0))
            {
                /* Arrays decay to pointers */
                t_named_value *val = NULL;

                LLVMValueRef indices[2] = {LLVMConstInt(LLVMInt32Type(), 0, 0),
                                           LLVMConstInt(LLVMInt32Type(), 0, 0)};

                HASH_FIND_STR(named_values, arg->variable.name, val);
                if (NULL == val)
                {
                    (void) LOGGER_log(logger, L_ERROR,
                                      "Variable %s is undefined.\n",
                                      arg->variable.name);
                    exit(LUKA_CODEGEN_ERROR);
                }
                args[i] = LLVMBuildInBoundsGEP2(builder, val->type,
                                                val->alloca_inst, indices, 2,
                                                "tempgep");
            }
            else
            {
                args[i] = GEN_codegen(arg, module, builder, logger);
            }
            if (NULL == args[i])
            {
                goto l_cleanup;
            }

            if (i < required_params_count)
            {
                type = LLVMTypeOf(args[i]);
                dest_type = LLVMTypeOf(LLVMGetParam(func, i));
                if (type != dest_type)
                {
                    if (arg->type != AST_TYPE_TYPE_EXPR)
                    {
                        args[i] = gen_codegen_cast(builder, args[i], dest_type,
                                                   logger);
                    }
                }
            }
            ++i;
        }
    }

    if (builtin)
    {
        LLVMDeleteFunction(func);
        call = gen_codegen_builtin_call(node, module, builder, logger);
    }
    else
    {
        call = LLVMBuildCall2(
            builder, func_type, func, args,
            (unsigned int) node->call_expr.args->size,
            LLVMGetReturnType(func_type) != LLVMVoidType() ? "calltmp" : "");
    }

l_cleanup:
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

    if (pushed_first_arg)
    {
        (void) UTILS_pop_first_arg(node, logger);
    }

    return call;
}

/**
 * @brief Generate LLVM IR for an expression statement.
 *
 * @param[in] n the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the expression statement.
 */
static LLVMValueRef gen_codegen_expression_stmt(t_ast_node *n,
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

/**
 * @brief Generate LLVM IR for a break statement.
 *
 * @param[in] n the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the break statement.
 */
static LLVMValueRef gen_codegen_break_stmt(t_ast_node *n,
                                           LLVMModuleRef UNUSED(module),
                                           LLVMBuilderRef builder,
                                           t_logger *logger)
{
    LLVMBasicBlockRef dest_block = NULL;

    if ((NULL == loop_blocks) || (0 == loop_blocks->size))
    {
        LOGGER_LOG_LOC(logger, L_WARNING, n->token,
                       "Cannot break when not inside a loop.\n", NULL);
        return NULL;
    }

    dest_block = VECTOR_GET_AS(LLVMBasicBlockRef, loop_blocks, 0);

    (void) LLVMBuildBr(builder, dest_block);
    // TODO: Find if there's a better way to supress "Terminator found in the
    // middle of a basic block"
    dest_block = LLVMAppendBasicBlock(
        LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "unused_block");
    (void) LLVMPositionBuilderAtEnd(builder, dest_block);
    return NULL;
}

/**
 * @brief Generate LLVM IR for a number literal.
 *
 * @param[in] node the AST node.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the number literal.
 */
static LLVMValueRef gen_codegen_number(t_ast_node *node, t_logger *logger)
{
    LLVMTypeRef type = gen_type_to_llvm_type(node->number.type, logger);
    switch (node->number.type->type)
    {
        case TYPE_F32:
            return LLVMConstReal(type, (double) node->number.value.f32);
        case TYPE_F64:
            return LLVMConstReal(type, node->number.value.f64);
        case TYPE_SINT8:
            return LLVMConstInt(
                type, (unsigned long long) node->number.value.s8, true);
        case TYPE_SINT16:
            return LLVMConstInt(
                type, (unsigned long long) node->number.value.s16, true);
        case TYPE_SINT32:
            return LLVMConstInt(
                type, (unsigned long long) node->number.value.s32, true);
        case TYPE_SINT64:
            return LLVMConstInt(
                type, (unsigned long long) node->number.value.s64, true);
        case TYPE_UINT8:
            return LLVMConstInt(
                type, (unsigned long long) node->number.value.u8, false);
        case TYPE_UINT16:
            return LLVMConstInt(
                type, (unsigned long long) node->number.value.u16, false);
        case TYPE_UINT32:
            return LLVMConstInt(
                type, (unsigned long long) node->number.value.u32, false);
        case TYPE_UINT64:
            return LLVMConstInt(type, node->number.value.u64, false);
        case TYPE_ALIAS:
        case TYPE_ANY:
        case TYPE_ARRAY:
        case TYPE_BOOL:
        case TYPE_ENUM:
        case TYPE_PTR:
        case TYPE_STRING:
        case TYPE_STRUCT:
        case TYPE_TYPE:
        case TYPE_VOID:
            {
                LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                               "%d is not a number type.\n",
                               node->number.type->type);
                exit(LUKA_GENERAL_ERROR);
            }
    }
}

static void gen_generate_struct_functions(t_struct_info *struct_info,
                                          LLVMModuleRef module,
                                          LLVMBuilderRef builder,
                                          t_logger *logger)
{
    size_t i = 0, functions_count = struct_info->number_of_functions;
    t_ast_node *function = NULL;
    char *function_name = NULL, *new_name = NULL;

    for (i = 0; i < functions_count; ++i)
    {
        function = struct_info->struct_functions[i];
        function_name = function->function.prototype->prototype.name;
        new_name = malloc(1024);
        if (NULL == new_name)
        {
            (void) LOGGER_log(
                logger, L_ERROR,
                "Failed allocating memory for new function name.\n");
            exit(LUKA_CANT_ALLOC_MEMORY);
        }
        snprintf(new_name, 1024, "%s.%s", struct_info->struct_name,
                 function_name);
        function->function.prototype->prototype.name = new_name;

        (void) gen_codegen_function(function, module, builder, logger);
    }
}

/**
 * @brief Generate LLVM IR for a struct definition.
 *
 * @param[in] node the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the struct definition.
 */
static LLVMValueRef gen_codegen_struct_definition(t_ast_node *node,
                                                  LLVMModuleRef module,
                                                  LLVMBuilderRef builder,
                                                  t_logger *logger)
{
    t_ast_struct_definition struct_definition = node->struct_definition;
    size_t elements_count = struct_definition.struct_fields->size;
    size_t functions_count = struct_definition.struct_functions->size;
    LLVMTypeRef struct_type = NULL;
    LLVMTypeRef *element_types = NULL;
    t_struct_info *struct_info = NULL;
    bool error = true;

    element_types = calloc(elements_count, sizeof(LLVMTypeRef));
    if (NULL == element_types)
    {
        goto l_cleanup;
    }

    struct_info = calloc(1, sizeof(t_struct_info));
    if (NULL == struct_info)
    {
        goto l_cleanup;
    }

    struct_info->struct_name = strdup(node->struct_definition.name);
    struct_info->number_of_fields = elements_count;
    struct_info->struct_fields = calloc(elements_count, sizeof(char **));
    if (NULL == struct_info->struct_fields)
    {
        goto l_cleanup;
    }

    struct_info->number_of_functions = functions_count;
    struct_info->struct_functions
        = calloc(functions_count, sizeof(t_ast_node **));
    if (NULL == struct_info->struct_functions)
    {
        goto l_cleanup;
    }

    for (size_t i = 0; i < elements_count; ++i)
    {
        struct_info->struct_fields[i] = NULL;
    }

    for (size_t i = 0; i < functions_count; ++i)
    {
        struct_info->struct_functions[i] = *(t_ast_node **) vector_get(
            struct_definition.struct_functions, i);
    }

    struct_type = LLVMStructCreateNamed(LLVMGetGlobalContext(),
                                        node->struct_definition.name);
    struct_info->struct_type = struct_type;
    HASH_ADD_KEYPTR(hh, struct_infos, struct_info->struct_name,
                    strlen(struct_info->struct_name), struct_info);
    for (size_t i = 0; i < elements_count; ++i)
    {
        element_types[i] = gen_type_to_llvm_type(
            (VECTOR_GET_AS(t_struct_field_ptr,
                           node->struct_definition.struct_fields, i))
                ->type,
            logger);
        struct_info->struct_fields[i]
            = strdup((VECTOR_GET_AS(t_struct_field_ptr,
                                    node->struct_definition.struct_fields, i))
                         ->name);
    }

    (void) LLVMStructSetBody(struct_type, element_types,
                             (unsigned int) elements_count, false);

    struct_info->struct_type = struct_type;

    gen_generate_struct_functions(struct_info, module, builder, logger);

    error = false;

l_cleanup:
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
        if (NULL != struct_info->struct_functions)
        {
            (void) free(struct_info->struct_functions);
            struct_info->struct_functions = NULL;
        }

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

/**
 * @brief Generate LLVM IR for a struct value.
 *
 * @param[in] node the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the struct value.
 */
static LLVMValueRef gen_codegen_struct_value(t_ast_node *node,
                                             LLVMModuleRef module,
                                             LLVMBuilderRef builder,
                                             t_logger *logger)
{
    size_t elements_count = node->struct_value.struct_values->size;
    t_struct_value_field *struct_value_field = NULL;
    LLVMValueRef struct_value = NULL, struct_var = NULL,
                 *element_values = calloc(elements_count, sizeof(LLVMValueRef));
    size_t i = 0;
    if (NULL == element_values)
    {
        return NULL;
    }

    for (i = 0; i < elements_count; ++i)
    {
        element_values[i] = NULL;
    }

    for (i = 0; i < elements_count; ++i)
    {
        struct_value_field = VECTOR_GET_AS(t_struct_value_field_ptr,
                                           node->struct_value.struct_values, i);
        element_values[i]
            = GEN_codegen(struct_value_field->expr, module, builder, logger);
    }

    for (i = 0; i < elements_count; ++i)
    {
        if (NULL == element_values[i])
        {
            element_values[i] = LLVMConstInt(LLVMInt32Type(), 0, true);
        }
    }

    struct_value
        = LLVMConstStruct(element_values, (unsigned int) elements_count, false);

    struct_var = LLVMAddGlobal(module, LLVMTypeOf(struct_value), "struct_val");

    (void) LLVMSetInitializer(struct_var, struct_value);

    return struct_var;
}

/**
 * @brief Generate LLVM IR for an enum definition.
 *
 * @param[in] node the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the enum definition.
 */
static LLVMValueRef gen_codegen_enum_definition(t_ast_node *node,
                                                LLVMModuleRef UNUSED(module),
                                                LLVMBuilderRef UNUSED(builder),
                                                t_logger *UNUSED(logger))
{
    size_t elements_count = node->enum_definition.enum_fields->size;
    t_enum_info *enum_info = NULL;
    bool error = true;

    enum_info = calloc(1, sizeof(t_enum_info));
    if (NULL == enum_info)
    {
        goto l_cleanup;
    }

    enum_info->enum_name = strdup(node->enum_definition.name);
    enum_info->number_of_fields = elements_count;
    enum_info->enum_field_names = calloc(elements_count, sizeof(char **));
    if (NULL == enum_info->enum_field_names)
    {
        goto l_cleanup;
    }

    enum_info->enum_field_values = calloc(elements_count, sizeof(int));
    if (NULL == enum_info->enum_field_values)
    {
        goto l_cleanup;
    }

    for (size_t i = 0; i < elements_count; ++i)
    {
        enum_info->enum_field_names[i] = NULL;
        enum_info->enum_field_values[i] = 0;
    }

    for (size_t i = 0; i < elements_count; ++i)
    {
        enum_info->enum_field_names[i]
            = strdup((VECTOR_GET_AS(t_enum_field_ptr,
                                    node->enum_definition.enum_fields, i))
                         ->name);
        enum_info->enum_field_values[i]
            = (VECTOR_GET_AS(t_enum_field_ptr,
                             node->enum_definition.enum_fields, i))
                  ->expr->number.value.s32;
    }

    HASH_ADD_KEYPTR(hh, enum_infos, enum_info->enum_name,
                    strlen(enum_info->enum_name), enum_info);

    error = false;

l_cleanup:
    if (!error)
    {
        return NULL;
    }

    if (NULL != enum_info)
    {
        if (NULL != enum_info->enum_field_names)
        {
            for (size_t i = 0; i < elements_count; ++i)
            {
                if (NULL != enum_info->enum_field_names[i])
                {
                    (void) free(enum_info->enum_field_names[i]);
                    enum_info->enum_field_names[i] = NULL;
                }
            }
            (void) free(enum_info->enum_field_names);
            enum_info->enum_field_names = NULL;
        }

        if (NULL != enum_info->enum_field_values)
        {
            (void) free(enum_info->enum_field_values);
            enum_info->enum_field_values = NULL;
        }

        (void) free(enum_info);
        enum_info = NULL;
    }

    return NULL;
}

/**
 * @brief Generate LLVM IR for a get expression.
 *
 * @param[in] node the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the get expression.
 */
static LLVMValueRef gen_codegen_get_expr(t_ast_node *node, LLVMModuleRef module,
                                         LLVMBuilderRef builder,
                                         t_logger *logger)
{
    LLVMValueRef field_pointer = NULL, load = NULL;
    t_enum_info *enum_info = NULL;
    char *key = node->get_expr.key;
    ssize_t alignment = 0;

    if (node->get_expr.is_enum)
    {
        HASH_FIND_STR(enum_infos,
                      (char *) node->get_expr.variable->variable.name,
                      enum_info);

        if (NULL == enum_info)
        {
            LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                           "Couldn't find enum info for enum %s.\n",
                           node->get_expr.variable->variable.name);
            exit(LUKA_CODEGEN_ERROR);
        }

        for (size_t i = 0; i < enum_info->number_of_fields; ++i)
        {
            if ((NULL != enum_info->enum_field_names)
                && (0 == strcmp(key, enum_info->enum_field_names[i])))
            {
                return LLVMConstInt(
                    LLVMInt32Type(),
                    (unsigned long long) enum_info->enum_field_values[i], true);
            }
        }

        LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                       "Enum %s has no member %s.\n", enum_info->enum_name,
                       key);
        exit(LUKA_CODEGEN_ERROR);
    }

    field_pointer = gen_get_address(node, module, builder, logger);
    load
        = LLVMBuildLoad2(builder, LLVMGetElementType(LLVMTypeOf(field_pointer)),
                         field_pointer, "loadtmp");

    alignment = TYPE_sizeof(
        gen_llvm_type_to_ttype(LLVMTypeOf(field_pointer), logger));
    if (0 != alignment)
    {
        (void) LLVMSetAlignment(load, (unsigned int) alignment);
    }
    return load;
}

/**
 * @brief Generate LLVM IR for a array dereference.
 *
 * @param[in] node the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the built LLVM IR for the array dereference.
 */
static LLVMValueRef gen_codegen_array_deref(t_ast_node *node,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder,
                                            t_logger *logger)
{
    return LLVMBuildLoad(
        builder, gen_get_address(node, module, builder, logger), "loadtmp");
}

/**
 * @brief Generate LLVM IR for a literal.
 *
 * @param[in] node the AST node.
 *
 * @return the built LLVM IR for the literal.
 */
static LLVMValueRef gen_codegen_literal(t_ast_node *node)
{
    switch (node->literal.type)
    {
        case AST_LITERAL_NULL:
            return LLVMConstPointerNull(LLVMVoidType());
        case AST_LITERAL_TRUE:
            return LLVMConstInt(LLVMInt1Type(), 1, false);
        case AST_LITERAL_FALSE:
            return LLVMConstInt(LLVMInt1Type(), 0, false);
    }
}

/**
 * @brief Generate LLVM IR for a sizeof.
 *
 * @param[in] node the AST node.
 * @param[in] type the type the sizeof is performed on.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the size of the type in the sizeof expr.
 */
static LLVMValueRef gen_codegen_sizeof(t_ast_node *node, t_type *type,
                                       t_logger *logger)
{
    LLVMTypeRef llvm_type = NULL;
    ssize_t size = -1;

    if (NULL == type)
    {
        LOGGER_LOG_LOC(logger, L_ERROR, node->token,
                       "Cannot get size of unknown type.\n", NULL);
    }

    size = TYPE_sizeof(type);
    if (-1 == size)
    {
        llvm_type = gen_type_to_llvm_type(type, logger);
        return LLVMSizeOf(llvm_type);
    }

    return LLVMConstInt(LLVMInt64Type(), (unsigned long long) size, false);
}

void GEN_module_prototypes(t_module *module, LLVMModuleRef llvm_module,
                           LLVMBuilderRef builder, t_logger *logger)
{
    t_ast_node *node = NULL;
    VECTOR_FOR_EACH(module->functions, functions)
    {
        node = ITERATOR_GET_AS(t_ast_node_ptr, &functions);
        if (NULL != node->function.prototype)
        {
            (void) GEN_codegen(node->function.prototype, llvm_module, builder,
                               logger);
        }
    }
}

void GEN_module_structs_without_functions(t_module *module,
                                          LLVMModuleRef llvm_module,
                                          LLVMBuilderRef builder,
                                          t_logger *logger)
{
    t_ast_node *node = NULL;
    t_vector *original_functions = NULL;
    t_vector empty_functions;

    (void) vector_setup(&empty_functions, 0, sizeof(t_ast_node_ptr));
    VECTOR_FOR_EACH(module->structs, structs)
    {
        node = ITERATOR_GET_AS(t_ast_node_ptr, &structs);
        original_functions = node->struct_definition.struct_functions;
        node->struct_definition.struct_functions = &empty_functions;
        (void) GEN_codegen(node, llvm_module, builder, logger);
        node->struct_definition.struct_functions = original_functions;
    }
    (void) vector_destroy(&empty_functions);
}

/**
 * @brief Generate LLVM IR for an array literal.
 *
 * @param[in] node the AST node.
 * @param[in] module the LLVM module.
 * @param[in] builder the LLVM builder.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return a reference to the array literal in the global scope.
 */
static LLVMValueRef gen_codegen_array_literal(t_ast_node *node,
                                              LLVMModuleRef module,
                                              LLVMBuilderRef builder,
                                              t_logger *logger)
{
    t_ast_array_literal lit = node->array_literal;
    size_t i = 0, elements_count = lit.exprs->size;
    LLVMValueRef *constant_vals = NULL, arr_val = NULL;
    t_ast_node *expr = NULL;
    constant_vals = calloc(elements_count, sizeof(LLVMValueRef));
    if (NULL == constant_vals)
    {
        (void) LOGGER_log(logger, L_ERROR,
                          "Couldn't allocate memory for constant_vals.\n");
        exit(LUKA_CANT_ALLOC_MEMORY);
    }

    for (i = 0; i < elements_count; ++i)
    {
        expr = VECTOR_GET_AS(t_ast_node_ptr, lit.exprs, i);
        constant_vals[i] = GEN_codegen(expr, module, builder, logger);
    }

    arr_val
        = LLVMAddGlobal(module,
                        LLVMArrayType(gen_type_to_llvm_type(lit.type, logger),
                                      (unsigned int) elements_count),
                        "arraylit");

    LLVMSetInitializer(
        arr_val, LLVMConstArray(gen_type_to_llvm_type(lit.type, logger),
                                constant_vals, (unsigned int) elements_count));

    return arr_val;
}

static LLVMValueRef gen_codegen_type_expr(t_ast_node *node,
                                          LLVMModuleRef UNUSED(module),
                                          LLVMBuilderRef UNUSED(builder),
                                          t_logger *UNUSED(logger))
{
    return (LLVMValueRef) node->type_expr.type;
}

static LLVMValueRef gen_codegen_defer_stmt(t_ast_node *node,
                                           LLVMModuleRef UNUSED(module),
                                           LLVMBuilderRef UNUSED(builder),
                                           t_logger *logger)
{
    if (NULL != defer_blocks)
    {
        (void) vector_push_front(defer_blocks, &node);
    }
    else
    {
        (void) LOGGER_log(logger, L_ERROR, "Defer blocks is null\n");
        exit(LUKA_CODEGEN_ERROR);
    }

    return NULL;
}

LLVMValueRef GEN_codegen(t_ast_node *node, LLVMModuleRef module,
                         LLVMBuilderRef builder, t_logger *logger)
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
        case AST_TYPE_ENUM_DEFINITION:
            return gen_codegen_enum_definition(node, module, builder, logger);
        case AST_TYPE_GET_EXPR:
            return gen_codegen_get_expr(node, module, builder, logger);
        case AST_TYPE_ARRAY_DEREF:
            return gen_codegen_array_deref(node, module, builder, logger);
        case AST_TYPE_LITERAL:
            return gen_codegen_literal(node);
        case AST_TYPE_ARRAY_LITERAL:
            return gen_codegen_array_literal(node, module, builder, logger);
        case AST_TYPE_TYPE_EXPR:
            return gen_codegen_type_expr(node, module, builder, logger);
        case AST_TYPE_DEFER_STMT:
            return gen_codegen_defer_stmt(node, module, builder, logger);
        case AST_TYPE_BUILTIN:
            {
                /* No code needs to be generated for builtin identifiers */
                return NULL;
            }
    }
}

void GEN_codegen_initialize()
{
    loop_blocks = calloc(1, sizeof(t_vector));
    if (NULL == loop_blocks)
    {
        exit(LUKA_CODEGEN_ERROR);
    }
    (void) vector_setup(loop_blocks, 6, sizeof(LLVMBasicBlockRef));

    defer_blocks = calloc(1, sizeof(t_vector));
    if (NULL == defer_blocks)
    {
        exit(LUKA_CODEGEN_ERROR);
    }
    (void) vector_setup(defer_blocks, 6, sizeof(t_ast_node));
}

void GEN_codegen_reset()
{
    t_struct_info *struct_info = NULL, *struct_info_iter = NULL;
    t_enum_info *enum_info = NULL, *enum_info_iter = NULL;

    gen_named_values_clear();

    HASH_ITER(hh, struct_infos, struct_info, struct_info_iter)
    {
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

    HASH_ITER(hh, enum_infos, enum_info, enum_info_iter)
    {
        HASH_DEL(enum_infos, enum_info);
        if (NULL != enum_info)
        {
            if (NULL != enum_info->enum_name)
            {
                (void) free((char *) enum_info->enum_name);
                enum_info->enum_name = NULL;
            }

            if (NULL != enum_info->enum_field_names)
            {
                for (size_t i = 0; i < enum_info->number_of_fields; ++i)
                {
                    if (NULL != enum_info->enum_field_names[i])
                    {
                        (void) free(enum_info->enum_field_names[i]);
                        enum_info->enum_field_names[i] = NULL;
                    }
                }
                (void) free(enum_info->enum_field_names);
                enum_info->enum_field_names = NULL;
            }

            if (NULL != enum_info->enum_field_values)
            {
                (void) free(enum_info->enum_field_values);
                enum_info->enum_field_values = NULL;
            }
            (void) free(enum_info);
            enum_info = NULL;
        }
    }

    if (NULL != loop_blocks)
    {
        (void) vector_clear(loop_blocks);
        (void) vector_destroy(loop_blocks);
        (void) free(loop_blocks);
    }

    if (NULL != defer_blocks)
    {
        (void) vector_clear(defer_blocks);
        (void) vector_destroy(defer_blocks);
        (void) free(defer_blocks);
    }
}
