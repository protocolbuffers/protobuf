/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 *
 * This file defines a simple string type, which has several important features:
 *
 * - strings are reference-counted.
 * - strings are logically immutable.
 * - ...however, if a string has no other referents, it can be "recycled"
 *   into a new string without having to free/malloc.
 * - strings can be substrings of other strings (owning a ref on the source string).
 * - strings can refer to un-owned memory; attempting to acquire a reference will
 *   copy the data at that time.
 */

typedef struct _upb_string {
  char *ptr;
  uint32_t byte_len;
  uint32_t byte_size;
  upb_atomic_refcount_t refcount;
  struct _upb_string *src;
} upb_string;

INLINE upb_strlen_t upb_string_bytelen(upb_string *str) { return str->byte_len; }
INLINE const char *upb_string_getrobuf(upb_string *str) { return str->ptr; }

upb_string *upb_string_getref(upb_string *str);
void upb_string_unref(upb_string *str);
upb_string *upb_string_tryrecycle(upb_string *str);

// The three options for creating a string.
char *upb_string_getrwbuf(upb_string *str, upb_strlen_t byte_len);
void upb_string_substr(upb_string *str, upb_string *target_str,
                       upb_strlen_t start, upb_strlen_t byte_len);
void upb_string_refexternal(upb_string *str, char *ptr, upb_strlen_t len);

