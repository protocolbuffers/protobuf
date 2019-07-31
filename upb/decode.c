
#include <string.h>
#include "upb/upb.h"
#include "upb/decode.h"

#include "upb/port_def.inc"

/* Maps descriptor type -> upb field type.  */
const uint8_t upb_desctype_to_fieldtype[] = {
  UPB_WIRE_TYPE_END_GROUP,  /* ENDGROUP */
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

/* Data pertaining to the parse. */
typedef struct {
  const char *ptr;           /* Current parsing position. */
  const char *field_start;   /* Start of this field. */
  const char *limit;         /* End of delimited region or end of buffer. */
  upb_arena *arena;
  int depth;
  uint32_t end_group;  /* Set to field number of END_GROUP tag, if any. */
} upb_decstate;

/* Data passed by value to each parsing function. */
typedef struct {
  char *msg;
  const upb_msglayout *layout;
  upb_decstate *state;
} upb_decframe;

#define CHK(x) if (!(x)) { return 0; }

static bool upb_skip_unknowngroup(upb_decstate *d, int field_number);
static bool upb_decode_message(upb_decstate *d, char *msg,
                               const upb_msglayout *l);

static bool upb_decode_varint(const char **ptr, const char *limit,
                              uint64_t *val) {
  uint8_t byte;
  int bitpos = 0;
  const char *p = *ptr;
  *val = 0;

  do {
    CHK(bitpos < 70 && p < limit);
    byte = *p;
    *val |= (uint64_t)(byte & 0x7F) << bitpos;
    p++;
    bitpos += 7;
  } while (byte & 0x80);

  *ptr = p;
  return true;
}

static bool upb_decode_varint32(const char **ptr, const char *limit,
                                uint32_t *val) {
  uint64_t u64;
  CHK(upb_decode_varint(ptr, limit, &u64) && u64 <= UINT32_MAX);
  *val = (uint32_t)u64;
  return true;
}

static bool upb_decode_64bit(const char **ptr, const char *limit,
                             uint64_t *val) {
  CHK(limit - *ptr >= 8);
  memcpy(val, *ptr, 8);
  *ptr += 8;
  return true;
}

static bool upb_decode_32bit(const char **ptr, const char *limit,
                             uint32_t *val) {
  CHK(limit - *ptr >= 4);
  memcpy(val, *ptr, 4);
  *ptr += 4;
  return true;
}

static int32_t upb_zzdecode_32(uint32_t n) {
  return (n >> 1) ^ -(int32_t)(n & 1);
}

static int64_t upb_zzdecode_64(uint64_t n) {
  return (n >> 1) ^ -(int64_t)(n & 1);
}

static bool upb_decode_string(const char **ptr, const char *limit,
                              int *outlen) {
  uint32_t len;

  CHK(upb_decode_varint32(ptr, limit, &len) &&
      len < INT32_MAX &&
      limit - *ptr >= (int32_t)len);

  *outlen = len;
  return true;
}

static void upb_set32(void *msg, size_t ofs, uint32_t val) {
  memcpy((char*)msg + ofs, &val, sizeof(val));
}

static bool upb_append_unknown(upb_decstate *d, upb_decframe *frame) {
  upb_msg_addunknown(frame->msg, d->field_start, d->ptr - d->field_start,
                     d->arena);
  return true;
}


static bool upb_skip_unknownfielddata(upb_decstate *d, uint32_t tag,
                                      uint32_t group_fieldnum) {
  switch (tag & 7) {
    case UPB_WIRE_TYPE_VARINT: {
      uint64_t val;
      return upb_decode_varint(&d->ptr, d->limit, &val);
    }
    case UPB_WIRE_TYPE_32BIT: {
      uint32_t val;
      return upb_decode_32bit(&d->ptr, d->limit, &val);
    }
    case UPB_WIRE_TYPE_64BIT: {
      uint64_t val;
      return upb_decode_64bit(&d->ptr, d->limit, &val);
    }
    case UPB_WIRE_TYPE_DELIMITED: {
      int len;
      CHK(upb_decode_string(&d->ptr, d->limit, &len));
      d->ptr += len;
      return true;
    }
    case UPB_WIRE_TYPE_START_GROUP:
      return upb_skip_unknowngroup(d, tag >> 3);
    case UPB_WIRE_TYPE_END_GROUP:
      return (tag >> 3) == group_fieldnum;
  }
  return false;
}

static bool upb_skip_unknowngroup(upb_decstate *d, int field_number) {
  while (d->ptr < d->limit && d->end_group == 0) {
    uint32_t tag = 0;
    CHK(upb_decode_varint32(&d->ptr, d->limit, &tag));
    CHK(upb_skip_unknownfielddata(d, tag, field_number));
  }

  CHK(d->end_group == field_number);
  d->end_group = 0;
  return true;
}

static bool upb_array_grow(upb_array *arr, size_t elements, size_t elem_size,
                           upb_arena *arena) {
  size_t needed = arr->len + elements;
  size_t new_size = UPB_MAX(arr->size, 8);
  size_t new_bytes;
  size_t old_bytes;
  void *new_data;
  upb_alloc *alloc = upb_arena_alloc(arena);

  while (new_size < needed) {
    new_size *= 2;
  }

  old_bytes = arr->len * elem_size;
  new_bytes = new_size * elem_size;
  new_data = upb_realloc(alloc, arr->data, old_bytes, new_bytes);
  CHK(new_data);

  arr->data = new_data;
  arr->size = new_size;
  return true;
}

static void *upb_array_reserve(upb_array *arr, size_t elements,
                               size_t elem_size, upb_arena *arena) {
  if (arr->size - arr->len < elements) {
    CHK(upb_array_grow(arr, elements, elem_size, arena));
  }
  return (char*)arr->data + (arr->len * elem_size);
}

bool upb_array_add(upb_array *arr, size_t elements, size_t elem_size,
                   const void *data, upb_arena *arena) {
  void *dest = upb_array_reserve(arr, elements, elem_size, arena);

  CHK(dest);
  arr->len += elements;
  memcpy(dest, data, elements * elem_size);

  return true;
}

static upb_array *upb_getarr(upb_decframe *frame,
                             const upb_msglayout_field *field) {
  UPB_ASSERT(field->label == UPB_LABEL_REPEATED);
  return *(upb_array**)&frame->msg[field->offset];
}

static upb_array *upb_getorcreatearr(upb_decframe *frame,
                                     const upb_msglayout_field *field) {
  upb_array *arr = upb_getarr(frame, field);

  if (!arr) {
    arr = upb_array_new(frame->state->arena);
    CHK(arr);
    *(upb_array**)&frame->msg[field->offset] = arr;
  }

  return arr;
}

static upb_msg *upb_getorcreatemsg(upb_decframe *frame,
                                   const upb_msglayout_field *field,
                                   const upb_msglayout **subm) {
  upb_msg **submsg = (void*)(frame->msg + field->offset);
  *subm = frame->layout->submsgs[field->submsg_index];

  UPB_ASSERT(field->label != UPB_LABEL_REPEATED);

  if (!*submsg) {
    *submsg = upb_msg_new(*subm, frame->state->arena);
    CHK(*submsg);
  }

  return *submsg;
}

static upb_msg *upb_addmsg(upb_decframe *frame,
                           const upb_msglayout_field *field,
                           const upb_msglayout **subm) {
  upb_msg *submsg;
  upb_array *arr = upb_getorcreatearr(frame, field);

  *subm = frame->layout->submsgs[field->submsg_index];
  submsg = upb_msg_new(*subm, frame->state->arena);
  CHK(submsg);
  upb_array_add(arr, 1, sizeof(submsg), &submsg, frame->state->arena);

  return submsg;
}

static void upb_sethasbit(upb_decframe *frame,
                          const upb_msglayout_field *field) {
  int32_t hasbit = field->presence;
  UPB_ASSERT(field->presence > 0);
  frame->msg[hasbit / 8] |= (1 << (hasbit % 8));
}

static void upb_setoneofcase(upb_decframe *frame,
                             const upb_msglayout_field *field) {
  UPB_ASSERT(field->presence < 0);
  upb_set32(frame->msg, ~field->presence, field->number);
}

static bool upb_decode_addval(upb_decframe *frame,
                               const upb_msglayout_field *field, void *val,
                               size_t size) {
  char *field_mem = frame->msg + field->offset;
  upb_array *arr;

  if (field->label == UPB_LABEL_REPEATED) {
    arr = upb_getorcreatearr(frame, field);
    CHK(arr);
    field_mem = upb_array_reserve(arr, 1, size, frame->state->arena);
    CHK(field_mem);
  }

  memcpy(field_mem, val, size);
  return true;
}

static void upb_decode_setpresent(upb_decframe *frame,
                                  const upb_msglayout_field *field) {
  if (field->label == UPB_LABEL_REPEATED) {
   upb_array *arr = upb_getarr(frame, field);
   UPB_ASSERT(arr->len < arr->size);
   arr->len++;
  } else if (field->presence < 0) {
    upb_setoneofcase(frame, field);
  } else if (field->presence > 0) {
    upb_sethasbit(frame, field);
  }
}

static bool upb_decode_msgfield(upb_decstate *d, upb_msg *msg,
                                const upb_msglayout *layout, int limit) {
  const char* saved_limit = d->limit;
  d->limit = d->ptr + limit;
  CHK(--d->depth >= 0);
  upb_decode_message(d, msg, layout);
  d->depth++;
  d->limit = saved_limit;
  CHK(d->end_group == 0);
  return true;
}

static bool upb_decode_groupfield(upb_decstate *d, upb_msg *msg,
                                  const upb_msglayout *layout,
                                  int field_number) {
  CHK(--d->depth >= 0);
  upb_decode_message(d, msg, layout);
  d->depth++;
  CHK(d->end_group == field_number);
  d->end_group = 0;
  return true;
}

static bool upb_decode_varintfield(upb_decstate *d, upb_decframe *frame,
                                   const upb_msglayout_field *field) {
  uint64_t val;
  CHK(upb_decode_varint(&d->ptr, d->limit, &val));

  switch (field->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_UINT64:
      CHK(upb_decode_addval(frame, field, &val, sizeof(val)));
      break;
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_UINT32:
    case UPB_DESCRIPTOR_TYPE_ENUM: {
      uint32_t val32 = (uint32_t)val;
      CHK(upb_decode_addval(frame, field, &val32, sizeof(val32)));
      break;
    }
    case UPB_DESCRIPTOR_TYPE_BOOL: {
      bool valbool = val != 0;
      CHK(upb_decode_addval(frame, field, &valbool, sizeof(valbool)));
      break;
    }
    case UPB_DESCRIPTOR_TYPE_SINT32: {
      int32_t decoded = upb_zzdecode_32((uint32_t)val);
      CHK(upb_decode_addval(frame, field, &decoded, sizeof(decoded)));
      break;
    }
    case UPB_DESCRIPTOR_TYPE_SINT64: {
      int64_t decoded = upb_zzdecode_64(val);
      CHK(upb_decode_addval(frame, field, &decoded, sizeof(decoded)));
      break;
    }
    default:
      return upb_append_unknown(d, frame);
  }

  upb_decode_setpresent(frame, field);
  return true;
}

static bool upb_decode_64bitfield(upb_decstate *d, upb_decframe *frame,
                                  const upb_msglayout_field *field) {
  uint64_t val;
  CHK(upb_decode_64bit(&d->ptr, d->limit, &val));

  switch (field->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
      CHK(upb_decode_addval(frame, field, &val, sizeof(val)));
      break;
    default:
      return upb_append_unknown(d, frame);
  }

  upb_decode_setpresent(frame, field);
  return true;
}

static bool upb_decode_32bitfield(upb_decstate *d, upb_decframe *frame,
                                  const upb_msglayout_field *field) {
  uint32_t val;
  CHK(upb_decode_32bit(&d->ptr, d->limit, &val));

  switch (field->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_FLOAT:
    case UPB_DESCRIPTOR_TYPE_FIXED32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
      CHK(upb_decode_addval(frame, field, &val, sizeof(val)));
      break;
    default:
      return upb_append_unknown(d, frame);
  }

  upb_decode_setpresent(frame, field);
  return true;
}

static bool upb_decode_fixedpacked(upb_decstate *d, upb_array *arr,
                                   uint32_t len, int elem_size) {
  size_t elements = len / elem_size;

  CHK((size_t)(elements * elem_size) == len);
  CHK(upb_array_add(arr, elements, elem_size, d->ptr, d->arena));
  d->ptr += len;

  return true;
}

static upb_strview upb_decode_strfield(upb_decstate *d, uint32_t len) {
  upb_strview ret;
  ret.data = d->ptr;
  ret.size = len;
  d->ptr += len;
  return ret;
}

static bool upb_decode_toarray(upb_decstate *d, upb_decframe *frame,
                               const upb_msglayout_field *field, int len) {
  upb_array *arr = upb_getorcreatearr(frame, field);
  CHK(arr);

#define VARINT_CASE(ctype, decode) \
  VARINT_CASE_EX(ctype, decode, decode)

#define VARINT_CASE_EX(ctype, decode, dtype)                           \
  {                                                                    \
    const char *ptr = d->ptr;                                          \
    const char *limit = ptr + len;                                     \
    while (ptr < limit) {                                              \
      uint64_t val;                                                    \
      ctype decoded;                                                   \
      CHK(upb_decode_varint(&ptr, limit, &val));                       \
      decoded = (decode)((dtype)val);                                  \
      CHK(upb_array_add(arr, 1, sizeof(decoded), &decoded, d->arena)); \
    }                                                                  \
    d->ptr = ptr;                                                      \
    return true;                                                       \
  }

  switch (field->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_STRING:
    case UPB_DESCRIPTOR_TYPE_BYTES: {
      upb_strview str = upb_decode_strfield(d, len);
      return upb_array_add(arr, 1, sizeof(str), &str, d->arena);
    }
    case UPB_DESCRIPTOR_TYPE_FLOAT:
    case UPB_DESCRIPTOR_TYPE_FIXED32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
      return upb_decode_fixedpacked(d, arr, len, sizeof(int32_t));
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
      return upb_decode_fixedpacked(d, arr, len, sizeof(int64_t));
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
      const upb_msglayout *subm;
      upb_msg *submsg = upb_addmsg(frame, field, &subm);
      CHK(submsg);
      return upb_decode_msgfield(d, submsg, subm, len);
    }
    case UPB_DESCRIPTOR_TYPE_GROUP:
      return upb_append_unknown(d, frame);
  }
#undef VARINT_CASE
  UPB_UNREACHABLE();
}

static bool upb_decode_delimitedfield(upb_decstate *d, upb_decframe *frame,
                                      const upb_msglayout_field *field) {
  int len;

  CHK(upb_decode_string(&d->ptr, d->limit, &len));

  if (field->label == UPB_LABEL_REPEATED) {
    return upb_decode_toarray(d, frame, field, len);
  } else {
    switch (field->descriptortype) {
      case UPB_DESCRIPTOR_TYPE_STRING:
      case UPB_DESCRIPTOR_TYPE_BYTES: {
        upb_strview str = upb_decode_strfield(d, len);
        CHK(upb_decode_addval(frame, field, &str, sizeof(str)));
        break;
      }
      case UPB_DESCRIPTOR_TYPE_MESSAGE: {
        const upb_msglayout *subm;
        upb_msg *submsg = upb_getorcreatemsg(frame, field, &subm);
        CHK(submsg);
        CHK(upb_decode_msgfield(d, submsg, subm, len));
        break;
      }
      default:
        /* TODO(haberman): should we accept the last element of a packed? */
        d->ptr += len;
        return upb_append_unknown(d, frame);
    }
    upb_decode_setpresent(frame, field);
    return true;
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

static bool upb_decode_field(upb_decstate *d, upb_decframe *frame) {
  uint32_t tag;
  const upb_msglayout_field *field;
  int field_number;

  d->field_start = d->ptr;
  CHK(upb_decode_varint32(&d->ptr, d->limit, &tag));
  field_number = tag >> 3;
  field = upb_find_field(frame->layout, field_number);

  if (field) {
    switch (tag & 7) {
      case UPB_WIRE_TYPE_VARINT:
        return upb_decode_varintfield(d, frame, field);
      case UPB_WIRE_TYPE_32BIT:
        return upb_decode_32bitfield(d, frame, field);
      case UPB_WIRE_TYPE_64BIT:
        return upb_decode_64bitfield(d, frame, field);
      case UPB_WIRE_TYPE_DELIMITED:
        return upb_decode_delimitedfield(d, frame, field);
      case UPB_WIRE_TYPE_START_GROUP: {
        const upb_msglayout *layout;
        upb_msg *group;

        if (field->label == UPB_LABEL_REPEATED) {
          group = upb_addmsg(frame, field, &layout);
        } else {
          group = upb_getorcreatemsg(frame, field, &layout);
        }

        return upb_decode_groupfield(d, group, layout, field_number);
      }
      case UPB_WIRE_TYPE_END_GROUP:
        d->end_group = field_number;
        return true;
      default:
        CHK(false);
    }
  } else {
    CHK(field_number != 0);
    CHK(upb_skip_unknownfielddata(d, tag, -1));
    CHK(upb_append_unknown(d, frame));
    return true;
  }
}

static bool upb_decode_message(upb_decstate *d, char *msg, const upb_msglayout *l) {
  upb_decframe frame;
  frame.msg = msg;
  frame.layout = l;
  frame.state = d;

  while (d->ptr < d->limit) {
    CHK(upb_decode_field(d, &frame));
  }

  return true;
}

bool upb_decode(const char *buf, size_t size, void *msg, const upb_msglayout *l,
                upb_arena *arena) {
  upb_decstate state;
  state.ptr = buf;
  state.limit = buf + size;
  state.arena = arena;
  state.depth = 64;
  state.end_group = 0;

  CHK(upb_decode_message(&state, msg, l));
  return state.end_group == 0;
}

#undef CHK
