#include <modern-arg-parser/partitioner.h>

#include "catch.hpp"


using namespace MArgP;
using namespace std;

using namespace std::literals;

TEST_CASE( "Empty partitioner", "[partitioner") {

    Partitioner<unsigned> p;

    CHECK(p.paritionsCount() == 1);
    CHECK(p.minimumSequenceSize() == 0);
    auto res = p.partition(0).value();
    REQUIRE(res.size() == p.paritionsCount());
    CHECK(res[0] == 0);
}

TEST_CASE( "Empty Range partitioner", "[partitioner") {
    Partitioner<int> p;

    p.addRange(0, 0);
    CHECK(p.paritionsCount() == 2);
    CHECK(p.minimumSequenceSize() == 0);

    auto res = p.partition(0).value();
    REQUIRE(res.size() == p.paritionsCount());
    CHECK(res[0] == 0);
    CHECK(res[1] == 0);

    res = p.partition(1).value();
    CHECK(res[0] == 0);
    CHECK(res[1] == 1);
}

TEST_CASE( "Non-empty Range partitioner", "[partitioner") {
    Partitioner<short> p;

    p.addRange(0, 1);
    CHECK(p.paritionsCount() == 2);
    CHECK(p.minimumSequenceSize() == 0);

    auto res = p.partition(0).value();
    REQUIRE(res.size() == p.paritionsCount());
    CHECK(res[0] == 0);
    CHECK(res[1] == 0);

    res = p.partition(1).value();
    CHECK(res[0] == 1);
    CHECK(res[1] == 0);

    res = p.partition(2).value();
    CHECK(res[0] == 1);
    CHECK(res[1] == 1);
}


