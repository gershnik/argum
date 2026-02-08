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
#include "color.h"

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
        using Colorizer = BasicColorizer<CharType>;

        struct Layout {
            unsigned width = unsigned(-1);
            unsigned helpLeadingGap = 2;
            unsigned helpNameMaxWidth = 20;
            unsigned helpDescriptionGap = 2;
        };
        static constexpr Layout defaultLayout = {};

        struct SubCommandMark {
            size_t positionalIdx = size_t(-1);
            size_t optionIdx = size_t(-1);
        };
    private:
        using CharConstants = Argum::CharConstants<CharType>;
        using Messages = Argum::Messages<CharType>;

    public:
        BasicHelpFormatter(const BasicParser<Char> & parser, StringViewType progName, Layout layout = defaultLayout):
            m_progName(progName),
            m_parser(parser),
            m_layout(layout) {
            
            if (m_layout.helpNameMaxWidth == 0)
                m_layout.helpNameMaxWidth = 1;
            if (m_layout.width <= m_layout.helpLeadingGap + m_layout.helpNameMaxWidth + m_layout.helpDescriptionGap)
                m_layout.width = m_layout.helpLeadingGap + m_layout.helpNameMaxWidth + m_layout.helpDescriptionGap + 1;
        }

        auto formatUsage(const Colorizer & colorizer = {}) const -> StringType {
            return this->formatUsage(std::nullopt, colorizer);
        }

        auto formatUsage(const std::optional<StringType> & subCommand,
                         const Colorizer & colorizer = {}) const -> StringType {
            constexpr auto space = CharConstants::space;
            return wordWrap(colorizer.heading(Messages::usageStart()).
                            append(colorizer.progName(this->m_progName)).
                            append({space}).
                            append(this->formatSyntax(subCommand, colorizer)), 
                   m_layout.width);
        }

        auto formatHelp(const Colorizer & colorizer = {}) const -> StringType {
            return this->formatHelp(std::nullopt, colorizer);
        }

        auto formatHelp(const std::optional<StringType> & subCommand,
                        const Colorizer & colorizer = {}) const -> StringType {

            if (subCommand && this->m_parser.subCommandMark().positionalIdx == size_t(-1))
                ARGUM_INVALID_ARGUMENT("subcommand must be defined to use this function with non null subcommand");

            constexpr auto endl = CharConstants::endl;

            StringType ret;

            auto helpContent = this->calculateHelpContent(bool(subCommand), colorizer);
            if (helpContent.maxNameLen > m_layout.helpNameMaxWidth)
                helpContent.maxNameLen = m_layout.helpNameMaxWidth;

            if (!helpContent.positionalItems.empty()) {
                ret.append(wordWrap(colorizer.heading(Messages::positionalHeader()), m_layout.width));
                for(auto & [name, desc]: helpContent.positionalItems) {
                    ret.append({endl}).append(this->formatItemHelp(name, desc, helpContent.maxNameLen));
                }
                ret.append(2, endl);
            }

            if (!helpContent.optionItems.empty()) {
                ret.append(wordWrap(colorizer.heading(Messages::optionsHeader()), m_layout.width));
                for(auto & [name, desc]: helpContent.optionItems) {
                    ret.append({endl}).append(this->formatItemHelp(name, desc, helpContent.maxNameLen));
                }
                ret.append(2, endl);
            }
            return ret;
        }

        auto formatSyntax(const Colorizer & colorizer = {}) const -> StringType {

            return this->formatSyntax(std::nullopt, colorizer);
        }

        auto formatSyntax(const std::optional<StringType> & subCommand,
                          const Colorizer & colorizer = {}) const -> StringType {

            auto subCommandMark = this->m_parser.subCommandMark();
            if (subCommand && subCommandMark.positionalIdx == size_t(-1))
                ARGUM_INVALID_ARGUMENT("subcommand must be added to use this function with non null subcommand");

            constexpr auto space = CharConstants::space;

            auto getSyntax = [&](auto & obj) {
                return obj.formatSyntax(this->m_parser, colorizer);
            };

            StringType ret = this->appendSyntax(
                    join(this->optionsBegin(false), this->optionsEnd(false), space, getSyntax),
                    join(this->positionalsBegin(false), this->positionalsEnd(false), space, getSyntax)
            );
            if (subCommand) {
                ret = this->appendSyntax(std::move(ret), *subCommand); 
                ret = this->appendSyntax(std::move(ret), join(this->optionsBegin(true), this->optionsEnd(true), space, getSyntax));
                ret = this->appendSyntax(std::move(ret), join(this->positionalsBegin(true), this->positionalsEnd(true), space, getSyntax));
            } else if (subCommandMark.positionalIdx != size_t(-1)) {
                ret = this->appendSyntax(std::move(ret), getSyntax(this->m_parser.positionals()[subCommandMark.positionalIdx]));
            }

            return ret;
        }
        

        struct HelpContent {
            unsigned maxNameLen = 0;
            std::vector<std::pair<StringType, StringType>> optionItems;
            std::vector<std::pair<StringType, StringType>> positionalItems;
        };
        auto calculateHelpContent(bool forSubCommand, 
                                  const Colorizer & colorizer = {}) const -> HelpContent {
            
            HelpContent ret;
            auto subCommandMark = this->m_parser.subCommandMark();

            size_t positionalsSize;
            if (forSubCommand || subCommandMark.positionalIdx == size_t(-1))
                positionalsSize = this->m_parser.positionals().size();
            else 
                positionalsSize = subCommandMark.positionalIdx + 1;
            for(size_t i = 0; i < positionalsSize; ++ i) {
                if (forSubCommand && i == subCommandMark.positionalIdx)
                    continue;
                auto & pos = this->m_parser.positionals()[i];
                auto name = pos.formatHelpName(this->m_parser, colorizer);
                auto length = stringWidth(name);
                if (length > ret.maxNameLen)
                    ret.maxNameLen = length;
                ret.positionalItems.emplace_back(std::move(name), pos.formatHelpDescription());
            }
            std::for_each(this->m_parser.options().begin(), this->optionsEnd(forSubCommand), [&](auto & opt){
                auto name = opt.formatHelpName(this->m_parser, colorizer);
                auto length = stringWidth(name);
                if (length > ret.maxNameLen)
                    ret.maxNameLen = length;
                ret.optionItems.emplace_back(std::move(name), opt.formatHelpDescription());
            });
            return ret;
        }

        auto formatItemHelp(StringViewType name, 
                            StringViewType description,
                            unsigned maxNameLen) const -> StringType {
            constexpr auto space = CharConstants::space;
            constexpr auto endl = CharConstants::endl;

            auto descColumnOffset = this->m_layout.helpLeadingGap + maxNameLen + this->m_layout.helpDescriptionGap;

            StringType ret = indent(wordWrap(StringType(this->m_layout.helpLeadingGap, space).append(name), this->m_layout.width), this->m_layout.helpLeadingGap);
            auto lastEndlPos = ret.rfind(endl);
            auto lastLineLen = stringWidth(StringViewType(ret.c_str() + (lastEndlPos + 1), ret.size() - (lastEndlPos + 1)));

            if (lastLineLen > maxNameLen + this->m_layout.helpLeadingGap) {
                ret += endl;
                ret.append(descColumnOffset, space);
            } else {
                ret.append(descColumnOffset - lastLineLen, space);
            }

            ret.append(indent(wordWrap(description, this->m_layout.width - descColumnOffset), descColumnOffset));

            return ret;
        }

        auto optionsBegin(bool forSubCommand) const {
            if (!forSubCommand)
                return  this->m_parser.options().begin();
            auto subCommandMark = this->m_parser.subCommandMark();
            if (subCommandMark.optionIdx != size_t(-1))
                return this->m_parser.options().begin() + subCommandMark.optionIdx;
            return this->m_parser.options().end();
        }
        auto optionsEnd(bool forSubCommand) const {
            auto subCommandMark = this->m_parser.subCommandMark();
            if (forSubCommand || subCommandMark.optionIdx == size_t(-1))
                return this->m_parser.options().end();
            return this->m_parser.options().begin() + subCommandMark.optionIdx;
        }
        auto positionalsBegin(bool forSubCommand) const {
            if (!forSubCommand)
                return this->m_parser.positionals().begin();
            auto subCommandMark = this->m_parser.subCommandMark();
            if (subCommandMark.positionalIdx != size_t(-1))
                return this->m_parser.positionals().begin() + subCommandMark.positionalIdx + 1;
            return this->m_parser.positionals().end();
        }
        auto positionalsEnd(bool forSubCommand) const {
            auto subCommandMark = this->m_parser.subCommandMark();
            if (forSubCommand || subCommandMark.positionalIdx == size_t(-1))
                return this->m_parser.positionals().end();
            return this->m_parser.positionals().begin() + subCommandMark.positionalIdx;
        }

        static auto appendSyntax(StringType base, StringType addend) -> StringType {
            
            constexpr auto space = CharConstants::space;

            if (!base.empty()) {
                if (!addend.empty()) {
                    base += space;
                    base += std::move(addend);
                    return base;
                }
                return base;
            } 
            
            return addend;
        }
    private:
        StringType m_progName;
        const BasicParser<Char> & m_parser;
        Layout m_layout;
    };

    ARGUM_DECLARE_FRIENDLY_NAMES(HelpFormatter)

}


#endif

