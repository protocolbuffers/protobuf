/*
** Our memory representation for parsing tables and messages themselves.
** Functions in this file are used by generated code and possible reflection.
**
** The definitions in this file are internal to upb.
**/

#ifndef UPB_MSG_H_
#define UPB_MSG_H_

#include <stdint.h>
#include <string.h>

#include "upb/table.int.h"
#include "upb/upb.h"

#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef void upb_msg;

/** upb_msglayout *************************************************************/

/* upb_msglayout represents the memory layout of a given upb_msgdef.  The
 * members are public so generated code can initialize them, but users MUST NOT
 * read or write any of its members. */

typedef struct {
  uint32_t number;
  uint16_t offset;
  int16_t presence;      /* If >0, hasbit_index+1.  If <0, oneof_index+1. */
  uint16_t submsg_index;  /* undefined if descriptortype != MESSAGE or GROUP. */
  uint8_t descriptortype;
  uint8_t label;
} upb_msglayout_field;

typedef struct upb_msglayout {
  const struct upb_msglayout *const* submsgs;
  const upb_msglayout_field *fields;
  /* Must be aligned to sizeof(void*).  Doesn't include internal members like
   * unknown fields, extension dict, pointer to msglayout, etc. */
  uint16_t size;
  uint16_t field_count;
  bool extendable;
} upb_msglayout;

/** upb_msg *******************************************************************/

/* Representation is in msg.c for now. */

/* Maps upb_fieldtype_t -> memory size. */
extern char _upb_fieldtype_to_size[12];

/* Creates a new messages with the given layout on the given arena. */
upb_msg *_upb_msg_new(const upb_msglayout *l, upb_arena *a);

/* Adds unknown data (serialized protobuf data) to the given message.  The data
 * is copied into the message instance. */
void upb_msg_addunknown(upb_msg *msg, const char *data, size_t len,
                        upb_arena *arena);

/* Returns a reference to the message's unknown data. */
const char *upb_msg_getunknown(const upb_msg *msg, size_t *len);

/** upb_array *****************************************************************/

/* Our internal representation for repeated fields.  */
typedef struct {
  uintptr_t data;   /* Tagged ptr: low 2 bits of ptr are lg2(elem size). */
  size_t len;   /* Measured in elements. */
  size_t size;  /* Measured in elements. */
} upb_array;

UPB_INLINE const void *_upb_array_constptr(const upb_array *arr) {
  return (void*)((uintptr_t)arr->data & ~7UL);
}

UPB_INLINE void *_upb_array_ptr(upb_array *arr) {
  return (void*)_upb_array_constptr(arr);
}

/* Creates a new array on the given arena. */
upb_array *upb_array_new(upb_arena *a, upb_fieldtype_t type);

/* Resizes the capacity of the array to be at least min_size. */
bool _upb_array_realloc(upb_array *arr, size_t min_size, upb_arena *arena);

/* Fallback functions for when the accessors require a resize. */
void *_upb_array_resize_fallback(upb_array **arr_ptr, size_t size,
                                 upb_fieldtype_t type, upb_arena *arena);
bool _upb_array_append_fallback(upb_array **arr_ptr, const void *value,
                                upb_fieldtype_t type, upb_arena *arena);

/** upb_map *******************************************************************/

/* Right now we use strmaps for everything.  We'll likely want to use
 * integer-specific maps for integer-keyed maps.*/
typedef struct {
  /* We should pack these better and move them into table to avoid padding. */
  char key_size_lg2;
  char val_size_lg2;

  upb_strtable table;
} upb_map;

/* Creates a new map on the given arena with this key/value type. */
upb_map *upb_map_new(upb_arena *a, upb_fieldtype_t key_type,
                     upb_fieldtype_t value_type);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif /* UPB_MSG_H_ */
