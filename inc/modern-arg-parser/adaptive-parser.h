#ifndef HEADER_MARGP_ADAPTIVE_PARSER_H_INCLUDED
#define HEADER_MARGP_ADAPTIVE_PARSER_H_INCLUDED

#include "char-constants.h"
#include "messages.h"
#include "formatting.h"
#include "validators.h"
#include "tokenizer.h"
#include "partitioner.h"

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <variant>
#include <ostream>
#include <sstream>
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
        using CharConstants = CharConstants<CharType>;
        using Messages = Messages<CharType>;

        using ValidatorFunction = std::function<bool (const ParsingValidationData<CharType> &)>;


        template<OptionArgument Argument> struct OptionHandlerDeducer;

        template<>
        struct OptionHandlerDeducer<OptionArgument::None> { 
            using Type = std::function<void ()>; 
        };
        template<>
        struct OptionHandlerDeducer<OptionArgument::Optional> { 
            using Type = std::function<void (std::optional<StringViewType>)>; 
        };
        template<>
        struct OptionHandlerDeducer<OptionArgument::Required> { 
            using Type = std::function<void (StringViewType)>; 
        };

        using ArgumentTokenizer = BasicArgumentTokenizer<Char>;
        using ValidationData = ParsingValidationData<Char>;

    public:
        template<OptionArgument Argument> using OptionHandler = typename OptionHandlerDeducer<Argument>::Type;

        using PositionalHandler = std::function<void (unsigned, StringViewType)>;
        
        struct UnrecognizedOption : public ParsingException {
            UnrecognizedOption(StringViewType option_): 
                ParsingException(format(Messages::unrecognizedOptionError(), option_)),
                option(option_) {
            }
            StringType option;
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
                ParsingException(describeToStr<CharType>(validator)) {
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
            auto setHandler(H handler_) -> Option &
            requires(std::is_convertible_v<decltype(std::move(handler_)), Handler>) {
                this->handler = std::move(handler_);
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
        auto add(Option option) -> void {

            auto & added = this->m_options.emplace_back(std::move(option));
            this->m_optionsByName.add(added.names.main(), unsigned(this->m_options.size()) - 1);
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
            auto desc = describeToStr<CharType>(v);
            m_validators.emplace_back(std::move(v), std::move(desc));
        }

        auto parse(CharType * const * argFirst, CharType * const * argLast) const {
            return this->parse(const_cast<const CharType **>(argFirst), const_cast<const CharType **>(argLast));
        }

        auto parse(const CharType * const * argFirst, const CharType * const * argLast) const {
            
            ParsingState parsingState(*this);

            parsingState.parse(argFirst, argLast);
        }

        auto printUsage(std::basic_ostream<Char> & str, StringViewType progName) const {

            constexpr auto endl = CharConstants::endl;

            str << Messages::usageStart();
            this->printSyntax(str, progName);
            str << endl << endl;

            auto calcRes = this->calculateDescriptionNames();
            auto & [maxNameLen, optionNames] = calcRes;
            if (maxNameLen > 21)
                maxNameLen = 21;

            if (!this->m_positionals.empty()) {
                str << "positional arguments:";
                for(auto & pos: this->m_positionals) {
                    str << endl;
                    this->printItemDescription(str, pos.name, pos.description, 2, maxNameLen);
                }
                str << endl << endl;
            }

            if (!this->m_options.empty()) {
                str << "options:";
                for(size_t idx = 0; idx < this->m_options.size(); ++idx) {
                    auto & opt = this->m_options[idx];
                    auto name = optionNames[idx];
                    str << endl;
                    this->printItemDescription(str, name, opt.description, 2, maxNameLen);
                }
                str << endl << endl;
            }
        }

        auto printSyntax(std::basic_ostream<Char> & str, StringViewType progName) const {

            constexpr auto space = CharConstants::space;

            str << progName;

            for(auto & opt: this->m_options) {
                str << space;
                this->printOptionSyntax(str, opt);
            }

            for (auto & pos: this->m_positionals) {
                str << space;
                this->printPositionalSyntax(str, pos);
            }
        }

        auto printOptionSyntax(std::basic_ostream<Char> & str, const Option & opt) const {
            constexpr auto brop = CharConstants::optBracketOpen;
            constexpr auto brcl = CharConstants::optBracketClose;

            if (opt.repeated.min() == 0)
                str << brop;
            str << opt.names.main();
            this->printOptionArgSyntax(str, opt);
            if (opt.repeated.min() == 0)
                str << brcl;
        }

        auto printPositionalSyntax(std::basic_ostream<Char> & str, const Positional & pos) const {
            constexpr auto space = CharConstants::space;
            constexpr auto brop = CharConstants::optBracketOpen;
            constexpr auto brcl = CharConstants::optBracketClose;
            constexpr auto ellipsis = CharConstants::ellipsis;

            if (pos.repeated.min() == 0)
                str << brop;
            str << pos.name;
            unsigned idx = 1;
            for (; idx < pos.repeated.min(); ++idx)
                str << space << pos.name;
            if (idx != pos.repeated.min()) {
                str << space << brop << pos.name;
                if (pos.repeated.max() != pos.repeated.infinity) {
                    for (++idx; idx < pos.repeated.max(); ++idx)
                        str << space << pos.name;
                } else {
                    str << space << ellipsis;
                }
                str << brcl;
            }
            if (pos.repeated.min() == 0)
                str << brcl;
        }

        auto calculateDescriptionNames() const -> std::pair<size_t, std::vector<StringType>> {
            size_t maxNameLen = 0;
            std::vector<StringType> optionNames;

            std::for_each(this->m_positionals.begin(), this->m_positionals.end(), [&](auto & pos){
                if (pos.name.length() > maxNameLen)
                    maxNameLen = pos.name.length();
            });
            std::for_each(this->m_options.begin(), this->m_options.end(), [&](auto & opt){
                std::basic_ostringstream<CharType> buf;
                this->printOptionNameForDescription(buf, opt);
                optionNames.emplace_back(buf.str());
                if (optionNames.back().length() > maxNameLen)
                    maxNameLen = optionNames.back().length();
            });
            return {maxNameLen, std::move(optionNames)};
        }

        auto printOptionNameForDescription(std::basic_ostream<Char> & str, const Option & opt) const {
            constexpr auto space = CharConstants::space;
            constexpr auto comma = CharConstants::comma;

            str << opt.names.all().front();
            this->printOptionArgSyntax(str, opt);
            std::for_each(opt.names.all().begin() + 1, opt.names.all().end(), [&](const auto & name) {
                str << comma << space << name;
                this->printOptionArgSyntax(str, opt);
            });
        }

        auto printItemDescription(std::basic_ostream<Char> & str, 
                                  StringViewType name, 
                                  StringViewType description, 
                                  size_t leadingSpaceCount,
                                  size_t maxNameLen) const {
            constexpr auto space = CharConstants::space;
            constexpr auto endl = CharConstants::endl;

            Indent<CharType> descIndent{unsigned(maxNameLen + leadingSpaceCount + 2)};

            str << Indent<CharType>(unsigned(leadingSpaceCount)) << name;
            if (name.length() > maxNameLen) {
                str << endl << descIndent;
            } else {
                for(auto i = name.length(); i < maxNameLen; ++i)
                    str << space;
                str << space << space;
            }
            str << describe(descIndent, description);
        }

        auto printOptionArgSyntax(std::basic_ostream<Char> & str, const Option & opt) const {
            constexpr auto space = CharConstants::space;
            constexpr auto brop = CharConstants::optBracketOpen;
            constexpr auto brcl = CharConstants::optBracketClose;

            std::visit([&](const auto & handler) {
                using HandlerType = std::decay_t<decltype(handler)>;
                if constexpr (std::is_same_v<HandlerType, OptionHandler<OptionArgument::Optional>>) {
                    str << space << brop << opt.argName << brcl;
                } else if constexpr (std::is_same_v<HandlerType, OptionHandler<OptionArgument::Required>>)  {
                    str << space << opt.argName;
                }
            }, opt.handler);
        }

    private:
        class ParsingState {
        public:
            ParsingState(const BasicAdaptiveParser & owner): 
                m_owner(owner),
                m_updateCountAtLastRecalc(owner.m_updateCount - 1) {
            }

            auto parse(const CharType * const * argFirst, const CharType * const * argLast) {
            
                m_owner.m_tokenizer.tokenize(argFirst, argLast, [&](const auto & token) {

                    using TokenType = std::decay_t<decltype(token)>;

                    if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::OptionToken>) {

                        auto idxIt = m_owner.m_optionsByName.find(token.name);
                        MARGP_ALWAYS_ASSERT(idxIt != m_owner.m_optionsByName.end());
                        resetOption(idxIt->value(), token.usedName, token.argument);

                    } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::OptionStopToken>) {

                        completeOption();

                    } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::ArgumentToken>) {

                        handlePositional(token.value, token.containingArg, argLast);

                    } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::UnknownOptionToken>) {

                        handleUnknownOption(token.name, *token.containingArg);
                    }
                    return ArgumentTokenizer::Continue;
                });
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

            auto handlePositional(StringViewType value, const CharType * const * remainingArgFirst, const CharType * const * argLast) {
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

            auto handleUnknownOption(StringViewType name, StringViewType fullText) {
                if (completeOptionUsingArgument(fullText))
                    return;

                throw UnrecognizedOption(name);
            }

            auto calculateRemainingPositionals(const CharType * const * remainingArgFirst, const CharType * const * argLast)  {

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
            
            auto countRemainingPositionals(const CharType * const * remainingArgFirst, const CharType * const * argLast) -> unsigned {
                
                unsigned remainingPositionalCount = 0;
                bool currentOptionExpectsArgument = false;
                m_owner.m_tokenizer.tokenize(remainingArgFirst, argLast, [&](const auto & token) {
                    using TokenType = std::decay_t<decltype(token)>;

                    if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::OptionToken>) {

                        auto idxIt = m_owner.m_optionsByName.find(token.name);
                        MARGP_ALWAYS_ASSERT(idxIt != m_owner.m_optionsByName.end());
                        auto & option = m_owner.m_options[idxIt->value()];
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
            StringViewType m_optionName;
            std::optional<StringViewType> m_optionArgument;

            int m_positionalIndex = -1;
            std::vector<unsigned> m_positionalSizes;
            
            ParsingValidationData<CharType> m_validationData;
        };

    private:
        std::vector<Option> m_options;
        FlatMap<StringType, unsigned> m_optionsByName;
        std::vector<Positional> m_positionals;
        ArgumentTokenizer m_tokenizer;
        std::vector<std::pair<ValidatorFunction, StringType>> m_validators;
        size_t m_updateCount = 0;
    };

    MARGP_DECLARE_FRIENDLY_NAMES(AdaptiveParser);

}

#endif 
