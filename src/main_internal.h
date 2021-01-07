/** @file main_internal.h */
#ifndef __MAIN_INTERNAL_H__
#define __MAIN_INTERNAL_H__

#include "defs.h"
#include "parser.h"
#include <llvm-c/Core.h>
#include <llvm-c/TargetMachine.h>

typedef struct
{
    int argc;
    char **argv;
    char **file_paths;
    size_t files_count;
    size_t file_index;
    t_vector *tokens;
    t_module **modules;
    t_module *current_module;
    t_parser *parser;
    t_ast_node *node;
    LLVMModuleRef llvm_module;
    LLVMBuilderRef builder;
    LLVMPassManagerRef pass_manager;
    LLVMTargetMachineRef target_machine;
    LLVMTargetRef target;
    LLVMTargetDataRef target_data;
    char *triple;
    char *error;
    t_logger *logger;
    size_t verbosity;
    char *output_path;
    char *cmd;
    bool bitcode;
    char optimization;
    bool compile;
    bool assemble;
    bool link;
} t_main_context;

/**
 * @brief Print how to use the executable and meaning of different arguments.
 */
static void print_help(void);

/**
 * @brief Initialize the main context.
 *
 * @param[in,out] context the context to initialize.
 * @param[in] argc the number of arguments to the executable.
 * @param[in] argv the arguments to the executable.
 */
static void context_initialize(t_main_context *context, int argc, char **argv);
/**
 * @brief Destruct the main context, deallocates all memory allocated in the
 * context.
 *
 * @param[in,out] context the context to free.
 */
static void context_destruct(t_main_context *context);

/**
 * @brief Parse the executable arguments into the context.
 *
 * @param[in,out] context the context to put the arguments into.
 *
 * @return
 * - LUKA_SUCCESS if everything went fine.
 * - LUKA_WRONG_PARAMETERS if the arguments got weren't formatted correctly.
 */
static t_return_code get_args(t_main_context *context);

/**
 * @brief Perform the lexing stage.
 *
 * @param[in,out] context the context use.
 * @param[in] file_path the path of the file that should be lexed.
 *
 * @return
 * - LUKA_SUCCESS if everything went fine.
 * - LUKA_CANT_ALLOC_MEMORY if the memory for tokens couldn't have been
 * allocated.
 * - LUKA_VECTOR_FAILURE if the tokens vector couldn't have been setup.
 * - LUKA_LEXER_FAILED for problems inside the lexer.
 */
static t_return_code lex(t_main_context *context, const char *file_path);

/**
 * @brief Perform the parsing stage.
 *
 * @param[in,out] context the context use.
 * @param[in] file_path the path of the file that should be parsed.
 *
 * @return
 * - LUKA_SUCCESS if everything went fine.
 * - LUKA_CANT_ALLOC_MEMORY if the memory for tokens couldn't have been
 * allocated.
 * - LUKA_VECTOR_FAILURE if the tokens vector couldn't have been setup.
 */
static t_return_code parse(t_main_context *context, const char *file_path);

/**
 * @brief Initalize all LLVM related things both in global scope and in context
 * variable.
 *
 * @param[in,out] context the context to use.
 *
 * @return
 * - LUKA_SUCCESS on success.
 * - LUKA_GENERAL_ERROR if couldn't get target from provided triple.
 */
static t_return_code initialize_llvm(t_main_context *context);

/**
 * @brief Generate LLVM IR for the module inside @p context based on the vector
 * AST nodes @p nodes.
 *
 * @param[in,out] context the context to use.
 * @param[in] nodes the vector of nodes to use.
 *
 * @return
 * - LUKA_SUCCESS if everything went fine.
 * - LUKA_CODEGEN_ERROR if had a problem while generating LLVM IR.
 */
static t_return_code codegen_nodes(t_main_context *context, t_vector *nodes);

/**
 * @brief Performs the codegen stage of the compiler.
 *
 * @param[in,out] context the context to use.
 *
 * @return
 * - LUKA_SUCCESS on success.
 * - LUKA_CODEGEN_ERROR if the module couldn't have been verified.
 */
static t_return_code code_generation(t_main_context *context);

/**
 * @brief Add O1 optimization passes to the pass manager.
 *
 * @param[in,out] context the context to use.
 */
static void add_o1_optimizations(t_main_context *context);

/**
 * @brief Optimize the IR before saving it based on the optimization level.
 *
 * @param[in,out] context the context to use.
 *
 * @return
 * - LUKA_SUCCESS on success.
 * - LUKA_GENERAL_ERROR if the pass manager failed optimizing.
 */
static t_return_code optimize(t_main_context *context);

/**
 * @brief Generate output to the filesystem based on arguments from the user.
 *
 * @param[in,out] context the context to use.
 *
 * @return LUKA_SUCCESS.
 */
static t_return_code generate_output(t_main_context *context);

/**
 * @brief Perform all stages of the frontend - lexing and parsing.
 *
 * @param[in,out] context the context to use.
 * @param[in] file_path the path to the file that should be used.
 *
 * @return LUKA_SUCCESS on success or a status from one of stages on failure.
 */
static t_return_code frontend(t_main_context *context, const char *file_path);

/**
 * @brief Perform all stages of the backend - converting the module to an LLVM
 * module, codegen, optimizing and generating output files.
 *
 * @param[in,out] context the context to use.
 *
 * @return LUKA_SUCCESS on success or a status from one of stages on failure.
 */
static t_return_code backend(t_main_context *context);

/**
 * @brief Perform all stages of the compiler on the given file.
 *
 * @param[in,out] context the context to use.
 * @param[in] file_path the path to the file that should be used.
 *
 * @return LUKA_SUCCESS on success or a status from one of stages on failure.
 */
static t_return_code do_file(t_main_context *context, const char *file_path);

#endif
