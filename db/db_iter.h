#ifndef _LEVELDB_ITER_H_
#define _LEVELDB_ITER_H_

#include <stdint.h>
#include "leveldb/db.h"
#include "db/dbformat.h"

namespace leveldb {

class DBImpl;

/**
 * Retuns a new iterator that converts internal keys (yelded by
 * "internal_iter") that were live at the specified "sequence"
 * number into appropriate uses keys
 */

extern Iterator* NewDBIterator(
  DBImpl* db,
  const Comparator* user_key_comparator,
  Iterator* internal_iter,
  SequenceNumber sequence,
  uint32_t seed);

} // namespace leveldb 

#endif // _LEVELDB_ITER_H_
