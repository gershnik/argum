#include "test-common.h"

#include <argum/tokenizer.h>

#include <doctest/doctest.h>

#include <variant>
#include <array>

#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

using namespace Argum;
using namespace std;

using namespace std::literals;

using OptionToken = Tokenizer::OptionToken;
using OptionStopToken = Tokenizer::OptionStopToken;
using ArgumentToken = Tokenizer::ArgumentToken;
using UnknownOptionToken = Tokenizer::UnknownOptionToken;
using AmbiguousOptionToken = Tokenizer::AmbiguousOptionToken;



namespace Argum {
    using Token = variant<OptionToken, OptionStopToken, ArgumentToken, UnknownOptionToken, AmbiguousOptionToken>;
    
    static auto operator<<(ostream & str, const OptionToken & token) -> ostream & {
        str << "OptionToken: "  << token.idx << ", used as: " << token.usedName;
        if (token.argument)
            str << ", arg: " << *token.argument;
        return str << ", from arg: " << token.argIdx;
    }
    static auto operator<<(ostream & str, const ArgumentToken & token) -> ostream & {
        return str << "ArgumentToken: " << token.value  << ", from: " << token.argIdx;
    }
    static auto operator<<(ostream & str, const OptionStopToken & token) -> ostream & {
        return str << "OptionStopToken, from arg: " << token.argIdx;
    }
    static auto operator<<(ostream & str, const UnknownOptionToken & token) -> ostream & {
        str << "UnknownOptionToken: " << token.name;
        if (token.argument)
            str << ", arg: " << *token.argument;
        return str << ", from arg: " << token.argIdx;
    }
    static auto operator<<(ostream & str, const AmbiguousOptionToken & token) -> ostream & {
        str << "AmbiguousOptionToken: " << token.name;
        if (token.argument)
            str << ", arg: " << *token.argument;
        str << " (" << token.possibilities[0];
        for_each(token.possibilities.begin() + 1, token.possibilities.end(), [&](const auto & n) { str << ", " << n; });
        return str << ", from arg: " << token.argIdx;
    }
    
    static auto operator<<(ostream & str, const Token & token) -> ostream & {
        return visit([&str](const auto & value) -> ostream & { return str << value; }, token);
    }
}

TEST_SUITE("tokenizer") {

#ifndef ARGUM_NO_THROW
TEST_CASE( "Settings boundaries" ) {

    CHECK_THROWS_AS(Tokenizer::Settings().addShortPrefix("-").addLongPrefix("-"), invalid_argument);
    CHECK_THROWS_AS(Tokenizer::Settings().addLongPrefix("-").addShortPrefix("-"), invalid_argument);
    CHECK_THROWS_AS(Tokenizer::Settings().addValueDelimiter('-').addValueDelimiter('-'), invalid_argument);
}
#endif


TEST_CASE( "Null Command Line" ) {

    Tokenizer t;

    auto ret = t.tokenize((const char **)nullptr, (const char **)nullptr, []([[maybe_unused]] const auto & token) {
        CHECK(false);
        return Tokenizer::Continue;
    });
    CHECK(ARGUM_EXPECTED_VALUE(ret) == vector<string>{});
}


#define ARGS(...) (initializer_list<const char *>{__VA_ARGS__})
#define TOKENS(...) (initializer_list<Token>{__VA_ARGS__})


#ifndef ARGUM_NO_THROW

    #define TEST_TOKENIZER(t, args, results) {\
        auto argv = vector(args); \
        auto expected = vector(results); \
        size_t current = 0;\
        auto res = t.tokenize(argv.data(), argv.data() + size(argv), [&](const auto & token) {\
            REQUIRE(current < size(expected));\
            INFO("token index: " << current); \
            CHECK(Token(token) == expected[current]);\
            ++current;\
            return Tokenizer::Continue;\
        });\
        CHECK(current == size(expected));\
        CHECK(ARGUM_EXPECTED_VALUE(res) == vector<string>{});\
    }
#else
    #define TEST_TOKENIZER(t, args, results) {\
        auto argv = vector(args); \
        auto expected = vector(results); \
        size_t current = 0;\
        auto res = t.tokenize(argv.data(), argv.data() + size(argv), [&](const auto & token) {\
            CHECK(current < size(expected));\
            if (current >= size(expected)) abort(); \
            INFO("token index: " << current); \
            CHECK(Token(token) == expected[current]);\
            ++current;\
            return Tokenizer::Continue;\
        });\
        CHECK(current == size(expected));\
        CHECK(ARGUM_EXPECTED_VALUE(res) == vector<string>{});\
    }
    #endif



TEST_CASE( "Empty Tokenizer" ) {

    Tokenizer t;

    TEST_TOKENIZER(t, ARGS(), TOKENS(
    ));
    TEST_TOKENIZER(t, ARGS( "-c" ), TOKENS(
        UnknownOptionToken{ 0, "-c", nullopt }
    ));
    TEST_TOKENIZER(t, ARGS( "a", "xyz", "123"), TOKENS(
        ArgumentToken{ 0, "a" },
        ArgumentToken{ 1, "xyz" },
        ArgumentToken{ 2, "123" }
    ));
    TEST_TOKENIZER(t, ARGS("-a", "xyz", "-b", "--", "c"), TOKENS(
        UnknownOptionToken{  0, "-a", nullopt},
        ArgumentToken{       1, "xyz" },
        UnknownOptionToken{  2, "-b", nullopt},
        OptionStopToken{     3 },
        ArgumentToken{       4, "c" }
    ));
    
}

TEST_CASE( "Short option" ) {

    Tokenizer t;

    t.add(OptionNames("-c"));

    TEST_TOKENIZER(t, ARGS("-c"), TOKENS(
        OptionToken{ 0, 0, "-c", nullopt}
    ));
    TEST_TOKENIZER(t, ARGS("-c", "-c"), TOKENS(
        OptionToken{ 0, 0, "-c", nullopt},
        OptionToken{ 1, 0, "-c", nullopt}
    ));
    TEST_TOKENIZER(t, ARGS( "-cc" ), TOKENS(
        OptionToken{ 0, 0, "-c", nullopt},
        OptionToken{ 0, 0, "-c", nullopt}
    ));
    TEST_TOKENIZER(t, ARGS("-c", "c", "-c"), TOKENS(
        OptionToken{   0, 0, "-c", nullopt},
        ArgumentToken{ 1, "c" },
        OptionToken{   2, 0, "-c", nullopt}
    ));
    TEST_TOKENIZER(t, ARGS("-c", "--", "-c"), TOKENS(
        OptionToken{     0, 0, "-c", nullopt},
        OptionStopToken{ 1 },
        ArgumentToken{   2, "-c"}
    ));
}

TEST_CASE( "Tokenizer Single Char Stops" ) {

    Tokenizer t;

    t.add(OptionNames("-c"));
    t.add(OptionNames("-d"));
    t.add(OptionNames("-e"));
    SUBCASE("First single char in group"){
        const char * argv[] = {"-cdefg"};
        auto res = t.tokenize(begin(argv), end(argv), [&](const auto & token) {

            if constexpr (is_same_v<remove_cvref_t<decltype(token)>, OptionToken>) {
                if (token.usedName == "-c")
                    return Tokenizer::StopBefore;
            }

            return Tokenizer::Continue;
        });
        CHECK(ARGUM_EXPECTED_VALUE(res) == vector<string>{"-cdefg"});
        res = t.tokenize(begin(argv), end(argv), [&](const auto & token) {

            if constexpr (is_same_v<remove_cvref_t<decltype(token)>, OptionToken>) {
                if (token.usedName == "-c")
                    return Tokenizer::StopAfter;
            }

            return Tokenizer::Continue;
        });
        CHECK(ARGUM_EXPECTED_VALUE(res) == vector<string>{"-defg"});
    }
    SUBCASE("Middle single char in group"){
        const char * argv[] = {"-cdefg"};
        auto res = t.tokenize(begin(argv), end(argv), [&](const auto & token) {

            if constexpr (is_same_v<remove_cvref_t<decltype(token)>, OptionToken>) {
                if (token.usedName == "-d")
                    return Tokenizer::StopBefore;
            }

            return Tokenizer::Continue;
        });
        CHECK(ARGUM_EXPECTED_VALUE(res) == vector<string>{"-defg"});
        res = t.tokenize(begin(argv), end(argv), [&](const auto & token) {

            if constexpr (is_same_v<remove_cvref_t<decltype(token)>, OptionToken>) {
                if (token.usedName == "-d")
                    return Tokenizer::StopAfter;
            }

            return Tokenizer::Continue;
        });
        CHECK(ARGUM_EXPECTED_VALUE(res) == vector<string>{"-efg"});
    }
    SUBCASE("Last single char in group"){
        const char * argv[] = {"-cdefg"};
        auto res = t.tokenize(begin(argv), end(argv), [&](const auto & token) {

            if constexpr (is_same_v<remove_cvref_t<decltype(token)>, OptionToken>) {
                if (token.usedName == "-e")
                    return Tokenizer::StopBefore;
            }

            return Tokenizer::Continue;
        });
        CHECK(ARGUM_EXPECTED_VALUE(res) == vector<string>{"-efg"});
        res = t.tokenize(begin(argv), end(argv), [&](const auto & token) {

            if constexpr (is_same_v<remove_cvref_t<decltype(token)>, OptionToken>) {
                if (token.usedName == "-e")
                    return Tokenizer::StopAfter;
            }

            return Tokenizer::Continue;
        });
        CHECK(ARGUM_EXPECTED_VALUE(res) == vector<string>{});
    }
    SUBCASE("Middle single char in group, other parameters"){
        const char * argv[] = {"abc", "-cdefg", "qqq"};
        auto res = t.tokenize(begin(argv), end(argv), [&](const auto & token) {

            if constexpr (is_same_v<remove_cvref_t<decltype(token)>, OptionToken>) {
                if (token.usedName == "-d")
                    return Tokenizer::StopBefore;
            }

            return Tokenizer::Continue;
        });
        CHECK(ARGUM_EXPECTED_VALUE(res) == vector<string>{"-defg", "qqq"});
        res = t.tokenize(begin(argv), end(argv), [&](const auto & token) {

            if constexpr (is_same_v<remove_cvref_t<decltype(token)>, OptionToken>) {
                if (token.usedName == "-d")
                    return Tokenizer::StopAfter;
            }

            return Tokenizer::Continue;
        });
        CHECK(ARGUM_EXPECTED_VALUE(res) == vector<string>{"-efg", "qqq"});
    }
}

}
