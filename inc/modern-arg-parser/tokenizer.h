#ifndef HEADER_MARGP_TOKENIZER_H_INCLUDED
#define HEADER_MARGP_TOKENIZER_H_INCLUDED

#include "data.h"
#include "flat-map.h"

#include <vector>
#include <optional>
#include <string>
#include <string_view>

#include <assert.h>

namespace MArgP {

    template<class Char>
    class BasicArgumentTokenizer final {

    private:
        using CharConstants = MArgP::CharConstants<Char>;

    public:
        using CharType = Char;
        using StringType = std::basic_string<Char>;
        using StringViewType = std::basic_string_view<Char>;
        using OptionNames = BasicOptionNames<Char>;

        struct OptionToken {
            const CharType * const * containingArg;   //full argument containing the token in command line
            unsigned idx;                             //index of the option
            StringType usedName;                      //specific option name used
            std::optional<StringViewType> argument;   //argument if included with the otpion via = syntax

            auto operator==(const OptionToken & rhs) const -> bool = default;
            auto operator!=(const OptionToken & rhs) const -> bool = default;
        };

        struct ArgumentToken {
            const CharType * const * containingArg;   //full argument containing the token in command line
            StringViewType value;                     //text of the argument

            auto operator==(const ArgumentToken & rhs) const -> bool = default;
            auto operator!=(const ArgumentToken & rhs) const -> bool = default;
        };

        struct OptionStopToken {

            const CharType * const * containingArg;   //full argument containing the token in command line

            auto operator==(const OptionStopToken & rhs) const -> bool = default;
            auto operator!=(const OptionStopToken & rhs) const -> bool = default;
        };

        struct UnknownOptionToken {
            const CharType * const * containingArg;   //full argument containing the token in command line
            StringType name;                          //option name
            std::optional<StringViewType> argument;   //argument if included with the otpion via = syntax

            auto operator==(const UnknownOptionToken & rhs) const -> bool = default;
            auto operator!=(const UnknownOptionToken & rhs) const -> bool = default;
        };


        enum TokenResult {
            Stop,
            Continue
        };
        
    public:
        auto add(const OptionNames & names)  {

            int currentIndex = int(this->m_names.size());
            for(auto & opt: names.all()) {
                MARGP_ALWAYS_ASSERT(opt.size() > 1 && opt[0] == CharConstants::optionStart);

                if (opt[1] == CharConstants::optionStart) {
                    MARGP_ALWAYS_ASSERT(opt.size() > 2);
                    this->add(this->m_longs, opt.substr(2), currentIndex);
                } else {
                    if (opt.size() == 2)
                        this->add(this->m_singleShorts, opt[1], currentIndex);
                    else
                        this->add(this->m_multiShorts, opt.substr(1), currentIndex);
                }
            }
            this->m_names.emplace_back(names.main());
        }

        template<class Func>
        auto tokenize(CharType * const * argFirst, CharType * const * argLast, Func handler) const -> std::vector<StringType> {
            return this->tokenize(const_cast<const CharType **>(argFirst), const_cast<const CharType **>(argLast), handler);
        }

        template<class Func>
        auto tokenize(const CharType * const * argFirst, const CharType * const * argLast, Func handler) const -> std::vector<StringType> {

            bool noMoreOptions = false;
            std::vector<StringType> rest;

            const CharType * const * argCurrent = argFirst;

            for( ; argCurrent != argLast; ++argCurrent) {
                auto argp = *argCurrent;
                StringViewType arg = argp;

                if (!noMoreOptions && arg.size() > 1) {

                    if (arg.size() > 2 && arg[0] == CharConstants::optionStart && arg[1] == CharConstants::optionStart)
                    {
                        //start of a long option
                        if (this->handleLongPrefix(argCurrent, arg, 2, handler) == TokenResult::Stop) 
                            break;
                    }
                    else if (arg.size() == 2 && arg[0] == CharConstants::optionStart && arg[1] == CharConstants::optionStart) {
                        // "--" - no more options 
                        noMoreOptions = true;
                        if (handler(OptionStopToken{argCurrent}) == TokenResult::Stop)
                            break;
                    }
                    else if (arg[0] == CharConstants::optionStart) {
                        //start of a short option or negative number
                        if (this->handleShortPrefix(argCurrent, arg, 1, handler, rest) == TokenResult::Stop)
                            break;
                    } else {
                        //not an option
                        if (handler(ArgumentToken{argCurrent, arg}) == TokenResult::Stop)
                            break;
                    }
                } else {

                    if (handler(ArgumentToken{argCurrent, arg}) == TokenResult::Stop)
                        break;
                }
                
            }
            for ( ; argCurrent != argLast; ++argCurrent) {
                rest.emplace_back(*argCurrent);
            }
            return rest;
        }

    private:
        template<class Func>
        auto handleLongPrefix(const CharType * const * argCurrent, 
                              StringViewType option, 
                              unsigned nameStart,
                              const Func & handler) const -> TokenResult {
            
            auto [name, arg] = this->splitLongOption(option, nameStart);
            if (name.size() == 0)
                return handler(ArgumentToken{argCurrent, option});

            if (auto idx = this->findByPrefix(this->m_longs, name); idx >= 0) {
                return handler(OptionToken{argCurrent, unsigned(idx), StringType(name), std::move(arg)});
            } 
            
            return handler(UnknownOptionToken{argCurrent, StringType(name), std::move(arg)});
        }

        template<class Func>
        auto handleShortPrefix(const CharType * const * argCurrent, 
                               StringViewType option, 
                               unsigned nameStart,
                               const Func & handler, 
                               std::vector<StringType> & rest) const -> TokenResult {

            if (this->m_allowShortLongs) {

                auto [name, arg] = this->splitLongOption(option, nameStart);
                if (name.size() == 0)
                    return handler(ArgumentToken{argCurrent, option});

                if (auto idx = this->findByPrefix(this->m_longs, name); idx >= 0) {
                    return handler(OptionToken{argCurrent, unsigned(idx), StringType(name), std::move(arg)});
                } 
            }

            if (auto maybeResult = this->handleShortOption(argCurrent, option, nameStart, handler, rest)) {
                return *maybeResult;
            } 

            if (auto maybeToken = this->matchNumber(argCurrent, option, nameStart)) {
                return handler(*maybeToken);
            }

            return handler(UnknownOptionToken{argCurrent, StringType(option), std::nullopt});
        }

        auto splitLongOption(StringViewType option, unsigned nameStart) const -> std::pair<StringViewType, std::optional<StringViewType>> {

            StringViewType name;
            std::optional<StringViewType> arg;

            if (auto assignPos = option.find(CharConstants::argAssignment, nameStart); 
                assignPos != option.npos) {

                name = option.substr(nameStart, assignPos);
                arg = option.substr(assignPos + 1);
            } else {
                name = option;
            }
            return {std::move(name), std::move(arg)};
        }

        template<class Func>
        auto handleShortOption(const CharType * const * argCurrent, 
                               StringViewType option, 
                               unsigned nameStart, 
                               const Func & handler, 
                               std::vector<StringType> & rest) const -> std::optional<TokenResult> {

            auto prefix = StringType(option.substr(0, nameStart));
            StringViewType chars = option.substr(nameStart);
            assert(!chars.empty());

            if (auto idx = this->findByPrefix(this->m_multiShorts, chars); idx >= 0) {
                return handler(OptionToken{argCurrent, unsigned(idx), StringType(option), std::nullopt});
            } 

            auto idx = this->find(this->m_singleShorts, chars[0]);
            if (idx < 0)
                return std::nullopt;
            
            do {
                
                auto currentIdx = unsigned(idx);
                auto usedName = prefix + chars[0];
                std::optional<StringViewType> arg;

                unsigned consumed = 1;
                if (chars.size() > 1) {
                    idx = this->find(this->m_singleShorts, chars[1]);
                    if (idx < 0) {
                        arg = chars.substr(1);
                        consumed = unsigned(chars.size());
                    }
                }

                auto res = handler(OptionToken{argCurrent, currentIdx, std::move(usedName), std::move(arg)});
                if (res == TokenResult::Stop) {
                    rest.push_back(prefix + StringType(chars));
                    return TokenResult::Stop;
                }

                chars.remove_prefix(consumed);
                
            } while(!chars.empty());

            return TokenResult::Continue;
        }

        auto matchNumber(const CharType * const * argCurrent, 
                         StringViewType option, 
                         unsigned /*nameStart*/) const -> std::optional<ArgumentToken> {

            assert(&option[0] == *argCurrent); //option must be the entrie arg and null terminated

            errno = 0;
            const CharType * p = *argCurrent;
            CharType * pEnd;
            long long res;
            if constexpr (std::is_same_v<CharType, char>)
                res = strtoll(p, &pEnd, 0);
            else 
                res = wcstoll(p, &pEnd, 0);

            if (size_t(pEnd - p) != option.size() || errno == ERANGE)
                return std::nullopt;
    
            return ArgumentToken{argCurrent, option};
        }

        template<class Value, class Arg>
        static auto add(FlatMap<Value, int> & map, Arg arg, int idx) -> void {

            auto [it, inserted] = map.add(arg, idx);
            MARGP_ALWAYS_ASSERT(inserted); //duplicate option if this fails
        }

        template<class Value, class Arg>
        static auto find(const FlatMap<Value, int> & map, Arg arg) -> int {
            auto it = map.find(arg);
            return it != map.end() ? it->value() : -1;
        }

        template<class Value, class Arg>
        static auto findByPrefix(const FlatMap<Value, int> & map, Arg arg) -> int {
            
            auto it = findShortestMatchingPrefix(map, arg);
            return it != map.end() ? it->value() : -1;
        }
    private:
        std::vector<StringType> m_names;
        FlatMap<CharType, int> m_singleShorts;
        FlatMap<StringType, int> m_multiShorts;
        FlatMap<StringType, int> m_longs;

        bool m_allowShortLongs = true;
    };

    MARGP_DECLARE_FRIENDLY_NAMES(ArgumentTokenizer)
}

#endif


