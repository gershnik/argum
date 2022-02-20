#ifndef HEADER_MARGP_TOKENIZER_H_INCLUDED
#define HEADER_MARGP_TOKENIZER_H_INCLUDED

#include "data.h"
#include "flat-map.h"

#include <vector>
#include <optional>
#include <string>
#include <string_view>
#include <algorithm>

#include <math.h>
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
            unsigned argIdx;                        //index of the argument containing the token in command line
            unsigned idx;                           //index of the option
            StringType usedName;                    //specific option name used
            std::optional<StringViewType> argument; //argument if included with the otpion via = syntax

            auto operator==(const OptionToken & rhs) const -> bool = default;
            auto operator!=(const OptionToken & rhs) const -> bool = default;
        };

        struct ArgumentToken {
            unsigned argIdx;                        //index of the argument containing the token in command line
            StringViewType value;                   //text of the argument

            auto operator==(const ArgumentToken & rhs) const -> bool = default;
            auto operator!=(const ArgumentToken & rhs) const -> bool = default;
        };

        struct OptionStopToken {
            unsigned argIdx;                        //index of the argument containing the token in command line

            auto operator==(const OptionStopToken & rhs) const -> bool = default;
            auto operator!=(const OptionStopToken & rhs) const -> bool = default;
        };

        struct UnknownOptionToken {
            unsigned argIdx;                        //index of the argument containing the token in command line
            StringType name;                        //option name
            std::optional<StringViewType> argument; //argument if included with the otpion via = syntax

            auto operator==(const UnknownOptionToken & rhs) const -> bool = default;
            auto operator!=(const UnknownOptionToken & rhs) const -> bool = default;
        };

        struct AmbiguousOptionToken {
            unsigned argIdx;                        //index of the argument containing the token in command line
            StringType name;                        //option name
            std::optional<StringViewType> argument; //argument if included with the otpion via = syntax
            std::vector<StringType> possibilities;  //possible completions

            auto operator==(const AmbiguousOptionToken & rhs) const -> bool = default;
            auto operator!=(const AmbiguousOptionToken & rhs) const -> bool = default;
        };


        enum TokenResult {
            Stop,
            Continue
        };

        struct Settings {
            Settings() = default;
            
            std::vector<StringType> shortPrefixes = { StringType(1, CharConstants::optionStart) };
            std::vector<StringType> longPrefixes = { StringType(2, CharConstants::optionStart) };
            std::vector<StringType> valueDelimiters = { StringType(1, CharConstants::argAssignment) };
        };
        
    public:
        auto add(const OptionNames & names)  {

            auto currentIndex = unsigned(this->m_names.size());
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

        auto allowShortLongs(bool value) {
             this->m_allowShortLongs = value;
        }

        template<ArgIterator<CharType> It, class Func>
        auto tokenize(It argFirst, It argLast, Func handler) const -> std::vector<StringType>  {

            bool noMoreOptions = false;
            std::vector<StringType> rest;

            for(unsigned argIdx = 0; argFirst != argLast; ++argFirst, ++argIdx) {
                const CharType * argp = *argFirst;
                StringViewType arg = argp;

                if (!noMoreOptions && arg.size() > 1) {

                    if (arg.size() > 2 && arg[0] == CharConstants::optionStart && arg[1] == CharConstants::optionStart)
                    {
                        //start of a long option
                        if (this->handleLongPrefix(argIdx, arg, 2, handler) == TokenResult::Stop) 
                            break;
                    }
                    else if (arg.size() == 2 && arg[0] == CharConstants::optionStart && arg[1] == CharConstants::optionStart) {
                        // "--" - no more options 
                        noMoreOptions = true;
                        if (handler(OptionStopToken{argIdx}) == TokenResult::Stop)
                            break;
                    }
                    else if (arg[0] == CharConstants::optionStart) {
                        //start of a short option or negative number
                        if (this->handleShortPrefix(argIdx, arg, 1, handler, rest) == TokenResult::Stop)
                            break;
                    } else {
                        //not an option
                        if (handler(ArgumentToken{argIdx, arg}) == TokenResult::Stop)
                            break;
                    }
                } else {

                    if (handler(ArgumentToken{argIdx, arg}) == TokenResult::Stop)
                        break;
                }
                
            }
            for ( ; argFirst != argLast; ++argFirst) {
                rest.emplace_back(*argFirst);
            }
            return rest;
        }

    private:
        template<class Func>
        auto handleLongPrefix(unsigned argIdx, 
                              StringViewType option, 
                              unsigned nameStart,
                              const Func & handler) const -> TokenResult {
            
            auto [name, arg] = this->splitLongOption(option, nameStart);
            if (name.size() == 0)
                return handler(ArgumentToken{argIdx, option});
            StringType usedName(option.begin(), 
#if defined(_MSC_VER) && _ITERATOR_DEBUG_LEVEL > 0
                option.begin() + nameStart + name.size()
#else
                name.end()
#endif
            );

            const auto & [first, last] = findMatchOrMatchingPrefixRange(this->m_longs, name);
            if (last - first == 1) {
                return handler(OptionToken{argIdx, first->value(), std::move(usedName), std::move(arg)});
            } else if (last != first) {
                StringType prefix(option.substr(0, nameStart));
                std::vector<StringType> candidates(last - first);
                std::transform(first, last, candidates.begin(), [&](const auto & p) {return prefix + p.key(); });
                return handler(AmbiguousOptionToken{argIdx, std::move(usedName), std::move(arg), candidates});
            }
            
            return handler(UnknownOptionToken{argIdx, std::move(usedName), std::move(arg)});
        }

        template<class Func>
        auto handleShortPrefix(unsigned argIdx, 
                               StringViewType option, 
                               unsigned nameStart,
                               const Func & handler, 
                               std::vector<StringType> & rest) const -> TokenResult {

            if (auto maybeResult = this->handleShortOption(argIdx, option, nameStart, handler, rest)) {
                return *maybeResult;
            } 

            if (auto maybeToken = this->matchNumber(argIdx, option, nameStart)) {
                return handler(*maybeToken);
            }

            return handler(UnknownOptionToken{argIdx, StringType(option), std::nullopt});
        }

        auto splitLongOption(StringViewType option, unsigned nameStart) const -> std::pair<StringViewType, std::optional<StringViewType>> {

            StringViewType name;
            std::optional<StringViewType> arg;

            if (auto assignPos = option.find(CharConstants::argAssignment, nameStart); 
                assignPos != option.npos) {

                name = option.substr(nameStart, assignPos - nameStart);
                arg = option.substr(assignPos + 1);
            } else {
                name = option.substr(nameStart);
            }
            return {std::move(name), std::move(arg)};
        }

        template<class Func>
        auto handleShortOption(unsigned argIdx, 
                               StringViewType option, 
                               unsigned nameStart, 
                               const Func & handler, 
                               std::vector<StringType> & rest) const -> std::optional<TokenResult> {

            StringViewType chars = option.substr(nameStart);
            assert(!chars.empty());

            auto it = this->m_singleShorts.find(chars[0]);
            bool singleLetterFound = (it != this->m_singleShorts.end());

            if (chars.size() > 1 || !singleLetterFound) {

                if (auto res = this->handleMultiShortOption(argIdx, option, nameStart, handler))
                    return *res;
            }

            if (!singleLetterFound)
                return std::nullopt;
            
            auto prefix = StringType(option.substr(0, nameStart));
            auto idx = it->value();
            do {
                
                auto currentIdx = idx;
                auto usedName = prefix + chars[0];
                std::optional<StringViewType> arg;

                unsigned consumed = 1;
                if (chars.size() > 1) {

                    it = this->m_singleShorts.find(chars[1]);
                    if (it == this->m_singleShorts.end()) {
                        arg = chars.substr(1);
                        consumed = unsigned(chars.size());
                    }
                    else {
                        idx = it->value();
                    }
                }

                auto res = handler(OptionToken{argIdx, currentIdx, std::move(usedName), std::move(arg)});
                if (res == TokenResult::Stop) {
                    rest.push_back(prefix + StringType(chars));
                    return TokenResult::Stop;
                }

                chars.remove_prefix(consumed);
                
            } while(!chars.empty());

            return TokenResult::Continue;
        }

        template<class Func>
        auto handleMultiShortOption(unsigned argIdx, 
                                    StringViewType option,
                                    unsigned nameStart,
                                    const Func & handler) const -> std::optional<TokenResult> {

            auto [name, arg] = this->splitLongOption(option, nameStart);
            if (name.size() == 0)
                return handler(ArgumentToken{argIdx, option});

            const FlatMap<StringType, unsigned> * mapsToTest[] = {&this->m_multiShorts, (this->m_allowShortLongs ? &this->m_longs : nullptr), nullptr};

            for(const FlatMap<StringType, unsigned> ** ppmap = mapsToTest; *ppmap; ++ppmap) {
                const auto & [first, last] = findMatchOrMatchingPrefixRange(**ppmap, name);
                if (last != first) {
                    StringType usedName(option.begin(), 
#if defined(_MSC_VER) && _ITERATOR_DEBUG_LEVEL > 0
                        option.begin() + nameStart + name.size()
#else
                        name.end()
#endif
                    );
                    if (last - first == 1) {
                        return handler(OptionToken{argIdx, first->value(), std::move(usedName), std::move(arg)});
                    } else {
                        StringType prefix(option.substr(0, nameStart));
                        std::vector<StringType> candidates(last - first);
                        std::transform(first, last, candidates.begin(), [&](const auto & p) {return prefix + p.key(); });
                        return handler(AmbiguousOptionToken{argIdx, std::move(usedName), std::move(arg), candidates});
                    }
                }
            }

            return std::nullopt;
        }

        auto matchNumber(unsigned argIdx, 
                         StringViewType option, 
                         unsigned /*nameStart*/) const -> std::optional<ArgumentToken> {

            //we assume that option is null terminated here!

            errno = 0;
            const CharType * p = &option[0];
            CharType * pEnd;
            CharConstants::toLongLong(p, &pEnd, 0);
            if (size_t(pEnd - p) == option.size() && errno != ERANGE)
                return ArgumentToken{argIdx, option};

            long double dres = CharConstants::toLongDouble(p, &pEnd);
            if (size_t(pEnd - p) == option.size() && dres != HUGE_VALL)
                return ArgumentToken{argIdx, option};
                
            return std::nullopt;
        }

        template<class Value, class Arg>
        static auto add(FlatMap<Value, unsigned> & map, Arg arg, unsigned idx) -> void {

            auto [it, inserted] = map.add(arg, idx);
            MARGP_ALWAYS_ASSERT(inserted); //duplicate option if this fails
        }
    private:
        std::vector<StringType> m_names;
        FlatMap<CharType, unsigned> m_singleShorts;
        FlatMap<StringType, unsigned> m_multiShorts;
        FlatMap<StringType, unsigned> m_longs;

        bool m_allowShortLongs = false;
    };

    MARGP_DECLARE_FRIENDLY_NAMES(ArgumentTokenizer)
}

#endif


