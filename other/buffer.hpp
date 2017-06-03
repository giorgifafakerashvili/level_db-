#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "first.h"

#include "settings.h"

#include <sys/types.h>
#include <time.h>


#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif

typedef struct {
    char* ptr;

    size_t used;

    size_t size;
} buffer;

/* Create new buffer; either empty or copy given data */
buffer* buffer_init(void);
buffer* buffer_init_buffer(const buffer* src); // src can be NULL
buffer* buffer_init_string(const char* str); // str can be NULL

void buffer_free(buffer* b); // b can be NULL

/**
 * Trunaces to used == 0; free large buffers, might kep smaller ones for reuse
 */
void buffer_reset(buffer* b); // b can be NULL

/**
 * Reset b. If NULL != b && NULL != src, move src content to be. reset src
 */
void buffer_move(buffer* b, buffer* src);

/**
 * Make sure buffer is large enough to store a string of given size
 * and a terminating zero
 * sets b to an empty string, and may drop old content
 * @return b->ptr
 */
char* buffer_string_prepare_copy(buffer* b, size_t size);

/**
 *
 */
char* buffer_string_prepare_append(buffer* b, size_t size);

void buffer_string_set_length(buffer* b, size_t len);

void buffer_copy_string(buffer* b, const char* s);
void buffer_copy_string_len(buffer* b, const char* s, size_t s_len);
void buffer_copy_buffer(buffer* b, const buffer* src);

// Convert input to hex and store in buffer
void buffer_copy_string_hex(buffer* b, const char* in, size_t in_len);

void buffer_append_string(buffer* b, const char* s);
void buffer_append_string_len(buffer* b, const char* s, size_t s_len);
void buffer_append_string_buffer(buffer* b, const buffer* src);

void buffer_append_unit_hex(buffer* b, uintmax_t len);
void buffer_append_int(buffer* b, intmax_t val);
void buffer_copy_int(buffer* b, intmax_t val);

void buffer_append_strftime(buffer* b, const char* format, const struct tm* tm);

/* '-' log_10 (2^bits) = bits * log 2/ log 10 < bits * 0.31, termianting 0
 */






/**
 * Inline implementations
 */
static inline int buffer_is_empty(const buffer* b) {
    return NULL == b || 0 == b->used;
}

static inline int buffer_string_is_empty(const buffer* b) {
    return NULL == b || b->used < 2;
}

static inline size_t buffer_string_length(const buffer* b) {
    return NULL != b && 0 != b->used ? b->used  -1 : 0;
}

static inline size_t buffer_string_space(const buffer* b) {
    if(NULL == b || b->size == 0) return 0;
    if(0 == b->used) return b->size - 1;
    return b->size  - b->used;
}

static inline void buffer_append_slash(buffer* b) {
    size_t len = buffer_string_length(b);
    if(len > 0 && '/' != b->ptr[len-1]) BUFFER_APPEND_STRING_CONST(b, "/");
}

#endif // _BUFFER_H_