#ifndef HEADER_MARGP_MESSAGES_H_INCLUDED
#define HEADER_MARGP_MESSAGES_H_INCLUDED


namespace MArgP {

    template<class Char> struct Messages;

    #define MARGP_DEFINE_MESSAGES(type, pr) template<> struct Messages<type> { \
        static constexpr auto unrecognizedOptionError()     { return pr ## "unrecognized option: "; }\
        static constexpr auto missingOptionArgumentError()  { return pr ## "argument required for option: "; }\
        static constexpr auto extraOptionArgumentError()    { return pr ## "extraneous argument for option: "; }\
        static constexpr auto extraPositionalError()        { return pr ## "unexpected argument: "; }\
        static constexpr auto validationError()             { return pr ## "invalid arguments: "; }\
        static constexpr auto negationDesc()                { return pr ## "NOT {1}"; }\
        static constexpr auto allMustBeTrue()               { return pr ## "all of the following must be true:"; }\
        static constexpr auto oneOrMoreMustBeTrue()         { return pr ## "one or more of the following must be true:"; }\
        static constexpr auto onlyOneMustBeTrue()           { return pr ## "only one of the following must be true:"; }\
        static constexpr auto allOrNoneMustBeTrue()         { return pr ## "either all or none of the following must be true:"; }\
        static constexpr auto optionRequired()              { return pr ## "option {1} is required"; }\
        static constexpr auto optionMustNotBePresent()      { return pr ## "option {1} must not be present"; }\
    };

    MARGP_DEFINE_MESSAGES(char, )
    MARGP_DEFINE_MESSAGES(wchar_t, L)
    MARGP_DEFINE_MESSAGES(char8_t, u8)
    MARGP_DEFINE_MESSAGES(char16_t, u)
    MARGP_DEFINE_MESSAGES(char32_t, U)

    #undef MARGP_DEFINE_MESSAGES

}


#endif
