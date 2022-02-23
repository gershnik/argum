#ifndef HEADER_MARGP_PARTITIONER_H_INCLUDED
#define HEADER_MARGP_PARTITIONER_H_INCLUDED

#include "common.h"

#include <type_traits>
#include <span>
#include <vector>
#include <optional>


namespace MArgP {

    // Given:
    //   1. A length of a sequence N and
    //   2. A list of M pairs {A,B} each denoting a range of minimum A and maximum B elements (A >=0 && A <= B) 
    // This class finds a partition of range {N1, N2, N3, ..., Nm+1} such as:
    //   a. N1 + N2 + N3 + ... + Nm + Nm+1 == N
    //   b. Ai <= Ni <= Bi for i <= m
    // The last partion: Nm+1 is the "remainder", if any.
    template<class SizeType>
    requires(std::is_integral_v<SizeType>)
    class Partitioner {
    private:
        static constexpr auto infinity = std::numeric_limits<SizeType>::max();
    public:
        auto addRange(SizeType a, SizeType b) -> void {
            if constexpr (std::is_signed_v<SizeType>)
                assert(a >= 0);
            assert(a <= b);

            this->m_minima.push_back(a);
            if (b != infinity)
                this->m_lengthes.push_back(b - a);
            else
                this->m_lengthes.push_back(infinity);
            this->m_minimumExpected += a;
        }

        //returns M + 1: e.g. the number of added ranges + 1
        auto paritionsCount() -> size_t {
            return this->m_minima.size() + 1;
        }

        //The minimum size of sequence that can be partitioned
        auto minimumSequenceSize() const {
            return this->m_minimumExpected;
        }

        //Returns false if n < minimumSequenceSize()
        auto partition(SizeType n) -> std::optional<std::vector<SizeType>> {
            if (n < this->m_minimumExpected)
                return std::nullopt;
            n -= this->m_minimumExpected;

            std::vector<SizeType> results(this->paritionsCount());
            
            struct State {
                SizeType n;
                SizeType length;
                bool lengthLoopStarted = false;
            };

            std::vector<State> stack;
            stack.reserve(results.size());
            stack.push_back(State{n, 0, false});
            bool lastResult = false;
            
            for( ; ; ) {
                if (stack.empty())
                    return results;

                size_t idx = stack.size() - 1;
                auto & state = stack[idx];

                if (lastResult) {
                    results[idx] = state.length + this->m_minima[idx];
                    stack.pop_back();
                    continue;
                } 

                if (!state.lengthLoopStarted) {
                    
                    if (idx == results.size() - 1) {
                        results[idx] = state.n;
                        lastResult = true;
                        stack.pop_back();
                        continue;
                    }

                    state.length = std::min(this->m_lengthes[idx], state.n);
                    state.lengthLoopStarted = true;
                    lastResult = false;
                    stack.push_back(State{SizeType(state.n - state.length), 0, false});
                    continue;
                }
                

                if (state.length == 0) {
                    assert(!stack.empty());
                    lastResult = false;
                    stack.pop_back();
                    continue;
                }

                --state.length;
                lastResult = false;
                stack.push_back(State{SizeType(state.n - state.length), 0, false});
            }
        }

    private:
        std::vector<SizeType> m_minima;
        std::vector<SizeType> m_lengthes;
        SizeType m_minimumExpected = 0;
    };
}

#endif
