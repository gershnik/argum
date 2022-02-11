#include <modern-arg-parser/modern-arg-parser.h>

#include "catch.hpp"

#include <iostream>

using namespace MArgP;
using namespace std;

using namespace std::literals;


TEST_CASE( "xxx" , "[sequential]") {

    SequentialArgumentParser parser;
    using Option = SequentialArgumentParser::Option;
    using Positional = SequentialArgumentParser::Positional;

    int verbosity = 0;
    string name;

    parser.add(
        Option("-v")
        .setHandler([&]() {
            ++verbosity;
        }));
    parser.add(
        Option("--name", "-n")
        .setHandler([&](string_view value) {
            name = value;
        }));
    parser.add(
        Positional("bob")
        .set(Repeated(0, 25))
        .setHandler([](unsigned idx, string_view value) {
            CHECK(idx == 0);
        })
    );
    parser.add(
        Positional("fob")
        .set(Repeated(1,1))
        .setHandler([](unsigned idx, string_view value) {
        
        })
    );
    
    parser.addValidator(OptionRequired("hah") && OptionRequired("heh"));

    parser.addValidator([](const auto &) { return true; }, "hello");

    std::cout << '\n';
    parser.printUsage(std::cout, "ggg");

    const char * argv[] = { "-v", "hello" };
    try {
        parser.parse(std::begin(argv), std::end(argv));
    } catch (ParsingException & ex) {
        std::cout << ex.message();
    }
    CHECK(verbosity == 2);
    CHECK(name == "world");
}
