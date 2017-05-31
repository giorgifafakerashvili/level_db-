namespace leveldb {

class Counter64 {
public:
    // Atomically increment value
    void increment(uint64_t n = 1) {
        _counter.addAndFetch(n);
    }

    // Atomically decrement value
    void decrement(uint64_t n = 1) {
        _counter.subtractAndFetch(n);
    }

    // Return the current value
    long long get() const {
        return _counter.load();
    }


    operator long long() const {
        return get();
    }

private:
    AtomicInt64 _counter;
};

}