#ifndef HEADER_MARGP_ADAPTIVE_PARSER_H_INCLUDED
#define HEADER_MARGP_ADAPTIVE_PARSER_H_INCLUDED

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


namespace MArgP {

    template<class Char>
    class BasicAdaptiveParser {

    public:
        using CharType = Char;
        using StringType = std::basic_string<Char>;
        using StringViewType = std::basic_string_view<Char>;
        using OptionNames = BasicOptionNames<Char>;
        using ParsingException = BasicParsingException<Char>;

    private:
        using CharConstants = MArgP::CharConstants<CharType>;
        using Messages = MArgP::Messages<CharType>;

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
                ParsingException(describe<CharType>(validator)) {
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
                names(std::move(names_)) {
            }

            template<class... Args>
            requires(std::is_constructible_v<OptionNames, Args &&...>)
            Option(Args && ...args) :
                names(std::forward<Args>(args)...) {
            }

            template<class H>
            auto setHandler(H && handler_) -> Option &
            requires(std::is_invocable_v<std::decay_t<decltype(handler_)>> ||
                     std::is_invocable_v<std::decay_t<decltype(handler_)>, std::optional<StringViewType>> ||
                     std::is_invocable_v<std::decay_t<decltype(handler_)>, StringViewType>) {

                if constexpr (std::is_invocable_v<std::decay_t<decltype(handler_)>>) {
                    this->handler.template emplace<OptionHandler<OptionArgument::None>>(std::forward<H>(handler_));
                } else if constexpr (std::is_invocable_v<std::decay_t<decltype(handler_)>, std::optional<StringViewType>>)
                    this->handler.template emplace<OptionHandler<OptionArgument::Optional>>(std::forward<H>(handler_));
                else {
                    this->handler.template emplace<OptionHandler<OptionArgument::Required>>(std::forward<H>(handler_));
                }
                return *this;
            }

            auto setRepeated(Repeated r) -> Option &{
                this->repeated = r;
                return *this;
            }
            auto setArgName(StringViewType n) -> Option &{
                this->argName = n;
                return *this;
            }
            auto setDescription(StringViewType d)  -> Option &{
                this->description = d;
                return *this;
            }
        private:
            OptionNames names;
            Handler handler = []() {};
            Repeated repeated = Repeated::zeroOrMore;

            StringType argName = Messages::defaultArgName();
            StringType description;
        };

        class Positional {
            friend class BasicAdaptiveParser;
        public:
            Positional(StringViewType name_) :
                name(std::move(name_)) {
            }

            auto setHandler(PositionalHandler handler_) -> Positional & {
                this->handler = std::move(handler_);
                return *this;
            }

            auto set(Repeated r) -> Positional &{
                this->repeated = r;
                return *this;
            }
            auto setDescription(StringViewType d)  -> Positional &{
                this->description = d;
                return *this;
            }
        private:
            StringType name;
            PositionalHandler handler = [](unsigned, StringViewType) {};
            Repeated repeated = Repeated::once;
            StringType description;
        };

    public:
        auto allowShortLongs(bool value) {
             this->m_tokenizer.allowShortLongs(value);
        }

        auto add(Option option) -> void {

            auto & added = this->m_options.emplace_back(std::move(option));
            this->m_tokenizer.add(this->m_options.back().names);
            if (added.repeated.min() > 0)
                addValidator(OptionOccursAtLeast(added.names.main(), added.repeated.min()));
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

            parsingState.parse(argFirst, argLast);
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
                    ret.append({endl}).append(this->formatItemDescription(pos.name, pos.description, 2, maxNameLen));
                ret.append(2, endl);
            }

            if (!this->m_options.empty()) {
                ret.append(Messages::optionsHeader());
                for(size_t idx = 0; idx < this->m_options.size(); ++idx) {
                    auto & opt = this->m_options[idx];
                    auto name = optionNames[idx];
                    ret.append({endl}).append(this->formatItemDescription(name, opt.description, 2, maxNameLen));
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
            constexpr auto brop = CharConstants::optBracketOpen;
            constexpr auto brcl = CharConstants::optBracketClose;

            StringType ret;

            if (opt.repeated.min() == 0)
                ret += brop;
            ret.append(opt.names.main()).append(this->formatOptionArgSyntax(opt));
            if (opt.repeated.min() == 0)
                ret += brcl;

            return ret;
        }

        auto formatPositionalSyntax(const Positional & pos) const -> StringType {
            constexpr auto space = CharConstants::space;
            constexpr auto brop = CharConstants::optBracketOpen;
            constexpr auto brcl = CharConstants::optBracketClose;
            constexpr auto ellipsis = CharConstants::ellipsis;

            StringType ret;

            if (pos.repeated.min() == 0)
                ret += brop;
            ret += pos.name;
            unsigned idx = 1;
            for (; idx < pos.repeated.min(); ++idx) {
                ret.append({space}).append(pos.name);
            }
            if (idx != pos.repeated.min()) {
                ret.append({space, brop}).append(pos.name);
                if (pos.repeated.max() != pos.repeated.infinity) {
                    for (++idx; idx < pos.repeated.max(); ++idx)
                        ret.append({space}).append(pos.name);
                } else {
                    ret.append({space}).append(ellipsis);
                }
                ret += brcl;
            }
            if (pos.repeated.min() == 0)
                ret += brcl;

            return ret;
        }

        auto calculateDescriptionNames() const -> std::pair<size_t, std::vector<StringType>> {
            size_t maxNameLen = 0;
            std::vector<StringType> optionNames;

            std::for_each(this->m_positionals.begin(), this->m_positionals.end(), [&](auto & pos){
                if (pos.name.length() > maxNameLen)
                    maxNameLen = pos.name.length();
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
            
            StringType ret = opt.names.all().front();
            ret += this->formatOptionArgSyntax(opt);
            std::for_each(opt.names.all().begin() + 1, opt.names.all().end(), [&](const auto & name) {
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
            constexpr auto brop = CharConstants::optBracketOpen;
            constexpr auto brcl = CharConstants::optBracketClose;

            StringType ret;
            std::visit([&](const auto & handler) {
                using HandlerType = std::decay_t<decltype(handler)>;
                if constexpr (std::is_same_v<HandlerType, OptionHandler<OptionArgument::Optional>>) {
                    ret.append({space, brop}).append(opt.argName).append({brcl});
                } else if constexpr (std::is_same_v<HandlerType, OptionHandler<OptionArgument::Required>>)  {
                    ret.append({space}).append(opt.argName);
                }
            }, opt.handler);
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
            auto parse(It argFirst, It argLast) {
            
#ifdef _MSC_VER
    #pragma warning(push) 
    #pragma warning(disable:4702) //bogus "unreachable code"
#endif
                m_owner.m_tokenizer.tokenize(argFirst, argLast, [&](auto && token) {

                    using TokenType = std::decay_t<decltype(token)>;

                    if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::OptionToken>) {

                        resetOption(token.idx, token.usedName, token.argument);

                    } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::OptionStopToken>) {

                        completeOption();

                    } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::ArgumentToken>) {

                        handlePositional(token.value, argFirst + token.argIdx, argLast);

                    } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::UnknownOptionToken>) {

                        handleUnknownOption(token.name, argFirst + token.argIdx, argLast);

                    } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::AmbiguousOptionToken>) {

                        handleAmbiguousOption(token.name, std::move(token.possibilities));
                    } 
                    return ArgumentTokenizer::Continue;
                });
#ifdef _MSC_VER
    #pragma warning(pop) 
#endif
                completeOption();

                //We could use normal validators for this but it is faster to do it manually
                validatePositionals();
                
                for(auto & [validator, desc]: m_owner.m_validators) {
                    if (!validator(m_validationData))
                        throw ValidationError(desc);
                }
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
                }, option.handler);
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
                    }, option.handler);
                m_optionIndex = -1;
                m_optionArgument.reset();
                return ret;
            }

            auto validateOptionMax(const Option & option) {
                auto & name = option.names.main();
                ++m_validationData.optionCount(name);
                auto validator = OptionOccursAtMost(name, option.repeated.max());
                if (!validator(m_validationData)) {
                    throw ValidationError(validator);
                }
            }

            template<ArgIterator<CharType> It>
            auto handlePositional(StringViewType value, It remainingArgFirst, It argLast) {
                if (completeOptionUsingArgument(value))
                    return;

                calculateRemainingPositionals(remainingArgFirst, argLast);
                
                const Positional * positional = nullptr;
                if (m_positionalIndex >= 0) {
                    if (unsigned(m_positionalIndex) >= m_positionalSizes.size())
                        throw ExtraPositional(value);

                    auto & current = m_owner.m_positionals[unsigned(m_positionalIndex)];
                    if (m_positionalSizes[unsigned(m_positionalIndex)] > m_validationData.positionalCount(current.name))
                        positional = &current;

                }

                if (!positional) {
                    auto next = std::find_if(m_positionalSizes.begin() + (m_positionalIndex + 1), m_positionalSizes.end(), [](const unsigned val) {
                        return val > 0;
                    });
                    m_positionalIndex = int(next - m_positionalSizes.begin());
                    if (unsigned(m_positionalIndex) >= m_positionalSizes.size())
                        throw ExtraPositional(value);
                    positional = &m_owner.m_positionals[unsigned(m_positionalIndex)];
                }
                
                auto & count = m_validationData.positionalCount(positional->name);
                positional->handler(count, value);
                ++count;
            }

            template<ArgIterator<CharType> It>
            auto handleUnknownOption(StringViewType name, It remainingArgFirst, It argLast) {

                StringViewType fullText = *remainingArgFirst;
                if (m_owner.m_shouldTreatUnknownOptionAsPositional(name, fullText)) {
                    handlePositional(fullText, remainingArgFirst, argLast);
                } else {
                    completeOption();

                    throw UnrecognizedOption(name);
                }
            }

            auto handleAmbiguousOption(StringViewType name, std::vector<StringType> && possibilities) {

                completeOption();
                throw AmbiguousOption(name, std::move(possibilities));
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
                    auto count = m_validationData.positionalCount(positional.name);
                    if (positional.repeated.max() > count) {
                        remainingPositionalCount += count; //account for already processed
                        partitioner.addRange(positional.repeated.min(), positional.repeated.max());
                        --fillStartIndex;
                    }
                }
                std::for_each(m_owner.m_positionals.begin() + (m_positionalIndex + 1), m_owner.m_positionals.end(),
                                [&] (const Positional & positional) {
                    partitioner.addRange(positional.repeated.min(), positional.repeated.max());
                });

                //3. Partition the range
                unsigned maxRemainingPositionalCount = std::max(remainingPositionalCount, partitioner.minimumSequenceSize());
                auto res = partitioner.partition(maxRemainingPositionalCount);
                MARGP_ALWAYS_ASSERT(res); //this be true by construction

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
                        if (!std::holds_alternative<OptionHandler<OptionArgument::None>>(option.handler)) {
                            currentOptionExpectsArgument = true;
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
                    auto validator = PositionalOccursAtLeast(positional.name, positional.repeated.min());
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
        std::function<bool (StringViewType /*name*/, StringViewType /*fullText*/)> m_shouldTreatUnknownOptionAsPositional = 
            [](StringViewType, StringViewType) { return false; };
    };

    MARGP_DECLARE_FRIENDLY_NAMES(AdaptiveParser)

}

#endif 
