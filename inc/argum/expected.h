//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_EXPECTED_H_INCLUDED
#define HEADER_ARGUM_EXPECTED_H_INCLUDED

#include "data.h"
#include "messages.h"

#include <variant>
#include <optional>

namespace Argum {

    ARGUM_MOD_EXPORTED
    template<class Char>
    struct UnexpectedException : public BasicParsingException<Char> {
        ARGUM_IMPLEMENT_EXCEPTION(UnexpectedException, BasicParsingException<Char>)

        UnexpectedException(): 
            BasicParsingException<Char>(Messages<Char>::expectedValueNotPresent()) {
        }
    };

    ARGUM_MOD_EXPORTED
    template<class Char, class T>
    class [[nodiscard]] BasicExpected {
    public:
        using ParsingException = BasicParsingException<Char>;

    private:
        using ParsingExceptionPtr = std::unique_ptr<ParsingException>;

    public:
        BasicExpected() = default;

        BasicExpected(T value): m_impl(value) {
        }
        BasicExpected(ParsingExceptionPtr err): m_impl(std::move(err)) {
        }
        template<class Stored, class... Args>
        requires(std::is_same_v<Stored, T>)
        BasicExpected(std::in_place_type_t<Stored> inPlace, Args && ...args): 
            m_impl(inPlace, std::forward<Args>(args)...) {
        }
        template<class Stored, class... Args>
        requires(std::is_base_of_v<ParsingException, Stored>)
        BasicExpected(std::in_place_type_t<Stored>, Args && ...args): 
            m_impl(std::make_unique<Stored>(std::forward<Args>(args)...)) {
        }

        auto operator*() const & -> const T & {
            return *std::get_if<T>(&this->m_impl);
        }
        auto operator*() & -> T & {
            return *std::get_if<T>(&this->m_impl);
        }
        auto operator*() && -> T && {
            return std::move(*std::get_if<T>(&this->m_impl));
        }
        auto value() const & -> const T & {
            return std::visit([](const auto & val) -> const T & {
                if constexpr (std::is_same_v<std::decay_t<decltype(val)>, T>) {
                    return val;
                } else {
                    raise(val);
                }
            }, this->m_impl);
        }
        auto value() & -> T & {
            return std::visit([](auto & val) -> T & {
                if constexpr (std::is_same_v<std::decay_t<decltype(val)>, T>) {
                    return val;
                } else {
                    raise(val);
                }
            }, this->m_impl);
        }
        auto value() && -> T && {
            return std::visit([](auto && val) -> T && {
                if constexpr (std::is_same_v<std::decay_t<decltype(val)>, T>) {
                    return std::move(val);
                } else {
                    raise(val);
                }
            }, std::move(this->m_impl));
        }
        auto operator->() const -> const T * {
            return std::get_if<T>(&this->m_impl);
        }
        auto operator->() -> T * {
            return std::get_if<T>(&this->m_impl);
        }

        auto error() const & -> const ParsingExceptionPtr & {
            auto ppret = std::get_if<ParsingExceptionPtr>(&this->m_impl);
            if (!ppret)
                ARGUM_INVALID_ARGUMENT("BasicExpected doesn't hold an error");
            return *ppret;
        }
        auto error() & -> ParsingExceptionPtr & {
            auto ppret = std::get_if<ParsingExceptionPtr>(&this->m_impl);
            if (!ppret)
                ARGUM_INVALID_ARGUMENT("BasicExpected doesn't hold an error");
            return *ppret;
        }
        auto error() && -> ParsingExceptionPtr && {
            auto ppret = std::get_if<ParsingExceptionPtr>(&this->m_impl);
            if (!ppret)
                ARGUM_INVALID_ARGUMENT("BasicExpected doesn't hold an error");
            return std::move(*ppret);
        }

        explicit operator bool() const {
            return std::holds_alternative<T>(this->m_impl);
        }
        auto operator!() const -> bool {
            return !bool(*this);
        }
    private:
        [[noreturn]] static auto raise(const ParsingExceptionPtr & ptr) {
            if (ptr)
                ptr->raise();
            ARGUM_RAISE_EXCEPTION(UnexpectedException<Char>());
        }
    private:
        std::variant<T, ParsingExceptionPtr> m_impl;
    };

    ARGUM_MOD_EXPORTED
    template<class Char>
    class [[nodiscard]] BasicExpected<Char, void> {
    public:
        using ParsingException = BasicParsingException<Char>;

    private:
        using ParsingExceptionPtr = std::unique_ptr<ParsingException>;

    public:
        BasicExpected() = default;
        BasicExpected(ParsingExceptionPtr err): m_impl(std::move(err)) {
        }

        template<class Stored, class... Args>
        requires(std::is_base_of_v<ParsingException, Stored>)
        BasicExpected(std::in_place_type_t<Stored>, Args && ...args): 
            m_impl(std::make_unique<Stored>(std::forward<Args>(args)...)) {
        }

        auto operator*() const -> void {
        }
        auto value() const -> void {
            if (this->m_impl.has_value())
                raise(*this->m_impl);
        }

        auto error() const & -> const ParsingExceptionPtr & {
            if (!this->m_impl.has_value())
                ARGUM_INVALID_ARGUMENT("BasicExpected doesn't hold an error");
            return *this->m_impl;
        }
        auto error() & -> ParsingExceptionPtr & {
            if (!this->m_impl.has_value())
                ARGUM_INVALID_ARGUMENT("BasicExpected doesn't hold an error");
            return *this->m_impl;
        }
        auto error() && -> ParsingExceptionPtr && {
            if (!this->m_impl.has_value())
                ARGUM_INVALID_ARGUMENT("BasicExpected doesn't hold an error");
            return std::move(*this->m_impl);
        }

        explicit operator bool() const {
            return !this->m_impl.has_value();
        }
        auto operator!() const -> bool {
            return !bool(*this);
        }
    private:
        [[noreturn]] static auto raise(const ParsingExceptionPtr & ptr) {
            if (ptr)
                ptr->raise();
            ARGUM_RAISE_EXCEPTION(UnexpectedException<Char>());
        }
    private:
        std::optional<ParsingExceptionPtr> m_impl;
    };

    ARGUM_MOD_EXPORTED template<class T> using Expected = BasicExpected<char, T>;
    ARGUM_MOD_EXPORTED template<class T> using WExpected = BasicExpected<wchar_t, T>;

    #ifdef ARGUM_USE_EXPECTED
        #define ARGUM_EXPECTED(c, type) BasicExpected<c, type>
        #define ARGUM_PROPAGATE_ERROR(expr) if (auto res = (expr); !res) { return std::move(res).error(); }
        #define ARGUM_CHECK_RESULT(var, result)  if (!result) { return std::move(result).error(); }; var = *result
        #define ARGUM_THROW(type, ...) return {std::in_place_type<type> __VA_OPT__(,) __VA_ARGS__}
        #define ARGUM_VOID_SUCCESS {}
        #define ARGUM_EXPECTED_VALUE(x) (x).value()
    #else
        #define ARGUM_EXPECTED(c, x) x
        #define ARGUM_PROPAGATE_ERROR(expr) { expr; }
        #define ARGUM_CHECK_RESULT(var, result)  var = result
        #define ARGUM_THROW(type, ...) throw type(__VA_ARGS__)
        #define ARGUM_VOID_SUCCESS
        #define ARGUM_EXPECTED_VALUE(x) x

    #endif
}



#endif