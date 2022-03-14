#include "parser-common.h"

TEST_CASE( "Positional boundary Cases" , "[parser]") {

    map<string, vector<Value>> results;

    {
        Parser parser;
        parser.add(Positional("p")); //positional with no handler
        
        EXPECT_SUCCESS(ARGS("x"), RESULTS())
    }

    {
        Parser parser;
        parser.add(POSITIONAL("p")); 
        CHECK_THROWS_AS(parser.add(POSITIONAL("p")), invalid_argument);
    }

    {
        Parser parser;
        parser.add(POSITIONAL("p")); 
        CHECK_THROWS_AS(parser.add(POSITIONAL("p").occurs(Quantifier(6,0))), invalid_argument);
    }

}

TEST_CASE( "Simple positional" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    EXPECT_FAILURE(ARGS("a", "b"), EXTRA_POSITIONAL("b"))

    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("--=foo"), RESULTS({"foo", {"--=foo"}}))
    EXPECT_SUCCESS(ARGS("-=foo"), RESULTS({"foo", {"-=foo"}}))
}

TEST_CASE( "Positional with explicit repeat once" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo").occurs(oneTime));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    EXPECT_FAILURE(ARGS("a", "b"), EXTRA_POSITIONAL("b"))

    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
}

TEST_CASE( "Positional with explicit repeat twice" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo").occurs(Quantifier(2)));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))

    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a", "b"}}))
}

TEST_CASE( "Positional with unlimited repeat" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo").occurs(zeroOrMoreTimes));

    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a", "b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b", "c"}}))
}

TEST_CASE( "Positional with one or more repeat" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo").occurs(oneOrMoreTimes));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a", "b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b", "c"}}))
}

TEST_CASE( "Positional with zero or once repeat" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo").occurs(zeroOrOneTime));

    EXPECT_FAILURE(ARGS("a", "b"), EXTRA_POSITIONAL("b"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
}

TEST_CASE( "Two positionals" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
}

TEST_CASE( "Positional with no explict repeat followed by one with 1" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(once));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
}

TEST_CASE( "Positional with repeat 2 followed by one with 1" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo").occurs(2));
    parser.add(POSITIONAL("bar"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("a", "b"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("a", "b", "c", "d"), EXTRA_POSITIONAL("d"))
    
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b"}}, {"bar", {"c"}}))
}

TEST_CASE( "Positional with repeat 1 followed by one with unlimited" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(zeroOrMoreTimes));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}))
}

TEST_CASE( "Positional with repeat 1 followed by one with one or more" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(oneOrMoreTimes));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}))
}

TEST_CASE( "Positional with repeat 1 followed by one with an optional" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(neverOrOnce));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
}

TEST_CASE( "Positional with unlimited repeat followed by one with 1" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo").occurs(zeroOrMoreTimes));
    parser.add(POSITIONAL("bar"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"bar", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b"}}, {"bar", {"c"}}))
}

TEST_CASE( "Positional with one or more repeat followed by one with 1" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo").occurs(oneOrMoreTimes));
    parser.add(POSITIONAL("bar"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b"}}, {"bar", {"c"}}))
}
TEST_CASE( "Positional with an optional repeat followed by one with 1" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo").occurs(neverOrOnce));
    parser.add(POSITIONAL("bar"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"bar", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
}

TEST_CASE( "Positional with repeat 2 followed by one with unlimited" , "[parser]") {
    map<wstring, vector<WValue>> results;

    WParser parser;
    parser.add(WPOSITIONAL(L"foo").occurs(2));
    parser.add(WPOSITIONAL(L"bar").occurs(zeroOrMoreTimes));

    EXPECT_FAILURE(WARGS(), WVALIDATION_ERROR(L"invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(WARGS(L"--foo"), WUNRECOGNIZED_OPTION(L"--foo"))
    EXPECT_FAILURE(WARGS(L"a"), WVALIDATION_ERROR(L"invalid arguments: positional argument foo must occur at least 2 times"))
    
    EXPECT_SUCCESS(WARGS(L"a", L"b"), WRESULTS({L"foo", {L"a", L"b"}}))
    EXPECT_SUCCESS(WARGS(L"a", L"b", L"c"), WRESULTS({L"foo", {L"a", L"b"}}, {L"bar", {L"c"}}))
}

TEST_CASE( "Positional with repeat 2 followed by one with one or more" , "[parser]") {
    map<wstring, vector<WValue>> results;

    WParser parser;
    parser.add(WPOSITIONAL(L"foo").occurs(2));
    parser.add(WPOSITIONAL(L"bar").occurs(oneOrMoreTimes));

    EXPECT_FAILURE(WARGS(), WVALIDATION_ERROR(L"invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(WARGS(L"--foo"), WUNRECOGNIZED_OPTION(L"--foo"))
    EXPECT_FAILURE(WARGS(L"a"), WVALIDATION_ERROR(L"invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(WARGS(L"a", L"b"), WVALIDATION_ERROR(L"invalid arguments: positional argument bar must be present"))
    
    EXPECT_SUCCESS(WARGS(L"a", L"b", L"c"), WRESULTS({L"foo", {L"a", L"b"}}, {L"bar", {L"c"}}))
    EXPECT_SUCCESS(WARGS(L"a", L"b", L"c", L"d"), WRESULTS({L"foo", {L"a", L"b"}}, {L"bar", {L"c", L"d"}}))
}

TEST_CASE( "Positional with repeat 2 followed by one with optional" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo").occurs(2));
    parser.add(POSITIONAL("bar").occurs(neverOrOnce));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("a", "b", "c", "d"), EXTRA_POSITIONAL("d"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a", "b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b"}}, {"bar", {"c"}}))
}

TEST_CASE( "Three positionals with repeats: 1, *, 1" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(zeroOrMoreTimes));
    parser.add(POSITIONAL("baz"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument baz must be present"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"baz", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}, {"baz", {"c"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c", "d"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}, {"baz", {"d"}}))
}

TEST_CASE( "Three positionals with repeats: 1, +, 1" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(oneOrMoreTimes));
    parser.add(POSITIONAL("baz"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("a", "b"), VALIDATION_ERROR("invalid arguments: positional argument baz must be present"))
    
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}, {"baz", {"c"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c", "d"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}, {"baz", {"d"}}))
}

TEST_CASE( "Three positionals with repeats: 1, ?, 1" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(neverOrOnce));
    parser.add(POSITIONAL("baz"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument baz must be present"))
    EXPECT_FAILURE(ARGS("a", "b", "c", "d"), EXTRA_POSITIONAL("d"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"baz", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}, {"baz", {"c"}}))
}

TEST_CASE( "Two positionals with repeats: ?, ?" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo").occurs(neverOrOnce));
    parser.add(POSITIONAL("bar").occurs(neverOrOnce));

    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
}

TEST_CASE( "Two positionals with repeats: ?, *" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo").occurs(neverOrOnce));
    parser.add(POSITIONAL("bar").occurs(zeroOrMoreTimes));

    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}))
}

TEST_CASE( "Two positionals with repeats: ?, +" , "[parser]") {
    map<string, vector<Value>> results;

    Parser parser;
    parser.add(POSITIONAL("foo").occurs(neverOrOnce));
    parser.add(POSITIONAL("bar").occurs(onceOrMore));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"bar", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}))
}


