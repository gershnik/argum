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

#if MARGP_UTF_CHAR_SUPPORTED

TEST_CASE( "utf-8 formatting" , "[formatting]") {

    CHECK(formatToStr(u8"") == u8"");
    CHECK(formatToStr(u8"", 42) == u8"");
    CHECK(formatToStr(u8"abc", 42) == u8"abc");
    CHECK(formatToStr(u8"{", 42) == u8"{");
    CHECK(formatToStr(u8"{1", 42) == u8"{1");
    CHECK(formatToStr(u8"a{1", 42) == u8"a{1");
    CHECK(formatToStr(u8"{{", 42) == u8"{");
    CHECK(formatToStr(u8"{{1", 42) == u8"{1");
    CHECK(formatToStr(u8"{{1}", 42) == u8"{1}");
    CHECK(formatToStr(u8"{1}", 42) == u8"42");
    CHECK(formatToStr(u8"{1a}", 42) == u8"{1a}");
    CHECK(formatToStr(u8"{+1}", 42) == u8"{+1}");
    CHECK(formatToStr(u8"{1.}", 42) == u8"{1.}");
    CHECK(formatToStr(u8"{2}", 42) == u8"{2}");
    CHECK(formatToStr(u8"{12345678901234567890}", 42) == u8"{12345678901234567890}");
}

#endif
