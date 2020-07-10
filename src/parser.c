#include "../include/parser.h"
#include "../include/ast.h"
#include "../include/expr.h"

void initialize_parser(parser_t *parser, Vector *tokens,
                       const char *file_path) {
  assert(parser != NULL);

  parser->tokens = tokens;
  parser->index = 0;
  parser->file_path = file_path;
}

Vector *parse_top_level(parser_t *parser) {
  Vector *functions = NULL;
  token_t *token = NULL;
  ASTnode *function;

  functions = calloc(1, sizeof(Vector));

  vector_setup(functions, 5, sizeof(ast_node_ptr_t));

  while (parser->index < parser->tokens->size) {
    token = *(token_ptr_t *)vector_get(parser->tokens, parser->index);

    switch (token->type) {
    case T_FN: {
      function = parse_function(parser);
      vector_push_back(functions, &function);
      break;
    }
    case T_EOF:
      break;
    default: {
      fprintf(stderr, "Syntax error at %s %ld:%ld - %s\n", parser->file_path,
              token->line, token->offset, token->content);
    }
    }
    parser->index += 1;
  }

  vector_shrink_to_fit(functions);
  return functions;
}

void ERR(parser_t *parser, const char *message) {
  token_t *token = NULL;
  token = *(token_ptr_t *)vector_get(parser->tokens, parser->index + 1);

  fprintf(stderr, "%s\n", message);

  fprintf(stderr, "Error at %s %ld:%ld - %s\n", parser->file_path, token->line,
          token->offset, token->content);
  exit(1);
}

bool EXPECT(parser_t *parser, toktype_t type) {
  token_t *token = NULL;

  assert(parser->index + 1 < parser->tokens->size);

  token = *(token_ptr_t *)vector_get(parser->tokens, parser->index + 1);

  return token->type == type;
}

void ADVANCE(parser_t *parser) {
  ++parser->index;
  assert(parser->index < parser->tokens->size);
}

void EXPECT_ADVANCE(parser_t *parser, toktype_t type, const char *message) {
  if (!EXPECT(parser, type)) {
    ERR(parser, message);
  }

  ADVANCE(parser);
}

int parse_arithop(token_t *token) {
  switch (token->type) {
  case T_PLUS:
    return BINOP_ADD;
  case T_MINUS:
    return BINOP_SUBTRACT;
  case T_STAR:
    return BINOP_MULTIPLY;
  case T_SLASH:
    return BINOP_DIVIDE;
  default:
    fprintf(stderr, "unknown token in parse_arithop at %ld:%ld\n", token->line,
            token->offset);
    exit(1);
  }
}

ASTnode *parse_primary(parser_t *parser) {
  ASTnode *n;
  token_t *token = VECTOR_GET_AS(token_ptr_t, parser->tokens, parser->index);

  switch (token->type) {
  case T_NUMBER: {
    n = new_ast_number(atoi(token->content));
    ADVANCE(parser);
    return n;
  }

  default:
    fprintf(stderr, "parse_primary: Syntax error at %ld:%ld\n", token->line,
            token->offset);
    exit(1);
  }
}

ASTnode *parse_binexpr(parser_t *parser, int ptp) {
  ASTnode *left, *right;
  int nodetype;
  token_t *token = NULL, *lookahead = NULL;

  left = parse_primary(parser);

  token = VECTOR_GET_AS(token_ptr_t, parser->tokens, parser->index);

  if (T_CLOSE_BRACKET == token->type) {
    --parser->index;
    return left;
  }
  if ((T_EOF == token->type) || (T_SEMI_COLON == token->type)) {
    return left;
  }

  while (op_precedence(token) > ptp) {
    ADVANCE(parser);
    lookahead = VECTOR_GET_AS(token_ptr_t, parser->tokens, parser->index);
    if (T_CLOSE_BRACKET == lookahead->type) {
      --parser->index;
      return left;
    }

    right = parse_binexpr(parser, OpPrec[token->type - T_PLUS]);
    left = new_ast_binary_expr(parse_arithop(token), left, right);

    token = VECTOR_GET_AS(token_ptr_t, parser->tokens, parser->index);

    if (T_EOF == token->type) {
      return left;
    }
  }

  return left;
}

ASTnode *parse_function(parser_t *parser) {
  char *name = NULL;
  int number = 0;
  ASTnode *prototype = NULL, *body = NULL;
  EXPECT_ADVANCE(parser, T_IDENTIFIER,
                 "Expected an identifier after 'fn' keyword");
  name = (VECTOR_GET_AS(token_ptr_t, parser->tokens, parser->index))->content;

  EXPECT_ADVANCE(parser, T_OPEN_PAREN, "Expected a '('");
  EXPECT_ADVANCE(parser, T_CLOSE_PAREN, "Expected a ')'");
  prototype = new_ast_prototype(name, NULL, 0);

  EXPECT_ADVANCE(parser, T_OPEN_BRACKET, "Expected a '{'");
  ADVANCE(parser);
  body = parse_binexpr(parser, 0);
  EXPECT_ADVANCE(parser, T_CLOSE_BRACKET, "Expected a '}'");

  return new_ast_function(prototype, body);
}

void print_parser_tokens(parser_t *parser) {

  token_t *token = NULL;
  Iterator iterator = vector_begin(parser->tokens);
  Iterator last = vector_end(parser->tokens);
  for (; !iterator_equals(&iterator, &last); iterator_increment(&iterator)) {
    token = *(token_ptr_t *)iterator_get(&iterator);

    printf("%ld:%ld - %d - %s\n", token->line, token->offset, token->type,
           token->content);
  }
}
