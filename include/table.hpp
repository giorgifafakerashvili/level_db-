#ifndef STORAGE_LEVELDB_INCLUDE_TABLE_H_
#define STORAGE_LEVELDB_INCLUDE_TABLE_H_

#include <stdint.h>
#include "iterator.hpp"

namespace leveldb {

    class Block;

    class BlockHandle;

    class Footer;

    class Options;

    class RandomAccessFile;

    class RandomOptions;

    class TableCache;

// A table is sorted map from strings to strings. Tables
// are immutable and presistent.A table may be safely
// accessed from multiple threads without external synchronization

class Table {

    static Status Open(const Options& options,
                       RandomAccessFile* file,
                       uint64_t file_size,
                       Table** table);

    ~Table();


    Iterator* NewIterator(const ReadOperations&) const;

    uint64_t ApproximateOffsetOf(const Slice& key) const;

private:

    struct Rep;
    Rep* rep_;

    explicit Table(Rep* rep)  { rep_ = rep; }
    static Iterator* BlockReader(void*,const ReadOptions&, const Slice&);

    friend class TableCache;

    status InternalGet(const ReadOptions&, const Slice& key,
                       void* arg,
                       void (*handle_result)(void* arg, const Slice& k, const Slice& v));

    void ReadMeta(const Footer& footer);
    void ReadFilter(const Slice& filter_handle_value);

    Table(const Table&);
    void operator=(const Table&);

};

}

#endif