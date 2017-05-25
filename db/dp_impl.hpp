#ifndef STORAGE_LEVELDB_DB_DB_IMPL_H_
#define STORAGE_LEVELDB_DB_DB_IMPL_

#include <deque>
#include <set>
#include "db/dbformat.h"
#include "db/log_writer.h"
#include "db/snapshot.h"
#include "leveldb/db.h"
#include "leveldb/env.h"
#include "port/port.h"
#include "port/thread_annotations.h"

namespace leveldb {

class MemTable;
class TableCache;
class Version;
class VersionEdit;
class VersionSet;

class DBImpl : public DB {
public:
    DBImpl(const Options& options, const std::string& dbname);
    virtual ~DBImpl();

    // Implementations of the DB interface
    virtual Status Put(const WriteOptions&, const Slice& key, const Slice& value);
    virtual Status Delete(const WriteOptions&, const Slice& key);
    virtual Status Write(const WriteOptions& options, WriteBatch* updates);
    virtual Status Get(const ReadOptions& options,
                       const Slice& key,
                       std::string* value);
    virtual Iterator* NewIterator(const ReadOptions&);
    virtual const Snapshot* GetSnapshot();
    virtual void ReleaseSnapshot(const Snapshot* snapshot);
    virtual bool GetProperty(const Slice& property, std::string* value);
    virtual void GetApproximateSizes(const Range* range, int n, uint64_t* sizes);
    virtual void CompactRange(const Slice* begin, const Slice* end);

    // Extra methods (for testing) that are not in the public DB interface

    // Compact any files in the named level that overlap [*begin, *end]
    void TEST_CompactRange(int level, const Slice* begin, const Slice* end);

    // Force current memtable contents to be compacted
    Status TEST_CompactMemTable();


    Iterator* TEST_NewInternalIterator();

    int64_t TEST_MaxNextLevelOverlappingBytes();

    // Record a sample
    void RecordReadSample(Slice key);

private:
    friend class DB;
    struct CompactState;
    struct Writer;


};


}


#endif