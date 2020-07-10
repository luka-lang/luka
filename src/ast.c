#include "../include/ast.h"

ASTnode *new_ast_number(int value) {
  ASTnode *node = calloc(1, sizeof(ASTnode));
  node->type = AST_TYPE_NUMBER;
  node->number.value = value;
  return node;
}

ASTnode *new_ast_binary_expr(AST_binop_type operator, ASTnode * lhs,
                             ASTnode * rhs) {
  ASTnode *node = calloc(1, sizeof(ASTnode));
  node->type = AST_TYPE_BINARY_EXPR;
  node->binary_expr.operator= operator;
  node->binary_expr.lhs = lhs;
  node->binary_expr.rhs = rhs;
  return node;
}

void free_ast_node(ASTnode *node) {
  if (!node)
    return;

  switch (node->type) {
  case AST_TYPE_NUMBER:
    break;
  case AST_TYPE_BINARY_EXPR: {
    if (node->binary_expr.lhs)
      free_ast_node(node->binary_expr.lhs);
    if (node->binary_expr.rhs)
      free_ast_node(node->binary_expr.rhs);
    break;
  }
  }

  free(node);
}
