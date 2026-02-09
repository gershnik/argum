//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_DETECT_COLOR_H_INCLUDED
#define HEADER_ARGUM_DETECT_COLOR_H_INCLUDED

#include "color.h"

#include <string_view>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#if !defined(_WIN32) && __has_include(<unistd.h>)
    #define ARGUM_HAS_UNISTD_H
    #include <unistd.h>
#endif

#if !defined(_WIN32) && __has_include(<sys/ioctl.h>)
    #include <sys/ioctl.h>
    #ifdef TIOCGWINSZ
        #define ARGUM_HAS_TIOCGWINSZ
    #endif
#endif

#ifdef _WIN32
    #include <io.h>
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>
    #ifdef min
        #undef min
    #endif
    #ifdef max
        #undef max
    #endif
#endif

namespace Argum {
    
    ARGUM_MOD_EXPORTED
    enum class ColorStatus {
        unknown,
        forbidden,
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
    // https://andrey-zherikov.github.io/argparse/ansi-coloring-and-styling.html#heuristic
    ARGUM_MOD_EXPORTED
    inline auto environmentColorStatus() -> ColorStatus {
        using namespace std;
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
            return ColorStatus::allowed;
#endif

        if (auto val = getenv("COLORTERM"); val && *val)
            return ColorStatus::allowed;

        if (auto val = getenv("TERM")) {
            string_view term(val);

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

        return ColorStatus::unknown;
    }

    //Portable isatty
    ARGUM_MOD_EXPORTED
    inline bool isAtTty(FILE * fp) {
#if defined(ARGUM_HAS_UNISTD_H)
        return isatty(fileno(fp));
#elif defined(_WIN32)
        return _isatty(_fileno(fp));
#else
        return false;
#endif
    }


    ARGUM_MOD_EXPORTED
    inline bool shouldUseColor(ColorStatus envColorStatus, FILE * fp) {
        if (envColorStatus == ColorStatus::required)
            return true;
        if (envColorStatus == ColorStatus::forbidden)
            return false;

#ifndef _WIN32
        if (envColorStatus == ColorStatus::unknown)
            return false;
#endif

        if (!isAtTty(fp))
            return false;

#ifdef _WIN32
        if (envColorStatus == ColorStatus::unknown) {
        
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut == INVALID_HANDLE_VALUE)
                return false;

            DWORD dwMode = 0;
            if (!GetConsoleMode(hOut, &dwMode))
                return false;

            if (!(dwMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING))
                return false;
        }
#endif

        return true;
    }

    ARGUM_MOD_EXPORTED
    inline auto colorizerForFile(ColorStatus envColorStatus, FILE * fp) -> Colorizer {
        if (shouldUseColor(envColorStatus, fp))
            return defaultColorizer();
        return {};
    }

    ARGUM_MOD_EXPORTED
    inline unsigned terminalWidth(FILE * fp) {
        unsigned fallback = std::numeric_limits<unsigned>::max();

#if defined(ARGUM_HAS_UNISTD_H) && defined(ARGUM_HAS_TIOCGWINSZ) 
        struct winsize ws{};

        int desc = fileno(fp);
        if (isatty(desc) &&
            ioctl(desc, TIOCGWINSZ, &ws) == 0 &&
            ws.ws_col > 0) {

            return ws.ws_col;
        }

        if (auto * cols = getenv("COLUMNS")) {
            char * end;
            auto val = CharConstants<char>::toULong(cols, &end, 10);
            if (val > 0 && val < std::numeric_limits<unsigned>::max() &&
                end == cols + strlen(cols))

                return val;
        }

        return fallback;

#elif defined(_WIN32)

        if (!_isatty(_fileno(fp)))
            return fallback;

        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h == INVALID_HANDLE_VALUE)
            return fallback;

        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (!GetConsoleScreenBufferInfo(h, &csbi))
            return fallback;

        return csbi.srWindow.Right - csbi.srWindow.Left + 1;

#else
        return fallback;
#endif
    }

}

#endif
