#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "../include/interp.h"
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

  ASTnode *n = NULL;

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
  n = parse_binexpr(parser, 0);
  printf("%d\n", interpretAST(n));

  status_code = LUKA_SUCCESS;

exit:

  if (NULL != parser) {
    free(parser);
  }

  free_tokens_vector(&tokens);

  return status_code;
}
