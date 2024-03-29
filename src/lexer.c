/** @file lexer.c */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "lexer.h"
#include "logger.h"

/** A string representation of the keywords in the Luka programming language */
const char *keywords[NUMBER_OF_KEYWORDS]
    = {"fn", "return", "if", "else", "let", "mut", "extern", "while", "break",
       "as", "struct", "enum", "import", "type", "defer",

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
static int lexer_is_keyword(const char *identifier)
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
static char *lexer_lex_number(const char *source, size_t *index,
                              t_logger *logger)
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
            exit(LUKA_LEXER_FAILED);
        }

        do
        {
        } while (isdigit(source[++*index]));
    }

    string_length = *index - start_index;
    substring = malloc(string_length + 1);
    if (NULL == substring)
    {
        (void) LOGGER_log(logger, L_ERROR,
                          "Couldn't allocate memory for number substring.\n");
        exit(LUKA_CANT_ALLOC_MEMORY);
    }
    memcpy(substring, &source[start_index], string_length);
    substring[string_length] = '\0';

    if (is_floating && ('f' == source[*index]))
    {
        ++*index;
    }

    --*index;

    return substring;
}

/**
 * @brief Tokenize an identifier in the @p source from @p index.
 *
 * @param[in] source the source code.
 * @param[in,out] index the index to start from, will point at the next
 * @param[in] builtin whether the identifier is a builtin identifier or not.
 * character after the identifier when the function returns.
 *
 * @return the string representation of the identifier.
 */
static char *lexer_lex_identifier(const char *source, size_t *index,
                                  bool builtin)
{
    size_t i = *index;
    if (isalpha(source[i]) || ('_' == source[i]))
    {
        ++i;
        while ((0 != isalnum(source[i]) || ('_' == source[i])))
        {
            ++i;
        }
    }
    else
    {
        return "";
    }

    size_t size_to_allocate = (i - *index) + 1 + (builtin ? 1 : 0);
    char *ident = calloc(sizeof(char), size_to_allocate);
    if (NULL == ident)
    {
        return NULL;
    }

    if (builtin)
    {
        ident[0] = '@';
    }
    (void) strncpy(ident + (builtin ? 1 : 0), source + *index, i - *index);
    ident[i - *index + (builtin ? 1 : 0)] = '\0';

    *index = i - 1;
    return ident;
}

/**
 * @brief Tokenize a string in the @p source from @p index.
 *
 * @details strings are enclosed in `"` and can have the following escape
 * characters:
 * - `\\n` is a newline
 * - `\\t` is a tab
 * - `\\\\` is a backslash
 * - `\\"` is a double quotes
 * - `\\'` is a single quote
 * - `\\r` is a carriage return
 *
 * @param[in] source the source code.
 * @param[in,out] index the index to start from, will point at the next
 * character after the string when the function returns.
 * @param[in] logger a logger that can be used to log messages.
 * @param[in] end the ending character.
 *
 * @return the string representation of the string.
 */
static char *lexer_lex_string(const char *source, size_t *index,
                              t_logger *logger, char end)
{
    size_t i = *index;
    size_t char_count = 0, off = 0, ind = 0;
    while (end != source[i])
    {
        if ('\\' == source[i])
        {
            switch (source[i + 1])
            {
                case 'n':
                case 't':
                case '\\':
                case '"':
                case '\'':
                case '0':
                case 'r':
                    ++i;
                    break;
                default:
                    (void) LOGGER_log(logger, L_ERROR,
                                      "\\%c is not a valid esacpe sequence.\n",
                                      source[i + 1]);
                    exit(LUKA_LEXER_FAILED);
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
                case '\'':
                    str[ind] = '\'';
                    break;
                case '0':
                    str[ind] = '\0';
                case 'r':
                    str[ind] = '\r';
                    break;
                default:
                    (void) LOGGER_log(logger, L_ERROR,
                                      "\\%c is not a valid esacpe sequence.\n",
                                      source[*index + ind + off + 1]);
                    exit(LUKA_LEXER_FAILED);
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
    char *parsed_string;
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
            case '|':
                {
                    token->type = T_PIPE;
                    token->content = "|";
                    break;
                }
            case '^':
                {
                    token->type = T_CARET;
                    token->content = "^";
                    break;
                }
            case '~':
                {
                    token->type = T_TILDE;
                    token->content = "~";
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
                    else if ('<' == source[i + 1])
                    {
                        ++i;
                        ++offset;
                        token->type = T_SHL;
                        token->content = "<<";
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
                    else if ('>' == source[i + 1])
                    {
                        ++i;
                        ++offset;
                        token->type = T_SHR;
                        token->content = ">>";
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
                    parsed_string = lexer_lex_string(source, &i, logger, '"');
                    offset += i - saved_i;
                    if (NULL == parsed_string)
                    {
                        (void) LOGGER_log(logger, L_ERROR,
                                          "Couldn't lex string.\n");
                        return_code = LUKA_LEXER_FAILED;
                        goto l_cleanup;
                    }
                    token->content = parsed_string;
                    break;
                }
            case '\'':
                {
                    token->type = T_CHAR;
                    ++i;
                    ++offset;
                    saved_i = i;
                    parsed_string = lexer_lex_string(source, &i, logger, '\'');
                    if (strlen(parsed_string) > 1)
                    {
                        (void) LOGGER_log(logger, L_ERROR,
                                          "Character literal is too long "
                                          "(should be 1 character): '%s'\n",
                                          parsed_string);
                        return_code = LUKA_LEXER_FAILED;
                        goto l_cleanup;
                    }
                    offset += i - saved_i;
                    if (NULL == parsed_string)
                    {
                        (void) LOGGER_log(logger, L_ERROR,
                                          "Couldn't lex character.\n");
                        return_code = LUKA_LEXER_FAILED;
                        goto l_cleanup;
                    }
                    token->content = parsed_string;
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

            case '@':
                {
                    token->type = T_BUILTIN;
                    saved_i = i;
                    ++i;
                    parsed_string = lexer_lex_identifier(source, &i, true);
                    offset += i - saved_i;
                    if (NULL == parsed_string)
                    {
                        (void) LOGGER_log(logger, L_ERROR,
                                          "Couldn't lex identifier.\n");
                        return_code = LUKA_LEXER_FAILED;
                        goto l_cleanup;
                    }
                    token->content = parsed_string;
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
                        parsed_string = lexer_lex_identifier(source, &i, false);
                        offset += i - saved_i;
                        if (NULL == parsed_string)
                        {
                            (void) LOGGER_log(logger, L_ERROR,
                                              "Couldn't lex identifier.\n");
                            return_code = LUKA_LEXER_FAILED;
                            goto l_cleanup;
                        }
                        number = lexer_is_keyword(parsed_string);
                        if (-1 != number)
                        {
                            token->type = number;
                        }
                        token->content = parsed_string;
                        break;
                    }

                    (void) LOGGER_log(logger, L_ERROR,
                                      "Unrecognized character %c at %ld:%ld.\n",
                                      character, line, offset);
                    exit(LUKA_LEXER_FAILED);
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
