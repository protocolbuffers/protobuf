/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 *
 * This file defines a simple string type.  The overriding goal of upb_string
 * is to avoid memcpy(), malloc(), and free() wheverever possible, while
 * keeping both CPU and memory overhead low.  Throughout upb there are
 * situations where one wants to reference all or part of another string
 * without copying.  upb_string provides APIs for doing this.
 *
 * Characteristics of upb_string:
 * - strings are reference-counted.
 * - strings are logically immutable.
 * - if a string has no other referents, it can be "recycled" into a new string
 *   without having to reallocate the upb_string.
 * - strings can be substrings of other strings (owning a ref on the source
 *   string).
 * - strings can refer to memory that they do not own, in which case we avoid
 *   copies if possible (the exact strategy for doing this can vary).
 * - strings are not thread-safe by default, but can be made so by calling a
 *   function.  This is not the default because it causes extra CPU overhead.
 */

#ifndef UPB_STRING_H
#define UPB_STRING_H

#include <assert.h>
#include <string.h>
#include "upb_atomic.h"
#include "upb.h"

#ifdef __cplusplus
extern "C" {
#endif

// All members of this struct are private, and may only be read/written through
// the associated functions.  Also, strings may *only* be allocated on the heap.
struct _upb_string {
  char *ptr;
  uint32_t len;
  uint32_t size;
  upb_atomic_refcount_t refcount;
  union {
    // Used if this is a slice of another string.
    struct _upb_string *src;
    // Used if this string is referencing external unowned memory.
    upb_atomic_refcount_t reader_count;
  } extra;
};

// Returns a newly-created, empty, non-finalized string.  When the string is no
// longer needed, it should be unref'd, never freed directly.
upb_string *upb_string_new();

// Releases a ref on the given string, which may free the memory.
void upb_string_unref(upb_string *str);

// Returns a string with the same contents as "str".  The caller owns a ref on
// the returned string, which may or may not be the same object as "str.
upb_string *upb_string_getref(upb_string *str);

// Returns the length of the string.
INLINE upb_strlen_t upb_string_len(upb_string *str) { return str->len; }

// Use to read the bytes of the string.  The caller *must* call
// upb_string_endread() after the data has been read.  The window between
// upb_string_getrobuf() and upb_string_endread() should be kept as short as
// possible, because any pending upb_string_detach() may be blocked until
// upb_string_endread is called().  No other functions may be called on the
// string during this window except upb_string_len().
INLINE const char *upb_string_getrobuf(upb_string *str) { return str->ptr; }
INLINE void upb_string_endread(upb_string *str) { (void)str; }

// Attempts to recycle the string "str" so it may be reused and have different
// data written to it.  The returned string is either "str" if it could be
// recycled or a newly created string if "str" has other references.
upb_string *upb_string_tryrecycle(upb_string *str);

// The three options for setting the contents of a string.  These may only be
// called when a string is first created or recycled; once other functions have
// been called on the string, these functions are not allowed until the string
// is recycled.

// Gets a pointer suitable for writing to the string, which is guaranteed to
// have at least "len" bytes of data available.  The size of the string will
// become "len".
char *upb_string_getrwbuf(upb_string *str, upb_strlen_t len);

// Sets the contents of "str" to be the given substring of "target_str", to
// which the caller must own a ref.
void upb_string_substr(upb_string *str, upb_string *target_str,
                       upb_strlen_t start, upb_strlen_t len);

// Makes the string "str" a reference to the given string data.  The caller
// guarantees that the given string data will not change or be deleted until
// a matching call to upb_string_detach().
void upb_string_attach(upb_string *str, char *ptr, upb_strlen_t len);
void upb_string_detach(upb_string *str);

// Allows using upb_strings in printf, ie:
//   upb_strptr str = UPB_STRLIT("Hello, World!\n");
//   printf("String is: " UPB_STRFMT, UPB_STRARG(str)); */
#define UPB_STRARG(str) upb_strlen(str), upb_string_getrobuf(str)
#define UPB_STRFMT "%.*s"

/* upb_string library functions ***********************************************/

// Named like their <string.h> counterparts, these are all safe against buffer
// overflow.  These only use the public upb_string interface.

// More efficient than upb_strcmp if all you need is to test equality.
INLINE bool upb_streql(upb_string *s1, upb_string *s2) {
  upb_strlen_t len = upb_string_len(s1);
  if(len != upb_string_len(s2)) {
    return false;
  } else {
    bool ret =
        memcmp(upb_string_getrobuf(s1), upb_string_getrobuf(s2), len) == 0;
    upb_string_endread(s1);
    upb_string_endread(s2);
    return ret;
  }
}

// Like strcmp().
int upb_strcmp(upb_string *s1, upb_string *s2);

// Like upb_strcpy, but copies from a buffer and length.
INLINE void upb_strcpylen(upb_string *dest, const void *src, upb_strlen_t len) {
  memcpy(upb_string_getrwbuf(dest, len), src, len);
}

// Replaces the contents of "dest" with the contents of "src".
INLINE void upb_strcpy(upb_string *dest, upb_string *src) {
  upb_strcpylen(dest, upb_string_getrobuf(src), upb_string_len(src));
  upb_string_endread(src);
}

// Like upb_strcpy, but copies from a NULL-terminated string.
INLINE void upb_strcpyc(upb_string *dest, const char *src) {
  // This does two passes over src, but that is necessary unless we want to
  // repeatedly re-allocate dst, which seems worse.
  upb_strcpylen(dest, src, strlen(src));
}

// Returns a new string whose contents are a copy of s.
upb_string *upb_strdup(upb_string *s);

// Like upb_strdup(), but duplicates a given buffer and length.
INLINE upb_string *upb_strduplen(const void *src, upb_strlen_t len) {
  upb_string *s = upb_string_new();
  upb_strcpylen(s, src, len);
  return s;
}

// Like upb_strdup(), but duplicates a C NULL-terminated string.
upb_string *upb_strdupc(const char *src);

// Appends 'append' to 's' in-place, resizing s if necessary.
void upb_strcat(upb_string *s, upb_string *append);

// Returns a new string that is a substring of the given string.
upb_string *upb_strslice(upb_string *s, int offset, int len);

// Reads an entire file into a newly-allocated string.
upb_string *upb_strreadfile(const char *filename);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
