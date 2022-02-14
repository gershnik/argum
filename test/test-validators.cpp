#include <modern-arg-parser/validators.h>

#include "catch.hpp"

using namespace MArgP;
using namespace std;

using namespace std::literals;


TEST_CASE( "Simple Validators" , "[validators]") {

    ParsingValidationData<char> data;

    auto required = OptionRequired("hah");
    CHECK(describe<char>(required) == "option hah must be present");
    auto absent = OptionAbsent("hah");
    CHECK(describe<char>(absent) == "option hah must not be present");
    auto notRequired = !required;
    CHECK(describe<char>(notRequired) == "option hah must not be present");
    auto notAbsent = !absent;
    CHECK(describe<char>(notAbsent) == "option hah must be present");

    CHECK(!required(data));
    CHECK((!required)(data));
    CHECK(absent(data));
    CHECK(!notAbsent(data));
    

    data.optionCount("hah") = 0;

    CHECK(!required(data));
    CHECK(notRequired(data));
    CHECK(absent(data));
    CHECK(!notAbsent(data));

    data.optionCount("hah") = 1;   

    CHECK(required(data));
    CHECK(!notRequired(data));
    CHECK(!absent(data));
    CHECK(notAbsent(data));

    data.optionCount("hah") = 100;   

    CHECK(required(data));
    CHECK(!notRequired(data));
    CHECK(!absent(data));
    CHECK(notAbsent(data));
}

TEST_CASE( "Validators: anyOf" , "[validators]") {
    ParsingValidationData<char> data;

    auto required1 = OptionRequired("hah");
    auto required2 = OptionRequired("heh");
    auto anyOfRequired1 = required1 || required2;
    auto anyOfRequired2 = anyOf(required1, required2);
    auto notAnyOfRequired = !anyOfRequired1;


    CHECK(describe<char>(anyOfRequired1) ==
R"(one or more of the following must be true:
    option hah must be present
    option heh must be present)");
    CHECK(describe<char>(anyOfRequired2) == describe<char>(anyOfRequired1));
    CHECK(describe<char>(notAnyOfRequired) ==
R"(all of the following must be true:
    option hah must not be present
    option heh must not be present)");

    CHECK(!anyOfRequired1(data));
    CHECK(!anyOfRequired2(data));
    CHECK(notAnyOfRequired(data));

    data.optionCount("hah") = 0;
    data.optionCount("heh") = 0;

    CHECK(!anyOfRequired1(data));
    CHECK(!anyOfRequired2(data));
    CHECK(notAnyOfRequired(data));
    
    data.optionCount("hah") = 1;

    CHECK(anyOfRequired1(data));
    CHECK(anyOfRequired2(data));
    CHECK(!notAnyOfRequired(data));

    data.optionCount("heh") = 1;

    CHECK(anyOfRequired1(data));
    CHECK(anyOfRequired2(data));
    CHECK(!notAnyOfRequired(data));

    data.optionCount("hah") = 0;

    CHECK(anyOfRequired1(data));
    CHECK(anyOfRequired2(data));
    CHECK(!notAnyOfRequired(data));
}
