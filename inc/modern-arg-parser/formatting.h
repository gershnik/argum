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
#if MARGP_UTF_CHAR_SUPPORTED
    #include <cuchar>
#endif

#include <assert.h>

namespace MArgP {

    template<size_t I, Character Char, StreamPrintable<Char>... Args>
    auto printImpl(std::basic_ostream<Char> & str, size_t idx, const std::tuple<Args...> & args) {

        if (idx == 0) {
            str << std::get<I>(args);
            return;
        }

        if constexpr (I + 1 < sizeof...(Args))
            printImpl<I + 1>(str, idx - 1, args);
    }

    template<Character Char, StreamPrintable<Char>... Args>
    auto print(std::basic_ostream<Char> & str, size_t idx, const std::tuple<Args...> & args) {

        if constexpr (sizeof...(Args) > 0)
            printImpl<0>(str, idx, args);
    }

    template<Character Char, StreamPrintable<Char>... Args>
    struct Formatter {

        std::basic_string_view<Char> fmt;
        std::tuple<Args && ...> args;

        auto format(std::basic_ostream<Char> & str) -> std::basic_ostream<Char> & {
            auto outStart = this->fmt.data();
            auto current = outStart;
            const auto last = current + this->fmt.size();

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
                        auto maybeArgIdx = Formatter::parsePlaceholder(current, placeholderEnd);
                        if (!maybeArgIdx || *maybeArgIdx == 0 || *maybeArgIdx > sizeof...(Args)) 
                            continue;

                        flush(current - 1);
                        print(str, *maybeArgIdx - 1, this->args);
                        current = placeholderEnd + 1;
                        outStart = current;
                } else {
                    ++current; 
                }
            }
            flush(last);
            return str;
        }
    private:
        static auto parsePlaceholder(const Char * first, const Char * last) -> std::optional<size_t> {
            
            [[maybe_unused]] char buffer[16 * MB_LEN_MAX]; //16 characters for placeholder number ought to be enough :)
            const char * convertFirst;
            const char * convertLast;
            
            if constexpr (!std::is_same_v<Char, char>) {

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
                    else if constexpr (std::is_same_v<Char, char8_t>)
                        written = c8rtomb(bufferCurrent, *first, &state);
                    else if constexpr (std::is_same_v<Char, char16_t>)
                        written = c16rtomb(bufferCurrent, *first, &state);
                    else if constexpr (std::is_same_v<Char, char32_t>)
                        written = c32rtomb(bufferCurrent, *first, &state);

                    if (written == size_t(-1))
                        return std::nullopt;
                    bufferCurrent += written;
                }
                convertFirst = bufferStart;
                convertLast = bufferCurrent;

            } else {
                
                convertFirst = first;
                convertLast = last;
            }
            
            size_t ret;
            auto res = std::from_chars(convertFirst, convertLast, ret);
            if (res.ptr != convertLast || res.ec != std::errc())
                return std::nullopt;
            return ret;
        }
    };

    template<StringLike Fmt, StreamPrintable<std::decay_t<decltype(std::declval<Fmt>()[0])>>... Args>
    auto format(Fmt fmt, Args && ...args)  {

        using Char = std::decay_t<decltype(fmt[0])>;
        std::basic_ostringstream<Char> str;
        Formatter<Char, Args...>{fmt, {std::forward<Args>(args)...}}.format(str);
        return str.str();
    }

    template<class T>
    struct Printable {
    public:
        Printable(const T & value): m_value(value) {}
        Printable(T && value): m_value(std::move(value)) {}

        template<Character Char>
        friend auto operator<<(std::basic_ostream<Char> & str, const Printable & p) -> std::basic_ostream<Char> & {
            p.m_value(str);
            return str;
        }
    private:
        T m_value;
    };

    template<Character Char>
    class Indent {
    public:
        static constexpr unsigned defaultValue = 4;

        Indent() : m_count(Indent::defaultValue) {}
        Indent(unsigned c) : m_count(c) {}

        friend auto operator<<(std::basic_ostream<Char> & str, Indent val) -> std::basic_ostream<Char> & {
            for(unsigned i = 0; i < val.m_count; ++i)
                str << CharConstants<char>::space;
            return str;
        }

        auto operator+(unsigned rhs) const {
            return Indent{this->m_count + rhs};
        }
        auto operator-(unsigned rhs) const {
            return Indent{this->m_count > rhs ? this->m_count - rhs : 0};
        }
        auto operator*(unsigned rhs) const {
            return Indent{this->m_count * rhs};
        }
    private:
        unsigned m_count = 0;
    };

    template<class T, class Char>
    concept Describable = 
        requires(const T & val) {
            { describe(Indent<Char>{0}, val) } -> StreamPrintable<Char>;
        };

    template<class Char, StringLike T>
    auto describe(Indent<Char> indent, T val) {
        return Printable([=, val=std::move(val)](std::basic_ostream<Char> & str) {
            std::basic_string_view<Char> view(val);
            size_t start = 0;
            for ( ; ; ) {
                auto lineEnd = view.find(CharConstants<Char>::endl, start);
                str << view.substr(start, lineEnd);
                if (lineEnd == view.npos)
                    break;
                str << CharConstants<Char>::endl << indent;
                start = lineEnd + 1;
            }
        });
    }

    template<class Char, StreamPrintable<Char> T>
    requires(!StringLike<T>)
    auto describe(Indent<Char> indent, const T & val) {
        return describe(indent, (std::basic_ostringstream<Char>() << val).str());
    }

    template<class Char, Describable<Char> T>
    auto describe(const T & val) {
        return describe(Indent<Char>{0}, val);
    }

    template<class Char, Describable<Char> T>
    auto describeToStr(const T & val) {
        return (std::basic_ostringstream<Char>() << describe<Char>(val)).str();
    }

}

#endif
