#include "ast.h"

#include <stdio.h>
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

ASTnode *new_ast_if_expr(ASTnode *cond, Vector *then_body, Vector *else_body) {
  ASTnode *node = calloc(1, sizeof(ASTnode));
  node->type = AST_TYPE_IF_EXPR;
  node->if_expr.cond = cond;
  node->if_expr.then_body = then_body;
  node->if_expr.else_body = else_body;
  return node;
}

ASTnode *new_ast_variable(char *name) {
  ASTnode *node = calloc(1, sizeof(ASTnode));
  node->type = AST_TYPE_VARIABLE;
  node->variable.name = name;
  return node;
}

ASTnode *new_ast_let_stmt(ASTnode *var, ASTnode *expr) {
  ASTnode *node = calloc(1, sizeof(ASTnode));
  node->type = AST_TYPE_LET_STMT;
  node->let_stmt.var = var;
  node->let_stmt.expr = expr;
  return node;
}

ASTnode *new_ast_call_expr(char *name, Vector *args) {
  ASTnode *node = calloc(1, sizeof(ASTnode));
  node->type = AST_TYPE_CALL_EXPR;
  node->call_expr.name = name;
  node->call_expr.args = args;
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
  case AST_TYPE_IF_EXPR: {
    if (node->if_expr.cond)
      free_ast_node(node->if_expr.cond);
    if (node->if_expr.then_body) {
      ASTnode *stmt = NULL;
      VECTOR_FOR_EACH(node->if_expr.then_body, stmts) {
        stmt = ITERATOR_GET_AS(ast_node_ptr_t, &stmts);
        free_ast_node(stmt);
      }

      vector_clear(node->if_expr.then_body);
      vector_destroy(node->if_expr.then_body);
      free(node->if_expr.then_body);
    }
    if (node->if_expr.else_body) {
      ASTnode *stmt = NULL;
      VECTOR_FOR_EACH(node->if_expr.else_body, stmts) {
        stmt = ITERATOR_GET_AS(ast_node_ptr_t, &stmts);
        free_ast_node(stmt);
      }

      vector_clear(node->if_expr.else_body);
      vector_destroy(node->if_expr.else_body);
      free(node->if_expr.else_body);
    }
    break;
  }

  case AST_TYPE_VARIABLE: {
    if (node->variable.name) {
      free(node->variable.name);
    }
    break;
  }

  case AST_TYPE_LET_STMT: {
    if (node->let_stmt.var) {
      free_ast_node(node->let_stmt.var);
    }

    if (node->let_stmt.expr) {
      free_ast_node(node->let_stmt.expr);
    }
    break;
  }

  case AST_TYPE_CALL_EXPR: {
    if (node->call_expr.args) {
      ASTnode *arg = NULL;
      VECTOR_FOR_EACH(node->call_expr.args, args) {
        arg = ITERATOR_GET_AS(ast_node_ptr_t, &args);
        free_ast_node(arg);
      }

      vector_clear(node->call_expr.args);
      vector_destroy(node->call_expr.args);
      free(node->call_expr.args);
    }
    break;
  }

  default: {
    printf("I dont now how to free type - %d\n", node->type);
    break;
  }
  }

  free(node);
}

void print_statements_block(Vector *statements, int offset) {
  ASTnode *stmt = NULL;

  printf("%*c\b Statements block\n", offset, ' ');
  VECTOR_FOR_EACH(statements, stmts) {
    stmt = ITERATOR_GET_AS(ast_node_ptr_t, &stmts);
    print_ast(stmt, offset + 2);
  }
}

void print_functions(Vector *functions, int offset) {
  ASTnode *func = NULL;
  VECTOR_FOR_EACH(functions, funcs) {
    func = ITERATOR_GET_AS(ast_node_ptr_t, &funcs);
    print_ast(func, offset);
  }
}

char *op_to_str(AST_binop_type op) {
  switch (op) {
  case BINOP_ADD:
    return "+";
  case BINOP_SUBTRACT:
    return "-";
  case BINOP_MULTIPLY:
    return "*";
  case BINOP_DIVIDE:
    return "/";
  case BINOP_NOT:
    return "!";
  case BINOP_LESSER:
    return "<";
  case BINOP_GREATER:
    return ">";
  case BINOP_EQUALS:
    return "==";
  case BINOP_NEQ:
    return "!=";
  case BINOP_LEQ:
    return "<=";
  case BINOP_GEQ:
    return ">=";
  default:
    fprintf(stderr, "Unknown operator: %d\n", op);
    exit(1);
  }
}

void print_ast(ASTnode *node, int offset) {
  ASTnode *arg = NULL;
  size_t i = 0;
  if (!node)
    return;

  if (0 == offset) {
    printf("Printing AST:\n");
  }

  switch (node->type) {
  case AST_TYPE_NUMBER:
    printf("%*c\b AST number %d\n", offset, ' ', node->number.value);
    break;
  case AST_TYPE_BINARY_EXPR: {
    printf("%*c\b Binary Expression\n", offset, ' ');
    printf("%*c\b Operator: %s\n", offset + 2, ' ',
           op_to_str(node->binary_expr.operator));
    if (node->binary_expr.lhs)
      print_ast(node->binary_expr.lhs, offset + 4);
    if (node->binary_expr.rhs)
      print_ast(node->binary_expr.rhs, offset + 4);
    break;
  }
  case AST_TYPE_PROTOTYPE: {
    printf("%*c\b %s - %d args\n", offset, '-', node->prototype.name,
           node->prototype.arity);
    for (i = 0; i < node->prototype.arity; ++i) {
      printf("%*c\b %d: (%s) %s\n", offset, '-', i, "Int32",
             node->prototype.args[i]);
    }
    break;
  }

  case AST_TYPE_FUNCTION: {
    printf("%*c\b Function definition\n", offset, ' ');
    if (node->function.prototype)
      printf("%*c\b Prototype\n", offset + 2, ' ');
    print_ast(node->function.prototype, offset + 4);
    if (node->function.body) {
      printf("%*c\b Body\n", offset + 2, ' ');
      print_statements_block(node->function.body, offset + 4);
    }
    break;
  }
  case AST_TYPE_RETURN_STMT: {
    printf("%*c\b Return statement\n", offset, ' ');
    if (node->return_stmt.expr)
      print_ast(node->return_stmt.expr, offset + 2);
    break;
  }
  case AST_TYPE_IF_EXPR: {
    printf("%*c\b If Expression\n", offset, ' ');
    if (node->if_expr.cond) {
      printf("%*c\b Condition\n", offset + 2, ' ');

      print_ast(node->if_expr.cond, offset + 4);
    }
    if (node->if_expr.then_body) {
      printf("%*c\b Then Body\n", offset + 2, ' ');
      print_statements_block(node->if_expr.then_body, offset + 4);
    }
    if (node->if_expr.else_body) {
      printf("%*c\b Else Body\n", offset + 2, ' ');
      print_statements_block(node->if_expr.else_body, offset + 4);
    }
    break;
  }
  case AST_TYPE_VARIABLE: {
    printf("%*c\b Variable\n", offset, ' ');
    if (node->variable.name) {
      printf("%*c\b Name: %s\n", offset + 2, ' ', node->variable.name);
    }
    break;
  }
  case AST_TYPE_LET_STMT: {
    printf("%*c\b Let Statement\n", offset, ' ');
    if (node->let_stmt.var) {
      print_ast(node->let_stmt.var, offset + 2);
    }

    if (node->let_stmt.expr) {
      printf("%*c\b Expression\n", offset + 2, ' ');
      print_ast(node->let_stmt.expr, offset + 4);
    }
    break;
  }

  case AST_TYPE_CALL_EXPR: {
    printf("%*c\b Call Expression\n", offset, ' ');
    if (node->call_expr.name) {
      printf("%*c\b Name - %s\n", offset + 2, ' ', node->call_expr.name);
    }

    if (node->call_expr.args) {
      printf("%*c\b Arguments\n", offset + 2, ' ');
      printf("%*c\b Count - %d\n", offset + 4, ' ', node->call_expr.args->size);
      VECTOR_FOR_EACH(node->call_expr.args, args) {
        arg = ITERATOR_GET_AS(ast_node_ptr_t, &args);
        print_ast(arg, offset + 4);
      }
    }

    break;
  }
  default: {
    printf("I don't know how to print type - %d\n", node->type);
    break;
  }
  }
}
