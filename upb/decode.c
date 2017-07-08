
#include "upb/decode.h"

typedef enum {
  UPB_WIRE_TYPE_VARINT      = 0,
  UPB_WIRE_TYPE_64BIT       = 1,
  UPB_WIRE_TYPE_DELIMITED   = 2,
  UPB_WIRE_TYPE_START_GROUP = 3,
  UPB_WIRE_TYPE_END_GROUP   = 4,
  UPB_WIRE_TYPE_32BIT       = 5
} upb_wiretype_t;

static void upb_decode_seterr(upb_env *env, const char *msg) {
  upb_status status = UPB_STATUS_INIT;
  upb_status_seterrmsg(&status, msg);
  upb_env_reporterror(env, &status);
}

static bool upb_decode_varint(const char **ptr, const char *limit,
                              uint64_t *val) {
  uint8_t byte = 0x80;
  int bitpos = 0;
  const char *p = *ptr;
  *val = 0;

  while (byte & 0x80) {
    if (bitpos == 70 || p == limit) {
      return false;
    }

    byte = *p;
    *val |= (uint64_t)(byte & 0x7F) << bitpos;
    p++;
    bitpos += 7;
  }

  *ptr = p;
  return true;
}

static bool upb_decode_varint32(const char **ptr, const char *limit,
                                uint32_t *val) {
  uint64_t u64;
  if (!upb_decode_varint(ptr, limit, &u64) || u64 > UINT32_MAX) {
    return false;
  } else {
    *val = u64;
    return true;
  }
}

static const upb_msglayout_fieldinit_v1 *upb_find_field(
    const upb_msglayout_msginit_v1 *l, uint32_t field_number) {
  /* Lots of optimization opportunities here. */
  int i;
  for (i = 0; i < l->field_count; i++) {
    if (l->fields[i].number == field_number) {
      return &l->fields[i];
    }
  }

  return NULL;  /* Unknown field. */
}

static bool upb_decode_64bit(const char **ptr, const char *limit,
                             uint64_t *val) {
  if (limit - *ptr < 8) {
    return false;
  } else {
    memcpy(val, *ptr, 8);
    *ptr += 8;
    return true;
  }
}

static bool upb_decode_32bit(const char **ptr, const char *limit,
                             uint32_t *val) {
  if (limit - *ptr < 4) {
    return false;
  } else {
    memcpy(val, *ptr, 4);
    *ptr += 4;
    return true;
  }
}

static int32_t upb_zzdec_32(uint32_t n) {
  return (n >> 1) ^ -(int32_t)(n & 1);
}

static int64_t upb_zzdec_64(uint64_t n) {
  return (n >> 1) ^ -(int64_t)(n & 1);
}

static bool upb_decode_string(const char **ptr, const char *limit,
                              upb_stringview *val) {
  uint32_t len;

  if (!upb_decode_varint32(ptr, limit, &len) ||
      limit - *ptr < len) {
    return false;
  }

  *val = upb_stringview_make(*ptr, len);
  *ptr += len;
  return true;
}

static void upb_set32(void *msg, size_t ofs, uint32_t val) {
  memcpy((char*)msg + ofs, &val, sizeof(val));
}

bool upb_append_unknown(const char **ptr, const char *start, const char *limit,
                        char *msg) {
  UPB_UNUSED(limit);
  UPB_UNUSED(msg);
  *ptr = limit;
  return true;
}

bool upb_decode_field(const char **ptr, const char *limit, char *msg,
                      const upb_msglayout_msginit_v1 *l, upb_env *env) {
  uint32_t tag;
  uint32_t wire_type;
  uint32_t field_number;
  const char *p = *ptr;
  const char *field_start = p;
  const upb_msglayout_fieldinit_v1 *f;

  if (!upb_decode_varint32(&p, limit, &tag)) {
    upb_decode_seterr(env, "Error decoding tag.\n");
    return false;
  }

  wire_type = tag & 0x7;
  field_number = tag >> 3;

  if (field_number == 0) {
    return false;
  }

  f = upb_find_field(l, field_number);

  switch (wire_type) {
    case UPB_WIRE_TYPE_VARINT: {
      uint64_t val;
      if (!upb_decode_varint(&p, limit, &val)) {
        upb_decode_seterr(env, "Error decoding varint value.\n");
        return false;
      }

      if (!f) {
        return upb_append_unknown(ptr, field_start, p, msg);
      }

      switch (f->type) {
        case UPB_DESCRIPTOR_TYPE_INT64:
        case UPB_DESCRIPTOR_TYPE_UINT64:
          memcpy(msg + f->offset, &val, sizeof(val));
          break;
        case UPB_DESCRIPTOR_TYPE_INT32:
        case UPB_DESCRIPTOR_TYPE_UINT32:
        case UPB_DESCRIPTOR_TYPE_ENUM: {
          uint32_t val32 = val;
          memcpy(msg + f->offset, &val32, sizeof(val32));
          break;
        }
        case UPB_DESCRIPTOR_TYPE_SINT32: {
          int32_t decoded = upb_zzdec_32(val);
          memcpy(msg + f->offset, &decoded, sizeof(decoded));
          break;
        }
        case UPB_DESCRIPTOR_TYPE_SINT64: {
          int64_t decoded = upb_zzdec_64(val);
          memcpy(msg + f->offset, &decoded, sizeof(decoded));
          break;
        }
        default:
          return upb_append_unknown(ptr, field_start, p, msg);
      }

      break;
    }
    case UPB_WIRE_TYPE_64BIT: {
      uint64_t val;
      if (!upb_decode_64bit(&p, limit, &val)) {
        upb_decode_seterr(env, "Error decoding 64bit value.\n");
        return false;
      }

      if (!f) {
        return upb_append_unknown(ptr, field_start, p, msg);
      }

      break;
    }
    case UPB_WIRE_TYPE_32BIT: {
      uint32_t val;
      if (!upb_decode_32bit(&p, limit, &val)) {
        upb_decode_seterr(env, "Error decoding 32bit value.\n");
        return false;
      }

      if (!f) {
        return upb_append_unknown(ptr, field_start, p, msg);
      }

      break;
    }
    case UPB_WIRE_TYPE_DELIMITED: {
      upb_stringview val;
      if (!upb_decode_string(&p, limit, &val)) {
        upb_decode_seterr(env, "Error decoding delimited value.\n");
        return false;
      }

      if (!f) {
        return upb_append_unknown(ptr, field_start, p, msg);
      }

      memcpy(msg + f->offset, &val, sizeof(val));
      break;
    }
  }

  if (f->oneof_index != UPB_NOT_IN_ONEOF) {
    upb_set32(msg, l->oneofs[f->oneof_index].case_offset, f->number);
  }

  *ptr = p;
  return true;
}

bool upb_decode(upb_stringview buf, void *msg_void,
                const upb_msglayout_msginit_v1 *l, upb_env *env) {
  char *msg = msg_void;
  const char *ptr = buf.data;
  const char *limit = ptr + buf.size;

  while (ptr < limit) {
    if (!upb_decode_field(&ptr, limit, msg, l, env)) {
      return false;
    }
  }

  return true;
}
