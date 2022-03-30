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
    template<class Char, class T>
    class [[nodiscard]] BasicExpected {
    private:
        using ValueType = std::conditional_t<std::is_same_v<T, void>, std::monostate, T>;
    public:
        using ParsingException = BasicParsingException<Char>;
        using ParsingExceptionPtr = std::shared_ptr<ParsingException>;

        using ConstLValueReference = std::add_lvalue_reference_t<const T>;
        using LValueReference = std::add_lvalue_reference_t<T>;
        using RValueReference = std::add_rvalue_reference_t<T>;
    public:
        BasicExpected() = default;

        template<class Arg>
        requires(std::is_convertible_v<Arg &&, T> && !std::is_same_v<T, void>)
        BasicExpected(Arg && value): m_impl(std::in_place_type_t<T>(), std::forward<Arg>(value)) {
        }
        BasicExpected(ParsingExceptionPtr err): m_impl(BasicExpected::validate(err)) {
        }
        template<class Stored, class... Args>
        requires(std::is_same_v<Stored, T>)
        BasicExpected(std::in_place_type_t<Stored> inPlace, Args && ...args): 
            m_impl(inPlace, std::forward<Args>(args)...) {
        }
        template<class Stored, class... Args>
        requires(std::is_base_of_v<ParsingException, Stored>)
        BasicExpected(std::in_place_type_t<Stored>, Args && ...args): 
            m_impl(std::make_shared<Stored>(std::forward<Args>(args)...)) {
        }

        auto operator*() const & -> ConstLValueReference {
            if constexpr (!std::is_same_v<T, void>)
                return *std::get_if<T>(&this->m_impl);
        }
        auto operator*() & -> LValueReference {
            if constexpr (!std::is_same_v<T, void>)
                return *std::get_if<T>(&this->m_impl);
        }
        auto operator*() && -> RValueReference {
            if constexpr (!std::is_same_v<T, void>)
                return std::move(*std::get_if<T>(&this->m_impl));
        }
        auto value() const & -> ConstLValueReference {
            return std::visit([](const auto & val) -> ConstLValueReference {
                if constexpr (std::is_same_v<std::decay_t<decltype(val)>, ValueType>) {
                    if constexpr (!std::is_same_v<T, void>)
                        return val;
                } else {
                    BasicExpected::raise(val);
                }
            }, this->m_impl);
        }
        auto value() & -> LValueReference {
            return std::visit([](auto & val) -> LValueReference {
                if constexpr (std::is_same_v<std::decay_t<decltype(val)>, ValueType>) {
                    if constexpr (!std::is_same_v<T, void>)
                        return val;
                } else {
                    BasicExpected::raise(val);
                }
            }, this->m_impl);
        }
        auto value() && -> RValueReference {
            return std::visit([](auto && val) -> RValueReference {
                if constexpr (std::is_same_v<std::decay_t<decltype(val)>, ValueType>) {
                    if constexpr (!std::is_same_v<T, void>)
                        return std::move(val);
                } else {
                    BasicExpected::raise(val);
                }
            }, std::move(this->m_impl));
        }

        template<class X = T>
        requires(!std::is_same_v<X, void>)
        auto operator->() const -> const T * {
            return std::get_if<T>(&this->m_impl);
        }
        template<class X = T>
        requires(!std::is_same_v<X, void>)
        auto operator->() -> T * {
            return std::get_if<T>(&this->m_impl);
        }

        auto error() const -> ParsingExceptionPtr {
            return std::visit([](const auto & val) -> ParsingExceptionPtr {
                if constexpr (std::is_same_v<std::decay_t<decltype(val)>, ParsingExceptionPtr>) {
                    return val;
                } else {
                    return ParsingExceptionPtr();
                }
            }, this->m_impl);
        }

        explicit operator bool() const {
            return std::holds_alternative<ValueType>(this->m_impl);
        }
        auto operator!() const -> bool {
            return !bool(*this);
        }
    private:
        static auto validate(ParsingExceptionPtr & ptr) -> ParsingExceptionPtr && {
            if (!ptr)
                ARGUM_INVALID_ARGUMENT("error must be non-null");
            return std::move(ptr);
        }
        [[noreturn]] static auto raise(const ParsingExceptionPtr & ptr) {
            if (ptr)
                ptr->raise();
            ARGUM_ALWAYS_ASSERT(!"accessing value of moved-out BasicExpected");
        }
    private:
        std::variant<ValueType, ParsingExceptionPtr> m_impl;
    };

    ARGUM_MOD_EXPORTED template<class T> using Expected = BasicExpected<char, T>;
    ARGUM_MOD_EXPORTED template<class T> using WExpected = BasicExpected<wchar_t, T>;

    #ifdef ARGUM_USE_EXPECTED
        #define ARGUM_EXPECTED(c, type) BasicExpected<c, type>
        #define ARGUM_PROPAGATE_ERROR(expr) if (auto err = (expr).error()) { return err; }
        #define ARGUM_CHECK_RESULT(var, result)  if (auto err = (result).error()) { return err; }; var = *(result)
        #define ARGUM_THROW(type, ...) return {std::in_place_type<type> __VA_OPT__(,) __VA_ARGS__}
        #define ARGUM_VOID_SUCCESS {}
    #else
        #define ARGUM_EXPECTED(c, x) x
        #define ARGUM_PROPAGATE_ERROR(expr) do { expr; } while(false)
        #define ARGUM_CHECK_RESULT(var, result)  var = result
        #define ARGUM_THROW(type, ...) throw type(__VA_ARGS__)
        #define ARGUM_VOID_SUCCESS
    #endif
}



#endif
