#include "parser-common.h"


TEST_CASE( "Mix of options and positionals" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(OPTION_NO_ARG("-n", "--no-arg"));
    parser.add(OPTION_OPT_ARG("-o", "--opt-arg"));
    parser.add(OPTION_REQ_ARG("-r", "--req-arg"));
    parser.add(POSITIONAL("p").occurs(zeroOrMoreTimes));
    parser.add(POSITIONAL("f"));

    EXPECT_FAILURE(ARGS("a", "-o", "-q", "c"), UNRECOGNIZED_OPTION("-q"))


    EXPECT_SUCCESS(ARGS("a"), RESULTS({"f", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "-n", "b", "c"), RESULTS({"-n", {"+"}}, {"p", {"a", "b"}}, {"f", {"c"}}))
    EXPECT_SUCCESS(ARGS("a", "-o", "b", "c"), RESULTS({"-o", {"b"}}, {"p", {"a"}}, {"f", {"c"}}))
    EXPECT_SUCCESS(ARGS("a", "-ob", "c"), RESULTS({"-o", {"b"}}, {"p", {"a"}}, {"f", {"c"}}))
    EXPECT_SUCCESS(ARGS("a", "-o", "-n", "c"), RESULTS({"-o", {nullopt}}, {"-n", {"+"}}, {"p", {"a"}}, {"f", {"c"}}))
    EXPECT_SUCCESS(ARGS("a", "-o", "--", "-n", "c"), RESULTS({"-o", {nullopt}}, {"p", {"a", "-n"}}, {"f", {"c"}}))
}

TEST_CASE( "More complicated partitioning" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(OPTION_NO_ARG("-n", "--no-arg"));
    parser.add(OPTION_OPT_ARG("-o", "--opt-arg"));
    parser.add(OPTION_REQ_ARG("-r", "--req-arg"));
    parser.add(POSITIONAL("p1").occurs(Quantifier(2, 3)));
    parser.add(POSITIONAL("p2").occurs(Quantifier(1, 2)));
    parser.add(POSITIONAL("p3").occurs(zeroOrMoreTimes));
    parser.add(POSITIONAL("p4").occurs(zeroOrMoreTimes));
    parser.add(POSITIONAL("f"));

    EXPECT_FAILURE(ARGS("a", "-o", "-q", "c"), UNRECOGNIZED_OPTION("-q"))
    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument p1 must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("a", "-r", "x", "-r", "x", "b", "-r", "x", "c"), VALIDATION_ERROR("invalid arguments: positional argument f must be present"))


    EXPECT_SUCCESS(ARGS("a", "b", "c", "d"), RESULTS({"p1", {"a", "b"}}, {"p2", {"c"}}, {"f", {"d"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c", "d", "e"), RESULTS({"p1", {"a", "b", "c"}}, {"p2", {"d"}}, {"f", {"e"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c", "d", "e", "f"), RESULTS({"p1", {"a", "b", "c"}}, {"p2", {"d", "e"}}, {"f", {"f"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c", "d", "e", "f", "g"), RESULTS({"p1", {"a", "b", "c"}}, {"p2", {"d", "e"}}, {"p3", {"f"}}, {"f", {"g"}}))
}

TEST_CASE( "Negative number args when numeric options are present" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("x").occurs(neverOrOnce));
    parser.add(OPTION_NO_ARG("-4"));

    EXPECT_SUCCESS(ARGS(), RESULTS())
    //DIFFERENCE FROM ArgParse. These are failure there
    EXPECT_SUCCESS(ARGS("-2"), RESULTS({"x", {"-2"}}))
    EXPECT_SUCCESS(ARGS("-315"), RESULTS({"x", {"-315"}}))
    //DIFFERENCE FROM ArgParse.
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"x", {"a"}}))
    EXPECT_SUCCESS(ARGS("-4"), RESULTS({"-4", {"+"}}))
    EXPECT_SUCCESS(ARGS("-4", "-2"), RESULTS({"-4", {"+"}}, {"x", {"-2"}}))
}

TEST_CASE( "Empty arguments" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("x").occurs(neverOrOnce));
    parser.add(OPTION_REQ_ARG("-y", "--yyy"));

    EXPECT_FAILURE(ARGS("-y"), MISSING_OPTION_ARGUMENT("-y"))

    EXPECT_SUCCESS(ARGS(""), RESULTS({"x", {""}}))
    EXPECT_SUCCESS(ARGS("-y", ""), RESULTS({"-y", {""}}))
}

TEST_CASE( "Prefix character only arguments" , "[parser]") {
    
    map<string, vector<Value>> results;

    Parser parser(Parser::Settings::commonUnix().addShortPrefix("+"));
    
    parser.add(POSITIONAL("foo").occurs(neverOrOnce));
    parser.add(POSITIONAL("bar").occurs(neverOrOnce));
    parser.add(OPTION_NO_ARG("-+-"));

    EXPECT_FAILURE(ARGS("-y"), UNRECOGNIZED_OPTION("-y"))

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-"), RESULTS({"foo", {"-"}}))
    EXPECT_SUCCESS(ARGS("-", "+"), RESULTS({"foo", {"-"}}, {"bar", {"+"}}))
    EXPECT_SUCCESS(ARGS("-+-"), RESULTS({"-+-", {"+"}}))
}

TEST_CASE( "Option with optional arg followed by optional positional" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(OPTION_OPT_ARG("-x"));
    parser.add(POSITIONAL("y").occurs(zeroOrMoreTimes));

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-x"), RESULTS({"-x", {nullopt}}))
    EXPECT_SUCCESS(ARGS("-x", "a"), RESULTS({"-x", {"a"}}))
    EXPECT_SUCCESS(ARGS("-x", "a", "b"), RESULTS({"-x", {"a"}}, {"y", {"b"}}))
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"y", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "-x"), RESULTS({"-x", {nullopt}}, {"y", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "-x", "b"), RESULTS({"-x", {"b"}}, {"y", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "-x", "b", "c"), RESULTS({"-x", {"b"}}, {"y", {"a", "c"}}))
}

//DIFFERENCE FROM ArgParse. We do not have argparse.REMAINDER
TEST_CASE( "Stopping at unknown" , "[parser]") {
    map<string, vector<Value>> results;
    vector<string> remainder;

    Parser parser;
    parser.add(POSITIONAL("x"));
    parser.add(OPTION_REQ_ARG("-z"));

    EXPECT_SUCCESS_UNTIL_UNKNOWN(ARGS("X"), RESULTS({"x", {"X"}}), vector<string>{})
    EXPECT_SUCCESS_UNTIL_UNKNOWN(ARGS("-z", "Z", "X"), RESULTS({"x", {"X"}}, {"-z", {"Z"}}), vector<string>{})
    EXPECT_SUCCESS_UNTIL_UNKNOWN(ARGS("X", "A", "B", "-z", "Z"), RESULTS({"x", {"X"}}), (vector<string>{"A", "B", "-z", "Z"}))
    EXPECT_SUCCESS_UNTIL_UNKNOWN(ARGS("X", "Y", "--foo"), RESULTS({"x", {"X"}}), (vector<string>{"Y", "--foo"}))
    EXPECT_SUCCESS_UNTIL_UNKNOWN(ARGS("X", "--foo"), RESULTS({"x", {"X"}}), (vector<string>{"--foo"}))

}
//DIFFERENCE FROM ArgParse

TEST_CASE( "options that may or may not be arguments" , "[parser]") {
    map<string, vector<Value>> results;
    vector<string> remainder;

    Parser parser;
    parser.add(OPTION_REQ_ARG("-x"));
    parser.add(OPTION_REQ_ARG("-3"));
    parser.add(POSITIONAL("z").occurs(zeroOrMoreTimes));

    EXPECT_FAILURE(ARGS("-x"), MISSING_OPTION_ARGUMENT("-x"))
    EXPECT_FAILURE(ARGS("-y2.5"), UNRECOGNIZED_OPTION("-y2.5"))
    EXPECT_FAILURE(ARGS("-x", "-3"), MISSING_OPTION_ARGUMENT("-x"))
    EXPECT_FAILURE(ARGS("-x", "-3.5"), MISSING_OPTION_ARGUMENT("-x"))
    EXPECT_FAILURE(ARGS("-3", "-3.5"), MISSING_OPTION_ARGUMENT("-3"))

    //DIFFERENCE FROM ArgParse. We do parse numbers properly
    EXPECT_SUCCESS(ARGS("-x", "-2.5"), RESULTS({"-x", {"-2.5"}}))
    EXPECT_SUCCESS(ARGS("-x", "-2.5", "a"), RESULTS({"-x", {"-2.5"}}, {"z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-3", "-.5"), RESULTS({"-3", {"-.5"}}))
    EXPECT_SUCCESS(ARGS("a", "x", "-1"), RESULTS({"z", {"a", "x", "-1"}}))
    EXPECT_SUCCESS(ARGS("-x", "-1", "a"), RESULTS({"-x", {"-1"}}, {"z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-3", "-1", "a"), RESULTS({"-3", {"-1"}}, {"z", {"a"}}))
    //DIFFERENCE FROM ArgParse
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-x", "2.5"), RESULTS({"-x", {"2.5"}}))
    EXPECT_SUCCESS(ARGS("-3.5"), RESULTS({"-3", {".5"}}))
    EXPECT_SUCCESS(ARGS("-3-.5"), RESULTS({"-3", {"-.5"}}))
    EXPECT_SUCCESS(ARGS("a", "-3.5"), RESULTS({"-3", {".5"}}, {"z", {"a"}}))
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"z", {"a"}}))
    EXPECT_SUCCESS(ARGS("-3", "1", "a"), RESULTS({"-3", {"1"}}, {"z", {"a"}}))
}
