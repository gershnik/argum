//for testing let it throw exception rather than crash
[[noreturn]] void reportInvalidArgument(const char * message);
#define ARGUM_INVALID_ARGUMENT(message) reportInvalidArgument(message) 

#include <argum/help-formatter.h>
#include <argum/parser.h>

#include "catch.hpp"

#include <iostream>

using namespace Argum;
using namespace std;

using namespace std::literals;

using Option = Parser::Option;
using WOption = WParser::Option;
using Positional = Parser::Positional;
using WPositional = WParser::Positional;
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


inline void reportInvalidArgument(const char * message) {
    throw invalid_argument(message);
}

template<class Char>
inline auto parse(const BasicParser<Char> & parser, initializer_list<const Char *> args, 
                  map<basic_string<Char>, vector<BasicValue<Char>>> & results) {
    results.clear();
    parser.parse(args.begin(), args.end());
}

template<class Char>
inline auto parseUntilUnknown(const BasicParser<Char> & parser, initializer_list<const Char *> args,
                              map<basic_string<Char>, vector<BasicValue<Char>>> & results) {
    results.clear();
    return parser.parseUntilUnknown(args.begin(), args.end());
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

#define UNRECOGNIZED_OPTION(x) catch(UnrecognizedOption & ex) { CHECK(ex.option == (x)); }
#define WUNRECOGNIZED_OPTION(x) catch(WUnrecognizedOption & ex) { CHECK(ex.option == (x)); }
#define AMBIGUOUS_OPTION(x, ...) catch(AmbiguousOption & ex) { \
    CHECK(ex.option == (x)); \
    CHECK(ex.possibilities == vector<string>{__VA_ARGS__}); \
}
#define WAMBIGUOUS_OPTION(x, ...) catch(WAmbiguousOption & ex) { \
    CHECK(ex.option == (x)); \
    CHECK(ex.possibilities == vector<wstring>{__VA_ARGS__}); \
}
#define EXTRA_POSITIONAL(x) catch(ExtraPositional & ex) { CHECK(ex.value == (x)); }
#define WEXTRA_POSITIONAL(x) catch(WExtraPositional & ex) { CHECK(ex.value == (x)); }
#define MISSING_OPTION_ARGUMENT(x) catch(MissingOptionArgument & ex) { CHECK(ex.option == (x)); }
#define WMISSING_OPTION_ARGUMENT(x) catch(WMissingOptionArgument & ex) { CHECK(ex.option == (x)); }
#define EXTRA_OPTION_ARGUMENT(x) catch(ExtraOptionArgument & ex) { CHECK(ex.option == (x)); }
#define WEXTRA_OPTION_ARGUMENT(x) catch(WExtraOptionArgument & ex) { CHECK(ex.option == (x)); }
#define VALIDATION_ERROR(x) catch(ValidationError & ex) { CHECK(ex.message() == (x)); }
#define WVALIDATION_ERROR(x) catch(WValidationError & ex) { CHECK(ex.message() == (x)); }

#define EXPECT_FAILURE(args, failure) \
    REQUIRE_NOTHROW([&](){ try { parse(parser, args, results); CHECK(false); } failure }());
#define EXPECT_SUCCESS(args, expected) { \
    REQUIRE_NOTHROW(parse(parser, args, results)); \
    CHECK(results == expected); \
}
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
#define POSITIONAL(n) Positional(n).handler([&](unsigned idx, string_view value){ \
        auto & list = results[n]; \
        CHECK(list.size() == idx); \
        list.push_back(string(value)); \
    })
#define WPOSITIONAL(n) WPositional(n).handler([&](unsigned idx, wstring_view value){ \
        auto & list = results[n]; \
        CHECK(list.size() == idx); \
        list.push_back(wstring(value)); \
    })
