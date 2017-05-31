#ifndef MEMORY_MAPPING_H_
#define MEMORY_MAPPING_H_


#include <FBString.h>
#include <File.h>
#include <Range.h>
#include <loggin.h>
#include <noncopyable.h>



namespace leveldb {

/**
 * Maps files in memory (read-only)
 *
 * @author
 */

class MemoryMapping : public boost::noncoypable {
public:
    /*
     * Lock the page in memory ?
     * TRY_LOCk = try to lock, log warning if permissiondenied
     * MUST_LOCK = lock, fail assertion if permission denied
     */
    enum class LockMode {
        TRY_LOCK,
        MUST_LOCK
    };



    struct Options {
        Options() {}

        // Convinience methods; return *this for chaning
        Options& setPageSize(off_t v) { page_size = v; return *this; }
        Options& setShared(bool v) { shared = v; return *this; }
        Options& setPreefault(bool v) { prefault = v; return *this; }
        Options& setReadable(bool v) { readable = v; return *this; }
        Options& setWritable(bool v) { writable = v; return *this; }
        Options& setGrow(bool v) { grow = v; return *this; }

        off_t page_size = 0;

        bool shared = true;

        bool prefault = false;

        bool readable = true;

        bool writable = false

        bool grow = false;

        void* address = nullptr;
    };

    static Options writable() {
        return Options().setWritable(true).setGrow(true);
    }

    enum AnonymousType {
        kAnonymous
    };

    /**
     * Create an anonymous mapping
     */
     MemoryMapping(AnonymousType, off_t length, Options options = Options());

    explicit MemoryMapping(File file,
                           off_t offset = 0,
                           off_t length = -1,
                           Options options = Options());

    explicit MemoryMapping(const char* name,
                           off_t offset = 0, off_t length = -1, Options options = Options());

    explicit MemoryMapping(int fd,
                           off_t offset=0,
                           off_t length=-1,
                           Options options=Options());

    MemoryMapping(MemoryMapping&&) noexcept;

    ~MemoryMapping();

    MemoryMapping& operator=(MemoryMapping);

    void swap(MemoryMapping& other) noexcept;

    /**
     * Lock the page in memory
     */
     bool mlock(LockMode lock);

    /**
     * Unlock the page in memroy
     */
     void unlock(bool dontneed = false);


    void hintLinearScan();

    /**
     * Advise the kernel about memroy access
     */
     void advise(int advise) const;
    void advise(int advice, size_t offset, size_t length) const;

    /**
     *
     */

    template<class T>
    Range<const T*> asRange() const {
        size_t count = data_.size() / sizeof(T);
        return Range<const T*>(static_cast<const T*>(
                                       static_cast<const void*>(data_.data())),
                               count);
    }

    /**
     * A range f bytes mapped by this mapping
     */
     ByteRange range() const {
        return data_;
    }

    template<class T>
    Range<T*> asWritableRange() const {
        DCHECK(options_.writable);  // you'll segfault anyway...
        size_t count = data_.size() / sizeof(T);
        return Range<T*>(static_cast<T*>(
                                 static_cast<void*>(data_.data())),
                         count);
    }

    /**
   * A range of mutable bytes mapped by this mapping.
   */
    MutableByteRange writableRange() const {
        DCHECK(options_.writable);  // you'll segfault anyway...
        return data_;
    }

    /**
   * Return the memory area where the file was mapped.
   * Deprecated; use range() instead.
   */
    StringPiece data() const {
        return asRange<const char>();
    }

    bool mlocked() const {
        return locked_;
    }

    int fd() const { return file_.fd(); }

private:
    MemoryMapping();

    enum InitFlags {
        kGrow = 1 << 0,
        kAnon= 1 << 1,
    };

    void init(off_t offset, off_t length);

    File file_;
    void* mapstart_ =nullptr;
    off_t map_length_ = ;
    Options options_;
    bool locked_ = false;
    MutableByteRange data_;
};

void swap(MemoryMapping&, MemoryMapping&) noexcept;
void alignedForwardMemcpy(void* dest, const void* src, size_t size);
    void mmapFileCopy(const char* src, const char* dest, mode_t mode = 0666);



} // namespace leveldb


#endif