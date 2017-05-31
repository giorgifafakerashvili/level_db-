#include "db/db_impl.h"

#include <algorithm>
#include <set>
#include <string>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include "db/builder.h"
#include "db/db_iter.h"
#include "db/dbformat.h"
#include "db/filename.h"
#include "db/log_reader.h"
#include "db/log_writer.h"
#include "db/memtable.h"
#include "db/table_cache.h"
#include "db/version_set.h"
#include "db/write_batch_internal.h"
#include "leveldb/db.h"
#include "leveldb/env.h"
#include "leveldb/status.h"
#include "leveldb/table.h"
#include "leveldb/table_builder.h"
#include "port/port.h"
#include "table/block.h"
#include "table/merger.h"
#include "table/two_level_iterator.h"
#include "util/coding.h"
#include "util/logging.h"
#include "util/mutexlock.h"



DBImpl::DBImpl(const Options& raw_options,
               const std::string& dbname)
        : env_(raw_options.env),
          internal_comparator_(raw_options.comparator),
          internal_filter_policy_(raw_options.filter_policy),
          options_(SanitizeOptions(dbname, &internal_comparator,
                                   &internal_filter_policy_, raw_options)),
          owns_info_log_(options_.info_log != raw_options.info_log),
          owns_cache_(options_.block_cache != raw_options.block_cache),
          dbname_(dbname),
          db_lock_(NULL),
          shutting_down_(NULL),
          bg_cv_(&mutex_),
          mem_(NULL),
          logfile_number_(0),
          log_(NULL),
          seed_(0),
          tmp_batch_(new WriteBatch),
          bg_compaction_scheduled_(false),
          manual_compaction_(NULL) {
    has_imm_.Release_Store(NULL);

    // Reserve ten filesor so for other uses and give the rest to TableCache
    const int table_cache_size = options_.max_open_files - kNumberNonTableCacheFiles;
    table_cache_ = new TableCache(dbname_, &options_, table_cache_size);

    versions_ = new VersionSet(dbname_, &options_, table_cache_,
                               &internal_comparator_);
}


DBImpl::~DBImpl() {
    // Wait for background work to finish
    mutex_.Lock();
    shutting_down_.Release_Store(this); // Any non-NULL value is ok
    while(bg_compaction_scheduled_) {
        bg_cv_.Wait();
    }
    mutex_.Unlock();

    if(db_lock != NULL) {
        evn_->UnlockFile(db_lock_);
    }

    delete versions_;

    if(mem != NULL) mem_->Unref();
    if(imm != NULL) emm_->Unref();

    delete tmp_batch_;
    delete log_;
    delete logfile_;
    delete table_cache_;

    if(owns_info_log_) {
        delete options_.info_log;
    }

    if(owns_cache_) {
        delete options_.block_cache;
    }

}

Status DB::Open(const Options& options, const std::string& dbname,
                DB** dbptr) {
    *dbptr = NULL;

    DBImpl* impl = new DBImpl(options, dbname);
    impl->mutex_.Lock();
    VersionEdit edit;
    // Recover handlers create_if_missing, error_if_exists
    bool save_manifest = false;
    Status s = impl->Recover(&edit, &save_manifest);
    if(s.ok() && impl->mem_ == NULL) {
        // Create new log and a corresponding memtable
        uin64_t new_log_number = impl->versios_->NewFileNumber();
        WritableFile* lfile;
        s = options.env->NewWritableFile(LogFileName(dbname, new_log_number),
                                         &lfile);

        if(s.ok()) {
            edit.SetLogNumber(new_log_number);
            impl->logfile_ = lfile;
            impl->log_ = new Log::Writer(lfile);
            impl->mem_ = new MemTable(impl->internal_comparator_);
            impl->mem_->Ref();
        }
    }

    if(s.ok() && save_manifest) {
        edit.SetPrevLogNumber(0); // No older logs needed after recovery
        edit.SetLogNumber(impl->logfile_number_);
        s = impl->versions_->LogAndApply(&edit, &impl->mutex_);
    }

    if(s.ok()) {
        impl->DeleteObsoleteFiles();
        impl->MaybeScheduleCompaction();
    }

    impl->mutex_.unlock();

    if(s.ok()) {
        assert(impl->mem_ != NULL);
        *dbptr = impl;
    } else {
        delete impl;
    }

    return s;
}


Snapshot::~Snapshot() {
}

