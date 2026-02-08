//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_FORMATTING_H_INCLUDED
#define HEADER_ARGUM_FORMATTING_H_INCLUDED

#include "char-constants.h"
#include "wcwidth.h"

#include <string_view>
#include <string>
#include <charconv>
#include <concepts>
#include <optional>
#include <algorithm>

#include <limits.h>
#include <wchar.h>
#include <assert.h>

namespace Argum {

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

    inline auto append(std::string & dest, const wchar_t * wstr) -> std::string & {
        
        mbstate_t state = mbstate_t();
        size_t len = wcsrtombs(nullptr, &wstr, 0, &state);
        if (len == size_t(-1))
            return dest;
        auto currentSize = dest.size();
        dest.resize(currentSize + len + 1);
        wcsrtombs(&dest[currentSize], &wstr, len + 1, &state);
        dest.resize(currentSize + len);
        return dest;
    }

    inline auto append(std::string & dest, const std::wstring & str) -> std::string & {
        return append(dest, str.c_str());
    }

    inline auto append(std::string & dest, const std::wstring_view & str) -> std::string & {
        return append(dest, std::wstring(str));
    }

    inline auto append(std::wstring & dest, const char * str) -> std::wstring & {
        
        mbstate_t state = mbstate_t();
        size_t len = mbsrtowcs(nullptr, &str, 0, &state);
        if (len == size_t(-1))
            return dest;
        auto currentSize = dest.size();
        dest.resize(currentSize + len + 1);
        mbsrtowcs(&dest[currentSize], &str, len + 1, &state);
        dest.resize(currentSize + len);
        return dest;
    }

    inline auto append(std::wstring & dest, const std::string & str) -> std::wstring & {
        return append(dest, str.c_str());
    }

    inline auto append(std::wstring & dest, const std::string_view & str) -> std::wstring & {
        return append(dest, std::string(str));
    }

    template<class T, class Char>
    concept StringAppendable = Character<Char> && 
        requires(std::basic_string<Char> & str, T && val) {
            { append(str, std::forward<T>(val)) } -> std::same_as<std::basic_string<Char> &>;
    };

    namespace Impl {
        template <typename T>
        concept has_wcwidth = requires(T * t) {
            { wcwidth(t, size_t(0)) };
        };

        constexpr bool wcswidthPresent = has_wcwidth<wchar_t>;

        template<class T = wchar_t>
        inline auto simpleWidth(const T * str, size_t size) -> int {
            if constexpr (wcswidthPresent) {
                return wcswidth(str, size);
            } else {
                return Argum::Wcwidth::wcswidth(str, size);
            }
        }
    }

    inline auto stringWidth(const std::wstring_view & str) -> unsigned {

        int res = Impl::simpleWidth(str.data(), str.size());
        if (res >= 0)
            return res;
        
        std::wstring stripped;
        enum {
            stateNormal,
            stateEsc,
            stateControlStart,
            stateControlIntermediate
        } state = stateNormal;
        for (wchar_t c: str) {
            switch(state) {
            break; case stateNormal: restart:
                if (c == L'\x1b') {
                    state = stateEsc;
                    continue;
                }
                stripped += c;

            break; case stateEsc:
                if (c == L'[') {
                    state = stateControlStart;
                    continue;
                }
                state = stateNormal;
                goto restart;

            break; case stateControlStart:
                if (c >= 0x30 && c <= 0x3F) {
                    continue;
                }
                if (c >= 0x20 && c <= 0x2F) {
                    state = stateControlIntermediate;
                    continue;
                }
                if (c >= 0x40 && c <= 0x7E) {
                    state = stateNormal;
                    continue;
                }
                state = stateNormal;
                goto restart;

            break; case stateControlIntermediate:
                if (c >= 0x20 && c <= 0x2F) {
                    state = stateControlIntermediate;
                    continue;
                }
                if (c >= 0x40 && c <= 0x7E) {
                    state = stateNormal;
                    continue;
                }
                state = stateNormal;
                goto restart;
            }
        }

        res = Impl::simpleWidth(stripped.data(), stripped.size());
        if (res >= 0)
            return res;
        
        return unsigned(stripped.size());
    }

    inline auto stringWidth(const std::string_view & str) -> unsigned {

        std::wstring sigh;
        append(sigh, str);
        return stringWidth(sigh);
    }

    template<class Char, StringAppendable<Char> T>
    auto toString(T && val) -> std::basic_string<Char> {
        std::basic_string<Char> ret;
        append(ret, std::forward<T>(val));
        return ret;
    }
    
    template<Character Char>
    auto appendByIndex(std::basic_string<Char> & /*dest*/, size_t /*idx*/) {

        //do nothing
    }

    template<Character Char, StringAppendable<Char> First, StringAppendable<Char>... Rest>
    auto appendByIndex(std::basic_string<Char> & dest, size_t idx, First && first, Rest && ...rest) {

        if (idx == 0) {
            append(dest, std::forward<First>(first));
            return;
        }

        appendByIndex(dest, idx - 1, rest...);
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
                if (size_t(bufferEnd - bufferCurrent) < size_t(MB_CUR_MAX))
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

    template<StringLike Fmt, StringAppendable<CharTypeOf<Fmt>>... Args>
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

            if (*current == CharConstants::braceOpen) {
                    ++current; 
                    if (current == last) 
                        break;
                    if (*current == CharConstants::braceOpen) {
                        flush(current);
                        ++current;
                        outStart = current;
                        continue;
                    }

                    auto placeholderEnd = std::find(current, last, CharConstants::braceClose);
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

    template<StringLike T>
    auto indent(T && input, unsigned count = 4) -> std::basic_string<CharTypeOf<T>> {

        using Char = CharTypeOf<T>;
        std::basic_string_view<Char> str(std::forward<T>(input));

        std::basic_string<Char> ret;
        size_t start = 0;
        for ( ; ; ) {
            auto lineEnd = str.find(CharConstants<Char>::endl, start);
            if (lineEnd == str.npos)
                break;
            ret += str.substr(start, lineEnd + 1 - start);
            start = lineEnd + 1;
            ret.append(count, CharConstants<char>::space);
        }
        ret += str.substr(start);
        return ret;
    }

    template<StringLike T>
    auto wordWrap(T && input, unsigned maxLength) -> std::basic_string<CharTypeOf<T>> {
        
        using Char = CharTypeOf<T>;
        
        
        if (maxLength == 0)
            return std::basic_string<Char>();

        std::basic_string_view<Char> str(std::forward<T>(input));
        std::basic_string<Char> ret;
        ret.reserve(str.size());
        
        while (!str.empty()) {
            auto eolPos = str.find(CharConstants<Char>::endl);
            auto line = str.substr(0, eolPos);
            bool needLineBreak = (eolPos != str.npos);
            auto width = stringWidth(line);
            while (width > maxLength) {
                auto spacePos = line.rfind(CharConstants<Char>::space);
                if (spacePos == line.npos)
                    break;
                line = line.substr(0, spacePos);
                needLineBreak = true;
                width = stringWidth(line);
            }
            ret += line;
            if (needLineBreak) {
                ret += CharConstants<Char>::endl;
                str = str.substr(line.size() + 1);
            } else {
                str = str.substr(line.size());
            }
        }
        return ret;
    }

}

#endif
