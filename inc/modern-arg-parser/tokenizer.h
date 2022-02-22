#ifndef HEADER_MARGP_TOKENIZER_H_INCLUDED
#define HEADER_MARGP_TOKENIZER_H_INCLUDED

#include "data.h"
#include "flat-map.h"

#include <vector>
#include <optional>
#include <string>
#include <string_view>
#include <algorithm>
#include <iterator>

#include <math.h>
#include <assert.h>

namespace MArgP {

    template<class Char>
    class BasicArgumentTokenizer final {

    private:
        using CharConstants = MArgP::CharConstants<Char>;

        enum PrefixType {
            NotPrefix    = 0,
            ShortPrefix  = 1,
            LongPrefix   = 2,
            OptionStop   = 4
        };
        friend auto operator&(PrefixType lhs, PrefixType rhs) -> PrefixType {
            return PrefixType(std::underlying_type_t<PrefixType>(lhs) & std::underlying_type_t<PrefixType>(rhs));
        }
        friend auto operator&=(PrefixType & lhs, PrefixType rhs) -> PrefixType & {
            lhs = (lhs & rhs);
            return lhs;
        }
        friend auto operator|(PrefixType lhs, PrefixType rhs) -> PrefixType {
            return PrefixType(std::underlying_type_t<PrefixType>(lhs) | std::underlying_type_t<PrefixType>(rhs));
        }
        friend auto operator|=(PrefixType & lhs, PrefixType rhs) -> PrefixType & {
            lhs = (lhs | rhs);
            return lhs;
        }
        using PrefixId = unsigned;
        using NameIndex = unsigned;

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

        class Settings {
            friend BasicArgumentTokenizer;
        public:
            template<StringConvertibleOf<CharType> First, StringConvertibleOf<CharType>... Rest>
            auto addLongPrefix(First && first, Rest && ...rest) -> Settings & {
                std::initializer_list<StringType> values = {makeString(std::forward<First>(first)), makeString(std::forward<Rest>(rest))...};
                for(auto & value: values) {
                    auto [it, inserted] = m_prefixes.add(std::move(value), m_lastPrefixId);
                    auto & type = m_prefixTypes[it->value()];
                    if (!inserted) {
                        //the same prefix cannot be used for long and short
                        MARGP_ALWAYS_ASSERT((type & ShortPrefix) != ShortPrefix); 
                    }
                    type |= LongPrefix;
                }
                ++m_lastPrefixId;
                return *this;
            }

            template<StringConvertibleOf<CharType> First, StringConvertibleOf<CharType>... Rest>
            auto addShortPrefix(First && first, Rest && ...rest) -> Settings & {
                std::initializer_list<StringType> values = {makeString(std::forward<First>(first)), makeString(std::forward<Rest>(rest))...};
                for(auto & value: values) {
                    auto [it, inserted] = m_prefixes.add(std::move(value), m_lastPrefixId);
                    auto & type = m_prefixTypes[it->value()];
                    if (!inserted) {
                        //the same prefix cannot be used for long and short
                        MARGP_ALWAYS_ASSERT((type & LongPrefix) != LongPrefix); 
                    }
                    type |= ShortPrefix;
                }
                ++m_lastPrefixId;
                return *this;
            }

            template<StringConvertibleOf<CharType> First, StringConvertibleOf<CharType>... Rest>
            auto addOptionStopSequence(First && first, Rest && ...rest) -> Settings & {
                std::initializer_list<StringType> values = {makeString(std::forward<First>(first)), makeString(std::forward<Rest>(rest))...};
                for(auto & value: values) {
                    auto [it, inserted] = m_prefixes.add(std::move(value), m_lastPrefixId);
                    auto & type = m_prefixTypes[it->value()];
                    type |= OptionStop;
                }
                ++m_lastPrefixId;
                return *this;
            }

            auto addValueDelimiter(CharType c) -> Settings & {
                
                auto [it, inserted] = m_valueDelimiters.insert(c);
                MARGP_ALWAYS_ASSERT(inserted); //duplicate delimiter  
                return *this;
            }

            static auto commonUnix() -> Settings {
                Settings ret;
                ret.addLongPrefix(CharConstants::doubleDash)
                   .addShortPrefix(CharConstants::dash)
                   .addOptionStopSequence(CharConstants::doubleDash)
                   .addValueDelimiter(CharConstants::assignment);
                return ret;
            }

            static auto unixLongOnly() -> Settings {
                Settings ret;
                ret.addLongPrefix(CharConstants::doubleDash, CharConstants::dash)
                   .addOptionStopSequence(CharConstants::doubleDash)
                   .addValueDelimiter(CharConstants::assignment);
                return ret;
            }

            static auto windowsShort() -> Settings {
                Settings ret;
                ret.addShortPrefix(CharConstants::slash, CharConstants::dash)
                   .addOptionStopSequence(CharConstants::doubleDash)
                   .addValueDelimiter(CharConstants::colon);
                return ret;
            }

            static auto windowsLong() -> Settings {
                Settings ret;
                ret.addLongPrefix(CharConstants::slash, CharConstants::dash, CharConstants::doubleDash)
                   .addOptionStopSequence(CharConstants::doubleDash)
                   .addValueDelimiter(CharConstants::colon);
                return ret;
            }
        private: 
            FlatMap<StringType, PrefixId> m_prefixes;
            FlatMap<PrefixId, PrefixType> m_prefixTypes;
            FlatSet<CharType> m_valueDelimiters;
            PrefixId m_lastPrefixId = 0;
        };
        
    public:
        BasicArgumentTokenizer():
            BasicArgumentTokenizer(Settings::commonUnix()) {
        }

        BasicArgumentTokenizer(Settings settings) {

            this->m_prefixes = std::move(settings.m_prefixes);
            this->m_prefixTypes = std::move(settings.m_prefixTypes);
            this->m_valueDelimiters = std::move(settings.m_valueDelimiters);
        }

        auto add(const OptionNames & names)  {

            auto currentIndex = unsigned(this->m_names.size());
            for(auto & opt: names.all()) {
                auto findResult = findLongestPrefix(opt);
                MARGP_ALWAYS_ASSERT(findResult);                    //option must start with a valid prefix
                MARGP_ALWAYS_ASSERT(findResult->size < opt.size()); //option must have more than a prefix

                if ((findResult->type & LongPrefix) == LongPrefix) {
                    this->add(this->m_longs[findResult->index], opt.substr(findResult->size), currentIndex);
                } else if ((findResult->type & ShortPrefix) == ShortPrefix) {
                    if (opt.size() - findResult->size == 1)
                        this->add(this->m_singleShorts[findResult->index], opt[findResult->size], currentIndex);
                    else
                        this->add(this->m_multiShorts[findResult->index], opt.substr(findResult->size), currentIndex);
                } else {
                    MARGP_ALWAYS_ASSERT(false); //option is neither short nor long with currently defined prefixes
                }
            }
            this->m_names.emplace_back(names.main());
        }

        template<ArgIterator<CharType> It, class Func>
        auto tokenize(It argFirst, It argLast, Func handler) const -> std::vector<StringType>  {

            bool noMoreOptions = false;
            std::vector<StringType> rest;

            for(unsigned argIdx = 0; argFirst != argLast; ++argFirst, ++argIdx) {
                StringViewType arg = *argFirst;

                if (!noMoreOptions) {

                    auto prefixFindResult = this->findLongestPrefix(arg);
                    auto type = prefixFindResult ? prefixFindResult->type : NotPrefix;

                    if ((type & OptionStop) == OptionStop && prefixFindResult->size == arg.size()) {
                        noMoreOptions = true;
                        if (handler(OptionStopToken{argIdx}) == TokenResult::Stop)
                            break;
                    } else if ((type & LongPrefix) == LongPrefix) {
                        if (this->handleLongPrefix(argIdx, arg, 
                                                   prefixFindResult->index, prefixFindResult->size, 
                                                   handler) == TokenResult::Stop) 
                            break;
                    } else if ((type & ShortPrefix) == ShortPrefix) {
                        if (this->handleShortPrefix(argIdx, arg, 
                                                    prefixFindResult->index, prefixFindResult->size, 
                                                    handler, rest) == TokenResult::Stop)
                            break;
                    } else {
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
                              PrefixId prefixId,
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

            auto mapIt = this->m_longs.find(prefixId);
            if (mapIt == this->m_longs.end()) {
                return handler(UnknownOptionToken{argIdx, std::move(usedName), std::move(arg)});
            }
            auto & longsMap = mapIt->value();

            const auto & [first, last] = findMatchOrMatchingPrefixRange(longsMap, name);
            if (last - first == 1) {
                return handler(OptionToken{argIdx, first->value(), std::move(usedName), std::move(arg)});
            } else if (last != first) {
                StringType actualPrefix(option.substr(0, nameStart));
                std::vector<StringType> candidates(last - first);
                std::transform(first, last, candidates.begin(), [&](const auto & p) {return actualPrefix + p.key(); });
                return handler(AmbiguousOptionToken{argIdx, std::move(usedName), std::move(arg), candidates});
            }
            
            return handler(UnknownOptionToken{argIdx, std::move(usedName), std::move(arg)});
        }

        template<class Func>
        auto handleShortPrefix(unsigned argIdx, 
                               StringViewType option, 
                               PrefixId prefixId,
                               unsigned nameStart,
                               const Func & handler, 
                               std::vector<StringType> & rest) const -> TokenResult {

            if (auto maybeResult = this->handleShortOption(argIdx, option, prefixId, nameStart, handler, rest)) {
                return *maybeResult;
            } 

            if (auto maybeToken = this->matchNumber(argIdx, option, nameStart)) {
                return handler(*maybeToken);
            }

            return handler(UnknownOptionToken{argIdx, StringType(option), std::nullopt});
        }

        template<class Func>
        auto handleShortOption(unsigned argIdx, 
                               StringViewType option, 
                               PrefixId prefixId,
                               unsigned nameStart, 
                               const Func & handler, 
                               std::vector<StringType> & rest) const -> std::optional<TokenResult> {

            StringViewType chars = option.substr(nameStart);
            assert(!chars.empty());

            std::optional<unsigned> singleLetterNameIdx;
            
            auto mapIt = this->m_singleShorts.find(prefixId);
            if (mapIt != this->m_singleShorts.end()) {
                auto it = mapIt->value().find(chars[0]);
                if (it != mapIt->value().end())
                    singleLetterNameIdx = it->value();
            }

            if (chars.size() > 1 || !singleLetterNameIdx) {

                if (auto res = this->handleMultiShortOption(argIdx, option, prefixId, nameStart, singleLetterNameIdx.has_value(), handler))
                    return *res;
            }

            if (!singleLetterNameIdx)
                return std::nullopt;
            
            auto & shortsMap = mapIt->value();

            auto actualPrefix = StringType(option.substr(0, nameStart));
            do {
                
                auto currentIdx = *singleLetterNameIdx;
                auto usedName = actualPrefix + chars[0];
                std::optional<StringViewType> arg;

                unsigned consumed = 1;
                if (chars.size() > 1) {

                    auto it = shortsMap.find(chars[1]);
                    if (it == shortsMap.end()) {
                        arg = chars.substr(1);
                        consumed = unsigned(chars.size());
                    }
                    else {
                        singleLetterNameIdx = it->value();
                    }
                }

                auto res = handler(OptionToken{argIdx, currentIdx, std::move(usedName), std::move(arg)});
                if (res == TokenResult::Stop) {
                    rest.push_back(actualPrefix + StringType(chars));
                    return TokenResult::Stop;
                }

                chars.remove_prefix(consumed);
                
            } while(!chars.empty());

            return TokenResult::Continue;
        }

        template<class Func>
        auto handleMultiShortOption(unsigned argIdx, 
                                    StringViewType option,
                                    PrefixId prefixId,
                                    unsigned nameStart,
                                    bool mustMatchExact,
                                    const Func & handler) const -> std::optional<TokenResult> {

            auto [name, arg] = this->splitLongOption(option, nameStart);
            if (name.size() == 0)
                return handler(ArgumentToken{argIdx, option});

            auto mapIt = this->m_multiShorts.find(prefixId);
            if (mapIt == this->m_multiShorts.end()) {
                return std::nullopt;
            }
            auto & multiShortsMap = mapIt->value();

            
            const auto & [first, last] = findMatchOrMatchingPrefixRange(multiShortsMap, name);
            if (last != first) {
                StringType usedName(option.begin(), 
#if defined(_MSC_VER) && _ITERATOR_DEBUG_LEVEL > 0
                    option.begin() + nameStart + name.size()
#else
                    name.end()
#endif
                );
                if (last - first == 1) {
                    if (!mustMatchExact || first->key() == name) {
                        return handler(OptionToken{argIdx, first->value(), std::move(usedName), std::move(arg)});
                    } else {
                        std::vector<StringType> candidates = {
                            StringType(option.substr(0, nameStart + 1)),
                            StringType(option.substr(0, nameStart)) + first->key()
                        };
                        return handler(AmbiguousOptionToken{argIdx, std::move(usedName), std::move(arg), candidates});
                    }
                } else  {
                    StringType actualPrefix(option.substr(0, nameStart));
                    std::vector<StringType> candidates;
                    if (mustMatchExact) {
                        candidates.reserve(1 + (last - first));
                        candidates.push_back(StringType(option.substr(0, nameStart + 1)));
                    } else {
                        candidates.reserve(last - first);
                    }
                    std::transform(first, last, std::back_inserter(candidates), [&](const auto & p) { return actualPrefix + p.key(); });
                    return handler(AmbiguousOptionToken{argIdx, std::move(usedName), std::move(arg), candidates});
                }
            }
        

            return std::nullopt;
        }

        auto splitLongOption(StringViewType option, unsigned nameStart) const -> std::pair<StringViewType, std::optional<StringViewType>> {

            StringViewType name = option.substr(nameStart);
            std::optional<StringViewType> arg;

            for(auto symbol: this->m_valueDelimiters) {
                if (auto assignPos = option.find(symbol, nameStart); 
                    assignPos != option.npos && assignPos - nameStart < name.size()) {

                    name = option.substr(nameStart, assignPos - nameStart);
                    arg = option.substr(assignPos + 1);
                }
            }
            return {std::move(name), std::move(arg)};
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

        struct PrefixFindResult {
            unsigned index; //index of the prefix in m_prefixes;
            unsigned size;  //size of specific matched value of the prefix
            PrefixType type;  
        };

        auto findLongestPrefix(StringViewType arg) const -> std::optional<PrefixFindResult> {

            std::optional<PrefixFindResult> ret;
            for(auto entry: this->m_prefixes) {
                auto & prefix = entry.key();
                auto prefixId = entry.value();
                
                auto typeIt = this->m_prefixTypes.find(prefixId);
                assert(typeIt != this->m_prefixTypes.end());

                if (matchPrefix(arg, StringViewType(prefix))) {
                    if (!ret || prefix.size() > ret->size)
                        ret = {prefixId, unsigned(prefix.size()), typeIt->value()};
                }
                
            }
            return ret;
        }
    private:
        FlatMap<StringType, PrefixId> m_prefixes;
        FlatMap<PrefixId, PrefixType> m_prefixTypes;
        FlatSet<CharType> m_valueDelimiters;
        std::vector<StringType> m_names;

        
        FlatMap<PrefixId, FlatMap<CharType, NameIndex>> m_singleShorts;
        FlatMap<PrefixId, FlatMap<StringType, NameIndex>> m_multiShorts;
        FlatMap<PrefixId, FlatMap<StringType, NameIndex>> m_longs;

        
    };

    MARGP_DECLARE_FRIENDLY_NAMES(ArgumentTokenizer)
}

#endif


