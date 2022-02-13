#ifndef HEADER_MARGP_FLAT_MAP_H_INCLUDED
#define HEADER_MARGP_FLAT_MAP_H_INCLUDED

#include "common.h"

#include <vector>

namespace MArgP {

    template<class Key, class Value>
    class FlatMap {
    public:
        class value_type : private std::pair<Key, Value> {
            
        public:
            using std::pair<Key, Value>::pair;
            
            const Key & key() const noexcept {
                return this->first;
            }
            
            const Value & value() const noexcept {
                return this->second;
            }
            
            Value & value() noexcept {
                return this->second;
            }
        };
        using iterator = typename std::vector<value_type>::iterator;
        using const_iterator = typename std::vector<value_type>::const_iterator;
    private:
        struct Comparator {
            auto operator()(const value_type & lhs, const auto & rhs) {
                return lhs.key() < rhs;
            }
            auto operator()(const auto & lhs, const value_type & rhs) {
                return lhs < rhs.key();
            }
            auto operator()(const value_type & lhs, const value_type & rhs) {
                return lhs.key() < rhs.key();
            }
        };
    public:
        auto add(Key key, Value val) -> std::pair<iterator, bool> {
            auto it = std::lower_bound(this->m_data.begin(), this->m_data.end(), key, Comparator());
            if (it != this->m_data.end() && it->key() == key)
                return {std::move(it), false};
            it = this->m_data.emplace(it, std::move(key), std::move(val));
            return {std::move(it), true};
        }

        template<class KeyArg>
        auto operator[](const KeyArg & key) -> Value & {
            auto it = std::lower_bound(this->m_data.begin(), this->m_data.end(), key, Comparator());
            if (it == this->m_data.end() || it->key() != key)
                it = this->m_data.emplace(it, std::move(key), Value());
            return it->value();
        }

        template<class KeyArg>
        auto find(const KeyArg & key) const -> const_iterator {
            auto it = std::lower_bound(this->m_data.begin(), this->m_data.end(), key, Comparator());
            if (it == this->m_data.end() || it->key() != key)
                return this->m_data.end();
            return it;
        }

        template<class KeyArg>
        auto lower_bound(const KeyArg & key) const -> const_iterator {
            return std::lower_bound(this->m_data.begin(), this->m_data.end(), key, Comparator());
        }

        iterator begin() noexcept { return this->m_data.begin(); }
        const_iterator begin() const noexcept { return this->m_data.begin(); }
        const_iterator cbegin() const noexcept { return this->m_data.begin(); }
        iterator end() noexcept { return this->m_data.end(); }
        const_iterator end() const noexcept { return this->m_data.end(); }
        const_iterator cend() const noexcept { return this->m_data.end(); }

        size_t size() const { return m_data.size(); }
        size_t empty() const { return m_data.empty(); }

        void clear() { m_data.clear(); }

    private:
        std::vector<value_type> m_data;
    };

    template<class Key, class Value, class Arg>
    auto findShortestMatchingPrefix(const FlatMap<Key, Value> & map, Arg arg) -> typename FlatMap<Key, Value>::const_iterator {
        
        const auto last = map.end();

        auto ret = last;
        
        auto it = map.lower_bound(arg);

        //at this point it points to element grater or equal than arg
        //so range [it, last) either:
        // 1. starts with an exact match
        // 2. contains a run of items which start with arg followed by ones which don't
        // 3. is empty

        //discard the empty range
        if (it == last)
            return last;

        //if exact match, return it now
        if (it->key() == arg)
            return it;
    
        //figure out case 2 and the *shortest* match in it
        do {
            //we reached one with no prefix
            auto compRes = it->key().compare(0, arg.size(), arg);
            assert(compRes >= 0); //invariant of range [it, last)
            
            //break if we reached the end of prefix run
            if (compRes != 0)
                break;

            //pick the shortest
            if (ret == last || ret->key().size() > it->key().size())
                ret = it;
            
            ++it;
            
        } while (it != last);

        return ret;
    }

}


#endif 
