#include "../include/ast.h"

#include <string.h>

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

ASTnode *new_ast_prototype(char *name, char **args, int arity) {
  ASTnode *node = calloc(1, sizeof(ASTnode));
  node->type = AST_TYPE_PROTOTYPE;
  node->prototype.name = strdup(name);
  node->prototype.args = calloc(arity, sizeof(char *));
  for (int i = 0; i < arity; ++i) {
    node->prototype.args[i] = strdup(args[i]);
  }
  node->prototype.arity = arity;
  return node;
}

ASTnode *new_ast_function(ASTnode *prototype, Vector *body) {
  ASTnode *node = calloc(1, sizeof(ASTnode));
  node->type = AST_TYPE_FUNCTION;
  node->function.prototype = prototype;
  node->function.body = body;
  return node;
}

ASTnode *new_ast_return_stmt(ASTnode *expr) {
  ASTnode *node = calloc(1, sizeof(ASTnode));
  node->type = AST_TYPE_RETURN_STMT;
  node->return_stmt.expr = expr;
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
  case AST_TYPE_PROTOTYPE: {
    if (node->prototype.name)
      free(node->prototype.name);
    for (int i = 0; i < node->prototype.arity; ++i) {
      free(node->prototype.args[i]);
    }
    free(node->prototype.args);
    break;
  }

  case AST_TYPE_FUNCTION: {
    if (node->function.prototype)
      free_ast_node(node->function.prototype);
    if (node->function.body) {
      ASTnode *stmt = NULL;
      VECTOR_FOR_EACH(node->function.body, stmts) {
        stmt = ITERATOR_GET_AS(ast_node_ptr_t, &stmts);
        free_ast_node(stmt);
      }

      vector_clear(node->function.body);
      vector_destroy(node->function.body);
      free(node->function.body);
    }
    break;
  }
  case AST_TYPE_RETURN_STMT: {
    if (node->return_stmt.expr)
      free_ast_node(node->return_stmt.expr);
    break;
  }
  }

  free(node);
}
