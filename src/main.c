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


#define BITCODE_FILENAME ("a.out.bc")
#define CMD_LEN          (20 + 256)

static struct option long_options[] =
{
    {"help", no_argument, NULL, 'h'},
    {"verbose", no_argument, NULL, 'v'},
    {"output", required_argument, NULL, 'o'},
    {"bitcode", no_argument, NULL, 'b'},
    {"optimization", required_argument, NULL, 'O'},
    {NULL, 0, NULL, 0}
};

void print_help(void)
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
        "\n"
    );
}

int main(int argc, char **argv)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    char *file_path = NULL;

    char *file_contents = NULL;

    t_vector tokens;
    (void) vector_setup(&tokens, 1, sizeof(t_token_ptr));
    t_vector *functions = NULL;

    t_parser *parser = NULL;

    t_ast_node *function = NULL;

    LLVMModuleRef module = NULL;
    LLVMBuilderRef builder = NULL;
    LLVMPassManagerRef pass_manager = NULL;
    LLVMValueRef value = NULL;
    char *triple = NULL;
    char *error = NULL;

    t_logger *logger = NULL;
    char ch = '\0';
    size_t verbosity = 0;
    char *output_path = "a.out";
    char *cmd = NULL;
    bool bitcode = false;
    char optimization = '3';

    while (-1 != (ch = getopt_long(argc, argv, "hvo:bO:", long_options, NULL)))
    {
        switch (ch)
        {
            case 'h':
                (void) print_help();
                return LUKA_SUCCESS;
            case 'v':
                ++verbosity;
                break;
            case 'b':
                bitcode = true;
                break;
            case 'o':
                output_path = optarg;
                break;
            case 'O':
                optimization = optarg[0];
                break;
            case '?':
                break;
            default:
                abort();
        }
    }

    logger = LOGGER_initialize("/tmp/luka.log", verbosity);

    if (optind >= argc)
    {
        (void) print_help();
        status_code = LUKA_WRONG_PARAMETERS;
        goto cleanup;
    }

    file_path = argv[optind++];
    file_contents = IO_get_file_contents(file_path);

    status_code = LEXER_tokenize_source(&tokens, file_contents, logger);
    if (LUKA_SUCCESS != status_code)
    {
        goto cleanup;
    }

    if (NULL != file_contents)
    {
        (void) free(file_contents);
        file_contents = NULL;
    }

    parser = calloc(1, sizeof(t_parser));
    if (NULL == parser)
    {
       (void) LOGGER_log(logger, L_ERROR, "Failed allocating memory for parser.\n");
       status_code = LUKA_CANT_ALLOC_MEMORY;
       goto cleanup;
    }

    (void) PARSER_initialize(parser, &tokens, file_path, logger);

    (void) PARSER_print_parser_tokens(parser);

    functions = PARSER_parse_top_level(parser);

    (void) AST_print_functions(functions, 0, logger);

    (void) LLVMInitializeCore(LLVMGetGlobalPassRegistry());

    module = LLVMModuleCreateWithName(file_path);
    triple = LLVMGetDefaultTargetTriple();
    (void) LLVMSetTarget(module, triple);
    LLVMDisposeMessage(triple);
    builder = LLVMCreateBuilder();

    (void) GEN_codegen_initialize();

    VECTOR_FOR_EACH(functions, iterator)
    {
        function = ITERATOR_GET_AS(t_ast_node_ptr, &iterator);
        value = GEN_codegen(function, module, builder, logger);
        if (NULL == value)
        {
            (void) LOGGER_log(logger,
                              L_ERROR,
                              "Failed generating code for function %s.\n",
                              function->function.prototype->prototype.name);
            value = LLVMGetNamedFunction(module, function->function.prototype->prototype.name);
            if (NULL == value)
            {
                (void) LOGGER_log(logger, L_ERROR, "Failed finding the function inside the module.\n");
            }
            else
            {
                (void) LLVMDeleteFunction(value);
            }
        }
    }

    (void) GEN_codegen_reset();

    (void) LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
    (void) LLVMDisposeMessage(error);

    pass_manager = LLVMCreatePassManager();
    (void) LLVMAddVerifierPass(pass_manager);
    if (('0' == optimization) || ('1' == optimization))
    {
        (void) LLVMAddAlwaysInlinerPass(pass_manager);
    }

    if ('0' != optimization)
    {
        (void) LLVMAddDeadArgEliminationPass(pass_manager);
        (void) LLVMAddCalledValuePropagationPass(pass_manager);
        (void) LLVMAddAlignmentFromAssumptionsPass(pass_manager);
        (void) LLVMAddFunctionAttrsPass(pass_manager);
        (void) LLVMAddInstructionCombiningPass(pass_manager);
        (void) LLVMAddCFGSimplificationPass(pass_manager);
        (void) LLVMAddEarlyCSEMemSSAPass(pass_manager);
        (void) LLVMAddJumpThreadingPass(pass_manager);
        (void) LLVMAddCorrelatedValuePropagationPass(pass_manager);
        (void) LLVMAddTailCallEliminationPass(pass_manager);
        (void) LLVMAddReassociatePass(pass_manager);
        (void) LLVMAddLoopRotatePass(pass_manager);
        (void) LLVMAddLoopUnswitchPass(pass_manager);
        (void) LLVMAddIndVarSimplifyPass(pass_manager);
        (void) LLVMAddLoopIdiomPass(pass_manager);
        (void) LLVMAddLoopDeletionPass(pass_manager);
        (void) LLVMAddLoopUnrollPass(pass_manager);
        (void) LLVMAddMemCpyOptPass(pass_manager);
        (void) LLVMAddSCCPPass(pass_manager);
        (void) LLVMAddBitTrackingDCEPass(pass_manager);
        (void) LLVMAddDeadStoreEliminationPass(pass_manager);
        (void) LLVMAddAggressiveDCEPass(pass_manager);
        (void) LLVMAddGlobalDCEPass(pass_manager);
        (void) LLVMAddLoopVectorizePass(pass_manager);
        (void) LLVMAddAlignmentFromAssumptionsPass(pass_manager);
        (void) LLVMAddStripDeadPrototypesPass(pass_manager);
        (void) LLVMAddEarlyCSEPass(pass_manager);
        (void) LLVMAddLowerExpectIntrinsicPass(pass_manager);
        if ('s' != optimization)
        {
            (void) LLVMAddSimplifyLibCallsPass(pass_manager);
        }

        if ('1' != optimization)
        {
            (void) LLVMAddGVNPass(pass_manager);
            (void) LLVMAddMergedLoadStoreMotionPass(pass_manager);
            if ('2' == optimization)
            {
                (void) LLVMAddSLPVectorizePass(pass_manager);
            }
            (void) LLVMAddConstantMergePass(pass_manager);
        }


        if (('2' != optimization) && ('s' != optimization))
        {
            (void) LLVMAddArgumentPromotionPass(pass_manager);
        }


        (void) LLVMAddConstantPropagationPass(pass_manager);
        (void) LLVMAddPromoteMemoryToRegisterPass(pass_manager);

    }
    (void) LLVMRunPassManager(pass_manager, module);

    if (verbosity > 0)
    {
        (void) LLVMDumpModule(module);
    }

    if (bitcode)
    {
        (void) LLVMWriteBitcodeToFile(module, output_path);
    }
    else
    {
        (void) LLVMWriteBitcodeToFile(module, "a.out.bc");
        cmd = malloc(CMD_LEN);
        if (NULL != cmd) {
            (void) snprintf(cmd, CMD_LEN, "clang -o \"%s\" %s", output_path, BITCODE_FILENAME);
            (void) system(cmd);
            (void) snprintf(cmd, CMD_LEN, "./%s", BITCODE_FILENAME);
            (void) unlink(cmd);
        }
    }

    status_code = LUKA_SUCCESS;

cleanup:
    if (NULL != cmd) {
        (void) free(cmd);
        cmd = NULL;
    }

    (void) LIB_free_tokens_vector(&tokens);

    if (NULL != functions)
    {
        (void) LIB_free_functions_vector(functions, logger);
    }

    if (NULL != logger)
    {
        (void) LOGGER_free(logger);
        logger = NULL;
    }
   
    if (NULL != file_contents)
    {
        (void) free(file_contents);
        file_contents = NULL;
    }

    if (NULL != parser)
    {
        (void) free(parser);
        parser = NULL;
    }

    if (NULL != pass_manager)
    {
        (void) LLVMDisposePassManager(pass_manager);
    }

    if (NULL != builder)
    {
        (void) LLVMDisposeBuilder(builder);
    }

    if (NULL != module)
    {
        (void) LLVMDisposeModule(module);
    }

    (void) LLVMResetFatalErrorHandler();
    (void) LLVMShutdown();

    return status_code;
}
