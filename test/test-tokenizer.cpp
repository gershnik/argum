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



namespace MArgP {
    using Token = std::variant<OptionToken, OptionStopToken, ArgumentToken, UnknownOptionToken>;
    
    static auto operator<<(std::ostream & str, const OptionToken & token) -> std::ostream & {
        str << "OptionToken: "  << token.name << ", used as: " << token.usedName;
        if (token.argument)
            str << ", arg: " << *token.argument;
        return str << ", from: " << *token.containingArg;
    }
    static auto operator<<(std::ostream & str, const ArgumentToken & token) -> std::ostream & {
        return str << "ArgumentToken: " << token.value  << ", from: " << *token.containingArg;
    }
    static auto operator<<(std::ostream & str, const OptionStopToken & token) -> std::ostream & {
        return str << "OptionStopToken, from: " << *token.containingArg;
    }
    static auto operator<<(std::ostream & str, const UnknownOptionToken & token) -> std::ostream & {
        str << "UnknownOptionToken: " << token.name;
        if (token.argument)
            str << ", arg: " << *token.argument;
        return str << ", from: " << *token.containingArg;
    }
    
    static auto operator<<(std::ostream & str, const Token & token) -> std::ostream & {
        return std::visit([&str](const auto & value) -> std::ostream & { return str << value; }, token);
    }
}




TEST_CASE( "Empty Command Line" , "[tokenizer]") {

    ArgumentTokenizer t;

    auto ret = t.tokenize((const char **)nullptr, (const char **)nullptr, []([[maybe_unused]] const auto & token) {
        CHECK(false);
        return ArgumentTokenizer::Continue;
    });
    CHECK(ret == vector<string>{});

    const char * argv[] = { "xxx" };
    ret = t.tokenize(std::begin(argv), std::begin(argv), []([[maybe_unused]] const auto & token) {
        CHECK(false);
        return ArgumentTokenizer::Continue;
    });
    CHECK(ret == vector<string>{});

}


template<size_t N>
auto testTokenizer(const ArgumentTokenizer & t, size_t argc, const char ** argv, 
                   const Token (&expectedTokens)[N],
                   const vector<string> & expectedRes = vector<string>{}) {
    size_t current = 0;
    auto res = t.tokenize(argv, argv + argc, [&](const auto & token) {
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
        const char * argv[] = { "-c" };
        Token expected[] = {
            UnknownOptionToken{ &argv[0], "-c", std::nullopt }
        };
        testTokenizer(t, std::size(argv), argv, expected);
    }
    SECTION("Positional Only"){
        const char * argv[] = { "a", "xyz", "123" };
        Token expected[] = {
            ArgumentToken{ &argv[0], "a" },
            ArgumentToken{ &argv[1], "xyz" },
            ArgumentToken{ &argv[2], "123" }
        };
        testTokenizer(t, std::size(argv), argv, expected);
    }
    SECTION("Everything"){
        const char * argv[] = { "-a", "xyz", "-b", "--", "c" };
        Token expected[] = {
            UnknownOptionToken{  &argv[0], "-a", std::nullopt},
            ArgumentToken{       &argv[1], "xyz" },
            UnknownOptionToken{  &argv[2], "-b", std::nullopt},
            OptionStopToken{     &argv[3] },
            ArgumentToken{       &argv[4], "c" }
        };
        testTokenizer(t, std::size(argv), argv, expected);
    }
    
}

TEST_CASE( "Short option" , "[tokenizer]") {

    ArgumentTokenizer t;

    t.add(OptionNames("-c"));

    SECTION("One"){
        const char * argv[] = { "-c" };
        Token expected[] = {
            OptionToken{ &argv[0], "-c", "-c", std::nullopt}
        };
        testTokenizer(t, std::size(argv), argv, expected);
    }

    SECTION("Two"){
        const char * argv[] = { "-c", "-c" };
        Token expected[] = {
            OptionToken{ &argv[0], "-c", "-c", std::nullopt},
            OptionToken{ &argv[1], "-c", "-c", std::nullopt}
        };
        testTokenizer(t, std::size(argv), argv, expected);
    }

    SECTION("Two Merged"){
        const char * argv[] = { "-cc" };
        Token expected[] = {
            OptionToken{ &argv[0], "-c", "-c", std::nullopt},
            OptionToken{ &argv[0], "-c", "-c", std::nullopt}
        };
        testTokenizer(t, std::size(argv), argv, expected);
    }

    SECTION("Two With Arg In-Between"){
        const char * argv[] = { "-c", "c", "-c" };
        Token expected[] = {
            OptionToken{   &argv[0], "-c", "-c", std::nullopt},
            ArgumentToken{ &argv[1], "c" },
            OptionToken{   &argv[2], "-c", "-c", std::nullopt}
        };
        testTokenizer(t, std::size(argv), argv, expected);
    }

    SECTION("Two With Stop"){
        const char * argv[] = { "-c", "--", "-c" };
        Token expected[] = {
            OptionToken{     &argv[0], "-c", "-c", std::nullopt},
            OptionStopToken{ &argv[1] },
            ArgumentToken{   &argv[2], "-c"}
        };
        testTokenizer(t, std::size(argv), argv, expected);
    }
}
