#ifndef HEADER_MARGP_TOKENIZER_H_INCLUDED
#define HEADER_MARGP_TOKENIZER_H_INCLUDED

#include "data.h"

#include <vector>
#include <optional>
#include <string>
#include <string_view>

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
        using OptionNames = BasicOptionNames<Char>;

        struct OptionToken {
            int containingArgIdx;   //index of the full argument containing the token in command line
            StringType name;        //canonical name of the option
            StringType usedName;    //specific option name used
            std::optional<StringViewType> argument; //argument if included with the otpion via = syntax

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
            StringType name;        //option name
            std::optional<StringViewType> argument; //argument if included with the otpion via = syntax

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

        auto remove(StringViewType name) {


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
                        if (this->handleLongOption(argIdx, arg, 2, handler, rest) == TokenResult::Stop) 
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
                        if (this->handleShortOption(argIdx, arg, 1, handler, rest) == TokenResult::Stop)
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
        auto handleLongOption(int argIdx, 
                              StringViewType option, 
                              int nameStart,
                              const Func & handler, 
                              [[maybe_unused]] std::vector<StringType> & rest) const -> TokenResult {
            
            size_t nameEnd = StringViewType::npos;
            std::optional<StringViewType> arg = std::nullopt;

            if (auto assignPos = option.find(CharConstants::argAssignment, nameStart); 
                     assignPos != option.npos && assignPos != 0) {

                nameEnd = assignPos;
                arg = option.substr(assignPos + 1);
            } 

            auto name = StringType(option.substr(0, nameEnd));
            if (auto idx = this->findByPrefix(this->m_longs, option.substr(nameStart, nameEnd)); idx >= 0) {
                auto & canonicalName = this->m_names[idx];
                return handler(OptionToken{argIdx, canonicalName, name, arg});
            } 
            
            return handler(UnknownOptionToken{argIdx, name, arg});
            
        }

        template<class Func>
        auto handleShortOption(int argIdx, 
                               StringViewType option, 
                               int nameStart,
                               const Func & handler, 
                               std::vector<StringType> & rest) const {

            StringViewType chars = option.substr(nameStart);
            if (auto idx = this->find(this->m_multiShorts, chars); idx >= 0) {
                auto & name = this->m_names[idx];
                return handler(OptionToken{argIdx, name, StringType(option), std::nullopt});
            }

            //if any of single chars are unknown report the whole thing as unknown
            for(auto c: chars) {
                if (auto idx = this->find(this->m_singleShorts, c); idx < 0) {
                    return handler(UnknownOptionToken{argIdx, StringType(option), std::nullopt});
                }
            }

            auto prefix = StringType(option.substr(0, nameStart));
            while(!chars.empty()) {
                CharType single = chars.substr(0, 1).front();
                chars.remove_prefix(1);

                if (auto idx = this->find(this->m_singleShorts, single); idx >= 0) {
                    
                    auto & name = this->m_names[idx];
                    auto res = handler(OptionToken{argIdx, name, prefix + single, std::nullopt});
                    if (res == TokenResult::Stop) {
                        rest.push_back(prefix + StringType(chars));
                        return TokenResult::Stop;
                    }
                    
                } else {
                    //it is still possible for this to occur if previous handler removed 
                    //one of previously recognized options
                    //in this case report the remainder as unknown
                    return handler(UnknownOptionToken{argIdx, prefix + StringType(chars), std::nullopt});
                }
            }
            return TokenResult::Continue;
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
    };

    MARGP_DECLARE_FRIENDLY_NAMES(ArgumentTokenizer);
}

#endif


