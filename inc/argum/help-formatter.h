//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_HELP_FORMATTER_H_INCLUDED
#define HEADER_ARGUM_HELP_FORMATTER_H_INCLUDED

#include "messages.h"
#include "formatting.h"

#include <string_view>
#include <string>
#include <vector>

namespace Argum {

    template<class Parser>
    class HelpFormatter {
    public:
        using CharType = typename Parser::CharType;
        using StringViewType = std::basic_string_view<CharType>;
        using StringType = std::basic_string<CharType>;
    private:
        using CharConstants = Argum::CharConstants<CharType>;
        using Messages = Argum::Messages<CharType>;

    public:
        HelpFormatter(const Parser & parser, StringViewType progName):
            m_parser(parser),
            m_progName(progName) {    
        }

        auto formatUsage() const -> StringType {
            return StringType(Messages::usageStart()).append(this->formatSyntax());
        }

        auto formatHelp() const -> StringType {

            constexpr auto endl = CharConstants::endl;

            StringType ret = Messages::usageStart();
            ret.append(this->formatSyntax()).append(2, endl);

            auto helpContent = this->calculateHelpContent();
            if (helpContent.maxNameLen > 21)
                helpContent.maxNameLen = 21;

            if (!this->m_parser.positionals().empty()) {
                ret.append(Messages::positionalHeader());
                for(size_t idx = 0; idx < this->m_parser.positionals().size(); ++idx) {
                    auto & pos = this->m_parser.positionals()[idx];
                    auto & name = helpContent.positionalNames[idx];
                    ret.append({endl}).append(this->formatItemHelp(name, pos.formatHelpDescription(), 2, helpContent.maxNameLen));
                }
                ret.append(2, endl);
            }

            if (!this->m_parser.options().empty()) {
                ret.append(Messages::optionsHeader());
                for(size_t idx = 0; idx < this->m_parser.options().size(); ++idx) {
                    auto & opt = this->m_parser.options()[idx];
                    auto & name = helpContent.optionNames[idx];
                    ret.append({endl}).append(this->formatItemHelp(name, opt.formatHelpDescription(), 2, helpContent.maxNameLen));
                }
                ret.append(2, endl);
            }
            return ret;
        }

        auto formatSyntax() const -> StringType {

            constexpr auto space = CharConstants::space;

            StringType ret = this->m_progName;

            for(auto & opt: this->m_parser.options())
                ret.append({space}).append(opt.formatSyntax());

            for (auto & pos: this->m_parser.positionals())
                ret.append({space}).append(pos.formatSyntax());
            
            return ret;
        }
        

        struct HelpContent {
            size_t maxNameLen = 0;
            std::vector<StringType> optionNames;
            std::vector<StringType> positionalNames;
        };
        auto calculateHelpContent() const -> HelpContent {
            
            HelpContent ret;

            std::for_each(this->m_parser.positionals().begin(), this->m_parser.positionals().end(), [&](auto & pos){
                auto name = pos.formatHelpName();
                if (name.length() > ret.maxNameLen)
                    ret.maxNameLen = name.length();
                ret.positionalNames.emplace_back(std::move(name));
            });
            std::for_each(this->m_parser.options().begin(), this->m_parser.options().end(), [&](auto & opt){
                auto name = opt.formatHelpName();
                if (name.length() > ret.maxNameLen)
                    ret.maxNameLen = name.length();
                ret.optionNames.emplace_back(std::move(name));
            });
            return ret;
        }

        auto formatItemHelp(StringViewType name, 
                            StringViewType description, 
                            size_t leadingSpaceCount,
                            size_t maxNameLen) const -> StringType {
            constexpr auto space = CharConstants::space;
            constexpr auto endl = CharConstants::endl;

            StringType ret;

            ret.append(leadingSpaceCount, space).append(name);
            if (name.length() > maxNameLen) {
                ret += endl;
            } else {
                ret.append(maxNameLen - name.length() + 2, space);
            }
            ret += description;

            return Indent<CharType>{unsigned(maxNameLen + leadingSpaceCount + 2)}.apply(ret);
        }
    private:
        const Parser & m_parser;
        StringType m_progName;
    };

}


#endif

