#ifndef STORAGE_LEVELDB_INCLUDE_ITERATOR_H_
#define STORAGE_LEVELDB_INCLUDE_ITERATOR_H_

namespace leveldb
{

class Iterator {
public:
    Iterator() {}
    virtual ~Iterator() {}

    // An iterator is either positioned at a key/value pair, or
    //  not valid. This method returns true iff the iterator is valid
    virtual bool Valid() const = 0;


    //Position at the first key in the source. The iteraotr is Vliad()
    // after this call iff the source is not empty
    virtual void SeekToFirst() = 0;

    virtual void SeekToLast() = 0;

    virtual void Seek(const Slice& target) = 0;

    virtual void Next() = 0;

    virtual void Prev() = 0;

    virtual Slice key() const = 0;

    virtual Slice value() const = 0;

    virtual Status status() const = 0;

    typedef void (*CleanupFunction)(void* arg1, void* arg2);

    void RegisterCleanup(CleanupFunction function, void* arg1, void* arg2);

private:
    struct Cleanup {
        CleanupFunction function;
        void* arg1;
        void* arg2;
        CleanupFunction next_;
    };

    Cleanup cleanup_;

    Iterator(const Iterator&);
    void operator=(const Iterator&);

};

extern Iterator* NewEmptyIterator();

extern Iterator* NewErrorIterator(const Status& status);


}





#endif