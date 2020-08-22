#ifndef __LEXER_H__
#define __LEXER_H__

#include "defs.h"
#include "vector.h"

t_return_code LEXER_tokenize_source(t_vector *tokens, const char *source);

#endif // __LEXER_H__
