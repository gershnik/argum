//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_SIMPLE_FILE_H_INCLUDED
#define HEADER_ARGUM_SIMPLE_FILE_H_INCLUDED

#include "char-constants.h"

#include <filesystem>
#include <system_error>

#include <stdio.h>

namespace Argum {

    class SimpleFile {
    public:
        SimpleFile(const std::filesystem::path & path, const char * mode, std::error_code & ec) {
        #ifndef _MSC_VER
            m_fp = fopen(path.string().c_str(), mode);
        #else
            m_fp = _wfopen(path.native().c_str(), toString<wchar_t>(mode).c_str());
        #endif
            if (!m_fp) {
                int err = errno;
                ec = std::make_error_code(static_cast<std::errc>(err));
            }
        }
        SimpleFile(SimpleFile && src) {
            m_fp = src.m_fp;
            src.m_fp = nullptr;
        }
        auto operator=(SimpleFile && src) -> SimpleFile & {
            if (m_fp)
                fclose(m_fp);
            m_fp = nullptr;
            m_fp = src.m_fp;
            return *this;
        }
        ~SimpleFile() {
            if (m_fp)
                fclose(m_fp);
        }
        SimpleFile(const SimpleFile &) = delete;
        SimpleFile & operator=(const SimpleFile &) = delete;

        operator bool() const {
            return m_fp != nullptr;
        }
        auto eof() const -> bool {
            return feof(m_fp) != 0;
        }

        template<Character Char>
        std::basic_string<Char> readLine(std::error_code & ec) {
            std::basic_string<Char> buf;
            for ( ; ; ) {
                Char c;
                if constexpr (std::is_same_v<Char, char>) {
                    int res = fgetc(m_fp);
                    if (res == EOF) {
                        checkError(ec);
                        break;
                    }
                    c = Char(res);
                } else if constexpr (std::is_same_v<Char, wchar_t>) {
                    wint_t res = fgetwc(m_fp);
                    if (res == WEOF) {
                        checkError(ec);
                        break;
                    }
                    c = Char(res);
                }
                
                if (c == CharConstants<Char>::endl)
                    break;
                buf += c;
            }
            return buf;
        }
    private:
        auto checkError(std::error_code & ec) const -> bool{
            if (ferror(m_fp)) {
                int err = errno;
                 ec = std::make_error_code(static_cast<std::errc>(err));
                 return true;
            }
            return false;
        }
    private:
        FILE * m_fp;
    };
}


#endif