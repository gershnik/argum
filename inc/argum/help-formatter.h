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

        struct SubCommandMark {
            size_t positionalIdx = size_t(-1);
            size_t optionIdx = size_t(-1);
        };
    private:
        using CharConstants = Argum::CharConstants<CharType>;
        using Messages = Argum::Messages<CharType>;

    public:
        BasicHelpFormatter(const BasicParser<Char> & parser, StringViewType progName, Layout layout = defaultLayout):
            BasicHelpFormatter(progName, parser.options(), parser.positionals(), parser.subCommandMark(), layout) {
        }

        BasicHelpFormatter(StringViewType progName, 
                           const std::vector<Option> & options, 
                           const std::vector<Positional> & positionals,
                           SubCommandMark subCommandMark,
                           Layout layout = defaultLayout):
            m_progName(progName),
            m_options(options),
            m_positionals(positionals),
            m_subCommandMark(subCommandMark),
            m_layout(layout) {
                
            if (m_layout.helpNameMaxWidth == 0)
                m_layout.helpNameMaxWidth = 1;
            if (m_layout.width <= m_layout.helpLeadingGap + m_layout.helpNameMaxWidth + m_layout.helpDescriptionGap)
                m_layout.width = m_layout.helpLeadingGap + m_layout.helpNameMaxWidth + m_layout.helpDescriptionGap + 1;
        }

        auto formatUsage() const -> StringType {
            return this->formatUsage(std::nullopt);
        }

        auto formatUsage(const std::optional<StringType> & subCommand) const -> StringType {
            constexpr auto space = CharConstants::space;
            return wordWrap(StringType(Messages::usageStart()).
                            append(this->m_progName).
                            append({space}).
                            append(this->formatSyntax(subCommand)), 
                   m_layout.width);
        }

        auto formatHelp() const -> StringType {
            return this->formatHelp(std::nullopt);
        }

        auto formatHelp(const std::optional<StringType> & subCommand) const -> StringType {

            if (subCommand && this->m_subCommandMark.positionalIdx == size_t(-1))
                ARGUM_INVALID_ARGUMENT("subcommand must be defined to use this function with non null subcommand");

            constexpr auto endl = CharConstants::endl;

            StringType ret;

            auto helpContent = this->calculateHelpContent(bool(subCommand));
            if (helpContent.maxNameLen > m_layout.helpNameMaxWidth)
                helpContent.maxNameLen = m_layout.helpNameMaxWidth;

            if (!helpContent.positionalItems.empty()) {
                ret.append(wordWrap(Messages::positionalHeader(), m_layout.width));
                for(auto & [name, desc]: helpContent.positionalItems) {
                    ret.append({endl}).append(this->formatItemHelp(name, desc, helpContent.maxNameLen));
                }
                ret.append(2, endl);
            }

            if (!helpContent.optionItems.empty()) {
                ret.append(wordWrap(Messages::optionsHeader(), m_layout.width));
                for(auto & [name, desc]: helpContent.optionItems) {
                    ret.append({endl}).append(this->formatItemHelp(name, desc, helpContent.maxNameLen));
                }
                ret.append(2, endl);
            }
            return ret;
        }

        auto formatSyntax() const -> StringType {

            return this->formatSyntax(std::nullopt);
        }

        auto formatSyntax(const std::optional<StringType> & subCommand) const -> StringType {

            if (subCommand && this->m_subCommandMark.positionalIdx == size_t(-1))
                ARGUM_INVALID_ARGUMENT("subcommand must be added to use this function with non null subcommand");

            constexpr auto space = CharConstants::space;

            auto getSyntax = [](auto & obj) {
                return obj.formatSyntax();
            };

            StringType ret = this->appendSyntax(
                    join(this->optionsBegin(false), this->optionsEnd(false), space, getSyntax),
                    join(this->positionalsBegin(false), this->positionalsEnd(false), space, getSyntax)
            );
            if (subCommand) {
                ret = this->appendSyntax(std::move(ret), *subCommand); 
                ret = this->appendSyntax(std::move(ret), join(this->optionsBegin(true), this->optionsEnd(true), space, getSyntax));
                ret = this->appendSyntax(std::move(ret), join(this->positionalsBegin(true), this->positionalsEnd(true), space, getSyntax));
            } else if (this->m_subCommandMark.positionalIdx != size_t(-1)) {
                ret = this->appendSyntax(std::move(ret), getSyntax(this->m_positionals[this->m_subCommandMark.positionalIdx]));
            }

            return ret;
        }
        

        struct HelpContent {
            unsigned maxNameLen = 0;
            std::vector<std::pair<StringType, StringType>> optionItems;
            std::vector<std::pair<StringType, StringType>> positionalItems;
        };
        auto calculateHelpContent(bool forSubCommand) const -> HelpContent {
            
            HelpContent ret;

            size_t positionalsSize;
            if (forSubCommand || this->m_subCommandMark.positionalIdx == size_t(-1))
                positionalsSize = this->m_positionals.size();
            else 
                positionalsSize = this->m_subCommandMark.positionalIdx + 1;
            for(size_t i = 0; i < positionalsSize; ++ i) {
                if (forSubCommand && i == this->m_subCommandMark.positionalIdx)
                    continue;
                auto & pos = this->m_positionals[i];
                auto name = pos.formatHelpName();
                if (name.length() > ret.maxNameLen)
                    ret.maxNameLen = unsigned(name.length());
                ret.positionalItems.emplace_back(std::move(name), pos.formatHelpDescription());
            }
            std::for_each(this->m_options.begin(), this->optionsEnd(forSubCommand), [&](auto & opt){
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

            auto descColumnOffset = this->m_layout.helpLeadingGap + maxNameLen + this->m_layout.helpDescriptionGap;

            StringType ret = indent(wordWrap(StringType(this->m_layout.helpLeadingGap, space).append(name), this->m_layout.width), this->m_layout.helpLeadingGap);
            auto lastEndlPos = ret.rfind(endl);
            auto lastLineLen = ret.size() - (lastEndlPos + 1);

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
                return  this->m_options.begin();
            if (this->m_subCommandMark.optionIdx != size_t(-1))
                return this->m_options.begin() + this->m_subCommandMark.optionIdx;
            return this->m_options.end();
        }
        auto optionsEnd(bool forSubCommand) const {
            if (forSubCommand || this->m_subCommandMark.optionIdx == size_t(-1))
                return this->m_options.end();
            return this->m_options.begin() + this->m_subCommandMark.optionIdx;
        }
        auto positionalsBegin(bool forSubCommand) const {
            if (!forSubCommand)
                return this->m_positionals.begin();
            if (this->m_subCommandMark.positionalIdx != size_t(-1))
                return this->m_positionals.begin() + this->m_subCommandMark.positionalIdx + 1;
            return this->m_positionals.end();
        }
        auto positionalsEnd(bool forSubCommand) const {
            if (forSubCommand || this->m_subCommandMark.positionalIdx == size_t(-1))
                return this->m_positionals.end();
            return this->m_positionals.begin() + this->m_subCommandMark.positionalIdx;
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
        const std::vector<Option> & m_options;
        const std::vector<Positional> & m_positionals;
        SubCommandMark m_subCommandMark;
        Layout m_layout;
    };

    ARGUM_DECLARE_FRIENDLY_NAMES(HelpFormatter)

}


#endif

