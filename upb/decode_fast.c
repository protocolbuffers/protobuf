
#include "upb/decode.h"

#include "upb/port_def.inc"

#define UPB_PARSE_PARAMS                                                      \
  upb_decstate *d, const char *ptr, upb_msg *msg, const upb_msglayout *table, \
      uint64_t hasbits, uint64_t data

#define UPB_PARSE_ARGS d, ptr, msg, table, hasbits, data

typedef enum {
  CARD_s = 0,
  CARD_o = 1,
  CARD_r = 2,
  CARD_p = 3
} upb_card;

UPB_NOINLINE
const char *fastdecode_dispatch(upb_decstate *d, const char *ptr, upb_msg *msg,
                                const upb_msglayout *table, uint64_t hasbits) {
  uint16_t tag;
  uint64_t data = 0;;
  size_t idx;
  if (UPB_UNLIKELY(ptr >= d->fastlimit)) {
    return fastdecode_generic(UPB_PARSE_ARGS);
  }
  memcpy(&tag, ptr, 2);
  idx = (tag & 0xf8) >> 3;
  data = table->field_data[idx] ^ tag;
  return table->field_parser[idx](UPB_PARSE_ARGS);
}

UPB_FORCEINLINE
static bool fastdecode_checktag(uint64_t data, int tagbytes) {
  if (tagbytes == 1) {
    return (data & 0xff) == 0;
  } else {
    return (data & 0xffff) == 0;
  }
}

UPB_FORCEINLINE uint64_t fastdecode_munge(uint64_t val, int valbytes, bool zigzag) {
  if (valbytes == 1) {
    return val != 0;
  } else if (zigzag) {
    if (valbytes == 4) {
      uint32_t n = val;
      return (n >> 1) ^ -(int32_t)(n & 1);
    } else if (valbytes == 8) {
      return (val >> 1) ^ -(int64_t)(val & 1);
    }
    UPB_UNREACHABLE();
  }
  return val;
}

UPB_FORCEINLINE
static void *fastdecode_getfield_ofs(upb_msg *msg, size_t ofs, uint64_t *data,
                                     uint64_t *hasbits, upb_card card) {
  void *field = (char *)msg + ofs;

  switch (card) {
    case CARD_s:
      *hasbits |= *data;
      return field;
    case CARD_o: {
      uint32_t *case_ptr = UPB_PTR_AT(msg, (*data >> 16) & 0xffff, uint32_t);
      *case_ptr = (*data >> 32) & 0xffff;
      return field;
    }
    case CARD_r:
      UPB_ASSERT(false);  /* NYI */
    default:
      UPB_UNREACHABLE();
  }
}

UPB_FORCEINLINE
static void *fastdecode_getfield(upb_msg *msg, uint64_t *data,
                                 uint64_t *hasbits, upb_card card) {
  return fastdecode_getfield_ofs(msg, *data >> 48, data, hasbits, card);
}

/* varint fields **************************************************************/

UPB_FORCEINLINE
static const char *fastdecode_varint(UPB_PARSE_PARAMS, int tagbytes,
                                     int valbytes, upb_card card, bool zigzag) {
  uint64_t val = 0;
  void *dst;
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    return fastdecode_generic(UPB_PARSE_ARGS);;
  }
  dst = fastdecode_getfield(msg, &data, &hasbits, card);
  if (UPB_UNLIKELY(ptr[tagbytes] < 0)) {
    return fastdecode_generic(UPB_PARSE_ARGS);
  }
  val = fastdecode_munge(ptr[tagbytes], valbytes, zigzag);
  memcpy(dst, &val, valbytes);
  return fastdecode_dispatch(d, ptr + tagbytes + 1, msg, table, hasbits);
}

// Generate all varint functions.
// {s,o,r} x {b1,v4,z4,v8,z8} x {1bt,2bt}

#define z_ZZ true
#define b_ZZ false
#define v_ZZ false

#define F(card, type, valbytes, tagbytes)                                      \
  const char *upb_p##card##type##valbytes##_##tagbytes##bt(UPB_PARSE_PARAMS) { \
    return fastdecode_varint(UPB_PARSE_ARGS, tagbytes, valbytes, CARD_##card,  \
                             type##_ZZ);                                       \
  }

#define TYPES(card, tagbytes) \
  F(card, b, 1, tagbytes)     \
  F(card, v, 4, tagbytes)     \
  F(card, v, 8, tagbytes)     \
  F(card, z, 4, tagbytes)     \
  F(card, z, 8, tagbytes)

#define TAGBYTES(card) \
  TYPES(card, 1)       \
  TYPES(card, 2)

TAGBYTES(s)
TAGBYTES(o)
//TAGBYTES(r)

#undef z_ZZ
#undef b_ZZ
#undef v_ZZ
#undef o_ONEOF
#undef s_ONEOF
#undef r_ONEOF
#undef F
#undef TYPES
#undef TAGBYTES

/* string fields **************************************************************/

UPB_FORCEINLINE
bool fastdecode_boundscheck(const char *ptr, unsigned len, const char *end) {
  uintptr_t uptr = (uintptr_t)ptr;
  uintptr_t uend = (uintptr_t)end;
  uintptr_t res = uptr + len;
  return res < uptr || res > uend;
}

UPB_FORCEINLINE
static const char *fastdecode_string(UPB_PARSE_PARAMS, int tagbytes,
                                     upb_card card) {
  upb_strview *dst;
  int64_t len;
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    return fastdecode_generic(UPB_PARSE_ARGS);
  }

  dst = fastdecode_getfield(msg, &data, &hasbits, card);
  len = ptr[tagbytes];
  if (UPB_UNLIKELY(len < 0)) {
    return fastdecode_generic(UPB_PARSE_ARGS);
  }
  ptr += tagbytes + 1;
  dst->data = ptr;
  dst->size = len;
  if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, len, d->limit))) {
    return fastdecode_err(d);
  }
  ptr += len;
  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
}

const char *upb_pss_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 1, CARD_s);
}

const char *upb_pos_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 1, CARD_o);
}

const char *upb_pss_2bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 2, CARD_s);
}

const char *upb_pos_2bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 2, CARD_o);
}

/* message fields *************************************************************/

UPB_FORCEINLINE
static const char *fastdecode_submsg(UPB_PARSE_PARAMS, int tagbytes,
                                     upb_card card) {
  const char *saved_limit;
  const upb_msglayout_field *field = &table->fields[data >> 48];
  size_t ofs = field->offset;
  const upb_msglayout *subl = table->submsgs[field->submsg_index];
  upb_msg **submsg;
  int64_t len;

  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    return fastdecode_generic(UPB_PARSE_ARGS);
  }

  submsg = fastdecode_getfield_ofs(msg, ofs, &data, &hasbits, card);
  len = ptr[tagbytes];
  if (UPB_UNLIKELY(len < 0)) {
    return fastdecode_generic(UPB_PARSE_ARGS);
  }
  ptr += tagbytes + 1;
  if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, len, d->limit))) {
    return fastdecode_err(d);
  }
  if (!*submsg) {
    *submsg = decode_newmsg(d, subl);
  }

  saved_limit = d->limit;
  if (--d->depth < 0) return fastdecode_err(d);
  d->limit = ptr + len;
  d->fastlimit = UPB_MIN(d->limit, d->fastend);

  ptr = fastdecode_dispatch(d, ptr, *submsg, subl, 0);
  if (ptr != d->limit) return fastdecode_err(d);

  d->limit = saved_limit;
  d->fastlimit = UPB_MIN(d->limit, d->fastend);
  if (d->end_group != 0) return fastdecode_err(d);
  d->depth++;

  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
}

const char *upb_psm_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_submsg(UPB_PARSE_ARGS, 1, CARD_s);
}

const char *upb_pom_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_submsg(UPB_PARSE_ARGS, 1, CARD_o);
}

const char *upb_psm_2bt(UPB_PARSE_PARAMS) {
  return fastdecode_submsg(UPB_PARSE_ARGS, 2, CARD_s);
}

const char *upb_pom_2bt(UPB_PARSE_PARAMS) {
  return fastdecode_submsg(UPB_PARSE_ARGS, 2, CARD_o);
}
