#include <modern-arg-parser/modern-arg-parser.h>

#include "catch.hpp"

#include <iostream>

using namespace MArgP;
using namespace std;

using namespace std::literals;


TEST_CASE( "xxx" , "[sequential]") {

    SequentialArgumentParser parser;

    int verbosity = 0;
    string name;

    parser.add(OptionName("-v"), [&]() {
        ++verbosity;
    });
    parser.add(OptionName("--name", "-n"), [&](string_view value) {
        name = value;
    });
    
    const auto gug = OptionRequired("hah") && OptionRequired("heh");
    const auto geg = gug && OptionRequired("heh");
    const auto grg = OptionRequired("hah") && gug;
    const auto gag = !(geg || grg);

    describe(0, gag, std::cout);

    parser.addValidator(gug);

    parser.addValidator([](const auto &) { return true; }, "hello");

    const char * argv[] = { "prog", "-v" };
    try {
        parser.parse(std::size(argv), argv);
    } catch (ParsingException & ex) {
        ex.print(std::cout);
    }
    CHECK(verbosity == 2);
    CHECK(name == "world");
}
