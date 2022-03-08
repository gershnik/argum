//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_FLAT_MAP_H_INCLUDED
#define HEADER_ARGUM_FLAT_MAP_H_INCLUDED

#include "common.h"

#include <vector>
#include <algorithm>

namespace Argum {

    template<class T>
    class FlatSet {
    public:
        using value_type = T;
        using iterator = typename std::vector<value_type>::iterator;
        using const_iterator = typename std::vector<value_type>::const_iterator;

    private:
        struct Comparator {
            template<class X>
            auto operator()(const value_type & lhs, const X & rhs) const {
                return lhs < rhs;
            }
            template<class X>
            auto operator()(const X & lhs, const value_type & rhs) const {
                return lhs < rhs;
            }
            auto operator()(const value_type & lhs, const value_type & rhs) const {
                return lhs < rhs;
            }
        };

    public:
        FlatSet() = default;
        
        FlatSet(const value_type & value): m_data(1, value) {
        }
        FlatSet(value_type && value) {
            this->m_data.emplace_back(std::move(value));
        }
        FlatSet(std::initializer_list<value_type> values) : m_data(values) {
            std::sort(this->m_data.begin(), this->m_data.end(), Comparator());
        }

        auto insert(T val) -> std::pair<iterator, bool> {
            auto it = std::lower_bound(this->m_data.begin(), this->m_data.end(), val, Comparator());
            if (it != this->m_data.end() && *it == val)
                return {std::move(it), false};
            it = this->m_data.emplace(it, std::move(val));
            return {std::move(it), true};
        }

        template<class Arg>
        auto find(const Arg & key) const -> const_iterator {
            auto it = std::lower_bound(this->m_data.begin(), this->m_data.end(), key, Comparator());
            if (it == this->m_data.end() || *it != key)
                return this->m_data.end();
            return it;
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
            template<class X>
            auto operator()(const value_type & lhs, const X & rhs) const {
                return lhs.key() < rhs;
            }
            template<class X>
            auto operator()(const X & lhs, const value_type & rhs) const {
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

        auto valueByIndex(size_t idx) const -> Value & {
            return this->m_data[idx].value();
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
    auto findMatchOrMatchingPrefixRange(const FlatMap<Key, Value> & map, Arg arg) -> 
            std::pair<typename FlatMap<Key, Value>::const_iterator,
                      typename FlatMap<Key, Value>::const_iterator> {
        
        const auto end = map.end();

        auto first = map.lower_bound(arg);

        //at this point it points to element grater or equal than arg
        //so range [it, last) either:
        // 1. starts with an exact match
        // 2. contains a run of items which start with arg followed by ones which don't
        // 3. is empty

        //discard the empty range
        if (first == end)
            return {end, end};

        //if exact match, return it now
        if (first->key() == arg)
            return {first, first + 1};
    
        //figure out case 2 and the end of the prefix run
        auto last = first;
        do {
            auto compRes = last->key().compare(0, arg.size(), arg);
            assert(compRes >= 0); //invariant of range [it, last)
            
            //break if we reached the end of prefix run
            if (compRes != 0)
                break;
            
            ++last;
            
        } while (last != end);

        return {first, last};
    }

}


#endif 
