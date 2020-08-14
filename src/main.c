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

    char *error = NULL;

    if (argc != 2)
    {
        fprintf(stderr, "USAGE: luka [luka source file]\n");
        exit(LUKA_WRONG_PARAMETERS);
    }

    file_path = argv[1];
    file_contents = IO_get_file_contents(file_path);

    vector_setup(&tokens, 1, sizeof(t_token_ptr));
    LEXER_tokenize_source(&tokens, file_contents);
    if (NULL != file_contents)
    {
        free(file_contents);
    }

    parser = calloc(1, sizeof(t_parser));

    PARSER_initialize_parser(parser, &tokens, file_path);

    PARSER_print_parser_tokens(parser);

    functions = PARSER_parse_top_level(parser);

    AST_print_functions(functions, 0);

    LLVMInitializeCore(LLVMGetGlobalPassRegistry());

    module = LLVMModuleCreateWithName("luka_main_module");
    builder = LLVMCreateBuilder();

    VECTOR_FOR_EACH(functions, iterator)
    {
        function = ITERATOR_GET_AS(t_ast_node_ptr, &iterator);
        GEN_codegen(function, module, builder);
    }

    GEN_codegen_reset();

    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);

    pass_manager = LLVMCreatePassManager();
    LLVMAddConstantPropagationPass(pass_manager);
    LLVMAddInstructionCombiningPass(pass_manager);
    LLVMAddPromoteMemoryToRegisterPass(pass_manager);
    LLVMAddReassociatePass(pass_manager);
    LLVMAddGVNPass(pass_manager);
    LLVMAddCFGSimplificationPass(pass_manager);

    LLVMRunPassManager(pass_manager, module);

    LLVMDumpModule(module);

    LLVMWriteBitcodeToFile(module, "out.bc");

    status_code = LUKA_SUCCESS;

exit:

    if (NULL != parser)
    {
        free(parser);
    }

    LIB_free_tokens_vector(&tokens);
    LIB_free_functions_vector(functions);

    // LLVM STUFF

    if (NULL != pass_manager)
    {
        LLVMDisposePassManager(pass_manager);
    }

    if (NULL != builder)
    {

        LLVMDisposeBuilder(builder);
    }
    if (NULL != module)
    {

        LLVMDisposeModule(module);
    }

    LLVMShutdown();

    return status_code;
}
