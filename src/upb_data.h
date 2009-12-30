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

struct upb_msgdef;
struct upb_fielddef;

/* upb_data *******************************************************************/

// The "base class" of strings, arrays, and messages.  Contains a few flags and
// possibly a reference count.  None of the functions for upb_data are public,
// but some of the constants are.
typedef upb_atomic_refcount_t upb_data;

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
  if((reftype == UPB_REF_FROZEN && !upb_data_hasflag(d, UPB_DATA_FROZEN)) ||
     (reftype == UPB_REF_MUTABLE && upb_data_hasflag(d, UPB_DATA_FROZEN)) ||
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

typedef uint32_t upb_strlen_t;

// We have several different representations for string, depending on whether
// it has a refcount (and likely in the future, depending on whether it is a
// slice of another string).  We could just have one representation with
// members that are sometimes unused, but this is wasteful in memory.  The
// flags that are always part of the first word tell us which representation
// to use.
//
// upb_string_common is the members that are common to all representations.
typedef struct {
  upb_data base;
  upb_strlen_t byte_len;
  // We expect the data to be 8-bit clean (uint8_t), but char* is such an
  // ingrained convention that we follow it.
  char *ptr;
} upb_string_common;

// Used for a string without a refcount.
typedef struct {
  uint32_t byte_size_and_flags;
  upb_strlen_t byte_len;
  char *ptr;
} upb_norefcount_string;

// Used for a string with a refcount.
typedef struct {
  upb_data base;
  upb_strlen_t byte_len;
  char *ptr;
  uint32_t byte_size;
} upb_refcounted_string;

union upb_string {
  upb_norefcount_string norefcount;
  upb_string_common common;
  upb_refcounted_string refcounted;
};

// Returns a newly constructed, refcounted string which starts out empty.
// Caller owns one ref on it.  The returned string will not be frozen.
upb_string *upb_string_new(void);

// Creates a new string which is a duplicate of the given string.  If
// refcounted is true, the new string is refcounted, otherwise the caller
// has exlusive ownership of it.
INLINE upb_string *upb_strdup(upb_string *s);

// INTERNAL-ONLY:
// Frees the given string, alone with any memory the string owned.
void _upb_string_free(upb_string *s);

// Returns a string to which caller owns a ref, and contains the same contents
// as src.  The returned value may be a copy of src, if the requested flags
// were incompatible with src's.
INLINE upb_string *upb_string_getref(upb_string *s, int ref_flags) {
  if(_upb_data_incref(&s->common.base, ref_flags)) return s;
  return upb_strdup(s);
}

// The caller releases a ref on src, which it must previously have owned a ref
// on.
INLINE void upb_string_unref(upb_string *s) {
  if(_upb_data_unref(&s->common.base)) _upb_string_free(s);
}

// The string is resized to byte_len.  The string must not be frozen.
void upb_string_resize(upb_string *s, upb_strlen_t len);

// Returns a buffer to which the caller may write.  The string is resized to
// byte_len (which may or may not trigger a reallocation).  The string must not
// be frozen.
INLINE char *upb_string_getrwbuf(upb_string *s, upb_strlen_t byte_len) {
  upb_string_resize(s, byte_len);
  return s->common.ptr;
}

INLINE void upb_string_clear(upb_string *s) {
  upb_string_getrwbuf(s, 0);
}

// Returns a buffer that the caller may use to read the current contents of
// the string.  The number of bytes available is upb_strlen(s).
INLINE const char *upb_string_getrobuf(upb_string *s) {
  return s->common.ptr;
}

// Returns the current length of the string.
INLINE upb_strlen_t upb_strlen(upb_string *s) {
  return s->common.byte_len;
}

/* upb_string library functions ***********************************************/

// Named like their <string.h> counterparts, these are all safe against buffer
// overflow.  These only use the public upb_string interface.

// More efficient than upb_strcmp if all you need is to test equality.
INLINE bool upb_streql(upb_string *s1, upb_string *s2) {
  upb_strlen_t len = upb_strlen(s1);
  if(len != upb_strlen(s2)) {
    return false;
  } else {
    return memcmp(upb_string_getrobuf(s1), upb_string_getrobuf(s2), len) == 0;
  }
}

INLINE int upb_strcmp(upb_string *s1, upb_string *s2) {
  upb_strlen_t common_length = UPB_MIN(upb_strlen(s1), upb_strlen(s2));
  int common_diff = memcmp(upb_string_getrobuf(s1), upb_string_getrobuf(s2),
                           common_length);
  return common_diff ==
      0 ? ((int)upb_strlen(s1) - (int)upb_strlen(s2)) : common_diff;
}

INLINE void upb_strcpy(upb_string *dest, upb_string *src) {
  upb_strlen_t src_len = upb_strlen(src);
  memcpy(upb_string_getrwbuf(dest, src_len), upb_string_getrobuf(src), src_len);
}

INLINE upb_string *upb_strdup(upb_string *s) {
  upb_string *copy = upb_string_new();
  upb_strcpy(copy, s);
  return copy;
}

INLINE upb_string *upb_strdupc(const char *src) {
  upb_string *copy = upb_string_new();
  upb_strlen_t len = strlen(src);
  char *buf = upb_string_getrwbuf(copy, len);
  memcpy(buf, src, len);
  return copy;
}

// Appends 'append' to 's' in-place, resizing s if necessary.
INLINE void upb_strcat(upb_string *s, upb_string *append) {
  upb_strlen_t s_len = upb_strlen(s);
  upb_strlen_t append_len = upb_strlen(append);
  upb_strlen_t newlen = s_len + append_len;
  memcpy(upb_string_getrwbuf(s, newlen) + s_len,
         upb_string_getrobuf(append), append_len);
}

// Returns a string that is a substring of the given string.  Currently this
// returns a copy, but in the future this may return an object that references
// the original string data instead of copying it.  Both now and in the future,
// the caller owns a ref on whatever is returned.
INLINE upb_string *upb_strslice(upb_string *s, int offset, int len) {
  upb_string *slice = upb_string_new();
  len = UPB_MIN((upb_strlen_t)len, upb_strlen(s) - (upb_strlen_t)offset);
  memcpy(upb_string_getrwbuf(slice, len), upb_string_getrobuf(s) + offset, len);
  return slice;
}

// Reads an entire file into a newly-allocated string (caller owns one ref).
upb_string *upb_strreadfile(const char *filename);

// Typedef for a read-only string that is allocated statically or on the stack.
// Initialize with the given macro, which must resolve to a const char*.  You
// must not dynamically allocate this type.
typedef upb_string upb_static_string;
#define UPB_STRLIT_LEN(str, len) {0 | UPB_DATA_FROZEN, len, str}
#define UPB_STRLIT(str) {{0 | UPB_DATA_FROZEN, sizeof(str), str}}

// Allows using upb_strings in printf, ie:
//   upb_string str = UPB_STRLIT("Hello, World!\n");
//   printf("String is: " UPB_STRFMT, UPB_STRARG(str)); */
#define UPB_STRARG(str) (str)->common.byte_len, (str)->common.ptr
#define UPB_STRFMT "%.*s"

/* upb_array ******************************************************************/

typedef uint32_t upb_arraylen_t;

// The comments attached to upb_string above also apply here.
typedef struct {
  upb_data base;
  upb_arraylen_t len;
  union upb_value_ptr elements;
} upb_array_common;

typedef struct {
  uint32_t size_and_flags;
  upb_arraylen_t len;
  union upb_value_ptr elements;
} upb_norefcount_array;

typedef struct {
  upb_data base;
  upb_arraylen_t len;
  union upb_value_ptr elements;
  upb_arraylen_t size;
} upb_refcounted_array;

union upb_array {
  upb_norefcount_array norefcount;
  upb_array_common common;
  upb_refcounted_array refcounted;
};

// This type can be used either to perform read-only access on an array,
// or to statically define a non-reference-counted static array.
#define UPB_DEFINE_MSG_ARRAY(type) \
typedef struct type ## _array { \
  upb_data base; \
  upb_arraylen_t len;\
  type **elements; \
} type ## _array; \

#define UPB_MSG_ARRAY(type) struct type ## _array

// Constructs a newly-allocated, reference-counted array which starts out
// empty.  Caller owns one ref on it.
upb_array *upb_array_new(void);

// INTERNAL-ONLY:
// Frees the given message and releases references on members.
void _upb_array_free(upb_array *a, struct upb_fielddef *f);

// INTERNAL-ONLY:
// Returns a pointer to the given elem.
INLINE union upb_value_ptr _upb_array_getptr(upb_array *a,
                                             struct upb_fielddef *f, int elem) {
  assert(elem < upb_array_len(a));
  size_t type_size = upb_type_info[f->type].size;
  union upb_value_ptr p = {._void = &a->common.elements.uint8[elem * type_size]};
  return p;
}

INLINE union upb_value upb_array_get(upb_array *a, struct upb_fielddef *f,
                                     int elem) {
  return upb_value_read(_upb_array_getptr(a, f, elem), f->type);
}

// The caller releases a ref on the given array, which it must previously have
// owned a ref on.
INLINE void upb_array_unref(upb_array *a, struct upb_fielddef *f) {
  if(_upb_data_unref(&a->common.base)) _upb_array_free(a, f);
}

#if 0
// Returns an array to which caller owns a ref, and contains the same contents
// as src.  The returned value may be a copy of src, if the requested flags
// were incompatible with src's.
INLINE upb_array *upb_array_getref(upb_array *src, int ref_flags);

// Sets the given element in the array to val.  The current length of the array
// must be greater than elem.  If the field type is dynamic, the array will
// take a ref on val and release a ref on what was previously in the array.
INLINE void upb_array_set(upb_array *a, struct upb_fielddef *f, int elem,
                          union upb_value val);


// Note that array_append will attempt to take a reference on the given value,
// so to avoid a copy use append_default and get.
INLINE void upb_array_append(upb_array *a, struct upb_fielddef *f,
                             union upb_value val);
INLINE void upb_array_append_default(upb_array *a, struct upb_fielddef *f,
                             union upb_value val);
#endif

// Returns the current number of elements in the array.
INLINE size_t upb_array_len(upb_array *a) {
  return a->common.len;
}

/* upb_msg ********************************************************************/

// Note that some inline functions for upb_msg are defined in upb_def.h since
// they rely on the defs.

struct upb_msg {
  upb_data base;
  uint8_t data[4];  // We allocate the appropriate amount per message.
};

// Creates a new msg of the given type.
upb_msg *upb_msg_new(struct upb_msgdef *md);

// INTERNAL-ONLY:
// Frees the given message and releases references on members.
void _upb_msg_free(upb_msg *msg, struct upb_msgdef *md);

// INTERNAL-ONLY:
// Returns a pointer to the given field.
INLINE union upb_value_ptr _upb_msg_getptr(upb_msg *msg, struct upb_fielddef *f) {
  union upb_value_ptr p = {._void = &msg->data[f->byte_offset]};
  return p;
}

// Releases a references on msg.
INLINE void upb_msg_unref(upb_msg *msg, struct upb_msgdef *md) {
  if(_upb_data_unref(&msg->base)) _upb_msg_free(msg, md);
}

// Tests whether the given field is explicitly set, or whether it will return
// a default.
INLINE bool upb_msg_has(upb_msg *msg, struct upb_fielddef *f) {
  return msg->data[f->field_index/8] % (1 << (f->field_index % 8));
}

// Returns the current value if set, or the default value if not set, of the
// specified field.  The caller does *not* own a ref.
INLINE union upb_value upb_msg_get(upb_msg *msg, struct upb_fielddef *f) {
  if(upb_msg_has(msg, f)) {
    return upb_value_read(_upb_msg_getptr(msg, f), f->type);
  } else {
    return f->default_value;
  }
}

// Sets the given field to the given value.  The msg will take a ref on val,
// and will drop a ref on whatever was there before.
void upb_msg_set(upb_msg *msg, struct upb_fielddef *f, union upb_value val);

void upb_msg_clear(upb_msg *msg, struct upb_msgdef *md);

void upb_msg_parsestr(upb_msg *msg, struct upb_msgdef *md, upb_string *data,
                      struct upb_status *status);

#endif
