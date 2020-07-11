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

#include "../include/ast.h"
#include "../include/gen.h"
#include "../include/io.h"
#include "../include/lexer.h"
#include "../include/parser.h"

typedef enum {
  LUKA_UNINITIALIZED = -1,
  LUKA_SUCCESS = 0,
  LUKA_WRONG_PARAMETERS,
  LUKA_CANT_OPEN_FILE,
  LUKA_CANT_ALLOC_MEMORY,
} return_code_t;

void free_tokens_vector(Vector *tokens) {
  token_t *token = NULL;
  Iterator iterator = vector_begin(tokens);
  Iterator last = vector_end(tokens);
  for (; !iterator_equals(&iterator, &last); iterator_increment(&iterator)) {
    token = *(token_ptr_t *)iterator_get(&iterator);
    if ((token->type == T_NUMBER) || (token->type == T_STRING) ||
        ((token->type <= T_IDENTIFIER) && (token->type > T_UNKNOWN))) {
      free(token->content);
      token->content = NULL;
    }
    free(token);
    token = NULL;
  }
  vector_clear(tokens);
  vector_destroy(tokens);
}

void free_functions_vector(Vector *functions) {
  ASTnode *function = NULL;
  Iterator iterator = vector_begin(functions);
  Iterator last = vector_end(functions);
  for (; !iterator_equals(&iterator, &last); iterator_increment(&iterator)) {
    function = *(ast_node_ptr_t *)iterator_get(&iterator);
    free_ast_node(function);
  }

  vector_clear(functions);
  vector_destroy(functions);
}

int main(int argc, char **argv) {
  return_code_t status_code = LUKA_UNINITIALIZED;
  char *file_path = NULL;

  int file_size;
  char *file_contents = NULL;

  Vector tokens;
  Vector *functions;
  token_t *token = NULL;

  parser_t *parser = NULL;

  ASTnode *function = NULL;

  LLVMModuleRef module = NULL;
  LLVMBuilderRef builder = NULL;
  LLVMPassManagerRef pass_manager = NULL;

  LLVMTypeRef func_type = NULL;
  LLVMValueRef func = NULL, tmp = NULL;
  LLVMBasicBlockRef entry = NULL;

  char *error = NULL;

  if (argc != 2) {
    fprintf(stderr, "USAGE: luka [luka source file]\n");
    exit(LUKA_WRONG_PARAMETERS);
  }

  file_path = argv[1];
  file_contents = get_file_contents(file_path);

  vector_setup(&tokens, 1, sizeof(token_ptr_t));
  tokenize_source(&tokens, file_contents);
  if (NULL != file_contents) {
    free(file_contents);
  }

  parser = calloc(1, sizeof(parser_t));

  initialize_parser(parser, &tokens, file_path);

  print_parser_tokens(parser);

  functions = parse_top_level(parser); // parse_binexpr(parser, 0);

  module = LLVMModuleCreateWithName("luka_main_module");

  VECTOR_FOR_EACH(functions, iterator) {
    function = ITERATOR_GET_AS(ast_node_ptr_t, &iterator);

    int arity = function->prototype.arity;
    LLVMTypeRef params[arity];
    for (int i = 0; i < arity; ++i) {
      // int32_t by default
      params[i] = LLVMInt32Type();
    }
    func_type = LLVMFunctionType(LLVMInt32Type(), params, arity, 0);
    func = LLVMAddFunction(module, function->function.prototype->prototype.name,
                           func_type);

    entry = LLVMAppendBasicBlock(func, "entry");
    builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);

    tmp = codegen(function->function.body, module, builder);
    if (NULL != tmp) {
      LLVMBuildRet(builder, tmp);
    }
  }

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

  LLVMWriteBitcodeToFile(module, "life.bc");

  status_code = LUKA_SUCCESS;

exit:

  if (NULL != parser) {
    free(parser);
  }

  free_tokens_vector(&tokens);
  free_functions_vector(functions);

  // LLVM STUFF

  if (NULL != pass_manager) {
    LLVMDisposePassManager(pass_manager);
  }

  if (NULL != builder) {

    LLVMDisposeBuilder(builder);
  }
  if (NULL != module) {

    LLVMDisposeModule(module);
  }

  return status_code;
}
