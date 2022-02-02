#ifndef MODERN_ARG_PARSER_MODERN_ARG_PARSER_H_INCLUDED
#define MODERN_ARG_PARSER_MODERN_ARG_PARSER_H_INCLUDED

#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <map>
#include <span>
#include <optional>
#include <variant>
#include <stdexcept>
#include <ostream>
#include <sstream>
#include <concepts>

#include <assert.h>

namespace MArgP {
    
    //MARK: - CharConstants

    template<class Char>
    struct CharConstants;

    #define MARGP_DEFINE_CHAR_CONSTANTS(type, prefix) template<> struct CharConstants<type> { \
        static constexpr auto optionStart                   = prefix ## '-'; \
        static constexpr auto argAssignment                 = prefix ## '='; \
        \
        static constexpr auto indentation                   = prefix ## "    ";\
        static constexpr auto unrecognizedOptionError       = prefix ## "unrecognized option: "; \
        static constexpr auto missingOptionArgumentError    = prefix ## "argument required for option: "; \
        static constexpr auto extraOptionArgumentError      = prefix ## "extraneous argument for option: "; \
        static constexpr auto extraPositionalError          = prefix ## "unexpected argument: "; \
        static constexpr auto validationError               = prefix ## "invalid arguments: "; \
    };

    MARGP_DEFINE_CHAR_CONSTANTS(char, )
    MARGP_DEFINE_CHAR_CONSTANTS(wchar_t, L)
    MARGP_DEFINE_CHAR_CONSTANTS(char8_t, u8)
    MARGP_DEFINE_CHAR_CONSTANTS(char16_t, u)
    MARGP_DEFINE_CHAR_CONSTANTS(char32_t, U)

    #undef MARGP_DEFINE_CHAR_CONSTANTS
    
    //MARK: - Utility

    template<class Char>
    struct Indent {
        int count = 0;

        friend auto operator<<(std::basic_ostream<Char> & str, Indent val) -> std::basic_ostream<Char> & {
            for(int i = 0; i < val.count; ++i)
                str << CharConstants<char>::indentation;
            return str;
        }
    };

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
    
    //MARK: -
    
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
        virtual auto print(std::basic_ostream<Char> & str) const -> void = 0;
    protected:
        BasicParsingException(std::string_view message) : std::runtime_error(std::string(message)) {
        }

        auto formatOption(std::basic_ostream<Char> & str, std::basic_string_view<Char> opt) const -> void {

            str << CharConstants<Char>::optionStart;
            if (opt.size() > 1)
                str << CharConstants<Char>::optionStart;
            str << opt;
        }
    };

    //MARK: -
    
    template<class Char>
    using ParsingValidationData = std::map<std::basic_string<Char>, int>;

    template<class T, class Char>
    concept ParserValidator = std::is_invocable_r_v<bool, T, const ParsingValidationData<Char>>;
    
    template<class T, class Char>
    concept DescribableParserValidator = ParserValidator<T, Char> &&
        requires(const T & val, std::basic_ostream<Char> & str) {
            { describe(int(), val, str) } -> std::same_as<std::basic_ostream<Char> &>;
        };
    
    template<class Char, class First, class... Rest>
    constexpr bool CompatibleParserValidators = ParserValidator<First, Char> && (ParserValidator<Rest, Char> && ...);
    
    template<class Char, class First, class... Rest>
    constexpr bool CompatibleDescribableParserValidators = DescribableParserValidator<First, Char> && (DescribableParserValidator<Rest, Char> && ...);
    
    template<class T>
    concept AnyParserValidator =
        ParserValidator<T, char> ||
        ParserValidator<T, wchar_t> ||
        ParserValidator<T, char8_t> ||
        ParserValidator<T, char16_t> ||
        ParserValidator<T, char32_t>;
   
    
    template<class First, class... Rest>
    constexpr bool AnyCompatibleParserValidators =
        CompatibleParserValidators<char, First, Rest...> ||
        CompatibleParserValidators<char, First, Rest...> ||
        CompatibleParserValidators<wchar_t, First, Rest...> ||
        CompatibleParserValidators<char8_t, First, Rest...> ||
        CompatibleParserValidators<char16_t, First, Rest...> ||
        CompatibleParserValidators<char32_t, First, Rest...>;
    

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

            auto print(std::basic_ostream<CharType> & str) const -> void override {
                str << CharConstants::unrecognizedOptionError;
                this->formatOption(str, this->option);
            }

            StringType option;
        };

        class MissingOptionArgument : public ParsingException {
        public:
            MissingOptionArgument(StringViewType option_): 
                ParsingException("MissingOptionArgument"),
                option(option_) {
            }

            auto print(std::basic_ostream<CharType> & str) const -> void override {
                str << CharConstants::missingOptionArgumentError;
                this->formatOption(str, this->option);
            }

            StringType option;
        };

        class ExtraOptionArgument : public ParsingException {
        public:
            ExtraOptionArgument(StringViewType option_): 
                ParsingException("ExtraOptionArgument"),
                option(option_) {
            }

            auto print(std::basic_ostream<CharType> & str) const -> void override{
                str << CharConstants::extraOptionArgumentError;
                this->formatOption(str, this->option);
            }

            StringType option;
        };

        class ExtraPositional : public ParsingException {
        public:
            ExtraPositional(StringViewType value_): 
                ParsingException("ExtraPositional"),
                value(value_) {
            }

            auto print(std::basic_ostream<CharType> & str) const -> void override {
                str << CharConstants::extraPositionalError << this->value;
            }

            StringType value;
        };
        
        class ValidationError : public ParsingException {
        public:
            ValidationError(StringViewType value_):
                ParsingException("ValidationError"),
                value(value_) {
            }

            auto print(std::basic_ostream<CharType> & str) const -> void override {
                str << CharConstants::validationError << this->value;
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
            std::basic_ostringstream<CharType> str;
            describe(0, v, str);
            m_validators.emplace_back(std::move(v), std::move(str.str()));
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
    
    template<AnyParserValidator Impl>
    class Not {
    public:
        template<class X>
        Not(X && x) requires(std::is_convertible_v<X &&, Impl>): m_impl(std::forward<X>(x)) {
        }
        
        template<class Char>
        requires(ParserValidator<Impl, Char>)
        auto operator()(const ParsingValidationData<Char> & data) const -> bool {
            return !this->m_impl(data);
        }
        
    private:
        Impl m_impl;
    };
    
    template<AnyParserValidator Validator>
    auto operator!(const Validator & val) {
        return Not<std::decay_t<Validator>>(val);
    }
    template<AnyParserValidator Validator>
    auto operator~(const Validator & val) {
        return !std::forward<Validator>(val);
    }
    
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
        requires(std::is_constructible_v<TupleType, CArgs...>)
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
                    return CombinedValidator<ValidatorCombination::Or, decltype(!std::declval<Args>())...>(!args...);
                else if constexpr (Comb == ValidatorCombination::Or)
                    return CombinedValidator<ValidatorCombination::And, decltype(!std::declval<Args>())...>(!args...);
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
    auto describe(int indent, const CombinedValidator<Comb, Args...> & val, std::basic_ostream<Char> & str) -> std::basic_ostream<Char> & {
        if constexpr (Comb == ValidatorCombination::And)
            str << Indent<Char>{indent} << "all of the following must be true:";
        else if constexpr (Comb == ValidatorCombination::Or)
            str << Indent<Char>{indent} << "one or more of the following must be true:";
        else if constexpr (Comb == ValidatorCombination::Xor)
            str << Indent<Char>{indent} << "only one of the following must be true:";
        else if constexpr (Comb == ValidatorCombination::NXor)
            str << Indent<Char>{indent} << "either all or none of the following must be true:";
        std::apply([&str,indent] (const Args & ...args) {
            (describe(indent + 1, args, str << '\n'), ...);
        }, val.items());
        return str;
    }

    template<class V1, class V2>
    requires(AnyCompatibleParserValidators<std::decay_t<V1>, std::decay_t<V2>>)
    auto operator&&(V1 && v1, V2 && v2)  {
        using R1 = std::decay_t<V1>;
        using R2 = std::decay_t<V2>;
        return CombinedValidator<ValidatorCombination::And, R1, R2>(std::forward<V1>(v1), std::forward<V2>(v2));
    }

    template<class V1, class... Args2>
    requires(AnyCompatibleParserValidators<std::decay_t<V1>, Args2...>)
    auto operator&&(V1 && v1, const CombinedValidator<ValidatorCombination::And, Args2...> & v2)  {
        using R1 = std::decay_t<V1>;
        return CombinedValidator<ValidatorCombination::And, R1, Args2...>(
            std::tuple_cat(std::tuple<R1>(std::forward<V1>(v1)), v2.items())
        );
    }

    template<class V2, class... Args1>
    requires(AnyCompatibleParserValidators<Args1..., std::decay_t<V2>>)
    auto operator&&(const CombinedValidator<ValidatorCombination::And, Args1...> & v1, V2 && v2)  {
        using R2 = std::decay_t<V2>;
        return CombinedValidator<ValidatorCombination::And, Args1..., R2>(
            std::tuple_cat(v1.items(), std::tuple<R2>(std::forward<V2>(v2)))
        );
    }

    template<class... Args1, class... Args2>
    requires(AnyCompatibleParserValidators<Args1..., Args2...>)
    auto operator&&(const CombinedValidator<ValidatorCombination::And, Args1...> & v1,
                    const CombinedValidator<ValidatorCombination::And, Args2...> & v2)  {
        return CombinedValidator<ValidatorCombination::And, Args1..., Args2...>(
            std::tuple_cat(v1.items(), v2.items())
        );
    }

    template<class V1, class V2>
    requires(AnyCompatibleParserValidators<std::decay_t<V1>, std::decay_t<V2>>)
    auto operator||(V1 && v1, V2 && v2)  {
        using R1 = std::decay_t<V1>;
        using R2 = std::decay_t<V2>;
        return CombinedValidator<ValidatorCombination::Or, R1, R2>(std::forward<V1>(v1), std::forward<V2>(v2));
    }

    template<class V1, class... Args2>
    requires(AnyCompatibleParserValidators<std::decay_t<V1>, Args2...>)
    auto operator||(V1 && v1, const CombinedValidator<ValidatorCombination::Or, Args2...> & v2)  {
        using R1 = std::decay_t<V1>;
        return CombinedValidator<ValidatorCombination::Or, R1, Args2...>(
            std::tuple_cat(std::tuple<R1>(std::forward<V1>(v1)), v2.items())
        );
    }

    template<class V2, class... Args1>
    requires(AnyCompatibleParserValidators<Args1..., std::decay_t<V2>>)
    auto operator||(const CombinedValidator<ValidatorCombination::Or, Args1...> & v1, V2 && v2)  {
        using R2 = std::decay_t<V2>;
        return CombinedValidator<ValidatorCombination::Or, Args1..., R2>(
            std::tuple_cat(v1.items(), std::tuple<R2>(std::forward<V2>(v2)))
        );
    }

    template<class... Args1, class... Args2>
    requires(AnyCompatibleParserValidators<Args1..., Args2...>)
    auto operator||(const CombinedValidator<ValidatorCombination::Or, Args1...> & v1,
                    const CombinedValidator<ValidatorCombination::Or, Args2...> & v2)  {
        return CombinedValidator<ValidatorCombination::And, Args1..., Args2...>(
            std::tuple_cat(v1.items(), v2.items())
        );
    }
    
    template<class Char>
    class OptionAbsent;
    
    template<class Char>
    class OptionRequired  {
    public:
        OptionRequired(std::basic_string_view<Char> name) : m_name(name) {}

        auto operator()(const ParsingValidationData<Char> & data) const -> bool {
            auto it = data.find(m_name);
            if (it == data.end())
                return false;
            return it->second > 0;
        }
        
        auto operator!() const -> OptionAbsent<Char>;

        friend auto describe(int indent, const OptionRequired & val, std::basic_ostream<Char> & str) -> std::basic_ostream<Char> & {
            return str << Indent<Char>{indent} << "option " << val.m_name << " is required";
        }
    private:
        std::basic_string<Char> m_name;
    };
    template<class Char> OptionRequired(const Char *) -> OptionRequired<Char>;
    template<class Char> OptionRequired(const std::basic_string<Char> &) -> OptionRequired<Char>;
    
    template<class Char>
    class OptionAbsent  {
    public:
        OptionAbsent(std::basic_string_view<Char> name) : m_name(name) {}

        auto operator()(const ParsingValidationData<Char> & data) const -> bool {
            auto it = data.find(m_name);
            if (it == data.end())
                return true;
            return it->second == 0;
        }
        
        auto operator!() const -> OptionRequired<Char>;

        friend auto describe(int indent, const OptionAbsent & val, std::basic_ostream<Char> & str) -> std::basic_ostream<Char> & {
            return str << Indent<Char>{indent} << "option " << val.m_name << " must not be present";
        }
    private:
        std::basic_string<Char> m_name;
    };
    template<class Char> OptionAbsent(const Char *) -> OptionAbsent<Char>;
    template<class Char> OptionAbsent(const std::basic_string<Char> &) -> OptionAbsent<Char>;
    
    
    template<class Char>
    auto OptionRequired<Char>::operator!() const -> OptionAbsent<Char> {
        return OptionAbsent<Char>(this->m_name);
    }
    
    template<class Char>
    auto OptionAbsent<Char>::operator!() const -> OptionRequired<Char> {
        return OptionRequired<Char>(this->m_name);
    }
    

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
