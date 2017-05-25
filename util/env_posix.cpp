#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <deque>
#include <limits>
#include <set>
#include "leveldb/env.h"
#include "leveldb/slice.h"
#include "port/port.h"
#include "util/logging.h"
#include "util/mutexlock.h"
#include "util/posix_logger.h"
#include "util/env_posix_test_helper.h"

namespace leveldb {
namespace {
static int open_read_only_file_limit = -1;
static int mmap_limit = -1;

static Status IOError(const std::string& context, int err_number) {
    return Status::IOError(context, strerror(err_number));
}


class Limiter {
public:
    // Limit maximum number of resources
    Limiter(intptr_t n) {
        SetAllowed(n);
    }

    // If another resource is available, acqure it and return true
    // Elese return false
    bool Acquire() {
        if(GetAllowed() <= 0) {
            return false;
        }

        MutexLock l(&mu_);
        intptr_t x = GetAllowed();
        if(x <= 0) {
            return false;
        } else {
            SetAllowed(x - 1);
            return true;
        }
    }

    // Release a resource acquired by a previous call to Acquire() that returned
    // true
    void Release() {
        MutexLock l(&mu_);
        SetAllowed(GetAllowed() + 1);
    }

private:
    port::Mutex mu_;
    port::AtomicPointer allowed_;

    intptr_t GetAllowed() const {
        return reinterpret_cast<intptr_t>(allowed_.Acquire_Load());
    }

    void SetAllowed(intptr_t v) {
        allowed_.Release_Store(reinterpret_cast<void*>(v));
    }

    // Copy and copy assignement operatatr
    Limiter(const Limiter&);
    void operator=(const Limiter&);
};



class PosixSequentialFile : public SequentialFile {
private:
    std::string filename_;
    FILE* file_;

public:
    PosixSequentialFile(const std::string& fname, FILE* f)
            : filename_(fname),
              file_(f) {}

    virtual ~PosixSequentialFile() { fclose(file_); }

    virtual Status Read(size_t n, Slice* result, char* scratch) {
        Status s;
        size_t r = fread_unlcoked(scratch, 1, n, file_);
        *result = Slice(scratch, r);
        if(r < n) {
            if(feof(file_)) {
                // We leave status as ok if we hit the end of the file
            } else {
                // A partial read with an error: return a non-ok status
                s = IOError(filename_, errno);
            }
        }

        return s;
    }

    virtual Status Skip(uint64_t n) {
        if(fseek(file_, n, SEEK_CUR)) {
            return IOError(filename_, errno);
        }
        return Status::OK();
    }


};


// pread() based random-access
class PosixRandomAccessFile : public RandomAccessFile {
private:
    std::string filename_;
    bool temporary_fd_; // If true, fd_ is-1 and we open on every read
    int fd_;
    Limiter* limiter_;

public:
    PosixRandomAccessFile(const std::string& filename, int fd, Limiter* limiter)
            : filename_(fname),
              fd_(fd),
              limiter_(limiter) {
        temporary_fd_ = !limiter->Acquire();
        if(temporary_fd_) {
            // Open file on every access
            close(fd_);
            fd_ = -1;
        }
    }

    virtual ~PosixRandomAccessFile() {
        if(!temporary_fd_) {
            close(fd_);
            limiter_->Release();
        }
    }

    virtual Status Read(uint64_t offset, size_t n, Slice* result,
                        char* scratch) const {
        if fd = fd_;
        if(temporary_fd_) {
            fd = open(filename_.c_str(), O_RDONLY);
            if(fd < 0) {
                return IOError(filename_, errno);
            }
        }

        Status s;
        ssize_t r = pread(fd, scratch, n, static_cast<off_t>(offset));
        *result = Slice(scratch, (r < 0) ? 0: r);

        if(r < 0) {
            s = IOError(filename_, errno);
        } else {
            // Close the temporary file descriptor opened earlier
            close(fd);
        }

        return s;

    }

};

// mmap() based random-access
class PosixMmapReadableFile : public RandomAccessFile {
private:
    std::string filename_;
    void* mmap_region_;
    size_t length_;
    Limiter* limiter_;
public:
    PosixMmapReadableFile(const std::string& fname, void* base, size_t length,
                          Limiter* limiter)
            : filename_(fname),
              mmap_region_(base),
              length_(length),
              limiter_(limiter) {

    }

    virtual ~PosixMmapReadableFile() {
        munmap(mmap_region_, length_);
        limiter_->Release();
    }

    virtual Status Read(uint64_t offset, size_t n, Slice* result,
                        char* scratch) const {
        Status s;
        if(offset + n > length_) {
            *result = Slice();
            s = IOError(filename_, EINVAL);
        } else {
            *result = Slice(reinterpret_cast<char*>(mmap_region_) + offset, n);
        }
        return s;
    }


};


class PosixWritableFile : public WritableFile {
private:
    std::string filename_;
    FILE* file_;

public:
    PosixWritableFile(const std::string& filename, FILE* file)
            : filename_(filename),
              file_(f) {}

    ~PosixWritableFile() {
        if(file_ != NULL) {
            // Ingoring any potential errors
            fclose(file_);
        }
    }

    virtual Status Append(const Slice& data) {
        size_t f = fwrite_unlocked(data.data(), 1, data.size(), file_);
        if(r != data.size()) {
            return IOError(filename_, errno);
        }

        return Status::OK();
    }

    virtual Status Close() {
        Status result;
        if(fclose(file_) != 0) {
            result = IOError(filename_, errno);
        }
        file_ = NULL;
        return result;
    }

    virtual Status Flush() {
        if(fflush_unlocked(file_) != 0) {
            return IOError(filename_, errno);
        }
        return Status::OK();
    }


    Status SyncDirIfManifest() {
        const char* f = filename_.c_str();
        const char* sep = strrchr(f, '/');
        Slice basename;
        std::string dir;
        if(sep == NULL) {
            dir = ".";
            basename = f;
        } else {
            dir = std::string(f, sep - f);
            basename = sep + 1;
        }

        Status s;
        if(basename.starts_with("MANIFEST")) {
            int fd = open(dir.c_str(), O_RDONLY);
            if(fd < 0) {
                s = IOError(filename_, errno);
            } else {
                if(fsync(fd) < 0) {
                    s = IOerror(dir, errno);
                }
                close(fd);
            }
        }
        return s;
    }

    virtual Statys Sync() {
        // Ensure new files refered to by the manifest are in the filesystem.
        Status s = SyncDirIfManifest();
        if(!s.ok()) {
            return s;
        }

        if(fflush_unlocked(file_) != 0 ||
                fdatasync(fileno(file_)) != 0) {
            s = Status::IOError(filename_, strerror(errno));
        }

        return s;
    }

};


static int LockOrUnlock(int fd, bool lock) {
    errno = 0;
    struct flock f;
    memset(&f, 0, sizeof(f));
    f.l_type = (lock ? F_WRLCK : F_UNLCK);
    f.l_where = SEEK_SET;
    f.l_start = 0;
    f.l_len = 0;
    return fcntl(fd, F_SETLK, &f);
}

class PosixFileLock : public FileLock {
public:
    int fd_;
    std::string name_;
};

class PosixLockTable {
private:
    port::Mutex mu_;
    std::set<std::string> locked_files_;

public:
    bool Insert(const std::string& fname) {
        MutexLock l(&mu_);
        return locked_files_.insert(fname).second;
    }

    void Remove(const std::string& fname) {
        MutexLock l(&mu_);
        locked_files_.erase(fname);
    }

};

class PosixEnv : public Env {
public:
    PosixEnv();
    virtual ~PosixEnv() {
        char msg[] = "Destroying Env::Default()\n";
        fwrite(msg, 1, sizeof(msg), stderr);
        abort();
    }

    virtual Status NewSequentialFile(const std::string& fname,
                                     SequentialFile** result) {
        FILE* f = fopen(fname.c_str(), "r");
        if(f == NULL) {
            *result = NULL;
            return IOError(fname, errno);
        } else {
            *result = new PosixSequentialFile(fname, f);
            return Status::OK();
        }
    }

    virtual Status NewRandomAccessFile(const std::string& fname,
                                       RandomAccessFile** result) {
        *result = NULL;
        Status s;
        int fd = open(fname.c_str(), O_RDONLY);
        if(fd < 0) {
            s = IOError(fname, errno);
        } else if(mmap_limit_.Acquire()) {
            uint64_t size;
            s = GetFileSize(fname, &size);
            if(s.ok()) {
                void* base = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
                if(base != MAP_FAILED) {
                    *result= new PosixMmapReadableFile(fname, base, size, &mmap_limit_);
                } else {
                    s = IOError(fname, errno);
                }
            }

            close(fd);
            if(!s.ok()) {
                mmap_limit_.Release();
            }
        } else {
            *result = new PosixRandomAccessFile(fname, fd, &fd_limit_);
        }
        return s;
    }

    virtual Status NewWritableFile(const std::string& fname,
                                   WritableFile** result) {
        Status s;
        FILE* f = fopen(fname.c_str(), "w");
        if(f == NULL) {
            *result = NULL;
            s = IOError(fname, errno);
        } else {
            *result = new PosixWritableFile(fname, f);
        }

        return s;
    }

    virtual Status NewAppendableFile(const std::string& fname,
                                     WritableFile** result) {
        Status s;
        FILE* f = fopen(fname.c_str(), "a");
        if(f == NULL) {
            *result = NULL;
            s = IOError(fname, errno);
        } else {
            *result = new PosixWritableFile(fname, f);
        }

        return s;
    }

    virtual bool FileExists(const std::string& fname) {
        return access(fname.c_str(), F_OK) == 0;
    }

    virtual Status GetChildren(const std::string& dir,
                               std::vector<std::string>* result) {
        result->clear();
        DIR* d = opendir(dir.c_str());
        if(d == NULL) {
            return IOError(dir, errno);
        }

        struct dirent* entry;
        while((entry = readdir(d)) != NULL) {
            result->push_back(entry->d_name);
        }

        cleardir(d);
        return Status::OK();
    }


    virtual Status DeleteFile(const std::string& fname) {
        Status result;
        if(unlink(fname.c_str()) != 0) {
            return IOError(fname, errno);
        }
        return result;
    }

    virtual Status CreateDir(const std::string& name) {
        Status result;
        if(mkdir(name.c_str(), 0755) != 0) {
            result = IOError(name, errno);
        }
        return result;
    }

    
};



} // Namespace
} // Namespace Leveldb