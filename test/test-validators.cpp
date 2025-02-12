#include "test-common.h"

#include <argum/validators.h>

#include <doctest/doctest.h>

using namespace Argum;
using namespace std;

using namespace std::literals;

struct TrueValidator {

    auto operator()(const ValidationData &) const -> bool {
        return true;
    }

    operator const char *() { return "true"; }
    operator const wchar_t *() { return L"true"; }

    template<class Char>
    friend auto describe(TrueValidator t) -> basic_string<Char> {
        return (const Char *)t;
    }
};

static constexpr TrueValidator True;

struct FalseValidator {

    auto operator()(const ValidationData &) const -> bool {
        return false;
    }

    operator const char *() { return "false"; }
    operator const wchar_t *() { return L"false"; }

    template<class Char>
    friend auto describe(FalseValidator f) -> basic_string<Char> {
        return (const Char *)f;
    }
};

static constexpr FalseValidator False;

auto operator!(TrueValidator) { return False; }
auto operator!(FalseValidator) { return True; }

TEST_SUITE("validators") {

TEST_CASE( "Validators: optionPresent and optionAbsent" ) {

    ValidationData data;

    auto present = optionPresent("hah");
    CHECK(describe(present) == "option hah must be present");
    auto absent = optionAbsent("hah");
    CHECK(describe(absent) == "option hah must not be present");
    auto notPresent = !present;
    auto notAbsent = !absent;
    
    static_assert(is_same_v<remove_cvref_t<decltype(!notPresent)>, remove_cvref_t<decltype(present)>>);
    static_assert(is_same_v<remove_cvref_t<decltype(!notAbsent)>, remove_cvref_t<decltype(absent)>>);

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

TEST_CASE( "Validators: optionOccursAtLeast and optionOccursLessThan" ) {

    ValidationData data;

    auto atLeast = optionOccursAtLeast("hah", 2);
    CHECK(describe(atLeast) == "option hah must occur at least 2 times");
    auto lessThan = optionOccursLessThan("hah", 2);
    CHECK(describe(lessThan) == "option hah must occur less than 2 times");
    auto notAtLeast = !atLeast;
    auto notLestThan = !lessThan;
    
    static_assert(is_same_v<remove_cvref_t<decltype(notAtLeast)>, remove_cvref_t<decltype(lessThan)>>);
    static_assert(is_same_v<remove_cvref_t<decltype(notLestThan)>, remove_cvref_t<decltype(atLeast)>>);

    CHECK(!atLeast(data));
    CHECK(lessThan(data));

    data.optionCount("hah") = 1;
    CHECK(!atLeast(data));
    CHECK(lessThan(data));

    data.optionCount("hah") = 2;
    CHECK(atLeast(data));
    CHECK(!lessThan(data));
}

TEST_CASE( "Validators: optionOccursAtMost and optionOccursMoreThan" ) {

    ValidationData data;

    auto atMost = optionOccursAtMost("hah", 2);
    CHECK(describe(atMost) == "option hah must occur at most 2 times");
    auto moreThan = optionOccursMoreThan("hah", 2);
    CHECK(describe(moreThan) == "option hah must occur more than 2 times");
    auto notAtMost = !atMost;
    auto notMoreThan = !moreThan;
    
    static_assert(is_same_v<remove_cvref_t<decltype(notAtMost)>, remove_cvref_t<decltype(moreThan)>>);
    static_assert(is_same_v<remove_cvref_t<decltype(notMoreThan)>, remove_cvref_t<decltype(atMost)>>);

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

TEST_CASE( "Validators: optionOccursExactly and optionDoesntOccurExactly" ) {

    ValidationData data;

    auto exactly = optionOccursExactly("hah", 2);
    CHECK(describe(exactly) == "option hah must occur 2 times");
    auto opposite = optionDoesntOccurExactly("hah", 2);
    CHECK(describe(opposite) == "option hah must NOT occur 2 times");
    auto notExactly = !exactly;
    auto notOpposite = !opposite;
    
    static_assert(is_same_v<remove_cvref_t<decltype(notExactly)>, remove_cvref_t<decltype(opposite)>>);
    static_assert(is_same_v<remove_cvref_t<decltype(notOpposite)>, remove_cvref_t<decltype(exactly)>>);

    CHECK(!exactly(data));
    CHECK(opposite(data));

    data.optionCount("hah") = 1;
    CHECK(!exactly(data));
    CHECK(opposite(data));

    data.optionCount("hah") = 2;
    CHECK(exactly(data));
    CHECK(!opposite(data));
}

TEST_CASE( "Validators: oppositeOf" ) {
    ValidationData data;

    auto t = [](const ValidationData & ) { return true; };
    auto f = [](const ValidationData & ) { return false; };

    
#ifndef _MSC_VER
    //imbecilic MSVC applies built-in ! to lambdas producing bool
    CHECK(!(!t)(data));
    CHECK((!f)(data));
#endif

    CHECK(!oppositeOf(t)(data));
    CHECK(oppositeOf(f)(data));
}

TEST_CASE( "Validators: allOf" ) {
    ValidationData data;

    CHECK(!(False && False && False)(data));
    CHECK(!(False && True && False)(data));
    CHECK(!(False && False && True)(data));
    CHECK(!(False && True && True)(data));
    CHECK(!(True && False && False)(data));
    CHECK(!(True && True && False)(data));
    CHECK(!(True && False && True)(data));
    CHECK((True && True && True)(data));

    static_assert(is_same_v<remove_cvref_t<decltype(True && True && False)>, CombinedValidator<ValidatorCombination::And, TrueValidator, TrueValidator, FalseValidator>>);
    static_assert(is_same_v<remove_cvref_t<decltype(allOf(True, True, False))>, remove_cvref_t<decltype(True && True && False)>>);
    static_assert(is_same_v<decltype((True && False) && (True && False)), decltype(True && False && True && False)>);
    static_assert(is_same_v<decltype(True && (True && False)), decltype(True && True && False)>);
    static_assert(is_same_v<decltype((True && False) && True), decltype(True && False && True)>);
}

TEST_CASE( "Validators: anyOf" ) {
    ValidationData data;

    CHECK(!(False || False || False)(data));
    CHECK((False || True || False)(data));
    CHECK((False || False || True)(data));
    CHECK((False || True || True)(data));
    CHECK((True || False || False)(data));
    CHECK((True || True || False)(data));
    CHECK((True || False || True)(data));
    CHECK((True || True || True)(data));

    static_assert(is_same_v<remove_cvref_t<decltype(True || True || False)>, CombinedValidator<ValidatorCombination::Or, TrueValidator, TrueValidator, FalseValidator>>);
    static_assert(is_same_v<remove_cvref_t<decltype(anyOf(True, True, False))>, remove_cvref_t<decltype(True || True || False)>>);
    static_assert(is_same_v<decltype((True || False) || (True || False)), decltype(True || False || True || False)>);
    static_assert(is_same_v<decltype(True || (True || False)), decltype(True || True || False)>);
    static_assert(is_same_v<decltype((True || False) || True), decltype(True || False || True)>);
}

TEST_CASE( "Validators: noneOf" ) {
    ValidationData data;

    CHECK(noneOf(False, False, False)(data));
    CHECK(!noneOf(False, False, True)(data));
    CHECK(!noneOf(False, True, False)(data));
    CHECK(!noneOf(False, True, True)(data));
    CHECK(!noneOf(True, False, False)(data));
    CHECK(!noneOf(True, False, True)(data));
    CHECK(!noneOf(True, True, False)(data));
    CHECK(!noneOf(True, True, True)(data));
}

TEST_CASE( "Validators: onlyOneOf" ) {
    ValidationData data;

    CHECK(!onlyOneOf(False, False, False, False)(data));
    CHECK(onlyOneOf(False, False, False, True)(data));
    CHECK(onlyOneOf(False, False, True, False)(data));
    CHECK(!onlyOneOf(False, False, True, True)(data));
    CHECK(onlyOneOf(False, True, False, False)(data));
    CHECK(!onlyOneOf(False, True, False, True)(data));
    CHECK(!onlyOneOf(False, True, True, False)(data));
    CHECK(!onlyOneOf(False, True, True, True)(data));
    CHECK(onlyOneOf(True, False, False, False)(data));
    CHECK(!onlyOneOf(True, False, False, True)(data));
    CHECK(!onlyOneOf(True, False, True, False)(data));
    CHECK(!onlyOneOf(True, False, True, True)(data));
    CHECK(!onlyOneOf(True, True, False, False)(data));
    CHECK(!onlyOneOf(True, True, False, True)(data));
    CHECK(!onlyOneOf(True, True, True, False)(data));
    CHECK(!onlyOneOf(True, True, True, True)(data));
}

TEST_CASE( "Validators: oneOrNoneOf" ) {
    ValidationData data;

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


    auto val = oneOrNoneOf(anyOf(optionPresent("-a1"), optionPresent("-a2"), optionPresent("-a3")),
                           anyOf(optionPresent("-b1"), optionPresent("-b2"), optionPresent("-b3")));
    data.optionCount("-a1") = 1;
    data.optionCount("-b2") = 1;
    CHECK(!val(data));
}

TEST_CASE( "Validators: allOrNoneOf" ) {
    ValidationData data;

    CHECK(allOrNoneOf(False, False, False, False)(data));
    CHECK(!allOrNoneOf(False, False, False, True)(data));
    CHECK(!allOrNoneOf(False, False, True, False)(data));
    CHECK(!allOrNoneOf(False, False, True, True)(data));
    CHECK(!allOrNoneOf(False, True, False, False)(data));
    CHECK(!allOrNoneOf(False, True, False, True)(data));
    CHECK(!allOrNoneOf(False, True, True, False)(data));
    CHECK(!allOrNoneOf(False, True, True, True)(data));
    CHECK(!allOrNoneOf(True, False, False, False)(data));
    CHECK(!allOrNoneOf(True, False, False, True)(data));
    CHECK(!allOrNoneOf(True, False, True, False)(data));
    CHECK(!allOrNoneOf(True, False, True, True)(data));
    CHECK(!allOrNoneOf(True, True, False, False)(data));
    CHECK(!allOrNoneOf(True, True, False, True)(data));
    CHECK(!allOrNoneOf(True, True, True, False)(data));
    CHECK(allOrNoneOf(True, True, True, True)(data));
}

}
