//for testing let it throw exception rather than crash
[[noreturn]] void reportInvalidArgument(const char * message);
#define ARGUM_INVALID_ARGUMENT(message) reportInvalidArgument(message)

#include <argum/validators.h>

#include "catch.hpp"

using namespace Argum;
using namespace std;

using namespace std::literals;

struct TrueValidator {

    auto operator()(const ParsingValidationData<char> &) const -> bool {
        return true;
    }

    operator const char *() { return "true"; }
    operator const wchar_t *() { return L"true"; }

    template<class Char>
    friend auto describe(TrueValidator t) -> std::basic_string<Char> {
        return (const Char *)t;
    }
};

static constexpr TrueValidator True;

struct FalseValidator {

    auto operator()(const ParsingValidationData<char> &) const -> bool {
        return false;
    }

    operator const char *() { return "false"; }
    operator const wchar_t *() { return L"false"; }

    template<class Char>
    friend auto describe(FalseValidator f) -> std::basic_string<Char> {
        return (const Char *)f;
    }
};

static constexpr FalseValidator False;

auto operator!(TrueValidator) { return False; }
auto operator!(FalseValidator) { return True; }


TEST_CASE( "Validators: OptionPresent and OptionAbsent" , "[validators]") {

    ParsingValidationData<char> data;

    auto present = OptionPresent("hah");
    CHECK(describe(present) == "option hah must be present");
    auto absent = OptionAbsent("hah");
    CHECK(describe(absent) == "option hah must not be present");
    auto notPresent = !present;
    auto notAbsent = !absent;
    
    STATIC_REQUIRE(std::is_same_v<std::decay_t<decltype(!notPresent)>, std::decay_t<decltype(present)>>);
    STATIC_REQUIRE(std::is_same_v<std::decay_t<decltype(!notAbsent)>, std::decay_t<decltype(absent)>>);

    CHECK(!present(data));
    CHECK(notPresent(data));
    CHECK(absent(data));
    CHECK(!notAbsent(data));
    

    data.optionCount("hah") = 0;

    CHECK(!present(data));
    CHECK(notPresent(data));
    CHECK(absent(data));
    CHECK(!notAbsent(data));

    data.optionCount("hah") = 1;   

    CHECK(present(data));
    CHECK(!notPresent(data));
    CHECK(!absent(data));
    CHECK(notAbsent(data));

    data.optionCount("hah") = 100;   

    CHECK(present(data));
    CHECK(!notPresent(data));
    CHECK(!absent(data));
    CHECK(notAbsent(data));
}

TEST_CASE( "Validators: OptionOccursAtLeast and OptionOccursLessThan" , "[validators]") {

    ParsingValidationData<char> data;

    auto atLeast = OptionOccursAtLeast("hah", 2);
    CHECK(describe(atLeast) == "option hah must occur at least 2 times");
    auto lessThan = OptionOccursLessThan("hah", 2);
    CHECK(describe(lessThan) == "option hah must occur less than 2 times");
    auto notAtLeast = !atLeast;
    auto notLestThan = !lessThan;
    
    STATIC_REQUIRE(std::is_same_v<std::decay_t<decltype(notAtLeast)>, std::decay_t<decltype(lessThan)>>);
    STATIC_REQUIRE(std::is_same_v<std::decay_t<decltype(notLestThan)>, std::decay_t<decltype(atLeast)>>);

    CHECK(!atLeast(data));
    CHECK(lessThan(data));

    data.optionCount("hah") = 1;
    CHECK(!atLeast(data));
    CHECK(lessThan(data));

    data.optionCount("hah") = 2;
    CHECK(atLeast(data));
    CHECK(!lessThan(data));
}

TEST_CASE( "Validators: OptionOccursAtMost and OptionOccursMoreThan" , "[validators]") {

    ParsingValidationData<char> data;

    auto atMost = OptionOccursAtMost("hah", 2);
    CHECK(describe(atMost) == "option hah must occur at most 2 times");
    auto moreThan = OptionOccursMoreThan("hah", 2);
    CHECK(describe(moreThan) == "option hah must occur more than 2 times");
    auto notAtMost = !atMost;
    auto notMoreThan = !moreThan;
    
    STATIC_REQUIRE(std::is_same_v<std::decay_t<decltype(notAtMost)>, std::decay_t<decltype(moreThan)>>);
    STATIC_REQUIRE(std::is_same_v<std::decay_t<decltype(notMoreThan)>, std::decay_t<decltype(atMost)>>);

    CHECK(atMost(data));
    CHECK(!moreThan(data));

    data.optionCount("hah") = 1;
    CHECK(atMost(data));
    CHECK(!moreThan(data));

    data.optionCount("hah") = 2;
    CHECK(atMost(data));
    CHECK(!moreThan(data));

    data.optionCount("hah") = 3;
    CHECK(!atMost(data));
    CHECK(moreThan(data));
}

TEST_CASE( "Validators: oppositeOf" , "[validators]") {
    ParsingValidationData<char> data;

    auto t = [](const ParsingValidationData<char> & ) { return true; };
    auto f = [](const ParsingValidationData<char> & ) { return false; };

    CHECK(!(!t)(data));
    CHECK((!f)(data));
    CHECK(!oppositeOf(t)(data));
    CHECK(oppositeOf(f)(data));
   

    STATIC_REQUIRE(std::is_same_v<std::decay_t<decltype(!!t)>, std::decay_t<decltype(t)>>);
    STATIC_REQUIRE(std::is_same_v<std::decay_t<decltype(!!f)>, std::decay_t<decltype(f)>>);
}

TEST_CASE( "Validators: allOf" , "[validators]") {
    ParsingValidationData<char> data;

    CHECK(!(False && False && False)(data));
    CHECK(!(False && True && False)(data));
    CHECK(!(False && False && True)(data));
    CHECK(!(False && True && True)(data));
    CHECK(!(True && False && False)(data));
    CHECK(!(True && True && False)(data));
    CHECK(!(True && False && True)(data));
    CHECK((True && True && True)(data));

    STATIC_REQUIRE(std::is_same_v<std::decay_t<decltype(True && True && False)>, CombinedValidator<ValidatorCombination::And, TrueValidator, TrueValidator, FalseValidator>>);
    STATIC_REQUIRE(std::is_same_v<std::decay_t<decltype(allOf(True, True, False))>, std::decay_t<decltype(True && True && False)>>);
}

TEST_CASE( "Validators: anyOf" , "[validators]") {
    ParsingValidationData<char> data;

    CHECK(!(False || False || False)(data));
    CHECK((False || True || False)(data));
    CHECK((False || False || True)(data));
    CHECK((False || True || True)(data));
    CHECK((True || False || False)(data));
    CHECK((True || True || False)(data));
    CHECK((True || False || True)(data));
    CHECK((True || True || True)(data));

    STATIC_REQUIRE(std::is_same_v<std::decay_t<decltype(True || True || False)>, CombinedValidator<ValidatorCombination::Or, TrueValidator, TrueValidator, FalseValidator>>);
    STATIC_REQUIRE(std::is_same_v<std::decay_t<decltype(anyOf(True, True, False))>, std::decay_t<decltype(True || True || False)>>);
}

TEST_CASE( "Validators: noneOf" , "[validators]") {
    ParsingValidationData<char> data;

    CHECK(noneOf(False, False, False)(data));
    CHECK(!noneOf(False, False, True)(data));
    CHECK(!noneOf(False, True, False)(data));
    CHECK(!noneOf(False, True, True)(data));
    CHECK(!noneOf(True, False, False)(data));
    CHECK(!noneOf(True, False, True)(data));
    CHECK(!noneOf(True, True, False)(data));
    CHECK(!noneOf(True, True, True)(data));

    STATIC_REQUIRE(std::is_same_v<std::decay_t<decltype(noneOf(True, True, False))>, 
                    CombinedValidator<ValidatorCombination::And, FalseValidator, FalseValidator, TrueValidator>>);
}

TEST_CASE( "Validators: onlyOneOf" , "[validators]") {
    ParsingValidationData<char> data;

    CHECK(!onlyOneOf(False, False, False)(data));
    CHECK(onlyOneOf(False, False, True)(data));
    CHECK(onlyOneOf(False, True, False)(data));
    CHECK(!onlyOneOf(False, True, True)(data));
    CHECK(onlyOneOf(True, False, False)(data));
    CHECK(!onlyOneOf(True, False, True)(data));
    CHECK(!onlyOneOf(True, True, False)(data));
    CHECK(!onlyOneOf(True, True, True)(data));

    STATIC_REQUIRE(std::is_same_v<std::decay_t<decltype(onlyOneOf(onlyOneOf(True, True), False))>, 
                   CombinedValidator<ValidatorCombination::Xor, TrueValidator, TrueValidator, FalseValidator>>);
}

TEST_CASE( "Validators: oneOrNoneOf" , "[validators]") {
    ParsingValidationData<char> data;

    CHECK(oneOrNoneOf(False, False)(data));
    CHECK(oneOrNoneOf(False, True)(data));
    CHECK(oneOrNoneOf(True, False)(data));
    CHECK(!oneOrNoneOf(True, True)(data));

    CHECK(oneOrNoneOf(False, False, False)(data));
    CHECK(oneOrNoneOf(False, False, True)(data));
    CHECK(oneOrNoneOf(False, True, False)(data));
    CHECK(!oneOrNoneOf(False, True, True)(data));
    CHECK(oneOrNoneOf(True, False, False)(data));
    CHECK(!oneOrNoneOf(True, False, True)(data));
    CHECK(!oneOrNoneOf(True, True, False)(data));
    CHECK(!oneOrNoneOf(True, True, True)(data));


    auto val = oneOrNoneOf(anyOf(OptionPresent("-a1"), OptionPresent("-a2"), OptionPresent("-a3")),
                           anyOf(OptionPresent("-b1"), OptionPresent("-b2"), OptionPresent("-b3")));
    data.optionCount("-a1") = 1;
    data.optionCount("-b2") = 1;
    CHECK(!val(data));
}

TEST_CASE( "Validators: allOrNoneOf" , "[validators]") {
    ParsingValidationData<char> data;

    CHECK(allOrNoneOf(False, False, False)(data));
    CHECK(!allOrNoneOf(False, False, True)(data));
    CHECK(!allOrNoneOf(False, True, False)(data));
    CHECK(!allOrNoneOf(False, True, True)(data));
    CHECK(!allOrNoneOf(True, False, False)(data));
    CHECK(!allOrNoneOf(True, False, True)(data));
    CHECK(!allOrNoneOf(True, True, False)(data));
    CHECK(allOrNoneOf(True, True, True)(data));
}
