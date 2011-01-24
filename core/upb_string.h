/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 *
 * This file defines a simple string type which is length-delimited instead
 * of NULL-terminated, and which has useful sharing semantics.
 *
 * The overriding goal of upb_string is to avoid memcpy(), malloc(), and free()
 * wheverever possible, while keeping both CPU and memory overhead low.
 * Throughout upb there are situations where one wants to reference all or part
 * of another string without copying.  upb_string provides APIs for doing this.
 *
 * Characteristics of upb_string:
 * - strings are reference-counted.
 * - strings are immutable (can be mutated only when first created or recycled).
 * - if a string has no other referents, it can be "recycled" into a new string
 *   without having to reallocate the upb_string.
 * - strings can be substrings of other strings (owning a ref on the source
 *   string).
 *
 * Reference-counted strings have recently fallen out of favor because of the
 * performance impacts of doing thread-safe reference counting with atomic
 * operations.  We side-step this issue by not performing atomic operations
 * unless the string has been marked thread-safe.
 *
 * Strings are expected to be 8-bit-clean, but "char*" is such an entrenched
 * idiom that we go with it instead of making our pointers uint8_t*.
 *
 * WARNING: THE GETREF, UNREF, AND RECYCLE OPERATIONS ARE NOT THREAD_SAFE
 * UNLESS THE STRING HAS BEEN MARKED SYNCHRONIZED!  What this means is that if
 * you are logically passing a reference to a upb_string to another thread
 * (which implies that the other thread must eventually call unref of recycle),
 * you have two options:
 *
 * - create a copy of the string that will be used in the other thread only.
 * - call upb_string_get_synchronized_ref(), which will make getref, unref, and
 *   recycle thread-safe for this upb_string.
 */

#ifndef UPB_STRING_H
#define UPB_STRING_H

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include "upb_atomic.h"
#include "upb.h"

#ifdef __cplusplus
extern "C" {
#endif

// All members of this struct are private, and may only be read/written through
// the associated functions.
struct _upb_string {
  // The pointer to our currently active data.  This may be memory we own
  // or a pointer into memory we don't own.
  const char *ptr;

  // If non-NULL, this is a block of memory we own.  We keep this cached even
  // if "ptr" is currently aliasing memory we don't own.
  char *cached_mem;

  // The effective length of the string (the bytes at ptr).
  int32_t len;
#ifndef UPB_HAVE_MSIZE
  // How many bytes are allocated in cached_mem.
  //
  // Many platforms have a function that can tell you the size of a block
  // that was previously malloc'd.  In this case we can avoid storing the
  // size explicitly.
  uint32_t size;
#endif

  // The string's refcount.
  upb_atomic_refcount_t refcount;

  // Used if this is a slice of another string, NULL otherwise.  We own a ref
  // on src.
  struct _upb_string *src;
};

// Internal-only initializer for upb_string instances.
#ifdef UPB_HAVE_MSIZE
#define _UPB_STRING_INIT(str, len, refcount) {(char*)str, NULL, len, {refcount}, NULL}
#else
#define _UPB_STRING_INIT(str, len, refcount) {(char*)str, NULL, len, 0, {refcount}, NULL}
#endif

// Special pseudo-refcounts for static/stack-allocated strings, respectively.
#define _UPB_STRING_REFCOUNT_STATIC -1
#define _UPB_STRING_REFCOUNT_STACK -2

// Returns a newly-created, empty, non-finalized string.  When the string is no
// longer needed, it should be unref'd, never freed directly.
upb_string *upb_string_new();

// Internal-only; clients should call upb_string_unref().
void _upb_string_free(upb_string *str);

// Releases a ref on the given string, which may free the memory.  "str"
// can be NULL, in which case this is a no-op.  WARNING: NOT THREAD_SAFE
// UNLESS THE STRING IS SYNCHRONIZED.
INLINE void upb_string_unref(upb_string *str) {
  if (str && upb_atomic_read(&str->refcount) > 0 &&
      upb_atomic_unref(&str->refcount)) {
    _upb_string_free(str);
  }
}

upb_string *upb_strdup(upb_string *s);  // Forward-declare.

// Returns a string with the same contents as "str".  The caller owns a ref on
// the returned string, which may or may not be the same object as "str.
// WARNING: NOT THREAD-SAFE UNLESS THE STRING IS SYNCHRONIZED!
INLINE upb_string *upb_string_getref(upb_string *str) {
  int refcount = upb_atomic_read(&str->refcount);
  if (refcount == _UPB_STRING_REFCOUNT_STACK) return upb_strdup(str);
  // We don't ref the special <0 refcount for static strings.
  if (refcount > 0) upb_atomic_ref(&str->refcount);
  return str;
}

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
// data written to it.  After the function returns, "str" points to a writable
// string, which is either the original string if it had no other references
// or a newly created string if it did have other references.
//
// As a special case, passing a pointer to NULL will allocate a new string.
// This is convenient for the pattern:
//
//   upb_string *str = NULL;
//   while (x) {
//     if (y) {
//       upb_string_recycle(&str);
//       upb_src_getstr(str);
//     }
//   }
void upb_string_recycle(upb_string **str);

// The options for setting the contents of a string.  These may only be called
// when a string is first created or recycled; once other functions have been
// called on the string, these functions are not allowed until the string is
// recycled.

// Gets a pointer suitable for writing to the string, which is guaranteed to
// have at least "len" bytes of data available.  The size of the string will
// become "len".
char *upb_string_getrwbuf(upb_string *str, upb_strlen_t len);

// Replaces the contents of str with the contents of the given printf.
void upb_string_vprintf(upb_string *str, const char *format, va_list args);
INLINE void upb_string_printf(upb_string *str, const char *format, ...) {
  va_list args;
  va_start(args, format);
  upb_string_vprintf(str, format, args);
  va_end(args);
}

// Sets the contents of "str" to be the given substring of "target_str", to
// which the caller must own a ref.
void upb_string_substr(upb_string *str, upb_string *target_str,
                       upb_strlen_t start, upb_strlen_t len);

// Sketch of an API for allowing upb_strings to reference external, unowned
// data.  Waiting for a clear use case before actually implementing it.
//
// Makes the string "str" a reference to the given string data.  The caller
// guarantees that the given string data will not change or be deleted until a
// matching call to upb_string_detach(), which may block until any concurrent
// readers have finished reading.  upb_string_detach() preserves the contents
// of the string by copying the referenced data if there are any other
// referents.
// void upb_string_attach(upb_string *str, char *ptr, upb_strlen_t len);
// void upb_string_detach(upb_string *str);

// Allows using upb_strings in printf, ie:
//   upb_strptr str = UPB_STRLIT("Hello, World!\n");
//   printf("String is: " UPB_STRFMT, UPB_STRARG(str)); */
#define UPB_STRARG(str) upb_string_len(str), upb_string_getrobuf(str)
#define UPB_STRFMT "%.*s"

// Macros for constructing upb_string objects statically or on the stack.  These
// can be used like:
//
// upb_string static_str = UPB_STATIC_STRING("Foo");
//
// int main() {
//   upb_string stack_str = UPB_STACK_STRING("Foo");
//   // Now:
//   //   upb_streql(&static_str, &stack_str) == true
//   //   upb_streql(&static_str, UPB_STRLIT("Foo")) == true
// }
//
// You can also use UPB_STACK_STRING or UPB_STATIC_STRING with character arrays,
// but you must not change the underlying data once you've passed the string on:
//
// void foo() {
//   char data[] = "ABC123";
//   upb_string stack_str = UPB_STACK_STR(data);
//   bar(&stack_str);
//   data[0] = "B";  // NOT ALLOWED!!
// }
//
// TODO: should the stack business just be like attach/detach?  The latter seems
// more flexible, though it does require a stack allocation.  Maybe put this off
// until there is a clear use case.
#define UPB_STATIC_STRING(str) \
    _UPB_STRING_INIT(str, sizeof(str)-1, _UPB_STRING_REFCOUNT_STATIC)
#define UPB_STATIC_STRING_LEN(str, len) \
    _UPB_STRING_INIT(str, len, _UPB_STRING_REFCOUNT_STATIC)
#define UPB_STACK_STRING(str) \
    _UPB_STRING_INIT(str, sizeof(str)-1, _UPB_STRING_REFCOUNT_STACK)
#define UPB_STACK_STRING_LEN(str, len) \
    _UPB_STRING_INIT(str, len, _UPB_STRING_REFCOUNT_STACK)

// A convenient way of specifying upb_strings as literals, like:
//
//   upb_streql(UPB_STRLIT("expected"), other_str);
//
// However, this requires either C99 compound initializers or C++.
// Must ONLY be called with a string literal as its argument!
//#ifdef __cplusplus
//namespace upb {
//class String : public upb_string {
//  // This constructor must ONLY be called with a string literal.
//  String(const char *str) : upb_string(UPB_STATIC_STRING(str)) {}
//};
//}
//#define UPB_STRLIT(str) upb::String(str)
//#endif
#define UPB_STRLIT(str) &(upb_string)UPB_STATIC_STRING(str)

/* upb_string library functions ***********************************************/

// Named like their <string.h> counterparts, these are all safe against buffer
// overflow.  For the most part these only use the public upb_string interface.

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

// Compare a upb_string with memory or a NULL-terminated C string.
INLINE bool upb_streqllen(upb_string *str, const void *buf, upb_strlen_t len) {
  return len == upb_string_len(str) &&
      memcmp(upb_string_getrobuf(str), buf, len) == 0;
}

INLINE bool upb_streqlc(upb_string *str, const void *buf) {
  // Could be made one-pass.
  return upb_streqllen(str, buf, strlen((const char*)buf));
}

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
INLINE void upb_strcpyc(upb_string *dest, const void *src) {
  // This does two passes over src, but that is necessary unless we want to
  // repeatedly re-allocate dst, which seems worse.
  upb_strcpylen(dest, src, strlen((const char*)src));
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
INLINE upb_string *upb_strdupc(const char *src) {
  return upb_strduplen(src, strlen(src));
}

// Appends 'append' to 's' in-place, resizing s if necessary.
void upb_strcat(upb_string *s, upb_string *append);

// Returns a new string that is a substring of the given string.
INLINE upb_string *upb_strslice(upb_string *s, int offset, int len) {
  upb_string *str = upb_string_new();
  upb_string_substr(str, s, offset, len);
  return str;
}

// Reads an entire file into a newly-allocated string.
upb_string *upb_strreadfile(const char *filename);

// Returns a new string with the contents of the given printf.
upb_string *upb_string_asprintf(const char *format, ...);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
