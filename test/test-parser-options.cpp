#include "parser-common.h"

TEST_CASE( "Options boundary Cases" , "[parser]") {

    map<string, vector<Value>> results;
    {
        AdaptiveParser parser;
        parser.add(Option("-c")); //option with no handler
        
        EXPECT_SUCCESS(ARGS(), RESULTS())
        EXPECT_SUCCESS(ARGS("-c"), RESULTS())
    }

    {
        AdaptiveParser parser;
        CHECK_THROWS_AS(parser.add(OPTION_NO_ARG("a")), invalid_argument);
        CHECK_THROWS_AS(parser.add(OPTION_NO_ARG("+a")), invalid_argument);
        CHECK_THROWS_AS(parser.add(OPTION_NO_ARG("--")), invalid_argument);
        CHECK_THROWS_AS(parser.add(OPTION_NO_ARG("-c").occurs(Quantifier(3,2))), invalid_argument);
    }

    {
        AdaptiveParser parser(AdaptiveParser::Settings::commonUnix().addShortPrefix("+"));
        //DIFFERENCE FROM ArgParse. These are legal there
        CHECK_THROWS_AS(parser.add(OPTION_NO_ARG("-")), invalid_argument);
        CHECK_THROWS_AS(parser.add(OPTION_NO_ARG("+")), invalid_argument);
        //DIFFERENCE FROM ArgParse
    }

    {
        AdaptiveParser parser;
        parser.add(OPTION_NO_ARG("-a"));
        CHECK_THROWS_AS(parser.add(OPTION_NO_ARG("-a")), invalid_argument);
    }

}

TEST_CASE( "Option with a single-dash option string" , "[parser]") {

    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_REQ_ARG("-x"));

    EXPECT_FAILURE(ARGS("-x"), MISSING_OPTION_ARGUMENT("-x"))
    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a"))
    EXPECT_FAILURE(ARGS("--foo", "ff"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("-x", "--foo"), MISSING_OPTION_ARGUMENT( "-x"))
    EXPECT_FAILURE(ARGS("-x", "-y"), MISSING_OPTION_ARGUMENT("-x"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-x", "a"), RESULTS({"-x", {"a"}}))
    EXPECT_SUCCESS(ARGS("-xa"), RESULTS({"-x", {"a"}}))
    EXPECT_SUCCESS(ARGS("-x", "-1"), RESULTS({"-x", {"-1"}}))
    EXPECT_SUCCESS(ARGS("-x-1"), RESULTS({"-x", {"-1"}}))
}

TEST_CASE( "Wide Option with a single-dash option string" , "[parser]") {

    map<wstring, vector<WValue>> results;

    WAdaptiveParser parser;
    parser.add(WOPTION_REQ_ARG(L"-x"));

    EXPECT_FAILURE(WARGS(L"-x"), WMISSING_OPTION_ARGUMENT(L"-x"))
    EXPECT_FAILURE(WARGS(L"a"), WEXTRA_POSITIONAL(L"a"))
    EXPECT_FAILURE(WARGS(L"--foo", L"ff"), WUNRECOGNIZED_OPTION(L"--foo"))
    EXPECT_FAILURE(WARGS(L"-x", L"--foo"), WMISSING_OPTION_ARGUMENT(L"-x"))
    EXPECT_FAILURE(WARGS(L"-x", L"-y"), WMISSING_OPTION_ARGUMENT(L"-x"))

    EXPECT_SUCCESS(WARGS(), WRESULTS())
    EXPECT_SUCCESS(WARGS(L"-x", L"a"), WRESULTS({L"-x", {L"a"}}))
    EXPECT_SUCCESS(WARGS(L"-xa"), WRESULTS({L"-x", {L"a"}}))
    EXPECT_SUCCESS(WARGS(L"-x", L"-1"), WRESULTS({L"-x", {L"-1"}}))
    EXPECT_SUCCESS(WARGS(L"-x-1"), WRESULTS({L"-x", {L"-1"}}))
}

TEST_CASE( "Combined single-dash options" , "[parser]") {

    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_NO_ARG("-x"));
    parser.add(OPTION_NO_ARG("-yyy"));
    parser.add(OPTION_REQ_ARG("-z"));

    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a")) 
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo")) 
    EXPECT_FAILURE(ARGS("-xa"), EXTRA_OPTION_ARGUMENT("-x")) 
    EXPECT_FAILURE(ARGS("-xa", "a"), EXTRA_OPTION_ARGUMENT("-x"))
    EXPECT_FAILURE(ARGS("-x", "--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("-x", "-z"), MISSING_OPTION_ARGUMENT("-z"))
    EXPECT_FAILURE(ARGS("-z", "-x"), MISSING_OPTION_ARGUMENT("-z"))
    EXPECT_FAILURE(ARGS("-yx"), UNRECOGNIZED_OPTION("-yx"))
    EXPECT_FAILURE(ARGS("-yz", "a"), UNRECOGNIZED_OPTION("-yz"))
    EXPECT_FAILURE(ARGS("-yyyx"), UNRECOGNIZED_OPTION( "-yyyx"))
    EXPECT_FAILURE(ARGS("-yyy=x"), EXTRA_OPTION_ARGUMENT( "-yyy"))
    EXPECT_FAILURE(ARGS("-yyy=x", "a"), EXTRA_OPTION_ARGUMENT( "-yyy"))
    EXPECT_FAILURE(ARGS("-yyyza"), UNRECOGNIZED_OPTION( "-yyyza"))
    EXPECT_FAILURE(ARGS("-xyza"), EXTRA_OPTION_ARGUMENT("-x"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-x"), RESULTS({"-x", {"+"}}))
    EXPECT_SUCCESS(ARGS("-za"), RESULTS({"-z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-z=a"), RESULTS({"-z", {"=a"}}))
    EXPECT_SUCCESS(ARGS("-z", "a"), RESULTS({"-z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-xza"), RESULTS({"-x", {"+"}}, {"-z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-xz", "a"), RESULTS({"-x", {"+"}}, {"-z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-x", "-za"), RESULTS({"-x", {"+"}}, {"-z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-x", "-z", "a"), RESULTS({"-x", {"+"}}, {"-z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-y"), RESULTS({"-yyy", {"+"}}))
    EXPECT_SUCCESS(ARGS("-yyy"), RESULTS({"-yyy", {"+"}}))
    EXPECT_SUCCESS(ARGS("-x", "-yyy", "-za"), RESULTS({"-x", {"+"}}, {"-yyy", {"+"}}, {"-z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-x", "-yyy", "-z", "a"), RESULTS({"-x", {"+"}}, {"-yyy", {"+"}}, {"-z", {"a"}}))
}

TEST_CASE( "Option with a multi-character single-dash option string" , "[parser]") {

    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_REQ_ARG("-foo"));

    EXPECT_FAILURE(ARGS("-foo"), MISSING_OPTION_ARGUMENT("-foo"))
    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a")) 
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("-foo", "--foo"), MISSING_OPTION_ARGUMENT("-foo"))
    EXPECT_FAILURE(ARGS("-foo", "-y"), MISSING_OPTION_ARGUMENT("-foo"))
    EXPECT_FAILURE(ARGS("-fooa"), UNRECOGNIZED_OPTION("-fooa"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-foo", "a"), RESULTS({"-foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("-foo=a"), RESULTS({"-foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("-foo", "-1"), RESULTS({"-foo", {"-1"}}))
    EXPECT_SUCCESS(ARGS("-foo=-1"), RESULTS({"-foo", {"-1"}}))
    EXPECT_SUCCESS(ARGS("-fo", "a"), RESULTS({"-foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("-f", "a"), RESULTS({"-foo", {"a"}}))
    //DIFFERENCE FROM ArgParse. These are failure there
    EXPECT_SUCCESS(ARGS("-fo=a"), RESULTS({"-foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("-f=a"), RESULTS({"-foo", {"a"}}))
    //DIFFERENCE FROM ArgParse.
}

TEST_CASE( "Single dash options where option strings are subsets of each other" , "[parser]") {

    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_REQ_ARG("-f"));
    parser.add(OPTION_REQ_ARG("-foobar"));
    parser.add(OPTION_REQ_ARG("-foorab"));

    EXPECT_FAILURE(ARGS("-f"), MISSING_OPTION_ARGUMENT("-f"))
    EXPECT_FAILURE(ARGS("-fo"), AMBIGUOUS_OPTION("-fo", "-f", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-foo"), AMBIGUOUS_OPTION("-foo", "-f", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-foo", "b"), AMBIGUOUS_OPTION("-foo", "-f", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-foob"), AMBIGUOUS_OPTION("-foob", "-f", "-foobar"))
    EXPECT_FAILURE(ARGS("-fooba"), AMBIGUOUS_OPTION("-fooba", "-f", "-foobar"))
    EXPECT_FAILURE(ARGS("-foora"), AMBIGUOUS_OPTION("-foora", "-f", "-foorab"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-f", "a"), RESULTS({"-f", {"a"}}))
    EXPECT_SUCCESS(ARGS("-fa"), RESULTS({"-f", {"a"}}))
    EXPECT_SUCCESS(ARGS("-foa"), RESULTS({"-f", {"oa"}}))
    EXPECT_SUCCESS(ARGS("-fooa"), RESULTS({"-f", {"ooa"}}))
    EXPECT_SUCCESS(ARGS("-foobar", "a"), RESULTS({"-foobar", {"a"}}))
    EXPECT_SUCCESS(ARGS("-foorab", "a"), RESULTS({"-foorab", {"a"}}))
}

TEST_CASE( "Single dash options that partially match but are not subsets" , "[parser]") {

    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_REQ_ARG("-foobar"));
    parser.add(OPTION_REQ_ARG("-foorab"));

    EXPECT_FAILURE(ARGS("-f"), AMBIGUOUS_OPTION("-f", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-f", "a"), AMBIGUOUS_OPTION("-f", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-fa"), UNRECOGNIZED_OPTION("-fa"))
    EXPECT_FAILURE(ARGS("-foa"), UNRECOGNIZED_OPTION("-foa"))
    EXPECT_FAILURE(ARGS("-foo"), AMBIGUOUS_OPTION("-foo", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-fo"), AMBIGUOUS_OPTION("-fo", "-foobar", "-foorab"))
    EXPECT_FAILURE(ARGS("-foo", "b"), AMBIGUOUS_OPTION("-foo", "-foobar", "-foorab"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-foob", "a"), RESULTS({"-foobar", {"a"}}))
    EXPECT_SUCCESS(ARGS("-foor", "a"), RESULTS({"-foorab", {"a"}}))
    EXPECT_SUCCESS(ARGS("-fooba", "a"), RESULTS({"-foobar", {"a"}}))
    EXPECT_SUCCESS(ARGS("-foora", "a"), RESULTS({"-foorab", {"a"}}))
    EXPECT_SUCCESS(ARGS("-foobar", "a"), RESULTS({"-foobar", {"a"}}))
    EXPECT_SUCCESS(ARGS("-foorab", "a"), RESULTS({"-foorab", {"a"}}))
}

TEST_CASE( "Short option with a numeric option string" , "[parser]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_REQ_ARG("-1"));

    EXPECT_FAILURE(ARGS("-1"), MISSING_OPTION_ARGUMENT("-1"))
    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a"))
    EXPECT_FAILURE(ARGS("-1", "--foo"), MISSING_OPTION_ARGUMENT("-1"))
    EXPECT_FAILURE(ARGS("-1", "-y"), MISSING_OPTION_ARGUMENT("-1"))
    EXPECT_FAILURE(ARGS("-1", "-1"), MISSING_OPTION_ARGUMENT("-1"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-1", "a"), RESULTS({"-1", {"a"}}))
    EXPECT_SUCCESS(ARGS("-1a"), RESULTS({"-1", {"a"}}))
    //DIFFERENCE FROM ArgParse. This is failure there
    EXPECT_SUCCESS(ARGS("-1", "-2"), RESULTS({"-1", {"-2"}}))
    //DIFFERENCE FROM ArgParse
    EXPECT_SUCCESS(ARGS("-1-2"), RESULTS({"-1", {"-2"}}))
}

TEST_CASE( "Option with a double-dash option string" , "[parser]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_REQ_ARG("--foo"));

    EXPECT_FAILURE(ARGS("--foo"), MISSING_OPTION_ARGUMENT("--foo"))
    EXPECT_FAILURE(ARGS("-f"), UNRECOGNIZED_OPTION("-f"))
    EXPECT_FAILURE(ARGS("-f", "a"), UNRECOGNIZED_OPTION("-f"))
    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a"))
    EXPECT_FAILURE(ARGS("--foo", "-x"), MISSING_OPTION_ARGUMENT("--foo"))
    EXPECT_FAILURE(ARGS("--foo", "--bar"), MISSING_OPTION_ARGUMENT("--foo"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("--foo", "a"), RESULTS({"--foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("--foo=a"), RESULTS({"--foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("--foo", "-2.5"), RESULTS({"--foo", {"-2.5"}}))
    EXPECT_SUCCESS(ARGS("--foo=-2.5"), RESULTS({"--foo", {"-2.5"}}))
}

TEST_CASE( "Partial matching with a double-dash option string" , "[parser]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_NO_ARG("--badger"));
    parser.add(OPTION_REQ_ARG("--bat"));

    EXPECT_FAILURE(ARGS("--bar"), UNRECOGNIZED_OPTION("--bar"))
    EXPECT_FAILURE(ARGS("--b"), AMBIGUOUS_OPTION("--b", "--badger", "--bat"))
    EXPECT_FAILURE(ARGS("--ba"), AMBIGUOUS_OPTION("--ba", "--badger", "--bat"))
    EXPECT_FAILURE(ARGS("--b=2"), AMBIGUOUS_OPTION("--b", "--badger", "--bat"))
    EXPECT_FAILURE(ARGS("--ba=4"), AMBIGUOUS_OPTION("--ba", "--badger", "--bat"))
    EXPECT_FAILURE(ARGS("--badge", "5"), EXTRA_POSITIONAL("5"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("--bat", "X"), RESULTS({"--bat", {"X"}}))
    EXPECT_SUCCESS(ARGS("--bad"), RESULTS({"--badger", {"+"}}))
    EXPECT_SUCCESS(ARGS("--badg"), RESULTS({"--badger", {"+"}}))
    EXPECT_SUCCESS(ARGS("--badge"), RESULTS({"--badger", {"+"}}))
    EXPECT_SUCCESS(ARGS("--badger"), RESULTS({"--badger", {"+"}}))
}

TEST_CASE( "Wide Partial matching with a double-dash option string" , "[parser]") {
    map<wstring, vector<WValue>> results;

    WAdaptiveParser parser;
    parser.add(WOPTION_NO_ARG(L"--badger"));
    parser.add(WOPTION_REQ_ARG(L"--bat"));

    EXPECT_FAILURE(WARGS(L"--bar"), WUNRECOGNIZED_OPTION(L"--bar"))
    EXPECT_FAILURE(WARGS(L"--b"), WAMBIGUOUS_OPTION(L"--b", L"--badger", L"--bat"))
    EXPECT_FAILURE(WARGS(L"--ba"), WAMBIGUOUS_OPTION(L"--ba", L"--badger", L"--bat"))
    EXPECT_FAILURE(WARGS(L"--b=2"), WAMBIGUOUS_OPTION(L"--b", L"--badger", L"--bat"))
    EXPECT_FAILURE(WARGS(L"--ba=4"), WAMBIGUOUS_OPTION(L"--ba", L"--badger", L"--bat"))
    EXPECT_FAILURE(WARGS(L"--badge", L"5"), WEXTRA_POSITIONAL(L"5"))

    EXPECT_SUCCESS(WARGS(), WRESULTS())
    EXPECT_SUCCESS(WARGS(L"--bat", L"X"), WRESULTS({L"--bat", {L"X"}}))
    EXPECT_SUCCESS(WARGS(L"--bad"), WRESULTS({L"--badger", {L"+"}}))
    EXPECT_SUCCESS(WARGS(L"--badg"), WRESULTS({L"--badger", {L"+"}}))
    EXPECT_SUCCESS(WARGS(L"--badge"), WRESULTS({L"--badger", {L"+"}}))
    EXPECT_SUCCESS(WARGS(L"--badger"), WRESULTS({L"--badger", {L"+"}}))
}

TEST_CASE( "One double-dash option string is a prefix of another" , "[parser]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_NO_ARG("--badger"));
    parser.add(OPTION_REQ_ARG("--ba"));

    EXPECT_FAILURE(ARGS("--bar"), UNRECOGNIZED_OPTION("--bar"))
    EXPECT_FAILURE(ARGS("--b"), AMBIGUOUS_OPTION("--b", "--ba", "--badger"))
    EXPECT_FAILURE(ARGS("--ba"), MISSING_OPTION_ARGUMENT("--ba"))
    EXPECT_FAILURE(ARGS("--b=2"), AMBIGUOUS_OPTION("--b", "--ba", "--badger"))
    EXPECT_FAILURE(ARGS("--badge", "5"), EXTRA_POSITIONAL("5"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("--ba", "X"), RESULTS({"--ba", {"X"}}))
    EXPECT_SUCCESS(ARGS("--ba=X"), RESULTS({"--ba", {"X"}}))
    EXPECT_SUCCESS(ARGS("--bad"), RESULTS({"--badger", {"+"}}))
    EXPECT_SUCCESS(ARGS("--badg"), RESULTS({"--badger", {"+"}}))
    EXPECT_SUCCESS(ARGS("--badge"), RESULTS({"--badger", {"+"}}))
    EXPECT_SUCCESS(ARGS("--badger"), RESULTS({"--badger", {"+"}}))
}

TEST_CASE( "Mix of options with single- and double-dash option strings" , "[parser]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_NO_ARG("-f"));
    parser.add(OPTION_REQ_ARG("--bar"));
    parser.add(OPTION_NO_ARG("-baz"));

    EXPECT_FAILURE(ARGS("--bar"), MISSING_OPTION_ARGUMENT("--bar"))
    EXPECT_FAILURE(ARGS("-fbar"), EXTRA_OPTION_ARGUMENT("-f"))
    EXPECT_FAILURE(ARGS("-fbaz"), EXTRA_OPTION_ARGUMENT("-f"))
    EXPECT_FAILURE(ARGS("-bazf"), UNRECOGNIZED_OPTION("-bazf"))
    EXPECT_FAILURE(ARGS("-b", "B"), EXTRA_POSITIONAL("B"))
    EXPECT_FAILURE(ARGS("B"), EXTRA_POSITIONAL("B"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-f"), RESULTS({"-f", {"+"}}))
    EXPECT_SUCCESS(ARGS("--ba", "B"), RESULTS({"--bar", {"B"}}))
    EXPECT_SUCCESS(ARGS("-f", "--ba", "B"), RESULTS({"-f", {"+"}}, {"--bar", {"B"}}))
    EXPECT_SUCCESS(ARGS("-f", "-b"), RESULTS({"-f", {"+"}}, {"-baz", {"+"}}))
    EXPECT_SUCCESS(ARGS("-ba", "-f"), RESULTS({"-f", {"+"}}, {"-baz", {"+"}}))
}

TEST_CASE( "Combination of single- and double-dash option strings for an option" , "[parser]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_NO_ARG("-v", "--verbose", "-n", "--noisy"));

    EXPECT_FAILURE(ARGS("--x", "--verbose"), UNRECOGNIZED_OPTION("--x"))
    EXPECT_FAILURE(ARGS("-N"), UNRECOGNIZED_OPTION("-N"))
    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a"))
    EXPECT_FAILURE(ARGS("-v", "x"), EXTRA_POSITIONAL("x"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-v"), RESULTS({"-v", {"+"}}))
    EXPECT_SUCCESS(ARGS("--verbose"), RESULTS({"-v", {"+"}}))
    EXPECT_SUCCESS(ARGS("-n"), RESULTS({"-v", {"+"}}))
    EXPECT_SUCCESS(ARGS("--noisy"), RESULTS({"-v", {"+"}}))
}

TEST_CASE( "Allow long options to be abbreviated unambiguously" , "[parser]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_REQ_ARG("--foo"));
    parser.add(OPTION_REQ_ARG("--foobaz"));
    parser.add(OPTION_NO_ARG("--fooble"));


    EXPECT_FAILURE(ARGS("--foob", "5"), AMBIGUOUS_OPTION("--foob", "--foobaz", "--fooble"))
    EXPECT_FAILURE(ARGS("--foob"), AMBIGUOUS_OPTION("--foob", "--foobaz", "--fooble"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("--foo", "7"), RESULTS({"--foo", {"7"}}))
    EXPECT_SUCCESS(ARGS("--fooba", "a"), RESULTS({"--foobaz", {"a"}}))
    EXPECT_SUCCESS(ARGS("--foobl", "--foo", "g"), RESULTS({"--foo", {"g"}}, {"--fooble", {"+"}}))
}

TEST_CASE( "Disallow abbreviation setting" , "[parser]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser(AdaptiveParser::Settings::commonUnix().allowAbbreviation(false));
    parser.add(OPTION_REQ_ARG("--foo", "-foo"));
    parser.add(OPTION_NO_ARG("--foodle", "-foodle"));
    parser.add(OPTION_REQ_ARG("--foonly", "-foonly"));


    EXPECT_FAILURE(ARGS("-foon", "3"), UNRECOGNIZED_OPTION("-foon"))
    EXPECT_FAILURE(ARGS("--foon", "3"), UNRECOGNIZED_OPTION("--foon"))
    EXPECT_FAILURE(ARGS("--food"), UNRECOGNIZED_OPTION("--food"))
    EXPECT_FAILURE(ARGS("--food", "--foo", "2"), UNRECOGNIZED_OPTION("--food"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("--foo", "3"), RESULTS({"--foo", {"3"}}))
    EXPECT_SUCCESS(ARGS("-foo", "3"), RESULTS({"--foo", {"3"}}))
    EXPECT_SUCCESS(ARGS("--foonly", "7", "--foodle", "--foo", "2"), RESULTS({"--foo", {"2"}}, {"--foodle", {"+"}}, {"--foonly", {"7"}}))
    EXPECT_SUCCESS(ARGS("-foonly", "7", "-foodle", "-foo", "2"), RESULTS({"--foo", {"2"}}, {"--foodle", {"+"}}, {"--foonly", {"7"}}))
}

TEST_CASE( "Custom prefixes" , "[parser]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser(AdaptiveParser::Settings()
        .addLongPrefix("::")
        .addShortPrefix('+')
        .addShortPrefix('/')
        .addValueDelimiter('|')
        .addOptionStopSequence("^^")
    );
    parser.add(OPTION_NO_ARG("+f"));
    parser.add(OPTION_REQ_ARG("::bar"));
    parser.add(OPTION_NO_ARG("/baz"));

    EXPECT_FAILURE(ARGS("--bar"), EXTRA_POSITIONAL("--bar"))
    EXPECT_FAILURE(ARGS("-fbar"), EXTRA_POSITIONAL("-fbar"))
    EXPECT_FAILURE(ARGS("-b", "B"), EXTRA_POSITIONAL("-b"))
    EXPECT_FAILURE(ARGS("B"), EXTRA_POSITIONAL("B"))
    EXPECT_FAILURE(ARGS("-f"), EXTRA_POSITIONAL("-f"))
    EXPECT_FAILURE(ARGS("--bar", "B"), EXTRA_POSITIONAL("--bar"))
    EXPECT_FAILURE(ARGS("-baz"), EXTRA_POSITIONAL("-baz"))
    EXPECT_FAILURE(ARGS("::ba", "^^", "B"),MISSING_OPTION_ARGUMENT("::ba"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("+f"), RESULTS({"+f", {"+"}}))
    EXPECT_SUCCESS(ARGS("::ba", "B"), RESULTS({"::bar", {"B"}}))
    EXPECT_SUCCESS(ARGS("::ba|B"), RESULTS({"::bar", {"B"}}))
    EXPECT_SUCCESS(ARGS("+f", "::bar", "B"), RESULTS({"+f", {"+"}}, {"::bar", {"B"}}))
    EXPECT_SUCCESS(ARGS("+f", "/b"), RESULTS({"+f", {"+"}}, {"/baz", {"+"}}))
    EXPECT_SUCCESS(ARGS("/ba", "+f"), RESULTS({"+f", {"+"}}, {"/baz", {"+"}}))
}

TEST_CASE( "Equivalent custom prefixes" , "[parser]") {
    map<string, vector<Value>> results;

    AdaptiveParser::Settings settings;
    settings.addLongPrefix("::", ":")
        .addShortPrefix('+', '_')
        .addShortPrefix('/', '&')
        .addValueDelimiter('|')
        .addValueDelimiter('*')
        .addOptionStopSequence("^^", "%%");
    AdaptiveParser parser(std::move(settings));
    parser.add(OPTION_NO_ARG("+f"));
    parser.add(OPTION_REQ_ARG("::bar"));
    parser.add(OPTION_NO_ARG("/baz"));

    EXPECT_FAILURE(ARGS("--bar"), EXTRA_POSITIONAL("--bar"))
    EXPECT_FAILURE(ARGS("-fbar"), EXTRA_POSITIONAL("-fbar"))
    EXPECT_FAILURE(ARGS("-b", "B"), EXTRA_POSITIONAL("-b"))
    EXPECT_FAILURE(ARGS("B"), EXTRA_POSITIONAL("B"))
    EXPECT_FAILURE(ARGS("-f"), EXTRA_POSITIONAL("-f"))
    EXPECT_FAILURE(ARGS("--bar", "B"), EXTRA_POSITIONAL("--bar"))
    EXPECT_FAILURE(ARGS("-baz"), EXTRA_POSITIONAL("-baz"))
    EXPECT_FAILURE(ARGS("::ba", "^^", "B"),MISSING_OPTION_ARGUMENT("::ba"))
    EXPECT_FAILURE(ARGS("::ba", "%%", "B"),MISSING_OPTION_ARGUMENT("::ba"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("_f"), RESULTS({"+f", {"+"}}))
    EXPECT_SUCCESS(ARGS(":ba", "B"), RESULTS({"::bar", {"B"}}))
    EXPECT_SUCCESS(ARGS("::ba*A|B"), RESULTS({"::bar", {"A|B"}}))
    EXPECT_SUCCESS(ARGS("::ba|A*B"), RESULTS({"::bar", {"A*B"}}))
    EXPECT_SUCCESS(ARGS("_f", ":bar", "B"), RESULTS({"+f", {"+"}}, {"::bar", {"B"}}))
    EXPECT_SUCCESS(ARGS("_f", "&b"), RESULTS({"+f", {"+"}}, {"/baz", {"+"}}))
    EXPECT_SUCCESS(ARGS("&ba", "_f"), RESULTS({"+f", {"+"}}, {"/baz", {"+"}}))
}


TEST_CASE( "Optional arg for an option" , "[parser]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_OPT_ARG("-w", "--work"));

    EXPECT_FAILURE(ARGS("2"), EXTRA_POSITIONAL("2"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-w"), RESULTS({"-w", {nullopt}}))
    EXPECT_SUCCESS(ARGS("-w", "2"), RESULTS({"-w", {"2"}}))
    EXPECT_SUCCESS(ARGS("--work"), RESULTS({"-w", {nullopt}}))
    EXPECT_SUCCESS(ARGS("--work", "2"), RESULTS({"-w", {"2"}}))
    EXPECT_SUCCESS(ARGS("--work=2"), RESULTS({"-w", {"2"}}))
}

TEST_CASE( "Required option" , "[parser]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_OPT_ARG("-w", "--work").occurs(Once));

    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a"))
    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: option -w must be present"))

    EXPECT_SUCCESS(ARGS("-w"), RESULTS({"-w", {nullopt}}))
    EXPECT_SUCCESS(ARGS("-w", "42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("-w42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("--work"), RESULTS({"-w", {nullopt}}))
    EXPECT_SUCCESS(ARGS("--work", "42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("--work=42"), RESULTS({"-w", {"42"}}))
}

TEST_CASE( "Repeat one-or-more option" , "[parser]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_OPT_ARG("-w", "--work").occurs(OneOrMoreTimes));

    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a"))
    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: option -w must be present"))

    EXPECT_SUCCESS(ARGS("-w"), RESULTS({"-w", {nullopt}}))
    EXPECT_SUCCESS(ARGS("-w", "42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("-w42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("--work"), RESULTS({"-w", {nullopt}}))
    EXPECT_SUCCESS(ARGS("--work", "42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("--work=42"), RESULTS({"-w", {"42"}}))
    EXPECT_SUCCESS(ARGS("-ww", "42"), RESULTS({"-w", {nullopt, "42"}}))
    EXPECT_SUCCESS(ARGS("-www", "42"), RESULTS({"-w", {nullopt, nullopt, "42"}}))
    EXPECT_SUCCESS(ARGS("--work", "42", "-w"), RESULTS({"-w", {"42", nullopt}}))
}

TEST_CASE( "Repeat 2-or-3 option" , "[parser]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_OPT_ARG("-w", "--work").occurs(Quantifier(2,3)));

    EXPECT_FAILURE(ARGS("a"), EXTRA_POSITIONAL("a"))
    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: option -w must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("-w"), VALIDATION_ERROR("invalid arguments: option -w must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("-w", "-w", "--work", "--wo"), VALIDATION_ERROR("invalid arguments: option -w must occur at most 3 times"))

    EXPECT_SUCCESS(ARGS("-ww", "42"), RESULTS({"-w", {nullopt, "42"}}))
    EXPECT_SUCCESS(ARGS("-www", "42"), RESULTS({"-w", {nullopt, nullopt, "42"}}))
    EXPECT_SUCCESS(ARGS("--work=42", "-w", "-w34"), RESULTS({"-w", {"42", nullopt, "34"}}))
}



// TEST_CASE( "xxx" , "[sequential]") {

//     AdaptiveParser parser;
    

//     int verbosity = 0;
//     string name;

//     parser.add(
//         Option("-v")
//         .help("xcbfdjd")
//         .handler([&]() {
//             ++verbosity;
//         }));
//     parser.add(
//         Option("--name", "-n")
//         .help("dfsd\nhjjkll")
//         .handler([&](string_view value) {
//             name = value;
//         }));
//     parser.add(
//         Positional("bob")
//         .occurs(Quantifier(0, 25))
//         .help("hohahaha")
//         .handler([](unsigned idx, string_view) {
//             CHECK(idx == 0);
//         })
//     );
//     parser.add(
//         Positional("fob")
//         .occurs(Quantifier(1,1))
//         .help("ghakl\njdks")
//         .handler([](unsigned, string_view) {
        
//         })
//     );
    
//     parser.addValidator(OptionRequired("hah") && OptionRequired("heh"));

//     parser.addValidator([](const auto &) { return true; }, "hello");

//     std::cout << '\n' << parser.formatUsage("ggg");

//     const char * argv[] = { "-v", "hhh", "jjj" };
//     try {
//         parser.parse(std::begin(argv), std::end(argv));
//     } catch (ParsingException & ex) {
//         std::cout << ex.message();
//     }
//     //CHECK(verbosity == 2);
//     //CHECK(name == "world");

//    WAdaptiveParser wparser;
//     wparser.add(
//         WPositional(L"fob")
//         .occurs(Quantifier(7,7))
//         .help(L"ghakl\njdks")
//         .handler([](unsigned, wstring_view) {

//             })
//     );
//     const wchar_t * wargv[] = { L"-v", L"hhh", L"jjj" };
//     try {
//         wparser.parse(std::begin(wargv), std::end(wargv));
//     } catch (WParsingException & ex) {
//         std::wcout << ex.message();
//     }
// }
