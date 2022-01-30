#include <modern-arg-parser/modern-arg-parser.h>

#include "catch.hpp"

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

    const char * argv[] = { "prog", "-v", "--name", "world", "-v", "-q" };
    parser.parse(std::size(argv), argv);
    CHECK(verbosity == 2);
    CHECK(name == "world");
}
