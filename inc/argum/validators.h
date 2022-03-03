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

    template<ValidatorCombination Comb, class First, class... Rest>
    requires(AnyCompatibleParserValidators<First, Rest...>)
    class CombinedValidator;

    template<ValidatorCombination Comb, class First, class Second> 
    requires(AnyCompatibleParserValidators<First, Second>)
    class CombinedValidator<Comb, First, Second> {
    public:
        template<class CFirst, class CSecond>
        requires(std::is_constructible_v<First, CFirst> && std::is_constructible_v<Second, CSecond>)
        CombinedValidator(CFirst && first, CSecond && second): 
            m_first(std::forward<CFirst>(first)),
            m_second(std::forward<CSecond>(second)) {

        }

        template<class Char>
        requires(CompatibleParserValidators<Char, First, Second>)
        auto operator()(const ParsingValidationData<Char> & data) const {
            if constexpr (Comb == ValidatorCombination::And) {
                return this->m_first(data) && this->m_second(data);
            } else if constexpr (Comb == ValidatorCombination::Or) {
                return this->m_first(data) || this->m_second(data);
            } else if constexpr (Comb == ValidatorCombination::OnlyOne) {
                unsigned counter = 0;
                return this->evalOneOrNone(data, counter) && counter == 1;
            } else if constexpr (Comb == ValidatorCombination::OneOrNone) {
                unsigned counter = 0;
                return this->evalOneOrNone(data, counter);
            } else if constexpr (Comb == ValidatorCombination::AllOrNone) {
                unsigned counter = 0;
                unsigned totalCounter = 0;
                return this->evalAllOrNone(data, counter, totalCounter);
            }
        }

        auto operator!() const {
            
            if constexpr (Comb == ValidatorCombination::And)
                return CombinedValidator<ValidatorCombination::Or,
                                         std::decay_t<decltype(!std::declval<First>())>,
                                         std::decay_t<decltype(!std::declval<Second>())>>(!this->m_first, !this->m_second);
            else if constexpr (Comb == ValidatorCombination::Or)
                return CombinedValidator<ValidatorCombination::And,
                                         std::decay_t<decltype(!std::declval<First>())>,
                                         std::decay_t<decltype(!std::declval<Second>())>>(!this->m_first, !this->m_second);
            else 
                return NotValidator<CombinedValidator>(*this);
        }
    
        template<class Char>
        requires(CompatibleParserValidators<Char, First, Second>)
        auto evalOneOrNone(const ParsingValidationData<Char> & data, unsigned & counter) const {
            
            if (this->m_first(data)) {
                if (++counter > 1)
                    return false;
            }
            
            if (this->m_second(data)) {
                if (++counter > 1)
                    return false;
            }

            return true;
        }

        template<class Char>
        requires(CompatibleParserValidators<Char, First, Second>)
        auto evalAllOrNone(const ParsingValidationData<Char> & data, unsigned & counter, unsigned totalCounter) const {
            
            ++totalCounter;
            counter += this->m_first(data);

            if (counter != 0 && counter != totalCounter)
                return false;

            
            ++totalCounter;
            counter += this->m_second(data);

            return counter == 0 || counter == totalCounter;
        }
    private:
        std::decay_t<First> m_first;
        std::decay_t<Second> m_second;
    };

    template<ValidatorCombination Comb, class First, class... Rest>
    requires(AnyCompatibleParserValidators<First, Rest...>)
    class CombinedValidator {
    public:
        template<class CFirst, class... CRest>
        requires(std::is_constructible_v<First, CFirst> && (std::is_constructible_v<Rest, CRest> && ...))
        CombinedValidator(CFirst && first, CRest && ...rest): 
            m_first(std::forward<CFirst>(first)),
            m_rest(std::forward<CRest>(rest)...) {
        }

        CombinedValidator(First && first, CombinedValidator<Comb, Rest...> && rest): 
            m_first(std::move(first)),
            m_rest(std::move(rest)) {
        }

        template<class Char>
        requires(CompatibleParserValidators<Char, First, Rest...>)
        auto operator()(const ParsingValidationData<Char> & data) const {
            if constexpr (Comb == ValidatorCombination::And) {
                return this->m_first(data) && this->m_rest(data);
            } else if constexpr (Comb == ValidatorCombination::Or) {
                return this->m_first(data) || this->m_rest(data);
            } else if constexpr (Comb == ValidatorCombination::OnlyOne) {
                unsigned counter = 0;
                return this->evalOneOrNone(data, counter) && counter == 1;
            } else if constexpr (Comb == ValidatorCombination::OneOrNone) {
                unsigned counter = 0;
                return this->evalOneOrNone(data, counter);
            } else if constexpr (Comb == ValidatorCombination::AllOrNone) {
                unsigned counter = 0;
                unsigned totalCounter = 0;
                return this->evalAllOrNone(data, counter, totalCounter);
            }
        }

        auto operator!() const {
            
            if constexpr (Comb == ValidatorCombination::And)
                return CombinedValidator<ValidatorCombination::Or, 
                                         std::decay_t<decltype(!std::declval<First>())>,
                                         std::decay_t<decltype(!std::declval<Rest>())>...>(!this->m_first, !this->m_rest);
            else if constexpr (Comb == ValidatorCombination::Or)
                return CombinedValidator<ValidatorCombination::And, 
                                         std::decay_t<decltype(!std::declval<First>())>,
                                         std::decay_t<decltype(!std::declval<Rest>())>...>(!this->m_first, !this->m_rest);
            else 
                return NotValidator<CombinedValidator>(*this);
        }

        template<class Char>
        requires(CompatibleParserValidators<Char, First, Rest...>)
        auto evalOneOrNone(const ParsingValidationData<Char> & data, unsigned & counter) const {
            
            if (this->m_first(data)) {
                if (++counter > 1)
                    return false;
            }
            
            return this->m_rest.evalOneOrNone(data, counter);
        }

        template<class Char>
        requires(CompatibleParserValidators<Char, First, Rest...>)
        auto evalAllOrNone(const ParsingValidationData<Char> & data, unsigned & counter, unsigned totalCounter) const {
            
            ++totalCounter;
            counter += this->m_first(data);
            
            if (counter != 0 && counter != totalCounter)
                return false;
            
            return this->m_rest.evalAllOrNone(data, counter, totalCounter);
        }

    private:
        First m_first;
        CombinedValidator<Comb, Rest...> m_rest;
    };

    
    //MARK: - AllOf

    ARGUM_MOD_EXPORTED
    template<class V1, class V2>
    requires(AnyCompatibleParserValidators<std::decay_t<V1>, std::decay_t<V2>>)
    auto operator&&(V1 && v1, V2 && v2)  {
        return CombinedValidator<ValidatorCombination::And, std::decay_t<V1>, std::decay_t<V2>>(std::forward<V1>(v1), std::forward<V2>(v2));
    }

    ARGUM_MOD_EXPORTED
    template<class First, class... Rest>
    requires(AnyCompatibleParserValidators<std::decay_t<First>, std::decay_t<Rest>...>)
    auto allOf(First && first, Rest && ...rest)  {
        return CombinedValidator<ValidatorCombination::And, std::decay_t<First>, std::decay_t<Rest>...>(std::forward<First>(first), std::forward<Rest>(rest)...);
    }

    //MARK: - AnyOf

    ARGUM_MOD_EXPORTED
    template<class V1, class V2>
    requires(AnyCompatibleParserValidators<std::decay_t<V1>, std::decay_t<V2>>)
    auto operator||(V1 && v1, V2 && v2)  {
        return CombinedValidator<ValidatorCombination::Or, std::decay_t<V1>, std::decay_t<V2>>(std::forward<V1>(v1), std::forward<V2>(v2));
    }

    ARGUM_MOD_EXPORTED
    template<class First, class... Rest>
    requires(AnyCompatibleParserValidators<std::decay_t<First>, std::decay_t<Rest>...>)
    auto anyOf(First && first, Rest && ...rest)  {
        return CombinedValidator<ValidatorCombination::Or, std::decay_t<First>, std::decay_t<Rest>...>(std::forward<First>(first), std::forward<Rest>(rest)...);
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
