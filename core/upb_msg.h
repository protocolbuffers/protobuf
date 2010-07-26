/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 *
 * Data structure for storing a message of protobuf data.
 */

#ifndef UPB_MSG_H
#define UPB_MSG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  upb_atomic_refcount_t refcount;
  uint32_t len;
  uint32_t size;
  upb_valueptr elements;
};

upb_array *upb_array_new(void);

INLINE uint32_t upb_array_len(upb_array *a) {
  return a->len;
}

void _upb_array_free(upb_array *a, upb_fielddef *f);
INLINE void upb_array_unref(upb_array *a, upb_fielddef *f) {
  if (upb_atomic_unref(&a->refcount)) _upb_array_free(a, f);
}

INLINE upb_value upb_array_get(upb_array *a, upb_fielddef *f, uint32_t elem) {
  assert(elem < upb_array_len(a));
  return upb_value_read(_upb_array_getptr(a, f, elem), f->type);
}

// For string or submessages, will release a ref on the previously set value.
INLINE void upb_array_set(upb_array *a, upb_fielddef *f, uint32_t elem,
                          upb_value val) {
}

// Append an element with the default value, returning it.  For strings or
// submessages, this will try to reuse previously allocated memory.
INLINE upb_value upb_array_append_mutable(upb_array *a, upb_fielddef *f) {
}

typedef struct {
  upb_atomic_refcount_t refcount;
  uint8_t data[4];  // We allocate the appropriate amount per message.
} upb_msg;

// Creates a new msg of the given type.
upb_msg *upb_msg_new(upb_msgdef *md);

void _upb_msg_free(upb_msg *msg, upb_msgdef *md);
INLINE void upb_msg_unref(upb_msg *msg, upb_msgdef *md) {
  if (upb_atomic_unref(&msg->refcount)) _upb_msg_free(msg, md);
}

// Tests whether the given field is explicitly set, or whether it will return a
// default.
INLINE bool upb_msg_has(upb_msg *msg, upb_fielddef *f) {
  return (msg->data[f->field_index/8] & (1 << (f->field_index % 8))) != 0;
}

// Returns the current value of the given field if set, or the default value if
// not set.
INLINE upb_value upb_msg_get(upb_msg *msg, upb_fielddef *f) {
  if (upb_msg_has(msg, f)) {
    return upb_value_read(_upb_msg_getptr(msg, f), f->type);
  } else {
    return f->default_value;
  }
}

// If the given string, submessage, or array is already set, returns it.
// Otherwise sets it and returns an empty instance, attempting to reuse any
// previously allocated memory.
INLINE upb_value upb_msg_getmutable(upb_msg *msg, upb_fielddef *f) {
}

// Sets the current value of the field.  If this is a string, array, or
// submessage field, releases a ref on the value (if any) that was previously
// set.
INLINE void upb_msg_set(upb_msg *msg, upb_fielddef *f, upb_value val) {
}

// Unsets all field values back to their defaults.
INLINE void upb_msg_clear(upb_msg *msg, upb_msgdef *md) {
  memset(msg->data, 0, md->set_flags_bytes);
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
