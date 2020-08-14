#ifndef __LEXER_H__
#define __LEXER_H__

#include "defs.h"
#include "vector.h"

void tokenize_source(Vector *tokens, const char *source);

#endif // __LEXER_H__
