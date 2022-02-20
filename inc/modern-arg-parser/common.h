#ifndef HEADER_MARGP_COMMON_H_INCLUDED
#define HEADER_MARGP_COMMON_H_INCLUDED

#include <type_traits>
#include <string_view>
#include <concepts>

#include <assert.h>

#define MARGP_UTF_CHAR_SUPPORTED 0 //no compiler/library implements all the necessary machinery yet

#define MARGP_DECLARE_FRIENDLY_NAME(stem, type, prefix) using prefix ## stem = Basic##stem<type>;
    
#if MARGP_UTF_CHAR_SUPPORTED
    #define MARGP_DECLARE_FRIENDLY_NAMES(stem) \
        MARGP_DECLARE_FRIENDLY_NAME(stem, char, ) \
        MARGP_DECLARE_FRIENDLY_NAME(stem, wchar_t, L) \
        MARGP_DECLARE_FRIENDLY_NAME(stem, char8_t, U8) \
        MARGP_DECLARE_FRIENDLY_NAME(stem, char16_t, U16) \
        MARGP_DECLARE_FRIENDLY_NAME(stem, char32_t, U32)
#else
    #define MARGP_DECLARE_FRIENDLY_NAMES(stem) \
        MARGP_DECLARE_FRIENDLY_NAME(stem, char, ) \
        MARGP_DECLARE_FRIENDLY_NAME(stem, wchar_t, W)
#endif

#ifndef NDEBUG
    #define MARGP_ALWAYS_ASSERT(x) assert(x)
#else
    #define MARGP_ALWAYS_ASSERT(x)  ((void) ((x) ? ((void)0) : std::terminate()))
#endif

namespace MArgP {

    template<class Char>
    concept Character = std::is_same_v<Char, char> || 
                        std::is_same_v<Char, wchar_t>
#if MARGP_UTF_CHAR_SUPPORTED
                        ||
                        (std::is_same_v<Char, char8_t> ||
                        std::is_same_v<Char, char16_t> ||
                        std::is_same_v<Char, char32_t>
#endif
    ;

    template<class X>
    concept StringLike = requires(X name) {
        Character<std::decay_t<decltype(name[0])>>;
        std::is_convertible_v<X, std::basic_string_view<std::decay_t<decltype(name[0])>>>;
    };

    template<StringLike S> using CharTypeOf = std::decay_t<decltype(std::declval<S>()[0])>;

    template<class X, class Char>
    concept StringLikeOf = Character<Char> && std::is_convertible_v<X, std::basic_string_view<Char>>;


    template<class T, class Char>
    concept ArgIterator  = requires(T & t) {
        t + 0u;
        t - 0u;
        t++;
        t--;
        std::is_convertible_v<decltype(*t), std::basic_string_view<Char>>;
        std::is_same_v<decltype(t != t), bool>;
        std::is_same_v<decltype(t == t), bool>;
        std::is_convertible_v<decltype(t[0]), std::basic_string_view<Char>>;
    };

    template<class T, class Char>
    concept ArgRange = requires(T & t) {
        std::begin(t);
        ArgIterator<decltype(std::begin(t)), Char>;
        std::end(t);
        std::is_same_v<decltype(std::begin(t) != std::end(t)), bool>;
        std::is_convertible_v<decltype(t[0]), std::basic_string_view<Char>>;
    };
    
}

#endif
