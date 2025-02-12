#include "test-common.h"

#include <argum/expected.h>

#include <doctest/doctest.h>

#include <ostream>

using namespace Argum;
using namespace std;

using namespace std::literals;

#ifndef ARGUM_NO_THROW
    #define XREQUIRE(x) REQUIRE(x)
#else
    #define XREQUIRE(x) { bool res = bool(x); CHECK(res); if (!res) abort(); }
#endif 

namespace {
    struct foo {
        foo(int i):val(to_string(i)) { }
        foo(string s):val(s) {}
        foo(int i, char c):val(to_string(i) + c) {}

        string val;
    };

    struct Exc : public ParsingException {
        ARGUM_IMPLEMENT_EXCEPTION(Exc, ParsingException, Error::UserError)

        Exc(): ParsingException(ErrorCode, "blah") {
        }
        Exc(string s): ParsingException(ErrorCode, s) {
        }
    };
}

TEST_SUITE("expected") {

TEST_CASE( "expected properties" ) {
    static_assert(!is_default_constructible_v<Expected<foo>>);
    static_assert(is_copy_constructible_v<Expected<foo>>);
    static_assert(is_nothrow_move_constructible_v<Expected<foo>>);
    static_assert(is_copy_assignable_v<Expected<foo>>);
    static_assert(is_nothrow_move_assignable_v<Expected<foo>>);
    static_assert(is_constructible_v<Expected<foo>, foo>);
    static_assert(is_constructible_v<Expected<foo>, int>);
    static_assert(is_constructible_v<Expected<foo>, string>);
    static_assert(is_constructible_v<Expected<foo>, decltype(make_shared<Exc>())>);

    static_assert(is_nothrow_default_constructible_v<Expected<void>>);
    static_assert(is_copy_constructible_v<Expected<void>>);
    static_assert(is_nothrow_move_constructible_v<Expected<void>>);
    static_assert(is_copy_assignable_v<Expected<void>>);
    static_assert(is_nothrow_move_assignable_v<Expected<void>>);
    static_assert(!is_constructible_v<Expected<void>, int>);
    static_assert(!is_constructible_v<Expected<void>, monostate>);
    static_assert(is_constructible_v<Expected<void>, decltype(make_shared<Exc>())>);

    static_assert(is_constructible_v<Expected<void>, Expected<foo>>);
    static_assert(is_constructible_v<Expected<int>, Expected<short>>);
}

TEST_CASE( "expected value" ) {
    {
        Expected<foo> exp(3);
        XREQUIRE(exp);
        CHECK(!!exp);
        CHECK(exp.value().val == "3");
        CHECK(exp->val == "3");
        CHECK((*exp).val == "3");
    }
    {
        XREQUIRE(Expected<foo>(3));
        CHECK(!!Expected<foo>(3));
        CHECK(Expected<foo>(3).value().val == "3");
        CHECK(Expected<foo>(3)->val == "3");
        CHECK((*Expected<foo>(3)).val == "3");
    }    
    {    
        const Expected<foo> exp(3);
        XREQUIRE(exp);
        CHECK(!!exp);
        CHECK(exp.value().val == "3");
        CHECK(exp->val == "3");
        CHECK((*exp).val == "3");
    }

    CHECK(Expected<foo>(3, 'a')->val == "3a");
    CHECK(Expected<foo>(foo("x"s))->val == "x");
    
}

TEST_CASE( "expected void value" ) {
    {
        Expected<void> exp;
        XREQUIRE(exp);
        CHECK(!!exp);
        static_assert(is_void_v<decltype(exp.value())>);
        static_assert(is_void_v<decltype(*exp)>);
    }
    {
        XREQUIRE(Expected<void>());
        CHECK(!!Expected<void>());
        static_assert(is_void_v<decltype(Expected<void>().value())>);
        static_assert(is_void_v<decltype(*Expected<void>())>);
    }    
    {    
        const Expected<void> exp;
        XREQUIRE(exp);
        CHECK(!!exp);
        static_assert(is_void_v<decltype(exp.value())>);
        static_assert(is_void_v<decltype(*exp)>);
    }
}

TEST_CASE( "expected error" ) {
    {
        Expected<foo> exp(Failure<Exc>, "a");
        XREQUIRE(!bool(exp));
        CHECK(!exp);
        XREQUIRE(exp.error());
        CHECK(exp.error()->message() == "a");
        CHECK(exp.error()->what() == "a"s);
    }

    {
        XREQUIRE(!bool(Expected<foo>(Failure<Exc>, "a")));
        CHECK(!Expected<foo>(Failure<Exc>, "a"));
        XREQUIRE(Expected<foo>(Failure<Exc>, "a").error());
        CHECK(Expected<foo>(Failure<Exc>, "a").error()->message() == "a");
        CHECK(Expected<foo>(Failure<Exc>, "a").error()->what() == "a"s);
    }

    {
        const Expected<foo> exp(Failure<Exc>, "a");
        XREQUIRE(!bool(exp));
        CHECK(!exp);
        XREQUIRE(exp.error());
        CHECK(exp.error()->message() == "a");
        CHECK(exp.error()->what() == "a"s);
    }

    {
        auto ex = make_shared<Exc>();
        Expected<foo> exp(ex);
        XREQUIRE(!bool(exp));
        CHECK(!exp);
        XREQUIRE(exp.error() == ex);
        CHECK(exp.error()->message() == "blah");
        CHECK(exp.error()->what() == "blah"s);
    }
}

TEST_CASE( "expected error in void" ) {

    {
        Expected<void> exp(Failure<Exc>, "a");
        XREQUIRE(!bool(exp));
        CHECK(!exp);
        XREQUIRE(exp.error());
        CHECK(exp.error()->message() == "a");
        CHECK(exp.error()->what() == "a"s);
    }

    {
        XREQUIRE(!bool(Expected<void>(Failure<Exc>, "a")));
        CHECK(!Expected<void>(Failure<Exc>, "a"));
        XREQUIRE(Expected<void>(Failure<Exc>, "a").error());
        CHECK(Expected<void>(Failure<Exc>, "a").error()->message() == "a");
        CHECK(Expected<void>(Failure<Exc>, "a").error()->what() == "a"s);
    }

    {
        const Expected<void> exp(Failure<Exc>, "a");
        XREQUIRE(!bool(exp));
        CHECK(!exp);
        XREQUIRE(exp.error());
        CHECK(exp.error()->message() == "a");
        CHECK(exp.error()->what() == "a"s);
    }

    {
        auto ex = make_shared<Exc>();
        Expected<void> exp(ex);
        XREQUIRE(!bool(exp));
        CHECK(!exp);
        XREQUIRE(exp.error() == ex);
        CHECK(exp.error()->message() == "blah");
        CHECK(exp.error()->what() == "blah"s);
    }
}

TEST_CASE( "converting expecteds" ) {

    CHECK(Expected<string>(Expected<const char *>("hello")).value() == "hello");
    CHECK(Expected<foo>(Expected<int>(5)).value().val == "5");
    CHECK(Expected<void>(Expected<int>(5)));
}

}


