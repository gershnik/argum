//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_TYPE_PARSERS_H_INCLUDED
#define HEADER_ARGUM_TYPE_PARSERS_H_INCLUDED

#include "parser.h"

#include <string>
#include <type_traits>

namespace Argum {

    template<class T, StringLike S> 
    requires(std::is_integral_v<T>)
    auto parseIntegral(S && str, int base = 0) -> T {

        using Char = CharTypeOf<S>;
        using ValidationError = typename BasicParser<Char>::ValidationError;
        using CharConstants = CharConstants<Char>;
        using Messages = Messages<Char>;

        std::basic_string<Char> value(std::forward<S>(str));

        if (value.empty())
            throw ValidationError(format(Messages::notANumber(), value));
        
        T ret;
        Char * endPtr;

        errno = 0;
        if constexpr (std::is_signed_v<T>) {

            if constexpr (sizeof(T) <= sizeof(long)) {
                long res = CharConstants::toLong(value.data(), &endPtr, base);
                if constexpr (sizeof(T) < sizeof(res)) {
                    if (res < (long)std::numeric_limits<T>::min() || res > (long)std::numeric_limits<T>::max())
                        throw ValidationError(format(Messages::outOfRange(), value));
                }
                ret = T(res);
            } else  {
                long long res = CharConstants::toLongLong(value.data(), &endPtr, base);
                if constexpr (sizeof(T) < sizeof(res)) {
                    if (res < (long long)std::numeric_limits<T>::min() || res > (long long)std::numeric_limits<T>::max())
                        throw ValidationError(format(Messages::outOfRange(), value));
                }
                ret = T(res);
            }

        } else {

            if constexpr (sizeof(T) <= sizeof(unsigned long)) {
                unsigned long res = CharConstants::toULong(value.data(), &endPtr, base);
                if constexpr (sizeof(T) < sizeof(res)) {
                    if (res > (long)std::numeric_limits<T>::max())
                        throw ValidationError(format(Messages::outOfRange(), value));
                }
                ret = T(res);
            } else  {
                unsigned long long res = CharConstants::toULongLong(value.data(), &endPtr, base);
                if constexpr (sizeof(T) < sizeof(res)) {
                    if (res > (long long)std::numeric_limits<T>::max())
                        throw ValidationError(format(Messages::outOfRange(), value));
                }
                ret = T(res);
            }
        }
        if (errno == ERANGE)
            throw ValidationError(format(Messages::outOfRange(), value));
        for ( ; endPtr != value.data() + value.size(); ++endPtr) {
            if (!CharConstants::isSpace(*endPtr))
                throw ValidationError(format(Messages::notANumber(), value));
        }
        
        return ret;
    }

}

#endif