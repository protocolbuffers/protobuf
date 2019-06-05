/*
** Data structures for message tables, used for parsing and serialization.
** This are much lighter-weight than full reflection, but they are do not
** have enough information to convert to text format, JSON, etc.
**
** The definitions in this file are internal to upb.
**/

#ifndef UPB_MSG_H_
#define UPB_MSG_H_

#include <stdint.h>
#include <string.h>
#include "upb/upb.h"

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

/** Message internal representation *******************************************/

/* Our internal representation for repeated fields. */
typedef struct {
  void *data;   /* Each element is element_size. */
  size_t len;   /* Measured in elements. */
  size_t size;  /* Measured in elements. */
} upb_array;

upb_msg *upb_msg_new(const upb_msglayout *l, upb_arena *a);
upb_msg *upb_msg_new(const upb_msglayout *l, upb_arena *a);

void upb_msg_addunknown(upb_msg *msg, const char *data, size_t len,
                        upb_arena *arena);
const char *upb_msg_getunknown(const upb_msg *msg, size_t *len);

upb_array *upb_array_new(upb_arena *a);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* UPB_MSG_H_ */
