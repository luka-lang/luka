#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/InstCombine.h>
#include <llvm-c/Transforms/PassManagerBuilder.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Utils.h>

#include "ast.h"
#include "gen.h"
#include "io.h"
#include "lexer.h"
#include "lib.h"
#include "parser.h"

int main(int argc, char **argv)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    char *file_path = NULL;

    int file_size = 0;
    char *file_contents = NULL;

    t_vector tokens;
    t_vector *functions = NULL;
    t_token *token = NULL;

    t_parser *parser = NULL;

    t_ast_node *function = NULL;

    LLVMModuleRef module = NULL;
    LLVMBuilderRef builder = NULL;
    LLVMPassManagerRef pass_manager = NULL;
    LLVMValueRef value = NULL;

    char *error = NULL;

    if (2 != argc)
    {
        (void) fprintf(stderr, "USAGE: luka [luka source file]\n");
        status_code = LUKA_WRONG_PARAMETERS;
        goto cleanup;
    }

    file_path = argv[1];
    file_contents = IO_get_file_contents(file_path);

    (void) vector_setup(&tokens, 1, sizeof(t_token_ptr));
    status_code = LEXER_tokenize_source(&tokens, file_contents);
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
       (void) fprintf(stderr, "Failed allocating memory for parser.\n");
       status_code = LUKA_CANT_ALLOC_MEMORY;
       goto cleanup;
    }

    (void) PARSER_initialize_parser(parser, &tokens, file_path);

    (void) PARSER_print_parser_tokens(parser);

    functions = PARSER_parse_top_level(parser);

    (void) AST_print_functions(functions, 0);

    (void) LLVMInitializeCore(LLVMGetGlobalPassRegistry());

    module = LLVMModuleCreateWithName("luka_main_module");
    builder = LLVMCreateBuilder();

    VECTOR_FOR_EACH(functions, iterator)
    {
        function = ITERATOR_GET_AS(t_ast_node_ptr, &iterator);
        value = GEN_codegen(function, module, builder);
        if (NULL == value)
        {
            (void) fprintf(stderr, "Failed generating code for function %s.\n", function->function.prototype->prototype.name);
            value = LLVMGetNamedFunction(module, function->function.prototype->prototype.name);
            if (NULL == value)
            {
                (void) fprintf(stderr, "Failed finding the function inside the module.\n");
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
    (void) LLVMAddConstantPropagationPass(pass_manager);
    (void) LLVMAddInstructionCombiningPass(pass_manager);
    (void) LLVMAddPromoteMemoryToRegisterPass(pass_manager);
    (void) LLVMAddReassociatePass(pass_manager);
    (void) LLVMAddGVNPass(pass_manager);
    (void) LLVMAddCFGSimplificationPass(pass_manager);

    (void) LLVMRunPassManager(pass_manager, module);

    (void) LLVMDumpModule(module);

    (void) LLVMWriteBitcodeToFile(module, "out.bc");

    status_code = LUKA_SUCCESS;

cleanup:
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

    (void) LIB_free_tokens_vector(&tokens);
    (void) LIB_free_functions_vector(functions);

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

    (void) LLVMShutdown();

    return status_code;
}
