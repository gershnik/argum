#ifndef HEADER_MARGP_COMMON_H_INCLUDED
#define HEADER_MARGP_COMMON_H_INCLUDED

#include <type_traits>

#define MARGP_UTF_CHAR_SUPPORTED 0

namespace MArgP {

    template<class Char>
    concept Character = std::is_same_v<Char, char> || 
                        std::is_same_v<Char, wchar_t>
#if MARGP_UTF_CHAR_SUPPORTED
                        ||
                        (std::is_same_v<Char, char8_t> && requires(Char c) { c8rtomb((char*)0, c, (std::mbstate_t*)0); }) ||
                        std::is_same_v<Char, char16_t> ||
                        std::is_same_v<Char, char32_t>
#endif
    ;


}

#endif