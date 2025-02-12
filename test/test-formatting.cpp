#include "test-common.h"

#include <argum/formatting.h>

#include <doctest/doctest.h>

#include <ostream>

using namespace Argum;
using namespace std;

using namespace std::literals;

TEST_SUITE("formatting") {

TEST_CASE( "narrow formatting" ) {

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
    CHECK(format("{1}", "abc"sv) == "abc");
    CHECK(format("{1}", "abc"s) == "abc");
    CHECK(format("{1}", "abc") == "abc");
    CHECK(format("{1}", L"abc"sv) == "abc");
    CHECK(format("{1}", L"abc"s) == "abc");
    CHECK(format("{1}", L"abc") == "abc");
    CHECK(format("{1}", true) == to_string(true));
    CHECK(format("{1}", 1.2) == to_string(1.2));
}

TEST_CASE( "wide formatting" ) {

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
    CHECK(format(L"{1}", "abc"sv) == L"abc");
    CHECK(format(L"{1}", "abc"s) == L"abc");
    CHECK(format(L"{1}", "abc") == L"abc");
    CHECK(format(L"{1}", L"abc"sv) == L"abc");
    CHECK(format(L"{1}", L"abc"s) == L"abc");
    CHECK(format(L"{1}", L"abc") == L"abc");
    CHECK(format(L"{1}", true) == to_wstring(true));
    CHECK(format(L"{1}", 1.2) == to_wstring(1.2));
}

TEST_CASE( "indent" ) {
    CHECK(indent("", 0) == "");
    CHECK(indent("", 100) == "");
    CHECK(indent("abc", 3) == "abc");
    CHECK(indent("a\nb\nc", 0) == "a\nb\nc");
    CHECK(indent("a\nb\nc", 1) == "a\n b\n c");
}

TEST_CASE( "word wrap" ) {

    CHECK(wordWrap("", 0) == "");
    CHECK(wordWrap("a", 0) == "");
    CHECK(wordWrap("ab", 0) == "");

    CHECK(wordWrap("", 1) == "");
    CHECK(wordWrap("a", 1) == "a");
    CHECK(wordWrap("ab", 1) == "a\nb");
    CHECK(wordWrap("a b", 1) == "a\nb");
    CHECK(wordWrap("a\nb", 1) == "a\nb");
    CHECK(wordWrap("ab\n", 1) == "a\nb\n");
    CHECK(wordWrap("\nab", 1) == "\na\nb");

    CHECK(wordWrap("", 2) == "");
    CHECK(wordWrap("a", 2) == "a");
    CHECK(wordWrap("ab", 2) == "ab");
    CHECK(wordWrap("abc", 2) == "ab\nc");
    CHECK(wordWrap(" abc", 2) == "\nab\nc");
    CHECK(wordWrap("a bc", 2) == "a\nbc");
    CHECK(wordWrap("ab c", 2) == "ab\nc");
    CHECK(wordWrap("a\nbc", 2) == "a\nbc");
    CHECK(wordWrap("ab\nc", 2) == "ab\nc");
}

}
