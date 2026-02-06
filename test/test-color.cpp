#include "parser-common.h"

TEST_SUITE("color") {

TEST_CASE( "Default colors narrow" ) {

    map<string, vector<Value>> results;
    Parser parser;
    
    parser.add(OPTION_NO_ARG("-h", "--help").help("show this help message and exit"));
    parser.add(OPTION_NO_ARG("--version").help("show program's version number and exit"));
    parser.add(OPTION_NO_ARG("-x").help("X HELP"));
    parser.add(OPTION_REQ_ARG("--y").argName("Y").help("Y HELP"));
    parser.add(POSITIONAL("foo").help("FOO HELP"));
    parser.add(POSITIONAL("bar").help("BAR HELP"));

    CHECK(parser.formatUsage("PROG", defaultColorizer) == R"__([1;34mUsage: [0m[1;35mPROG[0m [[32m-h[0m] [[36m--version[0m] [[32m-x[0m] [[36m--y[0m [33mY[0m] [32mfoo[0m [32mbar[0m)__");

    CHECK(parser.formatHelp("PROG", defaultColorizer) == R"__([1;34mUsage: [0m[1;35mPROG[0m [[32m-h[0m] [[36m--version[0m] [[32m-x[0m] [[36m--y[0m [33mY[0m] [32mfoo[0m [32mbar[0m

[1;34mpositional arguments:[0m
  [1;32mfoo[0m         FOO HELP
  [1;32mbar[0m         BAR HELP

[1;34moptions:[0m
  [1;32m-h[0m, [1;36m--help[0m  show this help message and exit
  [1;36m--version[0m   show program's version number and exit
  [1;32m-x[0m          X HELP
  [1;36m--y[0m [1;33mY[0m       Y HELP

)__");
}

// TEST_CASE( "Default colors wide" ) {

//     map<wstring, vector<WValue>> results;
//     WParser parser;
    
//     parser.add(WOPTION_NO_ARG(L"-h", L"--help").help(L"show this help message and exit"));
//     parser.add(WOPTION_NO_ARG(L"-v", L"--version").help(L"show program's version number and exit"));
//     parser.add(WOPTION_NO_ARG(L"-x").help(L"X HELP"));
//     parser.add(WOPTION_REQ_ARG(L"--y").argName(L"Y").help(L"Y HELP"));
//     parser.add(WPOSITIONAL(L"foo").help(L"FOO HELP"));
//     parser.add(WPOSITIONAL(L"bar").help(L"BAR HELP"));

//     CHECK(parser.formatUsage(L"PROG", defaultWColorizer) == LR"__([1;34mUsage: [0m[1;35mPROG[0m [-h] [-v] [-x] [--y Y] foo bar)__");

// //     CHECK(parser.formatHelp("PROG") == R"__(Usage: PROG [-h] [-v] [-x] [--y Y] foo bar

// // positional arguments:
// //   foo            FOO HELP
// //   bar            BAR HELP

// // options:
// //   -h, --help     show this help message and exit
// //   -v, --version  show program's version number and exit
// //   -x             X HELP
// //   --y Y          Y HELP

// // )__");
// }

}