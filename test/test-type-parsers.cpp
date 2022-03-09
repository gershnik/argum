
//for testing let it throw exception rather than crash
[[noreturn]] void reportInvalidArgument(const char * message);
#define ARGUM_INVALID_ARGUMENT(message) reportInvalidArgument(message)

#include <argum/type-parsers.h>

#include "catch.hpp"


using namespace Argum;
using namespace std;

using Catch::Matchers::Message;

#define NOT_A_NUMBER "is not a number"
#define OUT_OF_RANGE "is out of range"
#define EXPECT_INT_FAILURE(type, expr, err) \
    CHECK_THROWS_MATCHES(parseIntegral<type>(expr), \
                         BasicParser<decay_t<decltype(expr[0])>>::ValidationError, \
                         Message(format("invalid arguments: value \"{1}\" {2}", (expr), (err))));
#define EXPECT_INT_SUCCESS(type, expr, res) CHECK(parseIntegral<type>(expr) == res);

#define EXPECT_CHOICE_FAILURE(expr, choices) \
    CHECK_THROWS_MATCHES(parser.parse(expr), \
                         BasicParser<decay_t<decltype(expr[0])>>::ValidationError, \
                         Message(format("invalid arguments: value \"{1}\" is not one of the valid choices {{{2}}", (expr), (choices))));

TEST_CASE( "Integral Bool", "[type-parsers") {

    EXPECT_INT_FAILURE(bool, "", NOT_A_NUMBER);
    EXPECT_INT_FAILURE(bool, "a", NOT_A_NUMBER);
    EXPECT_INT_FAILURE(bool, "1a", NOT_A_NUMBER);
    EXPECT_INT_FAILURE(bool, "5", OUT_OF_RANGE);
    EXPECT_INT_FAILURE(bool, "-1", OUT_OF_RANGE);

    EXPECT_INT_SUCCESS(bool, "0", false);
    EXPECT_INT_SUCCESS(bool, "1", true);
    EXPECT_INT_SUCCESS(bool, "  0", false);
    EXPECT_INT_SUCCESS(bool, "  0  ", false);
    EXPECT_INT_SUCCESS(bool, "  -0  ", false);
    EXPECT_INT_SUCCESS(bool, "0x0", false);
    EXPECT_INT_SUCCESS(bool, "01", true);
}

TEST_CASE( "Integral Char", "[type-parsers") {

    EXPECT_INT_FAILURE(char, "", NOT_A_NUMBER);
    EXPECT_INT_FAILURE(char, "a", NOT_A_NUMBER);
    EXPECT_INT_FAILURE(char, L"1a", NOT_A_NUMBER);
    EXPECT_INT_FAILURE(char, "257", OUT_OF_RANGE);
    
    EXPECT_INT_SUCCESS(char, "5", char(5));
    EXPECT_INT_SUCCESS(char, L"-1", char(-1));
    EXPECT_INT_SUCCESS(char, "65", 'A');
}

TEST_CASE( "Integral Int", "[type-parsers") {

    EXPECT_INT_FAILURE(int, "", NOT_A_NUMBER);
    EXPECT_INT_FAILURE(int, "a", NOT_A_NUMBER);
    EXPECT_INT_FAILURE(int, "1a", NOT_A_NUMBER);
    EXPECT_INT_FAILURE(int, "12345678901234", OUT_OF_RANGE);
    
    EXPECT_INT_SUCCESS(int, "5", 5);
    EXPECT_INT_SUCCESS(int, L"-1", -1);
    EXPECT_INT_SUCCESS(int, "65", 65);
}

TEST_CASE( "Simple Choice", "[type-parsers") {

    ChoiceParser parser;
    parser.addChoice("a");
    parser.addChoice("b");

    CHECK(parser.parse("a") == 0);
    CHECK(parser.parse("b") == 1);
    CHECK(parser.parse("A") == 0);
    CHECK(parser.parse("B") == 1);

    EXPECT_CHOICE_FAILURE("c", "a, b");
    EXPECT_CHOICE_FAILURE(" a", "a, b");
    EXPECT_CHOICE_FAILURE("b ", "a, b");
}

TEST_CASE( "Case Sensitive Choice", "[type-parsers") {

    WChoiceParser parser({.caseSensitive = true});
    parser.addChoice(L"a");
    parser.addChoice(L"b");

    CHECK(parser.parse(L"a") == 0);
    CHECK(parser.parse(L"b") == 1);

    EXPECT_CHOICE_FAILURE(L"c", L"a, b");
    EXPECT_CHOICE_FAILURE(L" a", L"a, b");
    EXPECT_CHOICE_FAILURE(L"b ", L"a, b");
    EXPECT_CHOICE_FAILURE(L"A", L"a, b");
    EXPECT_CHOICE_FAILURE(L"B", L"a, b");
}

TEST_CASE( "Escaped Choice", "[type-parsers") {

    ChoiceParser parser;
    parser.addChoice("a|");
    parser.addChoice("(b");

    CHECK(parser.parse("a|") == 0);
    CHECK(parser.parse("(b") == 1);
    CHECK(parser.parse("A|") == 0);
    CHECK(parser.parse("(B") == 1);

    EXPECT_CHOICE_FAILURE("a[", "a|, (b");
    EXPECT_CHOICE_FAILURE("[b", "a|, (b");
}

TEST_CASE( "Multi Choice", "[type-parsers") {

    ChoiceParser parser;
    parser.addChoice("a|", "b"sv, "|c"s);
    parser.addChoice("Q");

    CHECK(parser.parse("a|") == 0);
    CHECK(parser.parse("B") == 0);
    CHECK(parser.parse("|c") == 0);
    CHECK(parser.parse("q") == 1);
    
    EXPECT_CHOICE_FAILURE("m", "a|, b, |c, Q");
    EXPECT_CHOICE_FAILURE("", "a|, b, |c, Q");
}

TEST_CASE( "Else Choice", "[type-parsers") {

    ChoiceParser parser({.allowElse = true});
    parser.addChoice("a|", "b"sv, "|c"s);
    parser.addChoice("Q");

    CHECK(parser.parse("a|") == 0);
    CHECK(parser.parse("B") == 0);
    CHECK(parser.parse("|c") == 0);
    CHECK(parser.parse("q") == 1);
    CHECK(parser.parse("m") == 2);
    CHECK(parser.parse("") == 2);
}

TEST_CASE( "Boolean", "[type-parsers") {

    {
        BooleanParser parser;

        CHECK(parser.parse("1"));
        CHECK(parser.parse("on"));
        CHECK(parser.parse("true"));
        CHECK(parser.parse("yes"));
        CHECK(!parser.parse("0"));
        CHECK(!parser.parse("off"));
        CHECK(!parser.parse("false"));
        CHECK(!parser.parse("no"));

        EXPECT_CHOICE_FAILURE("y", "0, false, off, no, 1, true, on, yes");
    }

    {
        WBooleanParser parser;

        CHECK(parser.parse(L"1"));
        CHECK(parser.parse(L"on"));
        CHECK(parser.parse(L"true"));
        CHECK(parser.parse(L"yes"));
        CHECK(!parser.parse(L"0"));
        CHECK(!parser.parse(L"off"));
        CHECK(!parser.parse(L"false"));
        CHECK(!parser.parse(L"no"));

        EXPECT_CHOICE_FAILURE(L"y", L"0, false, off, no, 1, true, on, yes");
    }

}

