#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Transforms/AggressiveInstCombine.h>
#include <llvm-c/Transforms/Coroutines.h>
#include <llvm-c/Transforms/IPO.h>
#include <llvm-c/Transforms/InstCombine.h>
#include <llvm-c/Transforms/PassManagerBuilder.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Utils.h>
#include <llvm-c/Transforms/Vectorize.h>
#include <llvm-c/Types.h>

#include "ast.h"
#include "defs.h"
#include "gen.h"
#include "io.h"
#include "lexer.h"
#include "lib.h"
#include "logger.h"
#include "main_internal.h"
#include "parser.h"
#include "type_checker.h"
#include "uthash.h"
#include "vector.h"

#define DEFAULT_LOG_PATH ("/tmp/luka.log")
#define OUT_FILENAME     ("./a.out")
#define DEFAULT_OPT      ('3')
#define CMD_LEN          (14 + 4096)

static struct option S_LONG_OPTIONS[]
    = {{"help", no_argument, NULL, 'h'},
       {"verbose", no_argument, NULL, 'v'},
       {"output", required_argument, NULL, 'o'},
       {"bitcode", no_argument, NULL, 'b'},
       {"optimization", required_argument, NULL, 'O'},
       {"triple", required_argument, NULL, 't'},
       {NULL, no_argument, NULL, 'c'},
       {NULL, no_argument, NULL, 'S'},
       {NULL, 0, NULL, 0}};

static void print_help(void)
{
    (void) printf(
        "OVERVIEW: luka LLVM compiler\n"
        "\n"
        "USAGE: luka [options] file\n"
        "\n"
        "OPTIONS:\n"
        "  -h/--help            Display this help.\n"
        "  -o/--output          Output file path (a.out by default)\n"
        "  -v/--verbose         Increase verbosity level.\n"
        "  -b/--bitcode         Don't compile bitcode to native machine code.\n"
        "  -O/--optimization    Optimization level (-O0 for no optimization).\n"
        "                       Optimization levels: 0, 1, 2, 3, s (optimize "
        "for space)\n"
        "  -t/--triple          The LLVM Target to codegen for.\n"
        "  -c                   Compile and assemble, but do not link.\n"
        "  -S                   Compile only; do not assemble or link.\n"
        "\n");
}

static void context_initialize(t_main_context *context, int argc, char **argv)
{
    context->argc = argc;
    context->argv = argv;
    context->file_paths = NULL;
    context->files_count = 0;
    context->file_index = 0;
    context->tokens = NULL;
    context->modules = NULL;
    context->parser = NULL;
    context->node = NULL;
    context->type_aliases = NULL;
    context->llvm_module = NULL;
    context->builder = NULL;
    context->pass_manager = NULL;
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
    context->imported_modules = NULL;
    context->codegen_modules = NULL;
}

static void context_destruct(t_main_context *context)
{
    size_t i = 0;
    t_imported_module *imported_module = NULL, *imported_module_iter = NULL;

    if (NULL != context->tokens)
    {
        (void) LIB_free_tokens_vector(context->tokens);
        context->tokens = NULL;
    }

    if (NULL != context->modules)
    {
        for (i = 0; i < context->files_count; ++i)
        {
            if (NULL != context->modules[i])
            {
                (void) LIB_free_module(context->modules[i], context->logger);
                context->modules[i] = NULL;
            }
        }
        (void) free(context->modules);
        context->modules = NULL;
    }

    if (NULL != context->imported_modules)
    {
        HASH_ITER(hh, context->imported_modules, imported_module,
                  imported_module_iter)
        {
            HASH_DEL(context->imported_modules, imported_module);
            if (NULL != imported_module)
            {
                (void) free(imported_module);
                imported_module = NULL;
            }
        }
    }

    if (NULL != context->codegen_modules)
    {
        (void) vector_clear(context->codegen_modules);
        (void) vector_destroy(context->codegen_modules);
        (void) free(context->codegen_modules);
        context->codegen_modules = NULL;
    }

    if (NULL != context->file_paths)
    {
        for (i = 0; i < context->files_count; ++i)
        {
            if (NULL != context->file_paths[i])
            {
                (void) free(context->file_paths[i]);
                context->file_paths[i] = NULL;
            }
        }
        (void) free(context->file_paths);
        context->file_paths = NULL;
    }

    if (NULL != context->logger)
    {
        (void) LOGGER_free(context->logger);
        context->logger = NULL;
    }

    if (NULL != context->parser)
    {
        (void) PARSER_free(context->parser);
        (void) free(context->parser);
        context->parser = NULL;
    }

    if (NULL != context->type_aliases)
    {
        (void) LIB_free_type_aliases_vector(context->type_aliases);
        context->type_aliases = NULL;
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
    size_t i = 0;

    while (-1
           != (ch = getopt_long(context->argc, context->argv, "hvo:bO:t:cS",
                                S_LONG_OPTIONS, NULL)))
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
        goto l_cleanup;
    }

    context->files_count = context->argc - optind;
    context->file_paths = calloc(context->files_count, sizeof(char **));
    if (NULL == context->file_paths)
    {
        status_code = LUKA_CANT_ALLOC_MEMORY;
        goto l_cleanup;
    }

    for (i = 0; i < context->files_count; ++i)
    {
        context->file_paths[i]
            = IO_resolve_path(context->argv[optind++], ".", false);
        if (NULL == context->file_paths[i])
        {
            status_code = LUKA_NON_EXISTING_FILE;
            goto l_cleanup;
        }
    }

    context->modules = calloc(context->files_count, sizeof(t_module **));
    if (NULL == context->modules)
    {
        status_code = LUKA_CANT_ALLOC_MEMORY;
        goto l_cleanup;
    }

    status_code = LUKA_SUCCESS;
    return status_code;

l_cleanup:
    if (NULL != context->file_paths)
    {
        (void) free(context->file_paths);
        context->file_paths = NULL;
    }

    if (NULL != context->modules)
    {
        (void) free(context->modules);
        context->modules = NULL;
    }

    return status_code;
}

t_return_code lex(t_main_context *context, const char *file_path)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    char *file_contents = NULL;

    file_contents = IO_get_file_contents(file_path);
    context->tokens = calloc(1, sizeof(t_vector));
    if (NULL == context->tokens)
    {
        (void) LOGGER_log(context->logger, L_ERROR,
                          "Couldn't allocate memory for tokens vector.");
        status_code = LUKA_CANT_ALLOC_MEMORY;
        return status_code;
    }

    if (vector_setup(context->tokens, 1, sizeof(t_token_ptr)))
    {
        status_code = LUKA_VECTOR_FAILURE;
        return status_code;
    }

    RAISE_LUKA_STATUS_ON_ERROR(
        LEXER_tokenize_source(context->tokens, file_contents, context->logger,
                              file_path),
        status_code, l_cleanup);

    status_code = LUKA_SUCCESS;

l_cleanup:
    if (NULL != file_contents)
    {
        (void) free(file_contents);
        file_contents = NULL;
    }

    return status_code;
}

static t_return_code parse(t_main_context *context, const char *file_path)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    t_module *module = NULL;
    char *resolved_path = NULL;
    t_imported_module *imported_module = NULL;
    size_t i = 0;
    bool original_module = false;

    context->parser = calloc(1, sizeof(t_parser));
    if (NULL == context->parser)
    {
        (void) LOGGER_log(context->logger, L_ERROR,
                          "Failed allocating memory for parser.\n");
        status_code = LUKA_CANT_ALLOC_MEMORY;
        return status_code;
    }

    (void) PARSER_initialize(context->parser, context->tokens, file_path,
                             context->logger, context->type_aliases);

    (void) PARSER_print_parser_tokens(context->parser);

    module = PARSER_parse_file(context->parser);
    if (NULL == module)
    {
        status_code = LUKA_PARSER_FAILED;
        return status_code;
    }

    if (NULL == context->modules[context->file_index])
    {
        /* Making sure not to set this for modules imported from this module. */
        context->modules[context->file_index] = module;
        original_module = true;
    }

    VECTOR_FOR_EACH(module->functions, iterator)
    {
        context->node = ITERATOR_GET_AS(t_ast_node_ptr, &iterator);
        context->node = AST_fix_function_last_expression_stmt(context->node);
        (void) AST_fill_parameter_types(context->node, context->logger);
        (void) AST_fill_variable_types(context->node, context->logger, module);
    }

    (void) AST_print_functions(module->functions, 0, context->logger);

    VECTOR_FOR_EACH(module->import_paths, iterator)
    {
        resolved_path = ITERATOR_GET_AS(t_char_ptr, &iterator);
        (void) LOGGER_log(context->logger, "Importing file %s\n",
                          resolved_path);

        imported_module = NULL;
        HASH_FIND_STR(context->imported_modules, resolved_path,
                      imported_module);
        if (NULL != imported_module)
        {
            /* Already imported */
            context->current_module = imported_module->module;
        }
        else
        {
            /* Try solving circular imports */
            context->current_module = NULL;
            for (i = 0; i <= context->file_index; ++i)
            {
                if (0 == strcmp(resolved_path, context->modules[i]->file_path))
                {
                    context->current_module = context->modules[i];
                }
            }

            if (NULL == context->current_module)
            {
                /* Not imported yet and not found in other modules */
                RAISE_LUKA_STATUS_ON_ERROR(frontend(context, resolved_path),
                                           status_code, l_cleanup);
            }
        }

        vector_push_back(module->imports, &(context->current_module));

        imported_module = NULL;
        HASH_FIND_STR(context->imported_modules, file_path, imported_module);
        if (NULL == imported_module)
        {
            imported_module = malloc(sizeof(t_imported_module));
            if (NULL == imported_module)
            {
                (void) LOGGER_log(
                    context->logger, L_ERROR,
                    "Failed allocating memory for imported module.\n");
                status_code = LUKA_CANT_ALLOC_MEMORY;
                goto l_cleanup;
            }

            imported_module->module = context->current_module;
            imported_module->file_path = resolved_path;

            HASH_ADD_STR(context->imported_modules, file_path, imported_module);
        }
    }

    VECTOR_FOR_EACH(module->structs, iterator)
    {
        context->node = ITERATOR_GET_AS(t_ast_node_ptr, &iterator);
        context->node = AST_resolve_type_aliases(
            context->node, context->type_aliases, context->logger);
    }

    VECTOR_FOR_EACH(module->functions, iterator)
    {
        context->node = ITERATOR_GET_AS(t_ast_node_ptr, &iterator);
        context->node = AST_resolve_type_aliases(
            context->node, context->type_aliases, context->logger);
    }

    if (original_module)
    {
        context->modules[context->file_index] = module;
    }
    context->current_module = module;

    status_code = LUKA_SUCCESS;
l_cleanup:
    return status_code;
}

static t_return_code type_check(t_main_context *context)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    t_module *module = context->current_module;
    t_ast_node *node = NULL;
    bool success = false;

    VECTOR_FOR_EACH(module->functions, iterator)
    {
        node = ITERATOR_GET_AS(t_ast_node_ptr, &iterator);
        success = CHECK_function(module, node, context->logger);
        if (!success)
        {
            status_code = LUKA_TYPE_CHECK_ERROR;
            goto l_cleanup;
        }
    }

    status_code = LUKA_SUCCESS;
l_cleanup:
    return status_code;
}

static t_return_code initialize_llvm(t_main_context *context)
{
    t_return_code status_code = LUKA_UNINITIALIZED;

    (void) LLVMInitializeCore(LLVMGetGlobalPassRegistry());
    (void) LLVMInitializeAllTargets();
    (void) LLVMInitializeAllTargetInfos();
    (void) LLVMInitializeAllTargetMCs();
    (void) LLVMInitializeAllAsmPrinters();
    (void) LLVMInitializeAllAsmParsers();

    context->llvm_module = LLVMModuleCreateWithName("");
    if (NULL == context->triple)
    {
        context->triple = LLVMGetDefaultTargetTriple();
    }
    else
    {
        context->triple = LLVMNormalizeTargetTriple(context->triple);
    }

    if (LLVMGetTargetFromTriple(context->triple, &context->target,
                                &context->error))
    {
        (void) LOGGER_log(context->logger, L_ERROR,
                          "Getting target from triple failed:\n%s\n",
                          context->error);
        (void) LLVMDisposeMessage(context->error);
        status_code = LUKA_GENERAL_ERROR;
        return status_code;
    }

    if (NULL != context->error)
    {
        (void) LLVMDisposeMessage(context->error);
    }

    context->target_machine = LLVMCreateTargetMachine(
        context->target, context->triple, "", "", LLVMCodeGenLevelDefault,
        LLVMRelocPIC, LLVMCodeModelDefault);
    (void) LLVMSetTarget(context->llvm_module, context->triple);
    context->target_data = LLVMCreateTargetDataLayout(context->target_machine);
    (void) LLVMSetModuleDataLayout(context->llvm_module, context->target_data);
    (void) LLVMDisposeMessage(context->triple);
    context->triple = NULL;
    context->builder = LLVMCreateBuilder();

    status_code = LUKA_SUCCESS;
    return status_code;
}

static t_return_code codegen_nodes(t_main_context *context, t_vector *nodes)
{
    bool is_function = false, is_prototype = false;
    LLVMValueRef value = NULL;
    char *function_name = NULL;

    VECTOR_FOR_EACH(nodes, iterator)
    {
        context->node = ITERATOR_GET_AS(t_ast_node_ptr, &iterator);
        is_function = AST_TYPE_FUNCTION == context->node->type;
        is_prototype = is_function && context->node->function.body == NULL;
        if (is_function)
        {
            function_name = context->node->function.prototype->prototype.name;
            value = LLVMGetNamedFunction(context->llvm_module, function_name);
            if (NULL != value)
            {
                if ((0 == strcmp(function_name, "main"))
                    && (0 != LLVMCountBasicBlocks(value)))
                {
                    (void) LOGGER_log(context->logger, L_ERROR,
                                      "Cannot redefine main.\n");
                    return LUKA_CODEGEN_ERROR;
                }
                else if (!is_prototype)
                {
                    (void) LOGGER_log(context->logger, L_WARNING,
                                      "Redefining function %s.\n",
                                      function_name);
                }
                else
                {
                    /* Don't have to generate code for the same prototype */
                    continue;
                }
            }
        }
        value = GEN_codegen(context->node, context->llvm_module,
                            context->builder, context->logger);

        if ((NULL == value) && is_function)
        {
            (void) LOGGER_log(
                context->logger, L_ERROR,
                "Failed generating code for function %s.\n",
                context->node->function.prototype->prototype.name);
            value = LLVMGetNamedFunction(
                context->llvm_module,
                context->node->function.prototype->prototype.name);
            if (NULL == value)
            {
                (void) LOGGER_log(
                    context->logger, L_ERROR,
                    "Failed finding the function inside the module.\n");
            }
            else
            {
                (void) LLVMDeleteFunction(value);
            }

            return LUKA_CODEGEN_ERROR;
        }
    }

    return LUKA_SUCCESS;
}

static t_return_code code_generation(t_main_context *context)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    t_module *module = NULL;

    module = context->current_module;

    RAISE_LUKA_STATUS_ON_ERROR(codegen_nodes(context, module->variables),
                               status_code, l_cleanup);

    RAISE_LUKA_STATUS_ON_ERROR(codegen_nodes(context, module->structs),
                               status_code, l_cleanup);

    RAISE_LUKA_STATUS_ON_ERROR(codegen_nodes(context, module->enums),
                               status_code, l_cleanup);

    RAISE_LUKA_STATUS_ON_ERROR(codegen_nodes(context, module->functions),
                               status_code, l_cleanup);

    (void) LLVMVerifyModule(context->llvm_module, LLVMAbortProcessAction,
                            &context->error);
    if ((NULL != context->error) && (0 != strcmp("", context->error)))
    {
        (void) LOGGER_log(context->logger, L_ERROR,
                          "Couldn't verify module:\n%s\n", context->error);
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
        (void) LLVMWriteBitcodeToFile(context->llvm_module,
                                      context->output_path);
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

            if (LLVMTargetMachineEmitToFile(
                    context->target_machine, context->llvm_module,
                    object_template, LLVMObjectFile, &context->error))
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

            if (LLVMTargetMachineEmitToFile(context->target_machine,
                                            context->llvm_module, asm_template,
                                            LLVMAssemblyFile, &context->error))
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
            (void) LOGGER_log(context->logger, L_ERROR,
                              "Error while emitting file: %s\n",
                              context->error);
            status_code = LUKA_GENERAL_ERROR;
            return status_code;
        }

        if (context->link)
        {
            char *args[]
                = {"gcc", "-o", context->output_path, object_template, NULL};
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
                    RAISE_LUKA_STATUS_ON_ERROR(
                        IO_copy(object_template, context->output_path),
                        status_code, l_cleanup);
                }
            }
            else
            {
                ON_ERROR(rename(asm_template, context->output_path))
                {
                    RAISE_LUKA_STATUS_ON_ERROR(
                        IO_copy(asm_template, context->output_path),
                        status_code, l_cleanup);
                }
            }
        }
    }

    status_code = LUKA_SUCCESS;
l_cleanup:
    return status_code;
}

static t_return_code frontend(t_main_context *context, const char *file_path)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    RAISE_LUKA_STATUS_ON_ERROR(lex(context, file_path), status_code, l_cleanup);

    RAISE_LUKA_STATUS_ON_ERROR(parse(context, file_path), status_code,
                               l_cleanup);

    RAISE_LUKA_STATUS_ON_ERROR(type_check(context), status_code, l_cleanup);
    status_code = LUKA_SUCCESS;

l_cleanup:
    return status_code;
}

static t_return_code backend(t_main_context *context,
                             const t_module *original_module)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    t_module *module = NULL, current_module = {0};

    (void) GEN_module_prototypes(context->current_module, context->llvm_module,
                                 context->builder, context->logger);

    memcpy(&current_module, context->current_module, sizeof(t_module));
    VECTOR_FOR_EACH(context->current_module->imports, imports)
    {
        module = *(t_module **) iterator_get(&imports);
        if (LIB_module_in_list(context->codegen_modules, module))
        {
            continue;
        }
        context->current_module = module;
        if (NULL == original_module)
        {
            RAISE_LUKA_STATUS_ON_ERROR(backend(context, &current_module),
                                       status_code, l_cleanup);
        }
        else if (0 != strcmp(module->file_path, original_module->file_path))
        {
            RAISE_LUKA_STATUS_ON_ERROR(backend(context, original_module),
                                       status_code, l_cleanup);
        }
        else
        {
            continue;
        }

        (void) vector_push_back(context->codegen_modules, &module);
    }
    memcpy(context->current_module, &current_module, sizeof(t_module));
    RAISE_LUKA_STATUS_ON_ERROR(code_generation(context), status_code,
                               l_cleanup);
    if (!LIB_module_in_list(context->codegen_modules, context->current_module))
    {
        (void) vector_push_back(context->codegen_modules,
                                &(context->current_module));
    }
    status_code = LUKA_SUCCESS;

l_cleanup:
    return status_code;
}

static t_return_code do_file(t_main_context *context, const char *file_path)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    RAISE_LUKA_STATUS_ON_ERROR(frontend(context, file_path), status_code,
                               l_cleanup);
    RAISE_LUKA_STATUS_ON_ERROR(backend(context, NULL), status_code, l_cleanup);

    status_code = LUKA_SUCCESS;

l_cleanup:
    return status_code;
}

int main(int argc, char **argv)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    t_main_context context;

    (void) context_initialize(&context, argc, argv);

    RAISE_LUKA_STATUS_ON_ERROR(get_args(&context), status_code, l_cleanup);

    context.logger = LOGGER_initialize(DEFAULT_LOG_PATH, context.verbosity);

    RAISE_LUKA_STATUS_ON_ERROR(initialize_llvm(&context), status_code,
                               l_cleanup);

    (void) GEN_codegen_initialize();

    (void) LOGGER_log(context.logger, L_INFO, "%zu Files\n",
                      context.files_count);

    context.codegen_modules = malloc(sizeof(t_vector));
    if (NULL == context.codegen_modules)
    {
        status_code = LUKA_CANT_ALLOC_MEMORY;
        goto l_cleanup;
    }

    if (vector_setup(context.codegen_modules, 10, sizeof(t_module *)))
    {
        status_code = LUKA_VECTOR_FAILURE;
        goto l_cleanup;
    }

    for (context.file_index = 0; context.file_index < context.files_count;
         ++context.file_index)
    {
        context.type_aliases = calloc(1, sizeof(t_vector));
        if (NULL == context.type_aliases)
        {
            (void) LOGGER_log(context.logger, L_ERROR,
                              "Couldn't allocate memory for type aliases.");
            status_code = LUKA_CANT_ALLOC_MEMORY;
            goto l_cleanup;
        }

        if (vector_setup(context.type_aliases, 5, sizeof(t_type_alias *)))
        {
            status_code = LUKA_VECTOR_FAILURE;
            goto l_cleanup;
        }

        (void) LOGGER_log(context.logger, L_INFO, "File %zu: %s\n",
                          context.file_index,
                          context.file_paths[context.file_index]);
        RAISE_LUKA_STATUS_ON_ERROR(
            do_file(&context, context.file_paths[context.file_index]),
            status_code, l_cleanup);

        LIB_free_type_aliases_vector(context.type_aliases);
        context.type_aliases = NULL;
    }

    RAISE_LUKA_STATUS_ON_ERROR(optimize(&context), status_code, l_cleanup);
    RAISE_LUKA_STATUS_ON_ERROR(generate_output(&context), status_code,
                               l_cleanup);

    status_code = LUKA_SUCCESS;

l_cleanup:
    (void) context_destruct(&context);
    (void) GEN_codegen_reset();
    (void) LLVMResetFatalErrorHandler();
    (void) LLVMShutdown();

    return status_code;
}
