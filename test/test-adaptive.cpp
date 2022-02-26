//for testing let it throw exception rather than crash
[[noreturn]] void reportInvalidArgument(const char * message);
#define ARGUM_INVALID_ARGUMENT(message) reportInvalidArgument(message) 

#include <argum/adaptive-parser.h>

#include "catch.hpp"

#include <iostream>

using namespace Argum;
using namespace std;

using namespace std::literals;

using Option = AdaptiveParser::Option;
using WOption = WAdaptiveParser::Option;
using Positional = AdaptiveParser::Positional;
using WPositional = WAdaptiveParser::Positional;
using UnrecognizedOption = AdaptiveParser::UnrecognizedOption;
using WUnrecognizedOption = WAdaptiveParser::UnrecognizedOption;
using AmbiguousOption = AdaptiveParser::AmbiguousOption;
using WAmbiguousOption = WAdaptiveParser::AmbiguousOption;
using ExtraPositional = AdaptiveParser::ExtraPositional;
using WExtraPositional = WAdaptiveParser::ExtraPositional;
using MissingOptionArgument = AdaptiveParser::MissingOptionArgument;
using WMissingOptionArgument = WAdaptiveParser::MissingOptionArgument;
using ExtraOptionArgument = AdaptiveParser::ExtraOptionArgument;
using WExtraOptionArgument = WAdaptiveParser::ExtraOptionArgument;
using ValidationError = AdaptiveParser::ValidationError;
using WValidationError = WAdaptiveParser::ValidationError;

void reportInvalidArgument(const char * message) {
    throw invalid_argument(message);
}

template<class Char>
static auto parse(const BasicAdaptiveParser<Char> & parser, initializer_list<const Char *> args) {
    parser.parse(args.begin(), args.end());
}

template<class Char>
using BasicValue = optional<basic_string<Char>>;
using Value = BasicValue<char>;
using WValue = BasicValue<wchar_t>;

namespace std {

    static auto operator<<(ostream & str, const pair<const string, vector<Value>> & val) -> ostream & {
        str << '{' << val.first << ", {";
        for(size_t i = 0; i < val.second.size(); ++i) {
            if (i > 0) 
                str << ", ";
            str << val.second[i].value_or("<nullopt>");
        }
        str << "}}";
        return str;
    }

    static auto operator<<(ostream & str, const pair<const wstring, vector<WValue>> & val) -> ostream & {
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
    results.clear(); \
    REQUIRE_NOTHROW([&](){ try { parse(parser, args); CHECK(false); } failure }());
#define EXPECT_SUCCESS(args, expected) { \
    results.clear(); \
    REQUIRE_NOTHROW(parse(parser, args)); \
    CHECK(results == decltype(results)(expected)); \
}
#define ARGS(...) (initializer_list<const char *>{__VA_ARGS__})
#define WARGS(...) (initializer_list<const wchar_t *>{__VA_ARGS__})
#define RESULTS(...) (initializer_list<pair<const string, vector<Value>>>{__VA_ARGS__})
#define WRESULTS(...) (initializer_list<pair<const wstring, vector<WValue>>>{__VA_ARGS__})

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

#pragma region Test Options
//MARK: - Test Options

TEST_CASE( "Option with a single-dash option string" , "[adaptive]") {

    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_REQ_ARG("-x"));

    EXPECT_FAILURE(ARGS("-x"), MISSING_OPTION_ARGUMENT("-x"))
    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a"))
    EXPECT_FAILURE(ARGS("--foo", "ff"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("-x", "--foo"), MISSING_OPTION_ARGUMENT( "-x"))
    EXPECT_FAILURE(ARGS("-x", "-y"), MISSING_OPTION_ARGUMENT("-x"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-x", "a"), RESULTS({"-x", {"a"}}))
    EXPECT_SUCCESS(ARGS("-xa"), RESULTS({"-x", {"a"}}))
    EXPECT_SUCCESS(ARGS("-x", "-1"), RESULTS({"-x", {"-1"}}))
    EXPECT_SUCCESS(ARGS("-x-1"), RESULTS({"-x", {"-1"}}))
}

TEST_CASE( "Wide Option with a single-dash option string" , "[adaptive]") {

    map<wstring, vector<WValue>> results;

    WAdaptiveParser parser;
    parser.add(WOPTION_REQ_ARG(L"-x"));

    EXPECT_FAILURE(WARGS(L"-x"), WMISSING_OPTION_ARGUMENT(L"-x"))
    EXPECT_FAILURE(WARGS(L"a"), WEXTRA_POSITIONAL(L"a"))
    EXPECT_FAILURE(WARGS(L"--foo", L"ff"), WUNRECOGNIZED_OPTION(L"--foo"))
    EXPECT_FAILURE(WARGS(L"-x", L"--foo"), WMISSING_OPTION_ARGUMENT(L"-x"))
    EXPECT_FAILURE(WARGS(L"-x", L"-y"), WMISSING_OPTION_ARGUMENT(L"-x"))

    EXPECT_SUCCESS(WARGS(), WRESULTS())
    EXPECT_SUCCESS(WARGS(L"-x", L"a"), WRESULTS({L"-x", {L"a"}}))
    EXPECT_SUCCESS(WARGS(L"-xa"), WRESULTS({L"-x", {L"a"}}))
    EXPECT_SUCCESS(WARGS(L"-x", L"-1"), WRESULTS({L"-x", {L"-1"}}))
    EXPECT_SUCCESS(WARGS(L"-x-1"), WRESULTS({L"-x", {L"-1"}}))
}

TEST_CASE( "Combined single-dash options" , "[adaptive]") {

    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_NO_ARG("-x"));
    parser.add(OPTION_NO_ARG("-yyy"));
    parser.add(OPTION_REQ_ARG("-z"));

    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a")) 
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo")) 
    EXPECT_FAILURE(ARGS("-xa"), EXTRA_OPTION_ARGUMENT("-x")) 
    EXPECT_FAILURE(ARGS("-x", "--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("-x", "-z"), MISSING_OPTION_ARGUMENT("-z"))
    EXPECT_FAILURE(ARGS("-z", "-x"), MISSING_OPTION_ARGUMENT("-z"))
    EXPECT_FAILURE(ARGS("-yx"), UNRECOGNIZED_OPTION("-yx"))
    EXPECT_FAILURE(ARGS("-yz", "a"), UNRECOGNIZED_OPTION("-yz"))
    EXPECT_FAILURE(ARGS("-yyyx"), UNRECOGNIZED_OPTION( "-yyyx"))
    EXPECT_FAILURE(ARGS("-yyyza"), UNRECOGNIZED_OPTION( "-yyyza"))
    EXPECT_FAILURE(ARGS("-xyza"), EXTRA_OPTION_ARGUMENT("-x"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-x"), RESULTS({"-x", {"+"}}))
    EXPECT_SUCCESS(ARGS("-za"), RESULTS({"-z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-z=a"), RESULTS({"-z", {"=a"}}))
    EXPECT_SUCCESS(ARGS("-z", "a"), RESULTS({"-z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-xza"), RESULTS({"-x", {"+"}}, {"-z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-xz", "a"), RESULTS({"-x", {"+"}}, {"-z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-x", "-za"), RESULTS({"-x", {"+"}}, {"-z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-x", "-z", "a"), RESULTS({"-x", {"+"}}, {"-z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-y"), RESULTS({"-yyy", {"+"}}))
    EXPECT_SUCCESS(ARGS("-yyy"), RESULTS({"-yyy", {"+"}}))
    EXPECT_SUCCESS(ARGS("-x", "-yyy", "-za"), RESULTS({"-x", {"+"}}, {"-yyy", {"+"}}, {"-z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-x", "-yyy", "-z", "a"), RESULTS({"-x", {"+"}}, {"-yyy", {"+"}}, {"-z", {"a"}}))
}

TEST_CASE( "Option with a multi-character single-dash option string" , "[adaptive]") {

    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_REQ_ARG("-foo"));

    EXPECT_FAILURE(ARGS("-foo"), MISSING_OPTION_ARGUMENT("-foo"))
    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a")) 
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("-foo", "--foo"), MISSING_OPTION_ARGUMENT("-foo"))
    EXPECT_FAILURE(ARGS("-foo", "-y"), MISSING_OPTION_ARGUMENT("-foo"))
    EXPECT_FAILURE(ARGS("-fooa"), UNRECOGNIZED_OPTION("-fooa"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-foo", "a"), RESULTS({"-foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("-foo=a"), RESULTS({"-foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("-foo", "-1"), RESULTS({"-foo", {"-1"}}))
    EXPECT_SUCCESS(ARGS("-foo=-1"), RESULTS({"-foo", {"-1"}}))
    EXPECT_SUCCESS(ARGS("-fo", "a"), RESULTS({"-foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("-f", "a"), RESULTS({"-foo", {"a"}}))
    //DIFFERENCE FROM ArgParse. These are failure there
    EXPECT_SUCCESS(ARGS("-fo=a"), RESULTS({"-foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("-f=a"), RESULTS({"-foo", {"a"}}))
    //DIFFERENCE FROM ArgParse.
}

TEST_CASE( "Single dash options where option strings are subsets of each other" , "[adaptive]") {

    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_REQ_ARG("-f"));
    parser.add(OPTION_REQ_ARG("-foobar"));
    parser.add(OPTION_REQ_ARG("-foorab"));

    EXPECT_FAILURE(ARGS("-f"), MISSING_OPTION_ARGUMENT("-f"))
    EXPECT_FAILURE(ARGS("-fo"), AMBIGUOUS_OPTION("-fo", "-f", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-foo"), AMBIGUOUS_OPTION("-foo", "-f", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-foo", "b"), AMBIGUOUS_OPTION("-foo", "-f", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-foob"), AMBIGUOUS_OPTION("-foob", "-f", "-foobar"))
    EXPECT_FAILURE(ARGS("-fooba"), AMBIGUOUS_OPTION("-fooba", "-f", "-foobar"))
    EXPECT_FAILURE(ARGS("-foora"), AMBIGUOUS_OPTION("-foora", "-f", "-foorab"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-f", "a"), RESULTS({"-f", {"a"}}))
    EXPECT_SUCCESS(ARGS("-fa"), RESULTS({"-f", {"a"}}))
    EXPECT_SUCCESS(ARGS("-foa"), RESULTS({"-f", {"oa"}}))
    EXPECT_SUCCESS(ARGS("-fooa"), RESULTS({"-f", {"ooa"}}))
    EXPECT_SUCCESS(ARGS("-foobar", "a"), RESULTS({"-foobar", {"a"}}))
    EXPECT_SUCCESS(ARGS("-foorab", "a"), RESULTS({"-foorab", {"a"}}))
}

TEST_CASE( "Single dash options that partially match but are not subsets" , "[adaptive]") {

    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_REQ_ARG("-foobar"));
    parser.add(OPTION_REQ_ARG("-foorab"));

    EXPECT_FAILURE(ARGS("-f"), AMBIGUOUS_OPTION("-f", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-f", "a"), AMBIGUOUS_OPTION("-f", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-fa"), UNRECOGNIZED_OPTION("-fa"))
    EXPECT_FAILURE(ARGS("-foa"), UNRECOGNIZED_OPTION("-foa"))
    EXPECT_FAILURE(ARGS("-foo"), AMBIGUOUS_OPTION("-foo", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-fo"), AMBIGUOUS_OPTION("-fo", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-foo", "b"), AMBIGUOUS_OPTION("-foo", "-foobar", "-foorab"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-foob", "a"), RESULTS({"-foobar", {"a"}}))
    EXPECT_SUCCESS(ARGS("-foor", "a"), RESULTS({"-foorab", {"a"}}))
    EXPECT_SUCCESS(ARGS("-fooba", "a"), RESULTS({"-foobar", {"a"}}))
    EXPECT_SUCCESS(ARGS("-foora", "a"), RESULTS({"-foorab", {"a"}}))
    EXPECT_SUCCESS(ARGS("-foobar", "a"), RESULTS({"-foobar", {"a"}}))
    EXPECT_SUCCESS(ARGS("-foorab", "a"), RESULTS({"-foorab", {"a"}}))
}

TEST_CASE( "Short option with a numeric option string" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_REQ_ARG("-1"));

    EXPECT_FAILURE(ARGS("-1"), MISSING_OPTION_ARGUMENT("-1"))
    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a"))
    EXPECT_FAILURE(ARGS("-1", "--foo"), MISSING_OPTION_ARGUMENT("-1"))
    EXPECT_FAILURE(ARGS("-1", "-y"), MISSING_OPTION_ARGUMENT("-1"))
    EXPECT_FAILURE(ARGS("-1", "-1"), MISSING_OPTION_ARGUMENT("-1"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-1", "a"), RESULTS({"-1", {"a"}}))
    EXPECT_SUCCESS(ARGS("-1a"), RESULTS({"-1", {"a"}}))
    //DIFFERENCE FROM ArgParse. This is failure there
    EXPECT_SUCCESS(ARGS("-1", "-2"), RESULTS({"-1", {"-2"}}))
    //DIFFERENCE FROM ArgParse
    EXPECT_SUCCESS(ARGS("-1-2"), RESULTS({"-1", {"-2"}}))
}

TEST_CASE( "Option with a double-dash option string" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_REQ_ARG("--foo"));

    EXPECT_FAILURE(ARGS("--foo"), MISSING_OPTION_ARGUMENT("--foo"))
    EXPECT_FAILURE(ARGS("-f"), UNRECOGNIZED_OPTION("-f"))
    EXPECT_FAILURE(ARGS("-f", "a"), UNRECOGNIZED_OPTION("-f"))
    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a"))
    EXPECT_FAILURE(ARGS("--foo", "-x"), MISSING_OPTION_ARGUMENT("--foo"))
    EXPECT_FAILURE(ARGS("--foo", "--bar"), MISSING_OPTION_ARGUMENT("--foo"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("--foo", "a"), RESULTS({"--foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("--foo=a"), RESULTS({"--foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("--foo", "-2.5"), RESULTS({"--foo", {"-2.5"}}))
    EXPECT_SUCCESS(ARGS("--foo=-2.5"), RESULTS({"--foo", {"-2.5"}}))
}

TEST_CASE( "Partial matching with a double-dash option string" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_NO_ARG("--badger"));
    parser.add(OPTION_REQ_ARG("--bat"));

    EXPECT_FAILURE(ARGS("--bar"), UNRECOGNIZED_OPTION("--bar"))
    EXPECT_FAILURE(ARGS("--b"), AMBIGUOUS_OPTION("--b", "--badger", "--bat"))
    EXPECT_FAILURE(ARGS("--ba"), AMBIGUOUS_OPTION("--ba", "--badger", "--bat"))
    EXPECT_FAILURE(ARGS("--b=2"), AMBIGUOUS_OPTION("--b", "--badger", "--bat"))
    EXPECT_FAILURE(ARGS("--ba=4"), AMBIGUOUS_OPTION("--ba", "--badger", "--bat"))
    EXPECT_FAILURE(ARGS("--badge", "5"), EXTRA_POSITIONAL("5"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("--bat", "X"), RESULTS({"--bat", {"X"}}))
    EXPECT_SUCCESS(ARGS("--bad"), RESULTS({"--badger", {"+"}}))
    EXPECT_SUCCESS(ARGS("--badg"), RESULTS({"--badger", {"+"}}))
    EXPECT_SUCCESS(ARGS("--badge"), RESULTS({"--badger", {"+"}}))
    EXPECT_SUCCESS(ARGS("--badger"), RESULTS({"--badger", {"+"}}))
}

TEST_CASE( "Wide Partial matching with a double-dash option string" , "[adaptive]") {
    map<wstring, vector<WValue>> results;

    WAdaptiveParser parser;
    parser.add(WOPTION_NO_ARG(L"--badger"));
    parser.add(WOPTION_REQ_ARG(L"--bat"));

    EXPECT_FAILURE(WARGS(L"--bar"), WUNRECOGNIZED_OPTION(L"--bar"))
    EXPECT_FAILURE(WARGS(L"--b"), WAMBIGUOUS_OPTION(L"--b", L"--badger", L"--bat"))
    EXPECT_FAILURE(WARGS(L"--ba"), WAMBIGUOUS_OPTION(L"--ba", L"--badger", L"--bat"))
    EXPECT_FAILURE(WARGS(L"--b=2"), WAMBIGUOUS_OPTION(L"--b", L"--badger", L"--bat"))
    EXPECT_FAILURE(WARGS(L"--ba=4"), WAMBIGUOUS_OPTION(L"--ba", L"--badger", L"--bat"))
    EXPECT_FAILURE(WARGS(L"--badge", L"5"), WEXTRA_POSITIONAL(L"5"))

    EXPECT_SUCCESS(WARGS(), WRESULTS())
    EXPECT_SUCCESS(WARGS(L"--bat", L"X"), WRESULTS({L"--bat", {L"X"}}))
    EXPECT_SUCCESS(WARGS(L"--bad"), WRESULTS({L"--badger", {L"+"}}))
    EXPECT_SUCCESS(WARGS(L"--badg"), WRESULTS({L"--badger", {L"+"}}))
    EXPECT_SUCCESS(WARGS(L"--badge"), WRESULTS({L"--badger", {L"+"}}))
    EXPECT_SUCCESS(WARGS(L"--badger"), WRESULTS({L"--badger", {L"+"}}))
}

TEST_CASE( "One double-dash option string is a prefix of another" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_NO_ARG("--badger"));
    parser.add(OPTION_REQ_ARG("--ba"));

    EXPECT_FAILURE(ARGS("--bar"), UNRECOGNIZED_OPTION("--bar"))
    EXPECT_FAILURE(ARGS("--b"), AMBIGUOUS_OPTION("--b", "--ba", "--badger"))
    EXPECT_FAILURE(ARGS("--ba"), MISSING_OPTION_ARGUMENT("--ba"))
    EXPECT_FAILURE(ARGS("--b=2"), AMBIGUOUS_OPTION("--b", "--ba", "--badger"))
    EXPECT_FAILURE(ARGS("--badge", "5"), EXTRA_POSITIONAL("5"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("--ba", "X"), RESULTS({"--ba", {"X"}}))
    EXPECT_SUCCESS(ARGS("--ba=X"), RESULTS({"--ba", {"X"}}))
    EXPECT_SUCCESS(ARGS("--bad"), RESULTS({"--badger", {"+"}}))
    EXPECT_SUCCESS(ARGS("--badg"), RESULTS({"--badger", {"+"}}))
    EXPECT_SUCCESS(ARGS("--badge"), RESULTS({"--badger", {"+"}}))
    EXPECT_SUCCESS(ARGS("--badger"), RESULTS({"--badger", {"+"}}))
}

TEST_CASE( "Mix of options with single- and double-dash option strings" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_NO_ARG("-f"));
    parser.add(OPTION_REQ_ARG("--bar"));
    parser.add(OPTION_NO_ARG("-baz"));

    EXPECT_FAILURE(ARGS("--bar"), MISSING_OPTION_ARGUMENT("--bar"))
    EXPECT_FAILURE(ARGS("-fbar"), EXTRA_OPTION_ARGUMENT("-f"))
    EXPECT_FAILURE(ARGS("-fbaz"), EXTRA_OPTION_ARGUMENT("-f"))
    EXPECT_FAILURE(ARGS("-bazf"), UNRECOGNIZED_OPTION("-bazf"))
    EXPECT_FAILURE(ARGS("-b", "B"), EXTRA_POSITIONAL("B"))
    EXPECT_FAILURE(ARGS("B"), EXTRA_POSITIONAL("B"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-f"), RESULTS({"-f", {"+"}}))
    EXPECT_SUCCESS(ARGS("--ba", "B"), RESULTS({"--bar", {"B"}}))
    EXPECT_SUCCESS(ARGS("-f", "--ba", "B"), RESULTS({"-f", {"+"}}, {"--bar", {"B"}}))
    EXPECT_SUCCESS(ARGS("-f", "-b"), RESULTS({"-f", {"+"}}, {"-baz", {"+"}}))
    EXPECT_SUCCESS(ARGS("-ba", "-f"), RESULTS({"-f", {"+"}}, {"-baz", {"+"}}))
}

TEST_CASE( "Combination of single- and double-dash option strings for an option" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_NO_ARG("-v", "--verbose", "-n", "--noisy"));

    EXPECT_FAILURE(ARGS("--x", "--verbose"), UNRECOGNIZED_OPTION("--x"))
    EXPECT_FAILURE(ARGS("-N"), UNRECOGNIZED_OPTION("-N"))
    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a"))
    EXPECT_FAILURE(ARGS("-v", "x"), EXTRA_POSITIONAL("x"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-v"), RESULTS({"-v", {"+"}}))
    EXPECT_SUCCESS(ARGS("--verbose"), RESULTS({"-v", {"+"}}))
    EXPECT_SUCCESS(ARGS("-n"), RESULTS({"-v", {"+"}}))
    EXPECT_SUCCESS(ARGS("--noisy"), RESULTS({"-v", {"+"}}))
}

TEST_CASE( "Allow long options to be abbreviated unambiguously" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_REQ_ARG("--foo"));
    parser.add(OPTION_REQ_ARG("--foobaz"));
    parser.add(OPTION_NO_ARG("--fooble"));


    EXPECT_FAILURE(ARGS("--foob", "5"), AMBIGUOUS_OPTION("--foob", "--foobaz", "--fooble"))
    EXPECT_FAILURE(ARGS("--foob"), AMBIGUOUS_OPTION("--foob", "--foobaz", "--fooble"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("--foo", "7"), RESULTS({"--foo", {"7"}}))
    EXPECT_SUCCESS(ARGS("--fooba", "a"), RESULTS({"--foobaz", {"a"}}))
    EXPECT_SUCCESS(ARGS("--foobl", "--foo", "g"), RESULTS({"--foo", {"g"}}, {"--fooble", {"+"}}))
}

TEST_CASE( "Disallow abbreviation settin" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser(AdaptiveParser::Settings::commonUnix().allowAbbreviation(false));
    parser.add(OPTION_REQ_ARG("--foo", "-foo"));
    parser.add(OPTION_NO_ARG("--foodle", "-foodle"));
    parser.add(OPTION_REQ_ARG("--foonly", "-foonly"));


    EXPECT_FAILURE(ARGS("-foon", "3"), UNRECOGNIZED_OPTION("-foon"))
    EXPECT_FAILURE(ARGS("--foon", "3"), UNRECOGNIZED_OPTION("--foon"))
    EXPECT_FAILURE(ARGS("--food"), UNRECOGNIZED_OPTION("--food"))
    EXPECT_FAILURE(ARGS("--food", "--foo", "2"), UNRECOGNIZED_OPTION("--food"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("--foo", "3"), RESULTS({"--foo", {"3"}}))
    EXPECT_SUCCESS(ARGS("--foonly", "7", "--foodle", "--foo", "2"), RESULTS({"--foo", {"2"}}, {"--foodle", {"+"}}, {"--foonly", {"7"}}))
}

TEST_CASE( "Custom prefixes" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser(AdaptiveParser::Settings()
        .addLongPrefix("::")
        .addShortPrefix('+')
        .addShortPrefix('/')
        .addValueDelimiter('|')
        .addOptionStopSequence("^^")
    );
    parser.add(OPTION_NO_ARG("+f"));
    parser.add(OPTION_REQ_ARG("::bar"));
    parser.add(OPTION_NO_ARG("/baz"));

    EXPECT_FAILURE(ARGS("--bar"), EXTRA_POSITIONAL("--bar"))
    EXPECT_FAILURE(ARGS("-fbar"), EXTRA_POSITIONAL("-fbar"))
    EXPECT_FAILURE(ARGS("-b", "B"), EXTRA_POSITIONAL("-b"))
    EXPECT_FAILURE(ARGS("B"), EXTRA_POSITIONAL("B"))
    EXPECT_FAILURE(ARGS("-f"), EXTRA_POSITIONAL("-f"))
    EXPECT_FAILURE(ARGS("--bar", "B"), EXTRA_POSITIONAL("--bar"))
    EXPECT_FAILURE(ARGS("-baz"), EXTRA_POSITIONAL("-baz"))
    EXPECT_FAILURE(ARGS("::ba", "^^", "B"),MISSING_OPTION_ARGUMENT("::ba"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("+f"), RESULTS({"+f", {"+"}}))
    EXPECT_SUCCESS(ARGS("::ba", "B"), RESULTS({"::bar", {"B"}}))
    EXPECT_SUCCESS(ARGS("::ba|B"), RESULTS({"::bar", {"B"}}))
    EXPECT_SUCCESS(ARGS("+f", "::bar", "B"), RESULTS({"+f", {"+"}}, {"::bar", {"B"}}))
    EXPECT_SUCCESS(ARGS("+f", "/b"), RESULTS({"+f", {"+"}}, {"/baz", {"+"}}))
    EXPECT_SUCCESS(ARGS("/ba", "+f"), RESULTS({"+f", {"+"}}, {"/baz", {"+"}}))
}

TEST_CASE( "Equivalent custom prefixes" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser::Settings settings;
    settings.addLongPrefix("::", ":")
        .addShortPrefix('+', '_')
        .addShortPrefix('/', '&')
        .addValueDelimiter('|')
        .addValueDelimiter('*')
        .addOptionStopSequence("^^", "%%");
    AdaptiveParser parser(std::move(settings));
    parser.add(OPTION_NO_ARG("+f"));
    parser.add(OPTION_REQ_ARG("::bar"));
    parser.add(OPTION_NO_ARG("/baz"));

    EXPECT_FAILURE(ARGS("--bar"), EXTRA_POSITIONAL("--bar"))
    EXPECT_FAILURE(ARGS("-fbar"), EXTRA_POSITIONAL("-fbar"))
    EXPECT_FAILURE(ARGS("-b", "B"), EXTRA_POSITIONAL("-b"))
    EXPECT_FAILURE(ARGS("B"), EXTRA_POSITIONAL("B"))
    EXPECT_FAILURE(ARGS("-f"), EXTRA_POSITIONAL("-f"))
    EXPECT_FAILURE(ARGS("--bar", "B"), EXTRA_POSITIONAL("--bar"))
    EXPECT_FAILURE(ARGS("-baz"), EXTRA_POSITIONAL("-baz"))
    EXPECT_FAILURE(ARGS("::ba", "^^", "B"),MISSING_OPTION_ARGUMENT("::ba"))
    EXPECT_FAILURE(ARGS("::ba", "%%", "B"),MISSING_OPTION_ARGUMENT("::ba"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("_f"), RESULTS({"+f", {"+"}}))
    EXPECT_SUCCESS(ARGS(":ba", "B"), RESULTS({"::bar", {"B"}}))
    EXPECT_SUCCESS(ARGS("::ba*A|B"), RESULTS({"::bar", {"A|B"}}))
    EXPECT_SUCCESS(ARGS("::ba|A*B"), RESULTS({"::bar", {"A*B"}}))
    EXPECT_SUCCESS(ARGS("_f", ":bar", "B"), RESULTS({"+f", {"+"}}, {"::bar", {"B"}}))
    EXPECT_SUCCESS(ARGS("_f", "&b"), RESULTS({"+f", {"+"}}, {"/baz", {"+"}}))
    EXPECT_SUCCESS(ARGS("&ba", "_f"), RESULTS({"+f", {"+"}}, {"/baz", {"+"}}))
}


TEST_CASE( "Optional arg for an option" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_OPT_ARG("-w", "--work"));

    EXPECT_FAILURE(ARGS("2"), EXTRA_POSITIONAL("2"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-w"), RESULTS({"-w", {nullopt}}))
    EXPECT_SUCCESS(ARGS("-w", "2"), RESULTS({"-w", {"2"}}))
    EXPECT_SUCCESS(ARGS("--work"), RESULTS({"-w", {nullopt}}))
    EXPECT_SUCCESS(ARGS("--work", "2"), RESULTS({"-w", {"2"}}))
    EXPECT_SUCCESS(ARGS("--work=2"), RESULTS({"-w", {"2"}}))
}

TEST_CASE( "Required option" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_OPT_ARG("-w", "--work").occurs(Once));

    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a"))
    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: option -w must be present"))

    EXPECT_SUCCESS(ARGS("-w"), RESULTS({"-w", {nullopt}}))
    EXPECT_SUCCESS(ARGS("-w", "42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("-w42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("--work"), RESULTS({"-w", {nullopt}}))
    EXPECT_SUCCESS(ARGS("--work", "42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("--work=42"), RESULTS({"-w", {"42"}}))
}

TEST_CASE( "Repeat one-or-more option" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_OPT_ARG("-w", "--work").occurs(OneOrMoreTimes));

    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a"))
    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: option -w must be present"))

    EXPECT_SUCCESS(ARGS("-w"), RESULTS({"-w", {nullopt}}))
    EXPECT_SUCCESS(ARGS("-w", "42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("-w42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("--work"), RESULTS({"-w", {nullopt}}))
    EXPECT_SUCCESS(ARGS("--work", "42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("--work=42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("-ww", "42"), RESULTS({"-w", {nullopt, "42"}}))
    EXPECT_SUCCESS(ARGS("-www", "42"), RESULTS({"-w", {nullopt, nullopt, "42"}}))
    EXPECT_SUCCESS(ARGS("--work", "42", "-w"), RESULTS({"-w", {"42", nullopt}}))
}

TEST_CASE( "Repeat 2-or-3 option" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_OPT_ARG("-w", "--work").occurs(Quantifier(2,3)));

    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a"))
    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: option -w must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("-w"), VALIDATION_ERROR("invalid arguments: option -w must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("-w", "-w", "--work", "--wo"), VALIDATION_ERROR("invalid arguments: option -w must occur at most 3 times"))

    EXPECT_SUCCESS(ARGS("-ww", "42"), RESULTS({"-w", {nullopt, "42"}}))
    EXPECT_SUCCESS(ARGS("-www", "42"), RESULTS({"-w", {nullopt, nullopt, "42"}}))
    EXPECT_SUCCESS(ARGS("--work=42", "-w", "-w34"), RESULTS({"-w", {"42", nullopt, "34"}}))
}

#pragma endregion

#pragma region Test positionals
//MARK: - Test positionals

TEST_CASE( "Simple positional" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    EXPECT_FAILURE(ARGS("a", "b"), EXTRA_POSITIONAL("b"))

    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
}

TEST_CASE( "Positional with explicit repeat once" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(OneTime));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    EXPECT_FAILURE(ARGS("a", "b"), EXTRA_POSITIONAL("b"))

    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
}

TEST_CASE( "Positional with explicit repeat twice" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(Quantifier(2)));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))

    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a", "b"}}))
}

TEST_CASE( "Positional with unlimited repeat" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(ZeroOrMoreTimes));

    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a", "b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b", "c"}}))
}

TEST_CASE( "Positional with one or more repeat" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(OneOrMoreTimes));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a", "b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b", "c"}}))
}

TEST_CASE( "Positional with zero or once repeat" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(ZeroOrOneTime));

    EXPECT_FAILURE(ARGS("a", "b"), EXTRA_POSITIONAL("b"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
}

TEST_CASE( "Two positionals" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
}

TEST_CASE( "Positional with no explict repeat followed by one with 1" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(Once));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
}

TEST_CASE( "Positional with repeat 2 followed by one with 1" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(2));
    parser.add(POSITIONAL("bar"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("a", "b"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("a", "b", "c", "d"), EXTRA_POSITIONAL("d"))
    
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b"}}, {"bar", {"c"}}))
}

TEST_CASE( "Positional with repeat 1 followed by one with unlimited" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(ZeroOrMoreTimes));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}))
}

TEST_CASE( "Positional with repeat 1 followed by one with one or more" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(OneOrMoreTimes));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}))
}

TEST_CASE( "Positional with repeat 1 followed by one with an optional" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(NeverOrOnce));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
}

TEST_CASE( "Positional with unlimited repeat followed by one with 1" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(ZeroOrMoreTimes));
    parser.add(POSITIONAL("bar"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"bar", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b"}}, {"bar", {"c"}}))
}

TEST_CASE( "Positional with one or more repeat followed by one with 1" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(OneOrMoreTimes));
    parser.add(POSITIONAL("bar"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b"}}, {"bar", {"c"}}))
}
TEST_CASE( "Positional with an optional repeat followed by one with 1" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(NeverOrOnce));
    parser.add(POSITIONAL("bar"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"bar", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
}

TEST_CASE( "Positional with repeat 2 followed by one with unlimited" , "[adaptive]") {
    map<wstring, vector<WValue>> results;

    WAdaptiveParser parser;
    parser.add(WPOSITIONAL(L"foo").occurs(2));
    parser.add(WPOSITIONAL(L"bar").occurs(ZeroOrMoreTimes));

    EXPECT_FAILURE(WARGS(), WVALIDATION_ERROR(L"invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(WARGS(L"--foo"), WUNRECOGNIZED_OPTION(L"--foo"))
    EXPECT_FAILURE(WARGS(L"a"), WVALIDATION_ERROR(L"invalid arguments: positional argument foo must occur at least 2 times"))
    
    EXPECT_SUCCESS(WARGS(L"a", L"b"), WRESULTS({L"foo", {L"a", L"b"}}))
    EXPECT_SUCCESS(WARGS(L"a", L"b", L"c"), WRESULTS({L"foo", {L"a", L"b"}}, {L"bar", {L"c"}}))
}

TEST_CASE( "Positional with repeat 2 followed by one with one or more" , "[adaptive]") {
    map<wstring, vector<WValue>> results;

    WAdaptiveParser parser;
    parser.add(WPOSITIONAL(L"foo").occurs(2));
    parser.add(WPOSITIONAL(L"bar").occurs(OneOrMoreTimes));

    EXPECT_FAILURE(WARGS(), WVALIDATION_ERROR(L"invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(WARGS(L"--foo"), WUNRECOGNIZED_OPTION(L"--foo"))
    EXPECT_FAILURE(WARGS(L"a"), WVALIDATION_ERROR(L"invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(WARGS(L"a", L"b"), WVALIDATION_ERROR(L"invalid arguments: positional argument bar must be present"))
    
    EXPECT_SUCCESS(WARGS(L"a", L"b", L"c"), WRESULTS({L"foo", {L"a", L"b"}}, {L"bar", {L"c"}}))
    EXPECT_SUCCESS(WARGS(L"a", L"b", L"c", L"d"), WRESULTS({L"foo", {L"a", L"b"}}, {L"bar", {L"c", L"d"}}))
}

TEST_CASE( "Positional with repeat 2 followed by one with optional" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(2));
    parser.add(POSITIONAL("bar").occurs(NeverOrOnce));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("a", "b", "c", "d"), EXTRA_POSITIONAL("d"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a", "b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b"}}, {"bar", {"c"}}))
}

TEST_CASE( "Three positionals with repeats: 1, *, 1" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(ZeroOrMoreTimes));
    parser.add(POSITIONAL("baz"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument baz must be present"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"baz", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}, {"baz", {"c"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c", "d"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}, {"baz", {"d"}}))
}

TEST_CASE( "Three positionals with repeats: 1, +, 1" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(OneOrMoreTimes));
    parser.add(POSITIONAL("baz"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("a", "b"), VALIDATION_ERROR("invalid arguments: positional argument baz must be present"))
    
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}, {"baz", {"c"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c", "d"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}, {"baz", {"d"}}))
}

TEST_CASE( "Three positionals with repeats: 1, ?, 1" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(NeverOrOnce));
    parser.add(POSITIONAL("baz"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument baz must be present"))
    EXPECT_FAILURE(ARGS("a", "b", "c", "d"), EXTRA_POSITIONAL("d"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"baz", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}, {"baz", {"c"}}))
}

TEST_CASE( "Two positionals with repeats: ?, ?" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(NeverOrOnce));
    parser.add(POSITIONAL("bar").occurs(NeverOrOnce));

    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
}

TEST_CASE( "Two positionals with repeats: ?, *" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(NeverOrOnce));
    parser.add(POSITIONAL("bar").occurs(ZeroOrMoreTimes));

    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}))
}

TEST_CASE( "Two positionals with repeats: ?, +" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(NeverOrOnce));
    parser.add(POSITIONAL("bar").occurs(OnceOrMore));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"bar", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}))
}

#pragma endregion

#pragma region Test combined Options and Positionals
//MARK: - Test combined Options and Positionals

TEST_CASE( "Negative number args when numeric options are present" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("x").occurs(NeverOrOnce));
    parser.add(OPTION_NO_ARG("-4"));

    EXPECT_SUCCESS(ARGS(), RESULTS())
    //DIFFERENCE FROM ArgParse. These are failure there
    EXPECT_SUCCESS(ARGS("-2"), RESULTS({"x", {"-2"}}))
    EXPECT_SUCCESS(ARGS("-315"), RESULTS({"x", {"-315"}}))
    //DIFFERENCE FROM ArgParse.
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"x", {"a"}}))
    EXPECT_SUCCESS(ARGS("-4"), RESULTS({"-4", {"+"}}))
    EXPECT_SUCCESS(ARGS("-4", "-2"), RESULTS({"-4", {"+"}}, {"x", {"-2"}}))
}

TEST_CASE( "Empty arguments" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("x").occurs(NeverOrOnce));
    parser.add(OPTION_REQ_ARG("-y", "--yyy"));

    EXPECT_FAILURE(ARGS("-y"), MISSING_OPTION_ARGUMENT("-y"))

    EXPECT_SUCCESS(ARGS(""), RESULTS({"x", {""}}))
    EXPECT_SUCCESS(ARGS("-y", ""), RESULTS({"-y", {""}}))
}

TEST_CASE( "Prefix character only arguments" , "[adaptive]") {
    
    map<string, vector<Value>> results;

    {
        AdaptiveParser parser(AdaptiveParser::Settings::commonUnix().addShortPrefix("+"));
        //DIFFERENCE FROM ArgParse. These are legal there
        CHECK_THROWS_AS(parser.add(OPTION_NO_ARG("-")), invalid_argument);
        CHECK_THROWS_AS(parser.add(OPTION_NO_ARG("+")), invalid_argument);
        //DIFFERENCE FROM ArgParse
    }
    {

        AdaptiveParser parser(AdaptiveParser::Settings::commonUnix().addShortPrefix("+"));
        
        parser.add(POSITIONAL("foo").occurs(NeverOrOnce));
        parser.add(POSITIONAL("bar").occurs(NeverOrOnce));
        parser.add(OPTION_NO_ARG("-+-"));

        EXPECT_FAILURE(ARGS("-y"), UNRECOGNIZED_OPTION("-y"))

        EXPECT_SUCCESS(ARGS(), RESULTS())
        EXPECT_SUCCESS(ARGS("-"), RESULTS({"foo", {"-"}}))
        EXPECT_SUCCESS(ARGS("-", "+"), RESULTS({"foo", {"-"}}, {"bar", {"+"}}))
        EXPECT_SUCCESS(ARGS("-+-"), RESULTS({"-+-", {"+"}}))
    }
}


#pragma endregion

// TEST_CASE( "xxx" , "[sequential]") {

//     AdaptiveParser parser;
    

//     int verbosity = 0;
//     string name;

//     parser.add(
//         Option("-v")
//         .help("xcbfdjd")
//         .handler([&]() {
//             ++verbosity;
//         }));
//     parser.add(
//         Option("--name", "-n")
//         .help("dfsd\nhjjkll")
//         .handler([&](string_view value) {
//             name = value;
//         }));
//     parser.add(
//         Positional("bob")
//         .occurs(Quantifier(0, 25))
//         .help("hohahaha")
//         .handler([](unsigned idx, string_view) {
//             CHECK(idx == 0);
//         })
//     );
//     parser.add(
//         Positional("fob")
//         .occurs(Quantifier(1,1))
//         .help("ghakl\njdks")
//         .handler([](unsigned, string_view) {
        
//         })
//     );
    
//     parser.addValidator(OptionRequired("hah") && OptionRequired("heh"));

//     parser.addValidator([](const auto &) { return true; }, "hello");

//     std::cout << '\n' << parser.formatUsage("ggg");

//     const char * argv[] = { "-v", "hhh", "jjj" };
//     try {
//         parser.parse(std::begin(argv), std::end(argv));
//     } catch (ParsingException & ex) {
//         std::cout << ex.message();
//     }
//     //CHECK(verbosity == 2);
//     //CHECK(name == "world");

//    WAdaptiveParser wparser;
//     wparser.add(
//         WPositional(L"fob")
//         .occurs(Quantifier(7,7))
//         .help(L"ghakl\njdks")
//         .handler([](unsigned, wstring_view) {

//             })
//     );
//     const wchar_t * wargv[] = { L"-v", L"hhh", L"jjj" };
//     try {
//         wparser.parse(std::begin(wargv), std::end(wargv));
//     } catch (WParsingException & ex) {
//         std::wcout << ex.message();
//     }
// }
