#ifndef MODERN_ARG_PARSER_MODERN_ARG_PARSER_H_INCLUDED
#define MODERN_ARG_PARSER_MODERN_ARG_PARSER_H_INCLUDED

#include "char-constants.h"
#include "messages.h"
#include "formatting.h"
#include "validators.h"

#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <map>
#include <optional>
#include <variant>
#include <functional>
#include <stdexcept>
#include <ostream>
#include <sstream>
#include <concepts>

#include <assert.h>

namespace MArgP {
    
    //MARK: - Utility

    template<class Char>
    class BasicOptionName final {
    
    public:
        using CharType = Char;
        using StringType = std::basic_string<Char>;
        using StringViewType = std::basic_string_view<Char>;
        using CharConstants = CharConstants<Char>;
    public:
        template<class First, class... Rest>
        requires(std::is_convertible_v<First, StringViewType> && (std::is_convertible_v<Rest, StringViewType> && ...))
        BasicOptionName(First && first, Rest && ...rest) {
            int nameLevel = 0;
            init(nameLevel, std::forward<First>(first), std::forward<Rest>(rest)...);
        }

        auto name() const -> const StringType & {
            return this->m_name;
        }

        auto shortOptions() const -> const std::set<StringType> & {
            return this->m_shortOptions;
        }

        auto longOptions() const -> const std::set<StringType> & {
            return this->m_longOptions;
        }


    private:
        template<class Param>
        auto init(int & nameLevel, Param && param) -> void {
            
            add(nameLevel, param);
        }

        template<class First, class... Rest>
        auto init(int & nameLevel, First && first, Rest && ...rest) -> void {
            
            add(nameLevel, std::forward<First>(first));
            init(nameLevel, std::forward<Rest>(rest)...);
        }

        auto add(int & nameLevel, StringViewType opt) -> void {
            assert(opt.size() > 1 && opt[0] == CharConstants::optionStart);
            
            int currentNameLevel = 0;
            if (opt[1] == CharConstants::optionStart) {
                assert(opt.size() > 2);
                opt.remove_prefix(2);
                this->m_longOptions.emplace(opt);
                currentNameLevel = 2;
            } else {
                opt.remove_prefix(1);
                this->m_shortOptions.emplace(opt);
                currentNameLevel = 1;
            }
            
            if (nameLevel < currentNameLevel) {
                this->m_name = opt;
                nameLevel = currentNameLevel;
            }
        }

    private:
        StringType m_name;
        std::set<StringType> m_shortOptions;
        std::set<StringType> m_longOptions;
    };

    enum class OptionArgument  {
        None,
        Optional,
        Required
    };
    
    //MARK: - Tokenizer
    
    template<class Char>
    class BasicArgumentTokenizer final {

    private:
        using CharConstants = CharConstants<Char>;

    public:
        using CharType = Char;
        using StringType = std::basic_string<Char>;
        using StringViewType = std::basic_string_view<Char>;
        using OptionName = BasicOptionName<Char>;

        struct OptionToken {
            int containingArgIdx;   //index of the full argument containing the token in command line
            int index;              //index of the option
            StringViewType value;   //specific option name used
            std::optional<StringViewType> argument; //argument if included witht he otpion via = syntax

            auto operator==(const OptionToken & rhs) const -> bool = default;
            auto operator!=(const OptionToken & rhs) const -> bool = default;
        };

        struct ArgumentToken {
            int containingArgIdx;   //index of the full argument containing the token in command line
            StringViewType value;   //text of the argument

            auto operator==(const ArgumentToken & rhs) const -> bool = default;
            auto operator!=(const ArgumentToken & rhs) const -> bool = default;
        };

        struct OptionStopToken {

            int containingArgIdx;   //index of the full argument containing the token in command line

            auto operator==(const OptionStopToken & rhs) const -> bool = default;
            auto operator!=(const OptionStopToken & rhs) const -> bool = default;
        };

        struct UnknownOptionToken {
            int containingArgIdx;   //index of the full argument containing the token in command line
            StringViewType value;   //specific option name used

            auto operator==(const UnknownOptionToken & rhs) const -> bool = default;
            auto operator!=(const UnknownOptionToken & rhs) const -> bool = default;
        };


        enum TokenResult {
            Stop,
            Continue
        };
        
    public:
        auto add(const OptionName & name) -> int {
            for(StringViewType str: name.shortOptions()) {
                if (str.size() == 1)
                    this->add(this->m_singleShorts, str[0], m_optionCount);
                else
                    this->add(this->m_multiShorts, str, m_optionCount);
            }
            for(StringViewType str: name.longOptions()) {
                this->add(this->m_longs, str, m_optionCount);
            }
            return m_optionCount++;
        }
        
        template<class Func>
        auto tokenize(int argc, CharType * const * argv, Func handler) const -> int {
            return this->tokenize(argc, const_cast<const CharType **>(argv), handler);
        }

        template<class Func>
        auto tokenize(int argc, const CharType * const * argv, Func handler) const -> std::vector<StringType> {

            bool noMoreOptions = false;
            std::vector<StringType> rest;

            int argIdx = 0;

            for(argIdx = 1; argIdx < argc; ++argIdx) {
                auto argp = argv[argIdx];
                StringViewType arg = { argp, strlen(argp) };

                if (!noMoreOptions && arg.size() > 1 && arg[0] == CharConstants::optionStart) {

                    if (arg.size() > 2 && arg[1] == CharConstants::optionStart)
                    {
                        //start of a long option
                        auto body = arg.substr(2);
                        if (this->handleLongOption(argIdx, body, handler, rest) == TokenResult::Stop) 
                            break;
                    }
                    else if (arg.size() == 2 && arg[1] == CharConstants::optionStart) {
                        // "--" - no more options 
                        noMoreOptions = true;
                        if (handler(OptionStopToken{argIdx}) == TokenResult::Stop)
                            break;
                    }
                    else {
                        //start of a short option or negative number
                        auto body = arg.substr(1);
                        if (this->handleShortOption(argIdx, body, handler, rest) == TokenResult::Stop)
                            break;
                    }
                } else {

                    if (handler(ArgumentToken{argIdx, arg}) == TokenResult::Stop)
                        break;
                }
                
            }
            for ( ; argIdx < argc; ++argIdx) {
                rest.emplace_back(argv[argIdx]);
            }
            return rest;
        }

    private:
        template<class Func>
        auto handleLongOption(int argIdx, StringViewType option, const Func & handler, [[maybe_unused]] std::vector<StringType> & rest) const -> TokenResult {
            
            StringViewType name = option;
            std::optional<StringViewType> arg = std::nullopt;

            if (auto assignPos = option.find(CharConstants::argAssignment); 
                    assignPos != option.npos && assignPos != 0) {

                name = option.substr(0, assignPos);
                arg = option.substr(assignPos + 1);

            }

            if (auto idx = this->findByPrefix(this->m_longs, name); idx >= 0) {
                return handler(OptionToken{argIdx, idx, name, arg});
            } 
            
            return handler(UnknownOptionToken{argIdx, name});
            
        }

        template<class Func>
        auto handleShortOption(int argIdx, StringViewType chars, const Func & handler, std::vector<StringType> & rest) const {

            if (auto idx = this->find(this->m_multiShorts, chars); idx >= 0) {
                return handler(OptionToken{argIdx, idx, chars, std::nullopt});
            }

            while(!chars.empty()) {
                StringViewType option = chars.substr(0, 1);
                chars.remove_prefix(1);

                auto idx = this->find(this->m_singleShorts, option.front());
                auto res = idx >= 0 ?
                                handler(OptionToken{argIdx, idx, option, std::nullopt}) :
                                handler(UnknownOptionToken{argIdx, option});
                if (res == TokenResult::Stop) {
                    rest.push_back(StringType(CharConstants::optionStart, 1) + StringType(chars));
                    return TokenResult::Stop;
                }
            }
            return TokenResult::Continue;
        }

        template<class Value, class Arg>
        static auto add(std::vector<std::pair<Value, int>> & vec, Arg arg, int idx) -> void {
            auto it = std::lower_bound(vec.begin(), vec.end(), arg, [](const auto & lhs, const auto & rhs) {
                return lhs.first < rhs;
            });
            assert(it == vec.end() || it->first != arg); //duplicate option if this fails
            vec.emplace(it, arg, idx);
        }

        template<class Value, class Arg>
        static auto find(const std::vector<std::pair<Value, int>> & vec, Arg arg) -> int {
            auto it = std::lower_bound(vec.begin(), vec.end(), arg, [](const auto & lhs, const auto & rhs) {
                return lhs.first < rhs;
            });
            if (it == vec.end() || it->first != arg)
                return -1;
            return it->second;
        }

        template<class Value, class Arg>
        static auto findByPrefix(const std::vector<std::pair<Value, int>> & vec, Arg arg) -> int {
            auto it = std::lower_bound(vec.begin(), vec.end(), arg, [](const auto & lhs, const auto & rhs) {
                //prefix match comparator
                auto res = lhs.first.compare(0, rhs.size(), rhs);
                return res < 0;
            });
            if (it == vec.end())
                return -1;
            return it->second;
        }
    private:
        int m_optionCount = 0;
        std::vector<std::pair<CharType, int>> m_singleShorts;
        std::vector<std::pair<StringType, int>> m_multiShorts;
        std::vector<std::pair<StringType, int>> m_longs;
    };
    
    //MARK: -

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

    //MARK: - 
    

    template<class Char>
    class BasicSequentialArgumentParser {

    public:
        using CharType = Char;
        using StringType = std::basic_string<Char>;
        using StringViewType = std::basic_string_view<Char>;
        using OptionName = BasicOptionName<Char>;
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
            
            Option(OptionName && n, Handler && h): name(std::move(n)), handler(std::move(h)) {
            }

            OptionName name;
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
                ++validationData[m_option->name.name()];
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
                ++validationData[m_option->name.name()];
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
        auto add(OptionName && name, Handler handler) -> void
            requires(std::is_convertible_v<decltype(std::move(handler)), typename Option::Handler>) {

            this->m_options.emplace_back(std::move(name), std::move(handler));
            this->m_tokenizer.add(this->m_options.back().name);
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

                    auto & option = this->m_options[size_t(token.index)];
                    pendingOption.reset(option, token.value, token.argument, validationData);
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

                    if (pendingOption.completeUsingArgument(token.value, validationData))
                        return ArgumentTokenizer::Continue;

                    throw UnrecognizedOption(token.value);
                }
            });
            pendingOption.complete(validationData);
            
            for(auto & [validator, desc]: m_validators) {
                if (!validator(validationData))
                    throw ValidationError(desc);
            }
        }
    public:
        std::vector<Option> m_options;
        int m_maxPositionals = 0;
        PositionalHandler m_positionalHandler;
        ArgumentTokenizer m_tokenizer;
        std::vector<std::pair<ValidatorFunction, StringType>> m_validators;
    };
    
    
    //MARK: - Specializations

    #define MARGP_DECLARE_FRIENDLY_NAME(stem, type, prefix) using prefix ## stem = Basic##stem<type>;
    #define MARGP_DECLARE_FRIENDLY_NAMES(stem) \
        MARGP_DECLARE_FRIENDLY_NAME(stem, char, ) \
        MARGP_DECLARE_FRIENDLY_NAME(stem, wchar_t, L) \
        MARGP_DECLARE_FRIENDLY_NAME(stem, char8_t, U8) \
        MARGP_DECLARE_FRIENDLY_NAME(stem, char16_t, U16) \
        MARGP_DECLARE_FRIENDLY_NAME(stem, char32_t, U32)

    MARGP_DECLARE_FRIENDLY_NAMES(OptionName);
    MARGP_DECLARE_FRIENDLY_NAMES(ArgumentTokenizer);
    MARGP_DECLARE_FRIENDLY_NAMES(ParsingException);
    MARGP_DECLARE_FRIENDLY_NAMES(SequentialArgumentParser);

    #undef MARGP_DECLARE_FRIENDLY_NAMES
    #undef MARGP_DECLARE_FRIENDLY_NAME
}


#endif 
