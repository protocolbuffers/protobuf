/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * This file defines the in-memory format for messages, arrays, and strings
 * (which are the three dynamically-allocated structures that make up all
 * protobufs).
 *
 * The members of all structs should be considered private.  Access should
 * only happen through the provided functions.
 *
 * Unlike Google's protobuf, messages contain *pointers* to strings and arrays
 * instead of including them by value.  This makes unused strings and arrays
 * use less memory, and lets the strings and arrays have multiple possible
 * representations (for example, a string could be a slice).  It also gives
 * us more flexibility wrt refcounting.  The cost is that when a field *is*
 * being used, the net memory usage is one pointer more than if we had
 * included the thing directly. */

#ifndef UPB_DATA_H
#define UPB_DATA_H

#include <assert.h>
#include <string.h>
#include "upb.h"
#include "upb_atomic.h"
#include "upb_def.h"
#include "upb_sink.h"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_data *******************************************************************/

// The "base class" of strings, arrays, and messages.  Contains a few flags and
// possibly a reference count.  None of the functions for upb_data are public,
// but some of the constants are.

// typedef upb_atomic_refcount_t upb_data;

// The flags in upb_data.
typedef enum {
  // Set if the object itself was allocated with malloc() and should be freed
  // with free().  This flag would be false if the object was allocated on the
  // stack or is data from the static segment of an object file.  Note that this
  // flag does not apply to the data being referenced by a string or array.
  //
  // If this flag is false, UPB_FLAG_HAS_REFCOUNT must be false also; there is
  // no sense refcounting something that does not need to be freed.
  UPB_DATA_HEAPALLOCATED = 1,

  // Set if the object is frozen against modification.  While an object is
  // frozen, it is suitable for concurrent readonly access.  Note that this
  // flag alone is not a sufficient mechanism for preventing any kind of writes
  // to the object's memory, because the object could still have a refcount.
  UPB_DATA_FROZEN = (1<<1),

  // Set if the object has an embedded refcount.
  UPB_DATA_REFCOUNTED = (1<<2)
} upb_data_flag;

#define REFCOUNT_MASK 0xFFFFFFF8
#define REFCOUNT_SHIFT 3
#define REFCOUNT_ONE (1<<REFCOUNT_SHIFT)

INLINE bool upb_data_hasflag(upb_data *d, upb_data_flag flag) {
  // We read this unsynchronized, because the is_frozen flag (the only flag
  // that can change during the life of a upb_data) may not change if the
  // data has more than one owner.
  return d->v & flag;
}

// INTERNAL-ONLY
INLINE void upb_data_setflag(upb_data *d, upb_data_flag flag) {
  d->v |= flag;
}

INLINE uint32_t upb_data_getrefcount(upb_data *d) {
  int data;
  if(upb_data_hasflag(d, UPB_DATA_FROZEN))
    data = upb_atomic_read(d);
  else
    data = d->v;
  return (data & REFCOUNT_MASK) >> REFCOUNT_SHIFT;
}

// Returns true if the given data has only one owner.
INLINE bool upb_data_only(upb_data *data) {
  return !upb_data_hasflag(data, UPB_DATA_REFCOUNTED) ||
         upb_data_getrefcount(data) == 1;
}

// Specifies the type of ref that is requested based on the kind of access the
// caller needs to the object.
typedef enum {
  // Use when the client plans to perform read-only access to the object, and
  // only in one thread at a time.  This imposes the least requirements on the
  // object; it can be either frozen or not.  As a result, requesting a
  // reference of this type never performs a copy unless the object has no
  // refcount.
  //
  // A ref of this type can always be explicitly converted to frozen or
  // unfrozen later.
  UPB_REF_THREADUNSAFE_READONLY = 0,

  // Use when the client plans to perform read-only access, but from multiple
  // threads concurrently.  This will force the object to eagerly perform any
  // parsing that may have been lazily deferred, and will force a copy if the
  // object is not current frozen.
  //
  // Asking for a reference of this type is equivalent to:
  //   x = getref(y, UPB_REF_THREADUNSAFE_READONLY);
  //   x = freeze(x);
  // ...except it is more efficient.
  UPB_REF_FROZEN = 1,

  // Use when the client plans to perform read/write access.  As a result, the
  // reference will not be thread-safe for concurrent reading *or* writing; the
  // object must be externally synchronized if it is being accessed from more
  // than one thread.  This will force a copy if the object is currently frozen.
  //
  // Asking for a reference of this type is equivalent to:
  //   x = getref(y, UPB_REF_THREADUNSAFE_READONLY);
  //   x = thaw(x);
  // ...except it is more efficient.
  UPB_REF_MUTABLE = 2
} upb_reftype;

// INTERNAL-ONLY FUNCTION:
// Attempts to increment the reference on d with the given type of ref.  If
// this is not possible, returns false.
INLINE bool _upb_data_incref(upb_data *d, upb_reftype reftype) {
  bool frozen = upb_data_hasflag(d, UPB_DATA_FROZEN);
  if((reftype == UPB_REF_FROZEN && !frozen) ||
     (reftype == UPB_REF_MUTABLE && frozen) ||
     (upb_data_hasflag(d, UPB_DATA_HEAPALLOCATED) &&
      !upb_data_hasflag(d, UPB_DATA_REFCOUNTED))) {
    return false;
  }
  // Increment the ref.  Only need to use atomic ops if the ref is frozen.
  if(upb_data_hasflag(d, UPB_DATA_FROZEN)) upb_atomic_add(d, REFCOUNT_ONE);
  else d->v += REFCOUNT_ONE;
  return true;
}

// INTERNAL-ONLY FUNCTION:
// Releases a reference on d, returning true if the object should be deleted.
INLINE bool _upb_data_unref(upb_data *d) {
  if(upb_data_hasflag(d, UPB_DATA_HEAPALLOCATED)) {
    // A heap-allocated object without a refcount should never be decref'd.
    // Its owner owns it exlusively and should free it directly.
    assert(upb_data_hasflag(d, UPB_DATA_REFCOUNTED));
    if(upb_data_hasflag(d, UPB_DATA_FROZEN)) {
      int32_t old_val = upb_atomic_fetch_and_add(d, -REFCOUNT_ONE);
      return (old_val & REFCOUNT_MASK) == REFCOUNT_ONE;
    } else {
      d->v -= REFCOUNT_ONE;
      return (d->v & REFCOUNT_MASK) == 0;
    }
  } else {
    // Non heap-allocated data never should be deleted.
    return false;
  }
}

/* upb_string *****************************************************************/

// We have several different representations for string, depending on whether
// it has a refcount (and likely in the future, depending on whether it is a
// slice of another string).  We could just have one representation with
// members that are sometimes unused, but this is wasteful in memory.  The
// flags that are always part of the first word tell us which representation
// to use.
//
// In a way, this is like inheritance but instead of using a virtual pointer,
// we do switch/case in every "virtual" method.  This may sound expensive but
// in many cases the different cases compile to exactly the same code, so there
// is no branch.

struct upb_norefcount_string {
  uint32_t byte_size_and_flags;
  upb_strlen_t byte_len;
  // We expect the data to be 8-bit clean (uint8_t), but char* is such an
  // ingrained convention that we follow it.
  char *ptr;
};

// Used for a string with a refcount.
struct upb_refcounted_string {
  upb_data base;
  upb_strlen_t byte_len;
  char *ptr;
  uint32_t byte_size;
};


// Returns a newly constructed, refcounted string which starts out empty.
// Caller owns one ref on it.  The returned string will not be frozen.
upb_strptr upb_string_new(void);

// INTERNAL-ONLY:
// Frees the given string, alone with any memory the string owned.
void _upb_string_free(upb_strptr s);

// Returns a string to which caller owns a ref, and contains the same contents
// as src.  The returned value may be a copy of src, if the requested flags
// were incompatible with src's.
upb_strptr upb_string_getref(upb_strptr s, int ref_flags);

#define UPB_STRING_NULL_INITIALIZER {NULL}
static const upb_strptr UPB_STRING_NULL = UPB_STRING_NULL_INITIALIZER;
INLINE bool upb_string_isnull(upb_strptr s) { return s.base == NULL; }

// The caller releases a ref on src, which it must previously have owned a ref
// on.
INLINE void upb_string_unref(upb_strptr s) {
  if(_upb_data_unref(s.base)) _upb_string_free(s);
}

// The string is resized to byte_len.  The string must not be frozen.
void upb_string_resize(upb_strptr s, upb_strlen_t len);

// Returns a buffer to which the caller may write.  The string is resized to
// byte_len (which may or may not trigger a reallocation).  The string must not
// be frozen.
INLINE char *upb_string_getrwbuf(upb_strptr s, upb_strlen_t byte_len) {
  upb_string_resize(s, byte_len);
  if(upb_data_hasflag(s.base, UPB_DATA_REFCOUNTED))
    return s.refcounted->ptr;
  else
    return s.norefcount->ptr;
}

INLINE void upb_string_clear(upb_strptr s) {
  upb_string_getrwbuf(s, 0);
}

// INTERNAL-ONLY:
// Gets/sets the pointer.
INLINE char *_upb_string_getptr(upb_strptr s) {
  if(upb_data_hasflag(s.base, UPB_DATA_REFCOUNTED))
    return s.refcounted->ptr;
  else
    return s.norefcount->ptr;
}

// Returns a buffer that the caller may use to read the current contents of
// the string.  The number of bytes available is upb_strlen(s).
INLINE const char *upb_string_getrobuf(upb_strptr s) {
  return _upb_string_getptr(s);
}

// Returns the current length of the string.
INLINE upb_strlen_t upb_strlen(upb_strptr s) {
  if(upb_data_hasflag(s.base, UPB_DATA_REFCOUNTED))
    return s.refcounted->byte_len;
  else
    return s.norefcount->byte_len;
}

/* upb_string library functions ***********************************************/

// Named like their <string.h> counterparts, these are all safe against buffer
// overflow.  These only use the public upb_string interface.

// More efficient than upb_strcmp if all you need is to test equality.
INLINE bool upb_streql(upb_strptr s1, upb_strptr s2) {
  upb_strlen_t len = upb_strlen(s1);
  if(len != upb_strlen(s2)) {
    return false;
  } else {
    return memcmp(upb_string_getrobuf(s1), upb_string_getrobuf(s2), len) == 0;
  }
}

// Like strcmp().
int upb_strcmp(upb_strptr s1, upb_strptr s2);

// Like upb_strcpy, but copies from a buffer and length.
INLINE void upb_strcpylen(upb_strptr dest, const void *src, upb_strlen_t len) {
  memcpy(upb_string_getrwbuf(dest, len), src, len);
}

// Replaces the contents of "dest" with the contents of "src".
INLINE void upb_strcpy(upb_strptr dest, upb_strptr src) {
  upb_strcpylen(dest, upb_string_getrobuf(src), upb_strlen(src));
}

// Like upb_strcpy, but copies from a NULL-terminated string.
INLINE void upb_strcpyc(upb_strptr dest, const char *src) {
  // This does two passes over src, but that is necessary unless we want to
  // repeatedly re-allocate dst, which seems worse.
  upb_strcpylen(dest, src, strlen(src));
}

// Returns a new string whose contents are a copy of s.
upb_strptr upb_strdup(upb_strptr s);

// Like upb_strdup(), but duplicates a given buffer and length.
INLINE upb_strptr upb_strduplen(const void *src, upb_strlen_t len) {
  upb_strptr s = upb_string_new();
  upb_strcpylen(s, src, len);
  return s;
}

// Like upb_strdup(), but duplicates a C NULL-terminated string.
upb_strptr upb_strdupc(const char *src);

// Appends 'append' to 's' in-place, resizing s if necessary.
void upb_strcat(upb_strptr s, upb_strptr append);

// Returns a string that is a substring of the given string.  Currently this
// returns a copy, but in the future this may return an object that references
// the original string data instead of copying it.  Both now and in the future,
// the caller owns a ref on whatever is returned.
upb_strptr upb_strslice(upb_strptr s, int offset, int len);

// Reads an entire file into a newly-allocated string (caller owns one ref).
upb_strptr upb_strreadfile(const char *filename);

// Typedef for a read-only string that is allocated statically or on the stack.
// Initialize with the given macro, which must resolve to a const char*.  You
// must not dynamically allocate this type.  Example usage:
//
//   upb_static_string mystr = UPB_STATIC_STRING_INIT("biscuits");
//   upb_strptr mystr_ptr = UPB_STATIC_STRING_PTR_INIT(mystr);
//
// If C99 compund literals are available, the much nicer UPB_STRLIT macro is
// available instead:
//
//   upb_strtr mystr_ptr = UPB_STRLIT("biscuits");
//
typedef struct upb_norefcount_string upb_static_string;
#define UPB_STATIC_STRING_INIT_LEN(str, len) {0 | UPB_DATA_FROZEN, len, str}
#define UPB_STATIC_STRING_INIT(str) UPB_STATIC_STRING_INIT_LEN(str, sizeof(str)-1)
#define UPB_STATIC_STRING_PTR_INIT(static_string) {&static_string}
#define UPB_STRLIT(str) (upb_strptr){&(upb_static_string)UPB_STATIC_STRING_INIT(str)}

// Allows using upb_strings in printf, ie:
//   upb_strptr str = UPB_STRLIT("Hello, World!\n");
//   printf("String is: " UPB_STRFMT, UPB_STRARG(str)); */
#define UPB_STRARG(str) upb_strlen(str), upb_string_getrobuf(str)
#define UPB_STRFMT "%.*s"

/* upb_array ******************************************************************/

// The comments attached to upb_string above also apply here.
struct upb_norefcount_array {
  upb_data base;  // We co-opt the refcount for the size.
  upb_arraylen_t len;
  upb_valueptr elements;
};

struct upb_refcounted_array {
  upb_data base;
  upb_arraylen_t len;
  upb_valueptr elements;
  upb_arraylen_t size;
};

typedef struct upb_norefcount_array upb_static_array;
#define UPB_STATIC_ARRAY_INIT(arr, len) {{0 | UPB_DATA_FROZEN}, len, {._void=arr}}
#define UPB_STATIC_ARRAY_PTR_TYPED_INIT(static_arr) {{&static_arr}}

#define UPB_ARRAY_NULL_INITIALIZER {NULL}
static const upb_arrayptr UPB_ARRAY_NULL = UPB_ARRAY_NULL_INITIALIZER;
INLINE bool upb_array_isnull(upb_arrayptr a) { return a.base == NULL; }
INLINE bool upb_array_ptreql(upb_arrayptr a1, upb_arrayptr a2) {
  return a1.base == a2.base;
}

#define UPB_MSG_ARRAYPTR(type) type ## _array
#define UPB_DEFINE_MSG_ARRAY(type) \
typedef struct { upb_arrayptr ptr; } UPB_MSG_ARRAYPTR(type); \
INLINE upb_arraylen_t type ## _array_len(UPB_MSG_ARRAYPTR(type) a) { \
  return upb_array_len(a.ptr); \
} \
INLINE type* type ## _array_get(UPB_MSG_ARRAYPTR(type) a, upb_arraylen_t elem) { \
  return *(type**)_upb_array_getptr_raw(a.ptr, elem, sizeof(void*))._void; \
}

// Constructs a newly-allocated, reference-counted array which starts out
// empty.  Caller owns one ref on it.
upb_arrayptr upb_array_new(void);

// Returns the current number of elements in the array.
INLINE size_t upb_array_len(upb_arrayptr a) {
  if(upb_data_hasflag(a.base, UPB_DATA_REFCOUNTED))
    return a.refcounted->len;
  else
    return a.norefcount->len;
}

// INTERNAL-ONLY:
// Frees the given message and releases references on members.
void _upb_array_free(upb_arrayptr a, upb_fielddef *f);

// INTERNAL-ONLY:
// Returns a pointer to the given elem.
INLINE upb_valueptr _upb_array_getptr_raw(upb_arrayptr a, upb_arraylen_t elem,
                                          size_t type_size) {
  upb_valueptr p;
  if(upb_data_hasflag(a.base, UPB_DATA_REFCOUNTED))
    p._void = &a.refcounted->elements.uint8[elem * type_size];
  else
    p._void = &a.norefcount->elements.uint8[elem * type_size];
  return p;
}

INLINE upb_valueptr _upb_array_getptr(upb_arrayptr a, upb_fielddef *f,
                                      upb_arraylen_t elem) {
  return _upb_array_getptr_raw(a, elem, upb_types[f->type].size);
}

INLINE upb_value upb_array_get(upb_arrayptr a, upb_fielddef *f,
                               upb_arraylen_t elem) {
  assert(elem < upb_array_len(a));
  return upb_value_read(_upb_array_getptr(a, f, elem), f->type);
}

// The caller releases a ref on the given array, which it must previously have
// owned a ref on.
INLINE void upb_array_unref(upb_arrayptr a, upb_fielddef *f) {
  if(_upb_data_unref(a.base)) _upb_array_free(a, f);
}

#if 0
// Returns an array to which caller owns a ref, and contains the same contents
// as src.  The returned value may be a copy of src, if the requested flags
// were incompatible with src's.
INLINE upb_arrayptr upb_array_getref(upb_arrayptr src, int ref_flags);

// Sets the given element in the array to val.  The current length of the array
// must be greater than elem.  If the field type is dynamic, the array will
// take a ref on val and release a ref on what was previously in the array.
INLINE void upb_array_set(upb_arrayptr a, upb_fielddef *f, int elem,
                          upb_value val);


// Note that array_append will attempt to take a reference on the given value,
// so to avoid a copy use append_default and get.
INLINE void upb_array_append(upb_arrayptr a, upb_fielddef *f,
                             upb_value val);
INLINE void upb_array_append_default(upb_arrayptr a, upb_fielddef *f,
                             upb_value val);
#endif

INLINE void upb_array_truncate(upb_arrayptr a) {
  if(upb_data_hasflag(a.base, UPB_DATA_REFCOUNTED))
    a.refcounted->len = 0;
  else
    a.norefcount->len = 0;
}


/* upb_msg ********************************************************************/

// Note that some inline functions for upb_msg are defined in upb_def.h since
// they rely on the defs.

struct _upb_msg {
  upb_data base;
  uint8_t data[4];  // We allocate the appropriate amount per message.
};

// Creates a new msg of the given type.
upb_msg *upb_msg_new(upb_msgdef *md);

// INTERNAL-ONLY:
// Frees the given message and releases references on members.
void _upb_msg_free(upb_msg *msg, upb_msgdef *md);

// INTERNAL-ONLY:
// Returns a pointer to the given field.
INLINE upb_valueptr _upb_msg_getptr(upb_msg *msg, upb_fielddef *f) {
  upb_valueptr p;
  p._void = &msg->data[f->byte_offset];
  return p;
}

// Releases a references on msg.
INLINE void upb_msg_unref(upb_msg *msg, upb_msgdef *md) {
  if(_upb_data_unref(&msg->base)) _upb_msg_free(msg, md);
}

// Tests whether the given field is explicitly set, or whether it will return
// a default.
INLINE bool upb_msg_has(upb_msg *msg, upb_fielddef *f) {
  return (msg->data[f->field_index/8] & (1 << (f->field_index % 8))) != 0;
}

// Returns the current value if set, or the default value if not set, of the
// specified field.  The caller does *not* own a ref.
INLINE upb_value upb_msg_get(upb_msg *msg, upb_fielddef *f) {
  if(upb_msg_has(msg, f)) {
    return upb_value_read(_upb_msg_getptr(msg, f), f->type);
  } else {
    return f->default_value;
  }
}

// Sets the given field to the given value.  The msg will take a ref on val,
// and will drop a ref on whatever was there before.
void upb_msg_set(upb_msg *msg, upb_fielddef *f, upb_value val);

INLINE void upb_msg_clear(upb_msg *msg, upb_msgdef *md) {
  memset(msg->data, 0, md->set_flags_bytes);
}

// A convenience function for decoding an entire protobuf all at once, without
// having to worry about setting up the appropriate objects.
void upb_msg_decodestr(upb_msg *msg, upb_msgdef *md, upb_strptr str,
                       upb_status *status);

// A convenience function for encoding an entire protobuf all at once.  If an
// error occurs, the null string is returned and the status object contains
// the error.
void upb_msg_encodestr(upb_msg *msg, upb_msgdef *md, upb_strptr str,
                       upb_status *status);


/* upb_msgsrc *****************************************************************/

// A nonresumable, non-interruptable (but simple and fast) source for pushing
// the data of a upb_msg to a upb_sink.
void upb_msgsrc_produce(upb_msg *msg, upb_msgdef *md, upb_sink *sink,
                        bool reverse, upb_status *status);


/* upb_msgsink ****************************************************************/

// A upb_msgsink can accept the data from a source and write it into a message.
struct upb_msgsink;
typedef struct upb_msgsink upb_msgsink;

// Allocate and free a msgsink, respectively.
upb_msgsink *upb_msgsink_new(upb_msgdef *md);
void upb_msgsink_free(upb_msgsink *sink);

// Returns the upb_sink (like an upcast).
upb_sink *upb_msgsink_sink(upb_msgsink *sink);

// Resets the msgsink for the given msg.
void upb_msgsink_reset(upb_msgsink *sink, upb_msg *msg);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
