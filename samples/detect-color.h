//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_DETECT_COLOR_H_INCLUDED
#define HEADER_ARGUM_DETECT_COLOR_H_INCLUDED

#include <string_view>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
    #include <unistd.h>
#else
    #include <io.h>
    #include <Windows.h>
#endif

namespace Argum {
    
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
    bool isAtTty(FILE * fp) {
#ifndef _WIN32
        return isatty(fileno(fp));
#else
        return _isatty(_fileno(fp));
#endif
    }


    bool shouldUseColor(ColorStatus envColorStatus, FILE * fp) {
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

}

#endif
