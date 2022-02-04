#include <modern-arg-parser/tokenizer.h>

#include "catch.hpp"

#include <variant>

#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

using namespace MArgP;
using namespace std;

using namespace std::literals;

using OptionToken = ArgumentTokenizer::OptionToken;
using OptionStopToken = ArgumentTokenizer::OptionStopToken;
using ArgumentToken = ArgumentTokenizer::ArgumentToken;
using UnknownOptionToken = ArgumentTokenizer::UnknownOptionToken;

using Token = std::variant<OptionToken, OptionStopToken, ArgumentToken, UnknownOptionToken>;

namespace MArgP {
    static auto operator<<(std::ostream & str, const OptionToken & token) -> std::ostream & {
        return str << "OptionToken: "  << token.value << ", idx: " << token.index << ", arg: " << token.containingArgIdx;
    }
    static auto operator<<(std::ostream & str, const ArgumentToken & token) -> std::ostream & {
        return str << "ArgumentToken: " << token.value  << ", arg: " << token.containingArgIdx;
    }
    static auto operator<<(std::ostream & str, const OptionStopToken & token) -> std::ostream & {
        return str << "OptionStopToken, arg: " << token.containingArgIdx;
    }
    static auto operator<<(std::ostream & str, const UnknownOptionToken & token) -> std::ostream & {
        return str << "UnknownOptionToken: " << token.value  << ", arg: " << token.containingArgIdx;
    }
}

static auto operator<<(std::ostream & str, const Token & token) -> std::ostream & {
    return std::visit([&str](const auto & value) -> std::ostream & { return str << value; }, token);
}


TEST_CASE( "Empty Command Line" , "[tokenizer]") {

    ArgumentTokenizer t;

    auto ret = t.tokenize(0, (const char **)nullptr, []([[maybe_unused]] const auto & token) {
        CHECK(false);
        return ArgumentTokenizer::Continue;
    });
    CHECK(ret == vector<string>{});

    const char * argv[] = { "prog" };
    ret = t.tokenize(int(std::size(argv)), argv, []([[maybe_unused]] const auto & token) {
        CHECK(false);
        return ArgumentTokenizer::Continue;
    });
    CHECK(ret == vector<string>{});

}


template<size_t N>
auto testTokenizer(const ArgumentTokenizer & t, int argc, const char ** argv, 
                   const Token (&expectedTokens)[N],
                   const vector<string> & expectedRes = vector<string>{}) {
    size_t current = 0;
    auto res = t.tokenize(argc, argv, [&](const auto & token) {
        REQUIRE(current < N);
        INFO("token index: " << current);
        CHECK(Token(token) == expectedTokens[current]);
        ++current;
        return ArgumentTokenizer::Continue;
    });
    CHECK(current == N);
    CHECK(res == expectedRes);
}


TEST_CASE( "Empty Tokenizer" , "[tokenizer]") {

    ArgumentTokenizer t;

    SECTION("Option Only") {
        const char * argv[] = { "prog", "-c" };
        Token expected[] = {
            UnknownOptionToken{ 1, "c" }
        };
        testTokenizer(t, int(std::size(argv)), argv, expected);
    }
    SECTION("Positional Only"){
        const char * argv[] = { "prog", "a", "xyz", "123" };
        Token expected[] = {
            ArgumentToken{ 1, "a" },
            ArgumentToken{ 2, "xyz" },
            ArgumentToken{ 3, "123" }
        };
        testTokenizer(t, int(std::size(argv)), argv, expected);
    }
    SECTION("Everything"){
        const char * argv[] = { "prog", "-a", "xyz", "-b", "--", "c" };
        Token expected[] = {
            UnknownOptionToken{  1, "a" },
            ArgumentToken{       2, "xyz" },
            UnknownOptionToken{  3, "b" },
            OptionStopToken{     4 },
            ArgumentToken{       5, "c" }
        };
        testTokenizer(t, int(std::size(argv)), argv, expected);
    }
    
}

TEST_CASE( "Short option" , "[tokenizer]") {

    ArgumentTokenizer t;

    t.add(OptionName("-c"));

    SECTION("Empty"){
        const char * argv[] = { "prog" };
        auto ret = t.tokenize(int(std::size(argv)), argv, []([[maybe_unused]] const Token & token) {
            CHECK(false);
            return ArgumentTokenizer::Continue;
        });
        CHECK(ret == vector<string>());
    }

    SECTION("One"){
        const char * argv[] = { "prog", "-c" };
        Token expected[] = {
            OptionToken{ 1, 0, "c", std::nullopt}
        };
        testTokenizer(t, int(std::size(argv)), argv, expected);
    }

    SECTION("Two"){
        const char * argv[] = { "prog", "-c", "-c" };
        Token expected[] = {
            OptionToken{ 1, 0, "c", std::nullopt},
            OptionToken{ 2, 0, "c", std::nullopt}
        };
        testTokenizer(t, int(std::size(argv)), argv, expected);
    }

    SECTION("Two Merged"){
        const char * argv[] = { "prog", "-cc" };
        Token expected[] = {
            OptionToken{ 1, 0, "c", std::nullopt},
            OptionToken{ 1, 0, "c", std::nullopt}
        };
        testTokenizer(t, int(std::size(argv)), argv, expected);
    }

    SECTION("Two With Arg In-Between"){
        const char * argv[] = { "prog", "-c", "c", "-c" };
        Token expected[] = {
            OptionToken{   1, 0, "c", std::nullopt},
            ArgumentToken{ 2, "c" },
            OptionToken{   3, 0, "c", std::nullopt}
        };
        testTokenizer(t, int(std::size(argv)), argv, expected);
    }

    SECTION("Two With Stop"){
        const char * argv[] = { "prog", "-c", "--", "-c" };
        Token expected[] = {
            OptionToken{     1, 0, "c", std::nullopt},
            OptionStopToken{ 2 },
            ArgumentToken{   3, "-c"}
        };
        testTokenizer(t, int(std::size(argv)), argv, expected);
    }
}
