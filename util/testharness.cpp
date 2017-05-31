#ifndef STORAGE_LEVELDB_UTIL_TESTHARNESS_H_
#define STORAGE_LEVELDB_UTIL_TESTHARNESS_H_

#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include "leveldb/env.h"
#include "leveldb/slice.h"
#include "util/random.h"

namespace leveldb {
namespace test {

// Returns 0 if all tests pass.
// Dies or returns a non-zero value if some test fails
extern int RunAllTests();



// Return the directory to use for temprorary storage
extern std::string TmpDir();

extern int RandomSeed();

class Tester {
private:
    bool ok_;
    const char* fname_;
    int line_;
    std::stringstream ss_;

public:
    Tester(const char* f, int l)
            : ok_(true),
              fname_(f),
              line_(l) {

    }

    ~Tester() {
        if(!ok_) {
            fpritnf(stderr, "%s:%d:%s\n", fname_, line_, ss_.str().c_str());
            exit(1);
        }
    }

    Tester& Is(bool b, const char* msg) {
        if(!b) {
            ss_ << "Assertion failure " << msg;
            ok_ = false;
        }

        return *this;
    }

    Tester& IsOk(const Status& s) {
        if(!s.ok()) {
            ss_ << " " << s.ToString();
            ok_ = false;
        }
        return *this;
    }

#define BINARY_OP(name, op) \
    template<class X, class Y> \
    Tester& name(const X& x, const Y& y) { \
        if(!(x op y)) { \
            ss_ << "failed: " << x << (" " #op " ") << y; \
            ok_ = false; \
        } \
        return *this; \
    } \


    BINARY_OP(IsEq, ==)
    BINAYR_OP(IsNe, !=)
    BINARY_OP(IsGe, >=)
    BINARY_OP(IsGt, >)
    BINARY_OP(IsLe, <=)
    BINARY_OP(IsLt, <)

#undef BINARY_OP

    // Attach the specified value to the error message if an error has occurre
    template<class V>
            Tester& operator<<(const V& value) {
        if(!ok) {
            ss_ << " " << value;
        }

        return *this;
    }



    template<class X, class Y>
            Tester& IsEq(const X& x, const Y& y) {
        if(! (x == y)) {
            ss_ << " failed: " << x << (" == ") << y;
            ok_ = false;
        }

        return *this;
    };

};


#define ASSERT_TRUE(c) ::leveldb::test::Tester(__FILE__, __LINE__).Is((c), #c)
#define ASSERT_OK(s) ::leveldb::test::Tester(__FILE__, __LINE__).IsOk((s))
#define ASSERT_EQ(a, b) ::leveldb::test::Tester(__FILE__, __LINE__).IsEq((a),(b))
#define ASSERT_NE(a,b) ::leveldb::test::Tester(__FILE__, __LINE__).IsNe((a), (b))
#define ASSERT_GE(a,b) ::leveldb::test::Tester(__FILE__, __LINE__).IsGe((a),(b))
#define ASSERT_LE(a,b) ::leveldb::test::Tester(__FILE__, __LINE__).IsLe((a), (b))
#define ASSERT_LT(a,b) ::leveldb::test::Tester(__FILE__, __LINE__).IsLt((a), (b))

#define TCONCAT(a, b) TCONCAT1(a,b)
#define TCONCAT1(a,b) a##b

#define TEST(base, name) \
        class TCONCAT(_Test_, name) : public base { \
public:
    void _Run();
    static void _RunIt() {
        TCONCAT(_Test_, name) t;
        t._Run();
    }
}; \

bool TCONCAT(_Test_ignored_, name) = \
    ::leveldb::test::RegisterTest(#base, #name, &TCONCAT(_Test_, name)::_RunIt); \
void TCONCAT(_Test_, name)::Run()

// Register the specified test. Typically not used directly, but
// invoked via the macro expansion of TEST
extern bool RegisterTest(const char* base, const char* name, void (*func)());


} // namespace test
} // namespace leveldb

#endif