//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_PARSER_H_INCLUDED
#define HEADER_ARGUM_PARSER_H_INCLUDED

#include "char-constants.h"
#include "messages.h"
#include "formatting.h"
#include "validators.h"
#include "tokenizer.h"
#include "partitioner.h"
#include "command-line.h"
#include "help-formatter.h"

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <variant>
#include <functional>
#include <algorithm>
#include <concepts>


namespace Argum {

    template<class Char>
    class BasicParser {

    public:
        using CharType = Char;
        using StringType = std::basic_string<Char>;
        using StringViewType = std::basic_string_view<Char>;
        using OptionNames = BasicOptionNames<Char>;
        using ParsingException = BasicParsingException<Char>;

    private:
        using CharConstants = Argum::CharConstants<CharType>;
        using Messages = Argum::Messages<CharType>;

        using ValidatorFunction = std::function<bool (const ParsingValidationData<CharType> &)>;


        template<class C, OptionArgument Argument> struct OptionHandlerDeducer;

        template<class C>
        struct OptionHandlerDeducer<C, OptionArgument::None> { 
            using Type = std::function<void ()>; 
        };
        template<class C>
        struct OptionHandlerDeducer<C, OptionArgument::Optional> { 
            using Type = std::function<void (std::optional<std::basic_string_view<C>>)>; 
        };
        template<class C>
        struct OptionHandlerDeducer<C, OptionArgument::Required> { 
            using Type = std::function<void (std::basic_string_view<C>)>; 
        };

        using Tokenizer = BasicTokenizer<Char>;
        using ValidationData = ParsingValidationData<Char>;

    public:
        template<OptionArgument Argument> using OptionHandler = typename OptionHandlerDeducer<CharType, Argument>::Type;

        using PositionalHandler = std::function<void (unsigned, StringViewType)>;

        using Settings = typename Tokenizer::Settings;
        
        struct UnrecognizedOption : public ParsingException {
            UnrecognizedOption(StringViewType option_): 
                ParsingException(format(Messages::unrecognizedOptionError(), option_)),
                option(option_) {
            }
            StringType option;
        };

        struct AmbiguousOption : public ParsingException {
            AmbiguousOption(StringViewType option_, std::vector<StringType> possibilities_): 
                ParsingException(format(Messages::ambiguousOptionError(), option_, 
                                        join(possibilities_.begin(), possibilities_.end(), Messages::listJoiner()))),
                option(option_),
                possibilities(std::move(possibilities_)) {
            }
            StringType option;
            std::vector<StringType> possibilities;
        };

        struct MissingOptionArgument : public ParsingException {
            MissingOptionArgument(StringViewType option_): 
                ParsingException(format(Messages::missingOptionArgumentError(), option_)),
                option(option_) {
            }
            StringType option;
        };

        struct ExtraOptionArgument : public ParsingException {
            ExtraOptionArgument(StringViewType option_): 
                ParsingException(format(Messages::extraOptionArgumentError(), option_)),
                option(option_) {
            }
            StringType option;
        };

        struct ExtraPositional : public ParsingException {
            ExtraPositional(StringViewType value_): 
                ParsingException(format(Messages::extraPositionalError(), value_)),
                value(value_) {
            }
            StringType value;
        };
        
        struct ValidationError : public ParsingException {
            ValidationError(StringViewType message):
                ParsingException(format(Messages::validationError(), message)) {
            }
            template<DescribableParserValidator<CharType> Validator>
            ValidationError(Validator validator):
                ParsingException(format(Messages::validationError(), describe(validator))) {
            }
        };

        class Option {
            friend class BasicParser;
        public:
            using Handler = std::variant<
                OptionHandler<OptionArgument::None>,
                OptionHandler<OptionArgument::Optional>,
                OptionHandler<OptionArgument::Required>
            >;
            
            Option(OptionNames names_) :
                m_names(std::move(names_)) {
            }

            template<class... Args>
            requires(std::is_constructible_v<OptionNames, Args &&...>)
            Option(Args && ...args) :
                m_names(std::forward<Args>(args)...) {
            }

            template<class H>
            auto handler(H && h) & -> Option &
            requires(std::is_invocable_v<std::decay_t<decltype(h)>> ||
                     std::is_invocable_v<std::decay_t<decltype(h)>, std::optional<StringViewType>> ||
                     std::is_invocable_v<std::decay_t<decltype(h)>, StringViewType>) {

                if constexpr (std::is_invocable_v<std::decay_t<decltype(h)>>) {
                    this->m_handler.template emplace<OptionHandler<OptionArgument::None>>(std::forward<H>(h));
                } else if constexpr (std::is_invocable_v<std::decay_t<decltype(h)>, std::optional<StringViewType>>)
                    this->m_handler.template emplace<OptionHandler<OptionArgument::Optional>>(std::forward<H>(h));
                else {
                    this->m_handler.template emplace<OptionHandler<OptionArgument::Required>>(std::forward<H>(h));
                }
                return *this;
            }

            template<class H>
            auto handler(H && h) && -> Option &&
            requires(std::is_invocable_v<std::decay_t<decltype(h)>> ||
                     std::is_invocable_v<std::decay_t<decltype(h)>, std::optional<StringViewType>> ||
                     std::is_invocable_v<std::decay_t<decltype(h)>, StringViewType>) {
                return std::move(static_cast<Option *>(this)->handler(std::forward<H>(h)));
            }

            auto occurs(Quantifier r) & -> Option & {
                this->m_occurs = r;
                return *this;
            }
            auto occurs(Quantifier r) && -> Option && {
                return std::move(static_cast<Option *>(this)->occurs(r));
            }

            auto argName(StringViewType n) & -> Option & {
                this->m_argName = n;
                return *this;
            }
            auto argName(StringViewType n) && -> Option && {
                return std::move(static_cast<Option *>(this)->argName(n));
            }

            auto help(StringViewType str) & -> Option & {
                this->m_description = str;
                return *this;
            }
            auto help(StringViewType str) && -> Option && {
                return std::move(static_cast<Option *>(this)->help(str));
            }

            auto formatSyntax() const -> StringType {
                constexpr auto brop = CharConstants::squareBracketOpen;
                constexpr auto brcl = CharConstants::squareBracketClose;

                StringType ret;

                if (m_occurs.min() == 0)
                    ret += brop;
                ret.append(m_names.main()).append(formatArgSyntax());
                if (m_occurs.min() == 0)
                    ret += brcl;

                return ret;
            }

            auto formatArgSyntax() const -> StringType {
                constexpr auto space = CharConstants::space;
                constexpr auto brop = CharConstants::squareBracketOpen;
                constexpr auto brcl = CharConstants::squareBracketClose;

                StringType ret;
                std::visit([&](const auto & handler) {
                    using HandlerType = std::decay_t<decltype(handler)>;
                    if constexpr (std::is_same_v<HandlerType, OptionHandler<OptionArgument::Optional>>) {
                        ret.append({space, brop}).append(m_argName).append({brcl});
                    } else if constexpr (std::is_same_v<HandlerType, OptionHandler<OptionArgument::Required>>)  {
                        ret.append({space}).append(m_argName);
                    }
                }, m_handler);
                return ret;
            }

            auto formatHelpName() const -> StringType {
            
                auto argSyntax = this->formatArgSyntax();
                StringType ret = m_names.all().front();
                ret += argSyntax;
                std::for_each(m_names.all().begin() + 1, m_names.all().end(), [&](const auto & name) {
                    ret.append(Messages::listJoiner()).append(name).append(argSyntax);
                });

                return ret;
            }

            auto formatHelpDescription() const -> const StringType & {
                return m_description;
            }
        private:
            OptionNames m_names;
            Handler m_handler = []() {};
            Quantifier m_occurs = zeroOrMoreTimes;

            StringType m_argName = Messages::defaultArgName();
            StringType m_description;
        };

        class Positional {
            friend class BasicParser;
        public:
            Positional(StringViewType name_) :
                m_name(std::move(name_)) {
            }

            template<class H>
            auto handler(H && h) & -> Positional & 
            requires(std::is_invocable_v<std::decay_t<decltype(h)>, unsigned, StringViewType>) {
                this->m_handler = std::forward<H>(h);
                return *this;
            }
            template<class H>
            auto handler(H && h) && -> Positional && 
            requires(std::is_invocable_v<std::decay_t<decltype(h)>, unsigned, StringViewType>) {
                return std::move(static_cast<Positional *>(this)->handler(std::forward<H>(h)));
            }

            auto occurs(Quantifier r) & -> Positional & {
                this->m_occurs = r;
                return *this;
            }
            auto occurs(Quantifier r) && -> Positional && {
                return std::move(static_cast<Positional *>(this)->occurs(r));
            }

            auto help(StringViewType str) & -> Positional &{
                this->m_description = str;
                return *this;
            }
            auto help(StringViewType str) && -> Positional && {
                return std::move(static_cast<Positional *>(this)->help(str));
            }

            auto formatSyntax() const -> StringType {
                constexpr auto space = CharConstants::space;
                constexpr auto brop = CharConstants::squareBracketOpen;
                constexpr auto brcl = CharConstants::squareBracketClose;
                constexpr auto ellipsis = CharConstants::ellipsis;

                StringType ret;

                if (m_occurs.min() == 0)
                    ret += brop;
                ret += m_name;
                unsigned idx = 1;
                for (; idx < m_occurs.min(); ++idx) {
                    ret.append({space}).append(m_name);
                }
                if (idx != m_occurs.min()) {
                    ret.append({space, brop}).append(m_name);
                    if (m_occurs.max() != Quantifier::infinity) {
                        for (++idx; idx < m_occurs.max(); ++idx)
                            ret.append({space}).append(m_name);
                    } else {
                        ret.append({space}).append(ellipsis);
                    }
                    ret += brcl;
                }
                if (m_occurs.min() == 0)
                    ret += brcl;

                return ret;
            }

            auto formatHelpName() const -> const StringType & {
                return m_name;
            }

            auto formatHelpDescription() const -> const StringType & {
                return m_description;
            }
        private:
            StringType m_name;
            PositionalHandler m_handler = [](unsigned, StringViewType) {};
            Quantifier m_occurs = once;
            StringType m_description;
        };

    public:
        BasicParser() = default;

        BasicParser(Settings settings): m_tokenizer(settings) {
        }

        auto add(Option option) -> void {

            auto & added = this->m_options.emplace_back(std::move(option));
            this->m_tokenizer.add(this->m_options.back().m_names);
            if (added.m_occurs.min() > 0)
                addValidator(optionOccursAtLeast(added.m_names.main(), added.m_occurs.min()));
            ++m_updateCount;
        }
        
        auto add(Positional positional) {
            this->m_positionals.emplace_back(std::move(positional));
            ++m_updateCount;
        }

        template<ParserValidator<CharType> Validator>
        auto addValidator(Validator v, StringViewType description) -> void {
            m_validators.emplace_back(std::move(v), description);
        }

        template<DescribableParserValidator<CharType> Validator>
        auto addValidator(Validator v) -> void  {
            auto desc = describe(v);
            m_validators.emplace_back(std::move(v), std::move(desc));
        }

        auto parse(int argc, CharType ** argv) const {
            this->parse(makeArgSpan<CharType>(argc, argv));
        }

        template<ArgRange<CharType> Args>
        auto parse(const Args & args) const {
            
            this->parse(std::begin(args), std::end(args));
        }

        template<ArgIterator<CharType> It>
        auto parse(It argFirst, It argLast) const {
            
            ParsingState parsingState(*this);

            parsingState.parse(argFirst, argLast, /*stopOnUnknown=*/false);
        }

        auto parseUntilUnknown(int argc, CharType ** argv) const -> std::vector<StringType> {
            return this->parseUntilUnknown(makeArgSpan<CharType>(argc, argv));
        }

        template<ArgRange<CharType> Args>
        auto parseUntilUnknown(const Args & args) const {
            
            return this->parseUntilUnknown(std::begin(args), std::end(args));
        }

        template<ArgIterator<CharType> It>
        auto parseUntilUnknown(It argFirst, It argLast) const -> std::vector<StringType>{
            
            ParsingState parsingState(*this);

            return parsingState.parse(argFirst, argLast, /*stopOnUnknown=*/true);
        }

        auto options() const -> const std::vector<Option> & {
            return this->m_options;
        }

        auto positionals() const -> const std::vector<Positional> & {
            return this->m_positionals;
        }

        auto formatUsage(StringViewType progName) const -> StringType {
            return HelpFormatter(*this, progName).formatUsage();
        }

        auto formatHelp(StringViewType progName) const -> StringType {
            return HelpFormatter(*this, progName).formatHelp();
        }

    private:
        class ParsingState {
        public:
            ParsingState(const BasicParser & owner): 
                m_owner(owner),
                m_updateCountAtLastRecalc(owner.m_updateCount - 1) {
            }

            template<ArgIterator<CharType> It>
            auto parse(It argFirst, It argLast, bool stopOnUnknown) -> std::vector<StringType> {
            
                auto ret = m_owner.m_tokenizer.tokenize(argFirst, argLast, [&](auto && token) -> typename Tokenizer::TokenResult {

                    using TokenType = std::decay_t<decltype(token)>;

                    if constexpr (std::is_same_v<TokenType, typename Tokenizer::OptionToken>) {

                        resetOption(token.idx, token.usedName, token.argument);
                        return Tokenizer::Continue;

                    } else if constexpr (std::is_same_v<TokenType, typename Tokenizer::OptionStopToken>) {

                        completeOption();
                        return Tokenizer::Continue;

                    } else if constexpr (std::is_same_v<TokenType, typename Tokenizer::ArgumentToken>) {

                        if (!handlePositional(token.value, argFirst + token.argIdx, argLast)) {
                            if (stopOnUnknown)
                                return Tokenizer::StopBefore;
                            throw ExtraPositional(token.value);
                        }
                        return Tokenizer::Continue;

                    } else if constexpr (std::is_same_v<TokenType, typename Tokenizer::UnknownOptionToken>) {

                        completeOption();
                        if (stopOnUnknown)
                            return Tokenizer::StopBefore;
                        throw UnrecognizedOption(token.name);

                    } else if constexpr (std::is_same_v<TokenType, typename Tokenizer::AmbiguousOptionToken>) {

                        completeOption();
                        throw AmbiguousOption(token.name, std::move(token.possibilities));
                    } 
                });

                completeOption();
                validate();
                return ret;
            }

        private:
            auto resetOption(unsigned index, StringViewType name, const std::optional<StringViewType> & argument) {
                completeOption();
                m_optionName = std::move(name);
                m_optionArgument = argument;
                m_optionIndex = int(index);
            }

            auto completeOption() {
                if (m_optionIndex < 0)
                    return;

                auto & option = m_owner.m_options[unsigned(m_optionIndex)];
                validateOptionMax(option);
                std::visit([&](const auto & handler) {
                    using HandlerType = std::decay_t<decltype(handler)>;
                    if constexpr (std::is_same_v<HandlerType, OptionHandler<OptionArgument::None>>) {
                        if (m_optionArgument)
                            throw ExtraOptionArgument(m_optionName);
                        handler();
                    } else if constexpr (std::is_same_v<HandlerType, OptionHandler<OptionArgument::Optional>>) {
                        handler(m_optionArgument);
                    } else {
                        if (!m_optionArgument)
                            throw MissingOptionArgument(m_optionName);
                        handler(*m_optionArgument);
                    }
                }, option.m_handler);
                m_optionIndex = -1;
                m_optionArgument.reset();
            }

            auto completeOptionUsingArgument(StringViewType argument) -> bool {

                if (m_optionIndex < 0)
                    return false;

                auto & option = m_owner.m_options[unsigned(m_optionIndex)];
                validateOptionMax(option);
                auto ret = std::visit([&](const auto & handler) {
                        using HandlerType = std::decay_t<decltype(handler)>;
                        if constexpr (std::is_same_v<HandlerType, OptionHandler<OptionArgument::None>>) {
                            if (m_optionArgument)
                                throw ExtraOptionArgument(m_optionName);
                            handler();
                            return false;
                        } else {
                            if (m_optionArgument) {
                                handler(*m_optionArgument);
                                return false;
                            } else {
                                handler(argument);
                                return true;
                            }
                        }
                    }, option.m_handler);
                m_optionIndex = -1;
                m_optionArgument.reset();
                return ret;
            }

            auto validateOptionMax(const Option & option) {
                auto & name = option.m_names.main();
                ++m_validationData.optionCount(name);
                auto validator = optionOccursAtMost(name, option.m_occurs.max());
                if (!validator(m_validationData)) {
                    throw ValidationError(validator);
                }
            }

            template<ArgIterator<CharType> It>
            auto handlePositional(StringViewType value, It remainingArgFirst, It argLast) -> bool {
                if (completeOptionUsingArgument(value))
                    return true;

                calculateRemainingPositionals(remainingArgFirst, argLast);
                
                const Positional * positional = nullptr;
                if (m_positionalIndex >= 0) {
                    if (unsigned(m_positionalIndex) >= m_positionalSizes.size())
                        return false;

                    auto & current = m_owner.m_positionals[unsigned(m_positionalIndex)];
                    if (m_positionalSizes[unsigned(m_positionalIndex)] > m_validationData.positionalCount(current.m_name))
                        positional = &current;

                }

                if (!positional) {
                    auto next = std::find_if(m_positionalSizes.begin() + (m_positionalIndex + 1), m_positionalSizes.end(), [](const unsigned val) {
                        return val > 0;
                    });
                    m_positionalIndex = int(next - m_positionalSizes.begin());
                    if (unsigned(m_positionalIndex) >= m_positionalSizes.size())
                        return false;
                    positional = &m_owner.m_positionals[unsigned(m_positionalIndex)];
                }
                
                auto & count = m_validationData.positionalCount(positional->m_name);
                positional->m_handler(count, value);
                ++count;
                return true;
            }

            template<ArgIterator<CharType> It>
            auto calculateRemainingPositionals(It remainingArgFirst, It argLast)  {

                if (m_updateCountAtLastRecalc == m_owner.m_updateCount)
                    return;
                
                //1. Count remaining positional arguments
                unsigned remainingPositionalCount = countRemainingPositionals(remainingArgFirst, argLast);
                
                //2. Build the partitioner for positional ranges
                Partitioner<unsigned> partitioner;
                
                auto fillStartIndex = m_positionalIndex + 1;
                if (m_positionalIndex >= 0) {
                    auto & positional = m_owner.m_positionals[unsigned(m_positionalIndex)];
                    auto count = m_validationData.positionalCount(positional.m_name);
                    if (positional.m_occurs.max() > count) {
                        remainingPositionalCount += count; //account for already processed
                        partitioner.addRange(positional.m_occurs.min(), positional.m_occurs.max());
                        --fillStartIndex;
                    }
                }
                std::for_each(m_owner.m_positionals.begin() + (m_positionalIndex + 1), m_owner.m_positionals.end(),
                                [&] (const Positional & positional) {
                    partitioner.addRange(positional.m_occurs.min(), positional.m_occurs.max());
                });

                //3. Partition the range
                unsigned maxRemainingPositionalCount = std::max(remainingPositionalCount, partitioner.minimumSequenceSize());
                auto res = partitioner.partition(maxRemainingPositionalCount);
                ARGUM_ALWAYS_ASSERT(res); //this must be true by construction

                //4. Fill in expected sizes based on regex matches
                m_positionalSizes.resize(m_owner.m_positionals.size());
                std::copy(res->begin(), res->end() - 1, m_positionalSizes.begin() + fillStartIndex);

                this->m_updateCountAtLastRecalc = m_owner.m_updateCount;
            }
            
            template<ArgIterator<CharType> It>
            auto countRemainingPositionals(It remainingArgFirst, It argLast) -> unsigned {
                
                unsigned remainingPositionalCount = 0;
                bool currentOptionExpectsArgument = false;
                m_owner.m_tokenizer.tokenize(remainingArgFirst, argLast, [&](const auto & token) {
                    using TokenType = std::decay_t<decltype(token)>;

                    if constexpr (std::is_same_v<TokenType, typename Tokenizer::OptionToken>) {

                        auto & option = m_owner.m_options[token.idx];
                        if (!std::holds_alternative<OptionHandler<OptionArgument::None>>(option.m_handler)) {
                            currentOptionExpectsArgument = !token.argument;
                        } else {
                            currentOptionExpectsArgument = false;
                        }

                    } else if constexpr (std::is_same_v<TokenType, typename Tokenizer::OptionStopToken>) {

                        currentOptionExpectsArgument = false;

                    } else if constexpr (std::is_same_v<TokenType, typename Tokenizer::ArgumentToken>) {

                        if (!currentOptionExpectsArgument)
                            ++remainingPositionalCount;
                        else
                            currentOptionExpectsArgument = false;

                    } else if constexpr (std::is_same_v<TokenType, typename Tokenizer::UnknownOptionToken>) {

                        currentOptionExpectsArgument = false;
                    }
                    return Tokenizer::Continue;
                });
                
                return remainingPositionalCount;
            }

            auto validate() {

                //We could use normal validators for this but it is faster to do it manually
                for(auto idx = (m_positionalIndex >= 0 ? unsigned(m_positionalIndex) : 0u); 
                    idx != unsigned(m_owner.m_positionals.size());
                    ++idx) {
                    
                    auto & positional = m_owner.m_positionals[unsigned(idx)];
                    auto validator = positionalOccursAtLeast(positional.m_name, positional.m_occurs.min());
                    if (!validator(m_validationData))
                        throw ValidationError(validator);
                }
                
                for(auto & [validator, desc]: m_owner.m_validators) {
                    if (!validator(m_validationData))
                        throw ValidationError(desc);
                }
            }

        private:
            const BasicParser & m_owner;
            size_t m_updateCountAtLastRecalc;

            int m_optionIndex = -1;
            StringType m_optionName;
            std::optional<StringType> m_optionArgument;

            int m_positionalIndex = -1;
            std::vector<unsigned> m_positionalSizes;
            
            ParsingValidationData<CharType> m_validationData;
        };

    private:
        std::vector<Option> m_options;
        std::vector<Positional> m_positionals;
        Tokenizer m_tokenizer;
        std::vector<std::pair<ValidatorFunction, StringType>> m_validators;
        size_t m_updateCount = 0;
    };

    ARGUM_DECLARE_FRIENDLY_NAMES(Parser)

}

#endif 