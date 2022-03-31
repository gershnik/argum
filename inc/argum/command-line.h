//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_COMMAND_LINE_H_INCLUDED
#define HEADER_ARGUM_COMMAND_LINE_H_INCLUDED

#include "expected.h"
#include "messages.h"
#include "simple-file.h"

#include <string>
#include <string_view>
#include <iterator>
#include <vector>
#include <stack>
#include <span>

namespace Argum {

    ARGUM_MOD_EXPORTED 
    template<class Char>
    auto makeArgSpan(int argc, Char ** argv) -> std::span<const Char *> {
        if (argc > 0)
            return std::span(const_cast<const Char **>(argv + 1), size_t(argc - 1));
        return std::span(const_cast<const Char **>(argv), size_t(0));
    }

    ARGUM_MOD_EXPORTED
    template<class Char>
    class BasicResponseFileReader {
    public:
        using CharType = Char;
        using StringType = std::basic_string<Char>;
        using StringViewType = std::basic_string_view<Char>;

        struct Exception : public BasicParsingException<CharType> {
            ARGUM_IMPLEMENT_EXCEPTION(Exception, BasicParsingException<CharType>, Error::ResponseFileError)

            Exception(const std::filesystem::path & filename_, std::error_code error_): 
                BasicParsingException<CharType>(ErrorCode,
                                                format(Messages<CharType>::errorReadingResponseFile(), filename_.native(), error_.message())),
                filename(filename_),
                error(error_) {
            }
            std::filesystem::path filename;
            std::error_code error;
        };

    public:
        BasicResponseFileReader(char prefix) {
            m_prefixes.emplace_back(1, prefix);
        }
        BasicResponseFileReader(StringType prefix): m_prefixes(1, prefix) {
            m_prefixes.emplace_back(std::move(prefix));
        }
        BasicResponseFileReader(std::initializer_list<StringType> prefixes): m_prefixes(prefixes) {
        }

        auto expand(int argc, CharType ** argv) -> ARGUM_EXPECTED(CharType, std::vector<StringType>) {
            return expand(makeArgSpan(argc, argv));
        }

        template<class Splitter>
        auto expand(int argc, CharType ** argv, Splitter && splitter) -> ARGUM_EXPECTED(CharType, std::vector<StringType>) {
            return expand(makeArgSpan(argc, argv), std::forward<Splitter>(splitter));
        }

        template<ArgRange<CharType> Args>
        auto expand(const Args & args) -> ARGUM_EXPECTED(CharType, std::vector<StringType>) {
            return expand(args, [](StringType && str, auto dest) {
                trimInPlace(str);
                if (str.empty())
                    return;
                *dest = std::move(str);
            });
        }

        template<ArgRange<Char> Args, class Splitter>
        auto expand(const Args & args, Splitter && splitter) -> ARGUM_EXPECTED(CharType, std::vector<StringType>)
            requires(std::is_invocable_v<decltype(splitter), StringType &&, std::back_insert_iterator<std::vector<StringType>>>) {
            
            std::vector<StringType> ret;
            std::stack<StackEntry> stack;

            for(StringViewType arg: args) {

                ARGUM_PROPAGATE_ERROR(this->handleArg(arg, ret, stack, splitter));

                while(!stack.empty()) {

                    auto & entry = stack.top();
                    if (entry.current == entry.items.end()) {
                        stack.pop();
                        continue;
                    }
                    ARGUM_PROPAGATE_ERROR(this->handleArg(*entry.current, ret, stack, std::forward<Splitter>(splitter)));
                    ++entry.current;
                }
            }
            return ret;
        }
    private:
        struct StackEntry {
            std::vector<StringType> items;
            typename std::vector<StringType>::const_iterator current;
        };

        template<class Splitter>
        auto handleArg(StringViewType arg, std::vector<StringType> & dest, 
                       std::stack<StackEntry> & stack, 
                       Splitter && splitter) -> ARGUM_EXPECTED(CharType, void) {

            auto foundIt = std::find_if(this->m_prefixes.begin(), this->m_prefixes.end(), [arg](const StringViewType & prefix) {
                return matchStrictPrefix(arg, prefix);
            });

            if (foundIt != this->m_prefixes.end()) {
                auto filename = arg.substr(foundIt->size());
                StackEntry nextEntry;
                ARGUM_PROPAGATE_ERROR(this->readResponseFile(filename, nextEntry.items, std::forward<Splitter>(splitter)));
                nextEntry.current = nextEntry.items.cbegin();
                stack.emplace(std::move(nextEntry));

            } else {
                dest.emplace_back(arg);
            }
            return ARGUM_VOID_SUCCESS;
        }
    
        template<class Splitter>
        static auto readResponseFile(StringViewType filename, std::vector<StringType> & dest, 
                                     Splitter && splitter) -> ARGUM_EXPECTED(CharType, void){

            std::filesystem::path path(filename);
            std::error_code error;
            SimpleFile file(path, "r", error);
            if (!file)
                ARGUM_THROW(Exception, path, error);

            do {
                StringType line = file.readLine<CharType>(error);
                if (error)
                    ARGUM_THROW(Exception, path, error);
                
                if (!line.empty())
                    splitter(std::move(line), std::back_inserter(dest));
            } while(!file.eof());

            return ARGUM_VOID_SUCCESS;
        }
    private:
        std::vector<StringType> m_prefixes;
    };

    ARGUM_DECLARE_FRIENDLY_NAMES(ResponseFileReader)

}

#endif 
