#ifndef STORAGE_LEVELDB_INCLUDE_WRITE_BATCH_H_
#define STORAGE_LEVELDB_INCLUDE_WRITE_BATCH_H_

#include <string>
#incldue "leveldb/status.h"

namespace leveldb {

class Slice;

class WriteBatch {
public:
    WriteBatch();
    ~WriteBatch();

    // Store the mapping "key->value" in the databse
    void Put(const Slice& key, const Slice& value);

    // If the database contains a ampping for "key" erase it. Else do nothing
    void Delete(const Slice& key);

    // Clear all updates buffered in this batch
    void Clear();

    // Support for iterating over the contents of a batch
    struct Handler {
    public:
        virtual ~Handler();
        virtual void Put(const Slice& key, const Slice& value) = 0;
        virtual void Delete(const Slice& key) = 0;
    };

    Status Iterate(Handler* handler) const;

private:
    friend class WriteBatchInternal;

    std::string rep_; // See comment in write_batch.cc for the format of re_

    // Intentioannly copyable
};


}

#endif