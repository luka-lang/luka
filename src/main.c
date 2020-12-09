#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Transforms/InstCombine.h>
#include <llvm-c/Transforms/PassManagerBuilder.h>
#include <llvm-c/Transforms/IPO.h>
#include <llvm-c/Transforms/Vectorize.h>
#include <llvm-c/Transforms/Coroutines.h>
#include <llvm-c/Transforms/AggressiveInstCombine.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Utils.h>

#include "ast.h"
#include "gen.h"
#include "io.h"
#include "lexer.h"
#include "lib.h"
#include "logger.h"
#include "parser.h"


#define DEFAULT_LOG_PATH ("/tmp/luka.log")
#define BITCODE_FILENAME ("a.out.bc")
#define OBJECT_FILENAME  ("a.out.o")
#define ASM_FILENAME     ("a.out.S")
#define OUT_FILENAME     ("a.out")
#define DEFAULT_OPT      ('3')
#define CMD_LEN          (14 + 4096)

static struct option S_LONG_OPTIONS[] =
{
    {"help", no_argument, NULL, 'h'},
    {"verbose", no_argument, NULL, 'v'},
    {"output", required_argument, NULL, 'o'},
    {"bitcode", no_argument, NULL, 'b'},
    {"optimization", required_argument, NULL, 'O'},
    {"triple", required_argument, NULL, 't'},
    {NULL, no_argument, NULL, 'c'},
    {NULL, no_argument, NULL, 'S'},
    {NULL, 0, NULL, 0}
};

/**
 * @brief Print how to use the executable and meaning of different arguments.
 */
void main_print_help(void)
{
    (void) printf(
        "OVERVIEW: luka LLVM compiler\n"
        "\n"
        "USAGE: luka [options] file\n"
        "\n"
        "OPTIONS:\n"
        "  -h/--help\t\tDisplay this help.\n"
        "  -o/--output\t\tOutput file path (a.out by default)\n"
        "  -v/--verbose\t\tIncrease verbosity level.\n"
        "  -b/--bitcode\t\tDon't compile bitcode to native machine code.\n"
        "  -O/--optimization\tOptimization level (-O0 for no optimization).\n"
        "  \t\t\tOptimization levels: 0, 1, 2, 3, s (optimize for space)\n"
        "  -t/--triple\tThe LLVM Target to codegen for.\n"
        "  -c\t\tCompile and assemble, but do not link.\n"
        "  -S\t\tCompile only; do not assemble or link.\n"
        "\n"
    );
}

typedef struct
{
    int argc;
    char **argv;
    char *file_path;
    char *file_contents;
    t_vector *tokens;
    t_vector *functions;
    t_parser *parser;
    t_ast_node *function;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    LLVMPassManagerRef pass_manager;
    LLVMValueRef value;
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
 * @brief Initialize the main context.
 *
 * @param[in,out] context the context to initialize.
 * @param[in] argc the number of arguments to the executable.
 * @param[in] argv the arguments to the executable.
 */
void main_context_initialize(t_main_context *context, int argc, char **argv)
{
    context->argc = argc;
    context->argv = argv;
    context->file_path = NULL;
    context->file_contents = NULL;
    context->tokens = NULL;
    context->functions = NULL;
    context->parser = NULL;
    context->function = NULL;
    context->module = NULL;
    context->builder = NULL;
    context->pass_manager = NULL;
    context->value = NULL;
    context->target_machine = NULL;
    context->target = NULL;
    context->target_data = NULL;
    context->triple = NULL;
    context->error = NULL;
    context->logger = NULL;
    context->verbosity = 0;
    context->output_path = OUT_FILENAME;
    context->cmd = NULL;
    context->bitcode = false;
    context->optimization = DEFAULT_OPT;
    context->compile = true;
    context->assemble = true;
    context->link = true;
}

/**
 * @brief Destruct the main context, deallocates all memory allocated in the context.
 *
 * @param[in,out] context the context to free.
 */
void main_context_destruct(t_main_context *context)
{
    if (NULL != context->cmd) {
        (void) free(context->cmd);
        context->cmd = NULL;
    }

    if (NULL != context->tokens)
    {
        (void) LIB_free_tokens_vector(context->tokens);
        context->tokens = NULL;
    }

    if (NULL != context->functions)
    {
        (void) LIB_free_functions_vector(context->functions, context->logger);
        context->functions = NULL;
    }

    if (NULL != context->logger)
    {
        (void) LOGGER_free(context->logger);
        context->logger = NULL;
    }

    if (NULL != context->file_contents)
    {
        (void) free(context->file_contents);
        context->file_contents = NULL;
    }

    if (NULL != context->parser)
    {
        (void) PARSER_free(context->parser);
        (void) free(context->parser);
        context->parser = NULL;
    }

    if (NULL != context->target_data)
    {
        (void) LLVMDisposeTargetData(context->target_data);
        context->target_data = NULL;
    }

    if (NULL != context->target_machine)
    {
        (void) LLVMDisposeTargetMachine(context->target_machine);
        context->target_machine = NULL;
    }

    if (NULL != context->pass_manager)
    {
        (void) LLVMDisposePassManager(context->pass_manager);
        context->pass_manager = NULL;
    }

    if (NULL != context->builder)
    {
        (void) LLVMDisposeBuilder(context->builder);
        context->builder = NULL;
    }

    if (NULL != context->module)
    {
        (void) LLVMDisposeModule(context->module);
        context->module = NULL;
    }
}

/**
 * @brief Parse the executable arguments into the context.
 *
 * @param[in,out] context the context to put the arguments into.
 *
 * @return
 * - LUKA_SUCCESS if everything went fine.
 * - LUKA_WRONG_PARAMETERS if the arguments got weren't formatted correctly.
 */
t_return_code main_get_args(t_main_context *context)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    char ch = '\0';

    while (-1 != (ch = getopt_long(context->argc, context->argv, "hvo:bO:t:cS", S_LONG_OPTIONS, NULL)))
    {
        switch (ch)
        {
            case 'h':
                (void) main_print_help();
                (void) exit(LUKA_SUCCESS);
            case 'v':
                ++context->verbosity;
                break;
            case 'b':
                context->bitcode = true;
                break;
            case 'o':
                context->output_path = optarg;
                break;
            case 'O':
                context->optimization = optarg[0];
                break;
            case 't':
                context->triple = optarg;
                break;
            case 'c':
                context->link = false;
                break;
            case 'S':
                context->assemble = false;
                context->link = false;
                break;
            case '?':
                break;
            default:
                (void) abort();
        }
    }

    if (optind >= context->argc)
    {
        (void) main_print_help();
        status_code = LUKA_WRONG_PARAMETERS;
    }
    else
    {
        context->file_path = context->argv[optind++];
        status_code = LUKA_SUCCESS;
    }

    return status_code;
}

/**
 * @brief Perform the lexing stage.
 *
 * @param[in,out] context the context use.
 *
 * @return
 * - LUKA_SUCCESS if everything went fine.
 * - LUKA_CANT_ALLOC_MEMORY if the memory for tokens couldn't have been allocated.
 * - LUKA_VECTOR_FAILURE if the tokens vector couldn't have been setup.
 * - LUKA_LEXER_FAILED for problems inside the lexer.
 */
t_return_code main_lex(t_main_context *context)
{
    t_return_code status_code = LUKA_UNINITIALIZED;

    context->file_contents = IO_get_file_contents(context->file_path);
    context->tokens = calloc(1, sizeof(t_vector));
    if (NULL == context->tokens)
    {
        (void) LOGGER_log(context->logger, L_ERROR, "Couldn't allocate memory for tokens vector.");
        status_code = LUKA_CANT_ALLOC_MEMORY;
        return status_code;
    }

    if (vector_setup(context->tokens, 1, sizeof(t_token_ptr)))
    {
        status_code = LUKA_VECTOR_FAILURE;
        return status_code;
    }

    status_code = LEXER_tokenize_source(context->tokens, context->file_contents, context->logger);
    if (LUKA_SUCCESS != status_code)
    {
        return status_code;
    }

    status_code = LUKA_SUCCESS;
    return status_code;
}

/**
 * @brief Perform the parsing stage.
 *
 * @param[in,out] context the context use.
 *
 * @return
 * - LUKA_SUCCESS if everything went fine.
 * - LUKA_CANT_ALLOC_MEMORY if the memory for tokens couldn't have been allocated.
 * - LUKA_VECTOR_FAILURE if the tokens vector couldn't have been setup.
 */
t_return_code main_parse(t_main_context *context)
{
    t_return_code status_code = LUKA_UNINITIALIZED;

    context->parser = calloc(1, sizeof(t_parser));
    if (NULL == context->parser)
    {
       (void) LOGGER_log(context->logger, L_ERROR, "Failed allocating memory for parser.\n");
       status_code = LUKA_CANT_ALLOC_MEMORY;
       return status_code;
    }

    (void) PARSER_initialize(context->parser, context->tokens, context->file_path, context->logger);

    (void) PARSER_print_parser_tokens(context->parser);

    context->functions = PARSER_parse_top_level(context->parser);
    if (NULL == context->functions)
    {
        status_code = LUKA_PARSER_FAILED;
        return status_code;
    }

    VECTOR_FOR_EACH(context->functions, iterator)
    {
        context->function = ITERATOR_GET_AS(t_ast_node_ptr, &iterator);
        context->function = AST_fix_function_last_expression_stmt(context->function);
    }

    (void) AST_print_functions(context->functions, 0, context->logger);

    status_code = LUKA_SUCCESS;
    return status_code;
}

/**
 * @brief Initalize all LLVM related things both in global scope and in context variable.
 *
 * @param[in,out] context the context to use.
 *
 * @return
 * - LUKA_SUCCESS on success.
 * - LUKA_GENERAL_ERROR if couldn't get target from provided triple.
 */
t_return_code main_initialize_llvm(t_main_context *context)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
   
    (void) LLVMInitializeCore(LLVMGetGlobalPassRegistry());
    (void) LLVMInitializeAllTargets();
    (void) LLVMInitializeAllTargetInfos();
    (void) LLVMInitializeAllTargetMCs();
    (void) LLVMInitializeAllAsmPrinters();
    (void) LLVMInitializeAllAsmParsers();

    context->module = LLVMModuleCreateWithName(context->file_path);
    if (NULL == context->triple)
    {
        context->triple = LLVMGetDefaultTargetTriple();
    }
    else
    {
        context->triple = LLVMNormalizeTargetTriple(context->triple);
    }

    if (LLVMGetTargetFromTriple(context->triple, &context->target, &context->error))
    {
        (void) LOGGER_log(context->logger, L_ERROR, "Getting target from triple failed:\n%s\n", context->error);
        (void) LLVMDisposeMessage(context->error);
        status_code = LUKA_GENERAL_ERROR;
        return status_code;
    }

    if (NULL != context->error)
    {
        (void) LLVMDisposeMessage(context->error);
    }

    context->target_machine = LLVMCreateTargetMachine(context->target, context->triple, "", "", LLVMCodeGenLevelDefault, LLVMRelocPIC, LLVMCodeModelDefault);
    (void) LLVMSetTarget(context->module, context->triple);
    context->target_data = LLVMCreateTargetDataLayout(context->target_machine);
    (void) LLVMSetModuleDataLayout(context->module, context->target_data);
    (void) LLVMDisposeMessage(context->triple);
    context->builder = LLVMCreateBuilder();

    status_code = LUKA_SUCCESS;
    return status_code;
}

/**
 * @brief Performs the codegen stage of the compiler.
 *
 * @param[in,out] context the context to use.
 *
 * @return
 * - LUKA_SUCCESS on success.
 * - LUKA_CODEGEN_ERROR if the module couldn't have been verified.
 */
t_return_code main_code_generation(t_main_context *context)
{
    t_return_code status_code = LUKA_UNINITIALIZED;

    (void) GEN_codegen_initialize();

    VECTOR_FOR_EACH(context->functions, iterator)
    {
        context->function = ITERATOR_GET_AS(t_ast_node_ptr, &iterator);
        context->value = GEN_codegen(context->function, context->module, context->builder, context->logger);
        if ((NULL == context->value) && (AST_TYPE_STRUCT_DEFINITION != context->function->type) && (AST_TYPE_ENUM_DEFINITION != context->function->type))
        {
            (void) LOGGER_log(context->logger,
                              L_ERROR,
                              "Failed generating code for function %s.\n",
                              context->function->function.prototype->prototype.name);
            context->value = LLVMGetNamedFunction(context->module, context->function->function.prototype->prototype.name);
            if (NULL == context->value)
            {
                (void) LOGGER_log(context->logger, L_ERROR, "Failed finding the function inside the module.\n");
            }
            else
            {
                (void) LLVMDeleteFunction(context->value);
            }
        }
    }

    (void) GEN_codegen_reset();

    (void) LLVMVerifyModule(context->module, LLVMAbortProcessAction, &context->error);
    if ((NULL != context->error) && (0 != strcmp("", context->error)))
    {
        (void) LOGGER_log(context->logger, L_ERROR, "Couldn't verify module:\n%s\n", context->error);
        (void) LLVMDisposeMessage(context->error);
        context->error = NULL;
        status_code = LUKA_CODEGEN_ERROR;
        goto l_cleanup;
    }

    status_code = LUKA_SUCCESS;

l_cleanup:
    if (NULL != context->error)
    {
        (void) LLVMDisposeMessage(context->error);
        context->error = NULL;
    }

    return status_code;
}

/**
 * @brief Add O1 optimization passes to the pass manager.
 *
 * @param[in,out] context the context to use.
 */
void main_add_o1_optimizations(t_main_context *context)
{
    (void) LLVMAddDeadArgEliminationPass(context->pass_manager);
    (void) LLVMAddCalledValuePropagationPass(context->pass_manager);
    (void) LLVMAddAlignmentFromAssumptionsPass(context->pass_manager);
    (void) LLVMAddFunctionAttrsPass(context->pass_manager);
    (void) LLVMAddInstructionCombiningPass(context->pass_manager);
    (void) LLVMAddCFGSimplificationPass(context->pass_manager);
    (void) LLVMAddEarlyCSEMemSSAPass(context->pass_manager);
    (void) LLVMAddJumpThreadingPass(context->pass_manager);
    (void) LLVMAddCorrelatedValuePropagationPass(context->pass_manager);
    (void) LLVMAddTailCallEliminationPass(context->pass_manager);
    (void) LLVMAddReassociatePass(context->pass_manager);
    (void) LLVMAddLoopRotatePass(context->pass_manager);
    (void) LLVMAddLoopUnswitchPass(context->pass_manager);
    (void) LLVMAddIndVarSimplifyPass(context->pass_manager);
    (void) LLVMAddLoopIdiomPass(context->pass_manager);
    (void) LLVMAddLoopDeletionPass(context->pass_manager);
    (void) LLVMAddLoopUnrollPass(context->pass_manager);
    (void) LLVMAddMemCpyOptPass(context->pass_manager);
    (void) LLVMAddSCCPPass(context->pass_manager);
    (void) LLVMAddBitTrackingDCEPass(context->pass_manager);
    (void) LLVMAddDeadStoreEliminationPass(context->pass_manager);
    (void) LLVMAddAggressiveDCEPass(context->pass_manager);
    (void) LLVMAddGlobalDCEPass(context->pass_manager);
    (void) LLVMAddLoopVectorizePass(context->pass_manager);
    (void) LLVMAddAlignmentFromAssumptionsPass(context->pass_manager);
    (void) LLVMAddStripDeadPrototypesPass(context->pass_manager);
    (void) LLVMAddEarlyCSEPass(context->pass_manager);
    (void) LLVMAddLowerExpectIntrinsicPass(context->pass_manager);
}

/**
 * @brief Optimize the IR before saving it based on the optimization level.
 *
 * @param[in,out] context the context to use.
 *
 * @return
 * - LUKA_SUCCESS on success.
 * - LUKA_GENERAL_ERROR if the pass manager failed optimizing.
 */
t_return_code main_optimize(t_main_context *context)
{
    t_return_code status_code = LUKA_UNINITIALIZED;

    context->pass_manager = LLVMCreatePassManager();

    (void) LLVMAddVerifierPass(context->pass_manager);
    if (('0' == context->optimization) || ('1' == context->optimization))
    {
        (void) LLVMAddAlwaysInlinerPass(context->pass_manager);
    }

    if ('0' != context->optimization)
    {
        (void) main_add_o1_optimizations(context);
        if ('s' != context->optimization)
        {
            (void) LLVMAddSimplifyLibCallsPass(context->pass_manager);
        }

        if ('1' != context->optimization)
        {
            (void) LLVMAddGVNPass(context->pass_manager);
            (void) LLVMAddMergedLoadStoreMotionPass(context->pass_manager);
            if ('2' == context->optimization)
            {
                (void) LLVMAddSLPVectorizePass(context->pass_manager);
            }
            (void) LLVMAddConstantMergePass(context->pass_manager);
        }


        if (('2' != context->optimization) && ('s' != context->optimization))
        {
            (void) LLVMAddArgumentPromotionPass(context->pass_manager);
        }

        (void) LLVMAddConstantPropagationPass(context->pass_manager);
        (void) LLVMAddPromoteMemoryToRegisterPass(context->pass_manager);

    }
    if (!LLVMRunPassManager(context->pass_manager, context->module))
    {
        status_code = LUKA_GENERAL_ERROR;
    }

    status_code = LUKA_SUCCESS;
    return status_code;
}

/**
 * @brief Generate output to the filesystem based on arguments from the user.
 *
 * @param[in,out] context the context to use.
 *
 * @return LUKA_SUCCESS.
 */
t_return_code main_generate_output(t_main_context *context)
{
    t_return_code status_code = LUKA_UNINITIALIZED;

    if (context->verbosity > 0)
    {
        (void) LLVMDumpModule(context->module);
    }

    if (context->bitcode)
    {
        (void) LLVMWriteBitcodeToFile(context->module, context->output_path);
    }
    else
    {
        if (context->assemble)
        {
            (void) LLVMTargetMachineEmitToFile(context->target_machine, context->module, OBJECT_FILENAME, LLVMObjectFile, &context->error);
        }
        else
        {
            (void) LLVMTargetMachineEmitToFile(context->target_machine, context->module, ASM_FILENAME, LLVMAssemblyFile, &context->error);
        }

        if (NULL != context->error)
        {
            (void) LOGGER_log(context->logger, L_ERROR, "Error while emitting file: %s\n", context->error);
            goto l_cleanup;
        }

        if (context->link)
        {
            context->cmd = malloc(CMD_LEN);
            if (NULL == context->cmd)
            {
                status_code = LUKA_CANT_ALLOC_MEMORY;
                goto l_cleanup;
            }

            (void) snprintf(context->cmd, CMD_LEN, "gcc -o \"%s\" %s -lc", context->output_path, OBJECT_FILENAME);
            (void) system(context->cmd);
            (void) snprintf(context->cmd, CMD_LEN, "./%s", OBJECT_FILENAME);
            (void) unlink(context->cmd);
        }
        else
        {
            if (context->assemble)
            {
                (void) rename(OBJECT_FILENAME, context->output_path);
            }
            else
            {
                (void) rename(ASM_FILENAME, context->output_path);
            }
        }
    }

    status_code = LUKA_SUCCESS;

l_cleanup:
    if (NULL != context->cmd)
    {
        (void) free(context->cmd);
        context->cmd = NULL;
    }

    return status_code;
}

int main(int argc, char **argv)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    t_main_context context;

    (void) main_context_initialize(&context, argc, argv);
    status_code = main_get_args(&context);
    if (LUKA_SUCCESS != status_code)
    {
        goto l_cleanup;
    }

    context.logger = LOGGER_initialize(DEFAULT_LOG_PATH, context.verbosity);

    status_code = main_lex(&context);
    if (LUKA_SUCCESS != status_code)
    {
        goto l_cleanup;
    }

    if (NULL != context.file_contents)
    {
        (void) free(context.file_contents);
        context.file_contents = NULL;
    }

    status_code = main_parse(&context);
    if (LUKA_SUCCESS != status_code)
    {
        goto l_cleanup;
    }

    status_code = main_initialize_llvm(&context);
    if (LUKA_SUCCESS != status_code)
    {
        goto l_cleanup;
    }

    status_code = main_code_generation(&context);
    if (LUKA_SUCCESS != status_code)
    {
        goto l_cleanup;
    }

    status_code = main_optimize(&context);
    if (LUKA_SUCCESS != status_code)
    {
        goto l_cleanup;
    }

    status_code = main_generate_output(&context);
    if (LUKA_SUCCESS != status_code)
    {
        goto l_cleanup;
    }

    status_code = LUKA_SUCCESS;

l_cleanup:
    (void) main_context_destruct(&context);
    (void) LLVMResetFatalErrorHandler();
    (void) LLVMShutdown();

    return status_code;
}
