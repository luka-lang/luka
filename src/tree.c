#include "../include/tree.h"

ASTnode *mk_ast_node(int op, ASTnode *left, ASTnode *right, int intvalue) {
  ASTnode *node;
  node = calloc(1, sizeof(ASTnode));
  if (NULL == node) {
    fprintf(stderr, "Unable to malloc space for ast node\n");
    exit(1);
  }

  node->op = op;
  node->left = left;
  node->right = right;
  node->intvalue = intvalue;
  return node;
}

ASTnode *mk_ast_leaf(int op, int intvalue) {
  return mk_ast_node(op, NULL, NULL, intvalue);
}

ASTnode *mk_ast_unary(int op, ASTnode *left, int intvalue) {
  return mk_ast_node(op, left, NULL, intvalue);
}
