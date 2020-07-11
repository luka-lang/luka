#include "../include/parser.h"
#include "../include/ast.h"
#include "../include/expr.h"

#include <string.h>

Vector *parse_statements(parser_t *parser);

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

bool MATCH(parser_t *parser, toktype_t type) {
  token_t *token = NULL;

  assert(parser->index < parser->tokens->size);

  token = *(token_ptr_t *)vector_get(parser->tokens, parser->index);

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

void MATCH_ADVANCE(parser_t *parser, toktype_t type, const char *message) {
  if (!MATCH(parser, type)) {
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
    fprintf(stderr, "parse_primary: Syntax error at %ld:%ld - %s\n",
            token->line, token->offset, token->content);
    exit(1);
  }
}

bool should_finish_expression(token_t *token) {
  if (T_OPEN_BRACKET == token->type) {
    return true;
  }

  if (T_CLOSE_BRACKET == token->type) {
    return true;
  }

  if (T_SEMI_COLON == token->type) {
    return true;
  }

  if (T_EOF == token->type) {
    return true;
  }

  return false;
}

ASTnode *parse_binexpr(parser_t *parser, int ptp) {
  ASTnode *left, *right;
  int nodetype;
  token_t *token = NULL;

  left = parse_primary(parser);

  token = VECTOR_GET_AS(token_ptr_t, parser->tokens, parser->index);

  if (should_finish_expression(token)) {
    return left;
  }

  while (op_precedence(token) > ptp) {
    ADVANCE(parser);

    right = parse_binexpr(parser, OpPrec[token->type - T_PLUS]);
    left = new_ast_binary_expr(parse_arithop(token), left, right);

    token = VECTOR_GET_AS(token_ptr_t, parser->tokens, parser->index);

    if (should_finish_expression(token)) {
      return left;
    }
  }

  return left;
}

ASTnode *parse_expression(parser_t *parser) {
  ASTnode *node = NULL, *expr = NULL, *cond = false;
  Vector *then_body = NULL, *else_body = NULL;
  token_t *token = NULL;

  token = VECTOR_GET_AS(token_ptr_t, parser->tokens, parser->index);

  switch (token->type) {
  case T_IF: {
    ADVANCE(parser);
    cond = parse_expression(parser);
    --parser->index;
    then_body = parse_statements(parser);
    if (EXPECT(parser, T_ELSE)) {
      ADVANCE(parser);
      else_body = parse_statements(parser);
    } else {
      else_body = NULL;
    }
    ADVANCE(parser);

    node = new_ast_if_expr(cond, then_body, else_body);
    return node;
  };
  default: {
    return parse_binexpr(parser, 0);
  }
  }
}

ASTnode *parse_statement(parser_t *parser) {
  ASTnode *node = NULL, *expr = NULL;
  token_t *token = NULL;

  token = VECTOR_GET_AS(token_ptr_t, parser->tokens, parser->index);
  switch (token->type) {
  case T_RETURN: {
    ADVANCE(parser);
    expr = parse_expression(parser);
    MATCH_ADVANCE(parser, T_SEMI_COLON,
                  "Expected a ';' at the end of a return statement");
    node = new_ast_return_stmt(expr);
    return node;
  }
  default: {
    expr = parse_expression(parser);
    token = VECTOR_GET_AS(token_ptr_t, parser->tokens, parser->index);

    if (should_finish_expression(token)) {
      return expr;
    }
    fprintf(stderr, "Not a statement: %ld:%ld - %s\n", token->line,
            token->offset, token->content);
    exit(1);
  }
  }

  return NULL;
}

Vector *parse_statements(parser_t *parser) {
  Vector *stmts = NULL;
  ASTnode *stmt = NULL;
  token_t *token = NULL;
  stmts = calloc(1, sizeof(Vector));

  vector_setup(stmts, 10, sizeof(ast_node_ptr_t));

  EXPECT_ADVANCE(parser, T_OPEN_BRACKET,
                 "Expected '{' to open a body of statements");

  while (T_CLOSE_BRACKET !=
         (token = VECTOR_GET_AS(token_ptr_t, parser->tokens, parser->index + 1))
             ->type) {
    ADVANCE(parser);
    stmt = parse_statement(parser);
    vector_push_back(stmts, &stmt);
    --parser->index;
  }

  ADVANCE(parser);

  vector_shrink_to_fit(stmts);
  return stmts;
}

ASTnode *parse_prototype(parser_t *parser) {
  char *name = NULL;
  char **args = NULL;
  int arity = 0;

  token_t *token = NULL;

  EXPECT_ADVANCE(parser, T_IDENTIFIER,
                 "Expected an identifier after 'fn' keyword");
  name = (VECTOR_GET_AS(token_ptr_t, parser->tokens, parser->index))->content;

  EXPECT_ADVANCE(parser, T_OPEN_PAREN, "Expected a '('");

  if (T_CLOSE_PAREN ==
      (VECTOR_GET_AS(token_ptr_t, parser->tokens, parser->index + 1))->type) {
    // No args
    ADVANCE(parser);
    return new_ast_prototype(name, args, arity);
  }

  ADVANCE(parser);

  token = VECTOR_GET_AS(token_ptr_t, parser->tokens, parser->index);
  args = calloc(1, sizeof(char *));
  args[0] = strdup(token->content);
  arity = 1;

  while (T_CLOSE_PAREN !=
         (token = VECTOR_GET_AS(token_ptr_t, parser->tokens, parser->index + 1))
             ->type) {
    EXPECT_ADVANCE(parser, T_COMMA, "Expected ',' after arg");
    EXPECT_ADVANCE(parser, T_IDENTIFIER, "Expected another arg after ','");
    token = VECTOR_GET_AS(token_ptr_t, parser->tokens, parser->index);
    ++arity;
    args = realloc(args, sizeof(char *) * arity);
    args[arity - 1] = strdup(token->content);
  }
  EXPECT_ADVANCE(parser, T_CLOSE_PAREN, "Expected a ')'");

  return new_ast_prototype(name, args, arity);
}

ASTnode *parse_function(parser_t *parser) {
  int number = 0;
  ASTnode *prototype = NULL;
  Vector *body = NULL;
  prototype = parse_prototype(parser);
  body = parse_statements(parser);
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
