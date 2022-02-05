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

    parser.add(OptionNames("-v"), [&]() {
        ++verbosity;
    });
    parser.add(OptionNames("--name", "-n"), [&](string_view value) {
        name = value;
    });
    
    parser.addValidator(OptionRequired("hah") && OptionRequired("heh"));

    parser.addValidator([](const auto &) { return true; }, "hello");

    std::cout << '\n';
    parser.printUsage(std::cout, "ggg");

    const char * argv[] = { "prog", "-v" };
    try {
        parser.parse(int(std::size(argv)), argv);
    } catch (ParsingException & ex) {
        ex.print(std::cout);
    }
    CHECK(verbosity == 2);
    CHECK(name == "world");
}
