#ifndef HEADER_MARGP_DATA_H_INCLUDED
#define HEADER_MARGP_DATA_H_INCLUDED

#include "char-constants.h"

#include <string>
#include <set>

namespace MArgP {

    template<class Char>
    class BasicOptionName final {
    
    public:
        using CharType = Char;
        using StringType = std::basic_string<Char>;
        using StringViewType = std::basic_string_view<Char>;
        using CharConstants = CharConstants<Char>;
    public:
        template<class First, class... Rest>
        requires(std::is_convertible_v<First, StringViewType> && (std::is_convertible_v<Rest, StringViewType> && ...))
        BasicOptionName(First && first, Rest && ...rest) {
            int nameLevel = 0;
            init(nameLevel, std::forward<First>(first), std::forward<Rest>(rest)...);
        }

        auto name() const -> const StringType & {
            return this->m_name;
        }

        auto shortOptions() const -> const std::set<StringType> & {
            return this->m_shortOptions;
        }

        auto longOptions() const -> const std::set<StringType> & {
            return this->m_longOptions;
        }


    private:
        template<class Param>
        auto init(int & nameLevel, Param && param) -> void {
            
            add(nameLevel, param);
        }

        template<class First, class... Rest>
        auto init(int & nameLevel, First && first, Rest && ...rest) -> void {
            
            add(nameLevel, std::forward<First>(first));
            init(nameLevel, std::forward<Rest>(rest)...);
        }

        auto add(int & nameLevel, StringViewType opt) -> void {
            assert(opt.size() > 1 && opt[0] == CharConstants::optionStart);
            
            int currentNameLevel = 0;
            if (opt[1] == CharConstants::optionStart) {
                assert(opt.size() > 2);
                opt.remove_prefix(2);
                this->m_longOptions.emplace(opt);
                currentNameLevel = 2;
            } else {
                opt.remove_prefix(1);
                this->m_shortOptions.emplace(opt);
                currentNameLevel = 1;
            }
            
            if (nameLevel < currentNameLevel) {
                this->m_name = opt;
                nameLevel = currentNameLevel;
            }
        }

    private:
        StringType m_name;
        std::set<StringType> m_shortOptions;
        std::set<StringType> m_longOptions;
    };

    enum class OptionArgument  {
        None,
        Optional,
        Required
    };

    MARGP_DECLARE_FRIENDLY_NAMES(OptionName);
}


#endif
