#ifndef __TREE_H_
#define __TREE_H_

#include <stdio.h>
#include <stdlib.h>

#include "defs.h"

ASTnode *mk_ast_node(int op, ASTnode *left, ASTnode *right, int int_value);

ASTnode *mk_ast_leaf(int op, int intvalue);

ASTnode *mk_ast_unary(int op, ASTnode *left, int intvalue);

#endif // __TREE_H_
