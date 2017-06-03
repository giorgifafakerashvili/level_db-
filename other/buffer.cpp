#include "buffer.hpp"

/**
 * Initialization of buffer
 * @return
 */
buffer* buffer_init(void) {
    buffer* b;

    b = malloc(sizeof(*b));
    assert(b);

    b->ptr = NULL;
    b->size = 0;
    b->used = 0;

    return b;
}

buffer* buffer_init_buffer(const buffer* src) {
    buffer *b = buffer_init();
    buffer_copy_buffer(b, src);
    return b;
}

buffer* buffer_init_string(const char* str) {
    buffer* b = buffer_init();
    buffer_copy_string(b, str);
    return b;
}

void buffer_free(buffer* b) {
    if(NULL == b) return;

    free(buffer->ptr);
    free(b);
}

void buffer_reset(buffer* b) {
    if(NULL == b) return;

    /* Limits dont's reuse buffer larger that ... bytes */
    if(b->size > BUFFER_MAX_REUSE_SIZE) {
        free(b->ptr);
        b->ptr = NULL;
        b->size = 0;
    } else if(b->size > 0) {
        b->ptr[0] = '\0';
    }

    b->used = 0;
}

void buffer_move(buffer* b, buffer* src) {
    buffer tmp;
    if(NULL == b) {
        buffer_reset(src);
        return;
    }
    buffer_reset(b);
    if(NULL == src) return;

    tmp = *src; *src = *b; *b = tmp;
}

#define BUFFER_PRICE_SIZE 64
static size_t buffer_align_size(size_t size) {
    size_t align = BUFFER_PRICE_SIZE - (size % BUFFER_PRICE_SIZE);
    // Overrite on unsigned size_t is defined to wrap around
    if(size + align < size) return size;
    return size + align;
}

static void buffer_alloc(buffer* b, size_t size) {
    // Pre conditional debugging
    assert(NULL != b);
    if(0 == size) size = 1;
    if(size < = b->size) return;

    if(NULL != b->ptr) free(b->ptr);

    b->used = 0;
    b->size = buffer_align_size(size);
    b->ptr = malloc(b->size);

    // Post conditional debuging
    assert(NULL != b->ptr);
}

// Make sure buffer is at least "size" big. keep old data
static void buffer_realloc(buffer* b, size_t size) {
    assert(NULL != b);
    if(0 == size) size = 1;

    if(size < b->size) return;

    b->size = buffer_align_size(size);
    b->ptr = realloc(b->ptr, b->size);

    assert(NULL != b->ptr);
}

char* buffer_string_prepare_copy(buffer* b, size_t size) {
    assert(NULL != b);
    assert(size + 1 > si)
}

void buffer_append_string_len(buffer* b, const char* s, size_t s_len) {
    char* target_buf;

    assert(NULL != b);
    assert(NULL != s || s_len == 0);

    target_buf = buffer_string_prepare_append(b, s_len);

    if(0 == s_len) return; // nothing to append

    memcpy(target_buf, s, s_len);

    buffer_commit(s, s_len); 
}