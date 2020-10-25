// These are the specialized field parser functions for the fast parser.
// Generated tables will refer to these by name.
//
// Here we follow the same pattern of macros used in decode_fast.c to declare
// all of the variants.

#ifndef UPB_DECODE_FAST_H_
#define UPB_DECODE_FAST_H_

#include "upb/msg.h"

struct upb_decstate;

// The fallback, generic parsing function that can handle any field type.
// This just uses the regular (non-fast) parser to parse a single field.
const char *fastdecode_generic(struct upb_decstate *d, const char *ptr,
                               upb_msg *msg, const upb_msglayout *table,
                               uint64_t hasbits, uint64_t data);

#define UPB_PARSE_PARAMS                                 \
  struct upb_decstate *d, const char *ptr, upb_msg *msg, \
      const upb_msglayout *table, uint64_t hasbits, uint64_t data

/* primitive fields ***********************************************************/

#define F(card, type, valbytes, tagbytes) \
  const char *upb_p##card##type##valbytes##_##tagbytes##bt(UPB_PARSE_PARAMS);

#define TYPES(card, tagbytes) \
  F(card, b, 1, tagbytes)     \
  F(card, v, 4, tagbytes)     \
  F(card, v, 8, tagbytes)     \
  F(card, z, 4, tagbytes)     \
  F(card, z, 8, tagbytes)     \
  F(card, f, 4, tagbytes)     \
  F(card, f, 8, tagbytes)

#define TAGBYTES(card) \
  TYPES(card, 1)       \
  TYPES(card, 2)

TAGBYTES(s)
TAGBYTES(o)
TAGBYTES(r)
TAGBYTES(p)

#undef F
#undef TYPES
#undef TAGBYTES

/* string fields **************************************************************/

#define F(card, tagbytes)                                      \
  const char *upb_p##card##s_##tagbytes##bt(UPB_PARSE_PARAMS); \
  const char *upb_c##card##s_##tagbytes##bt(UPB_PARSE_PARAMS);

#define TAGBYTES(card) \
  F(card, 1)           \
  F(card, 2)

TAGBYTES(s)
TAGBYTES(o)
TAGBYTES(r)

#undef F
#undef TAGBYTES

/* sub-message fields *********************************************************/

#define F(card, tagbytes, size_ceil, ceil_arg) \
  const char *upb_p##card##m_##tagbytes##bt_max##size_ceil##b(UPB_PARSE_PARAMS);

#define SIZES(card, tagbytes) \
  F(card, tagbytes, 64, 64) \
  F(card, tagbytes, 128, 128) \
  F(card, tagbytes, 192, 192) \
  F(card, tagbytes, 256, 256) \
  F(card, tagbytes, max, -1)

#define TAGBYTES(card) \
  SIZES(card, 1) \
  SIZES(card, 2)

TAGBYTES(s)
TAGBYTES(o)
TAGBYTES(r)

#undef TAGBYTES
#undef SIZES
#undef F

#undef UPB_PARSE_PARAMS

#endif  /* UPB_DECODE_FAST_H_ */
