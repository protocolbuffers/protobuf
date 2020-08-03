
#include <setjmp.h>
#include <string.h>

#include "upb/decode.h"
#include "upb/upb.h"

#include "upb/port_def.inc"

/* Maps descriptor type -> upb field type.  */
static const uint8_t desctype_to_fieldtype[] = {
    -1,               /* invalid descriptor type */
    UPB_TYPE_DOUBLE,  /* DOUBLE */
    UPB_TYPE_FLOAT,   /* FLOAT */
    UPB_TYPE_INT64,   /* INT64 */
    UPB_TYPE_UINT64,  /* UINT64 */
    UPB_TYPE_INT32,   /* INT32 */
    UPB_TYPE_UINT64,  /* FIXED64 */
    UPB_TYPE_UINT32,  /* FIXED32 */
    UPB_TYPE_BOOL,    /* BOOL */
    UPB_TYPE_STRING,  /* STRING */
    UPB_TYPE_MESSAGE, /* GROUP */
    UPB_TYPE_MESSAGE, /* MESSAGE */
    UPB_TYPE_BYTES,   /* BYTES */
    UPB_TYPE_UINT32,  /* UINT32 */
    UPB_TYPE_ENUM,    /* ENUM */
    UPB_TYPE_INT32,   /* SFIXED32 */
    UPB_TYPE_INT64,   /* SFIXED64 */
    UPB_TYPE_INT32,   /* SINT32 */
    UPB_TYPE_INT64,   /* SINT64 */
};

/* Maps descriptor type -> upb map size.  */
static const uint8_t desctype_to_mapsize[] = {
    -1,                 /* invalid descriptor type */
    8,                  /* DOUBLE */
    4,                  /* FLOAT */
    8,                  /* INT64 */
    8,                  /* UINT64 */
    4,                  /* INT32 */
    8,                  /* FIXED64 */
    4,                  /* FIXED32 */
    1,                  /* BOOL */
    UPB_MAPTYPE_STRING, /* STRING */
    sizeof(void *),     /* GROUP */
    sizeof(void *),     /* MESSAGE */
    UPB_MAPTYPE_STRING, /* BYTES */
    4,                  /* UINT32 */
    4,                  /* ENUM */
    4,                  /* SFIXED32 */
    8,                  /* SFIXED64 */
    4,                  /* SINT32 */
    8,                  /* SINT64 */
};

static const unsigned fixed32_ok = (1 << UPB_DTYPE_FLOAT) |
                                   (1 << UPB_DTYPE_FIXED32) |
                                   (1 << UPB_DTYPE_SFIXED32);

static const unsigned fixed64_ok = (1 << UPB_DTYPE_DOUBLE) |
                                   (1 << UPB_DTYPE_FIXED64) |
                                   (1 << UPB_DTYPE_SFIXED64);

/* Op: an action to be performed for a wire-type/field-type combination. */
#define OP_SCALAR_LG2(n) (n)      /* n in [0, 2, 3] => op in [0, 2, 3] */
#define OP_STRING 4
#define OP_BYTES 5
#define OP_SUBMSG 6
/* Ops above are scalar-only. Repeated fields can use any op.  */
#define OP_FIXPCK_LG2(n) (n + 5)  /* n in [2, 3] => op in [7, 8] */
#define OP_VARPCK_LG2(n) (n + 9)  /* n in [0, 2, 3] => op in [9, 11, 12] */

static const int8_t varint_ops[19] = {
    -1,               /* field not found */
    -1,               /* DOUBLE */
    -1,               /* FLOAT */
    OP_SCALAR_LG2(3), /* INT64 */
    OP_SCALAR_LG2(3), /* UINT64 */
    OP_SCALAR_LG2(2), /* INT32 */
    -1,               /* FIXED64 */
    -1,               /* FIXED32 */
    OP_SCALAR_LG2(0), /* BOOL */
    -1,               /* STRING */
    -1,               /* GROUP */
    -1,               /* MESSAGE */
    -1,               /* BYTES */
    OP_SCALAR_LG2(2), /* UINT32 */
    OP_SCALAR_LG2(2), /* ENUM */
    -1,               /* SFIXED32 */
    -1,               /* SFIXED64 */
    OP_SCALAR_LG2(2), /* SINT32 */
    OP_SCALAR_LG2(3), /* SINT64 */
};

static const int8_t delim_ops[37] = {
    /* For non-repeated field type. */
    -1,        /* field not found */
    -1,        /* DOUBLE */
    -1,        /* FLOAT */
    -1,        /* INT64 */
    -1,        /* UINT64 */
    -1,        /* INT32 */
    -1,        /* FIXED64 */
    -1,        /* FIXED32 */
    -1,        /* BOOL */
    OP_STRING, /* STRING */
    -1,        /* GROUP */
    OP_SUBMSG, /* MESSAGE */
    OP_BYTES,  /* BYTES */
    -1,        /* UINT32 */
    -1,        /* ENUM */
    -1,        /* SFIXED32 */
    -1,        /* SFIXED64 */
    -1,        /* SINT32 */
    -1,        /* SINT64 */
    /* For repeated field type. */
    OP_FIXPCK_LG2(3), /* REPEATED DOUBLE */
    OP_FIXPCK_LG2(2), /* REPEATED FLOAT */
    OP_VARPCK_LG2(3), /* REPEATED INT64 */
    OP_VARPCK_LG2(3), /* REPEATED UINT64 */
    OP_VARPCK_LG2(2), /* REPEATED INT32 */
    OP_FIXPCK_LG2(3), /* REPEATED FIXED64 */
    OP_FIXPCK_LG2(2), /* REPEATED FIXED32 */
    OP_VARPCK_LG2(0), /* REPEATED BOOL */
    OP_STRING,        /* REPEATED STRING */
    OP_SUBMSG,        /* REPEATED GROUP */
    OP_SUBMSG,        /* REPEATED MESSAGE */
    OP_BYTES,         /* REPEATED BYTES */
    OP_VARPCK_LG2(2), /* REPEATED UINT32 */
    OP_VARPCK_LG2(2), /* REPEATED ENUM */
    OP_FIXPCK_LG2(2), /* REPEATED SFIXED32 */
    OP_FIXPCK_LG2(3), /* REPEATED SFIXED64 */
    OP_VARPCK_LG2(2), /* REPEATED SINT32 */
    OP_VARPCK_LG2(3), /* REPEATED SINT64 */
};

/* Data pertaining to the parse. */
typedef struct {
  const char *limit;       /* End of delimited region or end of buffer. */
  upb_arena *arena;
  int depth;
  uint32_t end_group; /* Set to field number of END_GROUP tag, if any. */
  jmp_buf err;
} upb_decstate;

typedef union {
  bool bool_val;
  uint32_t uint32_val;
  uint64_t uint64_val;
  upb_strview str_val;
} wireval;

static const char *decode_msg(upb_decstate *d, const char *ptr, upb_msg *msg,
                              const upb_msglayout *layout);

UPB_NORETURN static void decode_err(upb_decstate *d) { longjmp(d->err, 1); }

void decode_verifyutf8(upb_decstate *d, const char *buf, int len) {
  static const uint8_t utf8_offset[] = {
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0,
  };

  int i, j;
  uint8_t offset;

  i = 0;
  while (i < len) {
    offset = utf8_offset[(uint8_t)buf[i]];
    if (offset == 0 || i + offset > len) {
      decode_err(d);
    }
    for (j = i + 1; j < i + offset; j++) {
      if ((buf[j] & 0xc0) != 0x80) {
        decode_err(d);
      }
    }
    i += offset;
  }
  if (i != len) decode_err(d);
}

static bool decode_reserve(upb_decstate *d, upb_array *arr, size_t elem) {
  bool need_realloc = arr->size - arr->len < elem;
  if (need_realloc && !_upb_array_realloc(arr, arr->len + elem, d->arena)) {
    decode_err(d);
  }
  return need_realloc;
}

UPB_NOINLINE
static const char *decode_longvarint64(upb_decstate *d, const char *ptr,
                                       const char *limit, uint64_t *val) {
  uint8_t byte;
  int bitpos = 0;
  uint64_t out = 0;

  do {
    if (bitpos >= 70 || ptr == limit) decode_err(d);
    byte = *ptr;
    out |= (uint64_t)(byte & 0x7F) << bitpos;
    ptr++;
    bitpos += 7;
  } while (byte & 0x80);

  *val = out;
  return ptr;
}

UPB_FORCEINLINE
static const char *decode_varint64(upb_decstate *d, const char *ptr,
                                   const char *limit, uint64_t *val) {
  if (UPB_LIKELY(ptr < limit && (*ptr & 0x80) == 0)) {
    *val = (uint8_t)*ptr;
    return ptr + 1;
  } else {
    return decode_longvarint64(d, ptr, limit, val);
  }
}

static const char *decode_varint32(upb_decstate *d, const char *ptr,
                                   const char *limit, uint32_t *val) {
  uint64_t u64;
  ptr = decode_varint64(d, ptr, limit, &u64);
  if (u64 > UINT32_MAX) decode_err(d);
  *val = (uint32_t)u64;
  return ptr;
}

static void decode_munge(int type, wireval *val) {
  switch (type) {
    case UPB_DESCRIPTOR_TYPE_BOOL:
      val->bool_val = val->uint64_val != 0;
      break;
    case UPB_DESCRIPTOR_TYPE_SINT32: {
      uint32_t n = val->uint32_val;
      val->uint32_val = (n >> 1) ^ -(int32_t)(n & 1);
      break;
    }
    case UPB_DESCRIPTOR_TYPE_SINT64: {
      uint64_t n = val->uint64_val;
      val->uint64_val = (n >> 1) ^ -(int64_t)(n & 1);
      break;
    }
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_UINT32:
      if (!_upb_isle()) {
        /* The next stage will memcpy(dst, &val, 4) */
        val->uint32_val = val->uint64_val;
      }
      break;
  }
}

static const upb_msglayout_field *upb_find_field(const upb_msglayout *l,
                                                 uint32_t field_number) {
  static upb_msglayout_field none = {0, 0, 0, 0, 0, 0};

  /* Lots of optimization opportunities here. */
  int i;
  if (l == NULL) return &none;
  for (i = 0; i < l->field_count; i++) {
    if (l->fields[i].number == field_number) {
      return &l->fields[i];
    }
  }

  return &none; /* Unknown field. */
}

static upb_msg *decode_newsubmsg(upb_decstate *d, const upb_msglayout *layout,
                                 const upb_msglayout_field *field) {
  const upb_msglayout *subl = layout->submsgs[field->submsg_index];
  return _upb_msg_new(subl, d->arena);
}

static void decode_tosubmsg(upb_decstate *d, upb_msg *submsg,
                            const upb_msglayout *layout,
                            const upb_msglayout_field *field, upb_strview val) {
  const upb_msglayout *subl = layout->submsgs[field->submsg_index];
  const char *saved_limit = d->limit;
  if (--d->depth < 0) decode_err(d);
  d->limit = val.data + val.size;
  decode_msg(d, val.data, submsg, subl);
  d->limit = saved_limit;
  if (d->end_group != 0) decode_err(d);
  d->depth++;
}

static const char *decode_group(upb_decstate *d, const char *ptr,
                                upb_msg *submsg, const upb_msglayout *subl,
                                uint32_t number) {
  if (--d->depth < 0) decode_err(d);
  ptr = decode_msg(d, ptr, submsg, subl);
  if (d->end_group != number) decode_err(d);
  d->end_group = 0;
  d->depth++;
  return ptr;
}

static const char *decode_togroup(upb_decstate *d, const char *ptr,
                                  upb_msg *submsg, const upb_msglayout *layout,
                                  const upb_msglayout_field *field) {
  const upb_msglayout *subl = layout->submsgs[field->submsg_index];
  return decode_group(d, ptr, submsg, subl, field->number);
}

static const char *decode_toarray(upb_decstate *d, const char *ptr,
                                  upb_msg *msg, const upb_msglayout *layout,
                                  const upb_msglayout_field *field, wireval val,
                                  int op) {
  upb_array **arrp = UPB_PTR_AT(msg, field->offset, void);
  upb_array *arr = *arrp;
  void *mem;

  if (!arr) {
    upb_fieldtype_t type = desctype_to_fieldtype[field->descriptortype];
    arr = _upb_array_new(d->arena, type);
    if (!arr) decode_err(d);
    *arrp = arr;
  }

  decode_reserve(d, arr, 1);

  switch (op) {
    case OP_SCALAR_LG2(0):
    case OP_SCALAR_LG2(2):
    case OP_SCALAR_LG2(3):
      /* Append scalar value. */
      mem = UPB_PTR_AT(_upb_array_ptr(arr), arr->len << op, void);
      arr->len++;
      memcpy(mem, &val, 1 << op);
      return ptr;
    case OP_STRING:
      decode_verifyutf8(d, val.str_val.data, val.str_val.size);
      /* Fallthrough. */
    case OP_BYTES:
      /* Append bytes. */
      mem =
          UPB_PTR_AT(_upb_array_ptr(arr), arr->len * sizeof(upb_strview), void);
      arr->len++;
      memcpy(mem, &val, sizeof(upb_strview));
      return ptr;
    case OP_SUBMSG: {
      /* Append submessage / group. */
      upb_msg *submsg = decode_newsubmsg(d, layout, field);
      *UPB_PTR_AT(_upb_array_ptr(arr), arr->len * sizeof(void *), upb_msg *) =
          submsg;
      arr->len++;
      if (UPB_UNLIKELY(field->descriptortype == UPB_DTYPE_GROUP)) {
        ptr = decode_togroup(d, ptr, submsg, layout, field);
      } else {
        decode_tosubmsg(d, submsg, layout, field, val.str_val);
      }
      return ptr;
    }
    case OP_FIXPCK_LG2(2):
    case OP_FIXPCK_LG2(3): {
      /* Fixed packed. */
      int lg2 = op - OP_FIXPCK_LG2(0);
      int mask = (1 << lg2) - 1;
      size_t count = val.str_val.size >> lg2;
      if ((val.str_val.size & mask) != 0) {
        decode_err(d); /* Length isn't a round multiple of elem size. */
      }
      decode_reserve(d, arr, count);
      mem = UPB_PTR_AT(_upb_array_ptr(arr), arr->len << lg2, void);
      arr->len += count;
      memcpy(mem, val.str_val.data, val.str_val.size);
      return ptr;
    }
    case OP_VARPCK_LG2(0):
    case OP_VARPCK_LG2(2):
    case OP_VARPCK_LG2(3): {
      /* Varint packed. */
      int lg2 = op - OP_VARPCK_LG2(0);
      int scale = 1 << lg2;
      const char *ptr = val.str_val.data;
      const char *end = ptr + val.str_val.size;
      char *out = UPB_PTR_AT(_upb_array_ptr(arr), arr->len << lg2, void);
      while (ptr < end) {
        wireval elem;
        ptr = decode_varint64(d, ptr, end, &elem.uint64_val);
        decode_munge(field->descriptortype, &elem);
        if (decode_reserve(d, arr, 1)) {
          out = UPB_PTR_AT(_upb_array_ptr(arr), arr->len << lg2, void);
        }
        arr->len++;
        memcpy(out, &elem, scale);
        out += scale;
      }
      if (ptr != end) decode_err(d);
      return ptr;
    }
    default:
      UPB_UNREACHABLE();
  }
}

static void decode_tomap(upb_decstate *d, upb_msg *msg,
                         const upb_msglayout *layout,
                         const upb_msglayout_field *field, wireval val) {
  upb_map **map_p = UPB_PTR_AT(msg, field->offset, upb_map *);
  upb_map *map = *map_p;
  upb_map_entry ent;
  const upb_msglayout *entry = layout->submsgs[field->submsg_index];

  if (!map) {
    /* Lazily create map. */
    const upb_msglayout *entry = layout->submsgs[field->submsg_index];
    const upb_msglayout_field *key_field = &entry->fields[0];
    const upb_msglayout_field *val_field = &entry->fields[1];
    char key_size = desctype_to_mapsize[key_field->descriptortype];
    char val_size = desctype_to_mapsize[val_field->descriptortype];
    UPB_ASSERT(key_field->offset == 0);
    UPB_ASSERT(val_field->offset == sizeof(upb_strview));
    map = _upb_map_new(d->arena, key_size, val_size);
    *map_p = map;
  }

  /* Parse map entry. */
  memset(&ent, 0, sizeof(ent));

  if (entry->fields[1].descriptortype == UPB_DESCRIPTOR_TYPE_MESSAGE ||
      entry->fields[1].descriptortype == UPB_DESCRIPTOR_TYPE_GROUP) {
    /* Create proactively to handle the case where it doesn't appear. */
    ent.v.val = upb_value_ptr(_upb_msg_new(entry->submsgs[0], d->arena));
  }

  decode_tosubmsg(d, &ent.k, layout, field, val.str_val);

  /* Insert into map. */
  _upb_map_set(map, &ent.k, map->key_size, &ent.v, map->val_size, d->arena);
}

static const char *decode_tomsg(upb_decstate *d, const char *ptr, upb_msg *msg,
                                const upb_msglayout *layout,
                                const upb_msglayout_field *field, wireval val,
                                int op) {
  void *mem = UPB_PTR_AT(msg, field->offset, void);
  int type = field->descriptortype;

  /* Set presence if necessary. */
  if (field->presence < 0) {
    /* Oneof case */
    uint32_t *oneof_case = _upb_oneofcase_field(msg, field);
    if (op == OP_SUBMSG && *oneof_case != field->number) {
      memset(mem, 0, sizeof(void*));
    }
    *oneof_case = field->number;
  } else if (field->presence > 0) {
    _upb_sethas_field(msg, field);
  }

  /* Store into message. */
  switch (op) {
    case OP_SUBMSG: {
      upb_msg **submsgp = mem;
      upb_msg *submsg = *submsgp;
      if (!submsg) {
        submsg = decode_newsubmsg(d, layout, field);
        *submsgp = submsg;
      }
      if (UPB_UNLIKELY(type == UPB_DTYPE_GROUP)) {
        ptr = decode_togroup(d, ptr, submsg, layout, field);
      } else {
        decode_tosubmsg(d, submsg, layout, field, val.str_val);
      }
      break;
    }
    case OP_STRING:
      decode_verifyutf8(d, val.str_val.data, val.str_val.size);
      /* Fallthrough. */
    case OP_BYTES:
      memcpy(mem, &val, sizeof(upb_strview));
      break;
    case OP_SCALAR_LG2(3):
      memcpy(mem, &val, 8);
      break;
    case OP_SCALAR_LG2(2):
      memcpy(mem, &val, 4);
      break;
    case OP_SCALAR_LG2(0):
      memcpy(mem, &val, 1);
      break;
    default:
      UPB_UNREACHABLE();
  }

  return ptr;
}

static const char *decode_msg(upb_decstate *d, const char *ptr, upb_msg *msg,
                              const upb_msglayout *layout) {
  while (ptr < d->limit) {
    uint32_t tag;
    const upb_msglayout_field *field;
    int field_number;
    int wire_type;
    const char *field_start = ptr;
    wireval val;
    int op;

    ptr = decode_varint32(d, ptr, d->limit, &tag);
    field_number = tag >> 3;
    wire_type = tag & 7;

    field = upb_find_field(layout, field_number);

    switch (wire_type) {
      case UPB_WIRE_TYPE_VARINT:
        ptr = decode_varint64(d, ptr, d->limit, &val.uint64_val);
        op = varint_ops[field->descriptortype];
        decode_munge(field->descriptortype, &val);
        break;
      case UPB_WIRE_TYPE_32BIT:
        if (d->limit - ptr < 4) decode_err(d);
        memcpy(&val.uint32_val, ptr, 4);
        val.uint32_val = _upb_be_swap32(val.uint32_val);
        ptr += 4;
        op = OP_SCALAR_LG2(2);
        if (((1 << field->descriptortype) & fixed32_ok) == 0) goto unknown;
        break;
      case UPB_WIRE_TYPE_64BIT:
        if (d->limit - ptr < 8) decode_err(d);
        memcpy(&val.uint64_val, ptr, 8);
        val.uint64_val = _upb_be_swap64(val.uint64_val);
        ptr += 8;
        op = OP_SCALAR_LG2(3);
        if (((1 << field->descriptortype) & fixed64_ok) == 0) goto unknown;
        break;
      case UPB_WIRE_TYPE_DELIMITED: {
        uint32_t size;
        int ndx = field->descriptortype;
        if (_upb_isrepeated(field)) ndx += 18;
        ptr = decode_varint32(d, ptr, d->limit, &size);
        if (size >= INT32_MAX || (size_t)(d->limit - ptr) < size) {
          decode_err(d); /* Length overflow. */
        }
        val.str_val.data = ptr;
        val.str_val.size = size;
        ptr += size;
        op = delim_ops[ndx];
        break;
      }
      case UPB_WIRE_TYPE_START_GROUP:
        val.uint32_val = field_number;
        op = OP_SUBMSG;
        if (field->descriptortype != UPB_DTYPE_GROUP) goto unknown;
        break;
      case UPB_WIRE_TYPE_END_GROUP:
        d->end_group = field_number;
        return ptr;
      default:
        decode_err(d);
    }

    if (op >= 0) {
      /* Parse, using op for dispatch. */
      switch (field->label) {
        case UPB_LABEL_REPEATED:
        case _UPB_LABEL_PACKED:
          ptr = decode_toarray(d, ptr, msg, layout, field, val, op);
          break;
        case _UPB_LABEL_MAP:
          decode_tomap(d, msg, layout, field, val);
          break;
        default:
          ptr = decode_tomsg(d, ptr, msg, layout, field, val, op);
          break;
      }
    } else {
    unknown:
      /* Skip unknown field. */
      if (field_number == 0) decode_err(d);
      if (wire_type == UPB_WIRE_TYPE_START_GROUP) {
        ptr = decode_group(d, ptr, NULL, NULL, field_number);
      }
      if (msg) {
        if (!_upb_msg_addunknown(msg, field_start, ptr - field_start,
                                 d->arena)) {
          decode_err(d);
        }
      }
    }
  }

  if (ptr != d->limit) decode_err(d);
  return ptr;
}

bool upb_decode(const char *buf, size_t size, void *msg, const upb_msglayout *l,
                upb_arena *arena) {
  upb_decstate state;
  state.limit = buf + size;
  state.arena = arena;
  state.depth = 64;
  state.end_group = 0;

  if (setjmp(state.err)) return false;

  if (size == 0) return true;
  decode_msg(&state, buf, msg, l);

  return state.end_group == 0;
}

#undef OP_SCALAR_LG2
#undef OP_FIXPCK_LG2
#undef OP_VARPCK_LG2
#undef OP_STRING
#undef OP_SUBMSG
