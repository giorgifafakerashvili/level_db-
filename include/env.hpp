#ifndef STORAGE_LEVELDB_INCLUDE_ENV_H_
#define STORAGE_LEVELDB_INCLUDE_ENV_H_

#include <string>
#include <vector>
#include <stdarg.h>
#include <stdint.h>
#include "status.hpp"

namespace leveldb {

class FileLock;
class Logger;
class RandomAccessFile;
class SequentialFile;
class Slice;
class WritableFile;


class Env {
public:
    Env() {}
    virtual ~Env();

    static Env* Default();

    virtual Status NewSequentialFile(const std::strig& fname,
                                     SequentialFile** result) = 0;

    virtual Status NewRandomAccessFile(const std::strig& fname,
                                       RandomAccessFile** result) = 0;

    virtual Status NewWritableFile(const std::string& fname,
                                    WritableFile** result) = 0;


    virtual Status NewAppendableFile(const std::string& fname,
                                     WritableFile** result);

    virtual bool FileExists(const std::strig& fname) = 0;

    virtual Status GetChildren(const std::string& dir,
                               std::vector<std::string>* result) = 0;

    virtual Status DeleteFile(const std::string& fname) = 0;

    virtual Status CreateDir(const std::string& dirname) = 0;

    // Delete the specified directory.
    virtual Status DeleteDir(const std::string& dirname) = 0;

    // Store the size of fname in *file_size.
    virtual Status GetFileSize(const std::string& fname, uint64_t* file_size) = 0;

    // Rename file src to target.
    virtual Status RenameFile(const std::string& src,
                              const std::string& target) = 0;

    virtual Status LockFile(const std::string& fname, FileLock** lock) = 0;

    // Release the lock acquired by a previous successful call to LockFile.
    // REQUIRES: lock was returned by a successful LockFile() call
    // REQUIRES: lock has not already been unlocked.
    virtual Status UnlockFile(FileLock* lock) = 0;

    // Arrange to run (*function)(arg) once in a background thread.
    //
    // function may run ina n unspecified thread. Multiple functions
    // added to the same Env may run concurerenty in different thrads
    // I.e., the caller may not assume that background work items are
    // serialized
    virtual void Schedule(
            void (*function)(void* arg),
            void* arg)  = 0;

    // Start a new thread invoking "function(arg)" within the new thread
    // when "funtion(arg)" returns the thread will be destroyed
    virtual void StartThread(void (*function)(void* arg), void* arg) = 0;


    virtual Status GetTestDirectory(std::string* path) = 0;

    virtual Status NewLogger(const std::string& fname, Logger** result) = 0;

    virtual uint64_t NewMicros() = 0;

    virtual void SleepForMicroseconds(int micros) = 0;

private:
    // No copying allowed
    Env(const Env&);
    void operator=(const Env&);

};


// A file abstraction for reading sequentilly through a file
class SequentialFile {
public:
    SequentialFile() {}
    virtual ~SequentialFile();

    virtual Status Read(std::size_t n, Slice* result, char* scratch) = 0;

    virtual Status Skip(uin64_t n) = 0;

private:
    // No copying allowed
    SequentialFile(const SequentialFile&);
    void operator=(const SequentialFile&);

};

// A file abstraction for randomly reading the content of a file
class RandomAccessFile {
public:
    RandomAccessFile() {}
    virtual ~RandomAccessFile();

    virtual Status Read(uint64_t offset, size_t n, Slice* result,
                        char* scratch) const = 0;

private:
    RandomAccessFile(const RandomAccessFile&);
    void operator=(const RandomAccessFile&);
};



}

#endif