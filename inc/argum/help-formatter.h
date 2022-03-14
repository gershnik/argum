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

    template<class Char> class BasicOption;
    template<class Char> class BasicPositional;
    template<class Char> class BasicParser;

    ARGUM_MOD_EXPORTED
    template<class Char>
    class BasicHelpFormatter {
    public:
        using CharType = Char;
        using StringViewType = std::basic_string_view<CharType>;
        using StringType = std::basic_string<CharType>;
        using Option = BasicOption<CharType>;
        using Positional = BasicPositional<CharType>;

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
        BasicHelpFormatter(const BasicParser<Char> & parser, StringViewType progName, Layout layout = defaultLayout):
            BasicHelpFormatter(progName, parser.options(), parser.positionals(), layout) {
        }

        BasicHelpFormatter(StringViewType progName, 
                           const std::vector<Option> & options, 
                           const std::vector<Positional> & positionals,
                           Layout layout = defaultLayout):
            m_options(options),
            m_positionals(positionals),
            m_progName(progName),
            m_layout(layout) {
                
            if (m_layout.helpNameMaxWidth == 0)
                m_layout.helpNameMaxWidth = 1;
            if (m_layout.width <= m_layout.helpLeadingGap + m_layout.helpNameMaxWidth + m_layout.helpDescriptionGap)
                m_layout.width = m_layout.helpLeadingGap + m_layout.helpNameMaxWidth + m_layout.helpDescriptionGap + 1;
        }

        auto formatUsage() const -> StringType {
            constexpr auto space = CharConstants::space;
            return wordWrap(StringType(Messages::usageStart()).
                            append(this->m_progName).
                            append({space}).
                            append(this->formatSyntax()), 
                   m_layout.width);
        }

        auto formatHelp() const -> StringType {

            constexpr auto endl = CharConstants::endl;

            StringType ret;

            auto helpContent = this->calculateHelpContent();
            if (helpContent.maxNameLen > m_layout.helpNameMaxWidth)
                helpContent.maxNameLen = m_layout.helpNameMaxWidth;

            if (!this->m_positionals.empty()) {
                ret.append(wordWrap(Messages::positionalHeader(), m_layout.width));
                for(auto & [name, desc]: helpContent.positionalItems) {
                    ret.append({endl}).append(this->formatItemHelp(name, desc, helpContent.maxNameLen));
                }
                ret.append(2, endl);
            }

            if (!this->m_options.empty()) {
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

            auto getSyntax = [](auto & obj) {
                return obj.formatSyntax();
            };

            StringType options = join(this->m_options.begin(), this->m_options.end(), space, getSyntax);
            StringType positionals = join(this->m_positionals.begin(), this->m_positionals.end(), space, getSyntax);

            if (!options.empty()) {
                if (!positionals.empty()) {
                    options += space;
                    options += positionals;
                    return options;
                }
                return options;
            } 
            
            return positionals;
        }
        

        struct HelpContent {
            unsigned maxNameLen = 0;
            std::vector<std::pair<StringType, StringType>> optionItems;
            std::vector<std::pair<StringType, StringType>> positionalItems;
        };
        auto calculateHelpContent() const -> HelpContent {
            
            HelpContent ret;

            std::for_each(this->m_positionals.begin(), this->m_positionals.end(), [&](auto & pos){
                auto name = pos.formatHelpName();
                if (name.length() > ret.maxNameLen)
                    ret.maxNameLen = unsigned(name.length());
                ret.positionalItems.emplace_back(std::move(name), pos.formatHelpDescription());
            });
            std::for_each(this->m_options.begin(), this->m_options.end(), [&](auto & opt){
                auto name = opt.formatHelpName();
                if (name.length() > ret.maxNameLen)
                    ret.maxNameLen = unsigned(name.length());
                ret.optionItems.emplace_back(std::move(name), opt.formatHelpDescription());
            });
            return ret;
        }

        auto formatItemHelp(StringViewType name, 
                            StringViewType description,
                            unsigned maxNameLen) const -> StringType {
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
        const std::vector<Option> & m_options;
        const std::vector<Positional> & m_positionals;
        StringType m_progName;
        Layout m_layout;
    };

    ARGUM_DECLARE_FRIENDLY_NAMES(HelpFormatter)

}


#endif

