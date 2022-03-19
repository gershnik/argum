//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_TOKENIZER_H_INCLUDED
#define HEADER_ARGUM_TOKENIZER_H_INCLUDED

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

namespace Argum {

    ARGUM_MOD_EXPORTED
    template<class Char>
    class BasicTokenizer final {

    private:
        using CharConstants = Argum::CharConstants<Char>;

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
            Continue   = 0,
            StopAfter  = 0b10,
            StopBefore = 0b11
        };

        class Settings {
            friend BasicTokenizer;
        public:
            template<StringConvertibleOf<CharType> First, StringConvertibleOf<CharType>... Rest>
            auto addLongPrefix(First && first, Rest && ...rest) & -> Settings & {
                std::initializer_list<StringType> values = {makeString(std::forward<First>(first)), makeString(std::forward<Rest>(rest))...};
                for(auto & value: values) {
                    auto [it, inserted] = m_prefixes.add(std::move(value), m_lastPrefixId);
                    auto & type = m_prefixTypes[it->value()];
                    if (!inserted) {
                        if ((type & ShortPrefix) == ShortPrefix)
                            ARGUM_INVALID_ARGUMENT("the same prefix cannot be used for long and short options");
                    }
                    type |= LongPrefix;
                }
                ++m_lastPrefixId;
                return *this;
            }

            template<StringConvertibleOf<CharType> First, StringConvertibleOf<CharType>... Rest>
            auto addLongPrefix(First && first, Rest && ...rest) && -> Settings && {
                return std::move(static_cast<Settings *>(this)->addLongPrefix(std::forward<First>(first), std::forward<Rest>(rest)...));
            }

            template<StringConvertibleOf<CharType> First, StringConvertibleOf<CharType>... Rest>
            auto addShortPrefix(First && first, Rest && ...rest) & -> Settings & {
                std::initializer_list<StringType> values = {makeString(std::forward<First>(first)), makeString(std::forward<Rest>(rest))...};
                for(auto & value: values) {
                    auto [it, inserted] = m_prefixes.add(std::move(value), m_lastPrefixId);
                    auto & type = m_prefixTypes[it->value()];
                    if (!inserted) {
                        //the same prefix cannot be used for long and short
                        if ((type & LongPrefix) == LongPrefix) 
                            ARGUM_INVALID_ARGUMENT("the same prefix cannot be used for long and short options");
                    }
                    type |= ShortPrefix;
                }
                ++m_lastPrefixId;
                return *this;
            }

            template<StringConvertibleOf<CharType> First, StringConvertibleOf<CharType>... Rest>
            auto addShortPrefix(First && first, Rest && ...rest) && -> Settings && {
                return std::move(static_cast<Settings *>(this)->addShortPrefix(std::forward<First>(first), std::forward<Rest>(rest)...));
            }

            template<StringConvertibleOf<CharType> First, StringConvertibleOf<CharType>... Rest>
            auto addOptionTerminator(First && first, Rest && ...rest) & -> Settings & {
                std::initializer_list<StringType> values = {makeString(std::forward<First>(first)), makeString(std::forward<Rest>(rest))...};
                for(auto & value: values) {
                    auto [it, inserted] = m_prefixes.add(std::move(value), m_lastPrefixId);
                    auto & type = m_prefixTypes[it->value()];
                    type |= OptionStop;
                }
                ++m_lastPrefixId;
                return *this;
            }

            template<StringConvertibleOf<CharType> First, StringConvertibleOf<CharType>... Rest>
            auto addOptionTerminator(First && first, Rest && ...rest) && -> Settings && {
                return std::move(static_cast<Settings *>(this)->addOptionTerminator(std::forward<First>(first), std::forward<Rest>(rest)...));
            }

            auto addValueDelimiter(CharType c) & -> Settings & {
                
                auto [it, inserted] = m_valueDelimiters.insert(c);
                if (!inserted)
                    ARGUM_INVALID_ARGUMENT("duplicate delimiter");
                return *this;
            }

            auto addValueDelimiter(CharType c) && -> Settings && {
                return std::move(static_cast<Settings *>(this)->addValueDelimiter(c));
            }

            auto allowAbbreviation(bool value) & -> Settings & {
                m_allowAbrreviation = value;
                return *this;
            }

            auto allowAbbreviation(bool value) && -> Settings && {
                return std::move(static_cast<Settings *>(this)->allowAbbreviation(value));
            }

            static auto commonUnix() -> Settings {
                Settings ret;
                ret.addLongPrefix(CharConstants::doubleDash)
                   .addShortPrefix(CharConstants::dash)
                   .addOptionTerminator(CharConstants::doubleDash)
                   .addValueDelimiter(CharConstants::assignment);
                return ret;
            }

            static auto unixLongOnly() -> Settings {
                Settings ret;
                ret.addLongPrefix(CharConstants::doubleDash, CharConstants::dash)
                   .addOptionTerminator(CharConstants::doubleDash)
                   .addValueDelimiter(CharConstants::assignment);
                return ret;
            }

            static auto windowsShort() -> Settings {
                Settings ret;
                ret.addShortPrefix(CharConstants::slash, CharConstants::dash)
                   .addOptionTerminator(CharConstants::doubleDash)
                   .addValueDelimiter(CharConstants::colon);
                return ret;
            }

            static auto windowsLong() -> Settings {
                Settings ret;
                ret.addLongPrefix(CharConstants::slash, CharConstants::dash, CharConstants::doubleDash)
                   .addOptionTerminator(CharConstants::doubleDash)
                   .addValueDelimiter(CharConstants::colon);
                return ret;
            }
        private: 
            FlatMap<StringType, PrefixId> m_prefixes;
            FlatMap<PrefixId, PrefixType> m_prefixTypes;
            FlatSet<CharType> m_valueDelimiters;
            PrefixId m_lastPrefixId = 0;
            bool m_allowAbrreviation = true;
        };
        
    public:
        BasicTokenizer():
            BasicTokenizer(Settings::commonUnix()) {
        }

        BasicTokenizer(Settings settings) {

            this->m_prefixes = std::move(settings.m_prefixes);
            this->m_prefixTypes = std::move(settings.m_prefixTypes);
            this->m_valueDelimiters = std::move(settings.m_valueDelimiters);
            this->m_allowAbrreviation = settings.m_allowAbrreviation;
        }

        auto add(const OptionNames & names)  {

            auto currentIndex = unsigned(this->m_names.size());
            for(auto & opt: names.all()) {
                auto findResult = findLongestPrefix(opt);
                if (!findResult)
                    ARGUM_INVALID_ARGUMENT("option must start with a valid prefix");
                if (findResult->size == opt.size())
                    ARGUM_INVALID_ARGUMENT("option must have more than a prefix");

                if ((findResult->type & LongPrefix) == LongPrefix) {
                    this->add(this->m_longs[findResult->index], opt.substr(findResult->size), currentIndex);
                } else if ((findResult->type & ShortPrefix) == ShortPrefix) {
                    if (opt.size() - findResult->size == 1)
                        this->add(this->m_singleShorts[findResult->index], opt[findResult->size], currentIndex);
                    else
                        this->add(this->m_multiShorts[findResult->index], opt.substr(findResult->size), currentIndex);
                } else {
                    ARGUM_INVALID_ARGUMENT("option is neither short nor long with currently defined prefixes");
                }
            }
            this->m_names.emplace_back(names.main());
        }

        template<ArgIterator<CharType> It, class Func>
        auto tokenize(It argFirst, It argLast, Func handler) const -> std::vector<StringType>  {

            bool noMoreOptions = false;
            std::vector<StringType> rest;
            StringViewType arg; //last argument being processed
            unsigned consumed = 0; //how much of the arg was consumed before invoking a handler
            unsigned unconsumedPrefixSize = 0; //size of the arg option prefix if it is an option

            for(unsigned argIdx = 0; argFirst != argLast; ++argFirst, ++argIdx) {
                arg = *argFirst;
                consumed = 0;
                unconsumedPrefixSize = 0;

                std::optional<TokenResult> result;
                if (!noMoreOptions) {

                    if (auto prefixFindResult = this->findLongestPrefix(arg)) {
                    
                        auto type = prefixFindResult->type;
                    
                        if (prefixFindResult->size == arg.size()) {
                            
                            if ((type & OptionStop) == OptionStop) {
                                noMoreOptions = true;
                                result = handler(OptionStopToken{argIdx});
                                if (result == TokenResult::StopAfter)
                                    consumed = unsigned(arg.size());
                            }
                            
                        } else {
                            if ((type & LongPrefix) == LongPrefix) {
                                result = this->handleLongPrefix(argIdx, arg,
                                                                prefixFindResult->index, prefixFindResult->size,
                                                                handler);
                                if (result == TokenResult::StopAfter)
                                    consumed = unsigned(arg.size());
                            } else if ((type & ShortPrefix) == ShortPrefix) {
                                result = this->handleShortPrefix(argIdx, arg,
                                                                 prefixFindResult->index, prefixFindResult->size,
                                                                 consumed, handler);
                                unconsumedPrefixSize = prefixFindResult->size;
                            }
                        }
                    }
                }
                
                if (!result) {
                    result = handler(ArgumentToken{argIdx, arg});
                    if (result == TokenResult::StopAfter)
                        consumed = unsigned(arg.size());
                }

                if (*result != TokenResult::Continue)
                    break;
            }

            if (argFirst != argLast) {
                
                if (consumed == arg.size()) {
                    ++argFirst;
                } else if (consumed != 0) {
                    rest.emplace_back(StringType(arg.substr(0, unconsumedPrefixSize)) + StringType(arg.substr(consumed)));
                    ++argFirst;
                }

                for ( ; argFirst != argLast; ++argFirst) {
                    rest.emplace_back(*argFirst);
                }
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
            
            auto [name, arg] = this->splitDelimitedArgument(option, nameStart);
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

            if (this->m_allowAbrreviation) {
                const auto & [first, last] = findMatchOrMatchingPrefixRange(longsMap, name);
                if (last - first == 1) {
                    return handler(OptionToken{argIdx, first->value(), std::move(usedName), std::move(arg)});
                } else if (last != first) {
                    StringType actualPrefix(option.substr(0, nameStart));
                    std::vector<StringType> candidates(last - first);
                    std::transform(first, last, candidates.begin(), [&](const auto & p) {return actualPrefix + p.key(); });
                    return handler(AmbiguousOptionToken{argIdx, std::move(usedName), std::move(arg), candidates});
                }
            } else {
                auto it = longsMap.find(name);
                if (it != longsMap.end())
                    return handler(OptionToken{argIdx, it->value(), std::move(usedName), std::move(arg)});
            }
            
            if (auto maybeToken = this->matchNumber(argIdx, option, nameStart)) {
                return handler(*maybeToken);
            }
            return handler(UnknownOptionToken{argIdx, std::move(usedName), std::move(arg)});
        }

        template<class Func>
        auto handleShortPrefix(unsigned argIdx, 
                               StringViewType option, 
                               PrefixId prefixId,
                               unsigned nameStart,
                               unsigned & consumed,
                               const Func & handler) const -> TokenResult {

            if (auto maybeResult = this->handleShortOption(argIdx, option, prefixId, nameStart, consumed, handler)) {
                return *maybeResult;
            } 

            TokenResult result;
            if (auto maybeToken = this->matchNumber(argIdx, option, nameStart)) {
                result = handler(*maybeToken);
            } else {
                result = handler(UnknownOptionToken{argIdx, StringType(option), std::nullopt});
            }

            if (result == TokenResult::StopAfter)
                consumed = unsigned(option.size());
            return result;
        }

        template<class Func>
        auto handleShortOption(unsigned argIdx, 
                               StringViewType option, 
                               PrefixId prefixId,
                               unsigned nameStart, 
                               unsigned & consumed,
                               const Func & handler) const -> std::optional<TokenResult> {

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

                if (auto res = this->handleMultiShortOption(argIdx, option, prefixId, nameStart, singleLetterNameIdx.has_value(), handler)) {
                    if (*res == TokenResult::StopAfter)
                        consumed = unsigned(option.size());
                    return *res;
                }
            }

            if (!singleLetterNameIdx)
                return std::nullopt;
            
            auto & shortsMap = mapIt->value();
            consumed = nameStart;
            auto actualPrefix = StringType(option.substr(0, nameStart));
            do {
                
                auto currentIdx = *singleLetterNameIdx;
                auto usedName = actualPrefix + chars[0];
                std::optional<StringViewType> arg;

                unsigned charsConsumed = 1;
                if (chars.size() > 1) {

                    auto it = shortsMap.find(chars[1]);
                    if (it == shortsMap.end()) {
                        arg = chars.substr(1);
                        charsConsumed = unsigned(chars.size());
                    }
                    else {
                        singleLetterNameIdx = it->value();
                    }
                }

                auto res = handler(OptionToken{argIdx, currentIdx, std::move(usedName), std::move(arg)});
                if (res != TokenResult::Continue) {
                    if (res == TokenResult::StopAfter)
                        consumed += charsConsumed;
                    return res;
                }

                chars.remove_prefix(charsConsumed);
                consumed += charsConsumed;
                
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

            auto [name, arg] = this->splitDelimitedArgument(option, nameStart);
            if (name.size() == 0)
                return handler(ArgumentToken{argIdx, option});

            auto mapIt = this->m_multiShorts.find(prefixId);
            if (mapIt == this->m_multiShorts.end()) {
                return std::nullopt;
            }
            auto & multiShortsMap = mapIt->value();

            if (this->m_allowAbrreviation) {
                const auto & [first, last] = findMatchOrMatchingPrefixRange(multiShortsMap, name);
                if (last != first) {
                    StringType usedName(option.data(), name.data() + name.size());
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
            } else {
                auto it = multiShortsMap.find(name);
                if (it != multiShortsMap.end()) {
                    StringType usedName(option.data(), name.data() + name.size());
                    return handler(OptionToken{argIdx, it->value(), std::move(usedName), std::move(arg)});
                }
            }
        

            return std::nullopt;
        }

        auto splitDelimitedArgument(StringViewType option, unsigned nameStart) const -> std::pair<StringViewType, std::optional<StringViewType>> {

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
            if (!inserted)
                ARGUM_INVALID_ARGUMENT("duplicate option");
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

        bool m_allowAbrreviation = true;
    };

    ARGUM_DECLARE_FRIENDLY_NAMES(Tokenizer)
}

#endif


