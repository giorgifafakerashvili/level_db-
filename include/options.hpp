#ifndef STORAGE_LEVELDB_INCLUDE_OPTIONS_H_
#define STORAGE_LEVELDB_INCLUDE_OPTIONS_H_

namespace leveldb {

class Cache;
class Comparator;
class Env;
class FilterPolicy;
class Logger;
class Snapshot;


enum CompressionType {
    kNoCompression = 0x0,
    kSnappyCompression = 0x1
};

// Options to control the behavior of a database (passed DB::open)
struct Options {
    const Comparator* comparator;

    bool create_if_missing;

    bool error_if_exists;

    bool paranoid_checks;

    Env* env;

    Logger* info_log;

    std::size_t write_buffer_size;

    int max_open_files;

    Cache* block_cache;

    std::size_t block_size;

    int block_restart_interval;

    std::size_t max_file_size;

    CompressionType compression;

    bool reuse_logs;

    const FilterPolicy* filter_policy;

    Options();
};


struct ReadOptions {
    bool verify_checksums;

    bool fill_cache;

    const Snapshot* snapshot;

    ReadOptions()
            : verify_checksums(false),
              fill_cache(true),
              snapshot(NULL) {

    }

};


struct WriteOptions {
    bool sync;

    WriteOptions()
            : sync(false) {

    }
};

}

#endif