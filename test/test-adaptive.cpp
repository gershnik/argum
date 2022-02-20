#include <modern-arg-parser/adaptive-parser.h>

#include "catch.hpp"

#include <iostream>

using namespace MArgP;
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

template<class Char>
static auto parse(const BasicAdaptiveParser<Char> & parser, initializer_list<const Char *> args) {
    parser.parse(args.begin(), args.end());
}

using Value = optional<string>;

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
}

#define UNRECOGNIZED_OPTION(x) catch(UnrecognizedOption & ex) { CHECK(ex.option == (x)); }
#define AMBIGUOUS_OPTION(x, ...) catch(AmbiguousOption & ex) { CHECK(ex.option == (x)); CHECK(ex.possibilities == vector<string>{__VA_ARGS__}); }
#define EXTRA_POSITIONAL(x) catch(ExtraPositional & ex) { CHECK(ex.value == (x)); }
#define MISSING_OPTION_ARGUMENT(x) catch(MissingOptionArgument & ex) { CHECK(ex.option == (x)); }
#define EXTRA_OPTION_ARGUMENT(x) catch(ExtraOptionArgument & ex) { CHECK(ex.option == (x)); }
#define VALIDATION_ERROR(x) catch(ValidationError & ex) { CHECK(ex.message() == (x)); }

#define EXPECT_FAILURE(args, failure) REQUIRE_NOTHROW([&](){ try { parse(parser, args); CHECK(false); } failure }());
#define EXPECT_SUCCESS(args, expected) { \
    results.clear(); \
    REQUIRE_NOTHROW(parse(parser, args)); \
    CHECK(results == map<string, vector<Value>>(expected)); \
}
#define ARGS(...) (initializer_list<const char *>{__VA_ARGS__})
//#define WARGS(...) (initializer_list<const wchar_t *>{__VA_ARGS__})
#define RESULTS(...) (initializer_list<pair<const string, vector<Value>>>{__VA_ARGS__})

#define OPTION_NO_ARG(n, ...) Option(n __VA_OPT__(,) __VA_ARGS__).setHandler([&](){ \
        results[n].push_back("+"); \
    })
#define OPTION_REQ_ARG(n, ...) Option(n __VA_OPT__(,) __VA_ARGS__).setHandler([&](string_view arg){ \
        results[n].push_back(string(arg)); \
    })
#define OPTION_OPT_ARG(n, ...) Option(n __VA_OPT__(,) __VA_ARGS__).setHandler([&](optional<string_view> arg){ \
        arg ? results[n].push_back(string(*arg)) : results[n].push_back(nullopt); \
    })

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
    EXPECT_SUCCESS(ARGS("-foo", "-1"), RESULTS({"-foo", {"-1"}}))
    EXPECT_SUCCESS(ARGS("-fo", "a"), RESULTS({"-foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("-f", "a"), RESULTS({"-foo", {"a"}}))
}

TEST_CASE( "Single dash options where option strings are subsets of each other" , "[adaptive]") {

    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_REQ_ARG("-f"));
    parser.add(OPTION_REQ_ARG("-foobar"));
    parser.add(OPTION_REQ_ARG("-foorab"));

    EXPECT_FAILURE(ARGS("-f"), MISSING_OPTION_ARGUMENT("-f"))
    EXPECT_FAILURE(ARGS("-fo"), AMBIGUOUS_OPTION("-fo", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-foo"), AMBIGUOUS_OPTION("-foo", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-foo", "b"), AMBIGUOUS_OPTION("-foo", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-foob"), MISSING_OPTION_ARGUMENT("-foob"))
    EXPECT_FAILURE(ARGS("-fooba"), MISSING_OPTION_ARGUMENT("-fooba"))
    EXPECT_FAILURE(ARGS("-foora"), MISSING_OPTION_ARGUMENT("-foora"))

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
    parser.add(OPTION_OPT_ARG("-w", "--work").setRepeated(Repeated::once));

    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a"))
    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: option -w must be present"))

    EXPECT_SUCCESS(ARGS("-w"), RESULTS({"-w", {nullopt}}))
    EXPECT_SUCCESS(ARGS("-w", "42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("-w42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("--work"), RESULTS({"-w", {nullopt}}))
    EXPECT_SUCCESS(ARGS("--work", "42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("--work=42"), RESULTS({"-w", {"42"}}))
}

// TEST_CASE( "xxx" , "[sequential]") {

//     AdaptiveParser parser;
    

//     int verbosity = 0;
//     string name;

//     parser.add(
//         Option("-v")
//         .setDescription("xcbfdjd")
//         .setHandler([&]() {
//             ++verbosity;
//         }));
//     parser.add(
//         Option("--name", "-n")
//         .setDescription("dfsd\nhjjkll")
//         .setHandler([&](string_view value) {
//             name = value;
//         }));
//     parser.add(
//         Positional("bob")
//         .set(Repeated(0, 25))
//         .setDescription("hohahaha")
//         .setHandler([](unsigned idx, string_view) {
//             CHECK(idx == 0);
//         })
//     );
//     parser.add(
//         Positional("fob")
//         .set(Repeated(1,1))
//         .setDescription("ghakl\njdks")
//         .setHandler([](unsigned, string_view) {
        
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
//         .set(Repeated(7,7))
//         .setDescription(L"ghakl\njdks")
//         .setHandler([](unsigned, wstring_view) {

//             })
//     );
//     const wchar_t * wargv[] = { L"-v", L"hhh", L"jjj" };
//     try {
//         wparser.parse(std::begin(wargv), std::end(wargv));
//     } catch (WParsingException & ex) {
//         std::wcout << ex.message();
//     }
// }
