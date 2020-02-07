
#include <string.h>
#include "upb/upb.h"
#include "upb/decode.h"

#include "upb/port_def.inc"

/* Maps descriptor type -> upb field type.  */
static const uint8_t desctype_to_fieldtype[] = {
  -1,  /* invalid descriptor type */
  UPB_TYPE_DOUBLE,          /* DOUBLE */
  UPB_TYPE_FLOAT,           /* FLOAT */
  UPB_TYPE_INT64,           /* INT64 */
  UPB_TYPE_UINT64,          /* UINT64 */
  UPB_TYPE_INT32,           /* INT32 */
  UPB_TYPE_UINT64,          /* FIXED64 */
  UPB_TYPE_UINT32,          /* FIXED32 */
  UPB_TYPE_BOOL,            /* BOOL */
  UPB_TYPE_STRING,          /* STRING */
  UPB_TYPE_MESSAGE,         /* GROUP */
  UPB_TYPE_MESSAGE,         /* MESSAGE */
  UPB_TYPE_BYTES,           /* BYTES */
  UPB_TYPE_UINT32,          /* UINT32 */
  UPB_TYPE_ENUM,            /* ENUM */
  UPB_TYPE_INT32,           /* SFIXED32 */
  UPB_TYPE_INT64,           /* SFIXED64 */
  UPB_TYPE_INT32,           /* SINT32 */
  UPB_TYPE_INT64,           /* SINT64 */
};

/* Maps descriptor type -> upb map size.  */
static const uint8_t desctype_to_mapsize[] = {
  -1,  /* invalid descriptor type */
  8,          /* DOUBLE */
  4,          /* FLOAT */
  8,          /* INT64 */
  8,          /* UINT64 */
  4,          /* INT32 */
  8,          /* FIXED64 */
  4,          /* FIXED32 */
  1,          /* BOOL */
  UPB_MAPTYPE_STRING,    /* STRING */
  sizeof(void*),         /* GROUP */
  sizeof(void*),         /* MESSAGE */
  UPB_MAPTYPE_STRING,    /* BYTES */
  4,          /* UINT32 */
  4,          /* ENUM */
  4,          /* SFIXED32 */
  8,          /* SFIXED64 */
  4,          /* SINT32 */
  8,          /* SINT64 */
};

/* Data pertaining to the parse. */
typedef struct {
  const char *field_start;   /* Start of this field. */
  const char *limit;         /* End of delimited region or end of buffer. */
  upb_arena *arena;
  int depth;
  uint32_t end_group;  /* Set to field number of END_GROUP tag, if any. */
} upb_decstate;

#define CHK(x) if (!(x)) { return 0; }
#define PTR_AT(msg, ofs, type) (type*)((const char*)msg + ofs)

static const char *upb_decode_message(const char *ptr, const upb_msglayout *l,
                                      upb_msg *msg, upb_decstate *d);

static const char *upb_decode_varint(const char *ptr, const char *limit,
                                     uint64_t *val) {
  uint8_t byte;
  int bitpos = 0;
  *val = 0;

  do {
    CHK(bitpos < 70 && ptr < limit);
    byte = *ptr;
    *val |= (uint64_t)(byte & 0x7F) << bitpos;
    ptr++;
    bitpos += 7;
  } while (byte & 0x80);

  return ptr;
}

static const char *upb_decode_varint32(const char *ptr, const char *limit,
                                       uint32_t *val) {
  uint64_t u64;
  CHK(ptr = upb_decode_varint(ptr, limit, &u64))
  CHK(u64 <= UINT32_MAX);
  *val = (uint32_t)u64;
  return ptr;
}

static const char *upb_decode_64bit(const char *ptr, const char *limit,
                                    uint64_t *val) {
  CHK(limit - ptr >= 8);
  memcpy(val, ptr, 8);
  return ptr + 8;
}

static const char *upb_decode_32bit(const char *ptr, const char *limit,
                                    uint32_t *val) {
  CHK(limit - ptr >= 4);
  memcpy(val, ptr, 4);
  return ptr + 4;
}

static int32_t upb_zzdecode_32(uint32_t n) {
  return (n >> 1) ^ -(int32_t)(n & 1);
}

static int64_t upb_zzdecode_64(uint64_t n) {
  return (n >> 1) ^ -(int64_t)(n & 1);
}

static const char *upb_decode_string(const char *ptr, const char *limit,
                                     int *outlen) {
  uint32_t len;

  CHK(ptr = upb_decode_varint32(ptr, limit, &len));
  CHK(len < INT32_MAX);
  CHK(limit - ptr >= (int32_t)len);

  *outlen = len;
  return ptr;
}

static void upb_set32(void *msg, size_t ofs, uint32_t val) {
  memcpy((char*)msg + ofs, &val, sizeof(val));
}

static const char *upb_append_unknown(const char *ptr, upb_msg *msg,
                                      upb_decstate *d) {
  upb_msg_addunknown(msg, d->field_start, ptr - d->field_start, d->arena);
  return ptr;
}

static const char *upb_skip_unknownfielddata(const char *ptr, upb_decstate *d,
                                             uint32_t tag) {
  switch (tag & 7) {
    case UPB_WIRE_TYPE_VARINT: {
      uint64_t val;
      return upb_decode_varint(ptr, d->limit, &val);
    }
    case UPB_WIRE_TYPE_32BIT: {
      uint32_t val;
      return upb_decode_32bit(ptr, d->limit, &val);
    }
    case UPB_WIRE_TYPE_64BIT: {
      uint64_t val;
      return upb_decode_64bit(ptr, d->limit, &val);
    }
    case UPB_WIRE_TYPE_DELIMITED: {
      int len;
      CHK(ptr = upb_decode_string(ptr, d->limit, &len));
      return ptr + len;
    }
    case UPB_WIRE_TYPE_START_GROUP: {
      uint32_t field_number = tag >> 3;
      while (ptr < d->limit && d->end_group == 0) {
        uint32_t tag = 0;
        CHK(ptr = upb_decode_varint32(ptr, d->limit, &tag));
        CHK(ptr = upb_skip_unknownfielddata(ptr, d, tag));
      }
      CHK(d->end_group == field_number);
      d->end_group = 0;
      return ptr;
    }
    case UPB_WIRE_TYPE_END_GROUP:
      d->end_group = tag >> 3;
      return ptr;
  }
  return false;
}

static void *upb_array_reserve(upb_array *arr, size_t elements,
                               size_t elem_size, upb_arena *arena) {
  if (arr->size - arr->len < elements) {
    CHK(_upb_array_realloc(arr, arr->len + elements, arena));
  }
  return (char*)_upb_array_ptr(arr) + (arr->len * elem_size);
}

bool upb_array_add(upb_array *arr, size_t elements, size_t elem_size,
                   const void *data, upb_arena *arena) {
  void *dest = upb_array_reserve(arr, elements, elem_size, arena);

  CHK(dest);
  arr->len += elements;
  memcpy(dest, data, elements * elem_size);

  return true;
}

static upb_array *upb_getarr(upb_msg *msg, const upb_msglayout_field *field) {
  UPB_ASSERT(field->label == UPB_LABEL_REPEATED);
  return *PTR_AT(msg, field->offset, upb_array*);
}

static upb_array *upb_getorcreatearr(upb_msg *msg,
                                     const upb_msglayout_field *field,
                                     upb_decstate *d) {
  upb_array *arr = upb_getarr(msg, field);

  if (!arr) {
    upb_fieldtype_t type = desctype_to_fieldtype[field->descriptortype];
    arr = _upb_array_new(d->arena, type);
    CHK(arr);
    *PTR_AT(msg, field->offset, upb_array*) = arr;
  }

  return arr;
}

static upb_msg *upb_getorcreatemsg(upb_msg *msg,
                                   const upb_msglayout_field *field,
                                   const upb_msglayout *layout,
                                   upb_decstate *d) {
  upb_msg **submsg = PTR_AT(msg, field->offset, upb_msg*);

  UPB_ASSERT(field->label != UPB_LABEL_REPEATED);

  if (!*submsg) {
    *submsg = _upb_msg_new(layout, d->arena);
    CHK(*submsg);
  }

  return *submsg;
}

static upb_msg *upb_addmsg(upb_msg *msg,
                           const upb_msglayout_field *field,
                           const upb_msglayout *layout,
                           upb_decstate *d) {
  upb_msg *submsg;
  upb_array *arr = upb_getorcreatearr(msg, field, d);

  UPB_ASSERT(field->label == UPB_LABEL_REPEATED);
  UPB_ASSERT(field->descriptortype == UPB_DESCRIPTOR_TYPE_MESSAGE ||
             field->descriptortype == UPB_DESCRIPTOR_TYPE_GROUP);

  submsg = _upb_msg_new(layout, d->arena);
  CHK(submsg);
  upb_array_add(arr, 1, sizeof(submsg), &submsg, d->arena);

  return submsg;
}

static void upb_sethasbit(upb_msg *msg, const upb_msglayout_field *field) {
  int32_t hasbit = field->presence;
  UPB_ASSERT(field->presence > 0);
  *PTR_AT(msg, hasbit / 8, char) |= (1 << (hasbit % 8));
}

static void upb_setoneofcase(upb_msg *msg, const upb_msglayout_field *field) {
  UPB_ASSERT(field->presence < 0);
  upb_set32(msg, ~field->presence, field->number);
}

static bool upb_decode_addval(upb_msg *msg, const upb_msglayout_field *field,
                              void *val, size_t size, upb_decstate *d) {
  char *field_mem = PTR_AT(msg, field->offset, char);
  upb_array *arr;

  if (field->label == UPB_LABEL_REPEATED) {
    arr = upb_getorcreatearr(msg, field, d);
    CHK(arr);
    field_mem = upb_array_reserve(arr, 1, size, d->arena);
    CHK(field_mem);
  }

  memcpy(field_mem, val, size);
  return true;
}

static void upb_decode_setpresent(upb_msg *msg,
                                  const upb_msglayout_field *field) {
  if (field->label == UPB_LABEL_REPEATED) {
   upb_array *arr = upb_getarr(msg, field);
   UPB_ASSERT(arr->len < arr->size);
   arr->len++;
  } else if (field->presence < 0) {
    upb_setoneofcase(msg, field);
  } else if (field->presence > 0) {
    upb_sethasbit(msg, field);
  }
}

static const char *upb_decode_msgfield(const char *ptr,
                                       const upb_msglayout *layout, int limit,
                                       upb_msg *msg, upb_decstate *d) {
  const char* saved_limit = d->limit;
  d->limit = ptr + limit;
  CHK(--d->depth >= 0);
  ptr = upb_decode_message(ptr, layout, msg, d);
  d->depth++;
  d->limit = saved_limit;
  CHK(d->end_group == 0);
  return ptr;
}

static const char *upb_decode_groupfield(const char *ptr,
                                         const upb_msglayout *layout,
                                         int field_number, upb_msg *msg,
                                         upb_decstate *d) {
  CHK(--d->depth >= 0);
  ptr = upb_decode_message(ptr, layout, msg, d);
  d->depth++;
  CHK(d->end_group == field_number);
  d->end_group = 0;
  return ptr;
}

static const char *upb_decode_varintfield(const char *ptr, upb_msg *msg,
                                          const upb_msglayout_field *field,
                                          upb_decstate *d) {
  uint64_t val;
  CHK(ptr = upb_decode_varint(ptr, d->limit, &val));

  switch (field->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_UINT64:
      CHK(upb_decode_addval(msg, field, &val, sizeof(val), d));
      break;
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_UINT32:
    case UPB_DESCRIPTOR_TYPE_ENUM: {
      uint32_t val32 = (uint32_t)val;
      CHK(upb_decode_addval(msg, field, &val32, sizeof(val32), d));
      break;
    }
    case UPB_DESCRIPTOR_TYPE_BOOL: {
      bool valbool = val != 0;
      CHK(upb_decode_addval(msg, field, &valbool, sizeof(valbool), d));
      break;
    }
    case UPB_DESCRIPTOR_TYPE_SINT32: {
      int32_t decoded = upb_zzdecode_32((uint32_t)val);
      CHK(upb_decode_addval(msg, field, &decoded, sizeof(decoded), d));
      break;
    }
    case UPB_DESCRIPTOR_TYPE_SINT64: {
      int64_t decoded = upb_zzdecode_64(val);
      CHK(upb_decode_addval(msg, field, &decoded, sizeof(decoded), d));
      break;
    }
    default:
      return upb_append_unknown(ptr, msg, d);
  }

  upb_decode_setpresent(msg, field);
  return ptr;
}

static const char *upb_decode_64bitfield(const char *ptr,
                                         const upb_msglayout_field *field,
                                         upb_msg *msg, upb_decstate *d) {
  uint64_t val;
  CHK(ptr = upb_decode_64bit(ptr, d->limit, &val));

  switch (field->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
      CHK(upb_decode_addval(msg, field, &val, sizeof(val), d));
      break;
    default:
      return upb_append_unknown(ptr, msg, d);
  }

  upb_decode_setpresent(msg, field);
  return ptr;
}

static const char *upb_decode_32bitfield(const char *ptr,
                                         const upb_msglayout_field *field,
                                         upb_msg *msg, upb_decstate *d) {
  uint32_t val;
  CHK(ptr = upb_decode_32bit(ptr, d->limit, &val));

  switch (field->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_FLOAT:
    case UPB_DESCRIPTOR_TYPE_FIXED32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
      CHK(upb_decode_addval(msg, field, &val, sizeof(val), d));
      break;
    default:
      return upb_append_unknown(ptr, msg, d);
  }

  upb_decode_setpresent(msg, field);
  return ptr;
}

static const char *upb_decode_fixedpacked(const char *ptr, upb_decstate *d,
                                          upb_array *arr, uint32_t len,
                                          int elem_size) {
  size_t elements = len / elem_size;

  CHK((size_t)(elements * elem_size) == len);
  CHK(upb_array_add(arr, elements, elem_size, ptr, d->arena));
  return ptr + len;
}

static const char *upb_decode_strfield(const char *ptr, upb_decstate *d,
                                       uint32_t len, upb_strview *str) {
  str->data = ptr;
  str->size = len;
  return ptr + len;
}

static const char *upb_decode_toarray(const char *ptr,
                                      const upb_msglayout *layout,
                                      const upb_msglayout_field *field, int len,
                                      upb_msg *msg, upb_decstate *d) {
  upb_array *arr = upb_getorcreatearr(msg, field, d);
  CHK(arr);

#define VARINT_CASE(ctype, decode) \
  VARINT_CASE_EX(ctype, decode, decode)

#define VARINT_CASE_EX(ctype, decode, dtype)                           \
  {                                                                    \
    const char *limit = ptr + len;                                     \
    while (ptr < limit) {                                              \
      uint64_t val;                                                    \
      ctype decoded;                                                   \
      CHK(ptr = upb_decode_varint(ptr, limit, &val));                  \
      decoded = (decode)((dtype)val);                                  \
      CHK(upb_array_add(arr, 1, sizeof(decoded), &decoded, d->arena)); \
    }                                                                  \
    return ptr;                                                        \
  }

  switch (field->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_STRING:
    case UPB_DESCRIPTOR_TYPE_BYTES: {
      upb_strview str;
      ptr = upb_decode_strfield(ptr, d, len, &str);
      CHK(upb_array_add(arr, 1, sizeof(str), &str, d->arena));
      return ptr;
    }
    case UPB_DESCRIPTOR_TYPE_FLOAT:
    case UPB_DESCRIPTOR_TYPE_FIXED32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
      return upb_decode_fixedpacked(ptr, d, arr, len, sizeof(int32_t));
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
      return upb_decode_fixedpacked(ptr, d, arr, len, sizeof(int64_t));
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_UINT32:
    case UPB_DESCRIPTOR_TYPE_ENUM:
      VARINT_CASE(uint32_t, uint32_t);
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_UINT64:
      VARINT_CASE(uint64_t, uint64_t);
    case UPB_DESCRIPTOR_TYPE_BOOL:
      VARINT_CASE(bool, bool);
    case UPB_DESCRIPTOR_TYPE_SINT32:
      VARINT_CASE_EX(int32_t, upb_zzdecode_32, uint32_t);
    case UPB_DESCRIPTOR_TYPE_SINT64:
      VARINT_CASE_EX(int64_t, upb_zzdecode_64, uint64_t);
    case UPB_DESCRIPTOR_TYPE_MESSAGE: {
      const upb_msglayout *subl = layout->submsgs[field->submsg_index];
      upb_msg *submsg = upb_addmsg(msg, field, subl, d);
      CHK(submsg);
      return upb_decode_msgfield(ptr, subl, len, submsg, d);
    }
    case UPB_DESCRIPTOR_TYPE_GROUP:
      return upb_append_unknown(ptr, msg, d);
  }
#undef VARINT_CASE
  UPB_UNREACHABLE();
}

static const char *upb_decode_mapfield(const char *ptr,
                                       const upb_msglayout *layout,
                                       const upb_msglayout_field *field,
                                       int len, upb_msg *msg, upb_decstate *d) {
  upb_map *map = *PTR_AT(msg, field->offset, upb_map*);
  const upb_msglayout *entry = layout->submsgs[field->submsg_index];
  upb_map_entry ent;

  if (!map) {
    /* Lazily create map. */
    const upb_msglayout_field *key_field = &entry->fields[0];
    const upb_msglayout_field *val_field = &entry->fields[1];
    char key_size = desctype_to_mapsize[key_field->descriptortype];
    char val_size = desctype_to_mapsize[val_field->descriptortype];
    UPB_ASSERT(key_field->number == 1);
    UPB_ASSERT(val_field->number == 2);
    UPB_ASSERT(key_field->offset == 0);
    UPB_ASSERT(val_field->offset == sizeof(upb_strview));
    map = _upb_map_new(d->arena, key_size, val_size);
    *PTR_AT(msg, field->offset, upb_map*) = map;
  }

  /* Parse map entry. */
  memset(&ent, 0, sizeof(ent));
  CHK(ptr = upb_decode_msgfield(ptr, entry, len, &ent.k, d));

  /* Insert into map. */
  _upb_map_set(map, &ent.k, map->key_size, &ent.v, map->val_size, d->arena);
  return ptr;
}

static const char *upb_decode_delimitedfield(const char *ptr,
                                             const upb_msglayout *layout,
                                             const upb_msglayout_field *field,
                                             upb_msg *msg, upb_decstate *d) {
  int len;

  CHK(ptr = upb_decode_string(ptr, d->limit, &len));

  if (field->label == UPB_LABEL_REPEATED) {
    return upb_decode_toarray(ptr, layout, field, len, msg, d);
  } else if (field->label == UPB_LABEL_MAP) {
    return upb_decode_mapfield(ptr, layout, field, len, msg, d);
  } else {
    switch (field->descriptortype) {
      case UPB_DESCRIPTOR_TYPE_STRING:
      case UPB_DESCRIPTOR_TYPE_BYTES: {
        upb_strview str;
        ptr = upb_decode_strfield(ptr, d, len, &str);
        CHK(upb_decode_addval(msg, field, &str, sizeof(str), d));
        break;
      }
      case UPB_DESCRIPTOR_TYPE_MESSAGE: {
        const upb_msglayout *subl = layout->submsgs[field->submsg_index];
        upb_msg *submsg = upb_getorcreatemsg(msg, field, subl, d);
        CHK(submsg);
        CHK(ptr = upb_decode_msgfield(ptr, subl, len, submsg, d));
        break;
      }
      default:
        /* TODO(haberman): should we accept the last element of a packed? */
        return upb_append_unknown(ptr + len, msg, d);
    }
    upb_decode_setpresent(msg, field);
    return ptr;
  }
}

static const upb_msglayout_field *upb_find_field(const upb_msglayout *l,
                                                 uint32_t field_number) {
  /* Lots of optimization opportunities here. */
  int i;
  for (i = 0; i < l->field_count; i++) {
    if (l->fields[i].number == field_number) {
      return &l->fields[i];
    }
  }

  return NULL;  /* Unknown field. */
}

static const char *upb_decode_field(const char *ptr,
                                    const upb_msglayout *layout, upb_msg *msg,
                                    upb_decstate *d) {
  uint32_t tag;
  const upb_msglayout_field *field;
  int field_number;

  d->field_start = ptr;
  CHK(ptr = upb_decode_varint32(ptr, d->limit, &tag));
  field_number = tag >> 3;
  field = upb_find_field(layout, field_number);

  if (field) {
    switch (tag & 7) {
      case UPB_WIRE_TYPE_VARINT:
        return upb_decode_varintfield(ptr, msg, field, d);
      case UPB_WIRE_TYPE_32BIT:
        return upb_decode_32bitfield(ptr, field, msg, d);
      case UPB_WIRE_TYPE_64BIT:
        return upb_decode_64bitfield(ptr, field, msg, d);
      case UPB_WIRE_TYPE_DELIMITED:
        return upb_decode_delimitedfield(ptr, layout, field, msg, d);
      case UPB_WIRE_TYPE_START_GROUP: {
        const upb_msglayout *subl = layout->submsgs[field->submsg_index];
        upb_msg *group;

        if (field->label == UPB_LABEL_REPEATED) {
          group = upb_addmsg(msg, field, subl, d);
        } else {
          group = upb_getorcreatemsg(msg, field, subl, d);
        }

        return upb_decode_groupfield(ptr, subl, field_number, group, d);
      }
      case UPB_WIRE_TYPE_END_GROUP:
        d->end_group = field_number;
        return ptr;
      default:
        CHK(false);
    }
  } else {
    CHK(field_number != 0);
    CHK(ptr = upb_skip_unknownfielddata(ptr, d, tag));
    CHK(ptr = upb_append_unknown(ptr, msg, d));
    return ptr;
  }
  UPB_UNREACHABLE();
}

static const char *upb_decode_message(const char *ptr, const upb_msglayout *l,
                                      upb_msg *msg, upb_decstate *d) {
  while (ptr < d->limit) {
    CHK(ptr = upb_decode_field(ptr, l, msg, d));
  }

  return ptr;
}

bool upb_decode(const char *buf, size_t size, void *msg, const upb_msglayout *l,
                upb_arena *arena) {
  upb_decstate state;
  state.limit = buf + size;
  state.arena = arena;
  state.depth = 64;
  state.end_group = 0;

  /* Early exit required for buf==NULL case. */
  if (size == 0) return true;

  CHK(upb_decode_message(buf, l, msg, &state));
  return state.end_group == 0;
}

#undef CHK
#undef PTR_AT
