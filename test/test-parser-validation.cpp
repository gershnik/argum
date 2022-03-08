#include "parser-common.h"

TEST_CASE( "Mutually exclusive groups" , "[parser]") {

    map<string, vector<Value>> results;

    Parser parser;
    parser.add(OPTION_NO_ARG("-a1"));
    parser.add(OPTION_NO_ARG("-a2"));
    parser.add(OPTION_NO_ARG("-a3"));
    parser.add(OPTION_NO_ARG("-b1"));
    parser.add(OPTION_NO_ARG("-b2"));
    parser.add(OPTION_NO_ARG("-b3"));
    parser.addValidator(
        oneOrNoneOf(
            anyOf(optionPresent("-a1"), optionPresent("-a2"), optionPresent("-a3")),
            anyOf(optionPresent("-b1"), optionPresent("-b2"), optionPresent("-b3"))
        ), 
        "option groups (-a1, -a2, -a2) and (-b1, -b2, -b3) are mutually exclusive"
    );

    EXPECT_FAILURE(ARGS("-a1", "-b2"), VALIDATION_ERROR("invalid arguments: option groups (-a1, -a2, -a2) and (-b1, -b2, -b3) are mutually exclusive"));
    EXPECT_FAILURE(ARGS("-a3", "-b1"), VALIDATION_ERROR("invalid arguments: option groups (-a1, -a2, -a2) and (-b1, -b2, -b3) are mutually exclusive"));

    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-a1"), RESULTS({"-a1", {"+"}}))
    EXPECT_SUCCESS(ARGS("-a2"), RESULTS({"-a2", {"+"}}))
    EXPECT_SUCCESS(ARGS("-a3"), RESULTS({"-a3", {"+"}}))
    EXPECT_SUCCESS(ARGS("-a1", "-a2"), RESULTS({"-a1", {"+"}}, {"-a2", {"+"}}))
    EXPECT_SUCCESS(ARGS("-a1", "-a2", "-a3"), RESULTS({"-a1", {"+"}}, {"-a2", {"+"}}, {"-a3", {"+"}}))
    EXPECT_SUCCESS(ARGS("-b1"), RESULTS({"-b1", {"+"}}))
    EXPECT_SUCCESS(ARGS("-b2"), RESULTS({"-b2", {"+"}}))
    EXPECT_SUCCESS(ARGS("-b3"), RESULTS({"-b3", {"+"}}))
    EXPECT_SUCCESS(ARGS("-b1", "-b2"), RESULTS({"-b1", {"+"}}, {"-b2", {"+"}}))
    EXPECT_SUCCESS(ARGS("-b1", "-b2", "-b3"), RESULTS({"-b1", {"+"}}, {"-b2", {"+"}}, {"-b3", {"+"}}))
}


TEST_CASE( "If one then other" , "[parser]") {

    map<string, vector<Value>> results;

    Parser parser;
    parser.add(OPTION_NO_ARG("-a1"));
    parser.add(OPTION_NO_ARG("-a2"));
    parser.add(OPTION_NO_ARG("-b1"));
    parser.add(OPTION_NO_ARG("-b2"));
    parser.addValidator(
        !optionPresent("-a1") || optionPresent("-b1"), 
        "if -a1 is specified then -b1 must be specified also"
    );

    EXPECT_FAILURE(ARGS("-a1"), VALIDATION_ERROR("invalid arguments: if -a1 is specified then -b1 must be specified also"));
    EXPECT_FAILURE(ARGS("-a1", "-b2"), VALIDATION_ERROR("invalid arguments: if -a1 is specified then -b1 must be specified also"));
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("-a2"), RESULTS({"-a2", {"+"}}))
    EXPECT_SUCCESS(ARGS("-b1"), RESULTS({"-b1", {"+"}}))
    EXPECT_SUCCESS(ARGS("-b2"), RESULTS({"-b2", {"+"}}))
    EXPECT_SUCCESS(ARGS("-b1", "-b2"), RESULTS({"-b1", {"+"}}, {"-b2", {"+"}}))
    EXPECT_SUCCESS(ARGS("-a2", "-b1", "-b2"), RESULTS({"-a2", {"+"}}, {"-b1", {"+"}}, {"-b2", {"+"}}))
}

