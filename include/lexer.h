#ifndef __LEXER_H_
#define __LEXER_H_

#include "defs.h"
#include "vector.h"

void tokenize_source(Vector *tokens, const char *source);

#endif // __LEXER_H_
