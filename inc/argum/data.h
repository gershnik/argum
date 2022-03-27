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
#include <exception>
#include <limits>
#include <memory>

namespace Argum {

    ARGUM_MOD_EXPORTED
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

    enum class OptionArgumentKind  {
        None,
        Optional,
        Required
    };

    ARGUM_MOD_EXPORTED class Quantifier {
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

    ARGUM_MOD_EXPORTED inline constexpr Quantifier zeroOrOneTime  (0, 1);
    ARGUM_MOD_EXPORTED inline constexpr Quantifier neverOrOnce    = zeroOrOneTime;
    ARGUM_MOD_EXPORTED inline constexpr Quantifier oneTime        (1, 1);
    ARGUM_MOD_EXPORTED inline constexpr Quantifier once           = oneTime;
    ARGUM_MOD_EXPORTED inline constexpr Quantifier zeroOrMoreTimes(0, Quantifier::infinity);
    ARGUM_MOD_EXPORTED inline constexpr Quantifier oneOrMoreTimes (1, Quantifier::infinity);
    ARGUM_MOD_EXPORTED inline constexpr Quantifier onceOrMore     = oneOrMoreTimes;

    ARGUM_MOD_EXPORTED enum class Error {
        UnrecognizedOption = 1,
        AmbiguousOption,
        MissingOptionArgument,
        ExtraOptionArgument,
        ExtraPositional,
        ValidationError,
        ResponseFileError,

        Last = ResponseFileError,
        UserError = int(Last) + 100
    };

    ARGUM_MOD_EXPORTED
    template<class Char>
    class BasicParsingException : public std::exception {
    public:
        auto message() const noexcept -> std::basic_string_view<Char> {
            return m_message;
        }

        auto code() const noexcept -> Error {
            return m_code;
        }

        template<class Derived>
        requires(std::is_base_of_v<BasicParsingException, Derived>)
        auto as() const noexcept -> const Derived * {
            if (this->m_code == Derived::ErrorCode)
                return static_cast<const Derived *>(this);
            return nullptr;
        }

        template<class Derived>
        requires(std::is_base_of_v<BasicParsingException, Derived>)
        auto as() noexcept -> Derived * {
            return const_cast<Derived *>(const_cast<const BasicParsingException *>(this)->as<Derived>());
        }

        auto what() const noexcept -> const char * override {
            return this->m_what.c_str();
        }

        virtual auto clone() const & -> std::shared_ptr<BasicParsingException> = 0;
        virtual auto clone() & -> std::shared_ptr<BasicParsingException> = 0;
        virtual auto clone() && -> std::shared_ptr<BasicParsingException> = 0;
        [[noreturn]] virtual auto raise() const -> void = 0;
        
    protected:
        BasicParsingException(Error code, std::basic_string<Char> message) : 
            m_what(toString<char>(message)),
            m_message(std::move(message)),
            m_code(code) {
        }

    private:
        std::string m_what;
        std::basic_string<Char> m_message;
        Error m_code;
    };

    ARGUM_MOD_EXPORTED
    template<>
    class BasicParsingException<char> : public std::exception {
    public:
        auto message() const noexcept -> std::string_view {
            return m_what;
        }

        auto code() const noexcept -> Error {
            return m_code;
        }

        template<class Derived>
        requires(std::is_base_of_v<BasicParsingException, Derived>)
        auto as() const noexcept -> const Derived * {
            if (this->m_code == Derived::ErrorCode)
                return static_cast<const Derived *>(this);
            return nullptr;
        }

        template<class Derived>
        requires(std::is_base_of_v<BasicParsingException, Derived>)
        auto as() noexcept -> Derived * {
            return const_cast<Derived *>(const_cast<const BasicParsingException *>(this)->as<Derived>());
        }

        auto what() const noexcept -> const char * override {
            return this->m_what.c_str();
        }

        virtual auto clone() const & -> std::shared_ptr<BasicParsingException> = 0;
        virtual auto clone() & -> std::shared_ptr<BasicParsingException> = 0;
        virtual auto clone() && -> std::shared_ptr<BasicParsingException> = 0;
        [[noreturn]] virtual auto raise() const -> void = 0;

    protected:
        BasicParsingException(Error code, std::string message) : 
            m_what(std::move(message)),
            m_code(code) {
        }
    private:
        std::string m_what;
        Error m_code;
    };
    
    ARGUM_DECLARE_FRIENDLY_NAMES(ParsingException)

    #define ARGUM_IMPLEMENT_EXCEPTION(type, base, code) \
        static constexpr Error ErrorCode = code; \
        auto clone() const & -> std::shared_ptr<base> override { \
            return std::make_shared<type>(*this); \
        } \
        auto clone() & -> std::shared_ptr<base> override { \
            return std::make_shared<type>(*this); \
        } \
        auto clone() && -> std::shared_ptr<base> override { \
            return std::make_shared<type>(std::move(*this)); \
        } \
        [[noreturn]] auto raise() const -> void override { ARGUM_RAISE_EXCEPTION(*this); }

}


#endif
