#ifndef HEADER_MARGP_TOKENIZER_H_INCLUDED
#define HEADER_MARGP_TOKENIZER_H_INCLUDED

#include "data.h"

#include <vector>

#include <assert.h>

namespace MArgP {

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

    MARGP_DECLARE_FRIENDLY_NAMES(ArgumentTokenizer);
}

#endif


