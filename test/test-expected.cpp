#include "test-common.h"

#include <argum/expected.h>

#include "catch.hpp"

using namespace Argum;
using namespace std;

using namespace std::literals;

namespace {
    struct foo {
        foo(int i):val(std::to_string(i)) { }
        foo(string s):val(s) {}
        foo(int i, char c):val(std::to_string(i) + c) {}

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

TEST_CASE( "expected properties" , "[expected]") {
    STATIC_REQUIRE(!std::is_default_constructible_v<Expected<foo>>);
    STATIC_REQUIRE(std::is_copy_constructible_v<Expected<foo>>);
    STATIC_REQUIRE(std::is_nothrow_move_constructible_v<Expected<foo>>);
    STATIC_REQUIRE(std::is_copy_assignable_v<Expected<foo>>);
    STATIC_REQUIRE(std::is_nothrow_move_assignable_v<Expected<foo>>);
    STATIC_REQUIRE(std::is_constructible_v<Expected<foo>, foo>);
    STATIC_REQUIRE(std::is_constructible_v<Expected<foo>, int>);
    STATIC_REQUIRE(std::is_constructible_v<Expected<foo>, string>);
    STATIC_REQUIRE(std::is_constructible_v<Expected<foo>, decltype(std::make_shared<Exc>())>);

    STATIC_REQUIRE(std::is_nothrow_default_constructible_v<Expected<void>>);
    STATIC_REQUIRE(std::is_copy_constructible_v<Expected<void>>);
    STATIC_REQUIRE(std::is_nothrow_move_constructible_v<Expected<void>>);
    STATIC_REQUIRE(std::is_copy_assignable_v<Expected<void>>);
    STATIC_REQUIRE(std::is_nothrow_move_assignable_v<Expected<void>>);
    STATIC_REQUIRE(!std::is_constructible_v<Expected<void>, int>);
    STATIC_REQUIRE(!std::is_constructible_v<Expected<void>, monostate>);
    STATIC_REQUIRE(std::is_constructible_v<Expected<void>, decltype(std::make_shared<Exc>())>);

    STATIC_REQUIRE(std::is_constructible_v<Expected<void>, Expected<foo>>);
    STATIC_REQUIRE(std::is_constructible_v<Expected<int>, Expected<short>>);
}

TEST_CASE( "expected value" , "[expected]") {
    {
        Expected<foo> exp(3);
        REQUIRE(exp);
        CHECK(!!exp);
        CHECK(exp.value().val == "3");
        CHECK(exp->val == "3");
        CHECK((*exp).val == "3");
    }
    {
        REQUIRE(Expected<foo>(3));
        CHECK(!!Expected<foo>(3));
        CHECK(Expected<foo>(3).value().val == "3");
        CHECK(Expected<foo>(3)->val == "3");
        CHECK((*Expected<foo>(3)).val == "3");
    }    
    {    
        const Expected<foo> exp(3);
        REQUIRE(exp);
        CHECK(!!exp);
        CHECK(exp.value().val == "3");
        CHECK(exp->val == "3");
        CHECK((*exp).val == "3");
    }

    CHECK(Expected<foo>(3, 'a')->val == "3a");
    CHECK(Expected<foo>(foo("x"s))->val == "x");
    
}

TEST_CASE( "expected void value" , "[expected]") {
    {
        Expected<void> exp;
        REQUIRE(exp);
        CHECK(!!exp);
        STATIC_REQUIRE(std::is_void_v<decltype(exp.value())>);
        STATIC_REQUIRE(std::is_void_v<decltype(*exp)>);
    }
    {
        REQUIRE(Expected<void>());
        CHECK(!!Expected<void>());
        STATIC_REQUIRE(std::is_void_v<decltype(Expected<void>().value())>);
        STATIC_REQUIRE(std::is_void_v<decltype(*Expected<void>())>);
    }    
    {    
        const Expected<void> exp;
        REQUIRE(exp);
        CHECK(!!exp);
        STATIC_REQUIRE(std::is_void_v<decltype(exp.value())>);
        STATIC_REQUIRE(std::is_void_v<decltype(*exp)>);
    }
}

TEST_CASE( "expected error" , "[expected]") {
    {
        Expected<foo> exp(Failure<Exc>, "a");
        REQUIRE(!bool(exp));
        CHECK(!exp);
        REQUIRE(exp.error());
        CHECK(exp.error()->message() == "a");
        CHECK(exp.error()->what() == "a"s);
    }

    {
        REQUIRE(!bool(Expected<foo>(Failure<Exc>, "a")));
        CHECK(!Expected<foo>(Failure<Exc>, "a"));
        REQUIRE(Expected<foo>(Failure<Exc>, "a").error());
        CHECK(Expected<foo>(Failure<Exc>, "a").error()->message() == "a");
        CHECK(Expected<foo>(Failure<Exc>, "a").error()->what() == "a"s);
    }

    {
        const Expected<foo> exp(Failure<Exc>, "a");
        REQUIRE(!bool(exp));
        CHECK(!exp);
        REQUIRE(exp.error());
        CHECK(exp.error()->message() == "a");
        CHECK(exp.error()->what() == "a"s);
    }

    {
        auto ex = std::make_shared<Exc>();
        Expected<foo> exp(ex);
        REQUIRE(!bool(exp));
        CHECK(!exp);
        REQUIRE(exp.error() == ex);
        CHECK(exp.error()->message() == "blah");
        CHECK(exp.error()->what() == "blah"s);
    }
}

TEST_CASE( "expected error in void" , "[expected]") {

    {
        Expected<void> exp(Failure<Exc>, "a");
        REQUIRE(!bool(exp));
        CHECK(!exp);
        REQUIRE(exp.error());
        CHECK(exp.error()->message() == "a");
        CHECK(exp.error()->what() == "a"s);
    }

    {
        REQUIRE(!bool(Expected<void>(Failure<Exc>, "a")));
        CHECK(!Expected<void>(Failure<Exc>, "a"));
        REQUIRE(Expected<void>(Failure<Exc>, "a").error());
        CHECK(Expected<void>(Failure<Exc>, "a").error()->message() == "a");
        CHECK(Expected<void>(Failure<Exc>, "a").error()->what() == "a"s);
    }

    {
        const Expected<void> exp(Failure<Exc>, "a");
        REQUIRE(!bool(exp));
        CHECK(!exp);
        REQUIRE(exp.error());
        CHECK(exp.error()->message() == "a");
        CHECK(exp.error()->what() == "a"s);
    }

    {
        auto ex = std::make_shared<Exc>();
        Expected<void> exp(ex);
        REQUIRE(!bool(exp));
        CHECK(!exp);
        REQUIRE(exp.error() == ex);
        CHECK(exp.error()->message() == "blah");
        CHECK(exp.error()->what() == "blah"s);
    }
}

TEST_CASE( "converting expecteds" , "[expected]") {

    CHECK(Expected<std::string>(Expected<const char *>("hello")).value() == "hello");
    CHECK(Expected<foo>(Expected<int>(5)).value().val == "5");
    CHECK(Expected<void>(Expected<int>(5)));
}




