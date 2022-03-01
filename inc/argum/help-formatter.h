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
#include <algorithm>

namespace Argum {

    template<class Parser>
    class HelpFormatter {
    public:
        using CharType = typename Parser::CharType;
        using StringViewType = std::basic_string_view<CharType>;
        using StringType = std::basic_string<CharType>;

        struct Layout {
            unsigned width = 80;
            unsigned helpLeadingGap = 2;
            unsigned helpNameMaxWidth = 20;
            unsigned helpDescriptionGap = 2;
        };
        static constexpr Layout defaultLayout = {};
    private:
        using CharConstants = Argum::CharConstants<CharType>;
        using Messages = Argum::Messages<CharType>;

    public:
        HelpFormatter(const Parser & parser, StringViewType progName, Layout layout = defaultLayout):
            m_parser(parser),
            m_progName(progName),
            m_layout(layout) {
                
            if (m_layout.helpNameMaxWidth == 0)
                m_layout.helpNameMaxWidth = 1;
            if (m_layout.width <= m_layout.helpLeadingGap + m_layout.helpNameMaxWidth + m_layout.helpDescriptionGap)
                m_layout.width = m_layout.helpLeadingGap + m_layout.helpNameMaxWidth + m_layout.helpDescriptionGap + 1;
        }

        auto formatUsage() const -> StringType {
            return wordWrap(StringType(Messages::usageStart()).append(this->formatSyntax()), m_layout.width);
        }

        auto formatHelp() const -> StringType {

            constexpr auto endl = CharConstants::endl;

            StringType ret = this->formatUsage();
            ret.append(2, endl);

            auto helpContent = this->calculateHelpContent();
            if (helpContent.maxNameLen > m_layout.helpNameMaxWidth)
                helpContent.maxNameLen = m_layout.helpNameMaxWidth;

            if (!this->m_parser.positionals().empty()) {
                ret.append(wordWrap(Messages::positionalHeader(), m_layout.width));
                for(auto & [name, desc]: helpContent.positionalItems) {
                    ret.append({endl}).append(this->formatItemHelp(name, desc, helpContent.maxNameLen));
                }
                ret.append(2, endl);
            }

            if (!this->m_parser.options().empty()) {
                ret.append(wordWrap(Messages::optionsHeader(), m_layout.width));
                for(auto & [name, desc]: helpContent.optionItems) {
                    ret.append({endl}).append(this->formatItemHelp(name, desc, helpContent.maxNameLen));
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
            std::vector<std::pair<StringType, StringType>> optionItems;
            std::vector<std::pair<StringType, StringType>> positionalItems;
        };
        auto calculateHelpContent() const -> HelpContent {
            
            HelpContent ret;

            std::for_each(this->m_parser.positionals().begin(), this->m_parser.positionals().end(), [&](auto & pos){
                auto name = pos.formatHelpName();
                if (name.length() > ret.maxNameLen)
                    ret.maxNameLen = name.length();
                ret.positionalItems.emplace_back(std::move(name), pos.formatHelpDescription());
            });
            std::for_each(this->m_parser.options().begin(), this->m_parser.options().end(), [&](auto & opt){
                auto name = opt.formatHelpName();
                if (name.length() > ret.maxNameLen)
                    ret.maxNameLen = name.length();
                ret.optionItems.emplace_back(std::move(name), opt.formatHelpDescription());
            });
            return ret;
        }

        auto formatItemHelp(StringViewType name, 
                            StringViewType description,
                            size_t maxNameLen) const -> StringType {
            constexpr auto space = CharConstants::space;
            constexpr auto endl = CharConstants::endl;

            auto descColumnOffset = m_layout.helpLeadingGap + maxNameLen + m_layout.helpDescriptionGap;

            StringType ret = indent(wordWrap(StringType(m_layout.helpLeadingGap, space).append(name), m_layout.width), m_layout.helpLeadingGap);
            auto lastEndlPos = ret.rfind(endl);
            auto lastLineLen = ret.size() - (lastEndlPos + 1);

            if (lastLineLen > maxNameLen + m_layout.helpLeadingGap) {
                ret += endl;
                ret.append(descColumnOffset, space);
            } else {
                ret.append(descColumnOffset - lastLineLen, space);
            }

            ret.append(indent(wordWrap(description, m_layout.width - descColumnOffset), descColumnOffset));

            return ret;
        }
    private:
        const Parser & m_parser;
        StringType m_progName;
        Layout m_layout;
    };

}


#endif

