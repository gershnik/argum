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

    #if !ARGUM_OVERRIDE_MESSAGES

    #define ARGUM_DEFINE_MESSAGES(type, pr) template<> struct Messages<type> { \
        static constexpr auto unrecognizedOptionError()     { return pr ## "unrecognized option: {1}"; }\
        static constexpr auto ambiguousOptionError()        { return pr ## "ambigous option: {1}, candidates: {2}"; }\
        static constexpr auto missingOptionArgumentError()  { return pr ## "argument required for option: {1}"; }\
        static constexpr auto extraOptionArgumentError()    { return pr ## "extraneous argument for option: {1}"; }\
        static constexpr auto extraPositionalError()        { return pr ## "unexpected argument: {1}"; }\
        static constexpr auto validationError()             { return pr ## "invalid arguments: {1}"; }\
        static constexpr auto errorReadingResponseFile()    { return pr ## "error reading response file \"{1}\": {2}"; }\
        static constexpr auto option()                      { return pr ## "option"; }\
        static constexpr auto positionalArg()               { return pr ## "positional argument"; }\
        static constexpr auto usageStart()                  { return pr ## "Usage: "; }\
        static constexpr auto defaultArgName()              { return pr ## "ARG"; }\
        static constexpr auto positionalHeader()            { return pr ## "positional arguments:"; }\
        static constexpr auto optionsHeader()               { return pr ## "options:"; }\
        static constexpr auto listJoiner()                  { return pr ## ", "; }\
        static constexpr auto itemUnrestricted()            { return pr ## "{1} {2} can occur any number of times"; }\
        static constexpr auto itemMustBePresent()           { return pr ## "{1} {2} must be present"; }\
        static constexpr auto itemMustNotBePresent()        { return pr ## "{1} {2} must not be present"; }\
        static constexpr auto itemMustBePresentGE(unsigned val) { \
            switch(val) { \
                case 0:   return itemUnrestricted(); \
                case 1:   return itemMustBePresent(); \
                default:  return pr ## "{1} {2} must occur at least {3} times"; \
            } \
        }\
        static constexpr auto itemMustBePresentG(unsigned val) { \
            switch(val) { \
                case 0:   return itemMustBePresent(); \
                default:  return pr ## "{1} {2} must occur more than {3} times"; \
            }\
        }\
        static constexpr auto itemMustBePresentLE(unsigned val) { \
            constexpr auto inf = std::numeric_limits<unsigned>::max();\
            switch(val) { \
                case 0:   return itemMustNotBePresent(); \
                case 1:   return pr ## "{1} {2} must occur at most once"; \
                default:  return pr ## "{1} {2} must occur at most {3} times"; \
                case inf: return itemUnrestricted(); \
            }\
        }\
        static constexpr auto itemMustBePresentL(unsigned val) { \
            constexpr auto inf = std::numeric_limits<unsigned>::max();\
            switch(val) { \
                case 0:   return itemMustNotBePresent();\
                case 1:   return itemMustNotBePresent();\
                default:  return pr ## "{1} {2} must occur less than {3} times";\
                case inf: return itemUnrestricted();\
            }\
        }\
        static constexpr auto itemMustBePresentEQ(unsigned val) { \
            switch(val) { \
                case 0:   return itemMustNotBePresent();\
                case 1:   return itemMustBePresent();\
                default:  return pr ## "{1} {2} must occur {3} times";\
            }\
        }\
        static constexpr auto itemMustBePresentNEQ(unsigned val) { \
            constexpr auto inf = std::numeric_limits<unsigned>::max();\
            switch(val) { \
                case 0:   return itemMustBePresent();\
                default: return pr ## "{1} {2} must NOT occur {3} times"; \
                case inf: return itemUnrestricted();\
            }\
        }\
    };

    ARGUM_DEFINE_MESSAGES(char, )
    ARGUM_DEFINE_MESSAGES(wchar_t, L)

    #undef ARGUM_DEFINE_MESSAGES

    #endif

}


#endif
