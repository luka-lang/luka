#include "utest.h"

#include "lexer.h"

extern int lexer_is_keyword(const char *identifier);
extern char *lexer_lex_number(const char *source, size_t *index,
                              t_logger *logger);
extern char *lexer_lex_identifier(const char *source, size_t *index);
extern char *lexer_lex_string(const char *source, size_t *index,
                              t_logger *logger);

struct lexer
{
    t_logger *logger;
    size_t index;
};

UTEST_F_SETUP(lexer)
{
    utest_fixture->logger = LOGGER_initialize("/dev/null", 0);
    utest_fixture->index = 0;
    ASSERT_NE((char *) NULL, (char *) utest_fixture->logger);
}

UTEST_F_TEARDOWN(lexer)
{
    (void) LOGGER_free(utest_fixture->logger);
    utest_fixture->logger = NULL;
    ASSERT_EQ((char *) NULL, (char *) utest_fixture->logger);
}

UTEST(lexer, is_keyword_works_for_keywords)
{
    ASSERT_NE(-1, lexer_is_keyword("fn"));
    ASSERT_NE(-1, lexer_is_keyword("return"));
    ASSERT_NE(-1, lexer_is_keyword("if"));
    ASSERT_NE(-1, lexer_is_keyword("else"));
    ASSERT_NE(-1, lexer_is_keyword("let"));
    ASSERT_NE(-1, lexer_is_keyword("mut"));
    ASSERT_NE(-1, lexer_is_keyword("extern"));
    ASSERT_NE(-1, lexer_is_keyword("while"));
    ASSERT_NE(-1, lexer_is_keyword("break"));
    ASSERT_NE(-1, lexer_is_keyword("as"));
    ASSERT_NE(-1, lexer_is_keyword("struct"));
    ASSERT_NE(-1, lexer_is_keyword("enum"));
    ASSERT_NE(-1, lexer_is_keyword("import"));
    ASSERT_NE(-1, lexer_is_keyword("type"));
    ASSERT_NE(-1, lexer_is_keyword("null"));
    ASSERT_NE(-1, lexer_is_keyword("true"));
    ASSERT_NE(-1, lexer_is_keyword("false"));
    ASSERT_NE(-1, lexer_is_keyword("int"));
    ASSERT_NE(-1, lexer_is_keyword("char"));
    ASSERT_NE(-1, lexer_is_keyword("string"));
    ASSERT_NE(-1, lexer_is_keyword("void"));
    ASSERT_NE(-1, lexer_is_keyword("float"));
    ASSERT_NE(-1, lexer_is_keyword("double"));
    ASSERT_NE(-1, lexer_is_keyword("any"));
    ASSERT_NE(-1, lexer_is_keyword("bool"));
    ASSERT_NE(-1, lexer_is_keyword("u8"));
    ASSERT_NE(-1, lexer_is_keyword("u16"));
    ASSERT_NE(-1, lexer_is_keyword("u32"));
    ASSERT_NE(-1, lexer_is_keyword("u64"));
    ASSERT_NE(-1, lexer_is_keyword("s8"));
    ASSERT_NE(-1, lexer_is_keyword("s16"));
    ASSERT_NE(-1, lexer_is_keyword("s32"));
    ASSERT_NE(-1, lexer_is_keyword("s64"));
    ASSERT_NE(-1, lexer_is_keyword("f64"));
    ASSERT_NE(-1, lexer_is_keyword("f32"));
    ASSERT_NE(-1, lexer_is_keyword("s64"));
}

UTEST(lexer, is_keyword_works_for_not_keywords)
{
    ASSERT_EQ(-1, lexer_is_keyword("variable_that_is_not_a_keyword"));
    ASSERT_EQ(-1, lexer_is_keyword("a1"));
    ASSERT_EQ(-1, lexer_is_keyword("___asd___"));
    ASSERT_EQ(-1, lexer_is_keyword("s5"));
}

UTEST_F(lexer, lex_number_works_for_integers)
{
    utest_fixture->index = 0;
    ASSERT_STREQ("123", lexer_lex_number("123", &utest_fixture->index,
                                         utest_fixture->logger));
    ASSERT_EQ((size_t) 2, utest_fixture->index);
}

UTEST_F(lexer, lex_number_works_for_floats)
{
    utest_fixture->index = 0;
    ASSERT_STREQ("123.5", lexer_lex_number("123.5", &utest_fixture->index,
                                           utest_fixture->logger));
    ASSERT_EQ((size_t) 4, utest_fixture->index);
}

UTEST_F(lexer, lex_number_works_not_from_start)
{
    utest_fixture->index = 8;
    ASSERT_STREQ("123.5",
                 lexer_lex_number("let a = 123.5;", &utest_fixture->index,
                                  utest_fixture->logger));
    ASSERT_EQ((size_t) 12, utest_fixture->index);
}

UTEST_F(lexer, lex_number_works_with_f_suffix)
{
    utest_fixture->index = 0;
    ASSERT_STREQ("3.14", lexer_lex_number("3.14f", &utest_fixture->index,
                                          utest_fixture->logger));
    ASSERT_EQ((size_t) 4, utest_fixture->index);
}

UTEST_F(lexer, lex_identifier_empty_string)
{
    utest_fixture->index = 0;
    ASSERT_STREQ("", lexer_lex_identifier("", &utest_fixture->index));
    ASSERT_EQ((size_t) 0, utest_fixture->index);
}

UTEST_F(lexer, lex_identifier_invalid_identifier)
{
    utest_fixture->index = 0;
    ASSERT_STREQ("", lexer_lex_identifier("1 + 2", &utest_fixture->index));
    ASSERT_EQ((size_t) 0, utest_fixture->index);
}

UTEST_F(lexer, lex_identifier_valid_identifiers)
{
    utest_fixture->index = 0;
    ASSERT_STREQ("ident", lexer_lex_identifier("ident", &utest_fixture->index));
    ASSERT_EQ((size_t) 4, utest_fixture->index);

    utest_fixture->index = 0;
    ASSERT_STREQ("ident2",
                 lexer_lex_identifier("ident2", &utest_fixture->index));
    ASSERT_EQ((size_t) 5, utest_fixture->index);

    utest_fixture->index = 0;
    ASSERT_STREQ("my_ident",
                 lexer_lex_identifier("my_ident", &utest_fixture->index));
    ASSERT_EQ((size_t) 7, utest_fixture->index);
}

UTEST_F(lexer, lex_string_empty_string)
{
    utest_fixture->index = 1;
    ASSERT_STREQ("", lexer_lex_string("\"\"", &utest_fixture->index,
                                      utest_fixture->logger));
    ASSERT_EQ((size_t) 1, utest_fixture->index);
}

UTEST_F(lexer, lex_string_escape_characters)
{
    utest_fixture->index = 1;
    ASSERT_STREQ("\n", lexer_lex_string("\"\\n\"", &utest_fixture->index,
                                        utest_fixture->logger));
    ASSERT_EQ((size_t) 3, utest_fixture->index);

    utest_fixture->index = 1;
    ASSERT_STREQ("\t", lexer_lex_string("\"\\t\"", &utest_fixture->index,
                                        utest_fixture->logger));
    ASSERT_EQ((size_t) 3, utest_fixture->index);

    utest_fixture->index = 1;
    ASSERT_STREQ("\\", lexer_lex_string("\"\\\\\"", &utest_fixture->index,
                                        utest_fixture->logger));
    ASSERT_EQ((size_t) 3, utest_fixture->index);

    utest_fixture->index = 1;
    ASSERT_STREQ("\"", lexer_lex_string("\"\\\"\"", &utest_fixture->index,
                                        utest_fixture->logger));
    ASSERT_EQ((size_t) 3, utest_fixture->index);
}

UTEST_F(lexer, lex_string_normal_strings)
{
    utest_fixture->index = 1;
    ASSERT_STREQ("foo", lexer_lex_string("\"foo\"", &utest_fixture->index,
                                        utest_fixture->logger));
    ASSERT_EQ((size_t) 4, utest_fixture->index);

    utest_fixture->index = 1;
    ASSERT_STREQ("bar", lexer_lex_string("\"bar\"", &utest_fixture->index,
                                         utest_fixture->logger));
    ASSERT_EQ((size_t) 4, utest_fixture->index);

    utest_fixture->index = 1;
    ASSERT_STREQ("hello world!", lexer_lex_string("\"hello world!\"", &utest_fixture->index,
                                         utest_fixture->logger));
    ASSERT_EQ((size_t) 13, utest_fixture->index);
}
