#include "parser-common.h"

TEST_CASE( "xxx" , "[parser]") {

    AdaptiveParser parser;
    

    int verbosity = 0;
    string name;

    parser.add(
        Option("-v")
        .help("xcbfdjd")
        .handler([&]() {
            ++verbosity;
        }));
    parser.add(
        Option("--name", "-n")
        .help("dfsd\nhjjkll")
        .handler([&](string_view value) {
            name = value;
        }));
    parser.add(
        Positional("bob")
        .occurs(Quantifier(0, 25))
        .help("hohahaha")
        .handler([](unsigned idx, string_view) {
            CHECK(idx == 0);
        })
    );
    parser.add(
        Positional("fob")
        .occurs(Quantifier(1,1))
        .help("ghakl\njdks")
        .handler([](unsigned, string_view) {
        
        })
    );
    
    parser.addValidator(OptionRequired("hah") && OptionRequired("heh"));

    parser.addValidator([](const auto &) { return true; }, "hello");

    std::cout << '\n' <<  HelpFormatter(parser, "ggg").formatHelp();

}
