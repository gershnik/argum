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

    CHECK(parser.formatUsage("PROG", defaultColorizer()) == R"__([1;34mUsage: [0m[1;35mPROG[0m [[32m-h[0m] [[36m--version[0m] [[32m-x[0m] [[36m--y[0m [33mY[0m] [32mfoo[0m [32mbar[0m)__");

    CHECK(parser.formatHelp("PROG", defaultColorizer()) == R"__([1;34mUsage: [0m[1;35mPROG[0m [[32m-h[0m] [[36m--version[0m] [[32m-x[0m] [[36m--y[0m [33mY[0m] [32mfoo[0m [32mbar[0m

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

TEST_CASE( "Default colors wide" ) {

    map<wstring, vector<WValue>> results;
    WParser parser;
    
    parser.add(WOPTION_NO_ARG(L"-h", L"--help").help(L"show this help message and exit"));
    parser.add(WOPTION_NO_ARG(L"--version").help(L"show program's version number and exit"));
    parser.add(WOPTION_NO_ARG(L"-x").help(L"X HELP"));
    parser.add(WOPTION_REQ_ARG(L"--y").argName(L"Y").help(L"Y HELP"));
    parser.add(WPOSITIONAL(L"foo").help(L"FOO HELP"));
    parser.add(WPOSITIONAL(L"bar").help(L"BAR HELP"));

    CHECK(parser.formatUsage(L"PROG", defaultWColorizer()) == LR"__([1;34mUsage: [0m[1;35mPROG[0m [[32m-h[0m] [[36m--version[0m] [[32m-x[0m] [[36m--y[0m [33mY[0m] [32mfoo[0m [32mbar[0m)__");

    CHECK(parser.formatHelp(L"PROG", defaultWColorizer()) == LR"__([1;34mUsage: [0m[1;35mPROG[0m [[32m-h[0m] [[36m--version[0m] [[32m-x[0m] [[36m--y[0m [33mY[0m] [32mfoo[0m [32mbar[0m

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


TEST_CASE( "Custom color" ) {

    ColorScheme myScheme {
        .heading =              makeColor<Color::bold, Color::bright_blue>(),
        .progName =             makeColor<Color::bold, Color::bright_magenta>(),
        .shortOptionInUsage =   makeColor<Color::bright_green>(),
        .longOptionInUsage =    makeColor<Color::bright_cyan>(),
        .optionArgInUsage =     makeColor<Color::bright_yellow>(),
        .positionalInUsage =    makeColor<Color::bright_green>(),
        .shortOption =          defaultColorScheme().shortOption,
        .longOption =           makeColor<Color::bold, Color::bright_cyan>(),
        .optionArg =            makeColor<Color::bold, Color::bright_yellow>(),
        .positional =           makeColor<Color::bold, Color::bright_green>(),
        .error =                defaultColorScheme().error,
        .warning =              defaultColorScheme().warning,
    };

    map<string, vector<Value>> results;
    Parser parser;
    
    parser.add(OPTION_NO_ARG("-h", "--help").help("show this help message and exit"));
    parser.add(OPTION_NO_ARG("--version").help("show program's version number and exit"));
    parser.add(OPTION_NO_ARG("-x").help("X HELP"));
    parser.add(OPTION_REQ_ARG("--y").argName("Y").help("Y HELP"));
    parser.add(POSITIONAL("foo").help("FOO HELP"));
    parser.add(POSITIONAL("bar").help("BAR HELP"));

    CHECK(parser.formatUsage("PROG", Colorizer{myScheme}) == R"__([1;94mUsage: [0m[1;95mPROG[0m [[92m-h[0m] [[96m--version[0m] [[92m-x[0m] [[96m--y[0m [93mY[0m] [92mfoo[0m [92mbar[0m)__");

    CHECK(parser.formatHelp("PROG", Colorizer{myScheme}) == R"__([1;94mUsage: [0m[1;95mPROG[0m [[92m-h[0m] [[96m--version[0m] [[92m-x[0m] [[96m--y[0m [93mY[0m] [92mfoo[0m [92mbar[0m

[1;94mpositional arguments:[0m
  [1;92mfoo[0m         FOO HELP
  [1;92mbar[0m         BAR HELP

[1;94moptions:[0m
  [1;32m-h[0m, [1;96m--help[0m  show this help message and exit
  [1;96m--version[0m   show program's version number and exit
  [1;32m-x[0m          X HELP
  [1;96m--y[0m [1;93mY[0m       Y HELP

)__");

}

TEST_CASE( "Custom wide color" ) {

    WColorScheme myScheme {
        .heading =              makeWColor<Color::bold, Color::bright_blue>(),
        .progName =             makeWColor<Color::bold, Color::bright_magenta>(),
        .shortOptionInUsage =   makeWColor<Color::bright_green>(),
        .longOptionInUsage =    makeWColor<Color::bright_cyan>(),
        .optionArgInUsage =     makeWColor<Color::bright_yellow>(),
        .positionalInUsage =    makeWColor<Color::bright_green>(),
        .shortOption =          defaultWColorScheme().shortOption,
        .longOption =           makeWColor<Color::bold, Color::bright_cyan>(),
        .optionArg =            makeWColor<Color::bold, Color::bright_yellow>(),
        .positional =           makeWColor<Color::bold, Color::bright_green>(),
        .error =                defaultWColorScheme().error,
        .warning =              defaultWColorScheme().warning,
    };

    map<wstring, vector<WValue>> results;
    WParser parser;
    
    parser.add(WOPTION_NO_ARG(L"-h", L"--help").help(L"show this help message and exit"));
    parser.add(WOPTION_NO_ARG(L"--version").help(L"show program's version number and exit"));
    parser.add(WOPTION_NO_ARG(L"-x").help(L"X HELP"));
    parser.add(WOPTION_REQ_ARG(L"--y").argName(L"Y").help(L"Y HELP"));
    parser.add(WPOSITIONAL(L"foo").help(L"FOO HELP"));
    parser.add(WPOSITIONAL(L"bar").help(L"BAR HELP"));

    CHECK(parser.formatUsage(L"PROG", WColorizer{myScheme}) == LR"__([1;94mUsage: [0m[1;95mPROG[0m [[92m-h[0m] [[96m--version[0m] [[92m-x[0m] [[96m--y[0m [93mY[0m] [92mfoo[0m [92mbar[0m)__");

    CHECK(parser.formatHelp(L"PROG", WColorizer{myScheme}) == LR"__([1;94mUsage: [0m[1;95mPROG[0m [[92m-h[0m] [[96m--version[0m] [[92m-x[0m] [[96m--y[0m [93mY[0m] [92mfoo[0m [92mbar[0m

[1;94mpositional arguments:[0m
  [1;92mfoo[0m         FOO HELP
  [1;92mbar[0m         BAR HELP

[1;94moptions:[0m
  [1;32m-h[0m, [1;96m--help[0m  show this help message and exit
  [1;96m--version[0m   show program's version number and exit
  [1;32m-x[0m          X HELP
  [1;96m--y[0m [1;93mY[0m       Y HELP

)__");

}

}