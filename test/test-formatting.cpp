#include <modern-arg-parser/formatting.h>

#include "catch.hpp"

using namespace MArgP;
using namespace std;

using namespace std::literals;

template<class Char, StreamPrintable<Char>... Args>
auto formatToStr(const Char * fmt, Args && ...args) {
    return (std::basic_ostringstream<Char>() << MArgP::format(fmt, std::forward<Args>(args)...)).str();
}


TEST_CASE( "narrow formatting" , "[formatting]") {

    CHECK(formatToStr("") == "");
    CHECK(formatToStr("abc") == "abc");
    CHECK(formatToStr("{") == "{");
    CHECK(formatToStr("{1") == "{1");
    CHECK(formatToStr("a{1") == "a{1");
    CHECK(formatToStr("{{") == "{");
    CHECK(formatToStr("{{1") == "{1");
    CHECK(formatToStr("{{1}") == "{1}");
    CHECK(formatToStr("{1}") == "{1}");
    CHECK(formatToStr("{1a}") == "{1a}");
    CHECK(formatToStr("{+1}") == "{+1}");
    CHECK(formatToStr("{1.}") == "{1.}");
    CHECK(formatToStr("{2}") == "{2}");
    CHECK(formatToStr("{12345678901234567890}") == "{12345678901234567890}");

    CHECK(formatToStr("", 42) == "");
    CHECK(formatToStr("abc", 42) == "abc");
    CHECK(formatToStr("{", 42) == "{");
    CHECK(formatToStr("{1", 42) == "{1");
    CHECK(formatToStr("a{1", 42) == "a{1");
    CHECK(formatToStr("{{", 42) == "{");
    CHECK(formatToStr("{{1", 42) == "{1");
    CHECK(formatToStr("{{1}", 42) == "{1}");
    CHECK(formatToStr("{1}", 42) == "42");
    CHECK(formatToStr("{1a}", 42) == "{1a}");
    CHECK(formatToStr("{+1}", 42) == "{+1}");
    CHECK(formatToStr("{1.}", 42) == "{1.}");
    CHECK(formatToStr("{2}", 42) == "{2}");
    CHECK(formatToStr("{12345678901234567890}", 42) == "{12345678901234567890}");
}

TEST_CASE( "wide formatting" , "[formatting]") {

    CHECK(formatToStr(L"") == L"");
    CHECK(formatToStr(L"abc") == L"abc");
    CHECK(formatToStr(L"{") == L"{");
    CHECK(formatToStr(L"{1") == L"{1");
    CHECK(formatToStr(L"a{1") == L"a{1");
    CHECK(formatToStr(L"{{") == L"{");
    CHECK(formatToStr(L"{{1") == L"{1");
    CHECK(formatToStr(L"{{1}") == L"{1}");
    CHECK(formatToStr(L"{1}") == L"{1}");
    CHECK(formatToStr(L"{1a}") == L"{1a}");
    CHECK(formatToStr(L"{+1}") == L"{+1}");
    CHECK(formatToStr(L"{1.}") == L"{1.}");
    CHECK(formatToStr(L"{2}") == L"{2}");
    CHECK(formatToStr(L"{12345678901234567890}") == L"{12345678901234567890}");


    CHECK(formatToStr(L"", 42) == L"");
    CHECK(formatToStr(L"abc", 42) == L"abc");
    CHECK(formatToStr(L"{", 42) == L"{");
    CHECK(formatToStr(L"{1", 42) == L"{1");
    CHECK(formatToStr(L"a{1", 42) == L"a{1");
    CHECK(formatToStr(L"{{", 42) == L"{");
    CHECK(formatToStr(L"{{1", 42) == L"{1");
    CHECK(formatToStr(L"{{1}", 42) == L"{1}");
    CHECK(formatToStr(L"{1}", 42) == L"42");
    CHECK(formatToStr(L"{1a}", 42) == L"{1a}");
    CHECK(formatToStr(L"{+1}", 42) == L"{+1}");
    CHECK(formatToStr(L"{1.}", 42) == L"{1.}");
    CHECK(formatToStr(L"{2}", 42) == L"{2}");
    CHECK(formatToStr(L"{12345678901234567890}", 42) == L"{12345678901234567890}");
}


