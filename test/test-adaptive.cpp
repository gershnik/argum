#include <modern-arg-parser/modern-arg-parser.h>

#include "catch.hpp"

#include <iostream>

using namespace MArgP;
using namespace std;

using namespace std::literals;


TEST_CASE( "xxx" , "[sequential]") {

    AdaptiveParser parser;
    using Option = AdaptiveParser::Option;
    using Positional = AdaptiveParser::Positional;

    int verbosity = 0;
    string name;

    parser.add(
        Option("-v")
        .setDescription("xcbfdjd")
        .setHandler([&]() {
            ++verbosity;
        }));
    parser.add(
        Option("--name", "-n")
        .setDescription("dfsd\nhjjkll")
        .setHandler([&](string_view value) {
            name = value;
        }));
    parser.add(
        Positional("bob")
        .set(Repeated(0, 25))
        .setDescription("hohahaha")
        .setHandler([](unsigned idx, string_view) {
            CHECK(idx == 0);
        })
    );
    parser.add(
        Positional("fob")
        .set(Repeated(1,1))
        .setDescription("ghakl\njdks")
        .setHandler([](unsigned, string_view) {
        
        })
    );
    
    parser.addValidator(OptionRequired("hah") && OptionRequired("heh"));

    parser.addValidator([](const auto &) { return true; }, "hello");

    std::cout << '\n' << parser.formatUsage("ggg");

    const char * argv[] = { "-v", "hhh", "jjj" };
    try {
        parser.parse(std::begin(argv), std::end(argv));
    } catch (ParsingException & ex) {
        std::cout << ex.message();
    }
    CHECK(verbosity == 2);
    CHECK(name == "world");

    using WPositional = WAdaptiveParser::Positional;

    WAdaptiveParser wparser;
    wparser.add(
        WPositional(L"fob")
        .set(Repeated(7,7))
        .setDescription(L"ghakl\njdks")
        .setHandler([](unsigned, wstring_view) {

            })
    );
    const wchar_t * wargv[] = { L"-v", L"hhh", L"jjj" };
    try {
        wparser.parse(std::begin(wargv), std::end(wargv));
    } catch (WParsingException & ex) {
        std::wcout << ex.message();
    }
}
