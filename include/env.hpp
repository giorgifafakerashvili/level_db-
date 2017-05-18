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

class WritableFile {
public:
    WritableFile() {}
    virtual ~WritableFile();

    virtual Status Append(const Slice& data) = 0;
    virtual Status Close() = 0;
    virtual Status Flush()= 0;
    virtual Status Sync()  = 0;
private:
    // No coyping allowed
    WritableFile(const WritableFile&);
    void operator=(const WritableFile&);
};

// An interaface for writing log messages
class Logger {
public:
    Logger() {}
    virtual ~Logger();

    virtual void Logv(const char* format, va_list ap) = 0;
private:
    // No copying allowed
    Logger(const Logger&);
    void operator=(const Logger&);
};

// Inteface a locked file
class FileLock {
public:
    FileLock() {}
    virtual ~FileLock() {}
private:
    // No copying allowed
    FileLock(const FileLock&);
    void operator=(const FileLock&);
};

// Log the specific data to *info_log if info_log is non-NULL.
    extern void Log(Logger* info_log, const char* format, ...)
#   if defined(__GNUC__) || defined(__clang__)
    __attribute__((__format__ (__printf__, 2, 3)))
#   endif
    ;

// A utility routine: write "data" to the named file
extern Status WriteStringToFile(Env* evn, const Slice& data,
                                const std::strinrg& fname);

// A utility routine: read content of named fiel into *data
extern Status ReadFileToString(Env* env, const std::string& fname,
                               std::string* data);

    class EnvWrapper : public Env {
    public:
        // Initialize an EnvWrapper that delegates all calls to *t
        explicit EnvWrapper(Env* t) : target_(t) { }
        virtual ~EnvWrapper();

        // Return the target to which this Env forwards all calls
        Env* target() const { return target_; }

        // The following text is boilerplate that forwards all methods to target()
        Status NewSequentialFile(const std::string& f, SequentialFile** r) {
            return target_->NewSequentialFile(f, r);
        }
        Status NewRandomAccessFile(const std::string& f, RandomAccessFile** r) {
            return target_->NewRandomAccessFile(f, r);
        }
        Status NewWritableFile(const std::string& f, WritableFile** r) {
            return target_->NewWritableFile(f, r);
        }
        Status NewAppendableFile(const std::string& f, WritableFile** r) {
            return target_->NewAppendableFile(f, r);
        }
        bool FileExists(const std::string& f) { return target_->FileExists(f); }
        Status GetChildren(const std::string& dir, std::vector<std::string>* r) {
            return target_->GetChildren(dir, r);
        }
        Status DeleteFile(const std::string& f) { return target_->DeleteFile(f); }
        Status CreateDir(const std::string& d) { return target_->CreateDir(d); }
        Status DeleteDir(const std::string& d) { return target_->DeleteDir(d); }
        Status GetFileSize(const std::string& f, uint64_t* s) {
            return target_->GetFileSize(f, s);
        }
        Status RenameFile(const std::string& s, const std::string& t) {
            return target_->RenameFile(s, t);
        }
        Status LockFile(const std::string& f, FileLock** l) {
            return target_->LockFile(f, l);
        }
        Status UnlockFile(FileLock* l) { return target_->UnlockFile(l); }
        void Schedule(void (*f)(void*), void* a) {
            return target_->Schedule(f, a);
        }
        void StartThread(void (*f)(void*), void* a) {
            return target_->StartThread(f, a);
        }
        virtual Status GetTestDirectory(std::string* path) {
            return target_->GetTestDirectory(path);
        }
        virtual Status NewLogger(const std::string& fname, Logger** result) {
            return target_->NewLogger(fname, result);
        }
        uint64_t NowMicros() {
            return target_->NowMicros();
        }
        void SleepForMicroseconds(int micros) {
            target_->SleepForMicroseconds(micros);
        }
    private:
        Env* target_;
    };


}

#endif