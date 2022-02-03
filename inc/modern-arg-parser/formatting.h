#ifndef HEADER_MARGP_FORMATTING_H_INCLUDED
#define HEADER_MARGP_FORMATTING_H_INCLUDED

#include "char-constants.h"

#include <string_view>
#include <ostream>
#include <sstream>
#include <charconv>
#include <tuple>

#include <assert.h>

namespace MArgP {

    template<size_t I, class Char, class... Args>
    auto printImpl(std::basic_ostream<Char> & str, size_t idx, const std::tuple<Args...> & args) {

        if (idx == 0) {
            str << std::get<I>(args);
            return;
        }

        if constexpr (I + 1 < sizeof...(Args))
            printImpl<I + 1>(str, idx - 1, args);
    }

    template<class Char, class... Args>
    auto print(std::basic_ostream<Char> & str, size_t idx, const std::tuple<Args...> & args) {

        printImpl<0>(str, idx, args);
    }

    template<class Char, class... Args>
    struct Formatter {

        std::basic_string_view<Char> fmt;
        std::tuple<Args && ...> args;

        friend auto operator<<(std::basic_ostream<Char> & str, const Formatter & formatter) -> std::basic_ostream<Char> & {
            auto outStart = formatter.fmt.begin();
            auto current = formatter.fmt.begin();
            const auto last = formatter.fmt.end();

            auto flush = [&](auto end) {
                assert(end >= outStart);
                str << std::basic_string_view<Char>(outStart, size_t(end - outStart));
            };

            while(current != last) {

                if (*current == CharConstants<Char>::formatStart) {
                        ++current; 
                        if (current == last) 
                            break;
                        if (*current == CharConstants<Char>::formatStart) {
                            flush(current - 1);
                            ++current;
                            outStart = current;
                            continue;
                        }

                        auto placeholderEnd = std::find(current, last, CharConstants<Char>::formatEnd);
                        if (placeholderEnd == last) 
                            continue;
                        size_t argIdx;
                        auto res = std::from_chars(current, placeholderEnd, argIdx);
                        if (res.ptr != placeholderEnd || res.ec != std::errc() || argIdx == 0 || argIdx > sizeof...(Args)) 
                            continue;

                        flush(current - 1);
                        print(str, argIdx - 1, formatter.args);
                        current = placeholderEnd + 1;
                        outStart = current;
                } else {
                    ++current; 
                }
            }
            flush(last);
            return str;
        }

        operator std::basic_string<Char>() const {
            return (std::basic_ostringstream<Char>() << *this).str();
        }
    };

    template<class Char>
    struct Formatter<Char> {
        std::basic_string_view<Char> fmt;
        
        friend auto operator<<(std::basic_ostream<Char> & str, const Formatter & formatter) -> std::basic_ostream<Char> & {
            return str << formatter.fmt;
        }

        operator std::basic_string<Char>() const {
            return (std::basic_ostringstream<Char>() << *this).str();
        }
    };

    template<class Char, class... Args>
    auto format(std::basic_string_view<Char> fmt, Args && ...args)  {

        return Formatter<Char, Args...>{fmt, {std::forward<Args>(args)...}};
    }

    template<class Char, class... Args>
    auto format(const Char * fmt, Args && ...args)  {

        return Formatter<Char, Args...>{fmt, {std::forward<Args>(args)...}};
    }

    template<class Char>
    struct Indent {
        int count = 0;

        friend auto operator<<(std::basic_ostream<Char> & str, Indent val) -> std::basic_ostream<Char> & {
            for(int i = 0; i < val.count; ++i)
                str << CharConstants<char>::indentation;
            return str;
        }
    };

}

#endif
