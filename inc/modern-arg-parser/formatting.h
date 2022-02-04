#ifndef HEADER_MARGP_FORMATTING_H_INCLUDED
#define HEADER_MARGP_FORMATTING_H_INCLUDED

#include "char-constants.h"

#include <string_view>
#include <ostream>
#include <sstream>
#include <charconv>
#include <tuple>
#include <concepts>
#include <optional>
#include <variant>
#if MARGP_UTF_CHAR_SUPPORTED
    #include <cuchar>
#endif

#include <assert.h>

namespace MArgP {

    template<class T, class Char>
    concept StreamPrintable = requires(std::basic_ostream<Char> & str, T && val) {
        { str << val } -> std::same_as<std::basic_ostream<Char> &>;
    };

    template<size_t I, class Char, StreamPrintable<Char>... Args>
    auto printImpl(std::basic_ostream<Char> & str, size_t idx, const std::tuple<Args...> & args) {

        if (idx == 0) {
            str << std::get<I>(args);
            return;
        }

        if constexpr (I + 1 < sizeof...(Args))
            printImpl<I + 1>(str, idx - 1, args);
    }

    template<class Char, StreamPrintable<Char>... Args>
    auto print(std::basic_ostream<Char> & str, size_t idx, const std::tuple<Args...> & args) {

        printImpl<0>(str, idx, args);
    }

    template<Character Char> 
    auto parsePlaceholder(const Char * first, const Char * last) -> std::optional<size_t> {
        if constexpr (!std::is_same_v<Char, char>) {

            char buffer[16 * MB_LEN_MAX]; //16 characters for placeholder number ought to be enough :)
            char * const bufferStart = std::begin(buffer);
            char * const bufferEnd = std::end(buffer);

            char * bufferCurrent = bufferStart;
            std::mbstate_t state = {};
            for ( ; first != last; ++first) {
                if (bufferEnd - bufferCurrent < MB_CUR_MAX)
                    return std::nullopt;

                size_t written;
                if constexpr (std::is_same_v<Char, wchar_t>)
                    written = wcrtomb(bufferCurrent, *first, &state);
            #if MARGP_UTF_CHAR_SUPPORTED
                else if constexpr (std::is_same_v<Char, char8_t>)
                    written = c8rtomb(bufferCurrent, *first, &state);
                else if constexpr (std::is_same_v<Char, char16_t>)
                    written = c16rtomb(bufferCurrent, *first, &state);
                else if constexpr (std::is_same_v<Char, char32_t>)
                    written = c32rtomb(bufferCurrent, *first, &state);
            #endif

                if (written == size_t(-1))
                    return std::nullopt;
                bufferCurrent += written;
            }
            return parsePlaceholder(bufferStart, bufferCurrent);

        } else {

            size_t ret;
            auto res = std::from_chars(first, last, ret);
            if (res.ptr != last || res.ec != std::errc()) 
                return std::nullopt;
            return ret;

        }
    }

    template<class Char, StreamPrintable<Char>... Args>
    struct Formatter {

        std::basic_string_view<Char> fmt;
        std::tuple<Args && ...> args;

        friend auto operator<<(std::basic_ostream<Char> & str, const Formatter & formatter) -> std::basic_ostream<Char> & {
            auto outStart = formatter.fmt.data();
            auto current = outStart;
            const auto last = current + formatter.fmt.size();

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
                            flush(current);
                            ++current;
                            outStart = current;
                            continue;
                        }

                        auto placeholderEnd = std::find(current, last, CharConstants<Char>::formatEnd);
                        if (placeholderEnd == last) 
                            continue;
                        auto maybeArgIdx = parsePlaceholder(current, placeholderEnd);
                        if (!maybeArgIdx || *maybeArgIdx == 0 || *maybeArgIdx > sizeof...(Args)) 
                            continue;

                        flush(current - 1);
                        print(str, *maybeArgIdx - 1, formatter.args);
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
        std::monostate args;
        
        friend auto operator<<(std::basic_ostream<Char> & str, const Formatter & formatter) -> std::basic_ostream<Char> & {
            return str << formatter.fmt;
        }

        operator std::basic_string<Char>() const {
            return (std::basic_ostringstream<Char>() << *this).str();
        }
    };

    template<class Char, StreamPrintable<Char>... Args>
    auto format(std::basic_string_view<Char> fmt, Args && ...args)  {

        return Formatter<Char, Args...>{fmt, {std::forward<Args>(args)...}};
    }

    template<class Char, StreamPrintable<Char>... Args>
    auto format(const Char * fmt, Args && ...args)  {

        return Formatter<Char, Args...>{fmt, {std::forward<Args>(args)...}};
    }

    template<class T>
    struct Printable {
    public:
        Printable(const T & value): m_value(value) {}
        Printable(T && value): m_value(std::move(value)) {}

        template<class Char>
        friend auto operator<<(std::basic_ostream<Char> & str, const Printable & p) -> std::basic_ostream<Char> & {
            p.m_value(str);
            return str;
        }
    private:
        T m_value;
    };

    template<class Char>
    struct Indent {
        int count = 0;

        friend auto operator<<(std::basic_ostream<Char> & str, Indent val) -> std::basic_ostream<Char> & {
            for(int i = 0; i < val.count; ++i)
                str << CharConstants<char>::indentation;
            return str;
        }

        auto operator+(int rhs) const {
            return Indent{this->count + rhs};
        }
        auto operator-(int rhs) const {
            return Indent{this->count - rhs};
        }
    };

}

#endif
