//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_TYPE_PARSERS_H_INCLUDED
#define HEADER_ARGUM_TYPE_PARSERS_H_INCLUDED

#include "parser.h"

#include <string>
#include <type_traits>
#include <regex>

namespace Argum {

    ARGUM_MOD_EXPORTED
    template<class T, StringLike S> 
    requires(std::is_integral_v<T>)
    auto parseIntegral(S && str, int base = 0) -> ARGUM_EXPECTED(CharTypeOf<S>, T) {

        using Char = CharTypeOf<S>;
        using ValidationError = typename BasicParser<Char>::ValidationError;
        using CharConstants = CharConstants<Char>;
        using Messages = Messages<Char>;

        std::basic_string<Char> value(std::forward<S>(str));

        if (value.empty())
            ARGUM_THROW(ValidationError, format(Messages::notANumber(), value));

        T ret;
        Char * endPtr;

        errno = 0;
        if constexpr (std::is_signed_v<T>) {

            if constexpr (sizeof(T) <= sizeof(long)) {
                auto res = CharConstants::toLong(value.data(), &endPtr, base);
                if constexpr (sizeof(T) < sizeof(res)) {
                    if (res < (decltype(res))std::numeric_limits<T>::min() || res > (decltype(res))std::numeric_limits<T>::max())
                        ARGUM_THROW(ValidationError, format(Messages::outOfRange(), value));
                }
                ret = T(res);
            } else  {
                auto res = CharConstants::toLongLong(value.data(), &endPtr, base);
                if constexpr (sizeof(T) < sizeof(res)) {
                    if (res < (decltype(res))std::numeric_limits<T>::min() || res > (decltype(res))std::numeric_limits<T>::max())
                        ARGUM_THROW(ValidationError, format(Messages::outOfRange(), value));
                }
                ret = T(res);
            }

        } else {

            if constexpr (sizeof(T) <= sizeof(unsigned long)) {
                auto res = CharConstants::toULong(value.data(), &endPtr, base);
                if constexpr (sizeof(T) < sizeof(res)) {
                    if (res > (decltype(res))std::numeric_limits<T>::max())
                        ARGUM_THROW(ValidationError, format(Messages::outOfRange(), value));
                }
                ret = T(res);
            } else  {
                auto res = CharConstants::toULongLong(value.data(), &endPtr, base);
                if constexpr (sizeof(T) < sizeof(res)) {
                    if (res > (decltype(res))std::numeric_limits<T>::max())
                        ARGUM_THROW(ValidationError, format(Messages::outOfRange(), value));
                }
                ret = T(res);
            }
        }
        if (errno == ERANGE)
            ARGUM_THROW(ValidationError, format(Messages::outOfRange(), value));
        for ( ; endPtr != value.data() + value.size(); ++endPtr) {
            if (!CharConstants::isSpace(*endPtr))
                ARGUM_THROW(ValidationError, format(Messages::notANumber(), value));
        }
        
        return ret;
    }

    ARGUM_MOD_EXPORTED
    template<class T, StringLike S> 
    requires(std::is_floating_point_v<T>)
    auto parseFloatingPoint(S && str) -> ARGUM_EXPECTED(CharTypeOf<S>, T) {

        using Char = CharTypeOf<S>;
        using ValidationError = typename BasicParser<Char>::ValidationError;
        using CharConstants = CharConstants<Char>;
        using Messages = Messages<Char>;

        std::basic_string<Char> value(std::forward<S>(str));

        if (value.empty())
            ARGUM_THROW(ValidationError, format(Messages::notANumber(), value));

        T ret;
        Char * endPtr;

        errno = 0;
        if constexpr (std::is_same_v<T, float>) {
            ret = CharConstants::toFloat(value.data(), &endPtr);
        } else if constexpr (std::is_same_v<T, double>) {
            ret = CharConstants::toDouble(value.data(), &endPtr);
        } else if constexpr (std::is_same_v<T, long double>) {
            ret = CharConstants::toLongDouble(value.data(), &endPtr);
        }

        if (errno == ERANGE)
            ARGUM_THROW(ValidationError, format(Messages::outOfRange(), value));
        for ( ; endPtr != value.data() + value.size(); ++endPtr) {
            if (!CharConstants::isSpace(*endPtr))
                ARGUM_THROW(ValidationError, format(Messages::notANumber(), value));
        }
        
        return ret;
    }

    ARGUM_MOD_EXPORTED
    template<Character Char>
    class BasicChoiceParser {
    public:
        using StringViewType = std::basic_string_view<Char>;

        struct Settings {
            bool caseSensitive = false;
            bool allowElse = false;
        };
    private:
        using StringType = std::basic_string<Char>;
        using RegexType = std::basic_regex<Char>;
        using CharConstants = Argum::CharConstants<Char>;
        using Messages = Argum::Messages<Char>;
        using ValidationError = typename BasicParser<Char>::ValidationError;
    public:
        BasicChoiceParser(Settings settings = Settings()) : m_options(std::regex_constants::ECMAScript) {
            if (!settings.caseSensitive)
                m_options |= std::regex_constants::icase;
            this->m_allowElse = settings.allowElse;
        }

        template<StringLikeOf<Char> First, StringLikeOf<Char>... Rest>
        auto addChoice(First && first, Rest && ...rest) {
            StringType exp = this->escape(std::forward<First>(first));
            if (!this->m_description.empty())
                this->m_description += Messages::listJoiner();
            this->m_description += std::forward<First>(first);
            this->combine(exp, this->m_description, std::forward<Rest>(rest)...);
            this->m_choices.emplace_back(exp, m_options);
        }

        auto addChoice(std::initializer_list<const Char *> values) {
            auto it = values.begin();
            if (it == values.end())
                ARGUM_INVALID_ARGUMENT("choices list cannot be empty");
            StringType exp = this->escape(*it);
            if (!this->m_description.empty())
                this->m_description += Messages::listJoiner();
            this->m_description += *it;
            for(++it; it != values.end(); ++it)
                this->combine(exp, this->m_description, *it);
            this->m_choices.emplace_back(exp, m_options);
        }

        auto parse(StringViewType value) const -> ARGUM_EXPECTED(Char, size_t) {

            auto it = std::find_if(this->m_choices.begin(), this->m_choices.end(), [&](const RegexType & regex) {
                return regex_match(value.begin(), value.end(), regex);
            });
            if (it == this->m_choices.end()) {
                if (!m_allowElse)
                    ARGUM_THROW(ValidationError, format(Messages::notAValidChoice(), value, this->m_description));
                return this->m_choices.size();
            }
            return it - this->m_choices.begin();
        }

        auto description() const -> StringType {
            return CharConstants::braceOpen + this->m_description + CharConstants::braceClose;
        }
    private:
        auto combine(StringType &, StringType &) {
            return;
        }

        template<StringLikeOf<Char> First, StringLikeOf<Char>... Rest>
        auto combine(StringType & exp, StringType & desc, First && first, Rest && ...rest) {
            exp += CharConstants::pipe;
            exp += this->escape(std::forward<First>(first));
            desc += Messages::listJoiner();
            desc += std::forward<First>(first);
            this->combine(exp, desc, std::forward<Rest>(rest)...);
        }

        template<StringLikeOf<Char> Arg>
        static auto escape(Arg && arg) -> StringType {
            
            StringViewType view(arg);
            if (view.empty())
                ARGUM_INVALID_ARGUMENT("choice cannot be empty");
            
            StringType ret;
            regex_replace(std::back_inserter(ret), view.begin(), view.end(), 
                          BasicChoiceParser::s_needToBeEscaped, BasicChoiceParser::s_escaped,
                          std::regex_constants::match_default | std::regex_constants::format_sed);
            return ret;
        }
    private:
        std::vector<RegexType> m_choices;
        StringType m_description;
        std::regex_constants::syntax_option_type m_options;
        bool m_allowElse;

        static inline const RegexType s_needToBeEscaped{CharConstants::mustEscapeInRegex};
        static inline const StringType s_escaped = CharConstants::regexEscapeReplacement;
    };

    ARGUM_DECLARE_FRIENDLY_NAMES(ChoiceParser)

    ARGUM_MOD_EXPORTED
    template<Character Char>
    class BasicBooleanParser {
    public:
        BasicBooleanParser() {
            this->m_impl.addChoice(CharConstants<Char>::falseNames);
            this->m_impl.addChoice(CharConstants<Char>::trueNames);
        }

        auto parse(std::basic_string_view<Char> value) const -> ARGUM_EXPECTED(Char, bool) {
            ARGUM_CHECK_RESULT(auto ret, this->m_impl.parse(value));
            return bool(ret);
        }
    private:
        BasicChoiceParser<Char> m_impl;
    };

    ARGUM_DECLARE_FRIENDLY_NAMES(BooleanParser)

}

#endif