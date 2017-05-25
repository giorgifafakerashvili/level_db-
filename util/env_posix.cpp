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






} // Namespace
} // Namespace Leveldb