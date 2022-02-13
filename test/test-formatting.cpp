#include <modern-arg-parser/formatting.h>

#include "catch.hpp"

using namespace MArgP;
using namespace std;

using namespace std::literals;


TEST_CASE( "narrow formatting" , "[formatting]") {

    CHECK(format("") == "");
    CHECK(format("abc") == "abc");
    CHECK(format("{") == "{");
    CHECK(format("{1") == "{1");
    CHECK(format("a{1") == "a{1");
    CHECK(format("{{") == "{");
    CHECK(format("{{1") == "{1");
    CHECK(format("{{1}") == "{1}");
    CHECK(format("{1}") == "{1}");
    CHECK(format("{1a}") == "{1a}");
    CHECK(format("{+1}") == "{+1}");
    CHECK(format("{1.}") == "{1.}");
    CHECK(format("{2}") == "{2}");
    CHECK(format("{12345678901234567890}") == "{12345678901234567890}");

    CHECK(format("", 42) == "");
    CHECK(format("abc", 42) == "abc");
    CHECK(format("{", 42) == "{");
    CHECK(format("{1", 42) == "{1");
    CHECK(format("a{1", 42) == "a{1");
    CHECK(format("{{", 42) == "{");
    CHECK(format("{{1", 42) == "{1");
    CHECK(format("{{1}", 42) == "{1}");
    CHECK(format("{1}", 42) == "42");
    CHECK(format("{1a}", 42) == "{1a}");
    CHECK(format("{+1}", 42) == "{+1}");
    CHECK(format("{1.}", 42) == "{1.}");
    CHECK(format("{2}", 42) == "{2}");
    CHECK(format("{12345678901234567890}", 42) == "{12345678901234567890}");
}

TEST_CASE( "wide formatting" , "[formatting]") {

    CHECK(format(L"") == L"");
    CHECK(format(L"abc") == L"abc");
    CHECK(format(L"{") == L"{");
    CHECK(format(L"{1") == L"{1");
    CHECK(format(L"a{1") == L"a{1");
    CHECK(format(L"{{") == L"{");
    CHECK(format(L"{{1") == L"{1");
    CHECK(format(L"{{1}") == L"{1}");
    CHECK(format(L"{1}") == L"{1}");
    CHECK(format(L"{1a}") == L"{1a}");
    CHECK(format(L"{+1}") == L"{+1}");
    CHECK(format(L"{1.}") == L"{1.}");
    CHECK(format(L"{2}") == L"{2}");
    CHECK(format(L"{12345678901234567890}") == L"{12345678901234567890}");


    CHECK(format(L"", 42) == L"");
    CHECK(format(L"abc", 42) == L"abc");
    CHECK(format(L"{", 42) == L"{");
    CHECK(format(L"{1", 42) == L"{1");
    CHECK(format(L"a{1", 42) == L"a{1");
    CHECK(format(L"{{", 42) == L"{");
    CHECK(format(L"{{1", 42) == L"{1");
    CHECK(format(L"{{1}", 42) == L"{1}");
    CHECK(format(L"{1}", 42) == L"42");
    CHECK(format(L"{1a}", 42) == L"{1a}");
    CHECK(format(L"{+1}", 42) == L"{+1}");
    CHECK(format(L"{1.}", 42) == L"{1.}");
    CHECK(format(L"{2}", 42) == L"{2}");
    CHECK(format(L"{12345678901234567890}", 42) == L"{12345678901234567890}");
}


