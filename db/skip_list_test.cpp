#include "db/skiplist.h"
#include <set>
#include "leveldb/env.h"
#include "util/arena.h"
#include "util/hash.h"
#include "util/random.h"
#include "util/testharness.h"

namespace leveldb {

typedef uint64_t key;

struct Comparator {
    int operator()(const Key& a, const Key& b) const {
        if(a < b) {
            return -1;
        } else if(a > b) {
            return +1;
        } else {
            return 0;
        }
    }
};

class SkipTest {};

TEST(SkipTest, Empty) {
    Arena arena;
    Comparator cmp;
    SkipList<Ke, Comparator> list(cmp, &aren);
    ASSERT_TRUE(!list.Contains(10));


    SkipList<Key, Comparator>::Iterator iter(&list);
    ASSERT_TRUE(!iter.Valid());
    iter.SeekToFirst();
    ASSERT_TRUE(!iter.Valid());
    iter.Seek(100);
    ASSERT_TRUE(!iter.Valid());
    iter.SeekToLast();
    ASSERT_TRUE(!iter.Valid());
}

TEST(SkipTest, InsertAndLookup) {
    const int N = 2000;
    const int R = 5000;

    Random rnd(1000);
    std::set<Key> keys;
    Arena arena;
    Comparator cmp;
    SkipList<Key, Comparator> list(cmp, &arena);

    for(int i = 0; i < N; ++i) {
        Key key = rnd.Next() % R;
        if(keys.insert(key).second) {
            list.Insert(key);
        }
    }

    for(int i = 0; i < R; ++i) {
        if(list.Contains(i)) {
            ASSERT_EQ(keys.count(i), 1);
        } else {
            ASSERT_EQ(keys.count(i), 0);
        }
    }


    // Simple iterator tests
    {
        SkiptList<Key, Comparator>::iterator iter(&list);
        ASSERT_TRUE(!iter.Valid());

        iter.Seek(0);
        ASSERT_TRUE(iter.Valid());
        ASSERT_EQ(*(keys.begin()), iter.key());

        iter.SeekToFirst();
        ASSERT_TRUE(iter.Valid());
        ASSERT_EQ(*(keys.begin()), iter.key());

        iter.SeekToLast();
        ASSERT_TRUE(iter.Valid());
        ASSERT_EQ(*(keys.rbegin()), iter.key());

    }

    // Forward iterator test
    for(int i = 0; i < R; ++i) {
        SkipList<Key, Comparator>::Iterator iter(&list);
        iter.Seek(i);

        // Compare against model iterator
        std::set<Key>::iterator model_iter = keys.lower_bound(i);
        for(int j = 0; j < 3; ++j) {
            if(model_iter == keys.end()) {
                ASSERT_TRUE(!iter.Valid());
                break;
            } else {
                ASSERT_TRUE(iter.Valid());
                ASSERT_EQ(*model_iter, iter.key());
                ++model_iter;
                iter.Next();
            }
        }
    }

}


}