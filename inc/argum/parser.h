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

    template<class Char> class BasicParser;

    template<class T, class Char, class... Args>
    concept IsHandler = std::is_invocable_v<std::decay_t<T>, Args...> && (
                            std::is_same_v<void, decltype(std::declval<std::decay_t<T>>()(std::declval<Args>()...))> ||
                            std::is_same_v<BasicExpected<Char, void>, decltype(std::declval<std::decay_t<T>>()(std::declval<Args>()...))>);

    template<class Char, class H, class FuncType, class... Args>
    decltype(auto) adaptHandler(H && h) 
    requires(IsHandler<decltype(h), Char, Args...>) {

        using InHandlerType = std::decay_t<decltype(h)>;

        #ifdef ARGUM_USE_EXPECTED

            if constexpr (std::is_invocable_r_v<BasicExpected<Char, void>, InHandlerType, Args...>) {

                return std::forward<H>(h);

            } else {

                InHandlerType handler(std::forward<H>(h));

                #ifdef ARGUM_NO_THROW

                    return [=](Args ...args) -> BasicExpected<Char, void> {
                        handler(args...);
                        return {};
                    };

                #else

                    if constexpr (!std::is_nothrow_invocable_v<InHandlerType, Args...>) {

                        return [=](Args ...args) -> BasicExpected<Char, void> {
                            try {
                                handler(args...);
                            } catch(BasicParsingException<Char> & ex) {
                                return std::move(ex).clone();
                            }
                            return {};
                        };

                    } else {

                        return [=](Args ...args) -> BasicExpected<Char, void> {
                            handler(args...);
                            return {};
                        };
                    }
                    
                #endif
            }

        #else
            if constexpr (std::is_invocable_r_v<void, InHandlerType, Args...>) {

                return std::forward<H>(h);

            } else {

                InHandlerType handler(std::forward<H>(h));
                return [=](Args ...args) -> void {
                    handler(args...).value();
                };
            }
        #endif
    }

    template<class Char>
    class BasicOption {
        friend class BasicParser<Char>;

    public:
        using CharType = Char;
        using StringType = std::basic_string<Char>;
        using StringViewType = std::basic_string_view<Char>;
        using OptionNames = BasicOptionNames<Char>;
        using ArgumentKind = OptionArgumentKind;

    private:
        using CharConstants = Argum::CharConstants<CharType>;
        using Messages = Argum::Messages<CharType>;
        using HandlerReturnType = ARGUM_EXPECTED(CharType, void);

        using NoArgHandler = std::function<HandlerReturnType ()>; 
        using OptArgHandler = std::function<HandlerReturnType (std::optional<StringViewType>)>; 
        using ReqArgHandler = std::function<HandlerReturnType (StringViewType)>; 

        template<class HandlerType>
        static constexpr auto argumentKindOf() -> ArgumentKind {
            if constexpr (std::is_same_v<HandlerType, NoArgHandler>)
                return ArgumentKind::None;
            else if constexpr (std::is_same_v<HandlerType, OptArgHandler>) 
                return ArgumentKind::Optional;
            else if constexpr (std::is_same_v<HandlerType, ReqArgHandler>) 
                return ArgumentKind::Required;
        }

        auto canHaveArgument() const -> bool {
            return !std::holds_alternative<NoArgHandler>(m_handler);
        }

    public:
        using Handler = std::variant<
            NoArgHandler,
            OptArgHandler,
            ReqArgHandler
        >;
        
        BasicOption(OptionNames names) :
            m_names(std::move(names)) {
        }

        template<class... Args>
        requires(std::is_constructible_v<OptionNames, Args &&...>)
        BasicOption(Args && ...args) :
            m_names(std::forward<Args>(args)...) {
        }

        template<class H>
        auto handler(H && h) & -> BasicOption &
        requires(IsHandler<decltype(h), CharType> || 
                 IsHandler<decltype(h), CharType, std::optional<StringViewType>> ||
                 IsHandler<decltype(h), CharType, StringViewType>) {

            using InHandlerType = std::decay_t<decltype(h)>;

            if constexpr (std::is_invocable_v<InHandlerType>) {
                this->m_handler.template emplace<NoArgHandler>(adaptHandler<CharType, H, NoArgHandler>(std::forward<H>(h)));
            } else if constexpr (std::is_invocable_v<InHandlerType, std::optional<StringViewType>>)
                this->m_handler.template emplace<OptArgHandler>(adaptHandler<CharType, H, OptArgHandler, std::optional<StringViewType>>(std::forward<H>(h)));
            else {
                this->m_handler.template emplace<ReqArgHandler>(adaptHandler<CharType, H, ReqArgHandler, StringViewType>(std::forward<H>(h)));
            }
            return *this;
        }

        template<class H>
        auto handler(H && h) && -> BasicOption &&
        requires(IsHandler<decltype(h), CharType> || 
                 IsHandler<decltype(h), CharType, std::optional<StringViewType>> ||
                 IsHandler<decltype(h), CharType, StringViewType>) {
            return std::move(static_cast<BasicOption *>(this)->handler(std::forward<H>(h)));
        }

        auto occurs(Quantifier r) & -> BasicOption & {
            this->m_occurs = r;
            return *this;
        }
        auto occurs(Quantifier r) && -> BasicOption && {
            return std::move(static_cast<BasicOption *>(this)->occurs(r));
        }

        auto argName(StringViewType n) & -> BasicOption & {
            this->m_argName = n;
            return *this;
        }
        auto argName(StringViewType n) && -> BasicOption && {
            return std::move(static_cast<BasicOption *>(this)->argName(n));
        }

        auto help(StringViewType str) & -> BasicOption & {
            this->m_description = str;
            return *this;
        }
        auto help(StringViewType str) && -> BasicOption && {
            return std::move(static_cast<BasicOption *>(this)->help(str));
        }

        auto formatSyntax() const -> StringType {
            constexpr auto space = CharConstants::space;
            constexpr auto brop = CharConstants::squareBracketOpen;
            constexpr auto brcl = CharConstants::squareBracketClose;
            
            StringType ret;

            if (this->m_occurs.min() == 0)
                ret += brop;
            StringType nameAndArg = StringType(this->m_names.main()).append(this->formatArgSyntax());
            ret.append(nameAndArg);
            unsigned idx = 1;
            for (; idx < this->m_occurs.min(); ++idx) {
                ret.append({space}).append(nameAndArg);
            }
            if (this->m_occurs.max() != Quantifier::infinity && 
                idx < this->m_occurs.max()) {
                
                ret.append({space, brop}).append(nameAndArg);
                for (++idx; idx < this->m_occurs.max(); ++idx)
                    ret.append({space}).append(nameAndArg);
                ret += brcl;
            }
            if (this->m_occurs.min() == 0)
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
                constexpr auto argumentKind = BasicOption::template argumentKindOf<HandlerType>();
                if constexpr (argumentKind == ArgumentKind::Optional) {
                    ret.append({space, brop}).append(this->m_argName).append({brcl});
                } else if constexpr (argumentKind == ArgumentKind::Required)  {
                    ret.append({space}).append(this->m_argName);
                }
            }, this->m_handler);
            return ret;
        }

        auto formatHelpName() const -> StringType {
        
            auto argSyntax = this->formatArgSyntax();
            StringType ret = this->m_names.all().front();
            ret += argSyntax;
            std::for_each(this->m_names.all().begin() + 1, this->m_names.all().end(), [&](const auto & name) {
                ret.append(Messages::listJoiner()).append(name).append(argSyntax);
            });

            return ret;
        }

        auto formatHelpDescription() const -> const StringType & {
            return this->m_description;
        }
    private:
        OptionNames m_names;
        Handler m_handler = []() -> ARGUM_EXPECTED(CharType, void) { return ARGUM_VOID_SUCCESS; };
        Quantifier m_occurs = zeroOrMoreTimes;

        StringType m_argName = Messages::defaultArgName();
        StringType m_description;
    };

    ARGUM_DECLARE_FRIENDLY_NAMES(Option)

    template<class Char>
    class BasicPositional {
        friend class BasicParser<Char>;
    
    public:
        using CharType = Char;
        using StringType = std::basic_string<Char>;
        using StringViewType = std::basic_string_view<Char>;
        using Handler = std::function<ARGUM_EXPECTED(CharType, void) (StringViewType)>;

    private:
        using CharConstants = Argum::CharConstants<CharType>;
        using HandlerReturnType = ARGUM_EXPECTED(CharType, void);

    public:
        BasicPositional(StringViewType name) :
            m_name(std::move(name)) {
        }

        template<class H>
        auto handler(H && h) & -> BasicPositional & 
        requires(IsHandler<decltype(h), CharType, StringViewType>) {
            this->m_handler = adaptHandler<CharType, H, Handler, StringViewType>(std::forward<H>(h));
            return *this;
        }
        template<class H>
        auto handler(H && h) && -> BasicPositional && 
        requires(IsHandler<decltype(h), CharType, StringViewType>) {
            return std::move(static_cast<BasicPositional *>(this)->handler(std::forward<H>(h)));
        }

        auto occurs(Quantifier r) & -> BasicPositional & {
            this->m_occurs = r;
            return *this;
        }
        auto occurs(Quantifier r) && -> BasicPositional && {
            return std::move(static_cast<BasicPositional *>(this)->occurs(r));
        }

        auto help(StringViewType str) & -> BasicPositional &{
            this->m_description = str;
            return *this;
        }
        auto help(StringViewType str) && -> BasicPositional && {
            return std::move(static_cast<BasicPositional *>(this)->help(str));
        }

        auto formatSyntax() const -> StringType {
            constexpr auto space = CharConstants::space;
            constexpr auto brop = CharConstants::squareBracketOpen;
            constexpr auto brcl = CharConstants::squareBracketClose;
            constexpr auto ellipsis = CharConstants::ellipsis;

            StringType ret;

            if (this->m_occurs.min() == 0)
                ret += brop;
            ret += this->m_name;
            unsigned idx = 1;
            for (; idx < this->m_occurs.min(); ++idx) {
                ret.append({space}).append(this->m_name);
            }
            if (idx < this->m_occurs.max()) {
                ret.append({space, brop}).append(this->m_name);
                if (this->m_occurs.max() != Quantifier::infinity) {
                    for (++idx; idx < this->m_occurs.max(); ++idx)
                        ret.append({space}).append(this->m_name);
                } else {
                    ret.append({space}).append(ellipsis);
                }
                ret += brcl;
            }
            if (this->m_occurs.min() == 0)
                ret += brcl;

            return ret;
        }

        auto formatHelpName() const -> const StringType & {
            return this->m_name;
        }

        auto formatHelpDescription() const -> const StringType & {
            return this->m_description;
        }
    private:
        StringType m_name;
        Handler m_handler = [](StringViewType) -> ARGUM_EXPECTED(CharType, void) { return ARGUM_VOID_SUCCESS; };
        Quantifier m_occurs = once;
        StringType m_description;
    };

    ARGUM_DECLARE_FRIENDLY_NAMES(Positional)

    template<class Char>
    class BasicParser {

    public:
        using CharType = Char;
        using StringType = std::basic_string<Char>;
        using StringViewType = std::basic_string_view<Char>;
        using OptionNames = BasicOptionNames<Char>;
        using ParsingException = BasicParsingException<Char>;
        using Option = BasicOption<Char>;
        using Positional = BasicPositional<Char>;
        using Settings = typename BasicTokenizer<Char>::Settings;
        using HelpFormatter = BasicHelpFormatter<CharType>;
        using SubCommandMark = typename HelpFormatter::SubCommandMark;

    private:
        using CharConstants = Argum::CharConstants<CharType>;
        using Messages = Argum::Messages<CharType>;
        using Tokenizer = BasicTokenizer<CharType>;
        using ValidationData = BasicValidationData<CharType>;

        using ValidatorFunction = std::function<bool (const ValidationData &)>;

    public:
        struct UnrecognizedOption : public ParsingException {
            ARGUM_IMPLEMENT_EXCEPTION(UnrecognizedOption, ParsingException, Error::UnrecognizedOption)

            UnrecognizedOption(StringViewType option_): 
                ParsingException(ErrorCode, format(Messages::unrecognizedOptionError(), option_)),
                option(option_) {
            }
            StringType option;
        };

        struct AmbiguousOption : public ParsingException {
            ARGUM_IMPLEMENT_EXCEPTION(AmbiguousOption, ParsingException, Error::AmbiguousOption)

            AmbiguousOption(StringViewType option_, std::vector<StringType> possibilities_): 
                ParsingException(ErrorCode, 
                                 format(Messages::ambiguousOptionError(), option_, 
                                        join(possibilities_.begin(), possibilities_.end(), Messages::listJoiner()))),
                option(option_),
                possibilities(std::move(possibilities_)) {
            }
            StringType option;
            std::vector<StringType> possibilities;
        };

        struct MissingOptionArgument : public ParsingException {
            ARGUM_IMPLEMENT_EXCEPTION(MissingOptionArgument, ParsingException, Error::MissingOptionArgument)

            MissingOptionArgument(StringViewType option_): 
                ParsingException(ErrorCode, format(Messages::missingOptionArgumentError(), option_)),
                option(option_) {
            }
            StringType option;
        };

        struct ExtraOptionArgument : public ParsingException {
            ARGUM_IMPLEMENT_EXCEPTION(ExtraOptionArgument, ParsingException, Error::ExtraOptionArgument)

            ExtraOptionArgument(StringViewType option_): 
                ParsingException(ErrorCode, format(Messages::extraOptionArgumentError(), option_)),
                option(option_) {
            }
            StringType option;
        };

        struct ExtraPositional : public ParsingException {
            ARGUM_IMPLEMENT_EXCEPTION(ExtraPositional, ParsingException, Error::ExtraPositional)

            ExtraPositional(StringViewType value_): 
                ParsingException(ErrorCode, format(Messages::extraPositionalError(), value_)),
                value(value_) {
            }
            StringType value;
        };
        
        struct ValidationError : public ParsingException {
            ARGUM_IMPLEMENT_EXCEPTION(ValidationError, ParsingException, Error::ValidationError)

            ValidationError(StringViewType message):
                ParsingException(ErrorCode, format(Messages::validationError(), message)) {
            }
            template<DescribableParserValidator<CharType> Validator>
            ValidationError(Validator validator):
                ParsingException(ErrorCode, format(Messages::validationError(), describe(validator))) {
            }
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
            //for expected number of positionals this is faster than maintaining a map
            for(auto & existing: this->m_positionals) {
                if (existing.m_name == positional.m_name)
                    ARGUM_INVALID_ARGUMENT("duplicate positional name");
            }
            this->m_positionals.emplace_back(std::move(positional));
            ++m_updateCount;
        }

        auto addSubCommand(Positional positional) {
            this->add(std::move(positional));
            this->m_subCommandMark = {this->m_positionals.size() - 1,  this->m_options.size()};
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

        auto parse(int argc, CharType ** argv) const -> ARGUM_EXPECTED(CharType, void) {
            return this->parse(makeArgSpan<CharType>(argc, argv));
        }

        template<ArgRange<CharType> Args>
        auto parse(const Args & args) const -> ARGUM_EXPECTED(CharType, void) {
            
            return this->parse(std::begin(args), std::end(args));
        }

        template<ArgIterator<CharType> It>
        auto parse(It argFirst, It argLast) const -> ARGUM_EXPECTED(CharType, void) {
            
            using ReturnType = ARGUM_EXPECTED(CharType, void);
            ParsingState parsingState(*this);

            return ReturnType(parsingState.parse(argFirst, argLast, /*stopOnUnknown=*/false));
        }

        auto parseUntilUnknown(int argc, CharType ** argv) const -> ARGUM_EXPECTED(CharType, std::vector<StringType>) {
            return this->parseUntilUnknown(makeArgSpan<CharType>(argc, argv));
        }

        template<ArgRange<CharType> Args>
        auto parseUntilUnknown(const Args & args) const -> ARGUM_EXPECTED(CharType, std::vector<StringType>) {
            
            return this->parseUntilUnknown(std::begin(args), std::end(args));
        }

        template<ArgIterator<CharType> It>
        auto parseUntilUnknown(It argFirst, It argLast) const -> ARGUM_EXPECTED(CharType, std::vector<StringType>) {
            
            ParsingState parsingState(*this);

            return parsingState.parse(argFirst, argLast, /*stopOnUnknown=*/true);
        }

        auto options() const -> const std::vector<Option> & {
            return this->m_options;
        }

        auto positionals() const -> const std::vector<Positional> & {
            return this->m_positionals;
        }

        auto subCommandMark() const -> SubCommandMark {
            return this->m_subCommandMark;
        }

        auto formatUsage(StringViewType progName, std::optional<StringType> subCommand = std::nullopt) const -> StringType {
            return HelpFormatter(*this, progName).formatUsage(subCommand);
        }

        auto formatHelp(StringViewType progName, std::optional<StringType> subCommand = std::nullopt) const -> StringType {
            HelpFormatter formatter(*this, progName);
            StringType ret = formatter.formatUsage(subCommand);
            ret.append(2, CharConstants::endl);
            ret.append(formatter.formatHelp(subCommand));
            return ret;
        }

    private:
        class ParsingState {
        public:
            ParsingState(const BasicParser & owner): 
                m_owner(owner),
                m_updateCountAtLastRecalc(owner.m_updateCount - 1) {
            }

            template<ArgIterator<CharType> It>
            auto parse(It argFirst, It argLast, bool stopOnUnknown) -> ARGUM_EXPECTED(CharType, std::vector<StringType>) {
            
                ARGUM_CHECK_RESULT(auto ret, m_owner.m_tokenizer.tokenize(argFirst, argLast, [&](auto && token) -> ARGUM_EXPECTED(CharType, typename Tokenizer::TokenResult) {

                    using TokenType = std::decay_t<decltype(token)>;

                    if constexpr (std::is_same_v<TokenType, typename Tokenizer::OptionToken>) {

                        ARGUM_PROPAGATE_ERROR(resetOption(token.idx, token.usedName, token.argument));
                        return Tokenizer::Continue;

                    } else if constexpr (std::is_same_v<TokenType, typename Tokenizer::OptionStopToken>) {

                        ARGUM_PROPAGATE_ERROR(completeOption());
                        return Tokenizer::Continue;

                    } else if constexpr (std::is_same_v<TokenType, typename Tokenizer::ArgumentToken>) {

                        ARGUM_CHECK_RESULT(auto result, handlePositional(token.value, argFirst + token.argIdx, argLast));
                        if (!result) {
                            if (stopOnUnknown)
                                return Tokenizer::StopBefore;
                            ARGUM_THROW(ExtraPositional, token.value);
                        }
                        return Tokenizer::Continue;

                    } else if constexpr (std::is_same_v<TokenType, typename Tokenizer::UnknownOptionToken>) {

                        ARGUM_PROPAGATE_ERROR(completeOption());
                        if (stopOnUnknown)
                            return Tokenizer::StopBefore;
                        ARGUM_THROW(UnrecognizedOption, token.name);

                    } else if constexpr (std::is_same_v<TokenType, typename Tokenizer::AmbiguousOptionToken>) {

                        ARGUM_PROPAGATE_ERROR(completeOption());
                        ARGUM_THROW(AmbiguousOption, token.name, std::move(token.possibilities));
                    } 
                }));
                ARGUM_PROPAGATE_ERROR(completeOption());
                ARGUM_PROPAGATE_ERROR(validate());
                return ret;
            }

        private:
            auto resetOption(unsigned index, StringViewType name, const std::optional<StringViewType> & argument) -> ARGUM_EXPECTED(CharType, void) {
                ARGUM_PROPAGATE_ERROR(completeOption());
                m_optionName = std::move(name);
                m_optionArgument = argument;
                m_optionIndex = int(index);
                return ARGUM_VOID_SUCCESS;
            }

            auto completeOption() -> ARGUM_EXPECTED(CharType, void) {
                if (m_optionIndex < 0)
                    return ARGUM_VOID_SUCCESS;

                auto & option = m_owner.m_options[unsigned(m_optionIndex)];
                ARGUM_PROPAGATE_ERROR(validateOptionMax(option));
                ARGUM_PROPAGATE_ERROR(std::visit([&](const auto & handler) -> ARGUM_EXPECTED(CharType, void) {
                    using HandlerType = std::decay_t<decltype(handler)>;
                    constexpr auto argumentKind = Option::template argumentKindOf<HandlerType>();
                    if constexpr (argumentKind == OptionArgumentKind::None) {
                        if (m_optionArgument)
                            ARGUM_THROW(ExtraOptionArgument, m_optionName);
                        ARGUM_PROPAGATE_ERROR(handler());
                    } else if constexpr (argumentKind == OptionArgumentKind::Optional) {
                        ARGUM_PROPAGATE_ERROR(handler(m_optionArgument));
                    } else {
                        if (!m_optionArgument)
                            ARGUM_THROW(MissingOptionArgument, m_optionName);
                        ARGUM_PROPAGATE_ERROR(handler(*m_optionArgument));
                    }
                    return ARGUM_VOID_SUCCESS;
                }, option.m_handler));
                m_optionIndex = -1;
                m_optionArgument.reset();
                return ARGUM_VOID_SUCCESS;
            }

            auto completeOptionUsingArgument(StringViewType argument) -> ARGUM_EXPECTED(CharType, bool) {

                if (m_optionIndex < 0)
                    return false;

                auto & option = m_owner.m_options[unsigned(m_optionIndex)];
                ARGUM_PROPAGATE_ERROR(validateOptionMax(option));
                ARGUM_CHECK_RESULT(auto ret, std::visit([&](const auto & handler) -> ARGUM_EXPECTED(CharType, bool) {
                        using HandlerType = std::decay_t<decltype(handler)>;
                        constexpr auto argumentKind = Option::template argumentKindOf<HandlerType>();
                        if constexpr (argumentKind == OptionArgumentKind::None) {
                            if (m_optionArgument)
                                ARGUM_THROW(ExtraOptionArgument, m_optionName);
                            ARGUM_PROPAGATE_ERROR(handler());
                            return false;
                        } else {
                            if (m_optionArgument) {
                                ARGUM_PROPAGATE_ERROR(handler(*m_optionArgument));
                                return false;
                            } else {
                                ARGUM_PROPAGATE_ERROR(handler(argument));
                                return true;
                            }
                        }
                    }, option.m_handler));
                m_optionIndex = -1;
                m_optionArgument.reset();
                return ret;
            }

            auto validateOptionMax(const Option & option) -> ARGUM_EXPECTED(CharType, void) {
                auto & name = option.m_names.main();
                ++m_validationData.optionCount(name);
                auto validator = optionOccursAtMost(name, option.m_occurs.max());
                if (!validator(m_validationData)) {
                    ARGUM_THROW(ValidationError, validator);
                }
                return ARGUM_VOID_SUCCESS;
            }

            template<ArgIterator<CharType> It>
            auto handlePositional(StringViewType value, It remainingArgFirst, It argLast) -> ARGUM_EXPECTED(CharType, bool) {
                ARGUM_CHECK_RESULT(auto completed, completeOptionUsingArgument(value));
                if (completed)
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
                ARGUM_PROPAGATE_ERROR(positional->m_handler(value));
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
                (void)m_owner.m_tokenizer.tokenize(remainingArgFirst, argLast, [&](const auto & token) noexcept {
                    using TokenType = std::decay_t<decltype(token)>;

                    if constexpr (std::is_same_v<TokenType, typename Tokenizer::OptionToken>) {

                        auto & option = m_owner.m_options[token.idx];
                        if (option.canHaveArgument()) {
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

            auto validate() -> ARGUM_EXPECTED(CharType, void) {

                //We could use normal validators for this but it is faster to do it manually
                for(auto idx = (m_positionalIndex >= 0 ? unsigned(m_positionalIndex) : 0u); 
                    idx != unsigned(m_owner.m_positionals.size());
                    ++idx) {
                    
                    auto & positional = m_owner.m_positionals[unsigned(idx)];
                    auto validator = positionalOccursAtLeast(positional.m_name, positional.m_occurs.min());
                    if (!validator(m_validationData))
                        ARGUM_THROW(ValidationError, validator);
                }
                
                for(auto & [validator, desc]: m_owner.m_validators) {
                    if (!validator(m_validationData))
                        ARGUM_THROW(ValidationError, desc);
                }

                return ARGUM_VOID_SUCCESS;
            }

        private:
            const BasicParser & m_owner;
            size_t m_updateCountAtLastRecalc;

            int m_optionIndex = -1;
            StringType m_optionName;
            std::optional<StringType> m_optionArgument;

            int m_positionalIndex = -1;
            std::vector<unsigned> m_positionalSizes;
            
            ValidationData m_validationData;
        };

    private:
        std::vector<Option> m_options;
        std::vector<Positional> m_positionals;
        Tokenizer m_tokenizer;
        std::vector<std::pair<ValidatorFunction, StringType>> m_validators;
        size_t m_updateCount = 0;
        SubCommandMark m_subCommandMark;
    };

    ARGUM_DECLARE_FRIENDLY_NAMES(Parser)

}

#endif 
