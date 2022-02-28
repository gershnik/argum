//
// Copyright 2022 Eugene Gershnik
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://github.com/gershnik/argum/blob/master/LICENSE
//
#ifndef HEADER_ARGUM_PARTITIONER_H_INCLUDED
#define HEADER_ARGUM_PARTITIONER_H_INCLUDED

#include "common.h"

#include <type_traits>
#include <span>
#include <vector>
#include <optional>
#include <algorithm>


namespace Argum {

    // Given:
    //   1. A length of a sequence N and
    //   2. A list of M pairs {A,B} each denoting a range of minimum A and maximum B elements (A >=0 && A <= B) 
    // This class finds a partition of range {N1, N2, N3, ..., Nm+1} such as:
    //   a. N1 + N2 + N3 + ... + Nm + Nm+1 == N
    //   b. Ai <= Ni <= Bi for i <= m
    // The last partion: Nm+1 is the "remainder", if any.
    // The partitioning is "greedy" - each range consumes as much as it can from left to right
    template<class S>
    requires(std::is_integral_v<S>)
    class Partitioner {
    public:
        using SizeType = S;
        
        static constexpr auto infinity = std::numeric_limits<SizeType>::max();
    
    public:
        auto addRange(SizeType a, SizeType b) -> void {
            if constexpr (std::is_signed_v<SizeType>)
                assert(a >= 0);
            assert(a <= b);

            SizeType length;
            if (b != infinity)
                length = b - a;
            else
                length = infinity;
            this->m_ranges.push_back({a, length});
            this->m_minimumExpected += a;
        }

        //returns M + 1: e.g. the number of added ranges + 1
        auto paritionsCount() -> size_t {
            return this->m_ranges.size() + 1;
        }

        //The minimum size of sequence that can be partitioned
        auto minimumSequenceSize() const {
            return this->m_minimumExpected;
        }

        //Returns false if n < minimumSequenceSize()
        auto partition(SizeType n) -> std::optional<std::vector<SizeType>> {
            if (n < this->m_minimumExpected)
                return std::nullopt;
            //this "rebases" the sequence so now all we need is to match ranges of {0, length1}, {0, length2},...
            n -= this->m_minimumExpected;
            
            //because all ranges have minimum 0 and matching is greedy we can just assign
            //smaller of length and remaining n to each adding its minimum
            std::vector<SizeType> results(this->paritionsCount());
            std::transform(this->m_ranges.begin(), this->m_ranges.end(), results.begin(), [&n](const auto range) {
                auto length = std::min(n, range.second);
                n -= length;
                return SizeType(range.first + length);
            });
            results.back() = n;
            return results;
        }

    private:
        std::vector<std::pair<SizeType, SizeType>> m_ranges;
        SizeType m_minimumExpected = 0;
    };
}

#endif
