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
            { describe<Char>(val) } -> std::same_as<std::basic_string<Char>>;
        };
    
    template<class Char, class First, class... Rest>
    constexpr bool CompatibleParserValidators = ParserValidator<First, Char> && (ParserValidator<Rest, Char> && ...);
    
    template<class Char, class First, class... Rest>
    constexpr bool CompatibleDescribableParserValidators = DescribableParserValidator<First, Char> && (DescribableParserValidator<Rest, Char> && ...);
    
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
        
        template<class Char, DescribableParserValidator<Char> ImplD>
        friend auto describe(const NotValidator<ImplD> & val)  {
        
            return format(Messages<Char>::negationDesc(), val.m_impl);
        }
    private:
        Impl m_impl;
    };

    //MARK: - OppositeOf
    
    template<AnyParserValidator Validator>
    auto operator!(const Validator & val) {
        return NotValidator<std::decay_t<Validator>>(std::forward<Validator>(val));
    }

    template<AnyParserValidator Validator>
    auto oppositeOf(Validator && val) {
        return !std::forward<Validator>(val);
    }

    //MARK: - CombinedValidator

    enum class ValidatorCombination : int {
        And,
        Or,
        Xor,
        NXor
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

            return std::apply([&data](const Args & ...args) -> bool {
                if constexpr (Comb == ValidatorCombination::And)
                    return (args(data) && ...);
                else if constexpr (Comb == ValidatorCombination::Or)
                    return (args(data) || ...);
                else if constexpr (Comb == ValidatorCombination::Xor)
                    return bool((args(data) ^ ...));
                else if constexpr (Comb == ValidatorCombination::NXor)
                    return !bool((args(data) ^ ...));
            }, this->m_items);
        }

        auto operator!() const {

            return std::apply([](const Args & ...args) {
                if constexpr (Comb == ValidatorCombination::And)
                    return CombinedValidator<ValidatorCombination::Or, decltype(!args)...>(!args...);
                else if constexpr (Comb == ValidatorCombination::Or)
                    return CombinedValidator<ValidatorCombination::And, decltype(!args)...>(!args...);
                else if constexpr (Comb == ValidatorCombination::Xor)
                    return CombinedValidator<ValidatorCombination::NXor, Args...>(args...);
                else if constexpr (Comb == ValidatorCombination::NXor)
                    return CombinedValidator<ValidatorCombination::Xor, Args...>(args...);
            }, this->m_items);
        }
        
        auto items() const -> const TupleType & {
            return m_items;
        }

    private:
        TupleType m_items;
    };

    template<class Char, ValidatorCombination Comb, DescribableParserValidator<Char>... Args>
    auto describe(const CombinedValidator<Comb, Args...> & val)  {

        std::basic_string<Char> str;
        
        auto formatEntry = [&str](const auto & arg) {
            std::basic_string<Char> entry;
            entry += CharConstants<Char>::endl;
            Indent<Char> indent(indent.defaultValue);
            entry.append(indent.apply(describe<Char>(arg)));
            str.append(indent.apply(entry));
        };

        if constexpr (Comb == ValidatorCombination::And)
            str = Messages<Char>::allMustBeTrue();
        else if constexpr (Comb == ValidatorCombination::Or)
            str = Messages<Char>::oneOrMoreMustBeTrue();
        else if constexpr (Comb == ValidatorCombination::Xor)
            str = Messages<Char>::onlyOneMustBeTrue();
        else if constexpr (Comb == ValidatorCombination::NXor)
            str = Messages<Char>::allOrNoneMustBeTrue();
        std::apply([formatEntry] (const Args & ...args) {
            (formatEntry(args), ...);
        }, val.items());

        return str;
    }

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

    template<ValidatorCombination Comb, class V2, class... Args1>
    requires(AnyCompatibleParserValidators<Args1..., std::decay_t<V2>>)
    auto combine(const CombinedValidator<Comb, Args1...> & v1, V2 && v2)  {
        using R2 = std::decay_t<V2>;
        return CombinedValidator<Comb, Args1..., R2>(
            std::tuple_cat(v1.items(), std::tuple<R2>(std::forward<V2>(v2)))
        );
    }

    template<ValidatorCombination Comb, class... Args1, class... Args2>
    requires(AnyCompatibleParserValidators<Args1..., Args2...>)
    auto combine(const CombinedValidator<Comb, Args1...> & v1,
                 const CombinedValidator<Comb, Args2...> & v2)  {
        return CombinedValidator<Comb, Args1..., Args2...>(
            std::tuple_cat(v1.items(), v2.items())
        );
    }

    //MARK: - AllOf

    template<class V1, class V2>
    requires(AnyCompatibleParserValidators<std::decay_t<V1>, std::decay_t<V2>>)
    auto operator&&(V1 && v1, V2 && v2)  {
        return combine<ValidatorCombination::And>(std::forward<V1>(v1), std::forward<V2>(v2));
    }

    template<class First, class... Rest>
    requires(AnyCompatibleParserValidators<std::decay_t<First>, std::decay_t<Rest>...>)
    auto allOf(First && first, Rest && ...rest)  {
        return (std::forward<First>(first) &&  ... && std::forward<Rest>(rest));
    }

    //MARK: - AnyOf

    template<class V1, class V2>
    requires(AnyCompatibleParserValidators<std::decay_t<V1>, std::decay_t<V2>>)
    auto operator||(V1 && v1, V2 && v2)  {
        return combine<ValidatorCombination::Or>(std::forward<V1>(v1), std::forward<V2>(v2));
    }

    template<class First, class... Rest>
    requires(AnyCompatibleParserValidators<std::decay_t<First>, std::decay_t<Rest>...>)
    auto anyOf(First && first, Rest && ...rest)  {
        return (std::forward<First>(first) || ... || std::forward<Rest>(rest));
    }

    //MARK: - OnlyOneOf

    template<class V1, class V2>
    requires(AnyCompatibleParserValidators<std::decay_t<V1>, std::decay_t<V2>>)
    auto onlyOneOf(V1 && v1, V2 && v2)  {
        return combine<ValidatorCombination::Xor>(std::forward<V1>(v1), std::forward<V2>(v2));
    }

    template<class First, class... Rest>
    requires(AnyCompatibleParserValidators<std::decay_t<First>, std::decay_t<Rest>...>)
    auto onlyOneOf(First && first, Rest && ...rest)  {
        return combine<ValidatorCombination::Xor>(std::forward<First>(first), 
                                                  onlyOneOf(std::forward<Rest>(rest)...));
    }

    //MARK: - AllOrNoneOf

    template<class V1, class V2>
    requires(AnyCompatibleParserValidators<std::decay_t<V1>, std::decay_t<V2>>)
    auto allOrNoneOf(V1 && v1, V2 && v2)  {
        return combine<ValidatorCombination::NXor>(std::forward<V1>(v1), std::forward<V2>(v2));
    }

    template<class First, class... Rest>
    requires(AnyCompatibleParserValidators<std::decay_t<First>, std::decay_t<Rest>...>)
    auto allOrNoneOf(First && first, Rest && ...rest)  {
        return combine<ValidatorCombination::NXor>(std::forward<First>(first), 
                                                   allOrNoneOf(std::forward<Rest>(rest)...));
    }

    //MARK: - Occurence Validators

    template<class Char, bool IsOption, class Comp>
    requires(std::is_same_v<Comp, std::greater_equal<unsigned>> ||
             std::is_same_v<Comp, std::less_equal<unsigned>> || 
             std::is_same_v<Comp, std::greater<unsigned>> ||
             std::is_same_v<Comp, std::greater<unsigned>> ||
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

        template<class ResChar>
        friend auto describe(const ItemOccurs & val)  {

            using Messages = Messages<ResChar>;
            constexpr auto inf = std::numeric_limits<unsigned>::max();

            auto typeName = IsOption ? Messages::option() : Messages::positionalArg();
            if constexpr (std::is_same_v<Comp, std::greater_equal<unsigned>>) switch(val.m_count) {
                break; case 0:   return format(Messages::itemUnrestricted(), typeName, val.m_name);
                break; case 1:   return format(Messages::itemRequired(), typeName, val.m_name);
                break; default:  return format(Messages::itemOccursAtLeast(), typeName, val.m_name, val.m_count);
            }
            else if constexpr (std::is_same_v<Comp, std::less_equal<unsigned>>) switch(val.m_count) {
                break; case 0:   return format(Messages::itemMustNotBePresent(), typeName, val.m_name);
                break; case 1:   return format(Messages::itemRequired(), typeName, val.m_name);
                break; default:  return format(Messages::itemOccursAtMost(), typeName, val.m_name, val.m_count);
                break; case inf: return format(Messages::itemUnrestricted(), typeName, val.m_name);
            }
            else if constexpr (std::is_same_v<Comp, std::greater<unsigned>>) switch(val.m_count) {
                break; case 0:   return format(Messages::itemRequired(), typeName, val.m_name);
                break; default:  return format(Messages::itemOccursMoreThan(), typeName, val.m_name, val.m_count);
            }
            else if constexpr (std::is_same_v<Comp, std::less<unsigned>>) switch(val.m_count) {
                break; case 0:   return format(Messages::itemMustNotBePresent(), typeName, val.m_name);
                break; case 1:   return format(Messages::itemMustNotBePresent(), typeName, val.m_name);
                break; default:  return format(Messages::itemOccursLessThan(), typeName, val.m_name, val.m_count);
                break; case inf: return format(Messages::itemUnrestricted(), typeName, val.m_name);
            }
            else if constexpr (std::is_same_v<Comp, std::equal_to<unsigned>>) switch(val.m_count) {
                break; case 0:   return format(Messages::itemMustNotBePresent(), typeName, val.m_name);
                break; case 1:   return format(Messages::itemRequired(), typeName, val.m_name);
                break; default:  return format(Messages::itemOccursExactly(), typeName, val.m_name, val.m_count);
            }
            else if constexpr (std::is_same_v<Comp, std::not_equal_to<unsigned>>) switch(val.m_count) {
                break; case 0:   return format(Messages::itemRequired(), typeName, val.m_name);
                break; default:  return format(Messages::itemDoesNotOccursExactly(), typeName, val.m_name, val.m_count);
                break; case inf: return format(Messages::itemUnrestricted(), typeName, val.m_name);
            }
        }
    private:
        std::basic_string<Char> m_name;
        unsigned m_count;
    };


    template<StringLike S>
    auto OptionRequired(S name) {
        return ItemOccurs<CharTypeOf<S>, true, std::greater<unsigned>>(name, 0);
    }
    template<StringLike S>
    auto OptionAbsent(S name) {
        return ItemOccurs<CharTypeOf<S>, true, std::equal_to<unsigned>>(name, 0);
    }
    template<StringLike S>
    auto OptionOccursAtLeast(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, true, std::greater_equal<unsigned>>(name, count);
    }
    template<StringLike S>
    auto OptionOccursAtMost(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, true, std::less_equal<unsigned>>(name, count);
    }
    template<StringLike S>
    auto OptionOccursMoreThan(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, true, std::greater<unsigned>>(name, count);
    }
    template<StringLike S>
    auto OptionOccursLessThan(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, true, std::less<unsigned>>(name, count);
    }

    template<StringLike S>
    auto PositionalRequired(S name) {
        return ItemOccurs<CharTypeOf<S>, false, std::greater<unsigned>>(name, 0);
    }
    template<StringLike S>
    auto PositionalAbsent(S name) {
        return ItemOccurs<CharTypeOf<S>, false, std::equal_to<unsigned>>(name, 0);
    }
    template<StringLike S>
    auto PositionalOccursAtLeast(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, false, std::greater_equal<unsigned>>(name, count);
    }
    template<StringLike S>
    auto PositionalOccursAtMost(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, false, std::less_equal<unsigned>>(name, count);
    }
    template<StringLike S>
    auto PositionalOccursMoreThan(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, false, std::greater<unsigned>>(name, count);
    }
    template<StringLike S>
    auto PositionalOccursLessThan(S name, unsigned count) {
        return ItemOccurs<CharTypeOf<S>, false, std::less<unsigned>>(name, count);
    }
    
}

#endif 