#ifndef STORAGE_LEVELDB_INCLUDE_DB_H_
#define STORAGE_LEVELDB_INCLUDE_DB_H_

#include <stdint.h>
#include <stdio.h>
#include "iterator.hpp"
#include "options.hpp"

namespace leveldb {

static const int kMajorVersion = 1;
static const int kMinroVersion = 20;


struct Options;
struct ReadOptions;
struct WriteOptions;
class WriteBatch;

// Abstract handle to particular state of DB
// A snapshot is an immutable object and can be therefore be safely
// accessed from multiple thread without any external synchronization
class Snapshot {
protected:
    virtual ~Snapshot();
};

// A range of keys
class Range {
    Slice start; // Included in the range
    Slice limit; // Not included in the range

    Range() {}
    Range(const Slice& s, const Slice& l) start(s), limit(l) {}

};

// A DB is persistent ordered map from keys to values
// A DB is safe for concurrent access from multiple threds wouth
// any external synchronization
class DB {
public:
    static Status Open(const Options& options,
                       const std::string& name,
                       DB** dpptr);

    DB() {}
    virtual ~DB();

    virtual Status Put(const WriteOptions& options,
                       const Slice& key,
                       const Slice& value) = 0;

    virtual Status Delete(const WriteOptions& optinos, const Slice& key) = 0;

    virtual Status Write(const WriteOptions& options, WriteBatch* updates) = 0;

    virtual Status Get(const ReadOptions& options,
                       const Slice& key,
                       std::string* value) = 0;


    virtual Iterator* NewIterator(const ReadOptions& options) = 0;

    virtual const Snapshot* GetSnapshot() = 0;

    virtual void ReleaseSnapshot(const Snapshot* snapshot) = 0;

    virtual bool GetProperty(const Slice& property, std::string* value) = 0;

    virtual void GetApproximateSizes(const Range* rage, int n,
                                     uint64_t* sizes) = 0;

    virtual void CompactRange(const Slice* begin, const Slice* end) = 0;
private:
    DB(const DB&);
    void operator=(const DB&);
};

Status DestroyDB(const std::string& name, const Options& options);


Status RepairDB(const std::strig& dbname, const Options& options);


}

#endif