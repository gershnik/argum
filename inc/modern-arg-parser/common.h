#ifndef HEADER_MARGP_COMMON_H_INCLUDED
#define HEADER_MARGP_COMMON_H_INCLUDED

#include <type_traits>

#if __has_include(<cuchar>)
    #define MARGP_UTF_CHAR_SUPPORTED 1
#else
    #define MARGP_UTF_CHAR_SUPPORTED 0
#endif

namespace MArgP {

    template<class Char>
    concept Character = std::is_same_v<Char, char> || 
                        std::is_same_v<Char, wchar_t>
#if MARGP_UTF_CHAR_SUPPORTED
                        ||
                        std::is_same_v<Char, char8_t> ||
                        std::is_same_v<Char, char16_t> ||
                        std::is_same_v<Char, char32_t>
#endif
    ;


}

#endif