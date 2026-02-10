#include <argum.h>

#include <iostream>

using namespace Argum;
using namespace std;

int main(int argc, char * argv[]) {

    const char * progname = (argc ? argv[0] : "prog");

    int optionValue = 0;
    std::string positionalArg;

    Parser parser;
    parser.add(
        Option("--help", "-h"). 
        help("show this help message and exit"). 
        handler([&]() {  
            cout << parser.formatHelp(progname);
            exit(EXIT_SUCCESS);
        }));
    parser.add(
        Option("--option", "-o").
        argName("VALUE").
        help("an option with a value"). 
        handler([&](const string_view & value) {
            optionValue = parseIntegral<int>(value);
        }));
    parser.add(
        Positional("positional").
        help("positional argument"). 
        handler([&](const string_view & value) { 
            positionalArg = value;
        }));

    try {
        parser.parse(argc, argv);
    } catch (ParsingException & ex) {
        cerr << ex.message() << '\n';
        cerr << parser.formatUsage(progname) << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "option value: " << optionValue << '\n';
    std::cout << "positional argument: " << positionalArg << '\n';
}
