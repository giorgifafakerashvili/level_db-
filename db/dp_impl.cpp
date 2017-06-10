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

const int kNumNotTableCacheFiles = 10;

// Information kept for every waiting writer
struct DBImpl::Writer {
    Status status;
    WriteBatch* batch;
    bool sync;
    bool done;
    port::CondVar cv;

    explicit Writer(port::Mutex* mu) : cv(mu) {}
};

struct DBImpl::CompactionState {
    Compaction* const compaction;

    SequenceNumber smallest_snapshot;

    struct Output {
        uint64_t number;
        uint64_t file_size;
        InternalKey smallest, largest;
    };


    std::vector<Output> outputs;

    // State kept for output being generated
    WritableFile* outfile;
    TableBuilder* builder;

    Output* current_output()  { return &outputs[outputs.size()-1]; };

    explicit CompactionState(Compaction* c)
        : compaction(c),
          outfile(NULL),
          builder(NULL),
          total_bytes() {

    }
};

// Fix user-supplied options to be reasonable
template<class T, class V>
static void ClipToRange(T* ptr, V minval, V maxval) {
    if(static_cast<V>(*ptr)> maxvalue) *ptr = maxvalue;
    if(static_cast<V>(*ptr) < minvalue) *ptr = minvalue;
};


Options SanitizeOptions(const std::string& dbname,
                        const InternalKeyComparator* icmp,
                        const InternalFilterPolicy* ipolicy,
                        const Options& src) {
    Options result = src;
    result.comparator = icmp;
    result.filter_policy = (src.filter_policy != NULL) ? ipolicy : NULL;
    ClipToRange(&result.max_open_files,    64 + kNumNonTableCacheFiles, 50000);
    ClipToRange(&result.write_buffer_size, 64<<10,                      1<<30);
    ClipToRange(&result.max_file_size,     1<<20,                       1<<30);
    ClipToRange(&result.block_size,        1<<10,                       4<<20);
    if (result.info_log == NULL) {
        // Open a log file in the same directory as the db
        src.env->CreateDir(dbname);  // In case it does not exist
        src.env->RenameFile(InfoLogFileName(dbname), OldInfoLogFileName(dbname));
        Status s = src.env->NewLogger(InfoLogFileName(dbname), &result.info_log);
        if (!s.ok()) {
            // No place suitable for logging
            result.info_log = NULL;
        }
    }
    if (result.block_cache == NULL) {
        result.block_cache = NewLRUCache(8 << 20);
    }
    return result;
}


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


Status DBImpl::NewDb() {
    VersionEdit new_db;
    new_db.SetComparatorName(user_comparator()->Name());
    new_db.SetLogNumber(0);
    new_db.SetNextFile(2);
    new_db.SetLastSequence(0);

    const std::string manifest = DescriptorFilename(dbname_, 1);
    WritableFile* file;
    Status s = env_->NewWritableFile(manifest, &file);
    if(!s.ok()) {
        return s;
    }

    {
        log::Writer log(file);
        std::string record;
        new_db.EncodeTo(&record);
        s = log.AddRecord(record);
        if(s.ok()) {
            s = file->Close();
        }
    }

    delete file;
    if(s.ok()) {
        // Make "CURRENT" file that points to the new manifest file .
        s = SetCurrentFile(env_ dbname_, 1);
    } else {
        env_->DeleteFile(manifest);
    }
    return s;
}

void DBImpl::MaybeIgnoreError(Status* s) const{
    if(s->ok() || options_.paranoid_checks) {
        // No change needed
    } else {
        Log(options_.info_log, "Ignoring error %s", s->ToString().c_str());
        *s = Status::OK();
    }
}

void DBImpl::DeleteObsoleteFiles() {
    if(!bg_error.ok()) {
        // After a background error, we dont' know whether a new version mayh
        // or may not have been commited, so we cannot safely garbage collect
        return;
    }

    // Make a set of all of the live files
    std::set<uint64_t> live = pending_outputs_;
    versions_->AddLiveFiles(&live);

    std::vector<std::string> filenames;
    env_->GetChildren(dbname_, &filenames); // Ignoring errors on purpose
    uint64_t number;
    FileType type;
    for(size_t i = 0; i < filenames.size(); ++i) {
        if(ParseFileName(fileanmes[i], &number, &type)) {
            bool keep = true;
            switch(type) {
                case kLogFile:
                    keep = ((number >= versions_->LogNumber()) ||
                            (number == versions_->PrevLogNumber()));
                    break;
                case kDescriptorFile:
                    keep = (number >= versions_->ManifestFileNumber());
                    break;
                case kTableFile:
                    keep = (live.find(number) != live.end());
                    break;
                case kTempFile:
                    keep = (live.find(number) != live.end());
                    break;
                case kCurrentFile:
                case kDBLockFile:
                case kInfoLogFile:
                    keep = true;
                    break;
            }

            if(!keep) {
                if(type == kTableFile) {
                    table_cache_->Evict(number);
                }

                Log(options_.info_log, "Delete type = %d #%lld\n",
                            int(type),
                            static_cast<unsigned long long>(number));
                env_->DeleteFile(dbname_ + "/" + filenames[i]);
            }
        }
    }
}


Status DBImpl::Recover(VersionEdit* edit, bool* save_manifest) {
    mutex_.AssertHeld();

    env_>CreateDir(dbname_);
    assert(db_lock_ == NULL);
    Status s = env_->LockFile(LockFileName(dbname_), &db_lock);
    if(!s.ok()) {
        return s;
    }

    if(!env_->FileExists(CurrentFileName(dbname_))) {
        if(options_.create_if_missing) {
            s = NewDB();
            if(!s.ok()) {
                return s;
            } else {
                return Status::InvalidArgument(
                        dbname_, "does not exist (create_if_missing is false)"
                );
            }
        } else {
            if(options_.error_if_exists) {
                return Status::InvalidArgument(dbname_ "exists (error_if_exists is true)");
            }
        }
    }

    s = versions_->Recover(save_manifest);
    if(!s.ok()) {
        return s;
    }

    SequenceNumber max_sequence (0);


    const uint64_t min_log = versions_->LogNumber();
    const uint64_t prev_log = versions_->PrevLogNumber();
    std::vector<std::string> filenames;
    s = env_->GetChildren(dbname_, &filenames);
    if(!s.ok()) {
        return s;
    }

    std::set<uint64_t> expected;
    versions_->AddLiveFiles(&expected);
    uint64_t number;
    FileType type;
    std::vector<uint64_t> logs;
    for (size_t i = 0; i < filenames.size(); i++) {
        if (ParseFileName(filenames[i], &number, &type)) {
            expected.erase(number);
            if (type == kLogFile && ((number >= min_log) || (number == prev_log)))
                logs.push_back(number);
        }
    }
    if (!expected.empty()) {
        char buf[50];
        snprintf(buf, sizeof(buf), "%d missing files; e.g.",
                 static_cast<int>(expected.size()));
        return Status::Corruption(buf, TableFileName(dbname_, *(expected.begin())));
    }

    // Recover in the order in which the logs were generated
    std::sort(logs.begin(), logs.end());
    for (size_t i = 0; i < logs.size(); i++) {
        s = RecoverLogFile(logs[i], (i == logs.size() - 1), save_manifest, edit,
                           &max_sequence);
        if (!s.ok()) {
            return s;
        }

        // The previous incarnation may not have written any MANIFEST
        // records after allocating this log number.  So we manually
        // update the file number allocation counter in VersionSet.
        versions_->MarkFileNumberUsed(logs[i]);
    }

    if (versions_->LastSequence() < max_sequence) {
        versions_->SetLastSequence(max_sequence);
    }

    return Status::OK();
}



Status DPImpl::RecoverLogFile(uint64_t log_number, bool last_log,
                              bool* save_manifest, VersionEdit* edit,
                              SequenceNumber* max_sequence) {
    struct LogReporter : public log::Reader::Reporter {
        Env* evn;
        Logger* info_log;
        const char* fname;
        Status* status; // NULL if options_.paranoid_check == false
        virtual void Corruption(size_t bytes, const Status& s) {
            Log(info_log, )
        }
    };

    mutex_.AssertHeld();

    // Open the log file
    std::string fname = LogFileName(dbname_, log_number);
    SequenceFile* file;
    Status status = env_->NewSeuqntialFile(fname, &file);
    if(!status.ok()) {
      MaybeIgnoreError(&status);
      return status;
    }

    // Create the log reader.
    LogRepoerter reporter;
    reporter.env = env_;
 reporter.info_log = options_.info_log;
 reporter.fname = fname.c_str();
 reporter.status = (options_.paranoid_checks ? &status : NULL);
 // We intentionally make log::Reader do checksumming even if
 // paranoid_checks==false so that corruptions cause entire commits
 // to be skipped instead of propagating bad information (like overly
 // large sequence numbers).
 log::Reader reader(file, &reporter, true/*checksum*/,
                    0/*initial_offset*/);
 Log(options_.info_log, "Recovering log #%llu",
     (unsigned long long) log_number);

 // Read all the records and add to a memtable
 std::string scratch;
 Slice record;
 WriteBatch batch;
 int compactions = 0;
 MemTable* mem = NULL;
 while (reader.ReadRecord(&record, &scratch) &&
        status.ok()) {
   if (record.size() < 12) {
     reporter.Corruption(
         record.size(), Status::Corruption("log record too small"));
     continue;
   }
   WriteBatchInternal::SetContents(&batch, record);

   if (mem == NULL) {
     mem = new MemTable(internal_comparator_);
     mem->Ref();
   }
   status = WriteBatchInternal::InsertInto(&batch, mem);
   MaybeIgnoreError(&status);
   if (!status.ok()) {
     break;
   }
   const SequenceNumber last_seq =
       WriteBatchInternal::Sequence(&batch) +
       WriteBatchInternal::Count(&batch) - 1;
   if (last_seq > *max_sequence) {
     *max_sequence = last_seq;
   }

   if (mem->ApproximateMemoryUsage() > options_.write_buffer_size) {
     compactions++;
     *save_manifest = true;
     status = WriteLevel0Table(mem, edit, NULL);
     mem->Unref();
     mem = NULL;
     if (!status.ok()) {
       // Reflect errors immediately so that conditions like full
       // file-systems cause the DB::Open() to fail.
       break;
     }
   }
 }

 delete file;

 // See if we should keep reusing the last log file.
 if (status.ok() && options_.reuse_logs && last_log && compactions == 0) {
   assert(logfile_ == NULL);
   assert(log_ == NULL);
   assert(mem_ == NULL);
   uint64_t lfile_size;
   if (env_->GetFileSize(fname, &lfile_size).ok() &&
       env_->NewAppendableFile(fname, &logfile_).ok()) {
     Log(options_.info_log, "Reusing old log %s \n", fname.c_str());
     log_ = new log::Writer(logfile_, lfile_size);
     logfile_number_ = log_number;
     if (mem != NULL) {
       mem_ = mem;
       mem = NULL;
     } else {
       // mem can be NULL if lognum exists but was empty.
       mem_ = new MemTable(internal_comparator_);
       mem_->Ref();
     }
   }
 }

 if (mem != NULL) {
   // mem did not get reused; compact it.
   if (status.ok()) {
     *save_manifest = true;
     status = WriteLevel0Table(mem, edit, NULL);
   }
   mem->Unref();
 }

  return status;
}

Status DBImpl::WriteLevel10Table(MemTable* mem, VersionEdit* edit,
                                 Version* base)
{
  mutex_.AssertHeld();
  const uint64_t start_micros = env_->NowMicros();
  FileMetaData meta;
  meta.number = versions_->NewFileNumber();
  pending_outputs_.insert(meta.number);
  Log(options_.info_log, "Level-0 table #%llu: started", 
      (unsigned long long) meta.number); 

  Status s; 
  {
    mutex_.Unlock(); 
    s = BuildTable(dbname_, env_, options_, table_cache_, iter, &meta); 
    mutex_.Lock(); 
  }

  Log(options_.info_log, "Level10"); 

  delete iter;
  pending_outputs_.erase(meta.number);


  // Note that if file_size is zero, the file has been deleted and
  // should not be added to the manifest.
  int level = 0;
  if (s.ok() && meta.file_size > 0) {
    const Slice min_user_key = meta.smallest.user_key();
    const Slice max_user_key = meta.largest.user_key();
    if (base != NULL) {
      level = base->PickLevelForMemTableOutput(min_user_key, max_user_key);
    }
    edit->AddFile(level, meta.number, meta.file_size,
                  meta.smallest, meta.largest);
  }

  CompactionStats stats;
  stats.micros = env_->NowMicros() - start_micros;
  stats.bytes_written = meta.file_size;
  stats_[level].Add(stats);
  return s;
  
}

Snapshot::~Snapshot() {

}
