#include "parser-common.h"

TEST_SUITE("parser") {

TEST_CASE( "Changing positionals on the fly" ) {
    map<string, vector<Value>> results;
    
    Parser parser;
    parser.add(Positional("p").occurs(zeroOrMoreTimes).handler([&results, &parser](string_view val){
        auto & list = results["p"]; 
        list.push_back(string(val));

        if (list.size() == 2) {
            parser.add(POSITIONAL("f"));
        }
    }));
    Parser savedParser = parser;


    EXPECT_FAILURE(ARGS("a", "b"), VALIDATION_ERROR("invalid arguments: positional argument f must be present"))
    
    parser = savedParser;
    EXPECT_SUCCESS(ARGS(), RESULTS())
    parser = savedParser;
    EXPECT_SUCCESS(ARGS("a"), RESULTS({"p", {"a"}}))
    parser = savedParser;
    EXPECT_SUCCESS(ARGS("a", "b", "c"), RESULTS({"p", {"a", "b"}}, {"f", {"c"}}))
}

TEST_CASE( "Changing options on the fly" ) {
    map<string, vector<Value>> results;
    
    Parser parser;
    parser.add(Positional("p").occurs(zeroOrMoreTimes).handler([&](string_view val){
        auto & list = results["p"]; 
        list.push_back(string(val));

        if (list.size() == 2) {
            parser.add(Option("-c").handler([&](string_view val){
                auto & list = results["-c"]; 
                list.push_back(string(val));
                
                parser.add(POSITIONAL("f"));
            }));
        }
    }));    
    Parser savedParser = parser;

    
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

}

