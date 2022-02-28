#include "parser-common.h"

TEST_CASE( "Changing positionals on the fly" , "[parser]") {
    map<string, vector<Value>> results;
    
    AdaptiveParser parser;
    parser.add(Positional("p").occurs(ZeroOrMoreTimes).handler([&](unsigned idx, string_view val){
        if (idx == 1) {
            parser.add(POSITIONAL("f"));
        }
        auto & list = results["p"]; 
        CHECK(list.size() == idx); 
        list.push_back(string(val));
    }));
    AdaptiveParser savedParser = parser;


    EXPECT_FAILURE(ARGS("a", "b"), VALIDATION_ERROR("invalid arguments: positional argument f must be present"))
    
    parser = savedParser;
    EXPECT_SUCCESS(ARGS(), RESULTS())
    parser = savedParser;
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"p", {"a"}}))
    parser = savedParser;
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"p", {"a", "b"}}, {"f", {"c"}}))
}

TEST_CASE( "Changing options on the fly" , "[parser]") {
    map<string, vector<Value>> results;
    
    AdaptiveParser parser;
    parser.add(Positional("p").occurs(ZeroOrMoreTimes).handler([&](unsigned idx, string_view val){
        if (idx == 1) {
            parser.add(Option("-c").handler([&](string_view val){
                parser.add(POSITIONAL("f"));
                auto & list = results["-c"]; 
                list.push_back(string(val));
            }));
        }
        auto & list = results["p"]; 
        CHECK(list.size() == idx); 
        list.push_back(string(val));
    }));    
    AdaptiveParser savedParser = parser;

    
    EXPECT_FAILURE(ARGS("a", "-c", "1"), UNRECOGNIZED_OPTION("-c"))
    parser = savedParser;
    EXPECT_FAILURE(ARGS("a", "b", "-c", "d"), VALIDATION_ERROR("invalid arguments: positional argument f must be present"))

    parser = savedParser;
    EXPECT_SUCCESS(ARGS(), RESULTS())
    parser = savedParser;
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"p", {"a"}}))
    parser = savedParser;
    EXPECT_SUCCESS(ARGS("a", "b"), RESULTS({"p", {"a", "b"}}))
    parser = savedParser;
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"p", {"a", "b", "c"}}))
    parser = savedParser;
    EXPECT_SUCCESS(ARGS("a", "b", "-c", "d", "e"), RESULTS({"p", {"a", "b"}}, {"-c", {"d"}}, {"f", {"e"}}))
}
