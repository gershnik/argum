#ifndef HEADER_MARGP_CHAR_CONSTANTS_H_INCLUDED
#define HEADER_MARGP_CHAR_CONSTANTS_H_INCLUDED

namespace MArgP {

    template<class Char> struct CharConstants;

    #define MARGP_DEFINE_CHAR_CONSTANTS(type, prefix) template<> struct CharConstants<type> { \
        static constexpr auto optionStart                   = prefix ## '-'; \
        static constexpr auto argAssignment                 = prefix ## '='; \
        static constexpr auto formatStart                   = prefix ## '{'; \
        static constexpr auto formatEnd                     = prefix ## '}'; \
        static constexpr auto indentation                   = prefix ## "    ";\
    };

    MARGP_DEFINE_CHAR_CONSTANTS(char, )
    MARGP_DEFINE_CHAR_CONSTANTS(wchar_t, L)
    MARGP_DEFINE_CHAR_CONSTANTS(char8_t, u8)
    MARGP_DEFINE_CHAR_CONSTANTS(char16_t, u)
    MARGP_DEFINE_CHAR_CONSTANTS(char32_t, U)

    #undef MARGP_DEFINE_CHAR_CONSTANTS

}

#endif
