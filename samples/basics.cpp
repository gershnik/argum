#include <argum.h>

#include <iostream>

using namespace Argum;

int main(int argc, char * argv[]) {

    Parser parser; 

    try {
        parser.parse(argc, argv);
    } catch (ParsingException & ex) {
        std::cerr << ex.message() << '\n';
        std::cerr << parser.formatUsage(argc ? argv[0] : "prog") << '\n';
        return EXIT_FAILURE;
    }
}
