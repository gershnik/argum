#include "parser-common.h"


TEST_CASE( "Negative number args when numeric options are present" , "[parser]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("x").occurs(NeverOrOnce));
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

    AdaptiveParser parser;
    parser.add(POSITIONAL("x").occurs(NeverOrOnce));
    parser.add(OPTION_REQ_ARG("-y", "--yyy"));

    EXPECT_FAILURE(ARGS("-y"), MISSING_OPTION_ARGUMENT("-y"))

    EXPECT_SUCCESS(ARGS(""), RESULTS({"x", {""}}))
    EXPECT_SUCCESS(ARGS("-y", ""), RESULTS({"-y", {""}}))
}

TEST_CASE( "Prefix character only arguments" , "[parser]") {
    
    map<string, vector<Value>> results;

    {
        AdaptiveParser parser(AdaptiveParser::Settings::commonUnix().addShortPrefix("+"));
        //DIFFERENCE FROM ArgParse. These are legal there
        CHECK_THROWS_AS(parser.add(OPTION_NO_ARG("-")), invalid_argument);
        CHECK_THROWS_AS(parser.add(OPTION_NO_ARG("+")), invalid_argument);
        //DIFFERENCE FROM ArgParse
    }
    {

        AdaptiveParser parser(AdaptiveParser::Settings::commonUnix().addShortPrefix("+"));
        
        parser.add(POSITIONAL("foo").occurs(NeverOrOnce));
        parser.add(POSITIONAL("bar").occurs(NeverOrOnce));
        parser.add(OPTION_NO_ARG("-+-"));

        EXPECT_FAILURE(ARGS("-y"), UNRECOGNIZED_OPTION("-y"))

        EXPECT_SUCCESS(ARGS(), RESULTS())
        EXPECT_SUCCESS(ARGS("-"), RESULTS({"foo", {"-"}}))
        EXPECT_SUCCESS(ARGS("-", "+"), RESULTS({"foo", {"-"}}, {"bar", {"+"}}))
        EXPECT_SUCCESS(ARGS("-+-"), RESULTS({"-+-", {"+"}}))
    }
}

TEST_CASE( "Option with optional arg followed by optional positional" , "[parser]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(OPTION_OPT_ARG("-x"));
    parser.add(POSITIONAL("y").occurs(ZeroOrMoreTimes));

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

    AdaptiveParser parser;
    parser.add(POSITIONAL("x"));
    parser.add(OPTION_REQ_ARG("-z"));

    REQUIRE_NOTHROW(remainder = parseUntilUnknown(parser, ARGS("X"), results)); 
    CHECK(results == RESULTS({"x", {"X"}}));
    CHECK(remainder == vector<string>{});

    REQUIRE_NOTHROW(remainder = parseUntilUnknown(parser, ARGS("-z", "Z", "X"), results)); 
    CHECK(results == RESULTS({"x", {"X"}}, {"-z", {"Z"}}));
    CHECK(remainder == vector<string>{});

    REQUIRE_NOTHROW(remainder = parseUntilUnknown(parser, ARGS("X", "A", "B", "-z", "Z"), results)); 
    CHECK(results == RESULTS({"x", {"X"}}));
    CHECK(remainder == vector<string>{"A", "B", "-z", "Z"});

    REQUIRE_NOTHROW(remainder = parseUntilUnknown(parser, ARGS("X", "Y", "--foo"), results)); 
    CHECK(results == RESULTS({"x", {"X"}}));
    CHECK(remainder == vector<string>{"Y", "--foo"});

    REQUIRE_NOTHROW(remainder = parseUntilUnknown(parser, ARGS("X", "--foo"), results)); 
    CHECK(results == RESULTS({"x", {"X"}}));
    CHECK(remainder == vector<string>{"--foo"});

}
//DIFFERENCE FROM ArgParse
