/** @file lexer.c */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

/** A string representation of the keywords in the Luka programming language */
const char *keywords[NUMBER_OF_KEYWORDS]
    = {"fn", "return", "if", "else", "let", "mut", "extern", "while", "break",
       "as", "struct", "enum", "import", "type",

       /* Literals */
       "null", "true", "false",

       /* Builtin Types */
       "int", "char", "string", "void", "float", "double", "any", "bool", "u8",
       "u16", "u32", "u64", "s8", "s16", "s32", "s64", "f32", "f64"};

/**
 * @brief Check if an identifier is a predefined keyword.
 *
 * @param[in] identifier the identifier to check.
 *
 * @return -1 if the identifier is not a keyword or the index of the keyword in
 * the #keywords arrays.
 */
int lexer_is_keyword(const char *identifier)
{
    for (int i = 0; i < NUMBER_OF_KEYWORDS; ++i)
    {
        if (strlen(identifier) != strlen(keywords[i]))
        {
            continue;
        }
        if (0 == memcmp(identifier, keywords[i], strlen(identifier)))
        {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Tokenize a number in the @p source from @p index.
 *
 * @param[in] source the source code.
 * @param[in,out] index the index to start from, will point at the next
 * character after the number when the function returns.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the string representation of the number.
 */
char *lexer_lex_number(const char *source, size_t *index, t_logger *logger)
{
    bool is_floating = false;
    size_t start_index = *index;
    size_t string_length = 0;
    char *substring = NULL;

    do
    {
    } while (isdigit(source[++*index]));

    if ('.' == source[*index])
    {
        is_floating = true;
        ++*index;
        if (!isdigit(source[*index]))
        {
            (void) LOGGER_log(logger, L_ERROR,
                              "Floating point numbers must have at least one "
                              "digit after the '.'\n");
            (void) exit(LUKA_LEXER_FAILED);
        }

        do
        {
        } while (isdigit(source[++*index]));
    }

    if (is_floating && ('f' == source[*index]))
    {
        ++*index;
    }

    string_length = *index - start_index;
    substring = malloc(string_length + 1);
    if (NULL == substring)
    {
        (void) LOGGER_log(logger, L_ERROR,
                          "Couldn't allocate memory for number substring.\n");
        (void) exit(LUKA_CANT_ALLOC_MEMORY);
    }
    memcpy(substring, &source[start_index], string_length);
    substring[string_length] = '\0';
    --*index;

    return substring;
}

/**
 * @brief Tokenize an identifier in the @p source from @p index.
 *
 * @param[in] source the source code.
 * @param[in,out] index the index to start from, will point at the next
 * character after the identifier when the function returns.
 *
 * @return the string representation of the identifier.
 */
char *lexer_lex_identifier(const char *source, size_t *index)
{
    size_t i = (*index) + 1;
    while ((0 != isalnum(source[i]) || ('_' == source[i])))
    {
        ++i;
    }

    char *ident = calloc(sizeof(char), (i - *index) + 1);
    if (NULL == ident)
    {
        return NULL;
    }

    (void) strncpy(ident, source + *index, i - *index);
    ident[i - *index] = '\0';

    *index = i - 1;
    return ident;
}

/**
 * @brief Tokenize a string in the @p source from @p index.
 *
 * @details strings are enclosed in `"` and can have the following escape
 * characters:
 * - `\n` is a newline
 * - `\t` is a tab
 * - `\\` is a backslash
 * - `\"` is a quotation mark
 *
 * @param[in] source the source code.
 * @param[in,out] index the index to start from, will point at the next
 * character after the string when the function returns.
 * @param[in] logger a logger that can be used to log messages.
 *
 * @return the string representation of the string.
 */
char *lexer_lex_string(const char *source, size_t *index, t_logger *logger)
{
    size_t i = *index;
    size_t char_count = 0, off = 0, ind = 0;
    while ('"' != source[i])
    {
        if ('\\' == source[i])
        {
            switch (source[i + 1])
            {
                case 'n':
                case 't':
                case '\\':
                case '"':
                    ++i;
                    break;
                default:
                    (void) LOGGER_log(logger, L_ERROR,
                                      "\\%c is not a valid esacpe sequence.\n",
                                      source[i + 1]);
                    (void) exit(1);
            }
        }

        ++char_count;
        ++i;
    }

    ++i;

    char *str = calloc(sizeof(char), char_count + 1);
    if (NULL == str)
    {
        (void) LOGGER_log(
            logger, L_ERROR,
            "Couldn't allocate memory for string in lexer_lex_string.\n");
        return NULL;
    }

    for (ind = 0, off = 0; ind < char_count; ++ind)
    {
        if ('\\' == source[*index + ind + off])
        {
            switch (source[*index + ind + off + 1])
            {
                case 'n':
                    str[ind] = '\n';
                    break;
                case 't':
                    str[ind] = '\t';
                    break;
                case '\\':
                    str[ind] = '\\';
                    break;
                case '"':
                    str[ind] = '"';
                    break;
                default:
                    (void) LOGGER_log(logger, L_ERROR,
                                      "\\%c is not a valid esacpe sequence.\n",
                                      source[*index + ind + off + 1]);
                    (void) exit(1);
            }

            ++off;
        }
        else
        {
            str[ind] = source[*index + ind + off];
        }
    }

    str[char_count] = '\0';

    *index = i - 1;
    return str;
}

t_return_code LEXER_tokenize_source(t_vector *tokens, const char *source,
                                    t_logger *logger, const char *file_path)
{
    long line = 1, offset = 0;
    size_t length = strlen(source);
    char character = '\0';
    t_token *token = NULL;
    int number = 0;
    char *identifier;
    t_return_code return_code = LUKA_UNINITIALIZED;
    size_t i = 0, saved_i = 0;

    token = calloc(1, sizeof(t_token));
    if (NULL == token)
    {
        (void) LOGGER_log(logger, L_ERROR,
                          "Couldn't allocate memory for token.\n");
        return_code = LUKA_CANT_ALLOC_MEMORY;
        goto l_cleanup;
    }

    for (i = 0; i < length; ++i)
    {
        character = source[i];
        ++offset;
        if ('\n' == character)
        {
            ++line;
            offset = 0;
        }
        if (isspace(character))
        {
            continue;
        }

        token->line = line;
        token->offset = offset;
        token->type = T_UNKNOWN;
        token->content = "AAAA";
        token->file_path = file_path;

        switch (character)
        {
            case '(':
                {
                    token->type = T_OPEN_PAREN;
                    token->content = "(";
                    break;
                }
            case ')':
                {
                    token->type = T_CLOSE_PAREN;
                    token->content = ")";
                    break;
                }
            case '{':
                {
                    token->type = T_OPEN_BRACE;
                    token->content = "{";
                    break;
                }
            case '}':
                {
                    token->type = T_CLOSE_BRACE;
                    token->content = "}";
                    break;
                }
            case '[':
                {
                    token->type = T_OPEN_BRACKET;
                    token->content = "[";
                    break;
                }
            case ']':
                {
                    token->type = T_CLOSE_BRACKET;
                    token->content = "]";
                    break;
                }
            case ';':
                {
                    token->type = T_SEMI_COLON;
                    token->content = ";";
                    break;
                }
            case ',':
                {
                    token->type = T_COMMA;
                    token->content = ",";
                    break;
                }
            case '+':
                {
                    token->type = T_PLUS;
                    token->content = "+";
                    break;
                }
            case '-':
                {
                    token->type = T_MINUS;
                    token->content = "-";
                    break;
                }
            case '*':
                {
                    token->type = T_STAR;
                    token->content = "*";
                    break;
                }
            case '%':
                {
                    token->type = T_PERCENT;
                    token->content = "%";
                    break;
                }
            case '&':
                {
                    token->type = T_AMPERCENT;
                    token->content = "&";
                    break;
                }
            case ':':
                {
                    if (':' == source[i + 1])
                    {
                        ++i;
                        ++offset;
                        token->type = T_DOUBLE_COLON;
                        token->content = "::";
                        break;
                    }
                    else
                    {
                        token->type = T_COLON;
                        token->content = ":";
                        break;
                    }
                }
                break;
            case '/':
                {
                    if ('/' == source[i + 1])
                    {
                        // Found a comment
                        ++i;
                        ++offset;
                        while ('\n' != source[i + 1])
                        {
                            ++i;
                            ++offset;
                        }
                        continue;
                    }
                    token->type = T_SLASH;
                    token->content = "/";
                    break;
                }
            case '=':
                {
                    if ('=' == source[i + 1])
                    {
                        ++i;
                        ++offset;
                        token->type = T_EQEQ;
                        token->content = "==";
                    }
                    else
                    {
                        token->type = T_EQUALS;
                        token->content = "=";
                    }
                    break;
                }
            case '<':
                {
                    if ('=' == source[i + 1])
                    {
                        ++i;
                        ++offset;
                        token->type = T_LEQ;
                        token->content = "<=";
                    }
                    else
                    {
                        token->type = T_OPEN_ANG;
                        token->content = "<";
                    }
                    break;
                }
            case '>':
                {
                    if ('=' == source[i + 1])
                    {
                        ++i;
                        ++offset;
                        token->type = T_GEQ;
                        token->content = ">=";
                    }
                    else
                    {
                        token->type = T_CLOSE_ANG;
                        token->content = ">";
                    }
                    break;
                }
            case '!':
                {
                    if ('=' == source[i + 1])
                    {
                        ++i;
                        ++offset;
                        token->type = T_NEQ;
                        token->content = "!=";
                    }
                    else
                    {
                        token->type = T_BANG;
                        token->content = "!";
                    }
                    break;
                }
            case '"':
                {
                    token->type = T_STRING;
                    ++i;
                    ++offset;
                    saved_i = i;
                    identifier = lexer_lex_string(source, &i, logger);
                    offset += i - saved_i;
                    if (NULL == identifier)
                    {
                        (void) LOGGER_log(logger, L_ERROR,
                                          "Couldn't lex string.\n");
                        return_code = LUKA_LEXER_FAILED;
                        goto l_cleanup;
                    }
                    token->content = identifier;
                    break;
                }
            case '.':
                {
                    if (('.' == source[i + 1]) && ('.' == source[i + 2]))
                    {
                        i += 2;
                        offset += 2;
                        token->type = T_THREE_DOTS;
                        token->content = "...";
                    }
                    else
                    {
                        token->type = T_DOT;
                        token->content = ".";
                    }
                    break;
                }
            case EOF:
                {
                    token->type = T_EOF;
                    token->content = "~EOF~";
                    break;
                }
            default:
                {
                    if (isdigit(character))
                    {
                        token->type = T_NUMBER;
                        saved_i = i;
                        token->content = lexer_lex_number(source, &i, logger);
                        offset += i - saved_i;
                        break;
                    }

                    if (isalpha(character) || ('_' == character))
                    {
                        token->type = T_IDENTIFIER;
                        saved_i = i;
                        identifier = lexer_lex_identifier(source, &i);
                        offset += i - saved_i;
                        if (NULL == identifier)
                        {
                            (void) LOGGER_log(logger, L_ERROR,
                                              "Couldn't lex identifier.\n");
                            return_code = LUKA_LEXER_FAILED;
                            goto l_cleanup;
                        }
                        number = lexer_is_keyword(identifier);
                        if (-1 != number)
                        {
                            token->type = number;
                        }
                        token->content = identifier;
                        break;
                    }

                    (void) LOGGER_log(logger, L_ERROR,
                                      "Unrecognized character %c at %ld:%ld.\n",
                                      character, line, offset);
                    (void) exit(1);
                }
        }

        if (VECTOR_SUCCESS != vector_push_back(tokens, &token))
        {
            (void) LOGGER_log(logger, L_ERROR,
                              "Couldn't add token to tokens vector.\n");
            return_code = LUKA_VECTOR_FAILURE;
            goto l_cleanup;
        }
        token = calloc(1, sizeof(t_token));
        if (NULL == token)
        {
            (void) LOGGER_log(logger, L_ERROR,
                              "Couldn't allocate memory for token.\n");
            return_code = LUKA_CANT_ALLOC_MEMORY;
            goto l_cleanup;
        }
    }

    (void) vector_shrink_to_fit(tokens);

    return_code = LUKA_SUCCESS;

l_cleanup:
    if (NULL != token)
    {
        (void) free(token);
        token = NULL;
    }

    return return_code;
}
