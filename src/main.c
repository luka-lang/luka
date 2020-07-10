#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Object.h>
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

int main(int argc, char **argv) {
  return_code_t status_code = LUKA_UNINITIALIZED;
  char *file_path = NULL;

  int file_size;
  char *file_contents = NULL;

  Vector tokens;
  token_t *token = NULL;

  parser_t *parser = NULL;

  ASTnode *root = NULL;

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

  // start_parsing(parser);
  root = parse_binexpr(parser, 0);

  LLVMModuleRef module = LLVMModuleCreateWithName("luka_main_module");

  LLVMTypeRef main_type = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
  LLVMValueRef main = LLVMAddFunction(module, "main", main_type);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main, "entry");
  LLVMBuilderRef builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef tmp = codegen(root, module, builder);

  LLVMBuildRet(builder, tmp);

  char *error = NULL;
  LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
  LLVMDisposeMessage(error);

  LLVMPassManagerRef pm = LLVMCreatePassManager();
  LLVMAddConstantPropagationPass(pm);
  LLVMAddInstructionCombiningPass(pm);
  LLVMAddPromoteMemoryToRegisterPass(pm);
  LLVMAddReassociatePass(pm);
  LLVMAddGVNPass(pm);
  LLVMAddCFGSimplificationPass(pm);

  LLVMRunPassManager(pm, module);

  LLVMDumpModule(module);

  LLVMWriteBitcodeToFile(module, "life.bc");

  status_code = LUKA_SUCCESS;

exit:

  if (NULL != parser) {
    free(parser);
  }

  if (NULL != root) {
    free_ast_node(root);
  }

  free_tokens_vector(&tokens);

  // LLVM STUFF

  LLVMDisposePassManager(pm);
  LLVMDisposeBuilder(builder);
  LLVMDisposeModule(module);

  return status_code;
}
