
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
  /* Current decoding pointer.  Points to the beginning of a field until we
   * have finished decoding the whole field. */
  const char *ptr;
} upb_decstate;

/* Data pertaining to a single message frame. */
typedef struct {
  const char *limit;
  int32_t group_number;  /* 0 if we are not parsing a group. */

  /* These members are unset for an unknown group frame. */
  char *msg;
  const upb_msglayout *m;
} upb_decframe;

#define CHK(x) if (!(x)) { return false; }

static bool upb_skip_unknowngroup(upb_decstate *d, int field_number,
                                  const char *limit);
static bool upb_decode_message(upb_decstate *d, const char *limit,
                               int group_number, char *msg,
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
  *val = u64;
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

static bool upb_decode_tag(const char **ptr, const char *limit,
                           int *field_number, int *wire_type) {
  uint32_t tag = 0;
  CHK(upb_decode_varint32(ptr, limit, &tag));
  *field_number = tag >> 3;
  *wire_type = tag & 7;
  return true;
}

static int32_t upb_zzdecode_32(uint32_t n) {
  return (n >> 1) ^ -(int32_t)(n & 1);
}

static int64_t upb_zzdecode_64(uint64_t n) {
  return (n >> 1) ^ -(int64_t)(n & 1);
}

static bool upb_decode_string(const char **ptr, const char *limit,
                              upb_strview *val) {
  uint32_t len;

  CHK(upb_decode_varint32(ptr, limit, &len) &&
      len < INT32_MAX &&
      limit - *ptr >= (int32_t)len);

  *val = upb_strview_make(*ptr, len);
  *ptr += len;
  return true;
}

static void upb_set32(void *msg, size_t ofs, uint32_t val) {
  memcpy((char*)msg + ofs, &val, sizeof(val));
}

static bool upb_append_unknown(upb_decstate *d, upb_decframe *frame,
                               const char *start) {
  upb_msg_addunknown(frame->msg, start, d->ptr - start);
  return true;
}

static bool upb_skip_unknownfielddata(upb_decstate *d, upb_decframe *frame,
                                      int field_number, int wire_type) {
  switch (wire_type) {
    case UPB_WIRE_TYPE_VARINT: {
      uint64_t val;
      return upb_decode_varint(&d->ptr, frame->limit, &val);
    }
    case UPB_WIRE_TYPE_32BIT: {
      uint32_t val;
      return upb_decode_32bit(&d->ptr, frame->limit, &val);
    }
    case UPB_WIRE_TYPE_64BIT: {
      uint64_t val;
      return upb_decode_64bit(&d->ptr, frame->limit, &val);
    }
    case UPB_WIRE_TYPE_DELIMITED: {
      upb_strview val;
      return upb_decode_string(&d->ptr, frame->limit, &val);
    }
    case UPB_WIRE_TYPE_START_GROUP:
      return upb_skip_unknowngroup(d, field_number, frame->limit);
    case UPB_WIRE_TYPE_END_GROUP:
      CHK(field_number == frame->group_number);
      frame->limit = d->ptr;
      return true;
  }
  return false;
}

static bool upb_array_grow(upb_array *arr, size_t elements, size_t elem_size) {
  size_t needed = arr->len + elements;
  size_t new_size = UPB_MAX(arr->size, 8);
  size_t new_bytes;
  size_t old_bytes;
  void *new_data;
  upb_alloc *alloc = upb_arena_alloc(arr->arena);

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
                               size_t elem_size) {
  if (arr->size - arr->len < elements) {
    if (!upb_array_grow(arr, elements, elem_size)) return NULL;
  }
  return (char*)arr->data + (arr->len * elem_size);
}

bool upb_array_add(upb_array *arr, size_t elements, size_t elem_size,
                   const void *data) {
  void *dest = upb_array_reserve(arr, elements, elem_size);

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
    upb_fieldtype_t type = upb_desctype_to_fieldtype[field->descriptortype];
    arr = upb_array_new(type, upb_msg_arena(frame->msg));
    if (!arr) {
      return NULL;
    }
    *(upb_array**)&frame->msg[field->offset] = arr;
  }

  return arr;
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
    field_mem = upb_array_reserve(arr, 1, size);
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

static bool upb_decode_submsg(upb_decstate *d, upb_decframe *frame,
                              const char *limit,
                              const upb_msglayout_field *field,
                              int group_number) {
  upb_msg **submsg = (void*)(frame->msg + field->offset);
  const upb_msglayout *subm = frame->m->submsgs[field->submsg_index];

  if (!*submsg) {
    *submsg = upb_msg_new(subm, upb_msg_arena(frame->msg));
    CHK(*submsg);
  }

  upb_decode_message(d, limit, group_number, *submsg, subm);
  return true;
}

static bool upb_decode_varintfield(upb_decstate *d, upb_decframe *frame,
                                   const char *field_start,
                                   const upb_msglayout_field *field) {
  uint64_t val;
  CHK(upb_decode_varint(&d->ptr, frame->limit, &val));

  switch (field->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_UINT64:
      CHK(upb_decode_addval(frame, field, &val, sizeof(val)));
      break;
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_UINT32:
    case UPB_DESCRIPTOR_TYPE_ENUM: {
      uint32_t val32 = val;
      CHK(upb_decode_addval(frame, field, &val32, sizeof(val32)));
      break;
    }
    case UPB_DESCRIPTOR_TYPE_BOOL: {
      bool valbool = val != 0;
      CHK(upb_decode_addval(frame, field, &valbool, sizeof(valbool)));
      break;
    }
    case UPB_DESCRIPTOR_TYPE_SINT32: {
      int32_t decoded = upb_zzdecode_32(val);
      CHK(upb_decode_addval(frame, field, &decoded, sizeof(decoded)));
      break;
    }
    case UPB_DESCRIPTOR_TYPE_SINT64: {
      int64_t decoded = upb_zzdecode_64(val);
      CHK(upb_decode_addval(frame, field, &decoded, sizeof(decoded)));
      break;
    }
    default:
      return upb_append_unknown(d, frame, field_start);
  }

  upb_decode_setpresent(frame, field);
  return true;
}

static bool upb_decode_64bitfield(upb_decstate *d, upb_decframe *frame,
                                  const char *field_start,
                                  const upb_msglayout_field *field) {
  uint64_t val;
  CHK(upb_decode_64bit(&d->ptr, frame->limit, &val));

  switch (field->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
      CHK(upb_decode_addval(frame, field, &val, sizeof(val)));
      break;
    default:
      return upb_append_unknown(d, frame, field_start);
  }

  upb_decode_setpresent(frame, field);
  return true;
}

static bool upb_decode_32bitfield(upb_decstate *d, upb_decframe *frame,
                                  const char *field_start,
                                  const upb_msglayout_field *field) {
  uint32_t val;
  CHK(upb_decode_32bit(&d->ptr, frame->limit, &val));

  switch (field->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_FLOAT:
    case UPB_DESCRIPTOR_TYPE_FIXED32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
      CHK(upb_decode_addval(frame, field, &val, sizeof(val)));
      break;
    default:
      return upb_append_unknown(d, frame, field_start);
  }

  upb_decode_setpresent(frame, field);
  return true;
}

static bool upb_decode_fixedpacked(upb_array *arr, upb_strview data,
                                   int elem_size) {
  size_t elements = data.size / elem_size;

  CHK((size_t)(elements * elem_size) == data.size);
  CHK(upb_array_add(arr, elements, elem_size, data.data));

  return true;
}

static bool upb_decode_toarray(upb_decstate *d, upb_decframe *frame,
                               const char *field_start,
                               const upb_msglayout_field *field,
                               upb_strview val) {
  upb_array *arr = upb_getorcreatearr(frame, field);
  CHK(arr);

#define VARINT_CASE(ctype, decode)                           \
  {                                                          \
    const char *ptr = val.data;                              \
    const char *limit = ptr + val.size;                      \
    while (ptr < limit) {                                    \
      uint64_t val;                                          \
      ctype decoded;                                         \
      CHK(upb_decode_varint(&ptr, limit, &val));             \
      decoded = (decode)(val);                               \
      CHK(upb_array_add(arr, 1, sizeof(decoded), &decoded)); \
    }                                                        \
    return true;                                             \
  }

  switch (field->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_STRING:
    case UPB_DESCRIPTOR_TYPE_BYTES: {
      return upb_array_add(arr, 1, sizeof(val), &val);
    }
    case UPB_DESCRIPTOR_TYPE_FLOAT:
    case UPB_DESCRIPTOR_TYPE_FIXED32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
      return upb_decode_fixedpacked(arr, val, sizeof(int32_t));
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
      return upb_decode_fixedpacked(arr, val, sizeof(int64_t));
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
      VARINT_CASE(int32_t, upb_zzdecode_32);
    case UPB_DESCRIPTOR_TYPE_SINT64:
      VARINT_CASE(int64_t, upb_zzdecode_64);
    case UPB_DESCRIPTOR_TYPE_MESSAGE: {
      const upb_msglayout *subm = frame->m->submsgs[field->submsg_index];
      upb_msg *submsg = upb_msg_new(subm, upb_msg_arena(frame->msg));

      CHK(submsg);
      CHK(val.size <= (size_t)(frame->limit - val.data));
      upb_array_add(arr, 1, sizeof(submsg), &submsg);

      d->ptr -= val.size;
      return upb_decode_message(
          d, val.data + val.size, frame->group_number, submsg, subm);
    }
    case UPB_DESCRIPTOR_TYPE_GROUP:
      return upb_append_unknown(d, frame, field_start);
  }
#undef VARINT_CASE
  UPB_UNREACHABLE();
}

static bool upb_decode_delimitedfield(upb_decstate *d, upb_decframe *frame,
                                      const char *field_start,
                                      const upb_msglayout_field *field) {
  upb_strview val;

  CHK(upb_decode_string(&d->ptr, frame->limit, &val));

  if (field->label == UPB_LABEL_REPEATED) {
    return upb_decode_toarray(d, frame, field_start, field, val);
  } else {
    switch (field->descriptortype) {
      case UPB_DESCRIPTOR_TYPE_STRING:
      case UPB_DESCRIPTOR_TYPE_BYTES: {
        CHK(upb_decode_addval(frame, field, &val, sizeof(val)));
        break;
      }
      case UPB_DESCRIPTOR_TYPE_MESSAGE:
        CHK(val.size <= (size_t)(frame->limit - val.data));
        d->ptr -= val.size;
        CHK(upb_decode_submsg(d, frame, val.data + val.size, field, 0));
        break;
      default:
        /* TODO(haberman): should we accept the last element of a packed? */
        return upb_append_unknown(d, frame, field_start);
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
  int field_number;
  int wire_type;
  const char *field_start = d->ptr;
  const upb_msglayout_field *field;

  CHK(upb_decode_tag(&d->ptr, frame->limit, &field_number, &wire_type));
  field = upb_find_field(frame->m, field_number);

  if (field) {
    switch (wire_type) {
      case UPB_WIRE_TYPE_VARINT:
        return upb_decode_varintfield(d, frame, field_start, field);
      case UPB_WIRE_TYPE_32BIT:
        return upb_decode_32bitfield(d, frame, field_start, field);
      case UPB_WIRE_TYPE_64BIT:
        return upb_decode_64bitfield(d, frame, field_start, field);
      case UPB_WIRE_TYPE_DELIMITED:
        return upb_decode_delimitedfield(d, frame, field_start, field);
      case UPB_WIRE_TYPE_START_GROUP:
        CHK(field->descriptortype == UPB_DESCRIPTOR_TYPE_GROUP);
        return upb_decode_submsg(d, frame, frame->limit, field, field_number);
      case UPB_WIRE_TYPE_END_GROUP:
        CHK(frame->group_number == field_number)
        frame->limit = d->ptr;
        return true;
      default:
        CHK(false);
    }
  } else {
    CHK(field_number != 0);
    CHK(upb_skip_unknownfielddata(d, frame, field_number, wire_type));
    CHK(upb_append_unknown(d, frame, field_start));
    return true;
  }
}

static bool upb_skip_unknowngroup(upb_decstate *d, int field_number,
                                  const char *limit) {
  upb_decframe frame;
  frame.msg = NULL;
  frame.m = NULL;
  frame.group_number = field_number;
  frame.limit = limit;

  while (d->ptr < frame.limit) {
    int wire_type;
    int field_number;

    CHK(upb_decode_tag(&d->ptr, frame.limit, &field_number, &wire_type));
    CHK(upb_skip_unknownfielddata(d, &frame, field_number, wire_type));
  }

  return true;
}

static bool upb_decode_message(upb_decstate *d, const char *limit,
                               int group_number, char *msg,
                               const upb_msglayout *l) {
  upb_decframe frame;
  frame.group_number = group_number;
  frame.limit = limit;
  frame.msg = msg;
  frame.m = l;

  while (d->ptr < frame.limit) {
    CHK(upb_decode_field(d, &frame));
  }

  return true;
}

bool upb_decode(const char *buf, size_t size, void *msg,
                const upb_msglayout *l) {
  upb_decstate state;
  state.ptr = buf;

  return upb_decode_message(&state, buf + size, 0, msg, l);
}

#undef CHK
