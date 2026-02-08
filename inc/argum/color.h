//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_COLOR_H_INCLUDED
#define HEADER_ARGUM_COLOR_H_INCLUDED

#include "common.h"
#include "char-constants.h"

namespace Argum {

    ARGUM_MOD_EXPORTED
    enum class Color : unsigned {
        normal = 0,
        bold = 1,
        faint = 2,
        italic = 3,
        underline = 4,
        black = 30,
        red = 31,
        green = 32,
        yellow = 33,
        blue = 34,
        magenta = 35,
        cyan = 36,
        white = 37,
        bright_black = 90,
        grey = bright_black,
        bright_red = 91,
        bright_green = 92,
        bright_yellow = 93,
        bright_blue = 94,
        bright_magenta = 95,
        bright_cyan = 96,
        bright_white = 97
    };

    ARGUM_MOD_EXPORTED
    template<Character Char>
    class BasicColorMaker {
    private:
        using mytype = BasicColorMaker;

        template<size_t N>
        struct Str {
            Char wrapped[N];
        };

        template<size_t N>
        static constexpr auto append(Str<N> prev, Char c) {
            Str<N + 1> ret;
            std::copy(std::begin(prev.wrapped), std::begin(prev.wrapped) + N - 1, std::begin(ret.wrapped));
            ret.wrapped[N - 1] = c;
            ret.wrapped[N] = 0;
            return ret;
        }

        template<Color C, size_t N>
        static constexpr auto encode(Str<N> prev) {
            if constexpr (unsigned(C) < 10) {
                auto ret = mytype::append(prev, Char(CharConstants<Char>::digit_0 + unsigned(C)));
                return ret;
            } else {
                constexpr auto prevDigits = Color(unsigned(C) / 10);
                constexpr auto myDigit = Color(unsigned(C) % 10);
                return mytype::encode<myDigit>(mytype::encode<prevDigits>(prev));
            }
        }

        template<Color First, Color... Rest, size_t N>
        static constexpr auto addNextColor(Str<N> prev) {
            auto withSemicolon = mytype::append(prev, CharConstants<Char>::semicolon);
            auto next = mytype::encode<First>(withSemicolon);
            if constexpr (sizeof...(Rest) != 0) {
                return mytype::addNextColor<Rest...>(next);
            } else {
                return mytype::append(next, CharConstants<Char>::letter_m);
            }
        }

        template<Color First, Color... Rest>
        static constexpr auto doMakeColor() {
            constexpr Str<3> prefix{{CharConstants<Char>::esc, CharConstants<Char>::squareBracketOpen, 0}};
            constexpr auto start = mytype::encode<First>(prefix);

            if constexpr (sizeof...(Rest) != 0) {
                return mytype::addNextColor<Rest...>(start);
            } else {
                return mytype::append(start, CharConstants<Char>::letter_m);
            }
        }

    private:
        template<Color First, Color... Rest>
        static constexpr auto storage = mytype::doMakeColor<First, Rest...>();

    public:
        template<Color First, Color... Rest>
        static constexpr auto make() {
            return std::basic_string_view<Char>(mytype::storage<First, Rest...>.wrapped);
        }
    };

    ARGUM_DECLARE_FRIENDLY_NAMES(ColorMaker)

    ARGUM_MOD_EXPORTED
    template<Character Char, Color First, Color... Rest>
    constexpr auto basicMakeColor() {
        return BasicColorMaker<Char>::template make<First, Rest...>();
    }

    ARGUM_MOD_EXPORTED
    template<Color First, Color... Rest>
    constexpr auto makeColor() {
        return BasicColorMaker<char>::template make<First, Rest...>();
    }

    ARGUM_MOD_EXPORTED
    template<Color First, Color... Rest>
    constexpr auto makeWColor() {
        return BasicColorMaker<wchar_t>::template make<First, Rest...>();
    }


    ARGUM_MOD_EXPORTED
    template<Color First, Color... Rest, Character Char>
    auto colorize(std::basic_string_view<Char> str) -> std::basic_string<Char> {
        auto prefix = basicMakeColor<Char, First, Rest...>();
        auto suffix = basicMakeColor<Char, Color::normal>();

        std::basic_string<Char> ret;
        ret.reserve(prefix.size() + str.size() + suffix.size());
        ret = prefix;
        ret += str;
        ret += suffix;
        return ret;
    }


    ARGUM_MOD_EXPORTED
    template<class Char>
    struct BasicColorScheme {
        using StringViewType = std::basic_string_view<Char>;

        StringViewType heading;
        StringViewType progName;
        StringViewType shortOptionInUsage;
        StringViewType longOptionInUsage;
        StringViewType optionArgInUsage;
        StringViewType positionalInUsage;
        StringViewType shortOption;
        StringViewType longOption;
        StringViewType optionArg;
        StringViewType positional;
    };

    ARGUM_DECLARE_FRIENDLY_NAMES(ColorScheme)

    ARGUM_MOD_EXPORTED
    template<Character Char>
    constexpr BasicColorScheme<Char> nullColorScheme {};

    ARGUM_MOD_EXPORTED
    template<Character Char>
    constexpr BasicColorScheme<Char> basicDefaultColorScheme {
            .heading =              basicMakeColor<Char, Color::bold, Color::blue>(),
            .progName =             basicMakeColor<Char, Color::bold, Color::magenta>(),
            .shortOptionInUsage =   basicMakeColor<Char, Color::green>(),
            .longOptionInUsage =    basicMakeColor<Char, Color::cyan>(),
            .optionArgInUsage =     basicMakeColor<Char, Color::yellow>(),
            .positionalInUsage =    basicMakeColor<Char, Color::green>(),
            .shortOption =          basicMakeColor<Char, Color::bold, Color::green>(),
            .longOption =           basicMakeColor<Char, Color::bold, Color::cyan>(),
            .optionArg =            basicMakeColor<Char, Color::bold, Color::yellow>(),
            .positional =           basicMakeColor<Char, Color::bold, Color::green>(),
    };

    ARGUM_MOD_EXPORTED
    constexpr auto defaultColorScheme() -> const ColorScheme & { return basicDefaultColorScheme<char>; }

    ARGUM_MOD_EXPORTED
    constexpr auto defaultWColorScheme() -> const WColorScheme & { return basicDefaultColorScheme<wchar_t>; }

    ARGUM_MOD_EXPORTED
    template<Character Char>
    class BasicColorizer {
    public:
        using CharType = Char;
        using StringViewType = std::basic_string_view<CharType>;
        using StringType = std::basic_string<CharType>;
        using Scheme = BasicColorScheme<Char>;

    public:
        constexpr BasicColorizer() = default;

        constexpr BasicColorizer(const Scheme & scheme):
            m_scheme(&scheme)
        {}

        
        auto heading(StringViewType str) const -> StringType {
            return this->colorize(str, m_scheme->heading);
        }

        auto progName(StringViewType str) const -> StringType {
            return this->colorize(str, m_scheme->progName);
        }

        auto shortOptionInUsage(StringViewType str) const -> StringType {
            return this->colorize(str, m_scheme->shortOptionInUsage);
        }

        auto longOptionInUsage(StringViewType str) const -> StringType {
            return this->colorize(str, m_scheme->longOptionInUsage);
        }

        auto optionArgInUsage(StringViewType str) const -> StringType {
            return this->colorize(str, m_scheme->optionArgInUsage);
        }

        auto positionalInUsage(StringViewType str) const -> StringType {
            return this->colorize(str, m_scheme->positionalInUsage);
        }

        auto shortOption(StringViewType str) const -> StringType {
            return this->colorize(str, m_scheme->shortOption);
        }

        auto longOption(StringViewType str) const -> StringType {
            return this->colorize(str, m_scheme->longOption);
        }

        auto optionArg(StringViewType str) const -> StringType {
            return this->colorize(str, m_scheme->optionArg);
        }

        auto positional(StringViewType str) const -> StringType {
            return this->colorize(str, m_scheme->positional);
        }
    private:
        static auto colorize(StringViewType str, StringViewType prefix) -> StringType {
            if (!prefix.size())
                return StringType(str);

            auto suffix = basicMakeColor<Char, Color::normal>();

            StringType ret;
            ret.reserve(prefix.size() + str.size() + suffix.size());
            ret = prefix;
            ret += str;
            ret += suffix;
            return ret;
        }

    private:
        const Scheme * m_scheme = &nullColorScheme<Char>;
    };

    ARGUM_DECLARE_FRIENDLY_NAMES(Colorizer)

    ARGUM_MOD_EXPORTED 
    inline constexpr auto defaultColorizer() -> Colorizer {
        return {basicDefaultColorScheme<char>};
    }
    ARGUM_MOD_EXPORTED 
    inline constexpr auto defaultWColorizer() -> WColorizer {
        return {basicDefaultColorScheme<wchar_t>};
    }

    ARGUM_MOD_EXPORTED
    enum class ColorStatus {
        unknown,
        forbidden,
        //use color if isatty() == true or you know that the output 
        //goes to the end user rather than a pipe
        allowed,    
        required    
    };

    // Detect ColorStatus from the environment
    // Logic taken from combination of:
    // https://no-color.org
    // https://force-color.org
    // https://bixense.com/clicolors/
    // https://github.com/chalk/supports-color/blob/main/index.js
    // https://gist.github.com/scop/4d5902b98f0503abec3fcbb00b38aec3
    ARGUM_MOD_EXPORTED
    inline auto environmentColorStatus() -> ColorStatus {
        using namespace std::literals;

        if (auto val = getenv("NO_COLOR"); val && *val)
            return ColorStatus::forbidden;

        if (auto val = getenv("FORCE_COLOR"); val && *val)
            return ColorStatus::required;

        if (auto val = getenv("CLICOLOR_FORCE"); val && *val) {
            if (strcmp(val, "0") == 0 || strcmp(val, "false") == 0)
                return ColorStatus::forbidden;
            return ColorStatus::required;
        }

        if (auto val = getenv("CLICOLOR"); val && *val) {
            if (strcmp(val, "0") == 0 || strcmp(val, "false") == 0)
                return ColorStatus::forbidden;
            return ColorStatus::allowed;
        }

    #ifdef _WIN32
        if (auto val = getenv("WT_SESSION"); val && *val)
            return ColorStatus::suggested;
    #endif

        if (auto val = getenv("COLORTERM"))
            return ColorStatus::allowed;

        if (auto val = getenv("TERM")) {
            std::string_view term(val);

            if (term == "dumb")
                return ColorStatus::forbidden;

            for (auto & exact: {"xterm-256color"sv, "xterm-kitty"sv, "xterm-ghostty"sv, "wezterm"sv}) {
                if (term == exact)
                    return ColorStatus::allowed;
            }
            for (auto & start: {"screen"sv, "xterm"sv, "vt100"sv, "vt220"sv, "rxvt"sv}) {
                if (term.size() >= start.size() && term.substr(start.size()) == start)
                    return ColorStatus::allowed;
            }
            for (auto & inside: {"color"sv, "ansi"sv, "cygwin"sv, "linux"sv}) {
                if (term.find(inside) != term.npos)
                    return ColorStatus::allowed;
            }
            for (auto & end: {"-256"sv, "-256color"sv}) {
                if (term.size() >= end.size() && term.substr(term.size() - end.size()) == end)
                    return ColorStatus::allowed;
            }
        }

        if (auto val = getenv("CI")) {
            for (auto * valid: {"GITHUB_ACTIONS", "GITEA_ACTIONS", "CIRCLECI", "TRAVIS", "APPVEYOR", "GITLAB_CI", "BUILDKITE", "DRONE"}) {
                if (getenv(valid))
                    return ColorStatus::allowed;
            }
        }

        return ColorStatus::unknown;
    }
}

#endif
