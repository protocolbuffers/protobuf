/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * This file defines the in-memory format for messages, arrays, and strings
 * (which are the three dynamically-allocated structures that make up all
 * protobufs). */

#ifndef UPB_DATA_H
#define UPB_DATA_H

#include <string.h>
#include "upb.h"
#include "upb_def.h"

// Set if the object itself was allocated with malloc() and should be freed
// with free().  This flag would be false if the object was allocated on the
// stack or is data from the static segment of an object file.  Note that this
// flag does not apply to the data being referenced by a string or array.
//
// If this flag is false, UPB_FLAG_HAS_REFCOUNT must be false also; there is
// no sense refcounting something that does not need to be freed.
#define UPB_FLAG_IS_HEAP_ALLOCATED (1<<2)

// Set if the object is frozen against modification.  While an object is
// frozen, it is suitable for concurrent access.  Note that this flag alone is
// not a sufficient mechanism for preventing any kind of writes to the object's
// memory, because the object could still have a refcount and/or reflist.
#define UPB_FLAG_IS_FROZEN (1<<3)

// Set if the object has an embedded refcount.
#define UPB_FLAG_HAS_REFCOUNT (1<<4)

// Specifies the type of ref that is requested based on the kind of access the
// caller needs to the object.
enum upb_ref_flags {
  // Use when the client plans to perform read-only access to the object, and
  // only in one thread at a time.  This imposes the least requirements on the
  // object; it can be either frozen or not.  As a result, requesting a
  // reference of this type never performs a copy unless the object has no
  // refcount.
  UPB_REF_THREADUNSAFE_READONLY = 0,

  // Use when the client plans to perform read-only access, but from multiple
  // threads concurrently.  This will force the object to eagerly perform any
  // parsing that may have been lazily deferred, and will force a copy if the
  // object is not current frozen and there are any other referents (who expect
  // the object to stay writable).
  //
  // Asking for a reference of this type is equivalent to:
  //   x = getref(y, UPB_REF_THREADUNSAFE_READONLY);
  //   x = freeze(x);
  // ...except it is more efficient.
  UPB_REF_FROZEN = 1,

  // Use when the client plans to perform read/write access.  As a result, the
  // reference will not be thread-safe for concurrent reading *or* writing; the
  // object must be externally synchronized if it is being accessed from more
  // than one thread.  This will force a copy if the object is not currently
  // frozen and there are any other referents (who expect the object to stay
  // safe for unsynchronized reads).
  //
  // Asking for a reference of this type is equivalent to:
  //   x = getref(y, UPB_REF_THREADUNSAFE_READONLY);
  //   x = thaw(x);
  // ...except it is more efficient.
  UPB_REF_MUTABLE = 2
}

typedef uint8_t upb_flags_t;
struct upb_mmhead {};

/* upb_string *****************************************************************/

typedef uint32_t upb_strlen_t;

// The members of this struct are private.  Access should only be through the
// associated functions.
typedef struct upb_string {
  unsigned int byte_size:29;  // How many bytes we own, 0 if we don't own.
  bool is_heap_allocated:1;
  bool is_frozen:1;
  // TODO.  At the moment, all dynamically allocated strings have refcounts.
  bool has_refcount:1;
  upb_strlen_t byte_len;
  // We expect the data to be 8-bit clean (uint8_t), but char* is such an
  // ingrained convention that we follow it.
  char *ptr;
} upb_string;

typedef struct {
  upb_string s;
  upb_atomic_refcount_t refcount;
} upb_refcounted_string;

// Returns a newly constructed string, which starts out empty.  Caller owns one
// ref on it.
upb_string *upb_string_new(void);

// Returns a string to which caller owns a ref, and contains the same contents
// as src.  The returned value may be a copy of src, if the requested flags
// were incompatible with src's.
INLINE upb_string *upb_string_getref(upb_string *_s, upb_flags_t ref_flags) {
  upb_refcount_string *s = _s;
  if((ref_flags == UPB_REF_FROZEN && !s->is_frozen) ||
     (ref_flags == UPB_REF_MUTABLE && s->is_frozen) ||
     !s->has_refcount) {
    // Copy should always be refcounted.  Should be frozen iff a frozen ref was req.
    return upb_strdup(s, flags);
  }
  // Take a ref on the existing object.
  if(s->is_frozen) upb_atomic_ref(s->refcount);
  else s->refcount.val++;
  return s;
}

// The caller releases a ref on src, which it must previously have owned a ref
// on.
INLINE void upb_string_unref(upb_string *s) {
  void *refcount = s + 1;
  if(s->is_frozen) {
}

// Returns a buffer to which the caller may write.  The string is resized to
// byte_len (which may or may not trigger a reallocation).  The src string must
// not be frozen otherwise the program will assert-fail or abort().
char *upb_string_getrwbuf(upb_string *s, upb_strlen_t byte_len);

// Returns a buffer that the caller may use to read the current contents of
// the string.  The number of bytes available is upb_strlen(s).
INLINE const char *upb_string_getrobuf(upb_string *s) {
  return s->ptr;
}

// Returns the current length of the string.
INLINE size_t upb_strlen(upb_string *s) {
  return s->byte_len;
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

// Reads an entire file into a newly-allocated string (caller owns one ref).
upb_string *upb_strreadfile(const char *filename);

// Typedef for a read-only string that is allocated statically or on the stack.
// Initialize with the given macro, which must resolve to a const char*.  You
// must not dynamically allocate this type.
typedef upb_string upb_static_string;
#define UPB_STRLIT(str) {sizeof(str), false, true, false, 0, str}

// Allows using upb_strings in printf, ie:
//   upb_string str = UPB_STRLIT("Hello, World!\n");
//   printf("String is: " UPB_STRFMT, UPB_STRARG(str)); */
#define UPB_STRARG(str) (str)->byte_len, (str)->ptr
#define UPB_STRFMT "%.*s"

/* upb_array ******************************************************************/

typedef uint32_t upb_arraylen_t;

// The members of this struct are private.  Access should only be through the
// associated functions.
typedef struct {
  unsigned int size:29;  // How many bytes we own, 0 if we don't own.
  bool is_heap_allocated:1;
  bool is_frozen:1;
  bool has_refcount:1;
  upb_arraylen_t len;
  union upb_value_ptr *elements;
} upb_array;

#define UPB_DEFINE_MSG_ARRAY(type) \
typedef struct type ## array { \
  unsigned int size:29;  \
  bool is_heap_allocated:1; \
  bool is_frozen:1;\
  bool has_refcount:1;\
  upb_arraylen_t len;\
  type **elements; \
} type ## array; \

#define UPB_MSG_ARRAY(type) struct type ## array

// Constructs a newly-allocated array, which starts out empty.  Caller owns one
// ref on it.
upb_array *upb_array_new(void);

// Returns an array to which caller owns a ref, and contains the same contents
// as src.  The returned value may be a copy of src, if the requested flags
// were incompatible with src's.
INLINE upb_array *upb_array_getref(upb_array *src, upb_flags_t flags);

// The caller releases a ref on the given array, which it must previously have
// owned a ref on.
INLINE void upb_array_unref(upb_array *a, struct upb_fielddef *f);

// Sets the given element in the array to val.  The current length of the array
// must be greater than elem.  If the field type is dynamic, the array will
// take a ref on val and release a ref on what was previously in the array.
INLINE void upb_array_set(upb_array *a, struct upb_fielddef *f, int elem,
                          union upb_value val);

// Note that the caller does *not* own a ref on the returned value.
INLINE union upb_value upb_array_get(upb_array *a, struct upb_fielddef *f,
                                     int elem);
INLINE union upb_value upb_array_getmutable(upb_array *a,
                                            struct upb_fielddef *f, int elem,
                                            union upb_value val);

// Note that array_append will attempt to take a reference on the given value,
// so to avoid a copy use append_default and get.
INLINE void upb_array_append(upb_array *a, struct upb_fielddef *f,
                             union upb_value val);
INLINE void upb_array_append_default(upb_array *a, struct upb_fielddef *f,
                             union upb_value val);

// Returns the current number of elements in the array.
INLINE size_t upb_array_len(upb_array *a) {
  return a->len;
}

/* upb_msg ********************************************************************/

typedef struct {
  uint8_t data[1];
} upb_msg;

// Creates a new msg of the given type.
upb_msg *upb_msg_new(struct upb_msgdef *md);

void upb_msg_unref(upb_msg *msg, struct upb_msgdef *md);

// Tests whether the given field is explicitly set, or whether it will return
// a default.
bool upb_msg_isset(upb_msg *msg, struct upb_fielddef *f);

// Returns the current value if set, or the default value if not set, of the
// specified field.  The mutable version will first replace the value with a
// mutable copy if it is not already mutable.
union upb_value upb_msg_get(upb_msg *msg, struct upb_fielddef *f);
union upb_value upb_msg_getmutable(upb_msg *msg, struct upb_fielddef *f);

// Sets the given field to the given value.  The msg will take a ref on val,
// and will drop a ref on whatever was there before.
void upb_msg_set(upb_msg *msg, struct upb_fielddef *f, union upb_value val);

void upb_msg_clear(upb_msg *msg, struct upb_msgdef *md);

void upb_msg_parsestr(upb_msg *msg, struct upb_msgdef *md, upb_string *data,
                      struct upb_status *status);

#endif
