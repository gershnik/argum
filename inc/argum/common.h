//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_COMMON_H_INCLUDED
#define HEADER_ARGUM_COMMON_H_INCLUDED

#include <type_traits>
#include <string_view>
#include <string>
#include <concepts>

#include <assert.h>

#define ARGUM_DECLARE_FRIENDLY_NAME(stem, type, prefix) using prefix ## stem = Basic##stem<type>;
    
#define ARGUM_DECLARE_FRIENDLY_NAMES(stem) \
    ARGUM_DECLARE_FRIENDLY_NAME(stem, char, ) \
    ARGUM_DECLARE_FRIENDLY_NAME(stem, wchar_t, W)

#ifndef NDEBUG
    #define ARGUM_ALWAYS_ASSERT(x) assert(x)
#else
    #define ARGUM_ALWAYS_ASSERT(x)  ((void) ((x) ? ((void)0) : std::terminate()))
#endif

#ifndef ARGUM_INVALID_ARGUMENT
    #define ARGUM_INVALID_ARGUMENT(message) ARGUM_ALWAYS_ASSERT(!message)
#endif

namespace Argum {

    template<class Char>
    concept Character = std::is_same_v<Char, char> || 
                        std::is_same_v<Char, wchar_t>;

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


    template<Character Char>
    auto makeString(Char c) -> std::basic_string<Char> {
        return std::basic_string<Char>(1, c);
    }

    template<Character Char>
    auto makeString(std::basic_string_view<Char> view) -> std::basic_string<Char> {
        return std::basic_string<Char>(view);
    }

    template<Character Char>
    auto makeString(const Char * str) -> std::basic_string<Char> {
        return std::basic_string<Char>(str);
    }

    template<Character Char>
    auto makeString(std::basic_string<Char> && str) -> std::basic_string<Char> {
        return std::basic_string<Char>(std::move(str));
    }

    template<Character Char>
    auto makeString(const std::basic_string<Char> & str) -> std::basic_string<Char> {
        return std::basic_string<Char>(str);
    }

    template<class T, class Char>
    concept StringConvertibleOf = Character<Char> && requires(T && t) {
        {makeString(t)} -> std::same_as<std::basic_string<Char>>;
    };

    template<class It1, class It2, class Joiner>
    auto join(It1 first, It2 last, Joiner joiner) -> decltype(*first + joiner) {
        
        decltype(*first + joiner) ret;

        if (first == last)
            return ret;
        
        ret += *first;
        for(++first; first != last; ++first) {
            ret += joiner;
            ret += *first;
        };
        return ret;
    }

    template<class Char>
    auto matchPrefix(std::basic_string_view<Char> value, std::basic_string_view<Char> prefix) -> bool {

        if (value.size() < prefix.size())
            return false;

        auto prefixCurrent = prefix.begin();
        auto prefixLast = prefix.end();
        auto valueCurrent = value.begin();
        for ( ; ; ) {
            if (*prefixCurrent != *valueCurrent)
                return false;
            if (++prefixCurrent == prefixLast)
                return true;
            ++valueCurrent;
        }
    }

    template<class Char>
    auto matchStrictPrefix(std::basic_string_view<Char> value, std::basic_string_view<Char> prefix) -> bool {

        if (value.size() <= prefix.size())
            return false;

        auto prefixCurrent = prefix.begin();
        auto prefixLast = prefix.end();
        auto valueCurrent = value.begin();
        for ( ; ; ) {
            if (*prefixCurrent != *valueCurrent)
                return false;
            if (++prefixCurrent == prefixLast)
                return true;
            ++valueCurrent;
        }
    }
    
}

#endif
