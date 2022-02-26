//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_DATA_H_INCLUDED
#define HEADER_ARGUM_DATA_H_INCLUDED

#include "char-constants.h"
#include "formatting.h"

#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <limits>

namespace Argum {

    template<class Char>
    class BasicOptionNames final {
    
    public:
        using CharType = Char;
        using StringType = std::basic_string<Char>;
        using StringViewType = std::basic_string_view<Char>;
        using CharConstants = Argum::CharConstants<Char>;
    public:
        template<StringConvertibleOf<CharType> First, StringConvertibleOf<CharType>... Rest>
        BasicOptionNames(First && first, Rest && ...rest):
            m_values{makeString(std::forward<First>(first)), makeString(std::forward<Rest>(rest))...} {
        }

        auto main() const -> const StringType & {
            return this->m_values[0];
        }

        auto all() const -> const std::vector<StringType> & {
            return this->m_values;
        }


    private:
        std::vector<StringType> m_values;
    };

    ARGUM_DECLARE_FRIENDLY_NAMES(OptionNames)

    enum class OptionArgument  {
        None,
        Optional,
        Required
    };

    class Quantifier {
    public:
        static constexpr unsigned infinity = std::numeric_limits<unsigned>::max();

        constexpr Quantifier(unsigned val): m_min(val), m_max(val) {
        }
        constexpr Quantifier(unsigned min, unsigned max): m_min(min), m_max(max) {
            if (min > max)
                ARGUM_INVALID_ARGUMENT("min must be less or equal to max");
        }
        
        constexpr auto min() const {
            return this->m_min;
        }

        constexpr auto max() const {
            return this->m_max;
        }

    private:
        unsigned m_min = 0;
        unsigned m_max = 0;
    };

    inline constexpr Quantifier ZeroOrOneTime  (0, 1);
    inline constexpr Quantifier NeverOrOnce    = ZeroOrOneTime;
    inline constexpr Quantifier OneTime        (1, 1);
    inline constexpr Quantifier Once           = OneTime;
    inline constexpr Quantifier ZeroOrMoreTimes(0, Quantifier::infinity);
    inline constexpr Quantifier OneOrMoreTimes (1, Quantifier::infinity);
    inline constexpr Quantifier OnceOrMore     = OneOrMoreTimes;

    template<class Char>
    class BasicParsingException : public std::runtime_error {
    public:
        auto message() const -> std::basic_string_view<Char> {
            return m_message;
        }
        
    protected:
        BasicParsingException(std::basic_string<Char> message) : 
            std::runtime_error(toString<char>(message)),
            m_message(std::move(message)) {
        }

    private:
        std::basic_string<Char> m_message;
    };

    template<>
    class BasicParsingException<char> : public std::runtime_error {
    public:
        auto message() const -> std::string_view {
            return what();
        }
    protected:
        BasicParsingException(std::string message) : 
            std::runtime_error(message) {
        }
    };
    
    ARGUM_DECLARE_FRIENDLY_NAMES(ParsingException)

}


#endif
