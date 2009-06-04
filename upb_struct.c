/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Definitions that will emit code for inline functions, per C99 inlining
 * rules (see http://www.greenend.org.uk/rjk/2003/03/inline.html).
 */

#include "upb_struct.h"

#define UPB_DECLARE_ACCESSORS(ctype, name) \
  extern ctype *upb_struct_get_ ## name ## _ptr( \
      uint8_t *s, struct upb_struct_field *f); \
  extern ctype upb_struct_get_ ## name( \
      uint8_t *s, struct upb_struct_field *f); \
  extern void upb_struct_set_ ## name( \
      uint8_t *s, struct upb_struct_field *f, ctype val);

#define UPB_DECLARE_ARRAY_ACCESSORS(ctype, name) \
  extern ctype *upb_array_get_ ## name ## _ptr(struct upb_array *a, int n); \
  extern ctype upb_array_get_ ## name(struct upb_array *a, int n); \
  extern void upb_array_set_ ## name(struct upb_array *a, int n, ctype val);

#define UPB_DECLARE_ALL_ACCESSORS(ctype, name) \
  UPB_DECLARE_ACCESSORS(ctype, name) \
  UPB_DECLARE_ARRAY_ACCESSORS(ctype, name)

UPB_DECLARE_ALL_ACCESSORS(double,   double)
UPB_DECLARE_ALL_ACCESSORS(float,    float)
UPB_DECLARE_ALL_ACCESSORS(int32_t,  int32)
UPB_DECLARE_ALL_ACCESSORS(int64_t,  int64)
UPB_DECLARE_ALL_ACCESSORS(uint32_t, uint32)
UPB_DECLARE_ALL_ACCESSORS(uint64_t, uint64)
UPB_DECLARE_ALL_ACCESSORS(bool,     bool)
UPB_DECLARE_ALL_ACCESSORS(struct upb_struct_delimited*, bytes)
UPB_DECLARE_ALL_ACCESSORS(struct upb_struct_delimited*, string)
UPB_DECLARE_ALL_ACCESSORS(uint8_t*, substruct)
UPB_DECLARE_ACCESSORS(struct upb_array*, array)

extern void upb_struct_set(uint8_t *s, struct upb_struct_field *f);
extern void upb_struct_unset(uint8_t *s, struct upb_struct_field *f);
extern bool upb_struct_is_set(uint8_t *s, struct upb_struct_field *f);
extern bool upb_struct_all_required_fields_set(
    uint8_t *s, struct upb_struct_definition *d);
extern void upb_struct_clear(uint8_t *s, struct upb_struct_definition *d);

