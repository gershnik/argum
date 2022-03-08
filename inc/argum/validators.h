//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_VALIDATORS_H_INCLUDED
#define HEADER_ARGUM_VALIDATORS_H_INCLUDED

#include "messages.h"
#include "formatting.h"
#include "flat-map.h"

#include <concepts>
#include <tuple>
#include <map>
#include <limits>

namespace Argum {

    ARGUM_MOD_EXPORTED
    template<class Char>
    class ParsingValidationData {
    public:
        auto optionCount(std::basic_string_view<Char> name) const -> unsigned {
            auto it = this->m_optionCounts.find(name);
            if (it == this->m_optionCounts.end())
                return 0;
            return it->value();
        }
        auto optionCount(std::basic_string_view<Char> name) -> unsigned & {
            return this->m_optionCounts[name];
        }
        auto positionalCount(std::basic_string_view<Char> name) const -> unsigned {
            auto it = this->m_positionalCounts.find(name);
            if (it == this->m_positionalCounts.end())
                return 0;
            return it->value();
        }
        auto positionalCount(std::basic_string_view<Char> name) -> unsigned & {
            return this->m_positionalCounts[name];
        }
    private:
        FlatMap<std::basic_string<Char>, unsigned> m_optionCounts;
        FlatMap<std::basic_string<Char>, unsigned> m_positionalCounts;
    };

    template<class T, class Char>
    concept ParserValidator = std::is_invocable_r_v<bool, T, const ParsingValidationData<Char>>;
    
    template<class T, class Char>
    concept DescribableParserValidator = ParserValidator<T, Char> && 
        requires(const T & val) {
            { describe(val) } -> std::same_as<std::basic_string<Char>>;
        };
    
    template<class Char, class First, class... Rest>
    constexpr bool CompatibleParserValidators = ParserValidator<First, Char> && (ParserValidator<Rest, Char> && ...);
    
    template<class T>
    concept AnyParserValidator =
        ParserValidator<T, char> ||
        ParserValidator<T, wchar_t>;
   
    
    template<class First, class... Rest>
    constexpr bool AnyCompatibleParserValidators =
        CompatibleParserValidators<char, First, Rest...> ||
        CompatibleParserValidators<wchar_t, First, Rest...>;

    //MARK: - NotValidator

    template<AnyParserValidator Impl>
    class NotValidator {
    public:
        template<class X>
        NotValidator(X && x) requires(std::is_convertible_v<X &&, Impl>): m_impl(std::forward<X>(x)) {
        }
        
        template<class Char>
        requires(ParserValidator<Impl, Char>)
        auto operator()(const ParsingValidationData<Char> & data) const -> bool {
            return !this->m_impl(data);
        }
        
        auto operator!() const -> const Impl & {
            return m_impl;
        }
    private:
        Impl m_impl;
    };

    //MARK: - OppositeOf
    
    ARGUM_MOD_EXPORTED
    template<AnyParserValidator Validator>
    auto operator!(const Validator & val) {
        return NotValidator<std::decay_t<Validator>>(val);
    }

    ARGUM_MOD_EXPORTED
    template<AnyParserValidator Validator>
    auto oppositeOf(Validator && val) {
        return !std::forward<Validator>(val);
    }

    //MARK: - CombinedValidator

    enum class ValidatorCombination : int {
        And,
        Or,
        OnlyOne,
        OneOrNone,
        AllOrNone
    };

    template<ValidatorCombination Comb, class... Args>
    requires(sizeof...(Args) > 1 && AnyCompatibleParserValidators<Args...>)
    class CombinedValidator {
    public:
        using TupleType = std::tuple<Args...>;
    public:
        CombinedValidator(std::tuple<Args...> && args): m_items(std::move(args)) {
        }
        template<class... CArgs>
        requires(std::is_constructible_v<TupleType, CArgs && ...>)
        CombinedValidator(CArgs && ...args): m_items(std::forward<CArgs>(args)...) {  
        }

        template<class Char>
        requires(CompatibleParserValidators<Char, Args...>)
        auto operator()(const ParsingValidationData<Char> & data) const -> bool {
            if constexpr (Comb == ValidatorCombination::And) {
                return std::apply([&data](const Args & ...args) -> bool { return (args(data) && ...); }, this->m_items);
            } else if constexpr (Comb == ValidatorCombination::Or) {
                return std::apply([&data](const Args & ...args) -> bool { return (args(data) || ...); }, this->m_items);
            } else if constexpr (Comb == ValidatorCombination::OnlyOne) {
                unsigned counter = 0;
                return this->evalOneOrNone<sizeof...(Args)>(data, counter) && counter == 1;
            } else if constexpr (Comb == ValidatorCombination::OneOrNone) {
                unsigned counter = 0;
                return this->evalOneOrNone<sizeof...(Args)>(data, counter);
            } else if constexpr (Comb == ValidatorCombination::AllOrNone) {
                unsigned counter = 0;
                return this->evalAllOrNone<sizeof...(Args)>(data, counter);
            }
        }

        auto operator!() const {
            
            if constexpr (Comb == ValidatorCombination::And || Comb == ValidatorCombination::Or)
                return std::apply([](const Args & ...args) {
                    if constexpr (Comb == ValidatorCombination::And)
                        return CombinedValidator<ValidatorCombination::Or, decltype(!args)...>(!args...);
                    else if constexpr (Comb == ValidatorCombination::Or)
                        return CombinedValidator<ValidatorCombination::And, decltype(!args)...>(!args...);
                }, this->m_items);
            else 
                return NotValidator<CombinedValidator>(*this);
        }

        auto items() const -> const TupleType & {
            return m_items;
        }

    private:

        template<size_t N, class Char>
        requires(CompatibleParserValidators<Char, Args...>)
        auto evalOneOrNone(const ParsingValidationData<Char> & data, unsigned & counter) const {
            
            constexpr auto idx = sizeof...(Args) - N;

            if (std::get<idx>(this->m_items)(data)) {
                if (++counter > 1)
                    return false;
            }

            if constexpr (N > 1) {
                return this->evalOneOrNone<N - 1>(data, counter);
            } else {
                return true;
            }
        }

        template<size_t N, class Char>
        requires(CompatibleParserValidators<Char, Args...>)
        auto evalAllOrNone(const ParsingValidationData<Char> & data, unsigned & counter) const {

            constexpr auto idx = sizeof...(Args) - N;
            
            counter += std::get<idx>(this->m_items)(data);
            
            if (counter != 0 && counter != idx + 1)
                return false;

            if constexpr (N > 1) {
                return this->evalAllOrNone<N - 1>(data, counter);
            } else {
                return true;
            }
        }

    private:
        TupleType m_items;
    };

    template<ValidatorCombination Comb, class V1, class V2>
    requires(AnyCompatibleParserValidators<std::decay_t<V1>, std::decay_t<V2>>)
    auto combine(V1 && v1, V2 && v2)  {
        using R1 = std::decay_t<V1>;
        using R2 = std::decay_t<V2>;
        return CombinedValidator<Comb, R1, R2>(std::forward<V1>(v1), std::forward<V2>(v2));
    }

    template<ValidatorCombination Comb, class V1, class... Args2>
    requires(AnyCompatibleParserValidators<std::decay_t<V1>, Args2...>)
    auto combine(V1 && v1, const CombinedValidator<Comb, Args2...> & v2)  {
        using R1 = std::decay_t<V1>;
        return CombinedValidator<Comb, R1, Args2...>(
            std::tuple_cat(std::tuple<R1>(std::forward<V1>(v1)), v2.items())
        );
    }
    
    template<ValidatorCombination Comb, class V1, class... Args2>
    requires(AnyCompatibleParserValidators<std::decay_t<V1>, Args2...>)
    auto combine(V1 && v1, CombinedValidator<Comb, Args2...> && v2)  {
        using R1 = std::decay_t<V1>;
        return CombinedValidator<Comb, R1, Args2...>(
            std::tuple_cat(std::tuple<R1>(std::forward<V1>(v1)), std::move(v2.items()))
        );
    }

    template<ValidatorCombination Comb, class V2, class... Args1>
    requires(AnyCompatibleParserValidators<Args1..., std::decay_t<V2>>)
    auto combine(const CombinedValidator<Comb, Args1...> & v1, V2 && v2)  {
        using R2 = std::decay_t<V2>;
        return CombinedValidator<Comb, Args1..., R2>(
            std::tuple_cat(v1.items(), std::tuple<R2>(std::forward<V2>(v2)))
        );
    }
    
    template<ValidatorCombination Comb, class V2, class... Args1>
    requires(AnyCompatibleParserValidators<Args1..., std::decay_t<V2>>)
    auto combine(CombinedValidator<Comb, Args1...> && v1, V2 && v2)  {
        using R2 = std::decay_t<V2>;
        return CombinedValidator<Comb, Args1..., R2>(
            std::tuple_cat(std::move(v1.items()), std::tuple<R2>(std::forward<V2>(v2)))
        );
    }

    template<ValidatorCombination Comb, class... Args1, class... Args2>
    requires(AnyCompatibleParserValidators<Args1..., Args2...>)
    auto combine(const CombinedValidator<Comb, Args1...> & v1, const CombinedValidator<Comb, Args2...> & v2)  {
        return CombinedValidator<Comb, Args1..., Args2...>(
            std::tuple_cat(v1.items(), v2.items())
        );
    }
    
    template<ValidatorCombination Comb, class... Args1, class... Args2>
    requires(AnyCompatibleParserValidators<Args1..., Args2...>)
    auto combine(CombinedValidator<Comb, Args1...> && v1, const CombinedValidator<Comb, Args2...> & v2)  {
        return CombinedValidator<Comb, Args1..., Args2...>(
            std::tuple_cat(std::move(v1.items()), v2.items())
        );
    }
    
    template<ValidatorCombination Comb, class... Args1, class... Args2>
    requires(AnyCompatibleParserValidators<Args1..., Args2...>)
    auto combine(const CombinedValidator<Comb, Args1...> & v1, CombinedValidator<Comb, Args2...> && v2)  {
        return CombinedValidator<Comb, Args1..., Args2...>(
            std::tuple_cat(v1.items(), std::move(v2.items()))
        );
    }
    
    template<ValidatorCombination Comb, class... Args1, class... Args2>
    requires(AnyCompatibleParserValidators<Args1..., Args2...>)
    auto combine(CombinedValidator<Comb, Args1...> && v1, CombinedValidator<Comb, Args2...> && v2)  {
        return CombinedValidator<Comb, Args1..., Args2...>(
            std::tuple_cat(std::move(v1.items()), std::move(v2.items()))
        );
    }

    
    //MARK: - AllOf

    ARGUM_MOD_EXPORTED
    template<class V1, class V2>
    requires(AnyCompatibleParserValidators<std::decay_t<V1>, std::decay_t<V2>>)
    auto operator&&(V1 && v1, V2 && v2)  {
        return combine<ValidatorCombination::And>(std::forward<V1>(v1), std::forward<V2>(v2));
    }

    ARGUM_MOD_EXPORTED
    template<class First, class... Rest>
    requires(AnyCompatibleParserValidators<std::decay_t<First>, std::decay_t<Rest>...>)
    auto allOf(First && first, Rest && ...rest)  {
        return (std::forward<First>(first) &&  ... && std::forward<Rest>(rest));
    }

    //MARK: - AnyOf

    ARGUM_MOD_EXPORTED
    template<class V1, class V2>
    requires(AnyCompatibleParserValidators<std::decay_t<V1>, std::decay_t<V2>>)
    auto operator||(V1 && v1, V2 && v2)  {
        return combine<ValidatorCombination::Or>(std::forward<V1>(v1), std::forward<V2>(v2));
    }

    ARGUM_MOD_EXPORTED
    template<class First, class... Rest>
    requires(AnyCompatibleParserValidators<std::decay_t<First>, std::decay_t<Rest>...>)
    auto anyOf(First && first, Rest && ...rest)  {
        return (std::forward<First>(first) || ... || std::forward<Rest>(rest));
    }

    //MARK: - NoneOf

    ARGUM_MOD_EXPORTED
    template<class First, class... Rest>
    requires(AnyCompatibleParserValidators<std::decay_t<First>, std::decay_t<Rest>...>)
    auto noneOf(First && first, Rest && ...rest)  {
        return allOf(!std::forward<First>(first), !std::forward<Rest>(rest)...);
    }

    //MARK: - OnlyOneOf

    ARGUM_MOD_EXPORTED
    template<class First, class... Rest>
    requires(AnyCompatibleParserValidators<std::decay_t<First>, std::decay_t<Rest>...>)
    auto onlyOneOf(First && first, Rest && ...rest)  {
        
        return CombinedValidator<ValidatorCombination::OnlyOne, std::decay_t<First>, std::decay_t<Rest>...>(std::forward<First>(first), std::forward<Rest>(rest)...);
    }

    //MARK: - OneOrNoneOf

    ARGUM_MOD_EXPORTED
    template<class First, class... Rest>
    requires(AnyCompatibleParserValidators<std::decay_t<First>, std::decay_t<Rest>...>)
    auto oneOrNoneOf(First && first, Rest && ...rest)  {
        return CombinedValidator<ValidatorCombination::OneOrNone, std::decay_t<First>, std::decay_t<Rest>...>(std::forward<First>(first), std::forward<Rest>(rest)...);
    }

    //MARK: - AllOrNoneOf

    ARGUM_MOD_EXPORTED
    template<class First, class... Rest>
    requires(AnyCompatibleParserValidators<std::decay_t<First>, std::decay_t<Rest>...>)
    auto allOrNoneOf(First && first, Rest && ...rest)  {
        return CombinedValidator<ValidatorCombination::AllOrNone, std::decay_t<First>, std::decay_t<Rest>...>(std::forward<First>(first), std::forward<Rest>(rest)...);
    }

    //MARK: - Occurence Validators

    template<class Char, bool IsOption, class Comp>
    requires(std::is_same_v<Comp, std::greater_equal<unsigned>> ||
             std::is_same_v<Comp, std::less_equal<unsigned>> || 
             std::is_same_v<Comp, std::greater<unsigned>> ||
             std::is_same_v<Comp, std::less<unsigned>> ||
             std::is_same_v<Comp, std::equal_to<unsigned>> ||
             std::is_same_v<Comp, std::not_equal_to<unsigned>>)
    class ItemOccurs {
    public:
        ItemOccurs(std::basic_string_view<Char> name, unsigned count) : m_name(name), m_count(count) {}

        auto operator()(const ParsingValidationData<Char> & data) const -> bool {
            if constexpr (IsOption)
                return Comp()(data.optionCount(this->m_name), this->m_count);
            else
                return Comp()(data.positionalCount(this->m_name), this->m_count);
        }
        
        auto operator!() const {
            if constexpr (std::is_same_v<Comp, std::greater_equal<unsigned>>)
                return ItemOccurs<Char, IsOption, std::less<unsigned>>(this->m_name, this->m_count);
            else if constexpr (std::is_same_v<Comp, std::less_equal<unsigned>>)
                return ItemOccurs<Char, IsOption, std::greater<unsigned>>(this->m_name, this->m_count);
            else if constexpr (std::is_same_v<Comp, std::greater<unsigned>>)
                return ItemOccurs<Char, IsOption, std::less_equal<unsigned>>(this->m_name, this->m_count);
            else if constexpr (std::is_same_v<Comp, std::less<unsigned>>)
                return ItemOccurs<Char, IsOption, std::greater_equal<unsigned>>(this->m_name, this->m_count);
            else if constexpr (std::is_same_v<Comp, std::equal_to<unsigned>>)
                return ItemOccurs<Char, IsOption, std::not_equal_to<unsigned>>(this->m_name, this->m_count);
            else if constexpr (std::is_same_v<Comp, std::not_equal_to<unsigned>>)
                return ItemOccurs<Char, IsOption, std::equal_to<unsigned>>(this->m_name, this->m_count);
        }

        friend auto describe(const ItemOccurs & val)  {

            using Messages = Messages<Char>;
            
            auto typeName = IsOption ? Messages::option() : Messages::positionalArg();
            if constexpr (std::is_same_v<Comp, std::greater_equal<unsigned>>) {
                return format(Messages::itemMustBePresentGE(val.m_count), typeName, val.m_name, val.m_count);
            } else if constexpr (std::is_same_v<Comp, std::less_equal<unsigned>>) {
                return format(Messages::itemMustBePresentLE(val.m_count), typeName, val.m_name, val.m_count);
            } else if constexpr (std::is_same_v<Comp, std::greater<unsigned>>) {
                return format(Messages::itemMustBePresentG(val.m_count), typeName, val.m_name, val.m_count);
            } else if constexpr (std::is_same_v<Comp, std::less<unsigned>>) {
                return format(Messages::itemMustBePresentL(val.m_count), typeName, val.m_name, val.m_count);
            } else if constexpr (std::is_same_v<Comp, std::equal_to<unsigned>>) {
                return format(Messages::itemMustBePresentEQ(val.m_count), typeName, val.m_name, val.m_count);
            } else if constexpr (std::is_same_v<Comp, std::not_equal_to<unsigned>>) {
                return format(Messages::itemMustBePresentNEQ(val.m_count), typeName, val.m_name, val.m_count);
            } 
        }
    private:
        std::basic_string<Char> m_name;
        unsigned m_count;
    };


    ARGUM_MOD_EXPORTED template<StringLike S>
    auto optionPresent(S name) {
        return ItemOccurs<CharTypeOf<S>, true, std::greater<unsigned>>(name, 0);
    }
    ARGUM_MOD_EXPORTED template<StringLike S>
    auto optionAbsent(S name) {
        return ItemOccurs<CharTypeOf<S>, true, std::equal_to<unsigned>>(name, 0);
    }
    ARGUM_MOD_EXPORTED template<StringLike S>
    auto optionOccursAtLeast(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, true, std::greater_equal<unsigned>>(name, count);
    }
    ARGUM_MOD_EXPORTED template<StringLike S>
    auto optionOccursAtMost(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, true, std::less_equal<unsigned>>(name, count);
    }
    ARGUM_MOD_EXPORTED template<StringLike S>
    auto optionOccursMoreThan(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, true, std::greater<unsigned>>(name, count);
    }
    ARGUM_MOD_EXPORTED template<StringLike S>
    auto optionOccursLessThan(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, true, std::less<unsigned>>(name, count);
    }
    ARGUM_MOD_EXPORTED template<StringLike S>
    auto optionOccursExactly(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, true, std::equal_to<unsigned>>(name, count);
    }
    ARGUM_MOD_EXPORTED template<StringLike S>
    auto optionDoesntOccurExactly(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, true, std::not_equal_to<unsigned>>(name, count);
    }

    ARGUM_MOD_EXPORTED template<StringLike S>
    auto positionalPresent(S name) {
        return ItemOccurs<CharTypeOf<S>, false, std::greater<unsigned>>(name, 0);
    }
    ARGUM_MOD_EXPORTED template<StringLike S>
    auto positionalAbsent(S name) {
        return ItemOccurs<CharTypeOf<S>, false, std::equal_to<unsigned>>(name, 0);
    }
    ARGUM_MOD_EXPORTED template<StringLike S>
    auto positionalOccursAtLeast(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, false, std::greater_equal<unsigned>>(name, count);
    }
    ARGUM_MOD_EXPORTED template<StringLike S>
    auto positionalOccursAtMost(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, false, std::less_equal<unsigned>>(name, count);
    }
    ARGUM_MOD_EXPORTED template<StringLike S>
    auto positionalOccursMoreThan(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, false, std::greater<unsigned>>(name, count);
    }
    ARGUM_MOD_EXPORTED template<StringLike S>
    auto positionalOccursLessThan(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, false, std::less<unsigned>>(name, count);
    }
    ARGUM_MOD_EXPORTED template<StringLike S>
    auto positionalOccursExactly(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, false, std::equal_to<unsigned>>(name, count);
    }
    ARGUM_MOD_EXPORTED template<StringLike S>
    auto positionalDoesntOccurExactly(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, false, std::not_equal_to<unsigned>>(name, count);
    }
    
}

#endif 
