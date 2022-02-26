#include "parser-common.h"


TEST_CASE( "Simple positional" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    EXPECT_FAILURE(ARGS("a", "b"), EXTRA_POSITIONAL("b"))

    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
}

TEST_CASE( "Positional with explicit repeat once" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(OneTime));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    EXPECT_FAILURE(ARGS("a", "b"), EXTRA_POSITIONAL("b"))

    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
}

TEST_CASE( "Positional with explicit repeat twice" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(Quantifier(2)));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))

    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a", "b"}}))
}

TEST_CASE( "Positional with unlimited repeat" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(ZeroOrMoreTimes));

    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a", "b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b", "c"}}))
}

TEST_CASE( "Positional with one or more repeat" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(OneOrMoreTimes));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a", "b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b", "c"}}))
}

TEST_CASE( "Positional with zero or once repeat" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(ZeroOrOneTime));

    EXPECT_FAILURE(ARGS("a", "b"), EXTRA_POSITIONAL("b"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
}

TEST_CASE( "Two positionals" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
}

TEST_CASE( "Positional with no explict repeat followed by one with 1" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(Once));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("-x"), UNRECOGNIZED_OPTION("-x"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
}

TEST_CASE( "Positional with repeat 2 followed by one with 1" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(2));
    parser.add(POSITIONAL("bar"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("a", "b"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("a", "b", "c", "d"), EXTRA_POSITIONAL("d"))
    
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b"}}, {"bar", {"c"}}))
}

TEST_CASE( "Positional with repeat 1 followed by one with unlimited" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(ZeroOrMoreTimes));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}))
}

TEST_CASE( "Positional with repeat 1 followed by one with one or more" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(OneOrMoreTimes));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}))
}

TEST_CASE( "Positional with repeat 1 followed by one with an optional" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(NeverOrOnce));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
}

TEST_CASE( "Positional with unlimited repeat followed by one with 1" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(ZeroOrMoreTimes));
    parser.add(POSITIONAL("bar"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"bar", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b"}}, {"bar", {"c"}}))
}

TEST_CASE( "Positional with one or more repeat followed by one with 1" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(OneOrMoreTimes));
    parser.add(POSITIONAL("bar"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b"}}, {"bar", {"c"}}))
}
TEST_CASE( "Positional with an optional repeat followed by one with 1" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(NeverOrOnce));
    parser.add(POSITIONAL("bar"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"bar", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
}

TEST_CASE( "Positional with repeat 2 followed by one with unlimited" , "[adaptive]") {
    map<wstring, vector<WValue>> results;

    WAdaptiveParser parser;
    parser.add(WPOSITIONAL(L"foo").occurs(2));
    parser.add(WPOSITIONAL(L"bar").occurs(ZeroOrMoreTimes));

    EXPECT_FAILURE(WARGS(), WVALIDATION_ERROR(L"invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(WARGS(L"--foo"), WUNRECOGNIZED_OPTION(L"--foo"))
    EXPECT_FAILURE(WARGS(L"a"), WVALIDATION_ERROR(L"invalid arguments: positional argument foo must occur at least 2 times"))
    
    EXPECT_SUCCESS(WARGS(L"a", L"b"), WRESULTS({L"foo", {L"a", L"b"}}))
    EXPECT_SUCCESS(WARGS(L"a", L"b", L"c"), WRESULTS({L"foo", {L"a", L"b"}}, {L"bar", {L"c"}}))
}

TEST_CASE( "Positional with repeat 2 followed by one with one or more" , "[adaptive]") {
    map<wstring, vector<WValue>> results;

    WAdaptiveParser parser;
    parser.add(WPOSITIONAL(L"foo").occurs(2));
    parser.add(WPOSITIONAL(L"bar").occurs(OneOrMoreTimes));

    EXPECT_FAILURE(WARGS(), WVALIDATION_ERROR(L"invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(WARGS(L"--foo"), WUNRECOGNIZED_OPTION(L"--foo"))
    EXPECT_FAILURE(WARGS(L"a"), WVALIDATION_ERROR(L"invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(WARGS(L"a", L"b"), WVALIDATION_ERROR(L"invalid arguments: positional argument bar must be present"))
    
    EXPECT_SUCCESS(WARGS(L"a", L"b", L"c"), WRESULTS({L"foo", {L"a", L"b"}}, {L"bar", {L"c"}}))
    EXPECT_SUCCESS(WARGS(L"a", L"b", L"c", L"d"), WRESULTS({L"foo", {L"a", L"b"}}, {L"bar", {L"c", L"d"}}))
}

TEST_CASE( "Positional with repeat 2 followed by one with optional" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(2));
    parser.add(POSITIONAL("bar").occurs(NeverOrOnce));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument foo must occur at least 2 times"))
    EXPECT_FAILURE(ARGS("a", "b", "c", "d"), EXTRA_POSITIONAL("d"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a", "b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a", "b"}}, {"bar", {"c"}}))
}

TEST_CASE( "Three positionals with repeats: 1, *, 1" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(ZeroOrMoreTimes));
    parser.add(POSITIONAL("baz"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument baz must be present"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"baz", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}, {"baz", {"c"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c", "d"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}, {"baz", {"d"}}))
}

TEST_CASE( "Three positionals with repeats: 1, +, 1" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(OneOrMoreTimes));
    parser.add(POSITIONAL("baz"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("a", "b"), VALIDATION_ERROR("invalid arguments: positional argument baz must be present"))
    
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}, {"baz", {"c"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c", "d"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}, {"baz", {"d"}}))
}

TEST_CASE( "Three positionals with repeats: 1, ?, 1" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo"));
    parser.add(POSITIONAL("bar").occurs(NeverOrOnce));
    parser.add(POSITIONAL("baz"));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument foo must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a"), VALIDATION_ERROR("invalid arguments: positional argument baz must be present"))
    EXPECT_FAILURE(ARGS("a", "b", "c", "d"), EXTRA_POSITIONAL("d"))
    
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"baz", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}, {"baz", {"c"}}))
}

TEST_CASE( "Two positionals with repeats: ?, ?" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(NeverOrOnce));
    parser.add(POSITIONAL("bar").occurs(NeverOrOnce));

    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    EXPECT_FAILURE(ARGS("a", "b", "c"), EXTRA_POSITIONAL("c"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
}

TEST_CASE( "Two positionals with repeats: ?, *" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(NeverOrOnce));
    parser.add(POSITIONAL("bar").occurs(ZeroOrMoreTimes));

    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    
    EXPECT_SUCCESS(ARGS(), RESULTS())
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"foo", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}))
}

TEST_CASE( "Two positionals with repeats: ?, +" , "[adaptive]") {
    map<string, vector<Value>> results;

    AdaptiveParser parser;
    parser.add(POSITIONAL("foo").occurs(NeverOrOnce));
    parser.add(POSITIONAL("bar").occurs(OnceOrMore));

    EXPECT_FAILURE(ARGS(), VALIDATION_ERROR("invalid arguments: positional argument bar must be present"))
    EXPECT_FAILURE(ARGS("--foo"), UNRECOGNIZED_OPTION("--foo"))
    
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"bar", {"a"}}))
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"foo", {"a"}}, {"bar", {"b"}}))
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"foo", {"a"}}, {"bar", {"b", "c"}}))
}


