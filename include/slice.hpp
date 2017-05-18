#ifndef STORAGE_LEVEL_INCLUDE_SLICE_H_
#define STORAGE_LEVEL_INCLUDE_SLICE_H_

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <string>

namespace leveldb {


class Slice
{
public:
    // Create an empty slice
    Slice() : data_(""), size_(0) {}

    // Create a slice that refers to d[0, n-1]
    Slice(const char* d, std::size_t n) : data_(d), size_(n) {}

    Slice(const std::string& s) : data_(s.data()), size_(s.size()) {}

    Slice(const char* s) : data_(s), size_(strlen(s)) {}

    // Returns pointer to the begining of the referenced data
    const char* data() const { return data_; }

    // Returns the length (in bytes) of the referenced data
    std::size_t size() const { return size_; }

    char operator[](std::size_t n) const {
        assert(n < size());
        return data_[n];
    }

    // change the slice to refer to an emtpy array
    void clear() { data_ = ""; size_ = 0; }

    // Drop the first "n" bytes from this slice.
    void remove_prefix(std::size_t n) {
        assert(n <= size());
        data_ += n;
        size_ -= n;
    }

    std::string ToString() const { return std::string(data_, size_); }

    /**
     * Three way comparison
     *  < 0 iff "tis" < b
     *  == 0 iff "this" == b
     *  > 0 "iff" this > b
     */
     int compare(const Slice& b) const;

    // Returns true iff "x"
    bool starts_with(const Slice& x) const {
        return ((size_ >= x.size_) && (memcmp(data_, x.data_, x.size_) == 0));
    }
private:
    const char* data_;
    std::size_t size_;

    // Intentionally copyable
};


inline bool operator==(const Slice& x, const Slice& y) {
    return ((x.size() == y.size()) && (
            memcmp(x.data(), y.data(), x.size()) == 0
                                      ));
}

inline bool operator!=(const Slice& x, const Slice& y) {
    return !(x == y);
}

inline int Slice::compare(const Slice& b) const {
    const std::size_t min_len = (size_ < b.size_) ? size_ : b.size_;
    int r = memcmp(data_, b.data_, min_len);
    if(r == 0) {
        if(size_ < b.size_) r = -1;
        else if(size_ > b.size_) r = +1;
    }
    return r;
}

}

#endif