#include "test-common.h"

#include <argum/partitioner.h>

#include <doctest/doctest.h>


using namespace Argum;
using namespace std;

using namespace std::literals;

#define RESULT(...) vector<decltype(p)::SizeType>{initializer_list<decltype(p)::SizeType>{__VA_ARGS__}}

#ifndef ARGUM_NO_THROW
    #define EXPECT_SUCCESS(n, expected) {\
        auto res = p.partition(n).value(); \
        REQUIRE(res.size() == p.paritionsCount()); \
        CHECK(res == expected);\
    }
#else
    #define EXPECT_SUCCESS(n, expected) {\
        auto res = p.partition(n).value(); \
        CHECK(res.size() == p.paritionsCount()); \
        if (res.size() != p.paritionsCount()) abort(); \
        CHECK(res == expected);\
    }
#endif

#define EXPECT_FAILURE(n) CHECK(!p.partition(n));

TEST_SUITE("partitioner") {

TEST_CASE( "Empty partitioner" ) {

    Partitioner<unsigned> p;

    CHECK(p.paritionsCount() == 1);
    CHECK(p.minimumSequenceSize() == 0);
    
    EXPECT_SUCCESS(0, RESULT(0));
}

TEST_CASE( "Partitioner {0}" ) {
    Partitioner<int> p;

    p.addRange(0, 0);
    CHECK(p.paritionsCount() == 2);
    CHECK(p.minimumSequenceSize() == 0);
    
    EXPECT_SUCCESS(0, RESULT(0, 0));
    EXPECT_SUCCESS(1, RESULT(0, 1));
    EXPECT_SUCCESS(2, RESULT(0, 2));
}

TEST_CASE( "Partitioner {0,1}" ) {
    Partitioner<short> p;

    p.addRange(0, 1);
    CHECK(p.paritionsCount() == 2);
    CHECK(p.minimumSequenceSize() == 0);

    EXPECT_SUCCESS(0, RESULT(0, 0));
    EXPECT_SUCCESS(1, RESULT(1, 0));
    EXPECT_SUCCESS(2, RESULT(1, 1));
}

TEST_CASE( "Partitioner {1}" ) {
    Partitioner<unsigned long> p;

    p.addRange(1, 1);
    CHECK(p.paritionsCount() == 2);
    CHECK(p.minimumSequenceSize() == 1);

    EXPECT_FAILURE(0);
    EXPECT_SUCCESS(1, RESULT(1, 0));
    EXPECT_SUCCESS(2, RESULT(1, 1));
}

TEST_CASE( "Partitioner +{1}" ) {
    Partitioner<unsigned> p;

    p.addRange(1, p.infinity);
    p.addRange(1, 1);
    CHECK(p.paritionsCount() == 3);
    CHECK(p.minimumSequenceSize() == 2);

    EXPECT_FAILURE(0);
    EXPECT_FAILURE(1);
    EXPECT_SUCCESS(2, RESULT(1, 1, 0));
    EXPECT_SUCCESS(100, RESULT(99, 1, 0));
}

TEST_CASE( "Partitioner {1}+" ) {
    Partitioner<unsigned> p;

    p.addRange(1, 1);
    p.addRange(1, p.infinity);
    CHECK(p.paritionsCount() == 3);
    CHECK(p.minimumSequenceSize() == 2);

    EXPECT_FAILURE(0);
    EXPECT_FAILURE(1);
    EXPECT_SUCCESS(2, RESULT(1, 1, 0));
    EXPECT_SUCCESS(100, RESULT(1, 99, 0));
}

TEST_CASE( "Partitioner {0,2}*{0,2}" ) {
    Partitioner<unsigned> p;

    p.addRange(0, 2);
    p.addRange(0, numeric_limits<unsigned>::max());
    p.addRange(0, 2);
    CHECK(p.paritionsCount() == 4);
    CHECK(p.minimumSequenceSize() == 0);

    EXPECT_SUCCESS(0, RESULT(0,0,0,0));
    EXPECT_SUCCESS(1, RESULT(1,0,0,0));
    EXPECT_SUCCESS(2, RESULT(2,0,0,0));
    EXPECT_SUCCESS(3, RESULT(2,1,0,0));
    EXPECT_SUCCESS(4, RESULT(2,2,0,0));
}

}
