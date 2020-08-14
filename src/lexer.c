#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

const char *keywords[NUMBER_OF_KEYWORDS] = {"fn", "return", "if", "else",
                                            "let", "extern", "int", "str",
                                            "void", "float", "double"};

int is_keyword(const char *identifier)
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

int parse_number(const char *source, int *index)
{
    int number = 0;

    do
    {
        number *= 10;
        number += (source[*index] - '0');
    } while (isdigit(source[++*index]));

    --*index;

    return number;
}

char *parse_identifier(const char *source, int *index)
{
    int i = (*index) + 1;
    while ((0 != isalnum(source[i]) || ('_' == source[i])))
    {
        ++i;
    }

    char *ident = calloc(sizeof(char), (i - *index) + 1);
    if (NULL == ident)
    {
        return NULL;
    }

    strncpy(ident, source + *index, i - *index);
    ident[i - *index] = '\0';

    *index = i - 1;
    return ident;
}

char *parse_string(const char *source, int *index)
{
    int i = *index;
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
                fprintf(stderr, "\\%c is not a valid esacpe sequence.\n", source[i + 1]);
                exit(1);
            }
        }

        ++char_count;
        ++i;
    }

    ++i;

    char *str = calloc(sizeof(char), char_count);
    if (NULL == str)
    {
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
                fprintf(stderr, "\\%c is not a valid esacpe sequence.\n", source[*index + ind + off + 1]);
                exit(1);
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

void tokenize_source(t_vector *tokens, const char *source)
{
    bool error = false;
    long line = 1, offset = 0;
    size_t length = strlen(source);
    char character = '\0';
    t_token *token = NULL;
    int number = 0;
    char *identifier;
    token = calloc(1, sizeof(t_token));
    if (NULL == token)
    {
        error = true;
        goto tokenize_exit;
    }

    for (int i = 0; i < length; ++i)
    {
        character = source[i];
        ++offset;
        if ('\n' == character)
        {
            ++line;
            offset = 1;
        }
        if (isspace(character))
        {
            continue;
        }

        token->line = line;
        token->offset = offset;
        token->type = T_UNKNOWN;
        token->content = "AAAA";

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
            token->type = T_OPEN_BRACKET;
            token->content = "{";
            break;
        }
        case '}':
        {
            token->type = T_CLOSE_BRACKET;
            token->content = "}";
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
        case ':':
        {
            token->type = T_COLON;
            token->content = ":";
            break;
        }
        case '/':
        {
            if ('/' == source[i + 1])
            {
                // Found a comment
                ++i;
                while ('\n' != source[i + 1])
                {
                    ++i;
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
        case '"':
        {
            token->type = T_STRING;
            ++i;
            identifier = parse_string(source, &i);
            if (NULL == identifier)
            {
                error = true;
                goto tokenize_exit;
            }
            token->content = identifier;
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
                number = parse_number(source, &i);
                token->content = calloc(sizeof(char), 11);
                if (NULL == token->content)
                {
                    error = true;
                    goto tokenize_exit;
                }
                sprintf(token->content, "%d", number);
                break;
            }

            if (isalpha(character))
            {
                token->type = T_IDENTIFIER;
                identifier = parse_identifier(source, &i);
                if (NULL == identifier)
                {
                    error = true;
                    goto tokenize_exit;
                }
                number = is_keyword(identifier);
                if (-1 != number)
                {
                    token->type = number;
                }
                token->content = identifier;
                break;
            }

            printf("Unrecognized character %c at %ld:%ld\n", character, line, offset);
            exit(1);
        }
        }

        if (VECTOR_SUCCESS != vector_push_back(tokens, &token))
        {
            error = true;
            goto tokenize_exit;
        }
        token = calloc(1, sizeof(t_token));
        if (NULL == token)
        {
            error = true;
            goto tokenize_exit;
        }
    }

    vector_shrink_to_fit(tokens);

tokenize_exit:
    if (NULL != token)
    {
        free(token);
    }
}
