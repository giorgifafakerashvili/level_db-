#include "counter.cpp"

#include <climits>
#include <iostream>

#include <gtest/gtest.h>

namespace leveldb {

TEST(CounterTest, Test1) {
    Counter64 c;
    ASSERT_EQ(c.get(), 0);
    c.increment();
    ASSERT_EQ(c.get(), 1);
        c.decremnt();
        ASSERT_EQ(c.get(), 0);
        c.decrement(3);
        ASSERT_EQ(c.get(), -3);
        c.increment(1);
        ASSERT_EQ(c.get(), -2);
        c.decrement(-1);
        ASSERT_EQ(c.get(), -1);
        c.increment();
        ASSERT(static_cast<long long>(c), 0);
}

}