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

    class Repeated {
    public:
        static constexpr unsigned infinity = std::numeric_limits<unsigned>::max();

        constexpr explicit Repeated(unsigned val): m_min(val), m_max(val) {
        }
        constexpr Repeated(unsigned min, unsigned max): m_min(min), m_max(max) {
            ARGUM_ALWAYS_ASSERT(min <= max);
        }

        static const Repeated zeroOrOnce;
        static const Repeated once;
        static const Repeated zeroOrMore;
        static const Repeated oneOrMore;

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

    inline constexpr Repeated Repeated::zeroOrOnce = Repeated(0, 1);
    inline constexpr Repeated Repeated::once       = Repeated(1, 1);
    inline constexpr Repeated Repeated::zeroOrMore = Repeated(0, Repeated::infinity);
    inline constexpr Repeated Repeated::oneOrMore  = Repeated(1, Repeated::infinity);

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
