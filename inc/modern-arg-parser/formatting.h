#ifndef HEADER_MARGP_FORMATTING_H_INCLUDED
#define HEADER_MARGP_FORMATTING_H_INCLUDED

#include "char-constants.h"

#include <string_view>
#include <string>
#include <charconv>
#include <concepts>
#include <optional>
#if MARGP_UTF_CHAR_SUPPORTED
    #include <cuchar>
#endif

#include <assert.h>

namespace MArgP {

    template<class T>
    auto append(std::string & dest, T val) -> std::string &
    requires(std::is_same_v<decltype(std::to_string(val)), std::string>) {
        return dest += std::to_string(val);
    }

    template<class T>
    auto append(std::wstring & dest, T val) -> std::wstring &
    requires(std::is_same_v<decltype(std::to_wstring(val)), std::wstring>)  {
        return dest += std::to_wstring(val);
    }

    template<Character Char, class T>
    auto append(std::basic_string<Char> & dest, T && val) -> std::basic_string<Char> &
    requires(std::is_same_v<decltype(dest += std::forward<T>(val)), decltype(dest)>) {
        return dest += std::forward<T>(val);
    }

    inline auto append(std::string & dest, const std::wstring & val) -> std::string & {
        
        std::mbstate_t state {};
        char buf[MB_LEN_MAX];
        for(wchar_t wc : val) {
            std::size_t ret = std::wcrtomb(&buf[0], wc, &state);
            dest.append(buf, ret);
        }
        return dest;
    }

    inline auto append(std::wstring & dest, const std::string & val) -> std::wstring & {
        
        std::mbstate_t state = std::mbstate_t(); 
        for(size_t i = 0; i < val.size(); ) {
            wchar_t wc;
            auto len = std::mbrtowc(&wc, &val[i], val.size() - i, &state);
            if (ssize_t(len) <= 0)
                break;
            dest += wc;
            i += len;
        }
        return dest;
    }

    template<class T, class Char>
    concept Formattable = Character<Char> && 
            requires(std::basic_string<Char> & str, T && val) {
                { append(str, std::forward<T>(val)) } -> std::same_as<std::basic_string<Char> &>;
            };

    template<class Char, Formattable<Char> T>
    auto toString(T && val) -> std::basic_string<Char> {
        std::basic_string<Char> ret;
        append(ret, std::forward<T>(val));
        return ret;
    }

    template<Character Char, Formattable<Char> First, Formattable<Char>... Rest>
    auto appendByIndex(std::basic_string<Char> & dest, size_t idx, First && first, Rest && ...rest) {

        if (idx == 0) {
            append(dest, std::forward<First>(first));
            return;
        }

        if constexpr (sizeof...(Rest) > 0)
            appendByIndex(dest, idx - 1, rest...);
    }

    template<Character Char>
    auto appendByIndex(std::basic_string<Char> & /*dest*/, size_t /*idx*/) {

        //do nothing
    }

    template<Character Char>
    auto parseFormatPlaceholder(const Char * first, const Char * last) -> std::optional<size_t> {
            
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

    template<StringLike Fmt, Formattable<CharTypeOf<Fmt>>... Args>
    auto format(Fmt && fmt, Args && ...args)  {

        using CharType = CharTypeOf<Fmt>;
        using StringViewType = std::basic_string_view<CharType>;
        using StringType = std::basic_string<CharType>;
        using CharConstants = CharConstants<CharType>;

        StringViewType fmtView(fmt);

        StringType ret;

        auto outStart = fmtView.data();
        auto current = outStart;
        const auto last = current + fmtView.size();

        auto flush = [&](auto end) {
            assert(end >= outStart);
            append(ret, StringViewType(outStart, size_t(end - outStart)));
        };

        while(current != last) {

            if (*current == CharConstants::formatStart) {
                    ++current; 
                    if (current == last) 
                        break;
                    if (*current == CharConstants::formatStart) {
                        flush(current);
                        ++current;
                        outStart = current;
                        continue;
                    }

                    auto placeholderEnd = std::find(current, last, CharConstants::formatEnd);
                    if (placeholderEnd == last) 
                        continue;
                    auto maybeArgIdx = parseFormatPlaceholder(current, placeholderEnd);
                    if (!maybeArgIdx || *maybeArgIdx == 0 || *maybeArgIdx > sizeof...(Args)) 
                        continue;

                    flush(current - 1);
                    appendByIndex(ret, *maybeArgIdx - 1, args...);
                    current = placeholderEnd + 1;
                    outStart = current;
            } else {
                ++current; 
            }
        }
        flush(last);
        return ret;
    }

    template<Character Char>
    class Indent {
    public:
        static constexpr unsigned defaultValue = 4;

        Indent() : m_count(Indent::defaultValue) {}
        Indent(unsigned c) : m_count(c) {}

        auto apply(const std::basic_string<Char> & str) const -> std::basic_string<Char> {
            std::basic_string<Char> ret;
            size_t start = 0;
            for ( ; ; ) {
                auto lineEnd = str.find(CharConstants<Char>::endl, start);
                ret += str.substr(start, lineEnd + 1);
                if (lineEnd == str.npos)
                    break;
                for(unsigned i = 0; i < this->m_count; ++i)
                    ret += CharConstants<char>::space;
                start = lineEnd + 1;
            }
            ret += str.substr(start);
            return ret;
        }

        auto count() {
            return this->m_count;
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

}

#endif
