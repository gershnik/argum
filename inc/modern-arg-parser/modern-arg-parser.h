#ifndef HEADER_MARGP_MODERN_ARG_PARSER_H_INCLUDED
#define HEADER_MARGP_MODERN_ARG_PARSER_H_INCLUDED

#include "char-constants.h"
#include "messages.h"
#include "formatting.h"
#include "validators.h"
#include "tokenizer.h"

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <variant>
#include <functional>
#include <stdexcept>
#include <ostream>
#include <sstream>
#include <concepts>

#include <assert.h>

namespace MArgP {
    
    template<class Char>
    class BasicParsingException : public std::runtime_error {
    public:
        auto print(std::basic_ostream<Char> & str) const -> void {
            print(str, {0});
        }
        virtual auto print(std::basic_ostream<Char> & str, Indent<Char> indent) const -> void = 0;
    protected:
        BasicParsingException(std::string_view message) : std::runtime_error(std::string(message)) {
        }

        auto formatOption(std::basic_string_view<Char> opt) const  {
            return Printable([=](std::basic_ostream<Char> & str) {
                str << CharConstants<Char>::optionStart;
                if (opt.size() > 1)
                    str << CharConstants<Char>::optionStart;
                str << opt;
            });
        }
    };

    template<class Char>
    class BasicSequentialArgumentParser {

    public:
        using CharType = Char;
        using StringType = std::basic_string<Char>;
        using StringViewType = std::basic_string_view<Char>;
        using OptionNames = BasicOptionNames<Char>;
        using ParsingException = BasicParsingException<Char>;

    private:
        template<OptionArgument Argument> struct OptionHandlerDeducer;

        template<>
        struct OptionHandlerDeducer<OptionArgument::None> { using Type = std::function<void ()>; };
        template<>
        struct OptionHandlerDeducer<OptionArgument::Optional> { using Type = std::function<void (std::optional<StringViewType>)>; };
        template<>
        struct OptionHandlerDeducer<OptionArgument::Required> { using Type = std::function<void (StringViewType)>; };

        using CharConstants = CharConstants<CharType>;
        using Messages = Messages<CharType>;
        
        using ValidatorFunction = std::function<bool (const ParsingValidationData<CharType> &)>;

    public:
        template<OptionArgument Argument> using OptionHandler = typename OptionHandlerDeducer<Argument>::Type;

        using PositionalHandler = std::function<void (int, const StringViewType &)>;
        
        
        class UnrecognizedOption : public ParsingException {
        public:
            UnrecognizedOption(StringViewType value): 
                ParsingException("UnrecognizedOption"),
                option(value) {
            }

            auto print(std::basic_ostream<CharType> & str, Indent<Char> indent) const -> void override {
                str << format(Messages::unrecognizedOptionError(), indent, this->formatOption(this->option));
            }

            StringType option;
        };

        class MissingOptionArgument : public ParsingException {
        public:
            MissingOptionArgument(StringViewType option_): 
                ParsingException("MissingOptionArgument"),
                option(option_) {
            }

            auto print(std::basic_ostream<CharType> & str, Indent<Char> indent) const -> void override {
                str << format(Messages::missingOptionArgumentError(), indent, this->formatOption(this->option));
            }

            StringType option;
        };

        class ExtraOptionArgument : public ParsingException {
        public:
            ExtraOptionArgument(StringViewType option_): 
                ParsingException("ExtraOptionArgument"),
                option(option_) {
            }

            auto print(std::basic_ostream<CharType> & str, Indent<Char> indent) const -> void override{
                str << format(Messages::extraOptionArgumentError(), indent, this->formatOption(this->option));
            }

            StringType option;
        };

        class ExtraPositional : public ParsingException {
        public:
            ExtraPositional(StringViewType value_): 
                ParsingException("ExtraPositional"),
                value(value_) {
            }

            auto print(std::basic_ostream<CharType> & str, Indent<Char> indent) const -> void override {
                str << format(Messages::extraPositionalError(), indent, this->value);
            }

            StringType value;
        };
        
        class ValidationError : public ParsingException {
        public:
            ValidationError(StringViewType value_):
                ParsingException("ValidationError"),
                value(value_) {
            }

            auto print(std::basic_ostream<CharType> & str, Indent<Char> indent) const -> void override {
                str << format(Messages::validationError(), indent, this->value);
            }

            StringType value;
        };

    private:
        using ArgumentTokenizer = BasicArgumentTokenizer<Char>;
        using ValidationData = ParsingValidationData<Char>;

        struct Option {
            using Handler = std::variant<
                OptionHandler<OptionArgument::None>,
                OptionHandler<OptionArgument::Optional>,
                OptionHandler<OptionArgument::Required>
            >;
            
            Option(OptionNames && n, Handler && h): names(std::move(n)), handler(std::move(h)) {
            }

            OptionNames names;
            Handler handler;
        };

        class PendingOption {
        public:
            auto reset(const Option & opt, StringViewType name, const std::optional<StringViewType> & argument, ValidationData & validationData) {
                complete(validationData);
                m_name = std::move(name);
                m_argument = argument;
                m_option = &opt;
            }

            auto complete(ValidationData & validationData) {
                if (!m_option)
                    return;

                std::visit([&](const auto & handler) {
                    using HandlerType = std::decay_t<decltype(handler)>;
                    if constexpr (std::is_same_v<HandlerType, OptionHandler<OptionArgument::None>>) {
                        if (m_argument)
                            throw ExtraOptionArgument(m_name);
                        handler();
                    } else if constexpr (std::is_same_v<HandlerType, OptionHandler<OptionArgument::Optional>>) {
                        handler(m_argument);
                    } else {
                        if (!m_argument)
                            throw MissingOptionArgument(m_name);
                        handler(*m_argument);
                    }
                }, m_option->handler);
                ++validationData[m_option->names.main()];
                m_option = nullptr;
            }

            auto completeUsingArgument(StringViewType argument, ValidationData & validationData) -> bool {

                if (!m_option)
                    return false;

                auto ret = std::visit([&](const auto & handler) {
                        using HandlerType = std::decay_t<decltype(handler)>;
                        if constexpr (std::is_same_v<HandlerType, OptionHandler<OptionArgument::None>>) {
                            if (m_argument)
                                throw ExtraOptionArgument(m_name);
                            handler();
                            return false;
                        } else {
                            if (m_argument) {
                                handler(*m_argument);
                                return false;
                            } else {
                                handler(argument);
                                return true;
                            }
                        }
                    }, m_option->handler);
                ++validationData[m_option->names.main()];
                m_option = nullptr;
                return ret;
            }

        private:
            const Option * m_option = nullptr;
            StringViewType m_name;
            std::optional<StringViewType> m_argument;
        };

    public:
        template<class Handler>
        auto add(OptionNames && names, Handler handler) -> void
            requires(std::is_convertible_v<decltype(std::move(handler)), typename Option::Handler>) {

            auto & added = this->m_options.emplace_back(std::move(names), std::move(handler));
            this->m_optionsByName.add(added.names.main(), this->m_options.size() - 1);
            this->m_tokenizer.add(this->m_options.back().names);
        }

        auto setPositionals(int maxCount, PositionalHandler handler) -> void {
            assert(maxCount >= 0);
            this->m_maxPositionals = maxCount;
            this->m_positionalsHandler = handler;
        }
        
        template<ParserValidator<CharType> Validator>
        auto addValidator(Validator v, StringViewType description) -> void {
            m_validators.emplace_back(std::move(v), description);
        }

        template<DescribableParserValidator<CharType> Validator>
        auto addValidator(Validator v) -> void  {
            auto desc = (std::basic_ostringstream<CharType>() << describe(Indent<Char>{0}, v)).str();
            m_validators.emplace_back(std::move(v), std::move(desc));
        }

        auto parse(int argc, const CharType ** argv) {
            
            PendingOption pendingOption;
            int currentPositionalIdx = 0;
            ParsingValidationData<CharType> validationData;
            

            this->m_tokenizer.tokenize(argc, argv, [&](const auto & token) {

                using TokenType = std::decay_t<decltype(token)>;

                if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::OptionToken>) {

                    auto idxIt = this->m_optionsByName.find(token.name);
                    MARGP_ALWAYS_ASSERT(idxIt != this->m_optionsByName.end());
                    auto & option = this->m_options[idxIt->value()];
                    pendingOption.reset(option, token.usedName, token.argument, validationData);
                    return ArgumentTokenizer::Continue;

                } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::OptionStopToken>) {

                    pendingOption.complete(validationData);
                    return ArgumentTokenizer::Continue;

                } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::ArgumentToken>) {

                    if (pendingOption.completeUsingArgument(token.value, validationData))
                        return ArgumentTokenizer::Continue;

                    if (currentPositionalIdx >= this->m_maxPositionals)
                        throw ExtraPositional(token.value);

                    this->m_positionalHandler(currentPositionalIdx, token.value);
                    ++currentPositionalIdx;
                    return ArgumentTokenizer::Continue;

                } else if constexpr (std::is_same_v<TokenType, typename ArgumentTokenizer::UnknownOptionToken>) {

                    if (pendingOption.completeUsingArgument(argv[token.containingArgIdx], validationData))
                        return ArgumentTokenizer::Continue;

                    throw UnrecognizedOption(token.name);
                }
            });
            pendingOption.complete(validationData);
            
            for(auto & [validator, desc]: m_validators) {
                if (!validator(validationData))
                    throw ValidationError(desc);
            }
        }

        auto printUsage(std::basic_ostream<Char> & str, StringViewType progName) {

            str << Messages::usageStart() << progName;

            for(auto & opt: this->m_options) {
                str << CharConstants::space << opt.names.main();
            }
            str << std::endl;
        }
    public:
        std::vector<Option> m_options;
        FlatMap<StringType, int> m_optionsByName;
        int m_maxPositionals = 0;
        PositionalHandler m_positionalHandler;
        ArgumentTokenizer m_tokenizer;
        std::vector<std::pair<ValidatorFunction, StringType>> m_validators;
    };
    
    
    //MARK: - Specializations

    
    MARGP_DECLARE_FRIENDLY_NAMES(ParsingException);
    MARGP_DECLARE_FRIENDLY_NAMES(SequentialArgumentParser);
}


#endif 
