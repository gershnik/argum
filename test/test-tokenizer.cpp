#include <modern-arg-parser/tokenizer.h>

#include "catch.hpp"

#include <variant>
#include <array>

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
        str << "OptionToken: "  << token.idx << ", used as: " << token.usedName;
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


TEST_CASE( "Null Command Line" , "[tokenizer]") {

    ArgumentTokenizer t;

    auto ret = t.tokenize((const char **)nullptr, (const char **)nullptr, []([[maybe_unused]] const auto & token) {
        CHECK(false);
        return ArgumentTokenizer::Continue;
    });
    CHECK(ret == vector<string>{});
}


#define ARGS(...) (initializer_list<const char *>{__VA_ARGS__})
#define TOKENS(...) (initializer_list<Token>{__VA_ARGS__})


#define TEST_TOKENIZER(t, args, results) {\
    auto argv = vector(args); \
    auto expected = vector(results); \
    size_t current = 0;\
    auto res = t.tokenize(argv.data(), argv.data() + std::size(argv), [&](const auto & token) {\
        REQUIRE(current < std::size(expected));\
        INFO("token index: " << current); \
        CHECK(Token(token) == expected[current]);\
        ++current;\
        return ArgumentTokenizer::Continue;\
    });\
    CHECK(current == std::size(expected));\
    CHECK(res == vector<string>{});\
}



TEST_CASE( "Empty Tokenizer" , "[tokenizer]") {

    ArgumentTokenizer t;

    TEST_TOKENIZER(t, ARGS(), TOKENS(
    ));
    TEST_TOKENIZER(t, ARGS( "-c" ), TOKENS(
        UnknownOptionToken{ &argv[0], "-c", std::nullopt }
    ));
    TEST_TOKENIZER(t, ARGS( "a", "xyz", "123"), TOKENS(
        ArgumentToken{ &argv[0], "a" },
        ArgumentToken{ &argv[1], "xyz" },
        ArgumentToken{ &argv[2], "123" }
    ));
    TEST_TOKENIZER(t, ARGS("-a", "xyz", "-b", "--", "c"), TOKENS(
        UnknownOptionToken{  &argv[0], "-a", std::nullopt},
        ArgumentToken{       &argv[1], "xyz" },
        UnknownOptionToken{  &argv[2], "-b", std::nullopt},
        OptionStopToken{     &argv[3] },
        ArgumentToken{       &argv[4], "c" }
    ));
    
}

TEST_CASE( "Short option" , "[tokenizer]") {

    ArgumentTokenizer t;

    t.add(OptionNames("-c"));

    TEST_TOKENIZER(t, ARGS("-c"), TOKENS(
        OptionToken{ &argv[0], 0, "-c", std::nullopt}
    ));
    TEST_TOKENIZER(t, ARGS("-c", "-c"), TOKENS(
        OptionToken{ &argv[0], 0, "-c", std::nullopt},
        OptionToken{ &argv[1], 0, "-c", std::nullopt}
    ));
    TEST_TOKENIZER(t, ARGS( "-cc" ), TOKENS(
        OptionToken{ &argv[0], 0, "-c", std::nullopt},
        OptionToken{ &argv[0], 0, "-c", std::nullopt}
    ));
    TEST_TOKENIZER(t, ARGS("-c", "c", "-c"), TOKENS(
        OptionToken{   &argv[0], 0, "-c", std::nullopt},
        ArgumentToken{ &argv[1], "c" },
        OptionToken{   &argv[2], 0, "-c", std::nullopt}
    ));
    TEST_TOKENIZER(t, ARGS("-c", "--", "-c"), TOKENS(
        OptionToken{     &argv[0], 0, "-c", std::nullopt},
        OptionStopToken{ &argv[1] },
        ArgumentToken{   &argv[2], "-c"}
    ));
}
