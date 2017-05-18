#ifndef STORAGE_LEVEL_INCLUDE_CACHE_H_
#define STORAGE_LEVEL_INCLUDE_CACHE_H_

#include <stdint.h>
#include "slice.hpp"


namespace leveldb
{
class Cache;

// Create a new cache with a fixed size capacity. This implementation
// of Cache uses a least-recently-used eviction policy
extern Cache* NewLRUCache(std::size_t capacity);

class Cache {
public:
    Cache() {}

    // Destroy all existing entries by calling the deleter
    // function that was passed to the constructor
    virtual ~Cache() {}

    // Opaque handle to an entry stored in the cache
    struct Handle {};

    virtual Handle* Insert(const Slice& key, void* value, std::size_t charge,
                           void (*deleter)(const Slice& key, void* value)) = 0;

    virtual Handle* Lookup(const Slice& key) = 0;

    virtual void Release(Handle* handle) = 0;

    virtual void* Value(Handle* handle) = 0;

    virtual void Erase(const Slice& key) = 0;

    virtual uint64_t NewId() = 0;

    virtual void Prune() {}

    virtual std::size_t TotalCharge() const = 0;

private:
    void LRU_Remove(Handle* e);
    void LRU_Append(Handle* e);
    void Unref(Handle* e);

    struct Rep;
    Rep* rep_;

    // No copying allowed
    Cache(const Cache&);
    void operator=(const Cache&);

};

}


#endif