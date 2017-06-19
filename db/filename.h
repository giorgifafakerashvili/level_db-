#ifndef _LEVELDB_FILENAME_H_
#define _LEVELDB_FILENAME_H_

#include <stdint.h>
#include <string>
#include "leveldb/slice.h"
#include "leveldb/status.h"
#include "port/port.h"

namespace leveldb {

class Env;

enum FileType {
  kLogFile,
  kDBLockFile,
  kTableFile,
  kDescriptorFile,
  kCurrentFile,
  kTempFile,
  kInfoLogFile // Either the current one, or an old one
};

/**
 * Return the name of the log file with the specified number
 * in the db named by "dbname". The result will be prefixed with
 * "dbname"
 */
extern std::string LogFileName(const std::string& dbname, uint64_t number);

/**
 * Return the name of the sstable with the specified number
 * in the db named by "dbname". The result will be prefixed with
 * "dbname"
 */
extern std::string TableFileName(const std::string& dbname, uint64_t number);

extern std::string SSTTableFileName(const std::string& dbname, uint64_t number);

extern std::string DescriptorFileName(const std::string& dbname, uint64_t number);


/**
 * Returns the name of the current file. This file contains the name
 * of the current manifest file. The result will be prefixed with
 * "dbname"
 */
extern std::string CurrentFileName(const std::string& dbname);

// Return the name of the lock file for the db named by
// "dbname".  The result will be prefixed with "dbname".
extern std::string LockFileName(const std::string& dbname);

extern std::string TempFileName(const std::string& dbname, uint64_t number);

extern std::string InfoLogFileName(const std::string& dbname);

extern std::string OldInfoLogFileName(const std::string& dbname);

extern bool ParseFileName(const std::string& filename,
                          uint64_t* number,
                          FileType* type);


extern Status SetCurrentFile(Env* env, const std::string& dbname,
                             uint64_t descriptor_number);

} // namespace leveldb

#endif // _LEVELDB_FILENAME_
