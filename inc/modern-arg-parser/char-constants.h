#ifndef HEADER_MARGP_CHAR_CONSTANTS_H_INCLUDED
#define HEADER_MARGP_CHAR_CONSTANTS_H_INCLUDED

#include "common.h"

#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>
#include <wctype.h>

namespace MArgP {

    template<Character Char> struct CharConstants;

    #define MARGP_DEFINE_CHAR_CONSTANTS(type, prefix) template<> struct CharConstants<type> { \
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
        static constexpr auto endl                          = prefix ## '\n'; \
        static constexpr auto ellipsis                      = prefix ## "..."; \
        \
        static auto isSpace(type c) -> bool; \
        static auto toLongLong(const type * str, type ** str_end, int base) -> long long; \
        static auto toLongDouble(const type * str, type ** str_end) -> long double; \
    };

    MARGP_DEFINE_CHAR_CONSTANTS(char, )
    MARGP_DEFINE_CHAR_CONSTANTS(wchar_t, L)
#if MARGP_UTF_CHAR_SUPPORTED
    MARGP_DEFINE_CHAR_CONSTANTS(char8_t, u8)
    MARGP_DEFINE_CHAR_CONSTANTS(char16_t, u)
    MARGP_DEFINE_CHAR_CONSTANTS(char32_t, U)
#endif

    #undef MARGP_DEFINE_CHAR_CONSTANTS
    
    inline auto CharConstants<char>::isSpace(char c) -> bool { return isspace(c); }
    inline auto CharConstants<wchar_t>::isSpace(wchar_t c) -> bool { return iswspace(c); }
    
    inline auto CharConstants<char>::toLongLong(const char * str, char ** str_end, int base) -> long long {
        return strtoll(str, str_end, base);
    }
    inline auto CharConstants<wchar_t>::toLongLong(const wchar_t * str, wchar_t ** str_end, int base) -> long long {
        return wcstoll(str, str_end, base);
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
        auto lastNotSpace = std::find_if(str.begin(), str.end(), [](const auto c) {
            return CharConstants<Char>::isSpace(c);
        });
        str.erase(lastNotSpace, str.end());
        return str;
    }

}

#endif
