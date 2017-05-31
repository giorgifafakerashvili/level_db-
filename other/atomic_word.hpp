namespace leveldb {

/*
 * Implemenation of the AtomicWord interface in term of the c++11 Atomics,
 */

template<typename _WordType>
class AtomicWord {
public:
    // unredlying value type
    typedef _WordType WordType;

    explicit constexpr AtomicWord(WordType value = WordType(0)) : value_(value) {}

    WordType Load() const {
        return _value.Load();
    }

    WordType LoadRelaxed() const {
        return value_.load(std::memory_order_relaxed);
    }

    void Store(WordType new_value) {
        return value_.store(new_value);
    }


};
}