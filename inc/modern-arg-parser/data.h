#ifndef HEADER_MARGP_DATA_H_INCLUDED
#define HEADER_MARGP_DATA_H_INCLUDED

#include "char-constants.h"
#include "formatting.h"

#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <limits>

namespace MArgP {

    template<class Char>
    class BasicOptionNames final {
    
    public:
        using CharType = Char;
        using StringType = std::basic_string<Char>;
        using StringViewType = std::basic_string_view<Char>;
        using CharConstants = MArgP::CharConstants<Char>;
    public:
        template<class First, class... Rest>
        requires(std::is_convertible_v<First, StringViewType> && (std::is_convertible_v<Rest, StringViewType> && ...))
        BasicOptionNames(First && first, Rest && ...rest):
            m_values{std::forward<First>(first), std::forward<Rest>(rest)...} {
        }

        auto main() const -> const StringType & {
            return this->m_values[0];
        }

        auto all() const -> const std::vector<StringType> & {
            return this->m_values;
        }


    private:
        std::vector<StringType> m_values;
    };

    MARGP_DECLARE_FRIENDLY_NAMES(OptionNames)

    enum class OptionArgument  {
        None,
        Optional,
        Required
    };

    class Repeated {
    public:
        static constexpr unsigned infinity = std::numeric_limits<unsigned>::max();

        constexpr explicit Repeated(unsigned val): m_min(val), m_max(val) {
        }
        constexpr Repeated(unsigned min, unsigned max): m_min(min), m_max(max) {
            MARGP_ALWAYS_ASSERT(min <= max);
        }

        static const Repeated zeroOrOnce;
        static const Repeated once;
        static const Repeated zeroOrMore;
        static const Repeated oneOrMore;

        constexpr auto min() const {
            return this->m_min;
        }

        constexpr auto max() const {
            return this->m_max;
        }

    private:
        unsigned m_min = 0;
        unsigned m_max = 0;
    };

    inline constexpr Repeated Repeated::zeroOrOnce = Repeated(0, 1);
    inline constexpr Repeated Repeated::once       = Repeated(1, 1);
    inline constexpr Repeated Repeated::zeroOrMore = Repeated(0, Repeated::infinity);
    inline constexpr Repeated Repeated::oneOrMore  = Repeated(1, Repeated::infinity);

    template<class Char>
    class BasicParsingException : public std::runtime_error {
    public:
        auto message() const -> std::basic_string_view<Char> {
            return m_message;
        }
        
    protected:
        BasicParsingException(std::basic_string_view<Char> message) : 
            std::runtime_error(toString<char>(message)),
            m_message(message) {
        }

    private:
        std::basic_string<Char> m_message;
    };

    template<>
    class BasicParsingException<char> : public std::runtime_error {
    public:
        auto message() const -> std::string_view {
            return what();
        }
    protected:
        BasicParsingException(std::string_view message) : 
            std::runtime_error(std::string(message)) {
        }
    };
    
    MARGP_DECLARE_FRIENDLY_NAMES(ParsingException)

    template<class It1, class It2, class Joiner>
    auto join(It1 first, It2 last, Joiner joiner) -> decltype(*first + joiner) {
        
        decltype(*first + joiner) ret;

        if (first == last)
            return ret;
        
        ret += *first;
        for(++first; first != last; ++first) {
            ret += joiner;
            ret += *first;
        };
        return ret;
    }
    
    template<class Char>
    auto trimInPlace(std::basic_string<Char> & str) -> std::basic_string<Char> & {
        auto firstNotSpace = std::find_if(str.begin(), str.end(), [](const auto c) {
            return !CharConstants<Char>::isSpace(c);
        });
        str.erase(str.begin(), firstNotSpace);
        auto lastNotSpace = std::find_if(str.begin(), str.end(), [](const auto c) {
            return CharConstants<Char>::isSpace(c);
        });
        str.erase(lastNotSpace, str.end());
        return str;
    }
}


#endif
