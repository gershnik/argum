#include <modern-arg-parser/validators.h>

#include "catch.hpp"

using namespace MArgP;
using namespace std;

using namespace std::literals;

template<class Char, DescribableParserValidator<Char> Validator>
auto printValidator(const Validator & validator) {
    return (std::basic_ostringstream<Char>() << describe(Indent<Char>{0}, validator)).str();
}

TEST_CASE( "Simple Validators" , "[validators]") {

    ParsingValidationData<char> data;

    auto required = OptionRequired("hah");
    CHECK(printValidator<char>(required) == "option hah is required");
    auto absent = OptionAbsent("hah");
    CHECK(printValidator<char>(absent) == "option hah must not be present");
    auto notRequired = !required;
    CHECK(printValidator<char>(notRequired) == "option hah must not be present");
    auto notAbsent = !absent;
    CHECK(printValidator<char>(notAbsent) == "option hah is required");

    CHECK(!required(data));
    CHECK((!required)(data));
    CHECK(absent(data));
    CHECK(!notAbsent(data));
    

    data["hah"] = 0;

    CHECK(!required(data));
    CHECK(notRequired(data));
    CHECK(absent(data));
    CHECK(!notAbsent(data));

    data["hah"] = 1;   

    CHECK(required(data));
    CHECK(!notRequired(data));
    CHECK(!absent(data));
    CHECK(notAbsent(data));

    data["hah"] = 100;   

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


    CHECK(printValidator<char>(anyOfRequired1) ==
R"(one or more of the following must be true:
    option hah is required
    option heh is required)");
    CHECK(printValidator<char>(anyOfRequired2) == printValidator<char>(anyOfRequired1));
    CHECK(printValidator<char>(notAnyOfRequired) ==
R"(all of the following must be true:
    option hah must not be present
    option heh must not be present)");

    CHECK(!anyOfRequired1(data));
    CHECK(!anyOfRequired2(data));
    CHECK(notAnyOfRequired(data));

    data["hah"] = 0;
    data["heh"] = 0;

    CHECK(!anyOfRequired1(data));
    CHECK(!anyOfRequired2(data));
    CHECK(notAnyOfRequired(data));
    
    data["hah"] = 1;

    CHECK(anyOfRequired1(data));
    CHECK(anyOfRequired2(data));
    CHECK(!notAnyOfRequired(data));

    data["heh"] = 1;

    CHECK(anyOfRequired1(data));
    CHECK(anyOfRequired2(data));
    CHECK(!notAnyOfRequired(data));

    data["hah"] = 0;

    CHECK(anyOfRequired1(data));
    CHECK(anyOfRequired2(data));
    CHECK(!notAnyOfRequired(data));
}
