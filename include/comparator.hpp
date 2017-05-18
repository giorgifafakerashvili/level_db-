#ifndef STORAGE_LEVELDB_INCLUDE_COMPARATOR_H_
#define STORAGE_LEVELDB_INCLUDE_COMPARATOR_H_

namespace leveldb {

class Slice;

class Comparator {
public:
    virtual ~Comparator();

    virtual int Compare(const Slice& a, const Slice& b) const = 0;

    virtual const char* Name() const = 0;

    virtual void FindShortestSeparator(const std::string* start, const Slice& limit) const = 0;

    virtual void FndShortestSuccessor(const std::string* key) const = 0;

};

extern const Comparator* BytewiseComparator();

}

#endif // STORAGE_LEVELDB_INCLUDE_COMPARATOR_H_