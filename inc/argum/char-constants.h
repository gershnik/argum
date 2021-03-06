//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_CHAR_CONSTANTS_H_INCLUDED
#define HEADER_ARGUM_CHAR_CONSTANTS_H_INCLUDED

#include "common.h"

#include <algorithm>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>
#include <wctype.h>

namespace Argum {

    template<Character Char> struct CharConstants;

    #define ARGUM_DEFINE_CHAR_CONSTANTS(type, prefix) template<> struct CharConstants<type> { \
        static constexpr auto dash                          = prefix ## '-'; \
        static constexpr auto doubleDash                    = prefix ## "--"; \
        static constexpr auto assignment                    = prefix ## '='; \
        static constexpr auto slash                         = prefix ## '/'; \
        static constexpr auto colon                         = prefix ## ':'; \
        static constexpr auto braceOpen                     = prefix ## '{'; \
        static constexpr auto braceClose                    = prefix ## '}'; \
        static constexpr auto space                         = prefix ## ' '; \
        static constexpr auto squareBracketOpen             = prefix ## '['; \
        static constexpr auto squareBracketClose            = prefix ## ']'; \
        static constexpr auto pipe                          = prefix ## '|'; \
        static constexpr auto endl                          = prefix ## '\n'; \
        static constexpr auto ellipsis                      = prefix ## "..."; \
        static constexpr auto mustEscapeInRegex             = prefix ## R"([.^$|()\[\]{}*+?\\])"; \
        static constexpr auto regexEscapeReplacement        = prefix ## R"(\\&)"; \
        static constexpr auto falseNames                    = {prefix ## "0", prefix ## "false", prefix ## "off", prefix ## "no"}; \
        static constexpr auto trueNames                     = {prefix ## "1", prefix ## "true", prefix ## "on", prefix ## "yes"}; \
        \
        static auto isSpace(type c) -> bool; \
        static auto toLong(const type * str, type ** str_end, int base) -> long; \
        static auto toLongLong(const type * str, type ** str_end, int base) -> long long; \
        static auto toULong(const type * str, type ** str_end, int base) -> unsigned long; \
        static auto toULongLong(const type * str, type ** str_end, int base) -> unsigned long long; \
        static auto toFloat(const type * str, type ** str_end) -> float; \
        static auto toDouble(const type * str, type ** str_end) -> double; \
        static auto toLongDouble(const type * str, type ** str_end) -> long double; \
    };

    ARGUM_DEFINE_CHAR_CONSTANTS(char, )
    ARGUM_DEFINE_CHAR_CONSTANTS(wchar_t, L)

    #undef ARGUM_DEFINE_CHAR_CONSTANTS
    
    inline auto CharConstants<char>::isSpace(char c) -> bool { return isspace(c); }
    inline auto CharConstants<wchar_t>::isSpace(wchar_t c) -> bool { return iswspace(c); }

    inline auto CharConstants<char>::toLong(const char * str, char ** str_end, int base) -> long {
        return strtol(str, str_end, base);
    }
    inline auto CharConstants<wchar_t>::toLong(const wchar_t * str, wchar_t ** str_end, int base) -> long {
        return wcstol(str, str_end, base);
    }
    
    inline auto CharConstants<char>::toLongLong(const char * str, char ** str_end, int base) -> long long {
        return strtoll(str, str_end, base);
    }
    inline auto CharConstants<wchar_t>::toLongLong(const wchar_t * str, wchar_t ** str_end, int base) -> long long {
        return wcstoll(str, str_end, base);
    }

    inline auto CharConstants<char>::toULong(const char * str, char ** str_end, int base) -> unsigned long {
        return strtoul(str, str_end, base);
    }
    inline auto CharConstants<wchar_t>::toULong(const wchar_t * str, wchar_t ** str_end, int base) -> unsigned long {
        return wcstoul(str, str_end, base);
    }
    
    inline auto CharConstants<char>::toULongLong(const char * str, char ** str_end, int base) -> unsigned long long {
        return strtoull(str, str_end, base);
    }
    inline auto CharConstants<wchar_t>::toULongLong(const wchar_t * str, wchar_t ** str_end, int base) -> unsigned long long {
        return wcstoull(str, str_end, base);
    }

    inline auto CharConstants<char>::toFloat(const char * str, char ** str_end) -> float {
        return strtof(str, str_end);
    }
    inline auto CharConstants<wchar_t>::toFloat(const wchar_t * str, wchar_t ** str_end) -> float {
        return wcstof(str, str_end);
    }

    inline auto CharConstants<char>::toDouble(const char * str, char ** str_end) -> double {
        return strtod(str, str_end);
    }
    inline auto CharConstants<wchar_t>::toDouble(const wchar_t * str, wchar_t ** str_end) -> double {
        return wcstod(str, str_end);
    }
    
    inline auto CharConstants<char>::toLongDouble(const char * str, char ** str_end) -> long double {
        return strtold(str, str_end);
    }
    inline auto CharConstants<wchar_t>::toLongDouble(const wchar_t * str, wchar_t ** str_end) -> long double {
        return wcstold(str, str_end);
    }

    template<class Char>
    auto trimInPlace(std::basic_string<Char> & str) -> std::basic_string<Char> & {
        auto firstNotSpace = std::find_if(str.begin(), str.end(), [](const auto c) {
            return !CharConstants<Char>::isSpace(c);
        });
        str.erase(str.begin(), firstNotSpace);
        auto lastNotSpace = std::find_if(str.rbegin(), str.rend(), [](const auto c) {
            return !CharConstants<Char>::isSpace(c);
        }).base();
        str.erase(lastNotSpace, str.end());
        return str;
    }

}

#endif
