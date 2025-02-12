
#include "test-common.h"

#include <argum/type-parsers.h>

#include <doctest/doctest.h>


using namespace Argum;
using namespace std;


#define NOT_A_NUMBER "is not a number"
#define OUT_OF_RANGE "is out of range"

#ifndef ARGUM_NO_THROW
    #define EXPECT_INT_FAILURE(type, expr, err) \
        CHECK_THROWS_WITH_AS(ARGUM_EXPECTED_VALUE(parseIntegral<type>(expr)), \
                             format("invalid arguments: value \"{1}\" {2}", (expr), (err)).c_str(), \
                             BasicParser<remove_cvref_t<decltype(expr[0])>>::ValidationError)
    
    #define EXPECT_INT_SUCCESS(type, expr, res) CHECK(ARGUM_EXPECTED_VALUE(parseIntegral<type>(expr)) == res)

    #define EXPECT_FLOAT_FAILURE(type, expr, err) \
        CHECK_THROWS_WITH_AS(ARGUM_EXPECTED_VALUE(parseFloatingPoint<type>(expr)), \
                             format("invalid arguments: value \"{1}\" {2}", (expr), (err)).c_str(), \
                             BasicParser<remove_cvref_t<decltype(expr[0])>>::ValidationError)

    #define EXPECT_FLOAT_SUCCESS(type, expr, res) CHECK(ARGUM_EXPECTED_VALUE(parseFloatingPoint<type>(expr)) == res)

    #define EXPECT_CHOICE_FAILURE(expr, choices) \
        CHECK_THROWS_WITH_AS(ARGUM_EXPECTED_VALUE(parser.parse(expr)), \
                             format("invalid arguments: value \"{1}\" is not one of the valid choices {{{2}}", (expr), (choices)).c_str(), \
                             BasicParser<remove_cvref_t<decltype(expr[0])>>::ValidationError)
    #define EXPECT_CHOICE_SUCCESS(expr, expected) \
        CHECK(ARGUM_EXPECTED_VALUE(parser.parse(expr)) == expected)

#else
    #define EXPECT_INT_FAILURE(type, expr, err) {\
        auto error = parseIntegral<type>(expr).error(); \
        CHECK(error); \
        if (!error) abort(); \
        CHECK(toString<char>(error->message()) == format("invalid arguments: value \"{1}\" {2}", (expr), (err))); \
    }

    #define EXPECT_INT_SUCCESS(type, expr, res) {\
        auto result = parseIntegral<type>(expr); \
        CHECK(result); \
        if (!result) abort(); \
        CHECK(*result == res); \
    }

    #define EXPECT_FLOAT_FAILURE(type, expr, err) {\
        auto error = parseFloatingPoint<type>(expr).error(); \
        CHECK(error); \
        if (!error) abort(); \
        CHECK(toString<char>(error->message()) == format("invalid arguments: value \"{1}\" {2}", (expr), (err))); \
    }

    #define EXPECT_FLOAT_SUCCESS(type, expr, res) {\
        auto result = parseFloatingPoint<type>(expr); \
        CHECK(result); \
        if (!result) abort(); \
        CHECK(*result == res); \
    }

    #define EXPECT_CHOICE_FAILURE(expr, choices) {\
        auto error = parser.parse(expr).error(); \
        CHECK(error); \
        if (!error) abort(); \
        CHECK(toString<char>(error->message()) == \
              format("invalid arguments: value \"{1}\" is not one of the valid choices {{{2}}", (expr), (choices))); \
    }

    #define EXPECT_CHOICE_SUCCESS(expr, expected) {\
        auto result = parser.parse(expr); \
        CHECK(result); \
        if (!result) abort(); \
        CHECK(*result == expected); \
    }
#endif

TEST_SUITE("type-parsers") {

TEST_CASE( "Integral Bool" ) {

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

TEST_CASE( "Integral Char" ) {

    EXPECT_INT_FAILURE(char, "", NOT_A_NUMBER);
    EXPECT_INT_FAILURE(char, "a", NOT_A_NUMBER);
    EXPECT_INT_FAILURE(char, L"1a", NOT_A_NUMBER);
    EXPECT_INT_FAILURE(char, "257", OUT_OF_RANGE);
    
    EXPECT_INT_SUCCESS(char, "5", char(5));
    EXPECT_INT_SUCCESS(char, L"-1", char(-1));
    EXPECT_INT_SUCCESS(char, "65", 'A');
}

TEST_CASE( "Integral Int" ) {

    EXPECT_INT_FAILURE(int, "", NOT_A_NUMBER);
    EXPECT_INT_FAILURE(int, "a", NOT_A_NUMBER);
    EXPECT_INT_FAILURE(int, "1a", NOT_A_NUMBER);
    EXPECT_INT_FAILURE(int, "12345678901234", OUT_OF_RANGE);
    
    EXPECT_INT_SUCCESS(int, "5", 5);
    EXPECT_INT_SUCCESS(int, L"-1", -1);
    EXPECT_INT_SUCCESS(int, "65", 65);
}

TEST_CASE( "Floating Float" ) {

    EXPECT_FLOAT_FAILURE(float, "", NOT_A_NUMBER);
    EXPECT_FLOAT_FAILURE(float, "a", NOT_A_NUMBER);
    EXPECT_FLOAT_FAILURE(float, "1a", NOT_A_NUMBER);
    EXPECT_FLOAT_FAILURE(float, "12345678901234E3456", OUT_OF_RANGE);
    
    EXPECT_FLOAT_SUCCESS(float, "5", 5.f);
    EXPECT_FLOAT_SUCCESS(float, L"-1", -1.f);
    EXPECT_FLOAT_SUCCESS(float, "65.3", 65.3f);
}

TEST_CASE( "Floating Double" ) {

    EXPECT_FLOAT_FAILURE(double, "", NOT_A_NUMBER);
    EXPECT_FLOAT_FAILURE(double, "a", NOT_A_NUMBER);
    EXPECT_FLOAT_FAILURE(double, "1a", NOT_A_NUMBER);
    EXPECT_FLOAT_FAILURE(double, "12345678901234E3456", OUT_OF_RANGE);
    
    EXPECT_FLOAT_SUCCESS(double, "5", 5.);
    EXPECT_FLOAT_SUCCESS(double, L"-1", -1.);
    EXPECT_FLOAT_SUCCESS(double, "65.3", 65.3);
}

TEST_CASE( "Floating Long Double" ) {

    EXPECT_FLOAT_FAILURE(long double, "", NOT_A_NUMBER);
    EXPECT_FLOAT_FAILURE(long double, "a", NOT_A_NUMBER);
    EXPECT_FLOAT_FAILURE(long double, "1a", NOT_A_NUMBER);
    EXPECT_FLOAT_FAILURE(long double, "12345678901279934E345678", OUT_OF_RANGE);
    
    EXPECT_FLOAT_SUCCESS(long double, "5", 5.l);
    EXPECT_FLOAT_SUCCESS(long double, L"-1", -1.l);
    EXPECT_FLOAT_SUCCESS(long double, "65.3", 65.3l);
}

TEST_CASE( "Simple Choice" ) {

    ChoiceParser parser;
    parser.addChoice("a");
    parser.addChoice("b");

    EXPECT_CHOICE_SUCCESS("a", 0);
    EXPECT_CHOICE_SUCCESS("b", 1);
    EXPECT_CHOICE_SUCCESS("A", 0);
    EXPECT_CHOICE_SUCCESS("B", 1);

    EXPECT_CHOICE_FAILURE("c", "a, b");
    EXPECT_CHOICE_FAILURE(" a", "a, b");
    EXPECT_CHOICE_FAILURE("b ", "a, b");
}

TEST_CASE( "Case Sensitive Choice" ) {

    WChoiceParser parser({.caseSensitive = true});
    parser.addChoice(L"a");
    parser.addChoice(L"b");

    EXPECT_CHOICE_SUCCESS(L"a", 0);
    EXPECT_CHOICE_SUCCESS(L"b", 1);

    EXPECT_CHOICE_FAILURE(L"c", L"a, b");
    EXPECT_CHOICE_FAILURE(L" a", L"a, b");
    EXPECT_CHOICE_FAILURE(L"b ", L"a, b");
    EXPECT_CHOICE_FAILURE(L"A", L"a, b");
    EXPECT_CHOICE_FAILURE(L"B", L"a, b");
}

TEST_CASE( "Escaped Choice" ) {

    ChoiceParser parser;
    parser.addChoice("a|");
    parser.addChoice("(b");

    EXPECT_CHOICE_SUCCESS("a|", 0);
    EXPECT_CHOICE_SUCCESS("(b", 1);
    EXPECT_CHOICE_SUCCESS("A|", 0);
    EXPECT_CHOICE_SUCCESS("(B", 1);

    EXPECT_CHOICE_FAILURE("a[", "a|, (b");
    EXPECT_CHOICE_FAILURE("[b", "a|, (b");
}

TEST_CASE( "Multi Choice" ) {

    ChoiceParser parser;
    parser.addChoice("a|", "b"sv, "|c"s);
    parser.addChoice("Q");

    EXPECT_CHOICE_SUCCESS("a|", 0);
    EXPECT_CHOICE_SUCCESS("B", 0);
    EXPECT_CHOICE_SUCCESS("|c", 0);
    EXPECT_CHOICE_SUCCESS("q", 1);
    
    EXPECT_CHOICE_FAILURE("m", "a|, b, |c, Q");
    EXPECT_CHOICE_FAILURE("", "a|, b, |c, Q");
}

TEST_CASE( "Else Choice" ) {

    ChoiceParser parser({.allowElse = true});
    parser.addChoice("a|", "b"sv, "|c"s);
    parser.addChoice("Q");

    EXPECT_CHOICE_SUCCESS("a|", 0);
    EXPECT_CHOICE_SUCCESS("B", 0);
    EXPECT_CHOICE_SUCCESS("|c", 0);
    EXPECT_CHOICE_SUCCESS("q", 1);
    EXPECT_CHOICE_SUCCESS("m", 2);
    EXPECT_CHOICE_SUCCESS("", 2);
}

TEST_CASE( "Boolean" ) {

    {
        BooleanParser parser;

        EXPECT_CHOICE_SUCCESS("1", true);
        EXPECT_CHOICE_SUCCESS("on", true);
        EXPECT_CHOICE_SUCCESS("true", true);
        EXPECT_CHOICE_SUCCESS("yes", true);
        EXPECT_CHOICE_SUCCESS("0", false);
        EXPECT_CHOICE_SUCCESS("off", false);
        EXPECT_CHOICE_SUCCESS("false", false);
        EXPECT_CHOICE_SUCCESS("no", false);

        EXPECT_CHOICE_FAILURE("y", "0, false, off, no, 1, true, on, yes");
    }

    {
        WBooleanParser parser;

        EXPECT_CHOICE_SUCCESS(L"1", true);
        EXPECT_CHOICE_SUCCESS(L"on", true);
        EXPECT_CHOICE_SUCCESS(L"true", true);
        EXPECT_CHOICE_SUCCESS(L"yes", true);
        EXPECT_CHOICE_SUCCESS(L"0", false);
        EXPECT_CHOICE_SUCCESS(L"off", false);
        EXPECT_CHOICE_SUCCESS(L"false", false);
        EXPECT_CHOICE_SUCCESS(L"no", false);

        EXPECT_CHOICE_FAILURE(L"y", L"0, false, off, no, 1, true, on, yes");
    }

}

}
