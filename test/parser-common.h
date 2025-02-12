#ifndef HEADER_PARSER_COMMON_H_INCLUDED
#define HEADER_PARSER_COMMON_H_INCLUDED

#include "test-common.h"

#include <argum/help-formatter.h>
#include <argum/parser.h>

#include <doctest/doctest.h>

#include <iostream>

using namespace Argum;
using namespace std;

using namespace std::literals;

using UnrecognizedOption = Parser::UnrecognizedOption;
using WUnrecognizedOption = WParser::UnrecognizedOption;
using AmbiguousOption = Parser::AmbiguousOption;
using WAmbiguousOption = WParser::AmbiguousOption;
using ExtraPositional = Parser::ExtraPositional;
using WExtraPositional = WParser::ExtraPositional;
using MissingOptionArgument = Parser::MissingOptionArgument;
using WMissingOptionArgument = WParser::MissingOptionArgument;
using ExtraOptionArgument = Parser::ExtraOptionArgument;
using WExtraOptionArgument = WParser::ExtraOptionArgument;
using ValidationError = Parser::ValidationError;
using WValidationError = WParser::ValidationError;

template<class Char>
using BasicValue = optional<basic_string<Char>>;
using Value = BasicValue<char>;
using WValue = BasicValue<wchar_t>;


template<class Char>
inline auto parse(const BasicParser<Char> & parser, initializer_list<const Char *> args, 
                  map<basic_string<Char>, vector<BasicValue<Char>>> & results) {
    results.clear();
    #ifndef ARGUM_NO_THROW
        ARGUM_EXPECTED_VALUE(parser.parse(args.begin(), args.end()));
    #else 
        return parser.parse(args.begin(), args.end());
    #endif
}

template<class Char>
inline auto parseUntilUnknown(const BasicParser<Char> & parser, initializer_list<const Char *> args,
                              map<basic_string<Char>, vector<BasicValue<Char>>> & results) {
    results.clear();
    #ifndef ARGUM_NO_THROW
        return ARGUM_EXPECTED_VALUE(parser.parseUntilUnknown(args.begin(), args.end()));
    #else 
        return parser.parseUntilUnknown(args.begin(), args.end());
    #endif
}


namespace std {

    inline auto operator<<(ostream & str, const pair<const string, vector<Value>> & val) -> ostream & {
        str << '{' << val.first << ", {";
        for(size_t i = 0; i < val.second.size(); ++i) {
            if (i > 0) 
                str << ", ";
            str << val.second[i].value_or("<nullopt>");
        }
        str << "}}";
        return str;
    }

    inline auto operator<<(ostream & str, const pair<const wstring, vector<WValue>> & val) -> ostream & {
        str << '{' << toString<char>(val.first) << ", {";
        for(size_t i = 0; i < val.second.size(); ++i) {
            if (i > 0) 
                str << ", ";
            str << toString<char>(val.second[i].value_or(L"<nullopt>"));
        }
        str << "}}";
        return str;
    }
}

#ifndef ARGUM_NO_THROW

    #define HANDLE_FAILURE(type, h) catch (type & ex) { \
        h \
    }

    #define EXPECT_FAILURE(args, failure) \
        REQUIRE_NOTHROW([&](){ try { parse(parser, args, results); CHECK(false); } failure }());
    #define EXPECT_SUCCESS(args, expected) { \
        REQUIRE_NOTHROW(parse(parser, args, results)); \
        CHECK(results == expected); \
    }
    #define EXPECT_SUCCESS_UNTIL_UNKNOWN(args, expected, expectedRemainder) { \
        REQUIRE_NOTHROW(remainder = parseUntilUnknown(parser, args, results)); \
        CHECK(results == expected); \
        CHECK(remainder == expectedRemainder); \
    }
#else 
    #define HANDLE_FAILURE(type, h) [](auto * ex) { \
        auto specific = ex->template as<type>(); \
        CHECK(specific); \
        if (!specific) abort(); \
        [](const type & ex)h(*specific); \
    }

    #define EXPECT_FAILURE(args, failure) { \
        auto res = parse(parser, args, results); \
        CHECK(!res); \
        if (!!res) abort(); \
        failure(res.error().get()); \
    }
    #define EXPECT_SUCCESS(args, expected) { \
        auto res = parse(parser, args, results); \
        CHECK(res); \
        if (!res) abort(); \
        CHECK(results == expected); \
    }
    #define EXPECT_SUCCESS_UNTIL_UNKNOWN(args, expected, expectedRemainder) { \
        auto res = parseUntilUnknown(parser, args, results); \
        CHECK(res); \
        if (!res) abort(); \
        remainder = *res; \
        CHECK(results == expected); \
        CHECK(remainder == expectedRemainder); \
    }

#endif

#define UNRECOGNIZED_OPTION(x)      HANDLE_FAILURE(UnrecognizedOption,      { CHECK(ex.option == (x)); })
#define WUNRECOGNIZED_OPTION(x)     HANDLE_FAILURE(WUnrecognizedOption,     { CHECK(ex.option == (x)); })
#define AMBIGUOUS_OPTION(x, ...)    HANDLE_FAILURE(AmbiguousOption, { \
    CHECK(ex.option == (x)); \
    CHECK(ex.possibilities == vector<string>{__VA_ARGS__}); \
})
#define WAMBIGUOUS_OPTION(x, ...)   HANDLE_FAILURE(WAmbiguousOption, { \
    CHECK(ex.option == (x)); \
    CHECK(ex.possibilities == vector<wstring>{__VA_ARGS__}); \
})
#define EXTRA_POSITIONAL(x)         HANDLE_FAILURE(ExtraPositional,         { CHECK(ex.value == (x)); })
#define WEXTRA_POSITIONAL(x)        HANDLE_FAILURE(WExtraPositional,        { CHECK(ex.value == (x)); })
#define MISSING_OPTION_ARGUMENT(x)  HANDLE_FAILURE(MissingOptionArgument,   { CHECK(ex.option == (x)); })
#define WMISSING_OPTION_ARGUMENT(x) HANDLE_FAILURE(WMissingOptionArgument,  { CHECK(ex.option == (x)); })
#define EXTRA_OPTION_ARGUMENT(x)    HANDLE_FAILURE(ExtraOptionArgument,     { CHECK(ex.option == (x)); })
#define WEXTRA_OPTION_ARGUMENT(x)   HANDLE_FAILURE(WExtraOptionArgument,    { CHECK(ex.option == (x)); })
#define VALIDATION_ERROR(x)         HANDLE_FAILURE(ValidationError,         { CHECK(ex.message() == (x)); })
#define WVALIDATION_ERROR(x)        HANDLE_FAILURE(WValidationError,        { CHECK(ex.message() == (x)); })


#define ARGS(...) (initializer_list<const char *>{__VA_ARGS__})
#define WARGS(...) (initializer_list<const wchar_t *>{__VA_ARGS__})
#define RESULTS(...) decltype(results)(initializer_list<pair<const string, vector<Value>>>{__VA_ARGS__})
#define WRESULTS(...) decltype(results)(initializer_list<pair<const wstring, vector<WValue>>>{__VA_ARGS__})

#define OPTION_NO_ARG(n, ...) Option(n __VA_OPT__(,) __VA_ARGS__).handler([&](){ \
        results[n].push_back("+"); \
    })
#define WOPTION_NO_ARG(n, ...) WOption(n __VA_OPT__(,) __VA_ARGS__).handler([&](){ \
        results[n].push_back(L"+"); \
    })
#define OPTION_REQ_ARG(n, ...) Option(n __VA_OPT__(,) __VA_ARGS__).handler([&](string_view arg){ \
        results[n].push_back(string(arg)); \
    })
#define WOPTION_REQ_ARG(n, ...) WOption(n __VA_OPT__(,) __VA_ARGS__).handler([&](wstring_view arg){ \
        results[n].push_back(wstring(arg)); \
    })
#define OPTION_OPT_ARG(n, ...) Option(n __VA_OPT__(,) __VA_ARGS__).handler([&](optional<string_view> arg){ \
        arg ? results[n].push_back(string(*arg)) : results[n].push_back(nullopt); \
    })
#define WOPTION_OPT_ARG(n, ...) WOption(n __VA_OPT__(,) __VA_ARGS__).handler([&](optional<wstring_view> arg){ \
        arg ? results[n].push_back(wstring(*arg)) : results[n].push_back(nullopt); \
    })
#define POSITIONAL(n) Positional(n).handler([&](string_view value){ \
        results[n].push_back(string(value)); \
    })
#define WPOSITIONAL(n) WPositional(n).handler([&](wstring_view value){ \
        results[n].push_back(wstring(value)); \
    })

#endif