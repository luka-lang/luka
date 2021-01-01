#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
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
#include "main_internal.h"


#define DEFAULT_LOG_PATH ("/tmp/luka.log")
#define OUT_FILENAME     ("./a.out")
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

static void print_help(void)
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


static void context_initialize(t_main_context *context, int argc, char **argv)
{
    context->argc = argc;
    context->argv = argv;
    context->file_path = NULL;
    context->file_contents = NULL;
    context->tokens = NULL;
    context->module = NULL;
    context->parser = NULL;
    context->node = NULL;
    context->llvm_module = NULL;
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
    context->bitcode = false;
    context->optimization = DEFAULT_OPT;
    context->compile = true;
    context->assemble = true;
    context->link = true;
}

static void context_destruct(t_main_context *context)
{
    if (NULL != context->tokens)
    {
        (void) LIB_free_tokens_vector(context->tokens);
        context->tokens = NULL;
    }

    if (NULL != context->module)
    {
        (void) LIB_free_module(context->module, context->logger);
        context->module = NULL;
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

    if (NULL != context->llvm_module)
    {
        (void) LLVMDisposeModule(context->llvm_module);
        context->llvm_module = NULL;
    }
}

static t_return_code get_args(t_main_context *context)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    char ch = '\0';

    while (-1 != (ch = getopt_long(context->argc, context->argv, "hvo:bO:t:cS", S_LONG_OPTIONS, NULL)))
    {
        switch (ch)
        {
            case 'h':
                (void) print_help();
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
        (void) print_help();
        status_code = LUKA_WRONG_PARAMETERS;
    }
    else
    {
        context->file_path = context->argv[optind++];
        status_code = LUKA_SUCCESS;
    }

    return status_code;
}

t_return_code lex(t_main_context *context, const char * file_path)
{
    t_return_code status_code = LUKA_UNINITIALIZED;

    context->file_contents = IO_get_file_contents(file_path);
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

    RAISE_LUKA_STATUS_ON_ERROR(LEXER_tokenize_source(context->tokens, context->file_contents, context->logger), status_code, l_cleanup);

    status_code = LUKA_SUCCESS;

l_cleanup:
    return status_code;
}

static t_return_code parse(t_main_context *context, const char * file_path)
{
    t_return_code status_code = LUKA_UNINITIALIZED;

    context->parser = calloc(1, sizeof(t_parser));
    if (NULL == context->parser)
    {
       (void) LOGGER_log(context->logger, L_ERROR, "Failed allocating memory for parser.\n");
       status_code = LUKA_CANT_ALLOC_MEMORY;
       return status_code;
    }

    (void) PARSER_initialize(context->parser, context->tokens, file_path, context->logger);

    (void) PARSER_print_parser_tokens(context->parser);

    context->module = PARSER_parse_file(context->parser);
    if (NULL == context->module)
    {
        status_code = LUKA_PARSER_FAILED;
        return status_code;
    }

    VECTOR_FOR_EACH(context->module->functions, iterator)
    {
        context->node = ITERATOR_GET_AS(t_ast_node_ptr, &iterator);
        context->node = AST_fix_function_last_expression_stmt(context->node);
    }

    (void) AST_print_functions(context->module->functions, 0, context->logger);

    status_code = LUKA_SUCCESS;
    return status_code;
}

static t_return_code initialize_llvm(t_main_context *context, const char * file_path)
{
    t_return_code status_code = LUKA_UNINITIALIZED;

    context->llvm_module = LLVMModuleCreateWithName(file_path);
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
    (void) LLVMSetTarget(context->llvm_module, context->triple);
    context->target_data = LLVMCreateTargetDataLayout(context->target_machine);
    (void) LLVMSetModuleDataLayout(context->llvm_module, context->target_data);
    (void) LLVMDisposeMessage(context->triple);
    context->builder = LLVMCreateBuilder();

    status_code = LUKA_SUCCESS;
    return status_code;
}

static t_return_code codegen_nodes(t_main_context *context, t_vector *nodes)
{
    VECTOR_FOR_EACH(nodes, iterator)
    {
        context->node = ITERATOR_GET_AS(t_ast_node_ptr, &iterator);
        context->value = GEN_codegen(context->node, context->llvm_module, context->builder, context->logger);
        if ((NULL == context->value) && (AST_TYPE_STRUCT_DEFINITION != context->node->type) && (AST_TYPE_ENUM_DEFINITION != context->node->type))
        {
            (void) LOGGER_log(context->logger,
                              L_ERROR,
                              "Failed generating code for function %s.\n",
                              context->node->function.prototype->prototype.name);
            context->value = LLVMGetNamedFunction(context->llvm_module, context->node->function.prototype->prototype.name);
            if (NULL == context->value)
            {
                (void) LOGGER_log(context->logger, L_ERROR, "Failed finding the function inside the module.\n");
            }
            else
            {
                (void) LLVMDeleteFunction(context->value);
            }

            return LUKA_CODEGEN_ERROR;
        }
    }

    return LUKA_SUCCESS;
}

static t_return_code code_generation(t_main_context *context)
{
    t_return_code status_code = LUKA_UNINITIALIZED;

    (void) GEN_codegen_initialize();

    RAISE_LUKA_STATUS_ON_ERROR(codegen_nodes(context, context->module->structs), status_code, l_cleanup);
    RAISE_LUKA_STATUS_ON_ERROR(codegen_nodes(context, context->module->enums), status_code, l_cleanup);
    RAISE_LUKA_STATUS_ON_ERROR(codegen_nodes(context, context->module->functions), status_code, l_cleanup);

    (void) GEN_codegen_reset();

    (void) LLVMVerifyModule(context->llvm_module, LLVMAbortProcessAction, &context->error);
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

static void add_o1_optimizations(t_main_context *context)
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

static t_return_code optimize(t_main_context *context)
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
        (void) add_o1_optimizations(context);
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
    if (!LLVMRunPassManager(context->pass_manager, context->llvm_module))
    {
        status_code = LUKA_GENERAL_ERROR;
    }

    status_code = LUKA_SUCCESS;
    return status_code;
}

static t_return_code generate_output(t_main_context *context)
{
    t_return_code status_code = LUKA_UNINITIALIZED;

    if (context->verbosity > 0)
    {
        (void) LLVMDumpModule(context->llvm_module);
    }

    if (context->bitcode)
    {
        (void) LLVMWriteBitcodeToFile(context->llvm_module, context->output_path);
    }
    else
    {
        char object_template[] = "/tmp/XXXXXX.o";
        char asm_template[] = "/tmp/XXXXXX.S";
        int object_fd = -1;
        int asm_fd = -1;

        if (context->assemble)
        {
            object_fd = mkstemps(object_template, 2);
            if (-1 == object_fd)
            {
                status_code = LUKA_GENERAL_ERROR;
                goto l_cleanup;
            }

            if(LLVMTargetMachineEmitToFile(context->target_machine, context->llvm_module, object_template, LLVMObjectFile, &context->error))
            {
                status_code = LUKA_LLVM_ERROR;
                goto l_cleanup;
            }

            ON_ERROR(close(object_fd))
            {
                status_code = LUKA_IO_ERROR;
                goto l_cleanup;
            }
        }
        else
        {
            asm_fd = mkstemps(asm_template, 2);
            if (-1 == asm_fd)
            {
                status_code = LUKA_GENERAL_ERROR;
                goto l_cleanup;
            }

            if(LLVMTargetMachineEmitToFile(context->target_machine, context->llvm_module, asm_template, LLVMAssemblyFile, &context->error))
            {
                status_code = LUKA_LLVM_ERROR;
                goto l_cleanup;
            }

            ON_ERROR(close(asm_fd))
            {
                status_code = LUKA_IO_ERROR;
                goto l_cleanup;
            }
        }

        if (NULL != context->error)
        {
            (void) LOGGER_log(context->logger, L_ERROR, "Error while emitting file: %s\n", context->error);
            status_code = LUKA_GENERAL_ERROR;
            return status_code;
        }

        if (context->link)
        {
            char *args[] = {"gcc", "-o", context->output_path, object_template, NULL};
            pid_t pid = fork();
            if (0 == pid)
            {
                (void) execvp(args[0], args);
            }
            else
            {
                (void) wait(NULL);
                (void) unlink(object_template);
            }
        }
        else
        {
            if (context->assemble)
            {
                ON_ERROR(rename(object_template, context->output_path))
                {
                    RAISE_LUKA_STATUS_ON_ERROR(IO_copy(object_template, context->output_path), status_code, l_cleanup);
                }
            }
            else
            {
                ON_ERROR(rename(asm_template, context->output_path))
                {
                    RAISE_LUKA_STATUS_ON_ERROR(IO_copy(asm_template, context->output_path), status_code, l_cleanup);
                }
            }
        }
    }

    status_code = LUKA_SUCCESS;
l_cleanup:
    return status_code;
}

static t_return_code frontend(t_main_context * context, const char * file_path)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    RAISE_LUKA_STATUS_ON_ERROR(lex(context, file_path), status_code, l_cleanup);

    if (NULL != context->file_contents) {
      (void) free(context->file_contents);
      context->file_contents = NULL;
    }

    RAISE_LUKA_STATUS_ON_ERROR(parse(context, file_path), status_code, l_cleanup);

    status_code = LUKA_SUCCESS;

 l_cleanup:
    return status_code;
}

static t_return_code backend(t_main_context *context, const char *file_path)
{
  t_return_code status_code = LUKA_UNINITIALIZED;
  RAISE_LUKA_STATUS_ON_ERROR(initialize_llvm(context, file_path),
                             status_code, l_cleanup);
  RAISE_LUKA_STATUS_ON_ERROR(code_generation(context), status_code,
                             l_cleanup);
  RAISE_LUKA_STATUS_ON_ERROR(optimize(context), status_code, l_cleanup);
  RAISE_LUKA_STATUS_ON_ERROR(generate_output(context), status_code,
                             l_cleanup);

  status_code = LUKA_SUCCESS;

l_cleanup:
  return status_code;
}

static t_return_code do_file(t_main_context *context, const char *file_path)
{
  t_return_code status_code = LUKA_UNINITIALIZED;
  RAISE_LUKA_STATUS_ON_ERROR(frontend(context, file_path), status_code, l_cleanup);
  RAISE_LUKA_STATUS_ON_ERROR(backend(context, file_path), status_code, l_cleanup);

  status_code = LUKA_SUCCESS;

l_cleanup:
  return status_code;
}

int main(int argc, char **argv)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    t_main_context context;

    (void) LLVMInitializeCore(LLVMGetGlobalPassRegistry());
    (void) LLVMInitializeAllTargets();
    (void) LLVMInitializeAllTargetInfos();
    (void) LLVMInitializeAllTargetMCs();
    (void) LLVMInitializeAllAsmPrinters();
    (void) LLVMInitializeAllAsmParsers();

    (void) context_initialize(&context, argc, argv);

    RAISE_LUKA_STATUS_ON_ERROR(get_args(&context), status_code, l_cleanup);

    context.logger = LOGGER_initialize(DEFAULT_LOG_PATH, context.verbosity);


    RAISE_LUKA_STATUS_ON_ERROR(do_file(&context, context.file_path), status_code, l_cleanup);

    status_code = LUKA_SUCCESS;

l_cleanup:
    (void) context_destruct(&context);
    (void) LLVMResetFatalErrorHandler();
    (void) LLVMShutdown();

    return status_code;
}
