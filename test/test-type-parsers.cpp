
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

