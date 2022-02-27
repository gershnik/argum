//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_ADAPTIVE_PARSER_H_INCLUDED
#define HEADER_ARGUM_ADAPTIVE_PARSER_H_INCLUDED

#include "char-constants.h"
#include "messages.h"
#include "formatting.h"
#include "validators.h"
#include "tokenizer.h"
#include "partitioner.h"
#include "command-line.h"

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
    class BasicAdaptiveParser {

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

        using ArgumentTokenizer = BasicArgumentTokenizer<Char>;
        using ValidationData = ParsingValidationData<Char>;

    public:
        template<OptionArgument Argument> using OptionHandler = typename OptionHandlerDeducer<CharType, Argument>::Type;

        using PositionalHandler = std::function<void (unsigned, StringViewType)>;

        using Settings = typename ArgumentTokenizer::Settings;
        
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
                ParsingException(format(Messages::validationError(), describe<CharType>(validator))) {
            }
        };

        class Option {
            friend class BasicAdaptiveParser;
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
        private:
            OptionNames m_names;
            Handler m_handler = []() {};
            Quantifier m_occurs = ZeroOrMoreTimes;

            StringType m_argName = Messages::defaultArgName();
            StringType m_description;
        };

        class Positional {
            friend class BasicAdaptiveParser;
        public:
            Positional(StringViewType name_) :
                m_name(std::move(name_)) {
            }

            auto handler(PositionalHandler h) & -> Positional & {
                this->m_handler = std::move(h);
                return *this;
            }
            auto handler(PositionalHandler h) && -> Positional && {
                return std::move(static_cast<Positional *>(this)->handler(h));
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
        private:
            StringType m_name;
            PositionalHandler m_handler = [](unsigned, StringViewType) {};
            Quantifier m_occurs = Once;
            StringType m_description;
        };

    public:
        BasicAdaptiveParser() = default;

        BasicAdaptiveParser(Settings settings): m_tokenizer(settings) {
        }

        auto add(Option option) -> void {

            auto & added = this->m_options.emplace_back(std::move(option));
            this->m_tokenizer.add(this->m_options.back().m_names);
            if (added.m_occurs.min() > 0)
                addValidator(OptionOccursAtLeast(added.m_names.main(), added.m_occurs.min()));
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
            auto desc = describe<CharType>(v);
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

        auto formatUsage(StringViewType progName) const -> StringType {

            constexpr auto endl = CharConstants::endl;

            StringType ret = Messages::usageStart();
            ret.append(this->formatSyntax(progName)).append(2, endl);

            auto calcRes = this->calculateDescriptionNames();
            auto & [maxNameLen, optionNames] = calcRes;
            if (maxNameLen > 21)
                maxNameLen = 21;

            if (!this->m_positionals.empty()) {
                ret.append(Messages::positionalHeader());
                for(auto & pos: this->m_positionals)
                    ret.append({endl}).append(this->formatItemDescription(pos.m_name, pos.m_description, 2, maxNameLen));
                ret.append(2, endl);
            }

            if (!this->m_options.empty()) {
                ret.append(Messages::optionsHeader());
                for(size_t idx = 0; idx < this->m_options.size(); ++idx) {
                    auto & opt = this->m_options[idx];
                    auto name = optionNames[idx];
                    ret.append({endl}).append(this->formatItemDescription(name, opt.m_description, 2, maxNameLen));
                }
                ret.append(2, endl);
            }
            return ret;
        }

        auto formatSyntax(StringViewType progName) const -> StringType {

            constexpr auto space = CharConstants::space;

            StringType ret(progName);

            for(auto & opt: this->m_options)
                ret.append({space}).append(this->formatOptionSyntax(opt));

            for (auto & pos: this->m_positionals)
                ret.append({space}).append(this->formatPositionalSyntax(pos));
            
            return ret;
        }

        auto formatOptionSyntax(const Option & opt) const -> StringType {
            constexpr auto brop = CharConstants::squareBracketOpen;
            constexpr auto brcl = CharConstants::squareBracketClose;

            StringType ret;

            if (opt.m_occurs.min() == 0)
                ret += brop;
            ret.append(opt.m_names.main()).append(this->formatOptionArgSyntax(opt));
            if (opt.m_occurs.min() == 0)
                ret += brcl;

            return ret;
        }

        auto formatPositionalSyntax(const Positional & pos) const -> StringType {
            constexpr auto space = CharConstants::space;
            constexpr auto brop = CharConstants::squareBracketOpen;
            constexpr auto brcl = CharConstants::squareBracketClose;
            constexpr auto ellipsis = CharConstants::ellipsis;

            StringType ret;

            if (pos.m_occurs.min() == 0)
                ret += brop;
            ret += pos.m_name;
            unsigned idx = 1;
            for (; idx < pos.m_occurs.min(); ++idx) {
                ret.append({space}).append(pos.m_name);
            }
            if (idx != pos.m_occurs.min()) {
                ret.append({space, brop}).append(pos.m_name);
                if (pos.m_occurs.max() != Quantifier::infinity) {
                    for (++idx; idx < pos.m_occurs.max(); ++idx)
                        ret.append({space}).append(pos.m_name);
                } else {
                    ret.append({space}).append(ellipsis);
                }
                ret += brcl;
            }
            if (pos.m_occurs.min() == 0)
                ret += brcl;

            return ret;
        }

        auto calculateDescriptionNames() const -> std::pair<size_t, std::vector<StringType>> {
            size_t maxNameLen = 0;
            std::vector<StringType> optionNames;

            std::for_each(this->m_positionals.begin(), this->m_positionals.end(), [&](auto & pos){
                if (pos.m_name.length() > maxNameLen)
                    maxNameLen = pos.m_name.length();
            });
            std::for_each(this->m_options.begin(), this->m_options.end(), [&](auto & opt){
                auto name = this->formatOptionNameForDescription(opt);
                optionNames.emplace_back(std::move(name));
                if (optionNames.back().length() > maxNameLen)
                    maxNameLen = optionNames.back().length();
            });
            return {maxNameLen, std::move(optionNames)};
        }

        auto formatOptionNameForDescription(const Option & opt) const -> StringType {
            
            StringType ret = opt.m_names.all().front();
            ret += this->formatOptionArgSyntax(opt);
            std::for_each(opt.m_names.all().begin() + 1, opt.m_names.all().end(), [&](const auto & name) {
                ret.append(Messages::listJoiner()).append(name).append(this->formatOptionArgSyntax(opt));
            });

            return ret;
        }

        auto formatItemDescription(StringViewType name, 
                                   StringViewType description, 
                                   size_t leadingSpaceCount,
                                   size_t maxNameLen) const -> StringType {
            constexpr auto space = CharConstants::space;
            constexpr auto endl = CharConstants::endl;

            StringType ret;

            ret.append(leadingSpaceCount, space).append(name);
            if (name.length() > maxNameLen) {
                ret += endl;
            } else {
                ret.append(maxNameLen - name.length() + 2, space);
            }
            ret += description;

            return Indent<CharType>{unsigned(maxNameLen + leadingSpaceCount + 2)}.apply(ret);
        }

        auto formatOptionArgSyntax(const Option & opt) const -> StringType {
            constexpr auto space = CharConstants::space;
            constexpr auto brop = CharConstants::squareBracketOpen;
            constexpr auto brcl = CharConstants::squareBracketClose;

            StringType ret;
            std::visit([&](const auto & handler) {
                using HandlerType = std::decay_t<decltype(handler)>;
                if constexpr (std::is_same_v<HandlerType, OptionHandler<OptionArgument::Optional>>) {
                    ret.append({space, brop}).append(opt.m_argName).append({brcl});
                } else if constexpr (std::is_same_v<HandlerType, OptionHandler<OptionArgument::Required>>)  {
                    ret.append({space}).append(opt.m_argName);
                }
            }, opt.m_handler);
            return ret;
        }

    private:
        class ParsingState {
        public:
            ParsingState(const BasicAdaptiveParser & owner): 
                m_owner(owner),
                m_updateCountAtLastRecalc(owner.m_updateCount - 1) {
            }

            template<ArgIterator<CharType> It>
            auto parse(It argFirst, It argLast, bool stopOnUnknown) -> std::vector<StringType> {
            
                auto ret = m_owner.m_tokenizer.tokenize(argFirst, argLast, [&](auto && token) -> typename ArgumentTokenizer::TokenResult {

                    using TokenType = std::decay_t<decltype(token)>;

                    if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::OptionToken>) {

                        resetOption(token.idx, token.usedName, token.argument);
                        return ArgumentTokenizer::Continue;

                    } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::OptionStopToken>) {

                        completeOption();
                        return ArgumentTokenizer::Continue;

                    } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::ArgumentToken>) {

                        if (!handlePositional(token.value, argFirst + token.argIdx, argLast)) {
                            if (stopOnUnknown)
                                return ArgumentTokenizer::StopBefore;
                            throw ExtraPositional(token.value);
                        }
                        return ArgumentTokenizer::Continue;

                    } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::UnknownOptionToken>) {

                        completeOption();
                        if (stopOnUnknown)
                            return ArgumentTokenizer::StopBefore;
                        throw UnrecognizedOption(token.name);

                    } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::AmbiguousOptionToken>) {

                        completeOption();
                        throw AmbiguousOption(token.name, std::move(token.possibilities));
                    } 
                });

                completeOption();

                //We could use normal validators for this but it is faster to do it manually
                validatePositionals();
                
                for(auto & [validator, desc]: m_owner.m_validators) {
                    if (!validator(m_validationData))
                        throw ValidationError(desc);
                }
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
                auto validator = OptionOccursAtMost(name, option.m_occurs.max());
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

                    if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::OptionToken>) {

                        auto & option = m_owner.m_options[token.idx];
                        if (!std::holds_alternative<OptionHandler<OptionArgument::None>>(option.m_handler)) {
                            currentOptionExpectsArgument = !token.argument;
                        } else {
                            currentOptionExpectsArgument = false;
                        }

                    } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::OptionStopToken>) {

                        currentOptionExpectsArgument = false;

                    } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::ArgumentToken>) {

                        if (!currentOptionExpectsArgument)
                            ++remainingPositionalCount;
                        else
                            currentOptionExpectsArgument = false;

                    } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::UnknownOptionToken>) {

                        currentOptionExpectsArgument = false;
                    }
                    return ArgumentTokenizer::Continue;
                });
                
                return remainingPositionalCount;
            }

            auto validatePositionals() {
                for(auto idx = (m_positionalIndex >= 0 ? unsigned(m_positionalIndex) : 0u); 
                    idx != unsigned(m_owner.m_positionals.size());
                    ++idx) {
                    
                    auto & positional = m_owner.m_positionals[unsigned(idx)];
                    auto validator = PositionalOccursAtLeast(positional.m_name, positional.m_occurs.min());
                    if (!validator(this->m_validationData))
                        throw ValidationError(validator);
                }
            }

        private:
            const BasicAdaptiveParser & m_owner;
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
        ArgumentTokenizer m_tokenizer;
        std::vector<std::pair<ValidatorFunction, StringType>> m_validators;
        size_t m_updateCount = 0;
    };

    ARGUM_DECLARE_FRIENDLY_NAMES(AdaptiveParser)

}

#endif 
