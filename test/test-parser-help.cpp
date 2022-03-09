#include "parser-common.h"

TEST_CASE( "Argument help aligns when options are longer" , "[parser]") {

    map<string, vector<Value>> results;
    Parser parser;
    
    parser.add(OPTION_NO_ARG("-h", "--help").help("show this help message and exit"));
    parser.add(OPTION_NO_ARG("-v", "--version").help("show program's version number and exit"));
    parser.add(OPTION_NO_ARG("-x").help("X HELP"));
    parser.add(OPTION_REQ_ARG("--y").argName("Y").help("Y HELP"));
    parser.add(POSITIONAL("foo").help("FOO HELP"));
    parser.add(POSITIONAL("bar").help("BAR HELP"));

    CHECK(parser.formatUsage("PROG") == R"__(Usage: PROG [-h] [-v] [-x] [--y Y] foo bar)__");
    CHECK(parser.formatHelp("PROG") == R"__(Usage: PROG [-h] [-v] [-x] [--y Y] foo bar

positional arguments:
  foo            FOO HELP
  bar            BAR HELP

options:
  -h, --help     show this help message and exit
  -v, --version  show program's version number and exit
  -x             X HELP
  --y Y          Y HELP

)__");
}

TEST_CASE( "extremely small number of columns" , "[parser]") {

    map<string, vector<Value>> results;
    Parser parser;

    parser.add(OPTION_NO_ARG("-h", "--help").help("show this help message and exit"));
    parser.add(OPTION_NO_ARG("-v", "--version").help("show program's version number and exit"));
    parser.add(OPTION_NO_ARG("-x").help("X HELP"));
    parser.add(OPTION_REQ_ARG("--y").argName("Y").help("Y HELP"));
    parser.add(POSITIONAL("foo").help("FOO HELP"));
    parser.add(POSITIONAL("bar").help("BAR HELP"));

    HelpFormatter formatter(parser, "PROG", {
        .width = 15,
        .helpLeadingGap = 1,
        .helpNameMaxWidth = 1,
        .helpDescriptionGap = 1
    });

    CHECK(formatter.formatUsage() == R"__(Usage: PROG
[-h] [-v] [-x]
[--y Y] foo bar)__");

    CHECK(formatter.formatHelp() == R"__(positional
arguments:
 foo
   FOO HELP
 bar
   BAR HELP

options:
 -h, --help
   show this
   help message
   and exit
 -v, --version
   show
   program's
   version
   number and
   exit
 -x
   X HELP
 --y Y
   Y HELP

)__");
}

TEST_CASE( "argument help aligns when options are longer" , "[parser]") {

    map<string, vector<Value>> results;
    Parser parser;

    parser.add(OPTION_NO_ARG("-v").help("xcbfdjd"));
    parser.add(OPTION_REQ_ARG("--name", "-n").help("name\nhelp"));
    parser.add(POSITIONAL("bob").occurs(Quantifier(0, 25)).help("hohahaha"));
    parser.add(POSITIONAL("fob").help("ghakl\njdks"));

    CHECK(parser.formatUsage("PROG") == R"__(Usage: PROG [-v] [--name ARG] [bob [bob bob bob bob bob bob bob bob bob bob bob
bob bob bob bob bob bob bob bob bob bob bob bob bob]] fob)__");
    CHECK(parser.formatHelp("PROG") == R"__(Usage: PROG [-v] [--name ARG] [bob [bob bob bob bob bob bob bob bob bob bob bob
bob bob bob bob bob bob bob bob bob bob bob bob bob]] fob

positional arguments:
  bob                 hohahaha
  fob                 ghakl
                      jdks

options:
  -v                  xcbfdjd
  --name ARG, -n ARG  name
                      help

)__");

}

TEST_CASE( "quantifiers of positionals are printed correctly" , "[parser]") {
    map<string, vector<Value>> results;
    {
      Parser parser;
      parser.add(POSITIONAL("fob").occurs(neverOrOnce));
      CHECK(parser.formatUsage("PROG") == "Usage: PROG [fob]");
    }
    {
      Parser parser;
      parser.add(POSITIONAL("fob").occurs(Quantifier{0, 2}));
      CHECK(parser.formatUsage("PROG") == "Usage: PROG [fob [fob]]");
    }
    {
      Parser parser;
      parser.add(POSITIONAL("fob").occurs(Quantifier{1, 2}));
      CHECK(parser.formatUsage("PROG") == "Usage: PROG fob [fob]");
    }
    {
      Parser parser;
      parser.add(POSITIONAL("fob").occurs(zeroOrMoreTimes));
      CHECK(parser.formatUsage("PROG") == "Usage: PROG [fob [fob ...]]");
    }
    {
      Parser parser;
      parser.add(POSITIONAL("fob").occurs(oneOrMoreTimes));
      CHECK(parser.formatUsage("PROG") == "Usage: PROG fob [fob ...]");
    }
    {
      Parser parser;
      parser.add(POSITIONAL("fob").occurs(Quantifier(2)));
      CHECK(parser.formatUsage("PROG") == "Usage: PROG fob fob");
    }
}

TEST_CASE( "quantifiers of options are printed correctly" , "[parser]") {
    map<string, vector<Value>> results;
    {
      Parser parser;
      parser.add(OPTION_NO_ARG("--fob").occurs(neverOrOnce));
      CHECK(parser.formatUsage("PROG") == "Usage: PROG [--fob]");
    }
    {
      Parser parser;
      parser.add(OPTION_NO_ARG("--fob").occurs(Quantifier{0, 2}));
      CHECK(parser.formatUsage("PROG") == "Usage: PROG [--fob [--fob]]");
    }
    {
      Parser parser;
      parser.add(OPTION_NO_ARG("--fob").occurs(Quantifier{1, 2}));
      CHECK(parser.formatUsage("PROG") == "Usage: PROG --fob [--fob]");
    }
    {
      Parser parser;
      parser.add(OPTION_NO_ARG("--fob").occurs(zeroOrMoreTimes));
      CHECK(parser.formatUsage("PROG") == "Usage: PROG [--fob]");
    }
    {
      Parser parser;
      parser.add(OPTION_NO_ARG("--fob").occurs(oneOrMoreTimes));
      CHECK(parser.formatUsage("PROG") == "Usage: PROG --fob");
    }
    {
      Parser parser;
      parser.add(OPTION_NO_ARG("--fob").occurs(Quantifier(2)));
      CHECK(parser.formatUsage("PROG") == "Usage: PROG --fob --fob");
    }
}

