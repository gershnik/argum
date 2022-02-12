#ifndef HEADER_MARGP_MESSAGES_H_INCLUDED
#define HEADER_MARGP_MESSAGES_H_INCLUDED

#include "common.h"


namespace MArgP {

    template<Character Char> struct Messages;

    #define MARGP_DEFINE_MESSAGES(type, pr) template<> struct Messages<type> { \
        static constexpr auto unrecognizedOptionError()     { return pr ## "unrecognized option: {1}"; }\
        static constexpr auto missingOptionArgumentError()  { return pr ## "argument required for option: {1}"; }\
        static constexpr auto extraOptionArgumentError()    { return pr ## "extraneous argument for option: {1}"; }\
        static constexpr auto extraPositionalError()        { return pr ## "unexpected argument: {1}"; }\
        static constexpr auto validationError()             { return pr ## "invalid arguments: {1}"; }\
        static constexpr auto negationDesc()                { return pr ## "NOT {2}"; }\
        static constexpr auto allMustBeTrue()               { return pr ## "all of the following must be true:"; }\
        static constexpr auto oneOrMoreMustBeTrue()         { return pr ## "one or more of the following must be true:"; }\
        static constexpr auto onlyOneMustBeTrue()           { return pr ## "only one of the following must be true:"; }\
        static constexpr auto allOrNoneMustBeTrue()         { return pr ## "either all or none of the following must be true:"; }\
        static constexpr auto option()                      { return pr ## "option"; }\
        static constexpr auto positionalArg()               { return pr ## "positional argument"; }\
        static constexpr auto itemUnrestricted()            { return pr ## "{2} {3} can occur any number of times"; }\
        static constexpr auto itemRequired()                { return pr ## "{2} {3} must be present"; }\
        static constexpr auto itemMustNotBePresent()        { return pr ## "{2} {3} must not be present"; }\
        static constexpr auto itemOccursAtLeast()           { return pr ## "{2} {3} must occur at least {4} times"; }\
        static constexpr auto itemOccursAtMost()            { return pr ## "{2} {3} must occur at most {4} times"; }\
        static constexpr auto itemOccursLessThan()          { return pr ## "{2} {3} must occur less than {4} times"; }\
        static constexpr auto itemOccursMoreThan()          { return pr ## "{2} {3} must occur more than {4} times"; }\
        static constexpr auto itemOccursExactly()           { return pr ## "{2} {3} must occur exactly {4} times"; }\
        static constexpr auto itemDoesNotOccursExactly()    { return pr ## "{2} {3} must NOT occur {4} times"; }\
        static constexpr auto usageStart()                  { return pr ## "Usage: "; }\
        static constexpr auto defaultArgName()              { return pr ## "ARG"; }\
    };

    MARGP_DEFINE_MESSAGES(char, )
    MARGP_DEFINE_MESSAGES(wchar_t, L)
#if MARGP_UTF_CHAR_SUPPORTED
    MARGP_DEFINE_MESSAGES(char8_t, u8)
    MARGP_DEFINE_MESSAGES(char16_t, u)
    MARGP_DEFINE_MESSAGES(char32_t, U)
#endif

    #undef MARGP_DEFINE_MESSAGES

}


#endif
