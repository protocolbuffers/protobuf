/*
** structs.int.h: structures definitions that are internal to upb.
*/

#ifndef UPB_STRUCTS_H_
#define UPB_STRUCTS_H_

#include "upb/upb.h"

struct upb_array {
  upb_fieldtype_t type;
  uint8_t element_size;
  void *data;   /* Each element is element_size. */
  size_t len;   /* Measured in elements. */
  size_t size;  /* Measured in elements. */
  upb_arena *arena;
};

#endif  /* UPB_STRUCTS_H_ */

