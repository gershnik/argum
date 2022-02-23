//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_MESSAGES_H_INCLUDED
#define HEADER_ARGUM_MESSAGES_H_INCLUDED

#include "common.h"


namespace Argum {

    template<Character Char> struct Messages;

    #define ARGUM_DEFINE_MESSAGES(type, pr) template<> struct Messages<type> { \
        static constexpr auto unrecognizedOptionError()     { return pr ## "unrecognized option: {1}"; }\
        static constexpr auto ambiguousOptionError()        { return pr ## "ambigous option: {1}, candidates: {2}"; }\
        static constexpr auto missingOptionArgumentError()  { return pr ## "argument required for option: {1}"; }\
        static constexpr auto extraOptionArgumentError()    { return pr ## "extraneous argument for option: {1}"; }\
        static constexpr auto extraPositionalError()        { return pr ## "unexpected argument: {1}"; }\
        static constexpr auto validationError()             { return pr ## "invalid arguments: {1}"; }\
        static constexpr auto errorReadingResponseFile()    { return pr ## "error reading response file \"{1}\": {2}"; }\
        static constexpr auto negationDesc()                { return pr ## "NOT {1}"; }\
        static constexpr auto allMustBeTrue()               { return pr ## "all of the following must be true:"; }\
        static constexpr auto oneOrMoreMustBeTrue()         { return pr ## "one or more of the following must be true:"; }\
        static constexpr auto onlyOneMustBeTrue()           { return pr ## "only one of the following must be true:"; }\
        static constexpr auto allOrNoneMustBeTrue()         { return pr ## "either all or none of the following must be true:"; }\
        static constexpr auto option()                      { return pr ## "option"; }\
        static constexpr auto positionalArg()               { return pr ## "positional argument"; }\
        static constexpr auto itemUnrestricted()            { return pr ## "{1} {2} can occur any number of times"; }\
        static constexpr auto itemRequired()                { return pr ## "{1} {2} must be present"; }\
        static constexpr auto itemMustNotBePresent()        { return pr ## "{1} {2} must not be present"; }\
        static constexpr auto itemOccursAtLeast()           { return pr ## "{1} {2} must occur at least {3} times"; }\
        static constexpr auto itemOccursAtMost()            { return pr ## "{1} {2} must occur at most {3} times"; }\
        static constexpr auto itemOccursLessThan()          { return pr ## "{1} {2} must occur less than {3} times"; }\
        static constexpr auto itemOccursMoreThan()          { return pr ## "{1} {2} must occur more than {3} times"; }\
        static constexpr auto itemOccursExactly()           { return pr ## "{1} {2} must occur exactly {3} times"; }\
        static constexpr auto itemDoesNotOccursExactly()    { return pr ## "{1} {2} must NOT occur {3} times"; }\
        static constexpr auto usageStart()                  { return pr ## "Usage: "; }\
        static constexpr auto defaultArgName()              { return pr ## "ARG"; }\
        static constexpr auto positionalHeader()            { return pr ## "positional arguments:"; }\
        static constexpr auto optionsHeader()               { return pr ## "options:"; }\
        static constexpr auto listJoiner()                  { return pr ## ", "; }\
    };

    ARGUM_DEFINE_MESSAGES(char, )
    ARGUM_DEFINE_MESSAGES(wchar_t, L)

    #undef ARGUM_DEFINE_MESSAGES

}


#endif