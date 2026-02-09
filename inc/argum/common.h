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
#include <exception>

#include <assert.h>
#include <stdio.h>

#ifndef ARGUM_MOD_EXPORTED
    #define ARGUM_MOD_EXPORTED
#endif

#define ARGUM_DECLARE_FRIENDLY_NAME(stem, type, prefix) ARGUM_MOD_EXPORTED using prefix ## stem = Basic##stem<type>;
    
#define ARGUM_DECLARE_FRIENDLY_NAMES(stem) \
    ARGUM_DECLARE_FRIENDLY_NAME(stem, char, ) \
    ARGUM_DECLARE_FRIENDLY_NAME(stem, wchar_t, W)

#define ARGUM_CONCAT1(a, b) a ## b
#define ARGUM_CONCAT(a, b) ARGUM_CONCAT1(a, b)

#ifdef __COUNTER__
    #define ARGUM_UNIQUE_NAME(stem) ARGUM_CONCAT(stem, __COUNTER__)
#else
    #define ARGUM_UNIQUE_NAME(stem) ARGUM_CONCAT(stem, __LINE__)
#endif

#ifndef NDEBUG
    #define ARGUM_ALWAYS_ASSERT(x) assert(x)
#else
    #define ARGUM_ALWAYS_ASSERT(x)  ((void) ((x) ? ((void)0) : ::Argum::terminateApplication("failed assertion `" #x "'")))
#endif

#define ARGUM_INVALID_ARGUMENT(message) ::Argum::terminateApplication(message)


#if defined(__GNUC__)
    #ifndef __EXCEPTIONS
        #define ARGUM_NO_THROW
    #endif
#elif defined(_MSC_VER)
    #if !defined(_CPPUNWIND) || _HAS_EXCEPTIONS == 0
        #define ARGUM_NO_THROW
    #endif
#endif

#ifndef ARGUM_NO_THROW
    #define ARGUM_RAISE_EXCEPTION(x) throw x
#else
    #ifndef ARGUM_USE_EXPECTED
        #define ARGUM_USE_EXPECTED 1
    #endif
    #define ARGUM_RAISE_EXCEPTION(x) ::Argum::terminateApplication((x).what())
#endif 

namespace Argum {

    [[noreturn]] auto terminateApplication(const char * message) -> void; 

    #ifndef ARGUM_CUSTOM_TERMINATE
        [[noreturn]] inline auto terminateApplication(const char * message) -> void { 
            fprintf(stderr, "%s\n", message); 
            fflush(stderr); 
            #ifndef NDEBUG
                assert(false);
            #endif
            std::terminate();
        }
    #endif

    template<class Char>
    concept Character = std::is_same_v<Char, char> || 
                        std::is_same_v<Char, wchar_t>;

    template<class X>
    concept StringLike = requires(X name) {
        requires Character<std::remove_cvref_t<decltype(name[0])>>;
        requires std::is_convertible_v<X, std::basic_string_view<std::remove_cvref_t<decltype(name[0])>>>;
    };

    template<StringLike S> using CharTypeOf = std::remove_cvref_t<decltype(std::declval<S>()[0])>;

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
        requires ArgIterator<decltype(std::begin(t)), Char>;
        std::end(t);
        requires std::is_same_v<decltype(std::begin(t) != std::end(t)), bool>;
        requires std::is_convertible_v<decltype(t[0]), std::basic_string_view<Char>>;
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

    ARGUM_MOD_EXPORTED
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

    ARGUM_MOD_EXPORTED
    template<class It1, class It2, class Joiner, class Projection>
    auto join(It1 first, It2 last, Joiner joiner, Projection proj) -> decltype(proj(*first) + joiner) {
        
        decltype(proj(*first) + joiner) ret;

        if (first == last)
            return ret;
        
        ret += proj(*first);
        for(++first; first != last; ++first) {
            ret += joiner;
            ret += proj(*first);
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
