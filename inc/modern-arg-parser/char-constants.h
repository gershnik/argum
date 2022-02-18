#ifndef HEADER_MARGP_CHAR_CONSTANTS_H_INCLUDED
#define HEADER_MARGP_CHAR_CONSTANTS_H_INCLUDED

#include "common.h"

namespace MArgP {

    template<Character Char> struct CharConstants;

    #define MARGP_DEFINE_CHAR_CONSTANTS(type, prefix) template<> struct CharConstants<type> { \
        static constexpr auto optionStart                   = prefix ## '-'; \
        static constexpr auto argAssignment                 = prefix ## '='; \
        static constexpr auto formatStart                   = prefix ## '{'; \
        static constexpr auto formatEnd                     = prefix ## '}'; \
        static constexpr auto space                         = prefix ## ' '; \
        static constexpr auto optBracketOpen                = prefix ## '['; \
        static constexpr auto optBracketClose               = prefix ## ']'; \
        static constexpr auto endl                          = prefix ## '\n'; \
        static constexpr auto ellipsis                      = prefix ## "..."; \
    };

    MARGP_DEFINE_CHAR_CONSTANTS(char, )
    MARGP_DEFINE_CHAR_CONSTANTS(wchar_t, L)
#if MARGP_UTF_CHAR_SUPPORTED
    MARGP_DEFINE_CHAR_CONSTANTS(char8_t, u8)
    MARGP_DEFINE_CHAR_CONSTANTS(char16_t, u)
    MARGP_DEFINE_CHAR_CONSTANTS(char32_t, U)
#endif

    #undef MARGP_DEFINE_CHAR_CONSTANTS

}

#endif
