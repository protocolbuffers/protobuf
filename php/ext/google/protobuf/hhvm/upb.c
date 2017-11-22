// Amalgamated source file
#include "upb.h"


/* Maps descriptor type -> upb field type.  */
static const uint8_t upb_desctype_to_fieldtype[] = {
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
  upb_env *env;
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
  const upb_msglayout_msginit_v1 *m;
} upb_decframe;

#define CHK(x) if (!(x)) { return false; }

static bool upb_skip_unknowngroup(upb_decstate *d, int field_number,
                                  const char *limit);
static bool upb_decode_message(upb_decstate *d, const char *limit,
                               int group_number, char *msg,
                               const upb_msglayout_msginit_v1 *l);

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
                              upb_stringview *val) {
  uint32_t len;

  CHK(upb_decode_varint32(ptr, limit, &len) &&
      len < INT32_MAX &&
      limit - *ptr >= (int32_t)len);

  *val = upb_stringview_make(*ptr, len);
  *ptr += len;
  return true;
}

static void upb_set32(void *msg, size_t ofs, uint32_t val) {
  memcpy((char*)msg + ofs, &val, sizeof(val));
}

static bool upb_append_unknown(upb_decstate *d, upb_decframe *frame,
                               const char *start) {
  UPB_UNUSED(d);
  UPB_UNUSED(frame);
  UPB_UNUSED(start);
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
      upb_stringview val;
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

static bool upb_array_grow(upb_array *arr, size_t elements) {
  size_t needed = arr->len + elements;
  size_t new_size = UPB_MAX(arr->size, 8);
  size_t new_bytes;
  size_t old_bytes;
  void *new_data;

  while (new_size < needed) {
    new_size *= 2;
  }

  old_bytes = arr->len * arr->element_size;
  new_bytes = new_size * arr->element_size;
  new_data = upb_realloc(arr->alloc, arr->data, old_bytes, new_bytes);
  CHK(new_data);

  arr->data = new_data;
  arr->size = new_size;
  return true;
}

static void *upb_array_reserve(upb_array *arr, size_t elements) {
  if (arr->size - arr->len < elements) {
    CHK(upb_array_grow(arr, elements));
  }
  return (char*)arr->data + (arr->len * arr->element_size);
}

static void *upb_array_add(upb_array *arr, size_t elements) {
  void *ret = upb_array_reserve(arr, elements);
  arr->len += elements;
  return ret;
}

static upb_array *upb_getarr(upb_decframe *frame,
                             const upb_msglayout_fieldinit_v1 *field) {
  UPB_ASSERT(field->label == UPB_LABEL_REPEATED);
  return *(upb_array**)&frame->msg[field->offset];
}

static upb_array *upb_getorcreatearr(upb_decstate *d,
                                     upb_decframe *frame,
                                     const upb_msglayout_fieldinit_v1 *field) {
  upb_array *arr = upb_getarr(frame, field);

  if (!arr) {
    arr = upb_env_malloc(d->env, sizeof(*arr));
    if (!arr) {
      return NULL;
    }
    upb_array_init(arr, upb_desctype_to_fieldtype[field->type],
                   upb_arena_alloc(upb_env_arena(d->env)));
    *(upb_array**)&frame->msg[field->offset] = arr;
  }

  return arr;
}

static void upb_sethasbit(upb_decframe *frame,
                          const upb_msglayout_fieldinit_v1 *field) {
  UPB_ASSERT(field->hasbit != UPB_NO_HASBIT);
  frame->msg[field->hasbit / 8] |= (1 << (field->hasbit % 8));
}

static void upb_setoneofcase(upb_decframe *frame,
                             const upb_msglayout_fieldinit_v1 *field) {
  UPB_ASSERT(field->oneof_index != UPB_NOT_IN_ONEOF);
  upb_set32(frame->msg, frame->m->oneofs[field->oneof_index].case_offset,
            field->number);
}

static char *upb_decode_prepareslot(upb_decstate *d,
                                    upb_decframe *frame,
                                    const upb_msglayout_fieldinit_v1 *field) {
  char *field_mem = frame->msg + field->offset;
  upb_array *arr;

  if (field->label == UPB_LABEL_REPEATED) {
    arr = upb_getorcreatearr(d, frame, field);
    field_mem = upb_array_reserve(arr, 1);
  }

  return field_mem;
}

static void upb_decode_setpresent(upb_decframe *frame,
                                  const upb_msglayout_fieldinit_v1 *field) {
  if (field->label == UPB_LABEL_REPEATED) {
   upb_array *arr = upb_getarr(frame, field);
   UPB_ASSERT(arr->len < arr->size);
   arr->len++;
  } else if (field->oneof_index != UPB_NOT_IN_ONEOF) {
    upb_setoneofcase(frame, field);
  } else if (field->hasbit != UPB_NO_HASBIT) {
    upb_sethasbit(frame, field);
  }
}

static bool upb_decode_submsg(upb_decstate *d,
                              upb_decframe *frame,
                              const char *limit,
                              const upb_msglayout_fieldinit_v1 *field,
                              int group_number) {
  char *submsg = *(void**)&frame->msg[field->offset];
  const upb_msglayout_msginit_v1 *subm;

  UPB_ASSERT(field->submsg_index != UPB_NO_SUBMSG);
  subm = frame->m->submsgs[field->submsg_index];
  UPB_ASSERT(subm);

  if (!submsg) {
    submsg = upb_env_malloc(d->env, upb_msg_sizeof((upb_msglayout *)subm));
    CHK(submsg);
    submsg = upb_msg_init(
        submsg, (upb_msglayout*)subm, upb_arena_alloc(upb_env_arena(d->env)));
    *(void**)&frame->msg[field->offset] = submsg;
  }

  upb_decode_message(d, limit, group_number, submsg, subm);

  return true;
}

static bool upb_decode_varintfield(upb_decstate *d, upb_decframe *frame,
                                   const char *field_start,
                                   const upb_msglayout_fieldinit_v1 *field) {
  uint64_t val;
  void *field_mem;

  field_mem = upb_decode_prepareslot(d, frame, field);
  CHK(field_mem);
  CHK(upb_decode_varint(&d->ptr, frame->limit, &val));

  switch ((upb_descriptortype_t)field->type) {
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_UINT64:
      memcpy(field_mem, &val, sizeof(val));
      break;
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_UINT32:
    case UPB_DESCRIPTOR_TYPE_ENUM: {
      uint32_t val32 = val;
      memcpy(field_mem, &val32, sizeof(val32));
      break;
    }
    case UPB_DESCRIPTOR_TYPE_BOOL: {
      bool valbool = val != 0;
      memcpy(field_mem, &valbool, sizeof(valbool));
      break;
    }
    case UPB_DESCRIPTOR_TYPE_SINT32: {
      int32_t decoded = upb_zzdecode_32(val);
      memcpy(field_mem, &decoded, sizeof(decoded));
      break;
    }
    case UPB_DESCRIPTOR_TYPE_SINT64: {
      int64_t decoded = upb_zzdecode_64(val);
      memcpy(field_mem, &decoded, sizeof(decoded));
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
                                  const upb_msglayout_fieldinit_v1 *field) {
  void *field_mem;
  uint64_t val;

  field_mem = upb_decode_prepareslot(d, frame, field);
  CHK(field_mem);
  CHK(upb_decode_64bit(&d->ptr, frame->limit, &val));

  switch ((upb_descriptortype_t)field->type) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
      memcpy(field_mem, &val, sizeof(val));
      break;
    default:
      return upb_append_unknown(d, frame, field_start);
  }

  upb_decode_setpresent(frame, field);
  return true;
}

static bool upb_decode_32bitfield(upb_decstate *d, upb_decframe *frame,
                                  const char *field_start,
                                  const upb_msglayout_fieldinit_v1 *field) {
  void *field_mem;
  uint32_t val;

  field_mem = upb_decode_prepareslot(d, frame, field);
  CHK(field_mem);
  CHK(upb_decode_32bit(&d->ptr, frame->limit, &val));

  switch ((upb_descriptortype_t)field->type) {
    case UPB_DESCRIPTOR_TYPE_FLOAT:
    case UPB_DESCRIPTOR_TYPE_FIXED32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
      memcpy(field_mem, &val, sizeof(val));
      break;
    default:
      return upb_append_unknown(d, frame, field_start);
  }

  upb_decode_setpresent(frame, field);
  return true;
}

static bool upb_decode_fixedpacked(upb_array *arr, upb_stringview data,
                                   int elem_size) {
  int elements = data.size / elem_size;
  void *field_mem;

  CHK((size_t)(elements * elem_size) == data.size);
  field_mem = upb_array_add(arr, elements);
  CHK(field_mem);
  memcpy(field_mem, data.data, data.size);
  return true;
}

static bool upb_decode_toarray(upb_decstate *d, upb_decframe *frame,
                               const char *field_start,
                               const upb_msglayout_fieldinit_v1 *field,
                               upb_stringview val) {
  upb_array *arr = upb_getorcreatearr(d, frame, field);

#define VARINT_CASE(ctype, decode) { \
  const char *ptr = val.data; \
  const char *limit = ptr + val.size; \
  while (ptr < limit) { \
    uint64_t val; \
    void *field_mem; \
    ctype decoded; \
    CHK(upb_decode_varint(&ptr, limit, &val)); \
    decoded = (decode)(val); \
    field_mem = upb_array_add(arr, 1); \
    CHK(field_mem); \
    memcpy(field_mem, &decoded, sizeof(ctype)); \
  } \
  return true; \
}

  switch ((upb_descriptortype_t)field->type) {
    case UPB_DESCRIPTOR_TYPE_STRING:
    case UPB_DESCRIPTOR_TYPE_BYTES: {
      void *field_mem = upb_array_add(arr, 1);
      CHK(field_mem);
      memcpy(field_mem, &val, sizeof(val));
      return true;
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
      /* TODO: proto2 enum field that isn't in the enum. */
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
    case UPB_DESCRIPTOR_TYPE_MESSAGE:
      CHK(val.size <= (size_t)(frame->limit - val.data));
      return upb_decode_submsg(d, frame, val.data + val.size, field, 0);
    case UPB_DESCRIPTOR_TYPE_GROUP:
      return upb_append_unknown(d, frame, field_start);
  }
#undef VARINT_CASE
  UPB_UNREACHABLE();
}

static bool upb_decode_delimitedfield(upb_decstate *d, upb_decframe *frame,
                                      const char *field_start,
                                      const upb_msglayout_fieldinit_v1 *field) {
  upb_stringview val;

  CHK(upb_decode_string(&d->ptr, frame->limit, &val));

  if (field->label == UPB_LABEL_REPEATED) {
    return upb_decode_toarray(d, frame, field_start, field, val);
  } else {
    switch ((upb_descriptortype_t)field->type) {
      case UPB_DESCRIPTOR_TYPE_STRING:
      case UPB_DESCRIPTOR_TYPE_BYTES: {
        void *field_mem = upb_decode_prepareslot(d, frame, field);
        CHK(field_mem);
        memcpy(field_mem, &val, sizeof(val));
        break;
      }
      case UPB_DESCRIPTOR_TYPE_MESSAGE:
        CHK(val.size <= (size_t)(frame->limit - val.data));
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

static bool upb_decode_field(upb_decstate *d, upb_decframe *frame) {
  int field_number;
  int wire_type;
  const char *field_start = d->ptr;
  const upb_msglayout_fieldinit_v1 *field;

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
        CHK(field->type == UPB_DESCRIPTOR_TYPE_GROUP);
        return upb_decode_submsg(d, frame, frame->limit, field, field_number);
      case UPB_WIRE_TYPE_END_GROUP:
        CHK(frame->group_number == field_number)
        frame->limit = d->ptr;
        return true;
      default:
        return false;
    }
  } else {
    CHK(field_number != 0);
    return upb_skip_unknownfielddata(d, frame, field_number, wire_type);
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
                               const upb_msglayout_msginit_v1 *l) {
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

bool upb_decode(upb_stringview buf, void *msg,
                const upb_msglayout_msginit_v1 *l, upb_env *env) {
  upb_decstate state;
  state.ptr = buf.data;
  state.env = env;

  return upb_decode_message(&state, buf.data + buf.size, 0, msg, l);
}

#undef CHK


#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  size_t len;
  char str[1];  /* Null-terminated string data follows. */
} str_t;

static str_t *newstr(const char *data, size_t len) {
  str_t *ret = upb_gmalloc(sizeof(*ret) + len);
  if (!ret) return NULL;
  ret->len = len;
  memcpy(ret->str, data, len);
  ret->str[len] = '\0';
  return ret;
}

static void freestr(str_t *s) { upb_gfree(s); }

/* isalpha() etc. from <ctype.h> are locale-dependent, which we don't want. */
static bool upb_isbetween(char c, char low, char high) {
  return c >= low && c <= high;
}

static bool upb_isletter(char c) {
  return upb_isbetween(c, 'A', 'Z') || upb_isbetween(c, 'a', 'z') || c == '_';
}

static bool upb_isalphanum(char c) {
  return upb_isletter(c) || upb_isbetween(c, '0', '9');
}

static bool upb_isident(const char *str, size_t len, bool full, upb_status *s) {
  bool start = true;
  size_t i;
  for (i = 0; i < len; i++) {
    char c = str[i];
    if (c == '.') {
      if (start || !full) {
        upb_status_seterrf(s, "invalid name: unexpected '.' (%s)", str);
        return false;
      }
      start = true;
    } else if (start) {
      if (!upb_isletter(c)) {
        upb_status_seterrf(
            s, "invalid name: path components must start with a letter (%s)",
            str);
        return false;
      }
      start = false;
    } else {
      if (!upb_isalphanum(c)) {
        upb_status_seterrf(s, "invalid name: non-alphanumeric character (%s)",
                           str);
        return false;
      }
    }
  }
  return !start;
}

static bool upb_isoneof(const upb_refcounted *def) {
  return def->vtbl == &upb_oneofdef_vtbl;
}

static bool upb_isfield(const upb_refcounted *def) {
  return def->vtbl == &upb_fielddef_vtbl;
}

static const upb_oneofdef *upb_trygetoneof(const upb_refcounted *def) {
  return upb_isoneof(def) ? (const upb_oneofdef*)def : NULL;
}

static const upb_fielddef *upb_trygetfield(const upb_refcounted *def) {
  return upb_isfield(def) ? (const upb_fielddef*)def : NULL;
}


/* upb_def ********************************************************************/

upb_deftype_t upb_def_type(const upb_def *d) { return d->type; }

const char *upb_def_fullname(const upb_def *d) { return d->fullname; }

const char *upb_def_name(const upb_def *d) {
  const char *p;

  if (d->fullname == NULL) {
    return NULL;
  } else if ((p = strrchr(d->fullname, '.')) == NULL) {
    /* No '.' in the name, return the full string. */
    return d->fullname;
  } else {
    /* Return one past the last '.'. */
    return p + 1;
  }
}

bool upb_def_setfullname(upb_def *def, const char *fullname, upb_status *s) {
  UPB_ASSERT(!upb_def_isfrozen(def));
  if (!upb_isident(fullname, strlen(fullname), true, s)) {
    return false;
  }

  fullname = upb_gstrdup(fullname);
  if (!fullname) {
    upb_upberr_setoom(s);
    return false;
  }

  upb_gfree((void*)def->fullname);
  def->fullname = fullname;
  return true;
}

const upb_filedef *upb_def_file(const upb_def *d) { return d->file; }

static bool upb_def_init(upb_def *def, upb_deftype_t type,
                         const struct upb_refcounted_vtbl *vtbl,
                         const void *owner) {
  if (!upb_refcounted_init(upb_def_upcast_mutable(def), vtbl, owner)) return false;
  def->type = type;
  def->fullname = NULL;
  def->came_from_user = false;
  def->file = NULL;
  return true;
}

static void upb_def_uninit(upb_def *def) {
  upb_gfree((void*)def->fullname);
}

static const char *msgdef_name(const upb_msgdef *m) {
  const char *name = upb_def_fullname(upb_msgdef_upcast(m));
  return name ? name : "(anonymous)";
}

static bool upb_validate_field(upb_fielddef *f, upb_status *s) {
  if (upb_fielddef_name(f) == NULL || upb_fielddef_number(f) == 0) {
    upb_status_seterrmsg(s, "fielddef must have name and number set");
    return false;
  }

  if (!f->type_is_set_) {
    upb_status_seterrmsg(s, "fielddef type was not initialized");
    return false;
  }

  if (upb_fielddef_lazy(f) &&
      upb_fielddef_descriptortype(f) != UPB_DESCRIPTOR_TYPE_MESSAGE) {
    upb_status_seterrmsg(s,
                         "only length-delimited submessage fields may be lazy");
    return false;
  }

  if (upb_fielddef_hassubdef(f)) {
    const upb_def *subdef;

    if (f->subdef_is_symbolic) {
      upb_status_seterrf(s, "field '%s.%s' has not been resolved",
                         msgdef_name(f->msg.def), upb_fielddef_name(f));
      return false;
    }

    subdef = upb_fielddef_subdef(f);
    if (subdef == NULL) {
      upb_status_seterrf(s, "field %s.%s is missing required subdef",
                         msgdef_name(f->msg.def), upb_fielddef_name(f));
      return false;
    }

    if (!upb_def_isfrozen(subdef) && !subdef->came_from_user) {
      upb_status_seterrf(s,
                         "subdef of field %s.%s is not frozen or being frozen",
                         msgdef_name(f->msg.def), upb_fielddef_name(f));
      return false;
    }
  }

  if (upb_fielddef_type(f) == UPB_TYPE_ENUM) {
    bool has_default_name = upb_fielddef_enumhasdefaultstr(f);
    bool has_default_number = upb_fielddef_enumhasdefaultint32(f);

    /* Previously verified by upb_validate_enumdef(). */
    UPB_ASSERT(upb_enumdef_numvals(upb_fielddef_enumsubdef(f)) > 0);

    /* We've already validated that we have an associated enumdef and that it
     * has at least one member, so at least one of these should be true.
     * Because if the user didn't set anything, we'll pick up the enum's
     * default, but if the user *did* set something we should at least pick up
     * the one they set (int32 or string). */
    UPB_ASSERT(has_default_name || has_default_number);

    if (!has_default_name) {
      upb_status_seterrf(s,
                         "enum default for field %s.%s (%d) is not in the enum",
                         msgdef_name(f->msg.def), upb_fielddef_name(f),
                         upb_fielddef_defaultint32(f));
      return false;
    }

    if (!has_default_number) {
      upb_status_seterrf(s,
                         "enum default for field %s.%s (%s) is not in the enum",
                         msgdef_name(f->msg.def), upb_fielddef_name(f),
                         upb_fielddef_defaultstr(f, NULL));
      return false;
    }

    /* Lift the effective numeric default into the field's default slot, in case
     * we were only getting it "by reference" from the enumdef. */
    upb_fielddef_setdefaultint32(f, upb_fielddef_defaultint32(f));
  }

  /* Ensure that MapEntry submessages only appear as repeated fields, not
   * optional/required (singular) fields. */
  if (upb_fielddef_type(f) == UPB_TYPE_MESSAGE &&
      upb_fielddef_msgsubdef(f) != NULL) {
    const upb_msgdef *subdef = upb_fielddef_msgsubdef(f);
    if (upb_msgdef_mapentry(subdef) && !upb_fielddef_isseq(f)) {
      upb_status_seterrf(s,
                         "Field %s refers to mapentry message but is not "
                         "a repeated field",
                         upb_fielddef_name(f) ? upb_fielddef_name(f) :
                         "(unnamed)");
      return false;
    }
  }

  return true;
}

static bool upb_validate_enumdef(const upb_enumdef *e, upb_status *s) {
  if (upb_enumdef_numvals(e) == 0) {
    upb_status_seterrf(s, "enum %s has no members (must have at least one)",
                       upb_enumdef_fullname(e));
    return false;
  }

  return true;
}

/* All submessage fields are lower than all other fields.
 * Secondly, fields are increasing in order. */
uint32_t field_rank(const upb_fielddef *f) {
  uint32_t ret = upb_fielddef_number(f);
  const uint32_t high_bit = 1 << 30;
  UPB_ASSERT(ret < high_bit);
  if (!upb_fielddef_issubmsg(f))
    ret |= high_bit;
  return ret;
}

int cmp_fields(const void *p1, const void *p2) {
  const upb_fielddef *f1 = *(upb_fielddef*const*)p1;
  const upb_fielddef *f2 = *(upb_fielddef*const*)p2;
  return field_rank(f1) - field_rank(f2);
}

static bool assign_msg_indices(upb_msgdef *m, upb_status *s) {
  /* Sort fields.  upb internally relies on UPB_TYPE_MESSAGE fields having the
   * lowest indexes, but we do not publicly guarantee this. */
  upb_msg_field_iter j;
  upb_msg_oneof_iter k;
  int i;
  uint32_t selector;
  int n = upb_msgdef_numfields(m);
  upb_fielddef **fields;

  if (n == 0) {
    m->selector_count = UPB_STATIC_SELECTOR_COUNT;
    m->submsg_field_count = 0;
    return true;
  }

  fields = upb_gmalloc(n * sizeof(*fields));
  if (!fields) {
    upb_upberr_setoom(s);
    return false;
  }

  m->submsg_field_count = 0;
  for(i = 0, upb_msg_field_begin(&j, m);
      !upb_msg_field_done(&j);
      upb_msg_field_next(&j), i++) {
    upb_fielddef *f = upb_msg_iter_field(&j);
    UPB_ASSERT(f->msg.def == m);
    if (!upb_validate_field(f, s)) {
      upb_gfree(fields);
      return false;
    }
    if (upb_fielddef_issubmsg(f)) {
      m->submsg_field_count++;
    }
    fields[i] = f;
  }

  qsort(fields, n, sizeof(*fields), cmp_fields);

  selector = UPB_STATIC_SELECTOR_COUNT + m->submsg_field_count;
  for (i = 0; i < n; i++) {
    upb_fielddef *f = fields[i];
    f->index_ = i;
    f->selector_base = selector + upb_handlers_selectorbaseoffset(f);
    selector += upb_handlers_selectorcount(f);
  }
  m->selector_count = selector;

#ifndef NDEBUG
  {
    /* Verify that all selectors for the message are distinct. */
#define TRY(type) \
    if (upb_handlers_getselector(f, type, &sel)) upb_inttable_insert(&t, sel, v);

    upb_inttable t;
    upb_value v;
    upb_selector_t sel;

    upb_inttable_init(&t, UPB_CTYPE_BOOL);
    v = upb_value_bool(true);
    upb_inttable_insert(&t, UPB_STARTMSG_SELECTOR, v);
    upb_inttable_insert(&t, UPB_ENDMSG_SELECTOR, v);
    upb_inttable_insert(&t, UPB_UNKNOWN_SELECTOR, v);
    for(upb_msg_field_begin(&j, m);
        !upb_msg_field_done(&j);
        upb_msg_field_next(&j)) {
      upb_fielddef *f = upb_msg_iter_field(&j);
      /* These calls will assert-fail in upb_table if the value already
       * exists. */
      TRY(UPB_HANDLER_INT32);
      TRY(UPB_HANDLER_INT64)
      TRY(UPB_HANDLER_UINT32)
      TRY(UPB_HANDLER_UINT64)
      TRY(UPB_HANDLER_FLOAT)
      TRY(UPB_HANDLER_DOUBLE)
      TRY(UPB_HANDLER_BOOL)
      TRY(UPB_HANDLER_STARTSTR)
      TRY(UPB_HANDLER_STRING)
      TRY(UPB_HANDLER_ENDSTR)
      TRY(UPB_HANDLER_STARTSUBMSG)
      TRY(UPB_HANDLER_ENDSUBMSG)
      TRY(UPB_HANDLER_STARTSEQ)
      TRY(UPB_HANDLER_ENDSEQ)
    }
    upb_inttable_uninit(&t);
  }
#undef TRY
#endif

  for(upb_msg_oneof_begin(&k, m), i = 0;
      !upb_msg_oneof_done(&k);
      upb_msg_oneof_next(&k), i++) {
    upb_oneofdef *o = upb_msg_iter_oneof(&k);
    o->index = i;
  }

  upb_gfree(fields);
  return true;
}

bool _upb_def_validate(upb_def *const*defs, size_t n, upb_status *s) {
  size_t i;

  /* First perform validation, in two passes so we can check that we have a
   * transitive closure without needing to search. */
  for (i = 0; i < n; i++) {
    upb_def *def = defs[i];
    if (upb_def_isfrozen(def)) {
      /* Could relax this requirement if it's annoying. */
      upb_status_seterrmsg(s, "def is already frozen");
      goto err;
    } else if (def->type == UPB_DEF_FIELD) {
      upb_status_seterrmsg(s, "standalone fielddefs can not be frozen");
      goto err;
    } else {
      /* Set now to detect transitive closure in the second pass. */
      def->came_from_user = true;

      if (def->type == UPB_DEF_ENUM &&
          !upb_validate_enumdef(upb_dyncast_enumdef(def), s)) {
        goto err;
      }
    }
  }

  /* Second pass of validation.  Also assign selector bases and indexes, and
   * compact tables. */
  for (i = 0; i < n; i++) {
    upb_def *def = defs[i];
    upb_msgdef *m = upb_dyncast_msgdef_mutable(def);
    upb_enumdef *e = upb_dyncast_enumdef_mutable(def);
    if (m) {
      upb_inttable_compact(&m->itof);
      if (!assign_msg_indices(m, s)) {
        goto err;
      }
    } else if (e) {
      upb_inttable_compact(&e->iton);
    }
  }

  return true;

err:
  for (i = 0; i < n; i++) {
    upb_def *def = defs[i];
    def->came_from_user = false;
  }
  UPB_ASSERT(!(s && upb_ok(s)));
  return false;
}

bool upb_def_freeze(upb_def *const* defs, size_t n, upb_status *s) {
  /* Def graph contains FieldDefs between each MessageDef, so double the
   * limit. */
  const size_t maxdepth = UPB_MAX_MESSAGE_DEPTH * 2;

  if (!_upb_def_validate(defs, n, s)) {
    return false;
  }


  /* Validation all passed; freeze the objects. */
  return upb_refcounted_freeze((upb_refcounted *const*)defs, n, s, maxdepth);
}


/* upb_enumdef ****************************************************************/

static void visitenum(const upb_refcounted *r, upb_refcounted_visit *visit,
                      void *closure) {
  const upb_enumdef *e = (const upb_enumdef*)r;
  const upb_def *def = upb_enumdef_upcast(e);
  if (upb_def_file(def)) {
    visit(r, upb_filedef_upcast(upb_def_file(def)), closure);
  }
}

static void freeenum(upb_refcounted *r) {
  upb_enumdef *e = (upb_enumdef*)r;
  upb_inttable_iter i;
  upb_inttable_begin(&i, &e->iton);
  for( ; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    /* To clean up the upb_gstrdup() from upb_enumdef_addval(). */
    upb_gfree(upb_value_getcstr(upb_inttable_iter_value(&i)));
  }
  upb_strtable_uninit(&e->ntoi);
  upb_inttable_uninit(&e->iton);
  upb_def_uninit(upb_enumdef_upcast_mutable(e));
  upb_gfree(e);
}

const struct upb_refcounted_vtbl upb_enumdef_vtbl = {&visitenum, &freeenum};

upb_enumdef *upb_enumdef_new(const void *owner) {
  upb_enumdef *e = upb_gmalloc(sizeof(*e));
  if (!e) return NULL;

  if (!upb_def_init(upb_enumdef_upcast_mutable(e), UPB_DEF_ENUM,
                    &upb_enumdef_vtbl, owner)) {
    goto err2;
  }

  if (!upb_strtable_init(&e->ntoi, UPB_CTYPE_INT32)) goto err2;
  if (!upb_inttable_init(&e->iton, UPB_CTYPE_CSTR)) goto err1;
  return e;

err1:
  upb_strtable_uninit(&e->ntoi);
err2:
  upb_gfree(e);
  return NULL;
}

bool upb_enumdef_freeze(upb_enumdef *e, upb_status *status) {
  upb_def *d = upb_enumdef_upcast_mutable(e);
  return upb_def_freeze(&d, 1, status);
}

const char *upb_enumdef_fullname(const upb_enumdef *e) {
  return upb_def_fullname(upb_enumdef_upcast(e));
}

const char *upb_enumdef_name(const upb_enumdef *e) {
  return upb_def_name(upb_enumdef_upcast(e));
}

bool upb_enumdef_setfullname(upb_enumdef *e, const char *fullname,
                             upb_status *s) {
  return upb_def_setfullname(upb_enumdef_upcast_mutable(e), fullname, s);
}

bool upb_enumdef_addval(upb_enumdef *e, const char *name, int32_t num,
                        upb_status *status) {
  char *name2;

  if (!upb_isident(name, strlen(name), false, status)) {
    return false;
  }

  if (upb_enumdef_ntoiz(e, name, NULL)) {
    upb_status_seterrf(status, "name '%s' is already defined", name);
    return false;
  }

  if (!upb_strtable_insert(&e->ntoi, name, upb_value_int32(num))) {
    upb_status_seterrmsg(status, "out of memory");
    return false;
  }

  if (!upb_inttable_lookup(&e->iton, num, NULL)) {
    name2 = upb_gstrdup(name);
    if (!name2 || !upb_inttable_insert(&e->iton, num, upb_value_cstr(name2))) {
      upb_status_seterrmsg(status, "out of memory");
      upb_strtable_remove(&e->ntoi, name, NULL);
      return false;
    }
  }

  if (upb_enumdef_numvals(e) == 1) {
    bool ok = upb_enumdef_setdefault(e, num, NULL);
    UPB_ASSERT(ok);
  }

  return true;
}

int32_t upb_enumdef_default(const upb_enumdef *e) {
  UPB_ASSERT(upb_enumdef_iton(e, e->defaultval));
  return e->defaultval;
}

bool upb_enumdef_setdefault(upb_enumdef *e, int32_t val, upb_status *s) {
  UPB_ASSERT(!upb_enumdef_isfrozen(e));
  if (!upb_enumdef_iton(e, val)) {
    upb_status_seterrf(s, "number '%d' is not in the enum.", val);
    return false;
  }
  e->defaultval = val;
  return true;
}

int upb_enumdef_numvals(const upb_enumdef *e) {
  return upb_strtable_count(&e->ntoi);
}

void upb_enum_begin(upb_enum_iter *i, const upb_enumdef *e) {
  /* We iterate over the ntoi table, to account for duplicate numbers. */
  upb_strtable_begin(i, &e->ntoi);
}

void upb_enum_next(upb_enum_iter *iter) { upb_strtable_next(iter); }
bool upb_enum_done(upb_enum_iter *iter) { return upb_strtable_done(iter); }

bool upb_enumdef_ntoi(const upb_enumdef *def, const char *name,
                      size_t len, int32_t *num) {
  upb_value v;
  if (!upb_strtable_lookup2(&def->ntoi, name, len, &v)) {
    return false;
  }
  if (num) *num = upb_value_getint32(v);
  return true;
}

const char *upb_enumdef_iton(const upb_enumdef *def, int32_t num) {
  upb_value v;
  return upb_inttable_lookup32(&def->iton, num, &v) ?
      upb_value_getcstr(v) : NULL;
}

const char *upb_enum_iter_name(upb_enum_iter *iter) {
  return upb_strtable_iter_key(iter);
}

int32_t upb_enum_iter_number(upb_enum_iter *iter) {
  return upb_value_getint32(upb_strtable_iter_value(iter));
}


/* upb_fielddef ***************************************************************/

static void upb_fielddef_init_default(upb_fielddef *f);

static void upb_fielddef_uninit_default(upb_fielddef *f) {
  if (f->type_is_set_ && f->default_is_string && f->defaultval.bytes)
    freestr(f->defaultval.bytes);
}

const char *upb_fielddef_fullname(const upb_fielddef *e) {
  return upb_def_fullname(upb_fielddef_upcast(e));
}

static void visitfield(const upb_refcounted *r, upb_refcounted_visit *visit,
                       void *closure) {
  const upb_fielddef *f = (const upb_fielddef*)r;
  const upb_def *def = upb_fielddef_upcast(f);
  if (upb_fielddef_containingtype(f)) {
    visit(r, upb_msgdef_upcast2(upb_fielddef_containingtype(f)), closure);
  }
  if (upb_fielddef_containingoneof(f)) {
    visit(r, upb_oneofdef_upcast(upb_fielddef_containingoneof(f)), closure);
  }
  if (upb_fielddef_subdef(f)) {
    visit(r, upb_def_upcast(upb_fielddef_subdef(f)), closure);
  }
  if (upb_def_file(def)) {
    visit(r, upb_filedef_upcast(upb_def_file(def)), closure);
  }
}

static void freefield(upb_refcounted *r) {
  upb_fielddef *f = (upb_fielddef*)r;
  upb_fielddef_uninit_default(f);
  if (f->subdef_is_symbolic)
    upb_gfree(f->sub.name);
  upb_def_uninit(upb_fielddef_upcast_mutable(f));
  upb_gfree(f);
}

static const char *enumdefaultstr(const upb_fielddef *f) {
  const upb_enumdef *e;
  UPB_ASSERT(f->type_is_set_ && f->type_ == UPB_TYPE_ENUM);
  e = upb_fielddef_enumsubdef(f);
  if (f->default_is_string && f->defaultval.bytes) {
    /* Default was explicitly set as a string. */
    str_t *s = f->defaultval.bytes;
    return s->str;
  } else if (e) {
    if (!f->default_is_string) {
      /* Default was explicitly set as an integer; look it up in enumdef. */
      const char *name = upb_enumdef_iton(e, f->defaultval.sint);
      if (name) {
        return name;
      }
    } else {
      /* Default is completely unset; pull enumdef default. */
      if (upb_enumdef_numvals(e) > 0) {
        const char *name = upb_enumdef_iton(e, upb_enumdef_default(e));
        UPB_ASSERT(name);
        return name;
      }
    }
  }
  return NULL;
}

static bool enumdefaultint32(const upb_fielddef *f, int32_t *val) {
  const upb_enumdef *e;
  UPB_ASSERT(f->type_is_set_ && f->type_ == UPB_TYPE_ENUM);
  e = upb_fielddef_enumsubdef(f);
  if (!f->default_is_string) {
    /* Default was explicitly set as an integer. */
    *val = f->defaultval.sint;
    return true;
  } else if (e) {
    if (f->defaultval.bytes) {
      /* Default was explicitly set as a str; try to lookup corresponding int. */
      str_t *s = f->defaultval.bytes;
      if (upb_enumdef_ntoiz(e, s->str, val)) {
        return true;
      }
    } else {
      /* Default is unset; try to pull in enumdef default. */
      if (upb_enumdef_numvals(e) > 0) {
        *val = upb_enumdef_default(e);
        return true;
      }
    }
  }
  return false;
}

const struct upb_refcounted_vtbl upb_fielddef_vtbl = {visitfield, freefield};

upb_fielddef *upb_fielddef_new(const void *o) {
  upb_fielddef *f = upb_gmalloc(sizeof(*f));
  if (!f) return NULL;
  if (!upb_def_init(upb_fielddef_upcast_mutable(f), UPB_DEF_FIELD,
                    &upb_fielddef_vtbl, o)) {
    upb_gfree(f);
    return NULL;
  }
  f->msg.def = NULL;
  f->sub.def = NULL;
  f->oneof = NULL;
  f->subdef_is_symbolic = false;
  f->msg_is_symbolic = false;
  f->label_ = UPB_LABEL_OPTIONAL;
  f->type_ = UPB_TYPE_INT32;
  f->number_ = 0;
  f->type_is_set_ = false;
  f->tagdelim = false;
  f->is_extension_ = false;
  f->lazy_ = false;
  f->packed_ = true;

  /* For the moment we default this to UPB_INTFMT_VARIABLE, since it will work
   * with all integer types and is in some since more "default" since the most
   * normal-looking proto2 types int32/int64/uint32/uint64 use variable.
   *
   * Other options to consider:
   * - there is no default; users must set this manually (like type).
   * - default signed integers to UPB_INTFMT_ZIGZAG, since it's more likely to
   *   be an optimal default for signed integers. */
  f->intfmt = UPB_INTFMT_VARIABLE;
  return f;
}

bool upb_fielddef_typeisset(const upb_fielddef *f) {
  return f->type_is_set_;
}

upb_fieldtype_t upb_fielddef_type(const upb_fielddef *f) {
  UPB_ASSERT(f->type_is_set_);
  return f->type_;
}

uint32_t upb_fielddef_index(const upb_fielddef *f) {
  return f->index_;
}

upb_label_t upb_fielddef_label(const upb_fielddef *f) {
  return f->label_;
}

upb_intfmt_t upb_fielddef_intfmt(const upb_fielddef *f) {
  return f->intfmt;
}

bool upb_fielddef_istagdelim(const upb_fielddef *f) {
  return f->tagdelim;
}

uint32_t upb_fielddef_number(const upb_fielddef *f) {
  return f->number_;
}

bool upb_fielddef_isextension(const upb_fielddef *f) {
  return f->is_extension_;
}

bool upb_fielddef_lazy(const upb_fielddef *f) {
  return f->lazy_;
}

bool upb_fielddef_packed(const upb_fielddef *f) {
  return f->packed_;
}

const char *upb_fielddef_name(const upb_fielddef *f) {
  return upb_def_fullname(upb_fielddef_upcast(f));
}

size_t upb_fielddef_getjsonname(const upb_fielddef *f, char *buf, size_t len) {
  const char *name = upb_fielddef_name(f);
  size_t src, dst = 0;
  bool ucase_next = false;

#define WRITE(byte) \
  ++dst; \
  if (dst < len) buf[dst - 1] = byte; \
  else if (dst == len) buf[dst - 1] = '\0'

  if (!name) {
    WRITE('\0');
    return 0;
  }

  /* Implement the transformation as described in the spec:
   *   1. upper case all letters after an underscore.
   *   2. remove all underscores.
   */
  for (src = 0; name[src]; src++) {
    if (name[src] == '_') {
      ucase_next = true;
      continue;
    }

    if (ucase_next) {
      WRITE(toupper(name[src]));
      ucase_next = false;
    } else {
      WRITE(name[src]);
    }
  }

  WRITE('\0');
  return dst;

#undef WRITE
}

const upb_msgdef *upb_fielddef_containingtype(const upb_fielddef *f) {
  return f->msg_is_symbolic ? NULL : f->msg.def;
}

const upb_oneofdef *upb_fielddef_containingoneof(const upb_fielddef *f) {
  return f->oneof;
}

upb_msgdef *upb_fielddef_containingtype_mutable(upb_fielddef *f) {
  return (upb_msgdef*)upb_fielddef_containingtype(f);
}

const char *upb_fielddef_containingtypename(upb_fielddef *f) {
  return f->msg_is_symbolic ? f->msg.name : NULL;
}

static void release_containingtype(upb_fielddef *f) {
  if (f->msg_is_symbolic) upb_gfree(f->msg.name);
}

bool upb_fielddef_setcontainingtypename(upb_fielddef *f, const char *name,
                                        upb_status *s) {
  char *name_copy;
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  if (upb_fielddef_containingtype(f)) {
    upb_status_seterrmsg(s, "field has already been added to a message.");
    return false;
  }
  /* TODO: validate name (upb_isident() doesn't quite work atm because this name
   * may have a leading "."). */

  name_copy = upb_gstrdup(name);
  if (!name_copy) {
    upb_upberr_setoom(s);
    return false;
  }

  release_containingtype(f);
  f->msg.name = name_copy;
  f->msg_is_symbolic = true;
  return true;
}

bool upb_fielddef_setname(upb_fielddef *f, const char *name, upb_status *s) {
  if (upb_fielddef_containingtype(f) || upb_fielddef_containingoneof(f)) {
    upb_status_seterrmsg(s, "Already added to message or oneof");
    return false;
  }
  return upb_def_setfullname(upb_fielddef_upcast_mutable(f), name, s);
}

static void chkdefaulttype(const upb_fielddef *f, upb_fieldtype_t type) {
  UPB_UNUSED(f);
  UPB_UNUSED(type);
  UPB_ASSERT(f->type_is_set_ && upb_fielddef_type(f) == type);
}

int64_t upb_fielddef_defaultint64(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_INT64);
  return f->defaultval.sint;
}

int32_t upb_fielddef_defaultint32(const upb_fielddef *f) {
  if (f->type_is_set_ && upb_fielddef_type(f) == UPB_TYPE_ENUM) {
    int32_t val;
    bool ok = enumdefaultint32(f, &val);
    UPB_ASSERT(ok);
    return val;
  } else {
    chkdefaulttype(f, UPB_TYPE_INT32);
    return f->defaultval.sint;
  }
}

uint64_t upb_fielddef_defaultuint64(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_UINT64);
  return f->defaultval.uint;
}

uint32_t upb_fielddef_defaultuint32(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_UINT32);
  return f->defaultval.uint;
}

bool upb_fielddef_defaultbool(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_BOOL);
  return f->defaultval.uint;
}

float upb_fielddef_defaultfloat(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_FLOAT);
  return f->defaultval.flt;
}

double upb_fielddef_defaultdouble(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_DOUBLE);
  return f->defaultval.dbl;
}

const char *upb_fielddef_defaultstr(const upb_fielddef *f, size_t *len) {
  UPB_ASSERT(f->type_is_set_);
  UPB_ASSERT(upb_fielddef_type(f) == UPB_TYPE_STRING ||
         upb_fielddef_type(f) == UPB_TYPE_BYTES ||
         upb_fielddef_type(f) == UPB_TYPE_ENUM);

  if (upb_fielddef_type(f) == UPB_TYPE_ENUM) {
    const char *ret = enumdefaultstr(f);
    UPB_ASSERT(ret);
    /* Enum defaults can't have embedded NULLs. */
    if (len) *len = strlen(ret);
    return ret;
  }

  if (f->default_is_string) {
    str_t *str = f->defaultval.bytes;
    if (len) *len = str->len;
    return str->str;
  }

  return NULL;
}

static void upb_fielddef_init_default(upb_fielddef *f) {
  f->default_is_string = false;
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_DOUBLE: f->defaultval.dbl = 0; break;
    case UPB_TYPE_FLOAT: f->defaultval.flt = 0; break;
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64: f->defaultval.sint = 0; break;
    case UPB_TYPE_UINT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_BOOL: f->defaultval.uint = 0; break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      f->defaultval.bytes = newstr("", 0);
      f->default_is_string = true;
      break;
    case UPB_TYPE_MESSAGE: break;
    case UPB_TYPE_ENUM:
      /* This is our special sentinel that indicates "not set" for an enum. */
      f->default_is_string = true;
      f->defaultval.bytes = NULL;
      break;
  }
}

const upb_def *upb_fielddef_subdef(const upb_fielddef *f) {
  return f->subdef_is_symbolic ? NULL : f->sub.def;
}

const upb_msgdef *upb_fielddef_msgsubdef(const upb_fielddef *f) {
  const upb_def *def = upb_fielddef_subdef(f);
  return def ? upb_dyncast_msgdef(def) : NULL;
}

const upb_enumdef *upb_fielddef_enumsubdef(const upb_fielddef *f) {
  const upb_def *def = upb_fielddef_subdef(f);
  return def ? upb_dyncast_enumdef(def) : NULL;
}

upb_def *upb_fielddef_subdef_mutable(upb_fielddef *f) {
  return (upb_def*)upb_fielddef_subdef(f);
}

const char *upb_fielddef_subdefname(const upb_fielddef *f) {
  if (f->subdef_is_symbolic) {
    return f->sub.name;
  } else if (f->sub.def) {
    return upb_def_fullname(f->sub.def);
  } else {
    return NULL;
  }
}

bool upb_fielddef_setnumber(upb_fielddef *f, uint32_t number, upb_status *s) {
  if (upb_fielddef_containingtype(f)) {
    upb_status_seterrmsg(
        s, "cannot change field number after adding to a message");
    return false;
  }
  if (number == 0 || number > UPB_MAX_FIELDNUMBER) {
    upb_status_seterrf(s, "invalid field number (%u)", number);
    return false;
  }
  f->number_ = number;
  return true;
}

void upb_fielddef_settype(upb_fielddef *f, upb_fieldtype_t type) {
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  UPB_ASSERT(upb_fielddef_checktype(type));
  upb_fielddef_uninit_default(f);
  f->type_ = type;
  f->type_is_set_ = true;
  upb_fielddef_init_default(f);
}

void upb_fielddef_setdescriptortype(upb_fielddef *f, int type) {
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  switch (type) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
      upb_fielddef_settype(f, UPB_TYPE_DOUBLE);
      break;
    case UPB_DESCRIPTOR_TYPE_FLOAT:
      upb_fielddef_settype(f, UPB_TYPE_FLOAT);
      break;
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
    case UPB_DESCRIPTOR_TYPE_SINT64:
      upb_fielddef_settype(f, UPB_TYPE_INT64);
      break;
    case UPB_DESCRIPTOR_TYPE_UINT64:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
      upb_fielddef_settype(f, UPB_TYPE_UINT64);
      break;
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
    case UPB_DESCRIPTOR_TYPE_SINT32:
      upb_fielddef_settype(f, UPB_TYPE_INT32);
      break;
    case UPB_DESCRIPTOR_TYPE_UINT32:
    case UPB_DESCRIPTOR_TYPE_FIXED32:
      upb_fielddef_settype(f, UPB_TYPE_UINT32);
      break;
    case UPB_DESCRIPTOR_TYPE_BOOL:
      upb_fielddef_settype(f, UPB_TYPE_BOOL);
      break;
    case UPB_DESCRIPTOR_TYPE_STRING:
      upb_fielddef_settype(f, UPB_TYPE_STRING);
      break;
    case UPB_DESCRIPTOR_TYPE_BYTES:
      upb_fielddef_settype(f, UPB_TYPE_BYTES);
      break;
    case UPB_DESCRIPTOR_TYPE_GROUP:
    case UPB_DESCRIPTOR_TYPE_MESSAGE:
      upb_fielddef_settype(f, UPB_TYPE_MESSAGE);
      break;
    case UPB_DESCRIPTOR_TYPE_ENUM:
      upb_fielddef_settype(f, UPB_TYPE_ENUM);
      break;
    default: UPB_ASSERT(false);
  }

  if (type == UPB_DESCRIPTOR_TYPE_FIXED64 ||
      type == UPB_DESCRIPTOR_TYPE_FIXED32 ||
      type == UPB_DESCRIPTOR_TYPE_SFIXED64 ||
      type == UPB_DESCRIPTOR_TYPE_SFIXED32) {
    upb_fielddef_setintfmt(f, UPB_INTFMT_FIXED);
  } else if (type == UPB_DESCRIPTOR_TYPE_SINT64 ||
             type == UPB_DESCRIPTOR_TYPE_SINT32) {
    upb_fielddef_setintfmt(f, UPB_INTFMT_ZIGZAG);
  } else {
    upb_fielddef_setintfmt(f, UPB_INTFMT_VARIABLE);
  }

  upb_fielddef_settagdelim(f, type == UPB_DESCRIPTOR_TYPE_GROUP);
}

upb_descriptortype_t upb_fielddef_descriptortype(const upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_FLOAT:  return UPB_DESCRIPTOR_TYPE_FLOAT;
    case UPB_TYPE_DOUBLE: return UPB_DESCRIPTOR_TYPE_DOUBLE;
    case UPB_TYPE_BOOL:   return UPB_DESCRIPTOR_TYPE_BOOL;
    case UPB_TYPE_STRING: return UPB_DESCRIPTOR_TYPE_STRING;
    case UPB_TYPE_BYTES:  return UPB_DESCRIPTOR_TYPE_BYTES;
    case UPB_TYPE_ENUM:   return UPB_DESCRIPTOR_TYPE_ENUM;
    case UPB_TYPE_INT32:
      switch (upb_fielddef_intfmt(f)) {
        case UPB_INTFMT_VARIABLE: return UPB_DESCRIPTOR_TYPE_INT32;
        case UPB_INTFMT_FIXED:    return UPB_DESCRIPTOR_TYPE_SFIXED32;
        case UPB_INTFMT_ZIGZAG:   return UPB_DESCRIPTOR_TYPE_SINT32;
      }
    case UPB_TYPE_INT64:
      switch (upb_fielddef_intfmt(f)) {
        case UPB_INTFMT_VARIABLE: return UPB_DESCRIPTOR_TYPE_INT64;
        case UPB_INTFMT_FIXED:    return UPB_DESCRIPTOR_TYPE_SFIXED64;
        case UPB_INTFMT_ZIGZAG:   return UPB_DESCRIPTOR_TYPE_SINT64;
      }
    case UPB_TYPE_UINT32:
      switch (upb_fielddef_intfmt(f)) {
        case UPB_INTFMT_VARIABLE: return UPB_DESCRIPTOR_TYPE_UINT32;
        case UPB_INTFMT_FIXED:    return UPB_DESCRIPTOR_TYPE_FIXED32;
        case UPB_INTFMT_ZIGZAG:   return -1;
      }
    case UPB_TYPE_UINT64:
      switch (upb_fielddef_intfmt(f)) {
        case UPB_INTFMT_VARIABLE: return UPB_DESCRIPTOR_TYPE_UINT64;
        case UPB_INTFMT_FIXED:    return UPB_DESCRIPTOR_TYPE_FIXED64;
        case UPB_INTFMT_ZIGZAG:   return -1;
      }
    case UPB_TYPE_MESSAGE:
      return upb_fielddef_istagdelim(f) ?
          UPB_DESCRIPTOR_TYPE_GROUP : UPB_DESCRIPTOR_TYPE_MESSAGE;
  }
  return 0;
}

void upb_fielddef_setisextension(upb_fielddef *f, bool is_extension) {
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  f->is_extension_ = is_extension;
}

void upb_fielddef_setlazy(upb_fielddef *f, bool lazy) {
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  f->lazy_ = lazy;
}

void upb_fielddef_setpacked(upb_fielddef *f, bool packed) {
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  f->packed_ = packed;
}

void upb_fielddef_setlabel(upb_fielddef *f, upb_label_t label) {
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  UPB_ASSERT(upb_fielddef_checklabel(label));
  f->label_ = label;
}

void upb_fielddef_setintfmt(upb_fielddef *f, upb_intfmt_t fmt) {
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  UPB_ASSERT(upb_fielddef_checkintfmt(fmt));
  f->intfmt = fmt;
}

void upb_fielddef_settagdelim(upb_fielddef *f, bool tag_delim) {
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  f->tagdelim = tag_delim;
  f->tagdelim = tag_delim;
}

static bool checksetdefault(upb_fielddef *f, upb_fieldtype_t type) {
  if (!f->type_is_set_ || upb_fielddef_isfrozen(f) ||
      upb_fielddef_type(f) != type) {
    UPB_ASSERT(false);
    return false;
  }
  if (f->default_is_string) {
    str_t *s = f->defaultval.bytes;
    UPB_ASSERT(s || type == UPB_TYPE_ENUM);
    if (s) freestr(s);
  }
  f->default_is_string = false;
  return true;
}

void upb_fielddef_setdefaultint64(upb_fielddef *f, int64_t value) {
  if (checksetdefault(f, UPB_TYPE_INT64))
    f->defaultval.sint = value;
}

void upb_fielddef_setdefaultint32(upb_fielddef *f, int32_t value) {
  if ((upb_fielddef_type(f) == UPB_TYPE_ENUM &&
       checksetdefault(f, UPB_TYPE_ENUM)) ||
      checksetdefault(f, UPB_TYPE_INT32)) {
    f->defaultval.sint = value;
  }
}

void upb_fielddef_setdefaultuint64(upb_fielddef *f, uint64_t value) {
  if (checksetdefault(f, UPB_TYPE_UINT64))
    f->defaultval.uint = value;
}

void upb_fielddef_setdefaultuint32(upb_fielddef *f, uint32_t value) {
  if (checksetdefault(f, UPB_TYPE_UINT32))
    f->defaultval.uint = value;
}

void upb_fielddef_setdefaultbool(upb_fielddef *f, bool value) {
  if (checksetdefault(f, UPB_TYPE_BOOL))
    f->defaultval.uint = value;
}

void upb_fielddef_setdefaultfloat(upb_fielddef *f, float value) {
  if (checksetdefault(f, UPB_TYPE_FLOAT))
    f->defaultval.flt = value;
}

void upb_fielddef_setdefaultdouble(upb_fielddef *f, double value) {
  if (checksetdefault(f, UPB_TYPE_DOUBLE))
    f->defaultval.dbl = value;
}

bool upb_fielddef_setdefaultstr(upb_fielddef *f, const void *str, size_t len,
                                upb_status *s) {
  str_t *str2;
  UPB_ASSERT(upb_fielddef_isstring(f) || f->type_ == UPB_TYPE_ENUM);
  if (f->type_ == UPB_TYPE_ENUM && !upb_isident(str, len, false, s))
    return false;

  if (f->default_is_string) {
    str_t *s = f->defaultval.bytes;
    UPB_ASSERT(s || f->type_ == UPB_TYPE_ENUM);
    if (s) freestr(s);
  } else {
    UPB_ASSERT(f->type_ == UPB_TYPE_ENUM);
  }

  str2 = newstr(str, len);
  f->defaultval.bytes = str2;
  f->default_is_string = true;
  return true;
}

void upb_fielddef_setdefaultcstr(upb_fielddef *f, const char *str,
                                 upb_status *s) {
  UPB_ASSERT(f->type_is_set_);
  upb_fielddef_setdefaultstr(f, str, str ? strlen(str) : 0, s);
}

bool upb_fielddef_enumhasdefaultint32(const upb_fielddef *f) {
  int32_t val;
  UPB_ASSERT(f->type_is_set_ && f->type_ == UPB_TYPE_ENUM);
  return enumdefaultint32(f, &val);
}

bool upb_fielddef_enumhasdefaultstr(const upb_fielddef *f) {
  UPB_ASSERT(f->type_is_set_ && f->type_ == UPB_TYPE_ENUM);
  return enumdefaultstr(f) != NULL;
}

static bool upb_subdef_typecheck(upb_fielddef *f, const upb_def *subdef,
                                 upb_status *s) {
  if (f->type_ == UPB_TYPE_MESSAGE) {
    if (upb_dyncast_msgdef(subdef)) return true;
    upb_status_seterrmsg(s, "invalid subdef type for this submessage field");
    return false;
  } else if (f->type_ == UPB_TYPE_ENUM) {
    if (upb_dyncast_enumdef(subdef)) return true;
    upb_status_seterrmsg(s, "invalid subdef type for this enum field");
    return false;
  } else {
    upb_status_seterrmsg(s, "only message and enum fields can have a subdef");
    return false;
  }
}

static void release_subdef(upb_fielddef *f) {
  if (f->subdef_is_symbolic) {
    upb_gfree(f->sub.name);
  } else if (f->sub.def) {
    upb_unref2(f->sub.def, f);
  }
}

bool upb_fielddef_setsubdef(upb_fielddef *f, const upb_def *subdef,
                            upb_status *s) {
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  UPB_ASSERT(upb_fielddef_hassubdef(f));
  if (subdef && !upb_subdef_typecheck(f, subdef, s)) return false;
  release_subdef(f);
  f->sub.def = subdef;
  f->subdef_is_symbolic = false;
  if (f->sub.def) upb_ref2(f->sub.def, f);
  return true;
}

bool upb_fielddef_setmsgsubdef(upb_fielddef *f, const upb_msgdef *subdef,
                               upb_status *s) {
  return upb_fielddef_setsubdef(f, upb_msgdef_upcast(subdef), s);
}

bool upb_fielddef_setenumsubdef(upb_fielddef *f, const upb_enumdef *subdef,
                                upb_status *s) {
  return upb_fielddef_setsubdef(f, upb_enumdef_upcast(subdef), s);
}

bool upb_fielddef_setsubdefname(upb_fielddef *f, const char *name,
                                upb_status *s) {
  char *name_copy;
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  if (!upb_fielddef_hassubdef(f)) {
    upb_status_seterrmsg(s, "field type does not accept a subdef");
    return false;
  }

  name_copy = upb_gstrdup(name);
  if (!name_copy) {
    upb_upberr_setoom(s);
    return false;
  }

  /* TODO: validate name (upb_isident() doesn't quite work atm because this name
   * may have a leading "."). */
  release_subdef(f);
  f->sub.name = name_copy;
  f->subdef_is_symbolic = true;
  return true;
}

bool upb_fielddef_issubmsg(const upb_fielddef *f) {
  return upb_fielddef_type(f) == UPB_TYPE_MESSAGE;
}

bool upb_fielddef_isstring(const upb_fielddef *f) {
  return upb_fielddef_type(f) == UPB_TYPE_STRING ||
         upb_fielddef_type(f) == UPB_TYPE_BYTES;
}

bool upb_fielddef_isseq(const upb_fielddef *f) {
  return upb_fielddef_label(f) == UPB_LABEL_REPEATED;
}

bool upb_fielddef_isprimitive(const upb_fielddef *f) {
  return !upb_fielddef_isstring(f) && !upb_fielddef_issubmsg(f);
}

bool upb_fielddef_ismap(const upb_fielddef *f) {
  return upb_fielddef_isseq(f) && upb_fielddef_issubmsg(f) &&
         upb_msgdef_mapentry(upb_fielddef_msgsubdef(f));
}

bool upb_fielddef_haspresence(const upb_fielddef *f) {
  if (upb_fielddef_isseq(f)) return false;
  if (upb_fielddef_issubmsg(f)) return true;

  /* Primitive field: return true unless there is a message that specifies
   * presence should not exist. */
  if (f->msg_is_symbolic || !f->msg.def) return true;
  return f->msg.def->syntax == UPB_SYNTAX_PROTO2;
}

bool upb_fielddef_hassubdef(const upb_fielddef *f) {
  return upb_fielddef_issubmsg(f) || upb_fielddef_type(f) == UPB_TYPE_ENUM;
}

static bool between(int32_t x, int32_t low, int32_t high) {
  return x >= low && x <= high;
}

bool upb_fielddef_checklabel(int32_t label) { return between(label, 1, 3); }
bool upb_fielddef_checktype(int32_t type) { return between(type, 1, 11); }
bool upb_fielddef_checkintfmt(int32_t fmt) { return between(fmt, 1, 3); }

bool upb_fielddef_checkdescriptortype(int32_t type) {
  return between(type, 1, 18);
}

/* upb_msgdef *****************************************************************/

static void visitmsg(const upb_refcounted *r, upb_refcounted_visit *visit,
                     void *closure) {
  upb_msg_oneof_iter o;
  const upb_msgdef *m = (const upb_msgdef*)r;
  const upb_def *def = upb_msgdef_upcast(m);
  upb_msg_field_iter i;
  for(upb_msg_field_begin(&i, m);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    visit(r, upb_fielddef_upcast2(f), closure);
  }
  for(upb_msg_oneof_begin(&o, m);
      !upb_msg_oneof_done(&o);
      upb_msg_oneof_next(&o)) {
    upb_oneofdef *f = upb_msg_iter_oneof(&o);
    visit(r, upb_oneofdef_upcast(f), closure);
  }
  if (upb_def_file(def)) {
    visit(r, upb_filedef_upcast(upb_def_file(def)), closure);
  }
}

static void freemsg(upb_refcounted *r) {
  upb_msgdef *m = (upb_msgdef*)r;
  upb_strtable_uninit(&m->ntof);
  upb_inttable_uninit(&m->itof);
  upb_def_uninit(upb_msgdef_upcast_mutable(m));
  upb_gfree(m);
}

const struct upb_refcounted_vtbl upb_msgdef_vtbl = {visitmsg, freemsg};

upb_msgdef *upb_msgdef_new(const void *owner) {
  upb_msgdef *m = upb_gmalloc(sizeof(*m));
  if (!m) return NULL;

  if (!upb_def_init(upb_msgdef_upcast_mutable(m), UPB_DEF_MSG, &upb_msgdef_vtbl,
                    owner)) {
    goto err2;
  }

  if (!upb_inttable_init(&m->itof, UPB_CTYPE_PTR)) goto err2;
  if (!upb_strtable_init(&m->ntof, UPB_CTYPE_PTR)) goto err1;
  m->map_entry = false;
  m->syntax = UPB_SYNTAX_PROTO2;
  return m;

err1:
  upb_inttable_uninit(&m->itof);
err2:
  upb_gfree(m);
  return NULL;
}

bool upb_msgdef_freeze(upb_msgdef *m, upb_status *status) {
  upb_def *d = upb_msgdef_upcast_mutable(m);
  return upb_def_freeze(&d, 1, status);
}

const char *upb_msgdef_fullname(const upb_msgdef *m) {
  return upb_def_fullname(upb_msgdef_upcast(m));
}

const char *upb_msgdef_name(const upb_msgdef *m) {
  return upb_def_name(upb_msgdef_upcast(m));
}

bool upb_msgdef_setfullname(upb_msgdef *m, const char *fullname,
                            upb_status *s) {
  return upb_def_setfullname(upb_msgdef_upcast_mutable(m), fullname, s);
}

bool upb_msgdef_setsyntax(upb_msgdef *m, upb_syntax_t syntax) {
  if (syntax != UPB_SYNTAX_PROTO2 && syntax != UPB_SYNTAX_PROTO3) {
    return false;
  }

  m->syntax = syntax;
  return true;
}

upb_syntax_t upb_msgdef_syntax(const upb_msgdef *m) {
  return m->syntax;
}

/* Helper: check that the field |f| is safe to add to msgdef |m|. Set an error
 * on status |s| and return false if not. */
static bool check_field_add(const upb_msgdef *m, const upb_fielddef *f,
                            upb_status *s) {
  if (upb_fielddef_containingtype(f) != NULL) {
    upb_status_seterrmsg(s, "fielddef already belongs to a message");
    return false;
  } else if (upb_fielddef_name(f) == NULL || upb_fielddef_number(f) == 0) {
    upb_status_seterrmsg(s, "field name or number were not set");
    return false;
  } else if (upb_msgdef_itof(m, upb_fielddef_number(f))) {
    upb_status_seterrmsg(s, "duplicate field number");
    return false;
  } else if (upb_strtable_lookup(&m->ntof, upb_fielddef_name(f), NULL)) {
    upb_status_seterrmsg(s, "name conflicts with existing field or oneof");
    return false;
  }
  return true;
}

static void add_field(upb_msgdef *m, upb_fielddef *f, const void *ref_donor) {
  release_containingtype(f);
  f->msg.def = m;
  f->msg_is_symbolic = false;
  upb_inttable_insert(&m->itof, upb_fielddef_number(f), upb_value_ptr(f));
  upb_strtable_insert(&m->ntof, upb_fielddef_name(f), upb_value_ptr(f));
  upb_ref2(f, m);
  upb_ref2(m, f);
  if (ref_donor) upb_fielddef_unref(f, ref_donor);
}

bool upb_msgdef_addfield(upb_msgdef *m, upb_fielddef *f, const void *ref_donor,
                         upb_status *s) {
  /* TODO: extensions need to have a separate namespace, because proto2 allows a
   * top-level extension (ie. one not in any package) to have the same name as a
   * field from the message.
   *
   * This also implies that there needs to be a separate lookup-by-name method
   * for extensions.  It seems desirable for iteration to return both extensions
   * and non-extensions though.
   *
   * We also need to validate that the field number is in an extension range iff
   * it is an extension.
   *
   * This method is idempotent. Check if |f| is already part of this msgdef and
   * return immediately if so. */
  if (upb_fielddef_containingtype(f) == m) {
    if (ref_donor) upb_fielddef_unref(f, ref_donor);
    return true;
  }

  /* Check constraints for all fields before performing any action. */
  if (!check_field_add(m, f, s)) {
    return false;
  } else if (upb_fielddef_containingoneof(f) != NULL) {
    /* Fields in a oneof can only be added by adding the oneof to the msgdef. */
    upb_status_seterrmsg(s, "fielddef is part of a oneof");
    return false;
  }

  /* Constraint checks ok, perform the action. */
  add_field(m, f, ref_donor);
  return true;
}

bool upb_msgdef_addoneof(upb_msgdef *m, upb_oneofdef *o, const void *ref_donor,
                         upb_status *s) {
  upb_oneof_iter it;

  /* Check various conditions that would prevent this oneof from being added. */
  if (upb_oneofdef_containingtype(o)) {
    upb_status_seterrmsg(s, "oneofdef already belongs to a message");
    return false;
  } else if (upb_oneofdef_name(o) == NULL) {
    upb_status_seterrmsg(s, "oneofdef name was not set");
    return false;
  } else if (upb_strtable_lookup(&m->ntof, upb_oneofdef_name(o), NULL)) {
    upb_status_seterrmsg(s, "name conflicts with existing field or oneof");
    return false;
  }

  /* Check that all of the oneof's fields do not conflict with names or numbers
   * of fields already in the message. */
  for (upb_oneof_begin(&it, o); !upb_oneof_done(&it); upb_oneof_next(&it)) {
    const upb_fielddef *f = upb_oneof_iter_field(&it);
    if (!check_field_add(m, f, s)) {
      return false;
    }
  }

  /* Everything checks out -- commit now. */

  /* Add oneof itself first. */
  o->parent = m;
  upb_strtable_insert(&m->ntof, upb_oneofdef_name(o), upb_value_ptr(o));
  upb_ref2(o, m);
  upb_ref2(m, o);

  /* Add each field of the oneof directly to the msgdef. */
  for (upb_oneof_begin(&it, o); !upb_oneof_done(&it); upb_oneof_next(&it)) {
    upb_fielddef *f = upb_oneof_iter_field(&it);
    add_field(m, f, NULL);
  }

  if (ref_donor) upb_oneofdef_unref(o, ref_donor);

  return true;
}

const upb_fielddef *upb_msgdef_itof(const upb_msgdef *m, uint32_t i) {
  upb_value val;
  return upb_inttable_lookup32(&m->itof, i, &val) ?
      upb_value_getptr(val) : NULL;
}

const upb_fielddef *upb_msgdef_ntof(const upb_msgdef *m, const char *name,
                                    size_t len) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return NULL;
  }

  return upb_trygetfield(upb_value_getptr(val));
}

const upb_oneofdef *upb_msgdef_ntoo(const upb_msgdef *m, const char *name,
                                    size_t len) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return NULL;
  }

  return upb_trygetoneof(upb_value_getptr(val));
}

bool upb_msgdef_lookupname(const upb_msgdef *m, const char *name, size_t len,
                           const upb_fielddef **f, const upb_oneofdef **o) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return false;
  }

  *o = upb_trygetoneof(upb_value_getptr(val));
  *f = upb_trygetfield(upb_value_getptr(val));
  UPB_ASSERT((*o != NULL) ^ (*f != NULL));  /* Exactly one of the two should be set. */
  return true;
}

int upb_msgdef_numfields(const upb_msgdef *m) {
  /* The number table contains only fields. */
  return upb_inttable_count(&m->itof);
}

int upb_msgdef_numoneofs(const upb_msgdef *m) {
  /* The name table includes oneofs, and the number table does not. */
  return upb_strtable_count(&m->ntof) - upb_inttable_count(&m->itof);
}

void upb_msgdef_setmapentry(upb_msgdef *m, bool map_entry) {
  UPB_ASSERT(!upb_msgdef_isfrozen(m));
  m->map_entry = map_entry;
}

bool upb_msgdef_mapentry(const upb_msgdef *m) {
  return m->map_entry;
}

void upb_msg_field_begin(upb_msg_field_iter *iter, const upb_msgdef *m) {
  upb_inttable_begin(iter, &m->itof);
}

void upb_msg_field_next(upb_msg_field_iter *iter) { upb_inttable_next(iter); }

bool upb_msg_field_done(const upb_msg_field_iter *iter) {
  return upb_inttable_done(iter);
}

upb_fielddef *upb_msg_iter_field(const upb_msg_field_iter *iter) {
  return (upb_fielddef*)upb_value_getptr(upb_inttable_iter_value(iter));
}

void upb_msg_field_iter_setdone(upb_msg_field_iter *iter) {
  upb_inttable_iter_setdone(iter);
}

void upb_msg_oneof_begin(upb_msg_oneof_iter *iter, const upb_msgdef *m) {
  upb_strtable_begin(iter, &m->ntof);
  /* We need to skip past any initial fields. */
  while (!upb_strtable_done(iter) &&
         !upb_isoneof(upb_value_getptr(upb_strtable_iter_value(iter)))) {
    upb_strtable_next(iter);
  }
}

void upb_msg_oneof_next(upb_msg_oneof_iter *iter) {
  /* We need to skip past fields to return only oneofs. */
  do {
    upb_strtable_next(iter);
  } while (!upb_strtable_done(iter) &&
           !upb_isoneof(upb_value_getptr(upb_strtable_iter_value(iter))));
}

bool upb_msg_oneof_done(const upb_msg_oneof_iter *iter) {
  return upb_strtable_done(iter);
}

upb_oneofdef *upb_msg_iter_oneof(const upb_msg_oneof_iter *iter) {
  return (upb_oneofdef*)upb_value_getptr(upb_strtable_iter_value(iter));
}

void upb_msg_oneof_iter_setdone(upb_msg_oneof_iter *iter) {
  upb_strtable_iter_setdone(iter);
}

/* upb_oneofdef ***************************************************************/

static void visitoneof(const upb_refcounted *r, upb_refcounted_visit *visit,
                       void *closure) {
  const upb_oneofdef *o = (const upb_oneofdef*)r;
  upb_oneof_iter i;
  for (upb_oneof_begin(&i, o); !upb_oneof_done(&i); upb_oneof_next(&i)) {
    const upb_fielddef *f = upb_oneof_iter_field(&i);
    visit(r, upb_fielddef_upcast2(f), closure);
  }
  if (o->parent) {
    visit(r, upb_msgdef_upcast2(o->parent), closure);
  }
}

static void freeoneof(upb_refcounted *r) {
  upb_oneofdef *o = (upb_oneofdef*)r;
  upb_strtable_uninit(&o->ntof);
  upb_inttable_uninit(&o->itof);
  upb_gfree((void*)o->name);
  upb_gfree(o);
}

const struct upb_refcounted_vtbl upb_oneofdef_vtbl = {visitoneof, freeoneof};

upb_oneofdef *upb_oneofdef_new(const void *owner) {
  upb_oneofdef *o = upb_gmalloc(sizeof(*o));

  if (!o) {
    return NULL;
  }

  o->parent = NULL;
  o->name = NULL;

  if (!upb_refcounted_init(upb_oneofdef_upcast_mutable(o), &upb_oneofdef_vtbl,
                           owner)) {
    goto err2;
  }

  if (!upb_inttable_init(&o->itof, UPB_CTYPE_PTR)) goto err2;
  if (!upb_strtable_init(&o->ntof, UPB_CTYPE_PTR)) goto err1;

  return o;

err1:
  upb_inttable_uninit(&o->itof);
err2:
  upb_gfree(o);
  return NULL;
}

const char *upb_oneofdef_name(const upb_oneofdef *o) { return o->name; }

bool upb_oneofdef_setname(upb_oneofdef *o, const char *name, upb_status *s) {
  UPB_ASSERT(!upb_oneofdef_isfrozen(o));
  if (upb_oneofdef_containingtype(o)) {
    upb_status_seterrmsg(s, "oneof already added to a message");
    return false;
  }

  if (!upb_isident(name, strlen(name), true, s)) {
    return false;
  }

  name = upb_gstrdup(name);
  if (!name) {
    upb_status_seterrmsg(s, "One of memory");
    return false;
  }

  upb_gfree((void*)o->name);
  o->name = name;
  return true;
}

const upb_msgdef *upb_oneofdef_containingtype(const upb_oneofdef *o) {
  return o->parent;
}

int upb_oneofdef_numfields(const upb_oneofdef *o) {
  return upb_strtable_count(&o->ntof);
}

uint32_t upb_oneofdef_index(const upb_oneofdef *o) {
  return o->index;
}

bool upb_oneofdef_addfield(upb_oneofdef *o, upb_fielddef *f,
                           const void *ref_donor,
                           upb_status *s) {
  UPB_ASSERT(!upb_oneofdef_isfrozen(o));
  UPB_ASSERT(!o->parent || !upb_msgdef_isfrozen(o->parent));

  /* This method is idempotent. Check if |f| is already part of this oneofdef
   * and return immediately if so. */
  if (upb_fielddef_containingoneof(f) == o) {
    return true;
  }

  /* The field must have an OPTIONAL label. */
  if (upb_fielddef_label(f) != UPB_LABEL_OPTIONAL) {
    upb_status_seterrmsg(s, "fields in oneof must have OPTIONAL label");
    return false;
  }

  /* Check that no field with this name or number exists already in the oneof.
   * Also check that the field is not already part of a oneof. */
  if (upb_fielddef_name(f) == NULL || upb_fielddef_number(f) == 0) {
    upb_status_seterrmsg(s, "field name or number were not set");
    return false;
  } else if (upb_oneofdef_itof(o, upb_fielddef_number(f)) ||
             upb_oneofdef_ntofz(o, upb_fielddef_name(f))) {
    upb_status_seterrmsg(s, "duplicate field name or number");
    return false;
  } else if (upb_fielddef_containingoneof(f) != NULL) {
    upb_status_seterrmsg(s, "fielddef already belongs to a oneof");
    return false;
  }

  /* We allow adding a field to the oneof either if the field is not part of a
   * msgdef, or if it is and we are also part of the same msgdef. */
  if (o->parent == NULL) {
    /* If we're not in a msgdef, the field cannot be either. Otherwise we would
     * need to magically add this oneof to a msgdef to remain consistent, which
     * is surprising behavior. */
    if (upb_fielddef_containingtype(f) != NULL) {
      upb_status_seterrmsg(s, "fielddef already belongs to a message, but "
                              "oneof does not");
      return false;
    }
  } else {
    /* If we're in a msgdef, the user can add fields that either aren't in any
     * msgdef (in which case they're added to our msgdef) or already a part of
     * our msgdef. */
    if (upb_fielddef_containingtype(f) != NULL &&
        upb_fielddef_containingtype(f) != o->parent) {
      upb_status_seterrmsg(s, "fielddef belongs to a different message "
                              "than oneof");
      return false;
    }
  }

  /* Commit phase. First add the field to our parent msgdef, if any, because
   * that may fail; then add the field to our own tables. */

  if (o->parent != NULL && upb_fielddef_containingtype(f) == NULL) {
    if (!upb_msgdef_addfield((upb_msgdef*)o->parent, f, NULL, s)) {
      return false;
    }
  }

  release_containingtype(f);
  f->oneof = o;
  upb_inttable_insert(&o->itof, upb_fielddef_number(f), upb_value_ptr(f));
  upb_strtable_insert(&o->ntof, upb_fielddef_name(f), upb_value_ptr(f));
  upb_ref2(f, o);
  upb_ref2(o, f);
  if (ref_donor) upb_fielddef_unref(f, ref_donor);

  return true;
}

const upb_fielddef *upb_oneofdef_ntof(const upb_oneofdef *o,
                                      const char *name, size_t length) {
  upb_value val;
  return upb_strtable_lookup2(&o->ntof, name, length, &val) ?
      upb_value_getptr(val) : NULL;
}

const upb_fielddef *upb_oneofdef_itof(const upb_oneofdef *o, uint32_t num) {
  upb_value val;
  return upb_inttable_lookup32(&o->itof, num, &val) ?
      upb_value_getptr(val) : NULL;
}

void upb_oneof_begin(upb_oneof_iter *iter, const upb_oneofdef *o) {
  upb_inttable_begin(iter, &o->itof);
}

void upb_oneof_next(upb_oneof_iter *iter) {
  upb_inttable_next(iter);
}

bool upb_oneof_done(upb_oneof_iter *iter) {
  return upb_inttable_done(iter);
}

upb_fielddef *upb_oneof_iter_field(const upb_oneof_iter *iter) {
  return (upb_fielddef*)upb_value_getptr(upb_inttable_iter_value(iter));
}

void upb_oneof_iter_setdone(upb_oneof_iter *iter) {
  upb_inttable_iter_setdone(iter);
}

/* upb_filedef ****************************************************************/

static void visitfiledef(const upb_refcounted *r, upb_refcounted_visit *visit,
                         void *closure) {
  const upb_filedef *f = (const upb_filedef*)r;
  size_t i;

  for(i = 0; i < upb_filedef_defcount(f); i++) {
    visit(r, upb_def_upcast(upb_filedef_def(f, i)), closure);
  }
}

static void freefiledef(upb_refcounted *r) {
  upb_filedef *f = (upb_filedef*)r;
  size_t i;

  for(i = 0; i < upb_filedef_depcount(f); i++) {
    upb_filedef_unref(upb_filedef_dep(f, i), f);
  }

  upb_inttable_uninit(&f->defs);
  upb_inttable_uninit(&f->deps);
  upb_gfree((void*)f->name);
  upb_gfree((void*)f->package);
  upb_gfree((void*)f->phpprefix);
  upb_gfree((void*)f->phpnamespace);
  upb_gfree(f);
}

const struct upb_refcounted_vtbl upb_filedef_vtbl = {visitfiledef, freefiledef};

upb_filedef *upb_filedef_new(const void *owner) {
  upb_filedef *f = upb_gmalloc(sizeof(*f));

  if (!f) {
    return NULL;
  }

  f->package = NULL;
  f->name = NULL;
  f->phpprefix = NULL;
  f->phpnamespace = NULL;
  f->syntax = UPB_SYNTAX_PROTO2;

  if (!upb_refcounted_init(upb_filedef_upcast_mutable(f), &upb_filedef_vtbl,
                           owner)) {
    goto err;
  }

  if (!upb_inttable_init(&f->defs, UPB_CTYPE_CONSTPTR)) {
    goto err;
  }

  if (!upb_inttable_init(&f->deps, UPB_CTYPE_CONSTPTR)) {
    goto err2;
  }

  return f;


err2:
  upb_inttable_uninit(&f->defs);

err:
  upb_gfree(f);
  return NULL;
}

const char *upb_filedef_name(const upb_filedef *f) {
  return f->name;
}

const char *upb_filedef_package(const upb_filedef *f) {
  return f->package;
}

const char *upb_filedef_phpprefix(const upb_filedef *f) {
  return f->phpprefix;
}

const char *upb_filedef_phpnamespace(const upb_filedef *f) {
  return f->phpnamespace;
}

upb_syntax_t upb_filedef_syntax(const upb_filedef *f) {
  return f->syntax;
}

size_t upb_filedef_defcount(const upb_filedef *f) {
  return upb_inttable_count(&f->defs);
}

size_t upb_filedef_depcount(const upb_filedef *f) {
  return upb_inttable_count(&f->deps);
}

const upb_def *upb_filedef_def(const upb_filedef *f, size_t i) {
  upb_value v;

  if (upb_inttable_lookup32(&f->defs, i, &v)) {
    return upb_value_getconstptr(v);
  } else {
    return NULL;
  }
}

const upb_filedef *upb_filedef_dep(const upb_filedef *f, size_t i) {
  upb_value v;

  if (upb_inttable_lookup32(&f->deps, i, &v)) {
    return upb_value_getconstptr(v);
  } else {
    return NULL;
  }
}

bool upb_filedef_setname(upb_filedef *f, const char *name, upb_status *s) {
  name = upb_gstrdup(name);
  if (!name) {
    upb_upberr_setoom(s);
    return false;
  }
  upb_gfree((void*)f->name);
  f->name = name;
  return true;
}

bool upb_filedef_setpackage(upb_filedef *f, const char *package,
                            upb_status *s) {
  if (!upb_isident(package, strlen(package), true, s)) return false;
  package = upb_gstrdup(package);
  if (!package) {
    upb_upberr_setoom(s);
    return false;
  }
  upb_gfree((void*)f->package);
  f->package = package;
  return true;
}

bool upb_filedef_setphpprefix(upb_filedef *f, const char *phpprefix,
                              upb_status *s) {
  phpprefix = upb_gstrdup(phpprefix);
  if (!phpprefix) {
    upb_upberr_setoom(s);
    return false;
  }
  upb_gfree((void*)f->phpprefix);
  f->phpprefix = phpprefix;
  return true;
}

bool upb_filedef_setphpnamespace(upb_filedef *f, const char *phpnamespace,
                                 upb_status *s) {
  phpnamespace = upb_gstrdup(phpnamespace);
  if (!phpnamespace) {
    upb_upberr_setoom(s);
    return false;
  }
  upb_gfree((void*)f->phpnamespace);
  f->phpnamespace = phpnamespace;
  return true;
}

bool upb_filedef_setsyntax(upb_filedef *f, upb_syntax_t syntax,
                           upb_status *s) {
  UPB_UNUSED(s);
  if (syntax != UPB_SYNTAX_PROTO2 &&
      syntax != UPB_SYNTAX_PROTO3) {
    upb_status_seterrmsg(s, "Unknown syntax value.");
    return false;
  }
  f->syntax = syntax;

  {
    /* Set all messages in this file to match. */
    size_t i;
    for (i = 0; i < upb_filedef_defcount(f); i++) {
      /* Casting const away is safe since all defs in mutable filedef must
       * also be mutable. */
      upb_def *def = (upb_def*)upb_filedef_def(f, i);

      upb_msgdef *m = upb_dyncast_msgdef_mutable(def);
      if (m) {
        m->syntax = syntax;
      }
    }
  }

  return true;
}

bool upb_filedef_adddef(upb_filedef *f, upb_def *def, const void *ref_donor,
                        upb_status *s) {
  if (def->file) {
    upb_status_seterrmsg(s, "Def is already part of another filedef.");
    return false;
  }

  if (upb_inttable_push(&f->defs, upb_value_constptr(def))) {
    def->file = f;
    upb_ref2(def, f);
    upb_ref2(f, def);
    if (ref_donor) upb_def_unref(def, ref_donor);
    if (def->type == UPB_DEF_MSG) {
      upb_downcast_msgdef_mutable(def)->syntax = f->syntax;
    }
    return true;
  } else {
    upb_upberr_setoom(s);
    return false;
  }
}

bool upb_filedef_adddep(upb_filedef *f, const upb_filedef *dep) {
  if (upb_inttable_push(&f->deps, upb_value_constptr(dep))) {
    /* Regular ref instead of ref2 because files can't form cycles. */
    upb_filedef_ref(dep, f);
    return true;
  } else {
    return false;
  }
}

void upb_symtab_free(upb_symtab *s) {
  upb_strtable_iter i;
  upb_strtable_begin(&i, &s->symtab);
  for (; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    const upb_def *def = upb_value_getptr(upb_strtable_iter_value(&i));
    upb_def_unref(def, s);
  }
  upb_strtable_uninit(&s->symtab);
  upb_gfree(s);
}

upb_symtab *upb_symtab_new() {
  upb_symtab *s = upb_gmalloc(sizeof(*s));
  if (!s) {
    return NULL;
  }

  upb_strtable_init(&s->symtab, UPB_CTYPE_PTR);
  return s;
}

const upb_def *upb_symtab_lookup(const upb_symtab *s, const char *sym) {
  upb_value v;
  upb_def *ret = upb_strtable_lookup(&s->symtab, sym, &v) ?
      upb_value_getptr(v) : NULL;
  return ret;
}

const upb_msgdef *upb_symtab_lookupmsg(const upb_symtab *s, const char *sym) {
  upb_value v;
  upb_def *def = upb_strtable_lookup(&s->symtab, sym, &v) ?
      upb_value_getptr(v) : NULL;
  return def ? upb_dyncast_msgdef(def) : NULL;
}

const upb_enumdef *upb_symtab_lookupenum(const upb_symtab *s, const char *sym) {
  upb_value v;
  upb_def *def = upb_strtable_lookup(&s->symtab, sym, &v) ?
      upb_value_getptr(v) : NULL;
  return def ? upb_dyncast_enumdef(def) : NULL;
}

/* Given a symbol and the base symbol inside which it is defined, find the
 * symbol's definition in t. */
static upb_def *upb_resolvename(const upb_strtable *t,
                                const char *base, const char *sym) {
  if(strlen(sym) == 0) return NULL;
  if(sym[0] == '.') {
    /* Symbols starting with '.' are absolute, so we do a single lookup.
     * Slice to omit the leading '.' */
    upb_value v;
    return upb_strtable_lookup(t, sym + 1, &v) ? upb_value_getptr(v) : NULL;
  } else {
    /* Remove components from base until we find an entry or run out.
     * TODO: This branch is totally broken, but currently not used. */
    (void)base;
    UPB_ASSERT(false);
    return NULL;
  }
}

const upb_def *upb_symtab_resolve(const upb_symtab *s, const char *base,
                                  const char *sym) {
  upb_def *ret = upb_resolvename(&s->symtab, base, sym);
  return ret;
}

/* TODO(haberman): we need a lot more testing of error conditions. */
static bool symtab_add(upb_symtab *s, upb_def *const*defs, size_t n,
                       void *ref_donor, upb_refcounted *freeze_also,
                       upb_status *status) {
  size_t i;
  size_t add_n;
  size_t freeze_n;
  upb_strtable_iter iter;
  upb_refcounted **add_objs = NULL;
  upb_def **add_defs = NULL;
  size_t add_objs_size;
  upb_strtable addtab;

  if (n == 0 && !freeze_also) {
    return true;
  }

  if (!upb_strtable_init(&addtab, UPB_CTYPE_PTR)) {
    upb_status_seterrmsg(status, "out of memory");
    return false;
  }

  /* Add new defs to our "add" set. */
  for (i = 0; i < n; i++) {
    upb_def *def = defs[i];
    const char *fullname;
    upb_fielddef *f;

    if (upb_def_isfrozen(def)) {
      upb_status_seterrmsg(status, "added defs must be mutable");
      goto err;
    }
    UPB_ASSERT(!upb_def_isfrozen(def));
    fullname = upb_def_fullname(def);
    if (!fullname) {
      upb_status_seterrmsg(
          status, "Anonymous defs cannot be added to a symtab");
      goto err;
    }

    f = upb_dyncast_fielddef_mutable(def);

    if (f) {
      if (!upb_fielddef_containingtypename(f)) {
        upb_status_seterrmsg(status,
                             "Standalone fielddefs must have a containing type "
                             "(extendee) name set");
        goto err;
      }
    } else {
      if (upb_strtable_lookup(&addtab, fullname, NULL)) {
        upb_status_seterrf(status, "Conflicting defs named '%s'", fullname);
        goto err;
      }
      if (upb_strtable_lookup(&s->symtab, fullname, NULL)) {
        upb_status_seterrf(status, "Symtab already has a def named '%s'",
                           fullname);
        goto err;
      }
      if (!upb_strtable_insert(&addtab, fullname, upb_value_ptr(def)))
        goto oom_err;
      upb_def_donateref(def, ref_donor, s);
    }

    if (upb_dyncast_fielddef_mutable(def)) {
      /* TODO(haberman): allow adding extensions attached to files. */
      upb_status_seterrf(status, "Can't add extensions to symtab.\n");
      goto err;
    }
  }

  /* Now using the table, resolve symbolic references for subdefs. */
  upb_strtable_begin(&iter, &addtab);
  for (; !upb_strtable_done(&iter); upb_strtable_next(&iter)) {
    const char *base;
    upb_def *def = upb_value_getptr(upb_strtable_iter_value(&iter));
    upb_msgdef *m = upb_dyncast_msgdef_mutable(def);
    upb_msg_field_iter j;

    if (!m) continue;
    /* Type names are resolved relative to the message in which they appear. */
    base = upb_msgdef_fullname(m);

    for(upb_msg_field_begin(&j, m);
        !upb_msg_field_done(&j);
        upb_msg_field_next(&j)) {
      upb_fielddef *f = upb_msg_iter_field(&j);
      const char *name = upb_fielddef_subdefname(f);
      if (name && !upb_fielddef_subdef(f)) {
        /* Try the lookup in the current set of to-be-added defs first. If not
         * there, try existing defs. */
        upb_def *subdef = upb_resolvename(&addtab, base, name);
        if (subdef == NULL) {
          subdef = upb_resolvename(&s->symtab, base, name);
        }
        if (subdef == NULL) {
          upb_status_seterrf(
              status, "couldn't resolve name '%s' in message '%s'", name, base);
          goto err;
        } else if (!upb_fielddef_setsubdef(f, subdef, status)) {
          goto err;
        }
      }
    }
  }

  /* We need an array of the defs in addtab, for passing to
   * upb_refcounted_freeze(). */
  add_objs_size = upb_strtable_count(&addtab);
  if (freeze_also) {
    add_objs_size++;
  }

  add_defs = upb_gmalloc(sizeof(void*) * add_objs_size);
  if (add_defs == NULL) goto oom_err;
  upb_strtable_begin(&iter, &addtab);
  for (add_n = 0; !upb_strtable_done(&iter); upb_strtable_next(&iter)) {
    add_defs[add_n++] = upb_value_getptr(upb_strtable_iter_value(&iter));
  }

  /* Validate defs. */
  if (!_upb_def_validate(add_defs, add_n, status)) {
    goto err;
  }

  /* Cheat a little and give the array a new type.
   * This is probably undefined behavior, but this code will be deleted soon. */
  add_objs = (upb_refcounted**)add_defs;

  freeze_n = add_n;
  if (freeze_also) {
    add_objs[freeze_n++] = freeze_also;
  }

  if (!upb_refcounted_freeze(add_objs, freeze_n, status,
                             UPB_MAX_MESSAGE_DEPTH * 2)) {
    goto err;
  }

  /* This must be delayed until all errors have been detected, since error
   * recovery code uses this table to cleanup defs. */
  upb_strtable_uninit(&addtab);

  /* TODO(haberman) we don't properly handle errors after this point (like
   * OOM in upb_strtable_insert() below). */
  for (i = 0; i < add_n; i++) {
    upb_def *def = (upb_def*)add_objs[i];
    const char *name = upb_def_fullname(def);
    bool success;
    success = upb_strtable_insert(&s->symtab, name, upb_value_ptr(def));
    UPB_ASSERT(success);
  }
  upb_gfree(add_defs);
  return true;

oom_err:
  upb_status_seterrmsg(status, "out of memory");
err: {
    /* We need to donate the refs back. */
    upb_strtable_begin(&iter, &addtab);
    for (; !upb_strtable_done(&iter); upb_strtable_next(&iter)) {
      upb_def *def = upb_value_getptr(upb_strtable_iter_value(&iter));
      upb_def_donateref(def, s, ref_donor);
    }
  }
  upb_strtable_uninit(&addtab);
  upb_gfree(add_defs);
  UPB_ASSERT(!upb_ok(status));
  return false;
}

bool upb_symtab_add(upb_symtab *s, upb_def *const*defs, size_t n,
                    void *ref_donor, upb_status *status) {
  return symtab_add(s, defs, n, ref_donor, NULL, status);
}

bool upb_symtab_addfile(upb_symtab *s, upb_filedef *file, upb_status *status) {
  size_t n;
  size_t i;
  upb_def **defs;
  bool ret;

  n = upb_filedef_defcount(file);
  if (n == 0) {
    return true;
  }
  defs = upb_gmalloc(sizeof(*defs) * n);

  if (defs == NULL) {
    upb_status_seterrmsg(status, "Out of memory");
    return false;
  }

  for (i = 0; i < n; i++) {
    defs[i] = upb_filedef_mutabledef(file, i);
  }

  ret = symtab_add(s, defs, n, NULL, upb_filedef_upcast_mutable(file), status);

  upb_gfree(defs);
  return ret;
}

/* Iteration. */

static void advance_to_matching(upb_symtab_iter *iter) {
  if (iter->type == UPB_DEF_ANY)
    return;

  while (!upb_strtable_done(&iter->iter) &&
         iter->type != upb_symtab_iter_def(iter)->type) {
    upb_strtable_next(&iter->iter);
  }
}

void upb_symtab_begin(upb_symtab_iter *iter, const upb_symtab *s,
                      upb_deftype_t type) {
  upb_strtable_begin(&iter->iter, &s->symtab);
  iter->type = type;
  advance_to_matching(iter);
}

void upb_symtab_next(upb_symtab_iter *iter) {
  upb_strtable_next(&iter->iter);
  advance_to_matching(iter);
}

bool upb_symtab_done(const upb_symtab_iter *iter) {
  return upb_strtable_done(&iter->iter);
}

const upb_def *upb_symtab_iter_def(const upb_symtab_iter *iter) {
  return upb_value_getptr(upb_strtable_iter_value(&iter->iter));
}
/* We encode backwards, to avoid pre-computing lengths (one-pass encode). */


#define UPB_PB_VARINT_MAX_LEN 10
#define CHK(x) do { if (!(x)) { return false; } } while(0)

/* Maps descriptor type -> upb field type.  */
static const uint8_t upb_desctype_to_fieldtype2[] = {
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

static size_t upb_encode_varint(uint64_t val, char *buf) {
  size_t i;
  if (val < 128) { buf[0] = val; return 1; }
  i = 0;
  while (val) {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    buf[i++] = byte;
  }
  return i;
}

static uint32_t upb_zzencode_32(int32_t n) { return (n << 1) ^ (n >> 31); }
static uint64_t upb_zzencode_64(int64_t n) { return (n << 1) ^ (n >> 63); }

typedef struct {
  upb_env *env;
  char *buf, *ptr, *limit;
} upb_encstate;

static size_t upb_roundup_pow2(size_t bytes) {
  size_t ret = 128;
  while (ret < bytes) {
    ret *= 2;
  }
  return ret;
}

static bool upb_encode_growbuffer(upb_encstate *e, size_t bytes) {
  size_t old_size = e->limit - e->buf;
  size_t new_size = upb_roundup_pow2(bytes + (e->limit - e->ptr));
  char *new_buf = upb_env_realloc(e->env, e->buf, old_size, new_size);
  CHK(new_buf);

  /* We want previous data at the end, realloc() put it at the beginning. */
  memmove(e->limit - old_size, e->buf, old_size);

  e->ptr = new_buf + new_size - (e->limit - e->ptr);
  e->limit = new_buf + new_size;
  e->buf = new_buf;
  return true;
}

/* Call to ensure that at least "bytes" bytes are available for writing at
 * e->ptr.  Returns false if the bytes could not be allocated. */
static bool upb_encode_reserve(upb_encstate *e, size_t bytes) {
  CHK(UPB_LIKELY((size_t)(e->ptr - e->buf) >= bytes) ||
      upb_encode_growbuffer(e, bytes));

  e->ptr -= bytes;
  return true;
}

/* Writes the given bytes to the buffer, handling reserve/advance. */
static bool upb_put_bytes(upb_encstate *e, const void *data, size_t len) {
  CHK(upb_encode_reserve(e, len));
  memcpy(e->ptr, data, len);
  return true;
}

static bool upb_put_fixed64(upb_encstate *e, uint64_t val) {
  /* TODO(haberman): byte-swap for big endian. */
  return upb_put_bytes(e, &val, sizeof(uint64_t));
}

static bool upb_put_fixed32(upb_encstate *e, uint32_t val) {
  /* TODO(haberman): byte-swap for big endian. */
  return upb_put_bytes(e, &val, sizeof(uint32_t));
}

static bool upb_put_varint(upb_encstate *e, uint64_t val) {
  size_t len;
  char *start;
  CHK(upb_encode_reserve(e, UPB_PB_VARINT_MAX_LEN));
  len = upb_encode_varint(val, e->ptr);
  start = e->ptr + UPB_PB_VARINT_MAX_LEN - len;
  memmove(start, e->ptr, len);
  e->ptr = start;
  return true;
}

static bool upb_put_double(upb_encstate *e, double d) {
  uint64_t u64;
  UPB_ASSERT(sizeof(double) == sizeof(uint64_t));
  memcpy(&u64, &d, sizeof(uint64_t));
  return upb_put_fixed64(e, u64);
}

static bool upb_put_float(upb_encstate *e, float d) {
  uint32_t u32;
  UPB_ASSERT(sizeof(float) == sizeof(uint32_t));
  memcpy(&u32, &d, sizeof(uint32_t));
  return upb_put_fixed32(e, u32);
}

static uint32_t upb_readcase(const char *msg, const upb_msglayout_msginit_v1 *m,
                             int oneof_index) {
  uint32_t ret;
  memcpy(&ret, msg + m->oneofs[oneof_index].case_offset, sizeof(ret));
  return ret;
}

static bool upb_readhasbit(const char *msg,
                           const upb_msglayout_fieldinit_v1 *f) {
  UPB_ASSERT(f->hasbit != UPB_NO_HASBIT);
  return msg[f->hasbit / 8] & (1 << (f->hasbit % 8));
}

static bool upb_put_tag(upb_encstate *e, int field_number, int wire_type) {
  return upb_put_varint(e, (field_number << 3) | wire_type);
}

static bool upb_put_fixedarray(upb_encstate *e, const upb_array *arr,
                               size_t size) {
  size_t bytes = arr->len * size;
  return upb_put_bytes(e, arr->data, bytes) && upb_put_varint(e, bytes);
}

bool upb_encode_message(upb_encstate *e, const char *msg,
                        const upb_msglayout_msginit_v1 *m,
                        size_t *size);

static bool upb_encode_array(upb_encstate *e, const char *field_mem,
                             const upb_msglayout_msginit_v1 *m,
                             const upb_msglayout_fieldinit_v1 *f) {
  const upb_array *arr = *(const upb_array**)field_mem;

  if (arr == NULL || arr->len == 0) {
    return true;
  }

  UPB_ASSERT(arr->type == upb_desctype_to_fieldtype2[f->type]);

#define VARINT_CASE(ctype, encode) { \
  ctype *start = arr->data; \
  ctype *ptr = start + arr->len; \
  size_t pre_len = e->limit - e->ptr; \
  do { \
    ptr--; \
    CHK(upb_put_varint(e, encode)); \
  } while (ptr != start); \
  CHK(upb_put_varint(e, e->limit - e->ptr - pre_len)); \
} \
break; \
do { ; } while(0)

  switch (f->type) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
      CHK(upb_put_fixedarray(e, arr, sizeof(double)));
      break;
    case UPB_DESCRIPTOR_TYPE_FLOAT:
      CHK(upb_put_fixedarray(e, arr, sizeof(float)));
      break;
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
      CHK(upb_put_fixedarray(e, arr, sizeof(uint64_t)));
      break;
    case UPB_DESCRIPTOR_TYPE_FIXED32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
      CHK(upb_put_fixedarray(e, arr, sizeof(uint32_t)));
      break;
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_UINT64:
      VARINT_CASE(uint64_t, *ptr);
    case UPB_DESCRIPTOR_TYPE_UINT32:
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_ENUM:
      VARINT_CASE(uint32_t, *ptr);
    case UPB_DESCRIPTOR_TYPE_BOOL:
      VARINT_CASE(bool, *ptr);
    case UPB_DESCRIPTOR_TYPE_SINT32:
      VARINT_CASE(int32_t, upb_zzencode_32(*ptr));
    case UPB_DESCRIPTOR_TYPE_SINT64:
      VARINT_CASE(int64_t, upb_zzencode_64(*ptr));
    case UPB_DESCRIPTOR_TYPE_STRING:
    case UPB_DESCRIPTOR_TYPE_BYTES: {
      upb_stringview *start = arr->data;
      upb_stringview *ptr = start + arr->len;
      do {
        ptr--;
        CHK(upb_put_bytes(e, ptr->data, ptr->size) &&
            upb_put_varint(e, ptr->size) &&
            upb_put_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED));
      } while (ptr != start);
      return true;
    }
    case UPB_DESCRIPTOR_TYPE_GROUP: {
      void **start = arr->data;
      void **ptr = start + arr->len;
      const upb_msglayout_msginit_v1 *subm = m->submsgs[f->submsg_index];
      do {
        size_t size;
        ptr--;
        CHK(upb_put_tag(e, f->number, UPB_WIRE_TYPE_END_GROUP) &&
            upb_encode_message(e, *ptr, subm, &size) &&
            upb_put_tag(e, f->number, UPB_WIRE_TYPE_START_GROUP));
      } while (ptr != start);
      return true;
    }
    case UPB_DESCRIPTOR_TYPE_MESSAGE: {
      void **start = arr->data;
      void **ptr = start + arr->len;
      const upb_msglayout_msginit_v1 *subm = m->submsgs[f->submsg_index];
      do {
        size_t size;
        ptr--;
        CHK(upb_encode_message(e, *ptr, subm, &size) &&
            upb_put_varint(e, size) &&
            upb_put_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED));
      } while (ptr != start);
      return true;
    }
  }
#undef VARINT_CASE

  /* We encode all primitive arrays as packed, regardless of what was specified
   * in the .proto file.  Could special case 1-sized arrays. */
  CHK(upb_put_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED));
  return true;
}

static bool upb_encode_scalarfield(upb_encstate *e, const char *field_mem,
                                   const upb_msglayout_msginit_v1 *m,
                                   const upb_msglayout_fieldinit_v1 *f,
                                   bool is_proto3) {
  bool skip_zero_value = is_proto3 && f->oneof_index == UPB_NOT_IN_ONEOF;

#define CASE(ctype, type, wire_type, encodeval) do { \
  ctype val = *(ctype*)field_mem; \
  if (skip_zero_value && val == 0) { \
    return true; \
  } \
  return upb_put_ ## type(e, encodeval) && \
      upb_put_tag(e, f->number, wire_type); \
} while(0)

  switch (f->type) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
      CASE(double, double, UPB_WIRE_TYPE_64BIT, val);
    case UPB_DESCRIPTOR_TYPE_FLOAT:
      CASE(float, float, UPB_WIRE_TYPE_32BIT, val);
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_UINT64:
      CASE(uint64_t, varint, UPB_WIRE_TYPE_VARINT, val);
    case UPB_DESCRIPTOR_TYPE_UINT32:
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_ENUM:
      CASE(uint32_t, varint, UPB_WIRE_TYPE_VARINT, val);
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
      CASE(uint64_t, fixed64, UPB_WIRE_TYPE_64BIT, val);
    case UPB_DESCRIPTOR_TYPE_FIXED32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
      CASE(uint32_t, fixed32, UPB_WIRE_TYPE_32BIT, val);
    case UPB_DESCRIPTOR_TYPE_BOOL:
      CASE(bool, varint, UPB_WIRE_TYPE_VARINT, val);
    case UPB_DESCRIPTOR_TYPE_SINT32:
      CASE(int32_t, varint, UPB_WIRE_TYPE_VARINT, upb_zzencode_32(val));
    case UPB_DESCRIPTOR_TYPE_SINT64:
      CASE(int64_t, varint, UPB_WIRE_TYPE_VARINT, upb_zzencode_64(val));
    case UPB_DESCRIPTOR_TYPE_STRING:
    case UPB_DESCRIPTOR_TYPE_BYTES: {
      upb_stringview view = *(upb_stringview*)field_mem;
      if (skip_zero_value && view.size == 0) {
        return true;
      }
      return upb_put_bytes(e, view.data, view.size) &&
          upb_put_varint(e, view.size) &&
          upb_put_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED);
    }
    case UPB_DESCRIPTOR_TYPE_GROUP: {
      size_t size;
      void *submsg = *(void**)field_mem;
      const upb_msglayout_msginit_v1 *subm = m->submsgs[f->submsg_index];
      if (skip_zero_value && submsg == NULL) {
        return true;
      }
      return upb_put_tag(e, f->number, UPB_WIRE_TYPE_END_GROUP) &&
          upb_encode_message(e, submsg, subm, &size) &&
          upb_put_tag(e, f->number, UPB_WIRE_TYPE_START_GROUP);
    }
    case UPB_DESCRIPTOR_TYPE_MESSAGE: {
      size_t size;
      void *submsg = *(void**)field_mem;
      const upb_msglayout_msginit_v1 *subm = m->submsgs[f->submsg_index];
      if (skip_zero_value && submsg == NULL) {
        return true;
      }
      return upb_encode_message(e, submsg, subm, &size) &&
          upb_put_varint(e, size) &&
          upb_put_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED);
    }
  }
#undef CASE
  UPB_UNREACHABLE();
}

bool upb_encode_hasscalarfield(const char *msg,
                               const upb_msglayout_msginit_v1 *m,
                               const upb_msglayout_fieldinit_v1 *f) {
  if (f->oneof_index != UPB_NOT_IN_ONEOF) {
    return upb_readcase(msg, m, f->oneof_index) == f->number;
  } else if (m->is_proto2) {
    return upb_readhasbit(msg, f);
  } else {
    /* For proto3, we'll test for the field being empty later. */
    return true;
  }
}

bool upb_encode_message(upb_encstate* e, const char *msg,
                        const upb_msglayout_msginit_v1 *m,
                        size_t *size) {
  int i;
  char *buf_end = e->ptr;

  if (msg == NULL) {
    return true;
  }

  for (i = m->field_count - 1; i >= 0; i--) {
    const upb_msglayout_fieldinit_v1 *f = &m->fields[i];

    if (f->label == UPB_LABEL_REPEATED) {
      CHK(upb_encode_array(e, msg + f->offset, m, f));
    } else {
      if (upb_encode_hasscalarfield(msg, m, f)) {
        CHK(upb_encode_scalarfield(e, msg + f->offset, m, f, !m->is_proto2));
      }
    }
  }

  *size = buf_end - e->ptr;
  return true;
}

char *upb_encode(const void *msg, const upb_msglayout_msginit_v1 *m,
                 upb_env *env, size_t *size) {
  upb_encstate e;
  e.env = env;
  e.buf = NULL;
  e.limit = NULL;
  e.ptr = NULL;

  if (!upb_encode_message(&e, msg, m, size)) {
    *size = 0;
    return NULL;
  }

  *size = e.limit - e.ptr;

  if (*size == 0) {
    static char ch;
    return &ch;
  } else {
    UPB_ASSERT(e.ptr);
    return e.ptr;
  }
}

#undef CHK
/*
** TODO(haberman): it's unclear whether a lot of the consistency checks should
** UPB_ASSERT() or return false.
*/


#include <string.h>


static void *upb_calloc(size_t size) {
  void *mem = upb_gmalloc(size);
  if (mem) {
    memset(mem, 0, size);
  }
  return mem;
}

/* Defined for the sole purpose of having a unique pointer value for
 * UPB_NO_CLOSURE. */
char _upb_noclosure;

static void freehandlers(upb_refcounted *r) {
  upb_handlers *h = (upb_handlers*)r;

  upb_inttable_iter i;
  upb_inttable_begin(&i, &h->cleanup_);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    void *val = (void*)upb_inttable_iter_key(&i);
    upb_value func_val = upb_inttable_iter_value(&i);
    upb_handlerfree *func = upb_value_getfptr(func_val);
    func(val);
  }

  upb_inttable_uninit(&h->cleanup_);
  upb_msgdef_unref(h->msg, h);
  upb_gfree(h->sub);
  upb_gfree(h);
}

static void visithandlers(const upb_refcounted *r, upb_refcounted_visit *visit,
                          void *closure) {
  const upb_handlers *h = (const upb_handlers*)r;
  upb_msg_field_iter i;
  for(upb_msg_field_begin(&i, h->msg);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    const upb_handlers *sub;
    if (!upb_fielddef_issubmsg(f)) continue;
    sub = upb_handlers_getsubhandlers(h, f);
    if (sub) visit(r, upb_handlers_upcast(sub), closure);
  }
}

static const struct upb_refcounted_vtbl vtbl = {visithandlers, freehandlers};

typedef struct {
  upb_inttable tab;  /* maps upb_msgdef* -> upb_handlers*. */
  upb_handlers_callback *callback;
  const void *closure;
} dfs_state;

/* TODO(haberman): discard upb_handlers* objects that do not actually have any
 * handlers set and cannot reach any upb_handlers* object that does.  This is
 * slightly tricky to do correctly. */
static upb_handlers *newformsg(const upb_msgdef *m, const void *owner,
                               dfs_state *s) {
  upb_msg_field_iter i;
  upb_handlers *h = upb_handlers_new(m, owner);
  if (!h) return NULL;
  if (!upb_inttable_insertptr(&s->tab, m, upb_value_ptr(h))) goto oom;

  s->callback(s->closure, h);

  /* For each submessage field, get or create a handlers object and set it as
   * the subhandlers. */
  for(upb_msg_field_begin(&i, m);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    const upb_msgdef *subdef;
    upb_value subm_ent;

    if (!upb_fielddef_issubmsg(f)) continue;

    subdef = upb_downcast_msgdef(upb_fielddef_subdef(f));
    if (upb_inttable_lookupptr(&s->tab, subdef, &subm_ent)) {
      upb_handlers_setsubhandlers(h, f, upb_value_getptr(subm_ent));
    } else {
      upb_handlers *sub_mh = newformsg(subdef, &sub_mh, s);
      if (!sub_mh) goto oom;
      upb_handlers_setsubhandlers(h, f, sub_mh);
      upb_handlers_unref(sub_mh, &sub_mh);
    }
  }
  return h;

oom:
  upb_handlers_unref(h, owner);
  return NULL;
}

/* Given a selector for a STARTSUBMSG handler, resolves to a pointer to the
 * subhandlers for this submessage field. */
#define SUBH(h, selector) (h->sub[selector])

/* The selector for a submessage field is the field index. */
#define SUBH_F(h, f) SUBH(h, f->index_)

static int32_t trygetsel(upb_handlers *h, const upb_fielddef *f,
                         upb_handlertype_t type) {
  upb_selector_t sel;
  UPB_ASSERT(!upb_handlers_isfrozen(h));
  if (upb_handlers_msgdef(h) != upb_fielddef_containingtype(f)) {
    upb_status_seterrf(
        &h->status_, "type mismatch: field %s does not belong to message %s",
        upb_fielddef_name(f), upb_msgdef_fullname(upb_handlers_msgdef(h)));
    return -1;
  }
  if (!upb_handlers_getselector(f, type, &sel)) {
    upb_status_seterrf(
        &h->status_,
        "type mismatch: cannot register handler type %d for field %s",
        type, upb_fielddef_name(f));
    return -1;
  }
  return sel;
}

static upb_selector_t handlers_getsel(upb_handlers *h, const upb_fielddef *f,
                             upb_handlertype_t type) {
  int32_t sel = trygetsel(h, f, type);
  UPB_ASSERT(sel >= 0);
  return sel;
}

static const void **returntype(upb_handlers *h, const upb_fielddef *f,
                               upb_handlertype_t type) {
  return &h->table[handlers_getsel(h, f, type)].attr.return_closure_type_;
}

static bool doset(upb_handlers *h, int32_t sel, const upb_fielddef *f,
                  upb_handlertype_t type, upb_func *func,
                  upb_handlerattr *attr) {
  upb_handlerattr set_attr = UPB_HANDLERATTR_INITIALIZER;
  const void *closure_type;
  const void **context_closure_type;

  UPB_ASSERT(!upb_handlers_isfrozen(h));

  if (sel < 0) {
    upb_status_seterrmsg(&h->status_,
                         "incorrect handler type for this field.");
    return false;
  }

  if (h->table[sel].func) {
    upb_status_seterrmsg(&h->status_,
                         "cannot change handler once it has been set.");
    return false;
  }

  if (attr) {
    set_attr = *attr;
  }

  /* Check that the given closure type matches the closure type that has been
   * established for this context (if any). */
  closure_type = upb_handlerattr_closuretype(&set_attr);

  if (type == UPB_HANDLER_STRING) {
    context_closure_type = returntype(h, f, UPB_HANDLER_STARTSTR);
  } else if (f && upb_fielddef_isseq(f) &&
             type != UPB_HANDLER_STARTSEQ &&
             type != UPB_HANDLER_ENDSEQ) {
    context_closure_type = returntype(h, f, UPB_HANDLER_STARTSEQ);
  } else {
    context_closure_type = &h->top_closure_type;
  }

  if (closure_type && *context_closure_type &&
      closure_type != *context_closure_type) {
    /* TODO(haberman): better message for debugging. */
    if (f) {
      upb_status_seterrf(&h->status_,
                         "closure type does not match for field %s",
                         upb_fielddef_name(f));
    } else {
      upb_status_seterrmsg(
          &h->status_, "closure type does not match for message-level handler");
    }
    return false;
  }

  if (closure_type)
    *context_closure_type = closure_type;

  /* If this is a STARTSEQ or STARTSTR handler, check that the returned pointer
   * matches any pre-existing expectations about what type is expected. */
  if (type == UPB_HANDLER_STARTSEQ || type == UPB_HANDLER_STARTSTR) {
    const void *return_type = upb_handlerattr_returnclosuretype(&set_attr);
    const void *table_return_type =
        upb_handlerattr_returnclosuretype(&h->table[sel].attr);
    if (return_type && table_return_type && return_type != table_return_type) {
      upb_status_seterrmsg(&h->status_, "closure return type does not match");
      return false;
    }

    if (table_return_type && !return_type)
      upb_handlerattr_setreturnclosuretype(&set_attr, table_return_type);
  }

  h->table[sel].func = (upb_func*)func;
  h->table[sel].attr = set_attr;
  return true;
}

/* Returns the effective closure type for this handler (which will propagate
 * from outer frames if this frame has no START* handler).  Not implemented for
 * UPB_HANDLER_STRING at the moment since this is not needed.  Returns NULL is
 * the effective closure type is unspecified (either no handler was registered
 * to specify it or the handler that was registered did not specify the closure
 * type). */
const void *effective_closure_type(upb_handlers *h, const upb_fielddef *f,
                                   upb_handlertype_t type) {
  const void *ret;
  upb_selector_t sel;

  UPB_ASSERT(type != UPB_HANDLER_STRING);
  ret = h->top_closure_type;

  if (upb_fielddef_isseq(f) &&
      type != UPB_HANDLER_STARTSEQ &&
      type != UPB_HANDLER_ENDSEQ &&
      h->table[sel = handlers_getsel(h, f, UPB_HANDLER_STARTSEQ)].func) {
    ret = upb_handlerattr_returnclosuretype(&h->table[sel].attr);
  }

  if (type == UPB_HANDLER_STRING &&
      h->table[sel = handlers_getsel(h, f, UPB_HANDLER_STARTSTR)].func) {
    ret = upb_handlerattr_returnclosuretype(&h->table[sel].attr);
  }

  /* The effective type of the submessage; not used yet.
   * if (type == SUBMESSAGE &&
   *     h->table[sel = handlers_getsel(h, f, UPB_HANDLER_STARTSUBMSG)].func) {
   *   ret = upb_handlerattr_returnclosuretype(&h->table[sel].attr);
   * } */

  return ret;
}

/* Checks whether the START* handler specified by f & type is missing even
 * though it is required to convert the established type of an outer frame
 * ("closure_type") into the established type of an inner frame (represented in
 * the return closure type of this handler's attr. */
bool checkstart(upb_handlers *h, const upb_fielddef *f, upb_handlertype_t type,
                upb_status *status) {
  const void *closure_type;
  const upb_handlerattr *attr;
  const void *return_closure_type;

  upb_selector_t sel = handlers_getsel(h, f, type);
  if (h->table[sel].func) return true;
  closure_type = effective_closure_type(h, f, type);
  attr = &h->table[sel].attr;
  return_closure_type = upb_handlerattr_returnclosuretype(attr);
  if (closure_type && return_closure_type &&
      closure_type != return_closure_type) {
    upb_status_seterrf(status,
                       "expected start handler to return sub type for field %f",
                       upb_fielddef_name(f));
    return false;
  }
  return true;
}

/* Public interface ***********************************************************/

upb_handlers *upb_handlers_new(const upb_msgdef *md, const void *owner) {
  int extra;
  upb_handlers *h;

  UPB_ASSERT(upb_msgdef_isfrozen(md));

  extra = sizeof(upb_handlers_tabent) * (md->selector_count - 1);
  h = upb_calloc(sizeof(*h) + extra);
  if (!h) return NULL;

  h->msg = md;
  upb_msgdef_ref(h->msg, h);
  upb_status_clear(&h->status_);

  if (md->submsg_field_count > 0) {
    h->sub = upb_calloc(md->submsg_field_count * sizeof(*h->sub));
    if (!h->sub) goto oom;
  } else {
    h->sub = 0;
  }

  if (!upb_refcounted_init(upb_handlers_upcast_mutable(h), &vtbl, owner))
    goto oom;
  if (!upb_inttable_init(&h->cleanup_, UPB_CTYPE_FPTR)) goto oom;

  /* calloc() above initialized all handlers to NULL. */
  return h;

oom:
  freehandlers(upb_handlers_upcast_mutable(h));
  return NULL;
}

const upb_handlers *upb_handlers_newfrozen(const upb_msgdef *m,
                                           const void *owner,
                                           upb_handlers_callback *callback,
                                           const void *closure) {
  dfs_state state;
  upb_handlers *ret;
  bool ok;
  upb_refcounted *r;

  state.callback = callback;
  state.closure = closure;
  if (!upb_inttable_init(&state.tab, UPB_CTYPE_PTR)) return NULL;

  ret = newformsg(m, owner, &state);

  upb_inttable_uninit(&state.tab);
  if (!ret) return NULL;

  r = upb_handlers_upcast_mutable(ret);
  ok = upb_refcounted_freeze(&r, 1, NULL, UPB_MAX_HANDLER_DEPTH);
  UPB_ASSERT(ok);

  return ret;
}

const upb_status *upb_handlers_status(upb_handlers *h) {
  UPB_ASSERT(!upb_handlers_isfrozen(h));
  return &h->status_;
}

void upb_handlers_clearerr(upb_handlers *h) {
  UPB_ASSERT(!upb_handlers_isfrozen(h));
  upb_status_clear(&h->status_);
}

#define SETTER(name, handlerctype, handlertype) \
  bool upb_handlers_set ## name(upb_handlers *h, const upb_fielddef *f, \
                                handlerctype func, upb_handlerattr *attr) { \
    int32_t sel = trygetsel(h, f, handlertype); \
    return doset(h, sel, f, handlertype, (upb_func*)func, attr); \
  }

SETTER(int32,       upb_int32_handlerfunc*,       UPB_HANDLER_INT32)
SETTER(int64,       upb_int64_handlerfunc*,       UPB_HANDLER_INT64)
SETTER(uint32,      upb_uint32_handlerfunc*,      UPB_HANDLER_UINT32)
SETTER(uint64,      upb_uint64_handlerfunc*,      UPB_HANDLER_UINT64)
SETTER(float,       upb_float_handlerfunc*,       UPB_HANDLER_FLOAT)
SETTER(double,      upb_double_handlerfunc*,      UPB_HANDLER_DOUBLE)
SETTER(bool,        upb_bool_handlerfunc*,        UPB_HANDLER_BOOL)
SETTER(startstr,    upb_startstr_handlerfunc*,    UPB_HANDLER_STARTSTR)
SETTER(string,      upb_string_handlerfunc*,      UPB_HANDLER_STRING)
SETTER(endstr,      upb_endfield_handlerfunc*,    UPB_HANDLER_ENDSTR)
SETTER(startseq,    upb_startfield_handlerfunc*,  UPB_HANDLER_STARTSEQ)
SETTER(startsubmsg, upb_startfield_handlerfunc*,  UPB_HANDLER_STARTSUBMSG)
SETTER(endsubmsg,   upb_endfield_handlerfunc*,    UPB_HANDLER_ENDSUBMSG)
SETTER(endseq,      upb_endfield_handlerfunc*,    UPB_HANDLER_ENDSEQ)

#undef SETTER

bool upb_handlers_setunknown(upb_handlers *h, upb_unknown_handlerfunc *func,
                             upb_handlerattr *attr) {
  return doset(h, UPB_UNKNOWN_SELECTOR, NULL, UPB_HANDLER_INT32,
               (upb_func *)func, attr);
}

bool upb_handlers_setstartmsg(upb_handlers *h, upb_startmsg_handlerfunc *func,
                              upb_handlerattr *attr) {
  return doset(h, UPB_STARTMSG_SELECTOR, NULL, UPB_HANDLER_INT32,
               (upb_func *)func, attr);
}

bool upb_handlers_setendmsg(upb_handlers *h, upb_endmsg_handlerfunc *func,
                            upb_handlerattr *attr) {
  UPB_ASSERT(!upb_handlers_isfrozen(h));
  return doset(h, UPB_ENDMSG_SELECTOR, NULL, UPB_HANDLER_INT32,
               (upb_func *)func, attr);
}

bool upb_handlers_setsubhandlers(upb_handlers *h, const upb_fielddef *f,
                                 const upb_handlers *sub) {
  UPB_ASSERT(sub);
  UPB_ASSERT(!upb_handlers_isfrozen(h));
  UPB_ASSERT(upb_fielddef_issubmsg(f));
  if (SUBH_F(h, f)) return false;  /* Can't reset. */
  if (upb_msgdef_upcast(upb_handlers_msgdef(sub)) != upb_fielddef_subdef(f)) {
    return false;
  }
  SUBH_F(h, f) = sub;
  upb_ref2(sub, h);
  return true;
}

const upb_handlers *upb_handlers_getsubhandlers(const upb_handlers *h,
                                                const upb_fielddef *f) {
  UPB_ASSERT(upb_fielddef_issubmsg(f));
  return SUBH_F(h, f);
}

bool upb_handlers_getattr(const upb_handlers *h, upb_selector_t sel,
                          upb_handlerattr *attr) {
  if (!upb_handlers_gethandler(h, sel))
    return false;
  *attr = h->table[sel].attr;
  return true;
}

const upb_handlers *upb_handlers_getsubhandlers_sel(const upb_handlers *h,
                                                    upb_selector_t sel) {
  /* STARTSUBMSG selector in sel is the field's selector base. */
  return SUBH(h, sel - UPB_STATIC_SELECTOR_COUNT);
}

const upb_msgdef *upb_handlers_msgdef(const upb_handlers *h) { return h->msg; }

bool upb_handlers_addcleanup(upb_handlers *h, void *p, upb_handlerfree *func) {
  bool ok;
  if (upb_inttable_lookupptr(&h->cleanup_, p, NULL)) {
    return false;
  }
  ok = upb_inttable_insertptr(&h->cleanup_, p, upb_value_fptr(func));
  UPB_ASSERT(ok);
  return true;
}


/* "Static" methods ***********************************************************/

bool upb_handlers_freeze(upb_handlers *const*handlers, int n, upb_status *s) {
  /* TODO: verify we have a transitive closure. */
  int i;
  for (i = 0; i < n; i++) {
    upb_msg_field_iter j;
    upb_handlers *h = handlers[i];

    if (!upb_ok(&h->status_)) {
      upb_status_seterrf(s, "handlers for message %s had error status: %s",
                         upb_msgdef_fullname(upb_handlers_msgdef(h)),
                         upb_status_errmsg(&h->status_));
      return false;
    }

    /* Check that there are no closure mismatches due to missing Start* handlers
     * or subhandlers with different type-level types. */
    for(upb_msg_field_begin(&j, h->msg);
        !upb_msg_field_done(&j);
        upb_msg_field_next(&j)) {

      const upb_fielddef *f = upb_msg_iter_field(&j);
      if (upb_fielddef_isseq(f)) {
        if (!checkstart(h, f, UPB_HANDLER_STARTSEQ, s))
          return false;
      }

      if (upb_fielddef_isstring(f)) {
        if (!checkstart(h, f, UPB_HANDLER_STARTSTR, s))
          return false;
      }

      if (upb_fielddef_issubmsg(f)) {
        bool hashandler = false;
        if (upb_handlers_gethandler(
                h, handlers_getsel(h, f, UPB_HANDLER_STARTSUBMSG)) ||
            upb_handlers_gethandler(
                h, handlers_getsel(h, f, UPB_HANDLER_ENDSUBMSG))) {
          hashandler = true;
        }

        if (upb_fielddef_isseq(f) &&
            (upb_handlers_gethandler(
                 h, handlers_getsel(h, f, UPB_HANDLER_STARTSEQ)) ||
             upb_handlers_gethandler(
                 h, handlers_getsel(h, f, UPB_HANDLER_ENDSEQ)))) {
          hashandler = true;
        }

        if (hashandler && !upb_handlers_getsubhandlers(h, f)) {
          /* For now we add an empty subhandlers in this case.  It makes the
           * decoder code generator simpler, because it only has to handle two
           * cases (submessage has handlers or not) as opposed to three
           * (submessage has handlers in enclosing message but no subhandlers).
           *
           * This makes parsing less efficient in the case that we want to
           * notice a submessage but skip its contents (like if we're testing
           * for submessage presence or counting the number of repeated
           * submessages).  In this case we will end up parsing the submessage
           * field by field and throwing away the results for each, instead of
           * skipping the whole delimited thing at once.  If this is an issue we
           * can revisit it, but do remember that this only arises when you have
           * handlers (startseq/startsubmsg/endsubmsg/endseq) set for the
           * submessage but no subhandlers.  The uses cases for this are
           * limited. */
          upb_handlers *sub = upb_handlers_new(upb_fielddef_msgsubdef(f), &sub);
          upb_handlers_setsubhandlers(h, f, sub);
          upb_handlers_unref(sub, &sub);
        }

        /* TODO(haberman): check type of submessage.
         * This is slightly tricky; also consider whether we should check that
         * they match at setsubhandlers time. */
      }
    }
  }

  if (!upb_refcounted_freeze((upb_refcounted*const*)handlers, n, s,
                             UPB_MAX_HANDLER_DEPTH)) {
    return false;
  }

  return true;
}

upb_handlertype_t upb_handlers_getprimitivehandlertype(const upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_ENUM: return UPB_HANDLER_INT32;
    case UPB_TYPE_INT64: return UPB_HANDLER_INT64;
    case UPB_TYPE_UINT32: return UPB_HANDLER_UINT32;
    case UPB_TYPE_UINT64: return UPB_HANDLER_UINT64;
    case UPB_TYPE_FLOAT: return UPB_HANDLER_FLOAT;
    case UPB_TYPE_DOUBLE: return UPB_HANDLER_DOUBLE;
    case UPB_TYPE_BOOL: return UPB_HANDLER_BOOL;
    default: UPB_ASSERT(false); return -1;  /* Invalid input. */
  }
}

bool upb_handlers_getselector(const upb_fielddef *f, upb_handlertype_t type,
                              upb_selector_t *s) {
  switch (type) {
    case UPB_HANDLER_INT32:
    case UPB_HANDLER_INT64:
    case UPB_HANDLER_UINT32:
    case UPB_HANDLER_UINT64:
    case UPB_HANDLER_FLOAT:
    case UPB_HANDLER_DOUBLE:
    case UPB_HANDLER_BOOL:
      if (!upb_fielddef_isprimitive(f) ||
          upb_handlers_getprimitivehandlertype(f) != type)
        return false;
      *s = f->selector_base;
      break;
    case UPB_HANDLER_STRING:
      if (upb_fielddef_isstring(f)) {
        *s = f->selector_base;
      } else if (upb_fielddef_lazy(f)) {
        *s = f->selector_base + 3;
      } else {
        return false;
      }
      break;
    case UPB_HANDLER_STARTSTR:
      if (upb_fielddef_isstring(f) || upb_fielddef_lazy(f)) {
        *s = f->selector_base + 1;
      } else {
        return false;
      }
      break;
    case UPB_HANDLER_ENDSTR:
      if (upb_fielddef_isstring(f) || upb_fielddef_lazy(f)) {
        *s = f->selector_base + 2;
      } else {
        return false;
      }
      break;
    case UPB_HANDLER_STARTSEQ:
      if (!upb_fielddef_isseq(f)) return false;
      *s = f->selector_base - 2;
      break;
    case UPB_HANDLER_ENDSEQ:
      if (!upb_fielddef_isseq(f)) return false;
      *s = f->selector_base - 1;
      break;
    case UPB_HANDLER_STARTSUBMSG:
      if (!upb_fielddef_issubmsg(f)) return false;
      /* Selectors for STARTSUBMSG are at the beginning of the table so that the
       * selector can also be used as an index into the "sub" array of
       * subhandlers.  The indexes for the two into these two tables are the
       * same, except that in the handler table the static selectors come first. */
      *s = f->index_ + UPB_STATIC_SELECTOR_COUNT;
      break;
    case UPB_HANDLER_ENDSUBMSG:
      if (!upb_fielddef_issubmsg(f)) return false;
      *s = f->selector_base;
      break;
  }
  UPB_ASSERT((size_t)*s < upb_fielddef_containingtype(f)->selector_count);
  return true;
}

uint32_t upb_handlers_selectorbaseoffset(const upb_fielddef *f) {
  return upb_fielddef_isseq(f) ? 2 : 0;
}

uint32_t upb_handlers_selectorcount(const upb_fielddef *f) {
  uint32_t ret = 1;
  if (upb_fielddef_isseq(f)) ret += 2;    /* STARTSEQ/ENDSEQ */
  if (upb_fielddef_isstring(f)) ret += 2; /* [STRING]/STARTSTR/ENDSTR */
  if (upb_fielddef_issubmsg(f)) {
    /* ENDSUBMSG (STARTSUBMSG is at table beginning) */
    ret += 0;
    if (upb_fielddef_lazy(f)) {
      /* STARTSTR/ENDSTR/STRING (for lazy) */
      ret += 3;
    }
  }
  return ret;
}


/* upb_handlerattr ************************************************************/

void upb_handlerattr_init(upb_handlerattr *attr) {
  upb_handlerattr from = UPB_HANDLERATTR_INITIALIZER;
  memcpy(attr, &from, sizeof(*attr));
}

void upb_handlerattr_uninit(upb_handlerattr *attr) {
  UPB_UNUSED(attr);
}

bool upb_handlerattr_sethandlerdata(upb_handlerattr *attr, const void *hd) {
  attr->handler_data_ = hd;
  return true;
}

bool upb_handlerattr_setclosuretype(upb_handlerattr *attr, const void *type) {
  attr->closure_type_ = type;
  return true;
}

const void *upb_handlerattr_closuretype(const upb_handlerattr *attr) {
  return attr->closure_type_;
}

bool upb_handlerattr_setreturnclosuretype(upb_handlerattr *attr,
                                          const void *type) {
  attr->return_closure_type_ = type;
  return true;
}

const void *upb_handlerattr_returnclosuretype(const upb_handlerattr *attr) {
  return attr->return_closure_type_;
}

bool upb_handlerattr_setalwaysok(upb_handlerattr *attr, bool alwaysok) {
  attr->alwaysok_ = alwaysok;
  return true;
}

bool upb_handlerattr_alwaysok(const upb_handlerattr *attr) {
  return attr->alwaysok_;
}

/* upb_bufhandle **************************************************************/

size_t upb_bufhandle_objofs(const upb_bufhandle *h) {
  return h->objofs_;
}

/* upb_byteshandler ***********************************************************/

void upb_byteshandler_init(upb_byteshandler* h) {
  memset(h, 0, sizeof(*h));
}

/* For when we support handlerfree callbacks. */
void upb_byteshandler_uninit(upb_byteshandler* h) {
  UPB_UNUSED(h);
}

bool upb_byteshandler_setstartstr(upb_byteshandler *h,
                                  upb_startstr_handlerfunc *func, void *d) {
  h->table[UPB_STARTSTR_SELECTOR].func = (upb_func*)func;
  h->table[UPB_STARTSTR_SELECTOR].attr.handler_data_ = d;
  return true;
}

bool upb_byteshandler_setstring(upb_byteshandler *h,
                                upb_string_handlerfunc *func, void *d) {
  h->table[UPB_STRING_SELECTOR].func = (upb_func*)func;
  h->table[UPB_STRING_SELECTOR].attr.handler_data_ = d;
  return true;
}

bool upb_byteshandler_setendstr(upb_byteshandler *h,
                                upb_endfield_handlerfunc *func, void *d) {
  h->table[UPB_ENDSTR_SELECTOR].func = (upb_func*)func;
  h->table[UPB_ENDSTR_SELECTOR].attr.handler_data_ = d;
  return true;
}


static bool is_power_of_two(size_t val) {
  return (val & (val - 1)) == 0;
}

/* Align up to the given power of 2. */
static size_t align_up(size_t val, size_t align) {
  UPB_ASSERT(is_power_of_two(align));
  return (val + align - 1) & ~(align - 1);
}

static size_t div_round_up(size_t n, size_t d) {
  return (n + d - 1) / d;
}

bool upb_fieldtype_mapkeyok(upb_fieldtype_t type) {
  return type == UPB_TYPE_BOOL || type == UPB_TYPE_INT32 ||
         type == UPB_TYPE_UINT32 || type == UPB_TYPE_INT64 ||
         type == UPB_TYPE_UINT64 || type == UPB_TYPE_STRING;
}

void *upb_array_pack(const upb_array *arr, void *p, size_t *ofs, size_t size);
void *upb_map_pack(const upb_map *map, void *p, size_t *ofs, size_t size);

#define PTR_AT(msg, ofs, type) (type*)((char*)msg + ofs)
#define VOIDPTR_AT(msg, ofs) PTR_AT(msg, ofs, void)
#define ENCODE_MAX_NESTING 64
#define CHECK_TRUE(x) if (!(x)) { return false; }

/** upb_msgval ****************************************************************/

#define upb_alignof(t) offsetof(struct { char c; t x; }, x)

/* These functions will generate real memcpy() calls on ARM sadly, because
 * the compiler assumes they might not be aligned. */

static upb_msgval upb_msgval_read(const void *p, size_t ofs,
                                  uint8_t size) {
  upb_msgval val;
  p = (char*)p + ofs;
  memcpy(&val, p, size);
  return val;
}

static void upb_msgval_write(void *p, size_t ofs, upb_msgval val,
                             uint8_t size) {
  p = (char*)p + ofs;
  memcpy(p, &val, size);
}

static size_t upb_msgval_sizeof(upb_fieldtype_t type) {
  switch (type) {
    case UPB_TYPE_DOUBLE:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT64:
      return 8;
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_FLOAT:
      return 4;
    case UPB_TYPE_BOOL:
      return 1;
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE:
      return sizeof(void*);
    case UPB_TYPE_STRING:
      return sizeof(upb_stringview);
  }
  UPB_UNREACHABLE();
}

static uint8_t upb_msg_fieldsize(const upb_msglayout_fieldinit_v1 *field) {
  if (field->label == UPB_LABEL_REPEATED) {
    return sizeof(void*);
  } else {
    return upb_msgval_sizeof(field->type);
  }
}

static uint8_t upb_msg_fielddefsize(const upb_fielddef *f) {
  if (upb_fielddef_isseq(f)) {
    return sizeof(void*);
  } else {
    return upb_msgval_sizeof(upb_fielddef_type(f));
  }
}

/* TODO(haberman): this is broken right now because upb_msgval can contain
 * a char* / size_t pair, which is too big for a upb_value.  To fix this
 * we'll probably need to dynamically allocate a upb_msgval and store a
 * pointer to that in the tables for extensions/maps. */
static upb_value upb_toval(upb_msgval val) {
  upb_value ret;
  UPB_UNUSED(val);
  memset(&ret, 0, sizeof(upb_value));  /* XXX */
  return ret;
}

static upb_msgval upb_msgval_fromval(upb_value val) {
  upb_msgval ret;
  UPB_UNUSED(val);
  memset(&ret, 0, sizeof(upb_msgval));  /* XXX */
  return ret;
}

static upb_ctype_t upb_fieldtotabtype(upb_fieldtype_t type) {
  switch (type) {
    case UPB_TYPE_FLOAT: return UPB_CTYPE_FLOAT;
    case UPB_TYPE_DOUBLE: return UPB_CTYPE_DOUBLE;
    case UPB_TYPE_BOOL: return UPB_CTYPE_BOOL;
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE:
    case UPB_TYPE_STRING: return UPB_CTYPE_CONSTPTR;
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32: return UPB_CTYPE_INT32;
    case UPB_TYPE_UINT32: return UPB_CTYPE_UINT32;
    case UPB_TYPE_INT64: return UPB_CTYPE_INT64;
    case UPB_TYPE_UINT64: return UPB_CTYPE_UINT64;
    default: UPB_ASSERT(false); return 0;
  }
}

static upb_msgval upb_msgval_fromdefault(const upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
      case UPB_TYPE_FLOAT:
        return upb_msgval_float(upb_fielddef_defaultfloat(f));
      case UPB_TYPE_DOUBLE:
        return upb_msgval_double(upb_fielddef_defaultdouble(f));
      case UPB_TYPE_BOOL:
        return upb_msgval_bool(upb_fielddef_defaultbool(f));
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES: {
        size_t len;
        const char *ptr = upb_fielddef_defaultstr(f, &len);
        return upb_msgval_makestr(ptr, len);
      }
      case UPB_TYPE_MESSAGE:
        return upb_msgval_msg(NULL);
      case UPB_TYPE_ENUM:
      case UPB_TYPE_INT32:
        return upb_msgval_int32(upb_fielddef_defaultint32(f));
      case UPB_TYPE_UINT32:
        return upb_msgval_uint32(upb_fielddef_defaultuint32(f));
      case UPB_TYPE_INT64:
        return upb_msgval_int64(upb_fielddef_defaultint64(f));
      case UPB_TYPE_UINT64:
        return upb_msgval_uint64(upb_fielddef_defaultuint64(f));
      default:
        UPB_ASSERT(false);
        return upb_msgval_msg(NULL);
  }
}


/** upb_msglayout *************************************************************/

struct upb_msglayout {
  struct upb_msglayout_msginit_v1 data;
};

static void upb_msglayout_free(upb_msglayout *l) {
  upb_gfree(l->data.default_msg);
  upb_gfree(l);
}

static size_t upb_msglayout_place(upb_msglayout *l, size_t size) {
  size_t ret;

  l->data.size = align_up(l->data.size, size);
  ret = l->data.size;
  l->data.size += size;
  return ret;
}

static uint32_t upb_msglayout_offset(const upb_msglayout *l,
                                     const upb_fielddef *f) {
  return l->data.fields[upb_fielddef_index(f)].offset;
}

static uint32_t upb_msglayout_hasbit(const upb_msglayout *l,
                                     const upb_fielddef *f) {
  return l->data.fields[upb_fielddef_index(f)].hasbit;
}

static bool upb_msglayout_initdefault(upb_msglayout *l, const upb_msgdef *m) {
  upb_msg_field_iter it;

  if (upb_msgdef_syntax(m) == UPB_SYNTAX_PROTO2 && l->data.size) {
    /* Allocate default message and set default values in it. */
    l->data.default_msg = upb_gmalloc(l->data.size);
    if (!l->data.default_msg) {
      return false;
    }

    memset(l->data.default_msg, 0, l->data.size);

    for (upb_msg_field_begin(&it, m); !upb_msg_field_done(&it);
         upb_msg_field_next(&it)) {
      const upb_fielddef* f = upb_msg_iter_field(&it);

      if (upb_fielddef_containingoneof(f)) {
        continue;
      }

      /* TODO(haberman): handle strings. */
      if (!upb_fielddef_isstring(f) &&
          !upb_fielddef_issubmsg(f) &&
          !upb_fielddef_isseq(f)) {
        upb_msg_set(l->data.default_msg,
                    upb_fielddef_index(f),
                    upb_msgval_fromdefault(f),
                    l);
      }
    }
  }

  return true;
}

static upb_msglayout *upb_msglayout_new(const upb_msgdef *m) {
  upb_msg_field_iter it;
  upb_msg_oneof_iter oit;
  upb_msglayout *l;
  size_t hasbit;
  size_t submsg_count = 0;
  const upb_msglayout_msginit_v1 **submsgs;
  upb_msglayout_fieldinit_v1 *fields;
  upb_msglayout_oneofinit_v1 *oneofs;

  for (upb_msg_field_begin(&it, m);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* f = upb_msg_iter_field(&it);
    if (upb_fielddef_issubmsg(f)) {
      submsg_count++;
    }
  }

  l = upb_gmalloc(sizeof(*l));
  if (!l) return NULL;

  memset(l, 0, sizeof(*l));

  fields = upb_gmalloc(upb_msgdef_numfields(m) * sizeof(*fields));
  submsgs = upb_gmalloc(submsg_count * sizeof(*submsgs));
  oneofs = upb_gmalloc(upb_msgdef_numoneofs(m) * sizeof(*oneofs));

  if ((!fields && upb_msgdef_numfields(m)) ||
      (!submsgs && submsg_count) ||
      (!oneofs && upb_msgdef_numoneofs(m))) {
    /* OOM. */
    upb_gfree(l);
    upb_gfree(fields);
    upb_gfree(submsgs);
    upb_gfree(oneofs);
    return NULL;
  }

  l->data.field_count = upb_msgdef_numfields(m);
  l->data.oneof_count = upb_msgdef_numoneofs(m);
  l->data.fields = fields;
  l->data.submsgs = submsgs;
  l->data.oneofs = oneofs;
  l->data.is_proto2 = (upb_msgdef_syntax(m) == UPB_SYNTAX_PROTO2);

  /* Allocate data offsets in three stages:
   *
   * 1. hasbits.
   * 2. regular fields.
   * 3. oneof fields.
   *
   * OPT: There is a lot of room for optimization here to minimize the size.
   */

  /* Allocate hasbits and set basic field attributes. */
  for (upb_msg_field_begin(&it, m), hasbit = 0;
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* f = upb_msg_iter_field(&it);
    upb_msglayout_fieldinit_v1 *field = &fields[upb_fielddef_index(f)];

    field->number = upb_fielddef_number(f);
    field->type = upb_fielddef_type(f);
    field->label = upb_fielddef_label(f);

    if (upb_fielddef_containingoneof(f)) {
      field->oneof_index = upb_oneofdef_index(upb_fielddef_containingoneof(f));
    } else {
      field->oneof_index = UPB_NOT_IN_ONEOF;
    }

    if (upb_fielddef_haspresence(f) && !upb_fielddef_containingoneof(f)) {
      field->hasbit = hasbit++;
    }
  }

  /* Account for space used by hasbits. */
  l->data.size = div_round_up(hasbit, 8);

  /* Allocate non-oneof fields. */
  for (upb_msg_field_begin(&it, m); !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* f = upb_msg_iter_field(&it);
    size_t field_size = upb_msg_fielddefsize(f);
    size_t index = upb_fielddef_index(f);

    if (upb_fielddef_containingoneof(f)) {
      /* Oneofs are handled separately below. */
      continue;
    }

    fields[index].offset = upb_msglayout_place(l, field_size);
  }

  /* Allocate oneof fields.  Each oneof field consists of a uint32 for the case
   * and space for the actual data. */
  for (upb_msg_oneof_begin(&oit, m); !upb_msg_oneof_done(&oit);
       upb_msg_oneof_next(&oit)) {
    const upb_oneofdef* o = upb_msg_iter_oneof(&oit);
    upb_oneof_iter fit;

    size_t case_size = sizeof(uint32_t);  /* Could potentially optimize this. */
    upb_msglayout_oneofinit_v1 *oneof = &oneofs[upb_oneofdef_index(o)];
    size_t field_size = 0;

    /* Calculate field size: the max of all field sizes. */
    for (upb_oneof_begin(&fit, o);
         !upb_oneof_done(&fit);
         upb_oneof_next(&fit)) {
      const upb_fielddef* f = upb_oneof_iter_field(&fit);
      field_size = UPB_MAX(field_size, upb_msg_fielddefsize(f));
    }

    /* Align and allocate case offset. */
    oneof->case_offset = upb_msglayout_place(l, case_size);
    oneof->data_offset = upb_msglayout_place(l, field_size);
  }

  /* Size of the entire structure should be a multiple of its greatest
   * alignment.  TODO: track overall alignment for real? */
  l->data.size = align_up(l->data.size, 8);

  if (upb_msglayout_initdefault(l, m)) {
    return l;
  } else {
    upb_msglayout_free(l);
    return NULL;
  }
}


/** upb_msgfactory ************************************************************/

struct upb_msgfactory {
  const upb_symtab *symtab;  /* We own a ref. */
  upb_inttable layouts;
  upb_inttable mergehandlers;
};

upb_msgfactory *upb_msgfactory_new(const upb_symtab *symtab) {
  upb_msgfactory *ret = upb_gmalloc(sizeof(*ret));

  ret->symtab = symtab;
  upb_inttable_init(&ret->layouts, UPB_CTYPE_PTR);
  upb_inttable_init(&ret->mergehandlers, UPB_CTYPE_CONSTPTR);

  return ret;
}

void upb_msgfactory_free(upb_msgfactory *f) {
  upb_inttable_iter i;
  upb_inttable_begin(&i, &f->layouts);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_msglayout *l = upb_value_getptr(upb_inttable_iter_value(&i));
    upb_msglayout_free(l);
  }

  upb_inttable_begin(&i, &f->mergehandlers);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    const upb_handlers *h = upb_value_getconstptr(upb_inttable_iter_value(&i));
    upb_handlers_unref(h, f);
  }

  upb_inttable_uninit(&f->layouts);
  upb_inttable_uninit(&f->mergehandlers);
  upb_gfree(f);
}

const upb_symtab *upb_msgfactory_symtab(const upb_msgfactory *f) {
  return f->symtab;
}

const upb_msglayout *upb_msgfactory_getlayout(upb_msgfactory *f,
                                              const upb_msgdef *m) {
  upb_value v;
  UPB_ASSERT(upb_symtab_lookupmsg(f->symtab, upb_msgdef_fullname(m)) == m);
  UPB_ASSERT(!upb_msgdef_mapentry(m));

  if (upb_inttable_lookupptr(&f->layouts, m, &v)) {
    UPB_ASSERT(upb_value_getptr(v));
    return upb_value_getptr(v);
  } else {
    upb_msgfactory *mutable_f = (void*)f;
    upb_msglayout *l = upb_msglayout_new(m);
    upb_inttable_insertptr(&mutable_f->layouts, m, upb_value_ptr(l));
    UPB_ASSERT(l);
    return l;
  }
}

/* Our handlers that we don't expose externally. */

void *upb_msg_startstr(void *msg, const void *hd, size_t size_hint) {
  uint32_t ofs = (uintptr_t)hd;
  upb_alloc *alloc = upb_msg_alloc(msg);
  upb_msgval val;
  UPB_UNUSED(size_hint);

  val = upb_msgval_read(msg, ofs, upb_msgval_sizeof(UPB_TYPE_STRING));

  upb_free(alloc, (void*)val.str.data);
  val.str.data = NULL;
  val.str.size = 0;

  upb_msgval_write(msg, ofs, val, upb_msgval_sizeof(UPB_TYPE_STRING));
  return msg;
}

size_t upb_msg_str(void *msg, const void *hd, const char *ptr, size_t size,
                   const upb_bufhandle *handle) {
  uint32_t ofs = (uintptr_t)hd;
  upb_alloc *alloc = upb_msg_alloc(msg);
  upb_msgval val;
  size_t newsize;
  UPB_UNUSED(handle);

  val = upb_msgval_read(msg, ofs, upb_msgval_sizeof(UPB_TYPE_STRING));

  newsize = val.str.size + size;
  val.str.data = upb_realloc(alloc, (void*)val.str.data, val.str.size, newsize);

  if (!val.str.data) {
    return false;
  }

  memcpy((char*)val.str.data + val.str.size, ptr, size);
  val.str.size = newsize;
  upb_msgval_write(msg, ofs, val, upb_msgval_sizeof(UPB_TYPE_STRING));
  return size;
}

static void callback(const void *closure, upb_handlers *h) {
  upb_msgfactory *factory = (upb_msgfactory*)closure;
  const upb_msgdef *md = upb_handlers_msgdef(h);
  const upb_msglayout* layout = upb_msgfactory_getlayout(factory, md);
  upb_msg_field_iter i;
  UPB_UNUSED(factory);

  for(upb_msg_field_begin(&i, md);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    size_t offset = upb_msglayout_offset(layout, f);
    upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
    upb_handlerattr_sethandlerdata(&attr, (void*)offset);

    if (upb_fielddef_isseq(f)) {
    } else if (upb_fielddef_isstring(f)) {
      upb_handlers_setstartstr(h, f, upb_msg_startstr, &attr);
      upb_handlers_setstring(h, f, upb_msg_str, &attr);
    } else {
      upb_msg_setscalarhandler(
          h, f, offset, upb_msglayout_hasbit(layout, f));
    }
  }
}

const upb_handlers *upb_msgfactory_getmergehandlers(upb_msgfactory *f,
                                                    const upb_msgdef *m) {
  upb_msgfactory *mutable_f = (void*)f;

  /* TODO(haberman): properly cache these. */
  const upb_handlers *ret = upb_handlers_newfrozen(m, f, callback, f);
  upb_inttable_push(&mutable_f->mergehandlers, upb_value_constptr(ret));

  return ret;
}

const upb_visitorplan *upb_msgfactory_getvisitorplan(upb_msgfactory *f,
                                                     const upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);
  return (const upb_visitorplan*)upb_msgfactory_getlayout(f, md);
}


/** upb_visitor ***************************************************************/

struct upb_visitor {
  const upb_msglayout *layout;
  upb_sink *sink;
};

static upb_selector_t getsel2(const upb_fielddef *f, upb_handlertype_t type) {
  upb_selector_t ret;
  bool ok = upb_handlers_getselector(f, type, &ret);
  UPB_ASSERT(ok);
  return ret;
}

static bool upb_visitor_hasfield(const upb_msg *msg, const upb_fielddef *f,
                                 const upb_msglayout *layout) {
  int field_index = upb_fielddef_index(f);
  if (upb_fielddef_isseq(f)) {
    return upb_msgval_getarr(upb_msg_get(msg, field_index, layout)) != NULL;
  } else if (upb_msgdef_syntax(upb_fielddef_containingtype(f)) ==
             UPB_SYNTAX_PROTO2) {
    return upb_msg_has(msg, field_index, layout);
  } else {
    upb_msgval val = upb_msg_get(msg, field_index, layout);
    switch (upb_fielddef_type(f)) {
      case UPB_TYPE_FLOAT:
        return upb_msgval_getfloat(val) != 0;
      case UPB_TYPE_DOUBLE:
        return upb_msgval_getdouble(val) != 0;
      case UPB_TYPE_BOOL:
        return upb_msgval_getbool(val);
      case UPB_TYPE_ENUM:
      case UPB_TYPE_INT32:
        return upb_msgval_getint32(val) != 0;
      case UPB_TYPE_UINT32:
        return upb_msgval_getuint32(val) != 0;
      case UPB_TYPE_INT64:
        return upb_msgval_getint64(val) != 0;
      case UPB_TYPE_UINT64:
        return upb_msgval_getuint64(val) != 0;
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES:
        return upb_msgval_getstr(val).size > 0;
      case UPB_TYPE_MESSAGE:
        return upb_msgval_getmsg(val) != NULL;
    }
    UPB_UNREACHABLE();
  }
}

static bool upb_visitor_visitmsg2(const upb_msg *msg,
                                  const upb_msglayout *layout, upb_sink *sink,
                                  int depth) {
  const upb_msgdef *md = upb_handlers_msgdef(sink->handlers);
  upb_msg_field_iter i;
  upb_status status;

  upb_sink_startmsg(sink);

  /* Protect against cycles (possible because users may freely reassign message
   * and repeated fields) by imposing a maximum recursion depth. */
  if (depth > ENCODE_MAX_NESTING) {
    return false;
  }

  for (upb_msg_field_begin(&i, md);
       !upb_msg_field_done(&i);
       upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    upb_msgval val;

    if (!upb_visitor_hasfield(msg, f, layout)) {
      continue;
    }

    val = upb_msg_get(msg, upb_fielddef_index(f), layout);

    if (upb_fielddef_isseq(f)) {
      const upb_array *arr = upb_msgval_getarr(val);
      UPB_ASSERT(arr);
      /* TODO: putary(ary, f, sink, depth);*/
    } else if (upb_fielddef_issubmsg(f)) {
      const upb_map *map = upb_msgval_getmap(val);
      UPB_ASSERT(map);
      /* TODO: putmap(map, f, sink, depth);*/
    } else if (upb_fielddef_isstring(f)) {
      /* TODO putstr(); */
    } else {
      upb_selector_t sel = getsel2(f, upb_handlers_getprimitivehandlertype(f));
      UPB_ASSERT(upb_fielddef_isprimitive(f));

      switch (upb_fielddef_type(f)) {
        case UPB_TYPE_FLOAT:
          CHECK_TRUE(upb_sink_putfloat(sink, sel, upb_msgval_getfloat(val)));
          break;
        case UPB_TYPE_DOUBLE:
          CHECK_TRUE(upb_sink_putdouble(sink, sel, upb_msgval_getdouble(val)));
          break;
        case UPB_TYPE_BOOL:
          CHECK_TRUE(upb_sink_putbool(sink, sel, upb_msgval_getbool(val)));
          break;
        case UPB_TYPE_ENUM:
        case UPB_TYPE_INT32:
          CHECK_TRUE(upb_sink_putint32(sink, sel, upb_msgval_getint32(val)));
          break;
        case UPB_TYPE_UINT32:
          CHECK_TRUE(upb_sink_putuint32(sink, sel, upb_msgval_getuint32(val)));
          break;
        case UPB_TYPE_INT64:
          CHECK_TRUE(upb_sink_putint64(sink, sel, upb_msgval_getint64(val)));
          break;
        case UPB_TYPE_UINT64:
          CHECK_TRUE(upb_sink_putuint64(sink, sel, upb_msgval_getuint64(val)));
          break;
        case UPB_TYPE_STRING:
        case UPB_TYPE_BYTES:
        case UPB_TYPE_MESSAGE:
          UPB_UNREACHABLE();
      }
    }
  }

  upb_sink_endmsg(sink, &status);
  return true;
}

upb_visitor *upb_visitor_create(upb_env *e, const upb_visitorplan *vp,
                                upb_sink *output) {
  upb_visitor *visitor = upb_env_malloc(e, sizeof(*visitor));
  visitor->layout = (const upb_msglayout*)vp;
  visitor->sink = output;
  return visitor;
}

bool upb_visitor_visitmsg(upb_visitor *visitor, const upb_msg *msg) {
  return upb_visitor_visitmsg2(msg, visitor->layout, visitor->sink, 0);
}


/** upb_msg *******************************************************************/

/* If we always read/write as a consistent type to each address, this shouldn't
 * violate aliasing.
 */
#define DEREF(msg, ofs, type) *PTR_AT(msg, ofs, type)

/* Internal members of a upb_msg.  We can change this without breaking binary
 * compatibility.  We put these before the user's data.  The user's upb_msg*
 * points after the upb_msg_internal. */

/* Used when a message is not extendable. */
typedef struct {
  /* TODO(haberman): add unknown fields. */
  upb_alloc *alloc;
} upb_msg_internal;

/* Used when a message is extendable. */
typedef struct {
  upb_inttable *extdict;
  upb_msg_internal base;
} upb_msg_internal_withext;

static int upb_msg_internalsize(const upb_msglayout *l) {
    return sizeof(upb_msg_internal) - l->data.extendable * sizeof(void*);
}

static upb_msg_internal *upb_msg_getinternal(upb_msg *msg) {
  return VOIDPTR_AT(msg, -sizeof(upb_msg_internal));
}

static const upb_msg_internal *upb_msg_getinternal_const(const upb_msg *msg) {
  return VOIDPTR_AT(msg, -sizeof(upb_msg_internal));
}

static upb_msg_internal_withext *upb_msg_getinternalwithext(
    upb_msg *msg, const upb_msglayout *l) {
  UPB_ASSERT(l->data.extendable);
  return VOIDPTR_AT(msg, -sizeof(upb_msg_internal_withext));
}

static const upb_msglayout_fieldinit_v1 *upb_msg_checkfield(
    int field_index, const upb_msglayout *l) {
  UPB_ASSERT(field_index >= 0 && field_index < l->data.field_count);
  return &l->data.fields[field_index];
}

static bool upb_msg_inoneof(const upb_msglayout_fieldinit_v1 *field) {
  return field->oneof_index != UPB_NOT_IN_ONEOF;
}

static uint32_t *upb_msg_oneofcase(const upb_msg *msg, int field_index,
                                   const upb_msglayout *l) {
  const upb_msglayout_fieldinit_v1 *field = upb_msg_checkfield(field_index, l);
  UPB_ASSERT(upb_msg_inoneof(field));
  return PTR_AT(msg, l->data.oneofs[field->oneof_index].case_offset, uint32_t);
}

size_t upb_msg_sizeof(const upb_msglayout *l) {
  return l->data.size + upb_msg_internalsize(l);
}

upb_msg *upb_msg_init(void *mem, const upb_msglayout *l, upb_alloc *a) {
  upb_msg *msg = VOIDPTR_AT(mem, upb_msg_internalsize(l));

  /* Initialize normal members. */
  if (l->data.default_msg) {
    memcpy(msg, l->data.default_msg, l->data.size);
  } else {
    memset(msg, 0, l->data.size);
  }

  /* Initialize internal members. */
  upb_msg_getinternal(msg)->alloc = a;

  if (l->data.extendable) {
    upb_msg_getinternalwithext(msg, l)->extdict = NULL;
  }

  return msg;
}

void *upb_msg_uninit(upb_msg *msg, const upb_msglayout *l) {
  if (l->data.extendable) {
    upb_inttable *ext_dict = upb_msg_getinternalwithext(msg, l)->extdict;
    if (ext_dict) {
      upb_inttable_uninit2(ext_dict, upb_msg_alloc(msg));
      upb_free(upb_msg_alloc(msg), ext_dict);
    }
  }

  return VOIDPTR_AT(msg, -upb_msg_internalsize(l));
}

upb_msg *upb_msg_new(const upb_msglayout *l, upb_alloc *a) {
  void *mem = upb_malloc(a, upb_msg_sizeof(l));
  return mem ? upb_msg_init(mem, l, a) : NULL;
}

void upb_msg_free(upb_msg *msg, const upb_msglayout *l) {
  upb_free(upb_msg_alloc(msg), upb_msg_uninit(msg, l));
}

upb_alloc *upb_msg_alloc(const upb_msg *msg) {
  return upb_msg_getinternal_const(msg)->alloc;
}

bool upb_msg_has(const upb_msg *msg,
                 int field_index,
                 const upb_msglayout *l) {
  const upb_msglayout_fieldinit_v1 *field = upb_msg_checkfield(field_index, l);

  UPB_ASSERT(l->data.is_proto2);

  if (upb_msg_inoneof(field)) {
    /* Oneofs are set when the oneof number is set to this field. */
    return *upb_msg_oneofcase(msg, field_index, l) == field->number;
  } else {
    /* Other fields are set when their hasbit is set. */
    uint32_t hasbit = l->data.fields[field_index].hasbit;
    return DEREF(msg, hasbit / 8, char) | (1 << (hasbit % 8));
  }
}

upb_msgval upb_msg_get(const upb_msg *msg, int field_index,
                       const upb_msglayout *l) {
  const upb_msglayout_fieldinit_v1 *field = upb_msg_checkfield(field_index, l);
  int size = upb_msg_fieldsize(field);

  if (upb_msg_inoneof(field)) {
    if (*upb_msg_oneofcase(msg, field_index, l) == field->number) {
      size_t ofs = l->data.oneofs[field->oneof_index].data_offset;
      return upb_msgval_read(msg, ofs, size);
    } else {
      /* Return default. */
      return upb_msgval_read(l->data.default_msg, field->offset, size);
    }
  } else {
    return upb_msgval_read(msg, field->offset, size);
  }
}

void upb_msg_set(upb_msg *msg, int field_index, upb_msgval val,
                 const upb_msglayout *l) {
  const upb_msglayout_fieldinit_v1 *field = upb_msg_checkfield(field_index, l);
  int size = upb_msg_fieldsize(field);

  if (upb_msg_inoneof(field)) {
    size_t ofs = l->data.oneofs[field->oneof_index].data_offset;
    *upb_msg_oneofcase(msg, field_index, l) = field->number;
    upb_msgval_write(msg, ofs, val, size);
  } else {
    upb_msgval_write(msg, field->offset, val, size);
  }
}


/** upb_array *****************************************************************/

#define DEREF_ARR(arr, i, type) ((type*)arr->data)[i]

size_t upb_array_sizeof(upb_fieldtype_t type) {
  UPB_UNUSED(type);
  return sizeof(upb_array);
}

void upb_array_init(upb_array *arr, upb_fieldtype_t type, upb_alloc *alloc) {
  arr->type = type;
  arr->data = NULL;
  arr->len = 0;
  arr->size = 0;
  arr->element_size = upb_msgval_sizeof(type);
  arr->alloc = alloc;
}

void upb_array_uninit(upb_array *arr) {
  upb_free(arr->alloc, arr->data);
}

upb_array *upb_array_new(upb_fieldtype_t type, upb_alloc *a) {
  upb_array *ret = upb_malloc(a, upb_array_sizeof(type));

  if (ret) {
    upb_array_init(ret, type, a);
  }

  return ret;
}

void upb_array_free(upb_array *arr) {
  upb_array_uninit(arr);
  upb_free(arr->alloc, arr);
}

size_t upb_array_size(const upb_array *arr) {
  return arr->len;
}

upb_fieldtype_t upb_array_type(const upb_array *arr) {
  return arr->type;
}

upb_msgval upb_array_get(const upb_array *arr, size_t i) {
  UPB_ASSERT(i < arr->len);
  return upb_msgval_read(arr->data, i * arr->element_size, arr->element_size);
}

bool upb_array_set(upb_array *arr, size_t i, upb_msgval val) {
  UPB_ASSERT(i <= arr->len);

  if (i == arr->len) {
    /* Extending the array. */

    if (i == arr->size) {
      /* Need to reallocate. */
      size_t new_size = UPB_MAX(arr->size * 2, 8);
      size_t new_bytes = new_size * arr->element_size;
      size_t old_bytes = arr->size * arr->element_size;
      upb_msgval *new_data =
          upb_realloc(arr->alloc, arr->data, old_bytes, new_bytes);

      if (!new_data) {
        return false;
      }

      arr->data = new_data;
      arr->size = new_size;
    }

    arr->len = i + 1;
  }

  upb_msgval_write(arr->data, i * arr->element_size, val, arr->element_size);
  return true;
}


/** upb_map *******************************************************************/

struct upb_map {
  upb_fieldtype_t key_type;
  upb_fieldtype_t val_type;
  /* We may want to optimize this to use inttable where possible, for greater
   * efficiency and lower memory footprint. */
  upb_strtable strtab;
  upb_alloc *alloc;
};

static void upb_map_tokey(upb_fieldtype_t type, upb_msgval *key,
                          const char **out_key, size_t *out_len) {
  switch (type) {
    case UPB_TYPE_STRING:
      /* Point to string data of the input key. */
      *out_key = key->str.data;
      *out_len = key->str.size;
      return;
    case UPB_TYPE_BOOL:
    case UPB_TYPE_INT32:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT64:
      /* Point to the key itself.  XXX: big-endian. */
      *out_key = (const char*)key;
      *out_len = upb_msgval_sizeof(type);
      return;
    case UPB_TYPE_BYTES:
    case UPB_TYPE_DOUBLE:
    case UPB_TYPE_ENUM:
    case UPB_TYPE_FLOAT:
    case UPB_TYPE_MESSAGE:
      break;  /* Cannot be a map key. */
  }
  UPB_UNREACHABLE();
}

static upb_msgval upb_map_fromkey(upb_fieldtype_t type, const char *key,
                                  size_t len) {
  switch (type) {
    case UPB_TYPE_STRING:
      return upb_msgval_makestr(key, len);
    case UPB_TYPE_BOOL:
    case UPB_TYPE_INT32:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT64:
      return upb_msgval_read(key, 0, upb_msgval_sizeof(type));
    case UPB_TYPE_BYTES:
    case UPB_TYPE_DOUBLE:
    case UPB_TYPE_ENUM:
    case UPB_TYPE_FLOAT:
    case UPB_TYPE_MESSAGE:
      break;  /* Cannot be a map key. */
  }
  UPB_UNREACHABLE();
}

size_t upb_map_sizeof(upb_fieldtype_t ktype, upb_fieldtype_t vtype) {
  /* Size does not currently depend on key/value type. */
  UPB_UNUSED(ktype);
  UPB_UNUSED(vtype);
  return sizeof(upb_map);
}

bool upb_map_init(upb_map *map, upb_fieldtype_t ktype, upb_fieldtype_t vtype,
                  upb_alloc *a) {
  upb_ctype_t vtabtype = upb_fieldtotabtype(vtype);
  UPB_ASSERT(upb_fieldtype_mapkeyok(ktype));
  map->key_type = ktype;
  map->val_type = vtype;
  map->alloc = a;

  if (!upb_strtable_init2(&map->strtab, vtabtype, a)) {
    return false;
  }

  return true;
}

void upb_map_uninit(upb_map *map) {
  upb_strtable_uninit2(&map->strtab, map->alloc);
}

upb_map *upb_map_new(upb_fieldtype_t ktype, upb_fieldtype_t vtype,
                     upb_alloc *a) {
  upb_map *map = upb_malloc(a, upb_map_sizeof(ktype, vtype));

  if (!map) {
    return NULL;
  }

  if (!upb_map_init(map, ktype, vtype, a)) {
    return NULL;
  }

  return map;
}

void upb_map_free(upb_map *map) {
  upb_map_uninit(map);
  upb_free(map->alloc, map);
}

size_t upb_map_size(const upb_map *map) {
  return upb_strtable_count(&map->strtab);
}

upb_fieldtype_t upb_map_keytype(const upb_map *map) {
  return map->key_type;
}

upb_fieldtype_t upb_map_valuetype(const upb_map *map) {
  return map->val_type;
}

bool upb_map_get(const upb_map *map, upb_msgval key, upb_msgval *val) {
  upb_value tabval;
  const char *key_str;
  size_t key_len;
  bool ret;

  upb_map_tokey(map->key_type, &key, &key_str, &key_len);
  ret = upb_strtable_lookup2(&map->strtab, key_str, key_len, &tabval);
  if (ret) {
    memcpy(val, &tabval, sizeof(tabval));
  }

  return ret;
}

bool upb_map_set(upb_map *map, upb_msgval key, upb_msgval val,
                 upb_msgval *removed) {
  const char *key_str;
  size_t key_len;
  upb_value tabval = upb_toval(val);
  upb_value removedtabval;
  upb_alloc *a = map->alloc;

  upb_map_tokey(map->key_type, &key, &key_str, &key_len);

  /* TODO(haberman): add overwrite operation to minimize number of lookups. */
  if (upb_strtable_lookup2(&map->strtab, key_str, key_len, NULL)) {
    upb_strtable_remove3(&map->strtab, key_str, key_len, &removedtabval, a);
    memcpy(&removed, &removedtabval, sizeof(removed));
  }

  return upb_strtable_insert3(&map->strtab, key_str, key_len, tabval, a);
}

bool upb_map_del(upb_map *map, upb_msgval key) {
  const char *key_str;
  size_t key_len;
  upb_alloc *a = map->alloc;

  upb_map_tokey(map->key_type, &key, &key_str, &key_len);
  return upb_strtable_remove3(&map->strtab, key_str, key_len, NULL, a);
}


/** upb_mapiter ***************************************************************/

struct upb_mapiter {
  upb_strtable_iter iter;
  upb_fieldtype_t key_type;
};

size_t upb_mapiter_sizeof() {
  return sizeof(upb_mapiter);
}

void upb_mapiter_begin(upb_mapiter *i, const upb_map *map) {
  upb_strtable_begin(&i->iter, &map->strtab);
  i->key_type = map->key_type;
}

upb_mapiter *upb_mapiter_new(const upb_map *t, upb_alloc *a) {
  upb_mapiter *ret = upb_malloc(a, upb_mapiter_sizeof());

  if (!ret) {
    return NULL;
  }

  upb_mapiter_begin(ret, t);
  return ret;
}

void upb_mapiter_free(upb_mapiter *i, upb_alloc *a) {
  upb_free(a, i);
}

void upb_mapiter_next(upb_mapiter *i) {
  upb_strtable_next(&i->iter);
}

bool upb_mapiter_done(const upb_mapiter *i) {
  return upb_strtable_done(&i->iter);
}

upb_msgval upb_mapiter_key(const upb_mapiter *i) {
  return upb_map_fromkey(i->key_type, upb_strtable_iter_key(&i->iter),
                         upb_strtable_iter_keylength(&i->iter));
}

upb_msgval upb_mapiter_value(const upb_mapiter *i) {
  return upb_msgval_fromval(upb_strtable_iter_value(&i->iter));
}

void upb_mapiter_setdone(upb_mapiter *i) {
  upb_strtable_iter_setdone(&i->iter);
}

bool upb_mapiter_isequal(const upb_mapiter *i1, const upb_mapiter *i2) {
  return upb_strtable_iter_isequal(&i1->iter, &i2->iter);
}


/** Handlers for upb_msg ******************************************************/

typedef struct {
  size_t offset;
  int32_t hasbit;
} upb_msg_handlerdata;

/* Fallback implementation if the handler is not specialized by the producer. */
#define MSG_WRITER(type, ctype)                                               \
  bool upb_msg_set ## type (void *c, const void *hd, ctype val) {             \
    uint8_t *m = c;                                                           \
    const upb_msg_handlerdata *d = hd;                                        \
    if (d->hasbit > 0)                                                        \
      *(uint8_t*)&m[d->hasbit / 8] |= 1 << (d->hasbit % 8);                   \
    *(ctype*)&m[d->offset] = val;                                             \
    return true;                                                              \
  }                                                                           \

MSG_WRITER(double, double)
MSG_WRITER(float,  float)
MSG_WRITER(int32,  int32_t)
MSG_WRITER(int64,  int64_t)
MSG_WRITER(uint32, uint32_t)
MSG_WRITER(uint64, uint64_t)
MSG_WRITER(bool,   bool)

bool upb_msg_setscalarhandler(upb_handlers *h, const upb_fielddef *f,
                              size_t offset, int32_t hasbit) {
  upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
  bool ok;

  upb_msg_handlerdata *d = upb_gmalloc(sizeof(*d));
  if (!d) return false;
  d->offset = offset;
  d->hasbit = hasbit;

  upb_handlerattr_sethandlerdata(&attr, d);
  upb_handlerattr_setalwaysok(&attr, true);
  upb_handlers_addcleanup(h, d, upb_gfree);

#define TYPE(u, l) \
  case UPB_TYPE_##u: \
    ok = upb_handlers_set##l(h, f, upb_msg_set##l, &attr); break;

  ok = false;

  switch (upb_fielddef_type(f)) {
    TYPE(INT64,  int64);
    TYPE(INT32,  int32);
    TYPE(ENUM,   int32);
    TYPE(UINT64, uint64);
    TYPE(UINT32, uint32);
    TYPE(DOUBLE, double);
    TYPE(FLOAT,  float);
    TYPE(BOOL,   bool);
    default: UPB_ASSERT(false); break;
  }
#undef TYPE

  upb_handlerattr_uninit(&attr);
  return ok;
}

bool upb_msg_getscalarhandlerdata(const upb_handlers *h,
                                  upb_selector_t s,
                                  upb_fieldtype_t *type,
                                  size_t *offset,
                                  int32_t *hasbit) {
  const upb_msg_handlerdata *d;
  upb_func *f = upb_handlers_gethandler(h, s);

  if ((upb_int64_handlerfunc*)f == upb_msg_setint64) {
    *type = UPB_TYPE_INT64;
  } else if ((upb_int32_handlerfunc*)f == upb_msg_setint32) {
    *type = UPB_TYPE_INT32;
  } else if ((upb_uint64_handlerfunc*)f == upb_msg_setuint64) {
    *type = UPB_TYPE_UINT64;
  } else if ((upb_uint32_handlerfunc*)f == upb_msg_setuint32) {
    *type = UPB_TYPE_UINT32;
  } else if ((upb_double_handlerfunc*)f == upb_msg_setdouble) {
    *type = UPB_TYPE_DOUBLE;
  } else if ((upb_float_handlerfunc*)f == upb_msg_setfloat) {
    *type = UPB_TYPE_FLOAT;
  } else if ((upb_bool_handlerfunc*)f == upb_msg_setbool) {
    *type = UPB_TYPE_BOOL;
  } else {
    return false;
  }

  d = upb_handlers_gethandlerdata(h, s);
  *offset = d->offset;
  *hasbit = d->hasbit;
  return true;
}
/*
** upb::RefCounted Implementation
**
** Our key invariants are:
** 1. reference cycles never span groups
** 2. for ref2(to, from), we increment to's count iff group(from) != group(to)
**
** The previous two are how we avoid leaking cycles.  Other important
** invariants are:
** 3. for mutable objects "from" and "to", if there exists a ref2(to, from)
**    this implies group(from) == group(to).  (In practice, what we implement
**    is even stronger; "from" and "to" will share a group if there has *ever*
**    been a ref2(to, from), but all that is necessary for correctness is the
**    weaker one).
** 4. mutable and immutable objects are never in the same group.
*/


#include <setjmp.h>

static void freeobj(upb_refcounted *o);

const char untracked_val;
const void *UPB_UNTRACKED_REF = &untracked_val;

/* arch-specific atomic primitives  *******************************************/

#ifdef UPB_THREAD_UNSAFE /*---------------------------------------------------*/

static void atomic_inc(uint32_t *a) { (*a)++; }
static bool atomic_dec(uint32_t *a) { return --(*a) == 0; }

#elif defined(__GNUC__) || defined(__clang__) /*------------------------------*/

static void atomic_inc(uint32_t *a) { __sync_fetch_and_add(a, 1); }
static bool atomic_dec(uint32_t *a) { return __sync_sub_and_fetch(a, 1) == 0; }

#elif defined(WIN32) /*-------------------------------------------------------*/

#include <Windows.h>

static void atomic_inc(upb_atomic_t *a) { InterlockedIncrement(&a->val); }
static bool atomic_dec(upb_atomic_t *a) {
  return InterlockedDecrement(&a->val) == 0;
}

#else
#error Atomic primitives not defined for your platform/CPU.  \
       Implement them or compile with UPB_THREAD_UNSAFE.
#endif

/* All static objects point to this refcount.
 * It is special-cased in ref/unref below.  */
uint32_t static_refcount = -1;

/* We can avoid atomic ops for statically-declared objects.
 * This is a minor optimization but nice since we can avoid degrading under
 * contention in this case. */

static void refgroup(uint32_t *group) {
  if (group != &static_refcount)
    atomic_inc(group);
}

static bool unrefgroup(uint32_t *group) {
  if (group == &static_refcount) {
    return false;
  } else {
    return atomic_dec(group);
  }
}


/* Reference tracking (debug only) ********************************************/

#ifdef UPB_DEBUG_REFS

#ifdef UPB_THREAD_UNSAFE

static void upb_lock() {}
static void upb_unlock() {}

#else

/* User must define functions that lock/unlock a global mutex and link this
 * file against them. */
void upb_lock();
void upb_unlock();

#endif

/* UPB_DEBUG_REFS mode counts on being able to malloc() memory in some
 * code-paths that can normally never fail, like upb_refcounted_ref().  Since
 * we have no way to propagage out-of-memory errors back to the user, and since
 * these errors can only occur in UPB_DEBUG_REFS mode, we use an allocator that
 * immediately aborts on failure (avoiding the global allocator, which might
 * inject failures). */

#include <stdlib.h>

static void *upb_debugrefs_allocfunc(upb_alloc *alloc, void *ptr,
                                     size_t oldsize, size_t size) {
  UPB_UNUSED(alloc);
  UPB_UNUSED(oldsize);
  if (size == 0) {
    free(ptr);
    return NULL;
  } else {
    void *ret = realloc(ptr, size);

    if (!ret) {
      abort();
    }

    return ret;
  }
}

upb_alloc upb_alloc_debugrefs = {&upb_debugrefs_allocfunc};

typedef struct {
  int count;  /* How many refs there are (duplicates only allowed for ref2). */
  bool is_ref2;
} trackedref;

static trackedref *trackedref_new(bool is_ref2) {
  trackedref *ret = upb_malloc(&upb_alloc_debugrefs, sizeof(*ret));
  ret->count = 1;
  ret->is_ref2 = is_ref2;
  return ret;
}

static void track(const upb_refcounted *r, const void *owner, bool ref2) {
  upb_value v;

  UPB_ASSERT(owner);
  if (owner == UPB_UNTRACKED_REF) return;

  upb_lock();
  if (upb_inttable_lookupptr(r->refs, owner, &v)) {
    trackedref *ref = upb_value_getptr(v);
    /* Since we allow multiple ref2's for the same to/from pair without
     * allocating separate memory for each one, we lose the fine-grained
     * tracking behavior we get with regular refs.  Since ref2s only happen
     * inside upb, we'll accept this limitation until/unless there is a really
     * difficult upb-internal bug that can't be figured out without it. */
    UPB_ASSERT(ref2);
    UPB_ASSERT(ref->is_ref2);
    ref->count++;
  } else {
    trackedref *ref = trackedref_new(ref2);
    upb_inttable_insertptr2(r->refs, owner, upb_value_ptr(ref),
                            &upb_alloc_debugrefs);
    if (ref2) {
      /* We know this cast is safe when it is a ref2, because it's coming from
       * another refcounted object. */
      const upb_refcounted *from = owner;
      UPB_ASSERT(!upb_inttable_lookupptr(from->ref2s, r, NULL));
      upb_inttable_insertptr2(from->ref2s, r, upb_value_ptr(NULL),
                              &upb_alloc_debugrefs);
    }
  }
  upb_unlock();
}

static void untrack(const upb_refcounted *r, const void *owner, bool ref2) {
  upb_value v;
  bool found;
  trackedref *ref;

  UPB_ASSERT(owner);
  if (owner == UPB_UNTRACKED_REF) return;

  upb_lock();
  found = upb_inttable_lookupptr(r->refs, owner, &v);
  /* This assert will fail if an owner attempts to release a ref it didn't have. */
  UPB_ASSERT(found);
  ref = upb_value_getptr(v);
  UPB_ASSERT(ref->is_ref2 == ref2);
  if (--ref->count == 0) {
    free(ref);
    upb_inttable_removeptr(r->refs, owner, NULL);
    if (ref2) {
      /* We know this cast is safe when it is a ref2, because it's coming from
       * another refcounted object. */
      const upb_refcounted *from = owner;
      bool removed = upb_inttable_removeptr(from->ref2s, r, NULL);
      UPB_ASSERT(removed);
    }
  }
  upb_unlock();
}

static void checkref(const upb_refcounted *r, const void *owner, bool ref2) {
  upb_value v;
  bool found;
  trackedref *ref;

  upb_lock();
  found = upb_inttable_lookupptr(r->refs, owner, &v);
  UPB_ASSERT(found);
  ref = upb_value_getptr(v);
  UPB_ASSERT(ref->is_ref2 == ref2);
  upb_unlock();
}

/* Populates the given UPB_CTYPE_INT32 inttable with counts of ref2's that
 * originate from the given owner. */
static void getref2s(const upb_refcounted *owner, upb_inttable *tab) {
  upb_inttable_iter i;

  upb_lock();
  upb_inttable_begin(&i, owner->ref2s);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_value v;
    upb_value count;
    trackedref *ref;
    bool found;

    upb_refcounted *to = (upb_refcounted*)upb_inttable_iter_key(&i);

    /* To get the count we need to look in the target's table. */
    found = upb_inttable_lookupptr(to->refs, owner, &v);
    UPB_ASSERT(found);
    ref = upb_value_getptr(v);
    count = upb_value_int32(ref->count);

    upb_inttable_insertptr2(tab, to, count, &upb_alloc_debugrefs);
  }
  upb_unlock();
}

typedef struct {
  upb_inttable ref2;
  const upb_refcounted *obj;
} check_state;

static void visit_check(const upb_refcounted *obj, const upb_refcounted *subobj,
                        void *closure) {
  check_state *s = closure;
  upb_inttable *ref2 = &s->ref2;
  upb_value v;
  bool removed;
  int32_t newcount;

  UPB_ASSERT(obj == s->obj);
  UPB_ASSERT(subobj);
  removed = upb_inttable_removeptr(ref2, subobj, &v);
  /* The following assertion will fail if the visit() function visits a subobj
   * that it did not have a ref2 on, or visits the same subobj too many times. */
  UPB_ASSERT(removed);
  newcount = upb_value_getint32(v) - 1;
  if (newcount > 0) {
    upb_inttable_insert2(ref2, (uintptr_t)subobj, upb_value_int32(newcount),
                         &upb_alloc_debugrefs);
  }
}

static void visit(const upb_refcounted *r, upb_refcounted_visit *v,
                  void *closure) {
  /* In DEBUG_REFS mode we know what existing ref2 refs there are, so we know
   * exactly the set of nodes that visit() should visit.  So we verify visit()'s
   * correctness here. */
  check_state state;
  state.obj = r;
  upb_inttable_init2(&state.ref2, UPB_CTYPE_INT32, &upb_alloc_debugrefs);
  getref2s(r, &state.ref2);

  /* This should visit any children in the ref2 table. */
  if (r->vtbl->visit) r->vtbl->visit(r, visit_check, &state);

  /* This assertion will fail if the visit() function missed any children. */
  UPB_ASSERT(upb_inttable_count(&state.ref2) == 0);
  upb_inttable_uninit2(&state.ref2, &upb_alloc_debugrefs);
  if (r->vtbl->visit) r->vtbl->visit(r, v, closure);
}

static void trackinit(upb_refcounted *r) {
  r->refs = upb_malloc(&upb_alloc_debugrefs, sizeof(*r->refs));
  r->ref2s = upb_malloc(&upb_alloc_debugrefs, sizeof(*r->ref2s));
  upb_inttable_init2(r->refs, UPB_CTYPE_PTR, &upb_alloc_debugrefs);
  upb_inttable_init2(r->ref2s, UPB_CTYPE_PTR, &upb_alloc_debugrefs);
}

static void trackfree(const upb_refcounted *r) {
  upb_inttable_uninit2(r->refs, &upb_alloc_debugrefs);
  upb_inttable_uninit2(r->ref2s, &upb_alloc_debugrefs);
  upb_free(&upb_alloc_debugrefs, r->refs);
  upb_free(&upb_alloc_debugrefs, r->ref2s);
}

#else

static void track(const upb_refcounted *r, const void *owner, bool ref2) {
  UPB_UNUSED(r);
  UPB_UNUSED(owner);
  UPB_UNUSED(ref2);
}

static void untrack(const upb_refcounted *r, const void *owner, bool ref2) {
  UPB_UNUSED(r);
  UPB_UNUSED(owner);
  UPB_UNUSED(ref2);
}

static void checkref(const upb_refcounted *r, const void *owner, bool ref2) {
  UPB_UNUSED(r);
  UPB_UNUSED(owner);
  UPB_UNUSED(ref2);
}

static void trackinit(upb_refcounted *r) {
  UPB_UNUSED(r);
}

static void trackfree(const upb_refcounted *r) {
  UPB_UNUSED(r);
}

static void visit(const upb_refcounted *r, upb_refcounted_visit *v,
                  void *closure) {
  if (r->vtbl->visit) r->vtbl->visit(r, v, closure);
}

#endif  /* UPB_DEBUG_REFS */


/* freeze() *******************************************************************/

/* The freeze() operation is by far the most complicated part of this scheme.
 * We compute strongly-connected components and then mutate the graph such that
 * we preserve the invariants documented at the top of this file.  And we must
 * handle out-of-memory errors gracefully (without leaving the graph
 * inconsistent), which adds to the fun. */

/* The state used by the freeze operation (shared across many functions). */
typedef struct {
  int depth;
  int maxdepth;
  uint64_t index;
  /* Maps upb_refcounted* -> attributes (color, etc).  attr layout varies by
   * color. */
  upb_inttable objattr;
  upb_inttable stack;   /* stack of upb_refcounted* for Tarjan's algorithm. */
  upb_inttable groups;  /* array of uint32_t*, malloc'd refcounts for new groups */
  upb_status *status;
  jmp_buf err;
} tarjan;

static void release_ref2(const upb_refcounted *obj,
                         const upb_refcounted *subobj,
                         void *closure);

/* Node attributes -----------------------------------------------------------*/

/* After our analysis phase all nodes will be either GRAY or WHITE. */

typedef enum {
  BLACK = 0,  /* Object has not been seen. */
  GRAY,   /* Object has been found via a refgroup but may not be reachable. */
  GREEN,  /* Object is reachable and is currently on the Tarjan stack. */
  WHITE   /* Object is reachable and has been assigned a group (SCC). */
} color_t;

UPB_NORETURN static void err(tarjan *t) { longjmp(t->err, 1); }
UPB_NORETURN static void oom(tarjan *t) {
  upb_status_seterrmsg(t->status, "out of memory");
  err(t);
}

static uint64_t trygetattr(const tarjan *t, const upb_refcounted *r) {
  upb_value v;
  return upb_inttable_lookupptr(&t->objattr, r, &v) ?
      upb_value_getuint64(v) : 0;
}

static uint64_t getattr(const tarjan *t, const upb_refcounted *r) {
  upb_value v;
  bool found = upb_inttable_lookupptr(&t->objattr, r, &v);
  UPB_ASSERT(found);
  return upb_value_getuint64(v);
}

static void setattr(tarjan *t, const upb_refcounted *r, uint64_t attr) {
  upb_inttable_removeptr(&t->objattr, r, NULL);
  upb_inttable_insertptr(&t->objattr, r, upb_value_uint64(attr));
}

static color_t color(tarjan *t, const upb_refcounted *r) {
  return trygetattr(t, r) & 0x3;  /* Color is always stored in the low 2 bits. */
}

static void set_gray(tarjan *t, const upb_refcounted *r) {
  UPB_ASSERT(color(t, r) == BLACK);
  setattr(t, r, GRAY);
}

/* Pushes an obj onto the Tarjan stack and sets it to GREEN. */
static void push(tarjan *t, const upb_refcounted *r) {
  UPB_ASSERT(color(t, r) == BLACK || color(t, r) == GRAY);
  /* This defines the attr layout for the GREEN state.  "index" and "lowlink"
   * get 31 bits, which is plenty (limit of 2B objects frozen at a time). */
  setattr(t, r, GREEN | (t->index << 2) | (t->index << 33));
  if (++t->index == 0x80000000) {
    upb_status_seterrmsg(t->status, "too many objects to freeze");
    err(t);
  }
  upb_inttable_push(&t->stack, upb_value_ptr((void*)r));
}

/* Pops an obj from the Tarjan stack and sets it to WHITE, with a ptr to its
 * SCC group. */
static upb_refcounted *pop(tarjan *t) {
  upb_refcounted *r = upb_value_getptr(upb_inttable_pop(&t->stack));
  UPB_ASSERT(color(t, r) == GREEN);
  /* This defines the attr layout for nodes in the WHITE state.
   * Top of group stack is [group, NULL]; we point at group. */
  setattr(t, r, WHITE | (upb_inttable_count(&t->groups) - 2) << 8);
  return r;
}

static void tarjan_newgroup(tarjan *t) {
  uint32_t *group = upb_gmalloc(sizeof(*group));
  if (!group) oom(t);
  /* Push group and empty group leader (we'll fill in leader later). */
  if (!upb_inttable_push(&t->groups, upb_value_ptr(group)) ||
      !upb_inttable_push(&t->groups, upb_value_ptr(NULL))) {
    upb_gfree(group);
    oom(t);
  }
  *group = 0;
}

static uint32_t idx(tarjan *t, const upb_refcounted *r) {
  UPB_ASSERT(color(t, r) == GREEN);
  return (getattr(t, r) >> 2) & 0x7FFFFFFF;
}

static uint32_t lowlink(tarjan *t, const upb_refcounted *r) {
  if (color(t, r) == GREEN) {
    return getattr(t, r) >> 33;
  } else {
    return UINT32_MAX;
  }
}

static void set_lowlink(tarjan *t, const upb_refcounted *r, uint32_t lowlink) {
  UPB_ASSERT(color(t, r) == GREEN);
  setattr(t, r, ((uint64_t)lowlink << 33) | (getattr(t, r) & 0x1FFFFFFFF));
}

static uint32_t *group(tarjan *t, upb_refcounted *r) {
  uint64_t groupnum;
  upb_value v;
  bool found;

  UPB_ASSERT(color(t, r) == WHITE);
  groupnum = getattr(t, r) >> 8;
  found = upb_inttable_lookup(&t->groups, groupnum, &v);
  UPB_ASSERT(found);
  return upb_value_getptr(v);
}

/* If the group leader for this object's group has not previously been set,
 * the given object is assigned to be its leader. */
static upb_refcounted *groupleader(tarjan *t, upb_refcounted *r) {
  uint64_t leader_slot;
  upb_value v;
  bool found;

  UPB_ASSERT(color(t, r) == WHITE);
  leader_slot = (getattr(t, r) >> 8) + 1;
  found = upb_inttable_lookup(&t->groups, leader_slot, &v);
  UPB_ASSERT(found);
  if (upb_value_getptr(v)) {
    return upb_value_getptr(v);
  } else {
    upb_inttable_remove(&t->groups, leader_slot, NULL);
    upb_inttable_insert(&t->groups, leader_slot, upb_value_ptr(r));
    return r;
  }
}


/* Tarjan's algorithm --------------------------------------------------------*/

/* See:
 *   http://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm */
static void do_tarjan(const upb_refcounted *obj, tarjan *t);

static void tarjan_visit(const upb_refcounted *obj,
                         const upb_refcounted *subobj,
                         void *closure) {
  tarjan *t = closure;
  if (++t->depth > t->maxdepth) {
    upb_status_seterrf(t->status, "graph too deep to freeze (%d)", t->maxdepth);
    err(t);
  } else if (subobj->is_frozen || color(t, subobj) == WHITE) {
    /* Do nothing: we don't want to visit or color already-frozen nodes,
     * and WHITE nodes have already been assigned a SCC. */
  } else if (color(t, subobj) < GREEN) {
    /* Subdef has not yet been visited; recurse on it. */
    do_tarjan(subobj, t);
    set_lowlink(t, obj, UPB_MIN(lowlink(t, obj), lowlink(t, subobj)));
  } else if (color(t, subobj) == GREEN) {
    /* Subdef is in the stack and hence in the current SCC. */
    set_lowlink(t, obj, UPB_MIN(lowlink(t, obj), idx(t, subobj)));
  }
  --t->depth;
}

static void do_tarjan(const upb_refcounted *obj, tarjan *t) {
  if (color(t, obj) == BLACK) {
    /* We haven't seen this object's group; mark the whole group GRAY. */
    const upb_refcounted *o = obj;
    do { set_gray(t, o); } while ((o = o->next) != obj);
  }

  push(t, obj);
  visit(obj, tarjan_visit, t);
  if (lowlink(t, obj) == idx(t, obj)) {
    tarjan_newgroup(t);
    while (pop(t) != obj)
      ;
  }
}


/* freeze() ------------------------------------------------------------------*/

static void crossref(const upb_refcounted *r, const upb_refcounted *subobj,
                     void *_t) {
  tarjan *t = _t;
  UPB_ASSERT(color(t, r) > BLACK);
  if (color(t, subobj) > BLACK && r->group != subobj->group) {
    /* Previously this ref was not reflected in subobj->group because they
     * were in the same group; now that they are split a ref must be taken. */
    refgroup(subobj->group);
  }
}

static bool freeze(upb_refcounted *const*roots, int n, upb_status *s,
                   int maxdepth) {
  volatile bool ret = false;
  int i;
  upb_inttable_iter iter;

  /* We run in two passes so that we can allocate all memory before performing
   * any mutation of the input -- this allows us to leave the input unchanged
   * in the case of memory allocation failure. */
  tarjan t;
  t.index = 0;
  t.depth = 0;
  t.maxdepth = maxdepth;
  t.status = s;
  if (!upb_inttable_init(&t.objattr, UPB_CTYPE_UINT64)) goto err1;
  if (!upb_inttable_init(&t.stack, UPB_CTYPE_PTR)) goto err2;
  if (!upb_inttable_init(&t.groups, UPB_CTYPE_PTR)) goto err3;
  if (setjmp(t.err) != 0) goto err4;


  for (i = 0; i < n; i++) {
    if (color(&t, roots[i]) < GREEN) {
      do_tarjan(roots[i], &t);
    }
  }

  /* If we've made it this far, no further errors are possible so it's safe to
   * mutate the objects without risk of leaving them in an inconsistent state. */
  ret = true;

  /* The transformation that follows requires care.  The preconditions are:
   * - all objects in attr map are WHITE or GRAY, and are in mutable groups
   *   (groups of all mutable objs)
   * - no ref2(to, from) refs have incremented count(to) if both "to" and
   *   "from" are in our attr map (this follows from invariants (2) and (3)) */

  /* Pass 1: we remove WHITE objects from their mutable groups, and add them to
   * new groups  according to the SCC's we computed.  These new groups will
   * consist of only frozen objects.  None will be immediately collectible,
   * because WHITE objects are by definition reachable from one of "roots",
   * which the caller must own refs on. */
  upb_inttable_begin(&iter, &t.objattr);
  for(; !upb_inttable_done(&iter); upb_inttable_next(&iter)) {
    upb_refcounted *obj = (upb_refcounted*)upb_inttable_iter_key(&iter);
    /* Since removal from a singly-linked list requires access to the object's
     * predecessor, we consider obj->next instead of obj for moving.  With the
     * while() loop we guarantee that we will visit every node's predecessor.
     * Proof:
     *  1. every node's predecessor is in our attr map.
     *  2. though the loop body may change a node's predecessor, it will only
     *     change it to be the node we are currently operating on, so with a
     *     while() loop we guarantee ourselves the chance to remove each node. */
    while (color(&t, obj->next) == WHITE &&
           group(&t, obj->next) != obj->next->group) {
      upb_refcounted *leader;

      /* Remove from old group. */
      upb_refcounted *move = obj->next;
      if (obj == move) {
        /* Removing the last object from a group. */
        UPB_ASSERT(*obj->group == obj->individual_count);
        upb_gfree(obj->group);
      } else {
        obj->next = move->next;
        /* This may decrease to zero; we'll collect GRAY objects (if any) that
         * remain in the group in the third pass. */
        UPB_ASSERT(*move->group >= move->individual_count);
        *move->group -= move->individual_count;
      }

      /* Add to new group. */
      leader = groupleader(&t, move);
      if (move == leader) {
        /* First object added to new group is its leader. */
        move->group = group(&t, move);
        move->next = move;
        *move->group = move->individual_count;
      } else {
        /* Group already has at least one object in it. */
        UPB_ASSERT(leader->group == group(&t, move));
        move->group = group(&t, move);
        move->next = leader->next;
        leader->next = move;
        *move->group += move->individual_count;
      }

      move->is_frozen = true;
    }
  }

  /* Pass 2: GRAY and WHITE objects "obj" with ref2(to, obj) references must
   * increment count(to) if group(obj) != group(to) (which could now be the
   * case if "to" was just frozen). */
  upb_inttable_begin(&iter, &t.objattr);
  for(; !upb_inttable_done(&iter); upb_inttable_next(&iter)) {
    upb_refcounted *obj = (upb_refcounted*)upb_inttable_iter_key(&iter);
    visit(obj, crossref, &t);
  }

  /* Pass 3: GRAY objects are collected if their group's refcount dropped to
   * zero when we removed its white nodes.  This can happen if they had only
   * been kept alive by virtue of sharing a group with an object that was just
   * frozen.
   *
   * It is important that we do this last, since the GRAY object's free()
   * function could call unref2() on just-frozen objects, which will decrement
   * refs that were added in pass 2. */
  upb_inttable_begin(&iter, &t.objattr);
  for(; !upb_inttable_done(&iter); upb_inttable_next(&iter)) {
    upb_refcounted *obj = (upb_refcounted*)upb_inttable_iter_key(&iter);
    if (obj->group == NULL || *obj->group == 0) {
      if (obj->group) {
        upb_refcounted *o;

        /* We eagerly free() the group's count (since we can't easily determine
         * the group's remaining size it's the easiest way to ensure it gets
         * done). */
        upb_gfree(obj->group);

        /* Visit to release ref2's (done in a separate pass since release_ref2
         * depends on o->group being unmodified so it can test merged()). */
        o = obj;
        do { visit(o, release_ref2, NULL); } while ((o = o->next) != obj);

        /* Mark "group" fields as NULL so we know to free the objects later in
         * this loop, but also don't try to delete the group twice. */
        o = obj;
        do { o->group = NULL; } while ((o = o->next) != obj);
      }
      freeobj(obj);
    }
  }

err4:
  if (!ret) {
    upb_inttable_begin(&iter, &t.groups);
    for(; !upb_inttable_done(&iter); upb_inttable_next(&iter))
      upb_gfree(upb_value_getptr(upb_inttable_iter_value(&iter)));
  }
  upb_inttable_uninit(&t.groups);
err3:
  upb_inttable_uninit(&t.stack);
err2:
  upb_inttable_uninit(&t.objattr);
err1:
  return ret;
}


/* Misc internal functions  ***************************************************/

static bool merged(const upb_refcounted *r, const upb_refcounted *r2) {
  return r->group == r2->group;
}

static void merge(upb_refcounted *r, upb_refcounted *from) {
  upb_refcounted *base;
  upb_refcounted *tmp;

  if (merged(r, from)) return;
  *r->group += *from->group;
  upb_gfree(from->group);
  base = from;

  /* Set all refcount pointers in the "from" chain to the merged refcount.
   *
   * TODO(haberman): this linear algorithm can result in an overall O(n^2) bound
   * if the user continuously extends a group by one object.  Prevent this by
   * using one of the techniques in this paper:
   *     http://bioinfo.ict.ac.cn/~dbu/AlgorithmCourses/Lectures/Union-Find-Tarjan.pdf */
  do { from->group = r->group; } while ((from = from->next) != base);

  /* Merge the two circularly linked lists by swapping their next pointers. */
  tmp = r->next;
  r->next = base->next;
  base->next = tmp;
}

static void unref(const upb_refcounted *r);

static void release_ref2(const upb_refcounted *obj,
                         const upb_refcounted *subobj,
                         void *closure) {
  UPB_UNUSED(closure);
  untrack(subobj, obj, true);
  if (!merged(obj, subobj)) {
    UPB_ASSERT(subobj->is_frozen);
    unref(subobj);
  }
}

static void unref(const upb_refcounted *r) {
  if (unrefgroup(r->group)) {
    const upb_refcounted *o;

    upb_gfree(r->group);

    /* In two passes, since release_ref2 needs a guarantee that any subobjs
     * are alive. */
    o = r;
    do { visit(o, release_ref2, NULL); } while((o = o->next) != r);

    o = r;
    do {
      const upb_refcounted *next = o->next;
      UPB_ASSERT(o->is_frozen || o->individual_count == 0);
      freeobj((upb_refcounted*)o);
      o = next;
    } while(o != r);
  }
}

static void freeobj(upb_refcounted *o) {
  trackfree(o);
  o->vtbl->free((upb_refcounted*)o);
}


/* Public interface ***********************************************************/

bool upb_refcounted_init(upb_refcounted *r,
                         const struct upb_refcounted_vtbl *vtbl,
                         const void *owner) {
#ifndef NDEBUG
  /* Endianness check.  This is unrelated to upb_refcounted, it's just a
   * convenient place to put the check that we can be assured will run for
   * basically every program using upb. */
  const int x = 1;
#ifdef UPB_BIG_ENDIAN
  UPB_ASSERT(*(char*)&x != 1);
#else
  UPB_ASSERT(*(char*)&x == 1);
#endif
#endif

  r->next = r;
  r->vtbl = vtbl;
  r->individual_count = 0;
  r->is_frozen = false;
  r->group = upb_gmalloc(sizeof(*r->group));
  if (!r->group) return false;
  *r->group = 0;
  trackinit(r);
  upb_refcounted_ref(r, owner);
  return true;
}

bool upb_refcounted_isfrozen(const upb_refcounted *r) {
  return r->is_frozen;
}

void upb_refcounted_ref(const upb_refcounted *r, const void *owner) {
  track(r, owner, false);
  if (!r->is_frozen)
    ((upb_refcounted*)r)->individual_count++;
  refgroup(r->group);
}

void upb_refcounted_unref(const upb_refcounted *r, const void *owner) {
  untrack(r, owner, false);
  if (!r->is_frozen)
    ((upb_refcounted*)r)->individual_count--;
  unref(r);
}

void upb_refcounted_ref2(const upb_refcounted *r, upb_refcounted *from) {
  UPB_ASSERT(!from->is_frozen);  /* Non-const pointer implies this. */
  track(r, from, true);
  if (r->is_frozen) {
    refgroup(r->group);
  } else {
    merge((upb_refcounted*)r, from);
  }
}

void upb_refcounted_unref2(const upb_refcounted *r, upb_refcounted *from) {
  UPB_ASSERT(!from->is_frozen);  /* Non-const pointer implies this. */
  untrack(r, from, true);
  if (r->is_frozen) {
    unref(r);
  } else {
    UPB_ASSERT(merged(r, from));
  }
}

void upb_refcounted_donateref(
    const upb_refcounted *r, const void *from, const void *to) {
  UPB_ASSERT(from != to);
  if (to != NULL)
    upb_refcounted_ref(r, to);
  if (from != NULL)
    upb_refcounted_unref(r, from);
}

void upb_refcounted_checkref(const upb_refcounted *r, const void *owner) {
  checkref(r, owner, false);
}

bool upb_refcounted_freeze(upb_refcounted *const*roots, int n, upb_status *s,
                           int maxdepth) {
  int i;
  bool ret;
  for (i = 0; i < n; i++) {
    UPB_ASSERT(!roots[i]->is_frozen);
  }
  ret = freeze(roots, n, s, maxdepth);
  UPB_ASSERT(!s || ret == upb_ok(s));
  return ret;
}


bool upb_bufsrc_putbuf(const char *buf, size_t len, upb_bytessink *sink) {
  void *subc;
  bool ret;
  upb_bufhandle handle;
  upb_bufhandle_init(&handle);
  upb_bufhandle_setbuf(&handle, buf, 0);
  ret = upb_bytessink_start(sink, len, &subc);
  if (ret && len != 0) {
    ret = (upb_bytessink_putbuf(sink, subc, buf, len, &handle) >= len);
  }
  if (ret) {
    ret = upb_bytessink_end(sink);
  }
  upb_bufhandle_uninit(&handle);
  return ret;
}

struct upb_bufsink {
  upb_byteshandler handler;
  upb_bytessink sink;
  upb_env *env;
  char *ptr;
  size_t len, size;
};

static void *upb_bufsink_start(void *_sink, const void *hd, size_t size_hint) {
  upb_bufsink *sink = _sink;
  UPB_UNUSED(hd);
  UPB_UNUSED(size_hint);
  sink->len = 0;
  return sink;
}

static size_t upb_bufsink_string(void *_sink, const void *hd, const char *ptr,
                                size_t len, const upb_bufhandle *handle) {
  upb_bufsink *sink = _sink;
  size_t new_size = sink->size;

  UPB_ASSERT(new_size > 0);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  while (sink->len + len > new_size) {
    new_size *= 2;
  }

  if (new_size != sink->size) {
    sink->ptr = upb_env_realloc(sink->env, sink->ptr, sink->size, new_size);
    sink->size = new_size;
  }

  memcpy(sink->ptr + sink->len, ptr, len);
  sink->len += len;

  return len;
}

upb_bufsink *upb_bufsink_new(upb_env *env) {
  upb_bufsink *sink = upb_env_malloc(env, sizeof(upb_bufsink));
  upb_byteshandler_init(&sink->handler);
  upb_byteshandler_setstartstr(&sink->handler, upb_bufsink_start, NULL);
  upb_byteshandler_setstring(&sink->handler, upb_bufsink_string, NULL);

  upb_bytessink_reset(&sink->sink, &sink->handler, sink);

  sink->env = env;
  sink->size = 32;
  sink->ptr = upb_env_malloc(env, sink->size);
  sink->len = 0;

  return sink;
}

void upb_bufsink_free(upb_bufsink *sink) {
  upb_env_free(sink->env, sink->ptr);
  upb_env_free(sink->env, sink);
}

upb_bytessink *upb_bufsink_sink(upb_bufsink *sink) {
  return &sink->sink;
}

const char *upb_bufsink_getdata(const upb_bufsink *sink, size_t *len) {
  *len = sink->len;
  return sink->ptr;
}
/*
** upb_table Implementation
**
** Implementation is heavily inspired by Lua's ltable.c.
*/


#include <string.h>

#define UPB_MAXARRSIZE 16  /* 64k. */

/* From Chromium. */
#define ARRAY_SIZE(x) \
    ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

static void upb_check_alloc(upb_table *t, upb_alloc *a) {
  UPB_UNUSED(t);
  UPB_UNUSED(a);
  UPB_ASSERT_DEBUGVAR(t->alloc == a);
}

static const double MAX_LOAD = 0.85;

/* The minimum utilization of the array part of a mixed hash/array table.  This
 * is a speed/memory-usage tradeoff (though it's not straightforward because of
 * cache effects).  The lower this is, the more memory we'll use. */
static const double MIN_DENSITY = 0.1;

bool is_pow2(uint64_t v) { return v == 0 || (v & (v - 1)) == 0; }

int log2ceil(uint64_t v) {
  int ret = 0;
  bool pow2 = is_pow2(v);
  while (v >>= 1) ret++;
  ret = pow2 ? ret : ret + 1;  /* Ceiling. */
  return UPB_MIN(UPB_MAXARRSIZE, ret);
}

char *upb_strdup(const char *s, upb_alloc *a) {
  return upb_strdup2(s, strlen(s), a);
}

char *upb_strdup2(const char *s, size_t len, upb_alloc *a) {
  size_t n;
  char *p;

  /* Prevent overflow errors. */
  if (len == SIZE_MAX) return NULL;
  /* Always null-terminate, even if binary data; but don't rely on the input to
   * have a null-terminating byte since it may be a raw binary buffer. */
  n = len + 1;
  p = upb_malloc(a, n);
  if (p) {
    memcpy(p, s, len);
    p[len] = 0;
  }
  return p;
}

/* A type to represent the lookup key of either a strtable or an inttable. */
typedef union {
  uintptr_t num;
  struct {
    const char *str;
    size_t len;
  } str;
} lookupkey_t;

static lookupkey_t strkey2(const char *str, size_t len) {
  lookupkey_t k;
  k.str.str = str;
  k.str.len = len;
  return k;
}

static lookupkey_t intkey(uintptr_t key) {
  lookupkey_t k;
  k.num = key;
  return k;
}

typedef uint32_t hashfunc_t(upb_tabkey key);
typedef bool eqlfunc_t(upb_tabkey k1, lookupkey_t k2);

/* Base table (shared code) ***************************************************/

/* For when we need to cast away const. */
static upb_tabent *mutable_entries(upb_table *t) {
  return (upb_tabent*)t->entries;
}

static bool isfull(upb_table *t) {
  if (upb_table_size(t) == 0) {
    return true;
  } else {
    return ((double)(t->count + 1) / upb_table_size(t)) > MAX_LOAD;
  }
}

static bool init(upb_table *t, upb_ctype_t ctype, uint8_t size_lg2,
                 upb_alloc *a) {
  size_t bytes;

  t->count = 0;
  t->ctype = ctype;
  t->size_lg2 = size_lg2;
  t->mask = upb_table_size(t) ? upb_table_size(t) - 1 : 0;
#ifndef NDEBUG
  t->alloc = a;
#endif
  bytes = upb_table_size(t) * sizeof(upb_tabent);
  if (bytes > 0) {
    t->entries = upb_malloc(a, bytes);
    if (!t->entries) return false;
    memset(mutable_entries(t), 0, bytes);
  } else {
    t->entries = NULL;
  }
  return true;
}

static void uninit(upb_table *t, upb_alloc *a) {
  upb_check_alloc(t, a);
  upb_free(a, mutable_entries(t));
}

static upb_tabent *emptyent(upb_table *t) {
  upb_tabent *e = mutable_entries(t) + upb_table_size(t);
  while (1) { if (upb_tabent_isempty(--e)) return e; UPB_ASSERT(e > t->entries); }
}

static upb_tabent *getentry_mutable(upb_table *t, uint32_t hash) {
  return (upb_tabent*)upb_getentry(t, hash);
}

static const upb_tabent *findentry(const upb_table *t, lookupkey_t key,
                                   uint32_t hash, eqlfunc_t *eql) {
  const upb_tabent *e;

  if (t->size_lg2 == 0) return NULL;
  e = upb_getentry(t, hash);
  if (upb_tabent_isempty(e)) return NULL;
  while (1) {
    if (eql(e->key, key)) return e;
    if ((e = e->next) == NULL) return NULL;
  }
}

static upb_tabent *findentry_mutable(upb_table *t, lookupkey_t key,
                                     uint32_t hash, eqlfunc_t *eql) {
  return (upb_tabent*)findentry(t, key, hash, eql);
}

static bool lookup(const upb_table *t, lookupkey_t key, upb_value *v,
                   uint32_t hash, eqlfunc_t *eql) {
  const upb_tabent *e = findentry(t, key, hash, eql);
  if (e) {
    if (v) {
      _upb_value_setval(v, e->val.val, t->ctype);
    }
    return true;
  } else {
    return false;
  }
}

/* The given key must not already exist in the table. */
static void insert(upb_table *t, lookupkey_t key, upb_tabkey tabkey,
                   upb_value val, uint32_t hash,
                   hashfunc_t *hashfunc, eqlfunc_t *eql) {
  upb_tabent *mainpos_e;
  upb_tabent *our_e;

  UPB_ASSERT(findentry(t, key, hash, eql) == NULL);
  UPB_ASSERT_DEBUGVAR(val.ctype == t->ctype);

  t->count++;
  mainpos_e = getentry_mutable(t, hash);
  our_e = mainpos_e;

  if (upb_tabent_isempty(mainpos_e)) {
    /* Our main position is empty; use it. */
    our_e->next = NULL;
  } else {
    /* Collision. */
    upb_tabent *new_e = emptyent(t);
    /* Head of collider's chain. */
    upb_tabent *chain = getentry_mutable(t, hashfunc(mainpos_e->key));
    if (chain == mainpos_e) {
      /* Existing ent is in its main posisiton (it has the same hash as us, and
       * is the head of our chain).  Insert to new ent and append to this chain. */
      new_e->next = mainpos_e->next;
      mainpos_e->next = new_e;
      our_e = new_e;
    } else {
      /* Existing ent is not in its main position (it is a node in some other
       * chain).  This implies that no existing ent in the table has our hash.
       * Evict it (updating its chain) and use its ent for head of our chain. */
      *new_e = *mainpos_e;  /* copies next. */
      while (chain->next != mainpos_e) {
        chain = (upb_tabent*)chain->next;
        UPB_ASSERT(chain);
      }
      chain->next = new_e;
      our_e = mainpos_e;
      our_e->next = NULL;
    }
  }
  our_e->key = tabkey;
  our_e->val.val = val.val;
  UPB_ASSERT(findentry(t, key, hash, eql) == our_e);
}

static bool rm(upb_table *t, lookupkey_t key, upb_value *val,
               upb_tabkey *removed, uint32_t hash, eqlfunc_t *eql) {
  upb_tabent *chain = getentry_mutable(t, hash);
  if (upb_tabent_isempty(chain)) return false;
  if (eql(chain->key, key)) {
    /* Element to remove is at the head of its chain. */
    t->count--;
    if (val) _upb_value_setval(val, chain->val.val, t->ctype);
    if (removed) *removed = chain->key;
    if (chain->next) {
      upb_tabent *move = (upb_tabent*)chain->next;
      *chain = *move;
      move->key = 0;  /* Make the slot empty. */
    } else {
      chain->key = 0;  /* Make the slot empty. */
    }
    return true;
  } else {
    /* Element to remove is either in a non-head position or not in the
     * table. */
    while (chain->next && !eql(chain->next->key, key)) {
      chain = (upb_tabent*)chain->next;
    }
    if (chain->next) {
      /* Found element to remove. */
      upb_tabent *rm = (upb_tabent*)chain->next;
      t->count--;
      if (val) _upb_value_setval(val, chain->next->val.val, t->ctype);
      if (removed) *removed = rm->key;
      rm->key = 0;  /* Make the slot empty. */
      chain->next = rm->next;
      return true;
    } else {
      /* Element to remove is not in the table. */
      return false;
    }
  }
}

static size_t next(const upb_table *t, size_t i) {
  do {
    if (++i >= upb_table_size(t))
      return SIZE_MAX;
  } while(upb_tabent_isempty(&t->entries[i]));

  return i;
}

static size_t begin(const upb_table *t) {
  return next(t, -1);
}


/* upb_strtable ***************************************************************/

/* A simple "subclass" of upb_table that only adds a hash function for strings. */

static upb_tabkey strcopy(lookupkey_t k2, upb_alloc *a) {
  char *str = upb_malloc(a, k2.str.len + sizeof(uint32_t) + 1);
  if (str == NULL) return 0;
  memcpy(str, &k2.str.len, sizeof(uint32_t));
  memcpy(str + sizeof(uint32_t), k2.str.str, k2.str.len + 1);
  return (uintptr_t)str;
}

static uint32_t strhash(upb_tabkey key) {
  uint32_t len;
  char *str = upb_tabstr(key, &len);
  return MurmurHash2(str, len, 0);
}

static bool streql(upb_tabkey k1, lookupkey_t k2) {
  uint32_t len;
  char *str = upb_tabstr(k1, &len);
  return len == k2.str.len && memcmp(str, k2.str.str, len) == 0;
}

bool upb_strtable_init2(upb_strtable *t, upb_ctype_t ctype, upb_alloc *a) {
  return init(&t->t, ctype, 2, a);
}

void upb_strtable_uninit2(upb_strtable *t, upb_alloc *a) {
  size_t i;
  for (i = 0; i < upb_table_size(&t->t); i++)
    upb_free(a, (void*)t->t.entries[i].key);
  uninit(&t->t, a);
}

bool upb_strtable_resize(upb_strtable *t, size_t size_lg2, upb_alloc *a) {
  upb_strtable new_table;
  upb_strtable_iter i;

  upb_check_alloc(&t->t, a);

  if (!init(&new_table.t, t->t.ctype, size_lg2, a))
    return false;
  upb_strtable_begin(&i, t);
  for ( ; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    upb_strtable_insert3(
        &new_table,
        upb_strtable_iter_key(&i),
        upb_strtable_iter_keylength(&i),
        upb_strtable_iter_value(&i),
        a);
  }
  upb_strtable_uninit2(t, a);
  *t = new_table;
  return true;
}

bool upb_strtable_insert3(upb_strtable *t, const char *k, size_t len,
                          upb_value v, upb_alloc *a) {
  lookupkey_t key;
  upb_tabkey tabkey;
  uint32_t hash;

  upb_check_alloc(&t->t, a);

  if (isfull(&t->t)) {
    /* Need to resize.  New table of double the size, add old elements to it. */
    if (!upb_strtable_resize(t, t->t.size_lg2 + 1, a)) {
      return false;
    }
  }

  key = strkey2(k, len);
  tabkey = strcopy(key, a);
  if (tabkey == 0) return false;

  hash = MurmurHash2(key.str.str, key.str.len, 0);
  insert(&t->t, key, tabkey, v, hash, &strhash, &streql);
  return true;
}

bool upb_strtable_lookup2(const upb_strtable *t, const char *key, size_t len,
                          upb_value *v) {
  uint32_t hash = MurmurHash2(key, len, 0);
  return lookup(&t->t, strkey2(key, len), v, hash, &streql);
}

bool upb_strtable_remove3(upb_strtable *t, const char *key, size_t len,
                         upb_value *val, upb_alloc *alloc) {
  uint32_t hash = MurmurHash2(key, len, 0);
  upb_tabkey tabkey;
  if (rm(&t->t, strkey2(key, len), val, &tabkey, hash, &streql)) {
    upb_free(alloc, (void*)tabkey);
    return true;
  } else {
    return false;
  }
}

/* Iteration */

static const upb_tabent *str_tabent(const upb_strtable_iter *i) {
  return &i->t->t.entries[i->index];
}

void upb_strtable_begin(upb_strtable_iter *i, const upb_strtable *t) {
  i->t = t;
  i->index = begin(&t->t);
}

void upb_strtable_next(upb_strtable_iter *i) {
  i->index = next(&i->t->t, i->index);
}

bool upb_strtable_done(const upb_strtable_iter *i) {
  return i->index >= upb_table_size(&i->t->t) ||
         upb_tabent_isempty(str_tabent(i));
}

const char *upb_strtable_iter_key(const upb_strtable_iter *i) {
  UPB_ASSERT(!upb_strtable_done(i));
  return upb_tabstr(str_tabent(i)->key, NULL);
}

size_t upb_strtable_iter_keylength(const upb_strtable_iter *i) {
  uint32_t len;
  UPB_ASSERT(!upb_strtable_done(i));
  upb_tabstr(str_tabent(i)->key, &len);
  return len;
}

upb_value upb_strtable_iter_value(const upb_strtable_iter *i) {
  UPB_ASSERT(!upb_strtable_done(i));
  return _upb_value_val(str_tabent(i)->val.val, i->t->t.ctype);
}

void upb_strtable_iter_setdone(upb_strtable_iter *i) {
  i->index = SIZE_MAX;
}

bool upb_strtable_iter_isequal(const upb_strtable_iter *i1,
                               const upb_strtable_iter *i2) {
  if (upb_strtable_done(i1) && upb_strtable_done(i2))
    return true;
  return i1->t == i2->t && i1->index == i2->index;
}


/* upb_inttable ***************************************************************/

/* For inttables we use a hybrid structure where small keys are kept in an
 * array and large keys are put in the hash table. */

static uint32_t inthash(upb_tabkey key) { return upb_inthash(key); }

static bool inteql(upb_tabkey k1, lookupkey_t k2) {
  return k1 == k2.num;
}

static upb_tabval *mutable_array(upb_inttable *t) {
  return (upb_tabval*)t->array;
}

static upb_tabval *inttable_val(upb_inttable *t, uintptr_t key) {
  if (key < t->array_size) {
    return upb_arrhas(t->array[key]) ? &(mutable_array(t)[key]) : NULL;
  } else {
    upb_tabent *e =
        findentry_mutable(&t->t, intkey(key), upb_inthash(key), &inteql);
    return e ? &e->val : NULL;
  }
}

static const upb_tabval *inttable_val_const(const upb_inttable *t,
                                            uintptr_t key) {
  return inttable_val((upb_inttable*)t, key);
}

size_t upb_inttable_count(const upb_inttable *t) {
  return t->t.count + t->array_count;
}

static void check(upb_inttable *t) {
  UPB_UNUSED(t);
#if defined(UPB_DEBUG_TABLE) && !defined(NDEBUG)
  {
    /* This check is very expensive (makes inserts/deletes O(N)). */
    size_t count = 0;
    upb_inttable_iter i;
    upb_inttable_begin(&i, t);
    for(; !upb_inttable_done(&i); upb_inttable_next(&i), count++) {
      UPB_ASSERT(upb_inttable_lookup(t, upb_inttable_iter_key(&i), NULL));
    }
    UPB_ASSERT(count == upb_inttable_count(t));
  }
#endif
}

bool upb_inttable_sizedinit(upb_inttable *t, upb_ctype_t ctype,
                            size_t asize, int hsize_lg2, upb_alloc *a) {
  size_t array_bytes;

  if (!init(&t->t, ctype, hsize_lg2, a)) return false;
  /* Always make the array part at least 1 long, so that we know key 0
   * won't be in the hash part, which simplifies things. */
  t->array_size = UPB_MAX(1, asize);
  t->array_count = 0;
  array_bytes = t->array_size * sizeof(upb_value);
  t->array = upb_malloc(a, array_bytes);
  if (!t->array) {
    uninit(&t->t, a);
    return false;
  }
  memset(mutable_array(t), 0xff, array_bytes);
  check(t);
  return true;
}

bool upb_inttable_init2(upb_inttable *t, upb_ctype_t ctype, upb_alloc *a) {
  return upb_inttable_sizedinit(t, ctype, 0, 4, a);
}

void upb_inttable_uninit2(upb_inttable *t, upb_alloc *a) {
  uninit(&t->t, a);
  upb_free(a, mutable_array(t));
}

bool upb_inttable_insert2(upb_inttable *t, uintptr_t key, upb_value val,
                          upb_alloc *a) {
  upb_tabval tabval;
  tabval.val = val.val;
  UPB_ASSERT(upb_arrhas(tabval));  /* This will reject (uint64_t)-1.  Fix this. */

  upb_check_alloc(&t->t, a);

  if (key < t->array_size) {
    UPB_ASSERT(!upb_arrhas(t->array[key]));
    t->array_count++;
    mutable_array(t)[key].val = val.val;
  } else {
    if (isfull(&t->t)) {
      /* Need to resize the hash part, but we re-use the array part. */
      size_t i;
      upb_table new_table;

      if (!init(&new_table, t->t.ctype, t->t.size_lg2 + 1, a)) {
        return false;
      }

      for (i = begin(&t->t); i < upb_table_size(&t->t); i = next(&t->t, i)) {
        const upb_tabent *e = &t->t.entries[i];
        uint32_t hash;
        upb_value v;

        _upb_value_setval(&v, e->val.val, t->t.ctype);
        hash = upb_inthash(e->key);
        insert(&new_table, intkey(e->key), e->key, v, hash, &inthash, &inteql);
      }

      UPB_ASSERT(t->t.count == new_table.count);

      uninit(&t->t, a);
      t->t = new_table;
    }
    insert(&t->t, intkey(key), key, val, upb_inthash(key), &inthash, &inteql);
  }
  check(t);
  return true;
}

bool upb_inttable_lookup(const upb_inttable *t, uintptr_t key, upb_value *v) {
  const upb_tabval *table_v = inttable_val_const(t, key);
  if (!table_v) return false;
  if (v) _upb_value_setval(v, table_v->val, t->t.ctype);
  return true;
}

bool upb_inttable_replace(upb_inttable *t, uintptr_t key, upb_value val) {
  upb_tabval *table_v = inttable_val(t, key);
  if (!table_v) return false;
  table_v->val = val.val;
  return true;
}

bool upb_inttable_remove(upb_inttable *t, uintptr_t key, upb_value *val) {
  bool success;
  if (key < t->array_size) {
    if (upb_arrhas(t->array[key])) {
      upb_tabval empty = UPB_TABVALUE_EMPTY_INIT;
      t->array_count--;
      if (val) {
        _upb_value_setval(val, t->array[key].val, t->t.ctype);
      }
      mutable_array(t)[key] = empty;
      success = true;
    } else {
      success = false;
    }
  } else {
    success = rm(&t->t, intkey(key), val, NULL, upb_inthash(key), &inteql);
  }
  check(t);
  return success;
}

bool upb_inttable_push2(upb_inttable *t, upb_value val, upb_alloc *a) {
  upb_check_alloc(&t->t, a);
  return upb_inttable_insert2(t, upb_inttable_count(t), val, a);
}

upb_value upb_inttable_pop(upb_inttable *t) {
  upb_value val;
  bool ok = upb_inttable_remove(t, upb_inttable_count(t) - 1, &val);
  UPB_ASSERT(ok);
  return val;
}

bool upb_inttable_insertptr2(upb_inttable *t, const void *key, upb_value val,
                             upb_alloc *a) {
  upb_check_alloc(&t->t, a);
  return upb_inttable_insert2(t, (uintptr_t)key, val, a);
}

bool upb_inttable_lookupptr(const upb_inttable *t, const void *key,
                            upb_value *v) {
  return upb_inttable_lookup(t, (uintptr_t)key, v);
}

bool upb_inttable_removeptr(upb_inttable *t, const void *key, upb_value *val) {
  return upb_inttable_remove(t, (uintptr_t)key, val);
}

void upb_inttable_compact2(upb_inttable *t, upb_alloc *a) {
  /* A power-of-two histogram of the table keys. */
  size_t counts[UPB_MAXARRSIZE + 1] = {0};

  /* The max key in each bucket. */
  uintptr_t max[UPB_MAXARRSIZE + 1] = {0};

  upb_inttable_iter i;
  size_t arr_count;
  int size_lg2;
  upb_inttable new_t;

  upb_check_alloc(&t->t, a);

  upb_inttable_begin(&i, t);
  for (; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    uintptr_t key = upb_inttable_iter_key(&i);
    int bucket = log2ceil(key);
    max[bucket] = UPB_MAX(max[bucket], key);
    counts[bucket]++;
  }

  /* Find the largest power of two that satisfies the MIN_DENSITY
   * definition (while actually having some keys). */
  arr_count = upb_inttable_count(t);

  for (size_lg2 = ARRAY_SIZE(counts) - 1; size_lg2 > 0; size_lg2--) {
    if (counts[size_lg2] == 0) {
      /* We can halve again without losing any entries. */
      continue;
    } else if (arr_count >= (1 << size_lg2) * MIN_DENSITY) {
      break;
    }

    arr_count -= counts[size_lg2];
  }

  UPB_ASSERT(arr_count <= upb_inttable_count(t));

  {
    /* Insert all elements into new, perfectly-sized table. */
    size_t arr_size = max[size_lg2] + 1;  /* +1 so arr[max] will fit. */
    size_t hash_count = upb_inttable_count(t) - arr_count;
    size_t hash_size = hash_count ? (hash_count / MAX_LOAD) + 1 : 0;
    size_t hashsize_lg2 = log2ceil(hash_size);

    upb_inttable_sizedinit(&new_t, t->t.ctype, arr_size, hashsize_lg2, a);
    upb_inttable_begin(&i, t);
    for (; !upb_inttable_done(&i); upb_inttable_next(&i)) {
      uintptr_t k = upb_inttable_iter_key(&i);
      upb_inttable_insert2(&new_t, k, upb_inttable_iter_value(&i), a);
    }
    UPB_ASSERT(new_t.array_size == arr_size);
    UPB_ASSERT(new_t.t.size_lg2 == hashsize_lg2);
  }
  upb_inttable_uninit2(t, a);
  *t = new_t;
}

/* Iteration. */

static const upb_tabent *int_tabent(const upb_inttable_iter *i) {
  UPB_ASSERT(!i->array_part);
  return &i->t->t.entries[i->index];
}

static upb_tabval int_arrent(const upb_inttable_iter *i) {
  UPB_ASSERT(i->array_part);
  return i->t->array[i->index];
}

void upb_inttable_begin(upb_inttable_iter *i, const upb_inttable *t) {
  i->t = t;
  i->index = -1;
  i->array_part = true;
  upb_inttable_next(i);
}

void upb_inttable_next(upb_inttable_iter *iter) {
  const upb_inttable *t = iter->t;
  if (iter->array_part) {
    while (++iter->index < t->array_size) {
      if (upb_arrhas(int_arrent(iter))) {
        return;
      }
    }
    iter->array_part = false;
    iter->index = begin(&t->t);
  } else {
    iter->index = next(&t->t, iter->index);
  }
}

bool upb_inttable_done(const upb_inttable_iter *i) {
  if (i->array_part) {
    return i->index >= i->t->array_size ||
           !upb_arrhas(int_arrent(i));
  } else {
    return i->index >= upb_table_size(&i->t->t) ||
           upb_tabent_isempty(int_tabent(i));
  }
}

uintptr_t upb_inttable_iter_key(const upb_inttable_iter *i) {
  UPB_ASSERT(!upb_inttable_done(i));
  return i->array_part ? i->index : int_tabent(i)->key;
}

upb_value upb_inttable_iter_value(const upb_inttable_iter *i) {
  UPB_ASSERT(!upb_inttable_done(i));
  return _upb_value_val(
      i->array_part ? i->t->array[i->index].val : int_tabent(i)->val.val,
      i->t->t.ctype);
}

void upb_inttable_iter_setdone(upb_inttable_iter *i) {
  i->index = SIZE_MAX;
  i->array_part = false;
}

bool upb_inttable_iter_isequal(const upb_inttable_iter *i1,
                                          const upb_inttable_iter *i2) {
  if (upb_inttable_done(i1) && upb_inttable_done(i2))
    return true;
  return i1->t == i2->t && i1->index == i2->index &&
         i1->array_part == i2->array_part;
}

#ifdef UPB_UNALIGNED_READS_OK
/* -----------------------------------------------------------------------------
 * MurmurHash2, by Austin Appleby (released as public domain).
 * Reformatted and C99-ified by Joshua Haberman.
 * Note - This code makes a few assumptions about how your machine behaves -
 *   1. We can read a 4-byte value from any address without crashing
 *   2. sizeof(int) == 4 (in upb this limitation is removed by using uint32_t
 * And it has a few limitations -
 *   1. It will not work incrementally.
 *   2. It will not produce the same results on little-endian and big-endian
 *      machines. */
uint32_t MurmurHash2(const void *key, size_t len, uint32_t seed) {
  /* 'm' and 'r' are mixing constants generated offline.
   * They're not really 'magic', they just happen to work well. */
  const uint32_t m = 0x5bd1e995;
  const int32_t r = 24;

  /* Initialize the hash to a 'random' value */
  uint32_t h = seed ^ len;

  /* Mix 4 bytes at a time into the hash */
  const uint8_t * data = (const uint8_t *)key;
  while(len >= 4) {
    uint32_t k = *(uint32_t *)data;

    k *= m;
    k ^= k >> r;
    k *= m;

    h *= m;
    h ^= k;

    data += 4;
    len -= 4;
  }

  /* Handle the last few bytes of the input array */
  switch(len) {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0]; h *= m;
  };

  /* Do a few final mixes of the hash to ensure the last few
   * bytes are well-incorporated. */
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
}

#else /* !UPB_UNALIGNED_READS_OK */

/* -----------------------------------------------------------------------------
 * MurmurHashAligned2, by Austin Appleby
 * Same algorithm as MurmurHash2, but only does aligned reads - should be safer
 * on certain platforms.
 * Performance will be lower than MurmurHash2 */

#define MIX(h,k,m) { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }

uint32_t MurmurHash2(const void * key, size_t len, uint32_t seed) {
  const uint32_t m = 0x5bd1e995;
  const int32_t r = 24;
  const uint8_t * data = (const uint8_t *)key;
  uint32_t h = seed ^ len;
  uint8_t align = (uintptr_t)data & 3;

  if(align && (len >= 4)) {
    /* Pre-load the temp registers */
    uint32_t t = 0, d = 0;
    int32_t sl;
    int32_t sr;

    switch(align) {
      case 1: t |= data[2] << 16;
      case 2: t |= data[1] << 8;
      case 3: t |= data[0];
    }

    t <<= (8 * align);

    data += 4-align;
    len -= 4-align;

    sl = 8 * (4-align);
    sr = 8 * align;

    /* Mix */

    while(len >= 4) {
      uint32_t k;

      d = *(uint32_t *)data;
      t = (t >> sr) | (d << sl);

      k = t;

      MIX(h,k,m);

      t = d;

      data += 4;
      len -= 4;
    }

    /* Handle leftover data in temp registers */

    d = 0;

    if(len >= align) {
      uint32_t k;

      switch(align) {
        case 3: d |= data[2] << 16;
        case 2: d |= data[1] << 8;
        case 1: d |= data[0];
      }

      k = (t >> sr) | (d << sl);
      MIX(h,k,m);

      data += align;
      len -= align;

      /* ----------
       * Handle tail bytes */

      switch(len) {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0]; h *= m;
      };
    } else {
      switch(len) {
        case 3: d |= data[2] << 16;
        case 2: d |= data[1] << 8;
        case 1: d |= data[0];
        case 0: h ^= (t >> sr) | (d << sl); h *= m;
      }
    }

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
  } else {
    while(len >= 4) {
      uint32_t k = *(uint32_t *)data;

      MIX(h,k,m);

      data += 4;
      len -= 4;
    }

    /* ----------
     * Handle tail bytes */

    switch(len) {
      case 3: h ^= data[2] << 16;
      case 2: h ^= data[1] << 8;
      case 1: h ^= data[0]; h *= m;
    };

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
  }
}
#undef MIX

#endif /* UPB_UNALIGNED_READS_OK */

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool upb_dumptostderr(void *closure, const upb_status* status) {
  UPB_UNUSED(closure);
  fprintf(stderr, "%s\n", upb_status_errmsg(status));
  return false;
}

/* Guarantee null-termination and provide ellipsis truncation.
 * It may be tempting to "optimize" this by initializing these final
 * four bytes up-front and then being careful never to overwrite them,
 * this is safer and simpler. */
static void nullz(upb_status *status) {
  const char *ellipsis = "...";
  size_t len = strlen(ellipsis);
  UPB_ASSERT(sizeof(status->msg) > len);
  memcpy(status->msg + sizeof(status->msg) - len, ellipsis, len);
}


/* upb_upberr *****************************************************************/

upb_errorspace upb_upberr = {"upb error"};

void upb_upberr_setoom(upb_status *status) {
  status->error_space_ = &upb_upberr;
  upb_status_seterrmsg(status, "Out of memory");
}


/* upb_status *****************************************************************/

void upb_status_clear(upb_status *status) {
  if (!status) return;
  status->ok_ = true;
  status->code_ = 0;
  status->msg[0] = '\0';
}

bool upb_ok(const upb_status *status) { return status->ok_; }

upb_errorspace *upb_status_errspace(const upb_status *status) {
  return status->error_space_;
}

int upb_status_errcode(const upb_status *status) { return status->code_; }

const char *upb_status_errmsg(const upb_status *status) { return status->msg; }

void upb_status_seterrmsg(upb_status *status, const char *msg) {
  if (!status) return;
  status->ok_ = false;
  strncpy(status->msg, msg, sizeof(status->msg));
  nullz(status);
}

void upb_status_seterrf(upb_status *status, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  upb_status_vseterrf(status, fmt, args);
  va_end(args);
}

void upb_status_vseterrf(upb_status *status, const char *fmt, va_list args) {
  if (!status) return;
  status->ok_ = false;
  _upb_vsnprintf(status->msg, sizeof(status->msg), fmt, args);
  nullz(status);
}

void upb_status_copy(upb_status *to, const upb_status *from) {
  if (!to) return;
  *to = *from;
}


/* upb_alloc ******************************************************************/

static void *upb_global_allocfunc(upb_alloc *alloc, void *ptr, size_t oldsize,
                                  size_t size) {
  UPB_UNUSED(alloc);
  UPB_UNUSED(oldsize);
  if (size == 0) {
    free(ptr);
    return NULL;
  } else {
    return realloc(ptr, size);
  }
}

upb_alloc upb_alloc_global = {&upb_global_allocfunc};


/* upb_arena ******************************************************************/

/* Be conservative and choose 16 in case anyone is using SSE. */
static const size_t maxalign = 16;

static size_t align_up_max(size_t size) {
  return ((size + maxalign - 1) / maxalign) * maxalign;
}

typedef struct mem_block {
  struct mem_block *next;
  size_t size;
  size_t used;
  bool owned;
  /* Data follows. */
} mem_block;

typedef struct cleanup_ent {
  struct cleanup_ent *next;
  upb_cleanup_func *cleanup;
  void *ud;
} cleanup_ent;

static void upb_arena_addblock(upb_arena *a, void *ptr, size_t size,
                               bool owned) {
  mem_block *block = ptr;

  block->next = a->block_head;
  block->size = size;
  block->used = align_up_max(sizeof(mem_block));
  block->owned = owned;

  a->block_head = block;

  /* TODO(haberman): ASAN poison. */
}


static mem_block *upb_arena_allocblock(upb_arena *a, size_t size) {
  size_t block_size = UPB_MAX(size, a->next_block_size) + sizeof(mem_block);
  mem_block *block = upb_malloc(a->block_alloc, block_size);

  if (!block) {
    return NULL;
  }

  upb_arena_addblock(a, block, block_size, true);
  a->next_block_size = UPB_MIN(block_size * 2, a->max_block_size);

  return block;
}

static void *upb_arena_doalloc(upb_alloc *alloc, void *ptr, size_t oldsize,
                               size_t size) {
  upb_arena *a = (upb_arena*)alloc;  /* upb_alloc is initial member. */
  mem_block *block = a->block_head;
  void *ret;

  if (size == 0) {
    return NULL;  /* We are an arena, don't need individual frees. */
  }

  size = align_up_max(size);

  /* TODO(haberman): special-case if this is a realloc of the last alloc? */

  if (!block || block->size - block->used < size) {
    /* Slow path: have to allocate a new block. */
    block = upb_arena_allocblock(a, size);

    if (!block) {
      return NULL;  /* Out of memory. */
    }
  }

  ret = (char*)block + block->used;
  block->used += size;

  if (oldsize > 0) {
    memcpy(ret, ptr, oldsize);  /* Preserve existing data. */
  }

  /* TODO(haberman): ASAN unpoison. */

  a->bytes_allocated += size;
  return ret;
}

/* Public Arena API ***********************************************************/

void upb_arena_init(upb_arena *a) {
  a->alloc.func = &upb_arena_doalloc;
  a->block_alloc = &upb_alloc_global;
  a->bytes_allocated = 0;
  a->next_block_size = 256;
  a->max_block_size = 16384;
  a->cleanup_head = NULL;
  a->block_head = NULL;
}

void upb_arena_init2(upb_arena *a, void *mem, size_t size, upb_alloc *alloc) {
  upb_arena_init(a);

  if (size > sizeof(mem_block)) {
    upb_arena_addblock(a, mem, size, false);
  }

  if (alloc) {
    a->block_alloc = alloc;
  }
}

void upb_arena_uninit(upb_arena *a) {
  cleanup_ent *ent = a->cleanup_head;
  mem_block *block = a->block_head;

  while (ent) {
    ent->cleanup(ent->ud);
    ent = ent->next;
  }

  /* Must do this after running cleanup functions, because this will delete
   * the memory we store our cleanup entries in! */
  while (block) {
    mem_block *next = block->next;

    if (block->owned) {
      upb_free(a->block_alloc, block);
    }

    block = next;
  }

  /* Protect against multiple-uninit. */
  a->cleanup_head = NULL;
  a->block_head = NULL;
}

bool upb_arena_addcleanup(upb_arena *a, upb_cleanup_func *func, void *ud) {
  cleanup_ent *ent = upb_malloc(&a->alloc, sizeof(cleanup_ent));
  if (!ent) {
    return false;  /* Out of memory. */
  }

  ent->cleanup = func;
  ent->ud = ud;
  ent->next = a->cleanup_head;
  a->cleanup_head = ent;

  return true;
}

size_t upb_arena_bytesallocated(const upb_arena *a) {
  return a->bytes_allocated;
}


/* Standard error functions ***************************************************/

static bool default_err(void *ud, const upb_status *status) {
  UPB_UNUSED(ud);
  UPB_UNUSED(status);
  return false;
}

static bool write_err_to(void *ud, const upb_status *status) {
  upb_status *copy_to = ud;
  upb_status_copy(copy_to, status);
  return false;
}


/* upb_env ********************************************************************/

void upb_env_initonly(upb_env *e) {
  e->ok_ = true;
  e->error_func_ = &default_err;
  e->error_ud_ = NULL;
}

void upb_env_init(upb_env *e) {
  upb_arena_init(&e->arena_);
  upb_env_initonly(e);
}

void upb_env_init2(upb_env *e, void *mem, size_t n, upb_alloc *alloc) {
  upb_arena_init2(&e->arena_, mem, n, alloc);
  upb_env_initonly(e);
}

void upb_env_uninit(upb_env *e) {
  upb_arena_uninit(&e->arena_);
}

void upb_env_seterrorfunc(upb_env *e, upb_error_func *func, void *ud) {
  e->error_func_ = func;
  e->error_ud_ = ud;
}

void upb_env_reporterrorsto(upb_env *e, upb_status *s) {
  e->error_func_ = &write_err_to;
  e->error_ud_ = s;
}

bool upb_env_reporterror(upb_env *e, const upb_status *status) {
  e->ok_ = false;
  return e->error_func_(e->error_ud_, status);
}

void *upb_env_malloc(upb_env *e, size_t size) {
  return upb_malloc(&e->arena_.alloc, size);
}

void *upb_env_realloc(upb_env *e, void *ptr, size_t oldsize, size_t size) {
  return upb_realloc(&e->arena_.alloc, ptr, oldsize, size);
}

void upb_env_free(upb_env *e, void *ptr) {
  upb_free(&e->arena_.alloc, ptr);
}

bool upb_env_addcleanup(upb_env *e, upb_cleanup_func *func, void *ud) {
  return upb_arena_addcleanup(&e->arena_, func, ud);
}

size_t upb_env_bytesallocated(const upb_env *e) {
  return upb_arena_bytesallocated(&e->arena_);
}
/* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     upb/descriptor/descriptor.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */


static const upb_msgdef msgs[22];
static const upb_fielddef fields[107];
static const upb_enumdef enums[5];
static const upb_tabent strentries[236];
static const upb_tabent intentries[18];
static const upb_tabval arrays[187];

#ifdef UPB_DEBUG_REFS
static upb_inttable reftables[268];
#endif

static const upb_msgdef msgs[22] = {
  UPB_MSGDEF_INIT("google.protobuf.DescriptorProto", 41, 8, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[0], 11, 10), UPB_STRTABLE_INIT(10, 15, UPB_CTYPE_PTR, 4, &strentries[0]), false, UPB_SYNTAX_PROTO2, &reftables[0], &reftables[1]),
  UPB_MSGDEF_INIT("google.protobuf.DescriptorProto.ExtensionRange", 5, 0, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[11], 3, 2), UPB_STRTABLE_INIT(2, 3, UPB_CTYPE_PTR, 2, &strentries[16]), false, UPB_SYNTAX_PROTO2, &reftables[2], &reftables[3]),
  UPB_MSGDEF_INIT("google.protobuf.DescriptorProto.ReservedRange", 5, 0, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[14], 3, 2), UPB_STRTABLE_INIT(2, 3, UPB_CTYPE_PTR, 2, &strentries[20]), false, UPB_SYNTAX_PROTO2, &reftables[4], &reftables[5]),
  UPB_MSGDEF_INIT("google.protobuf.EnumDescriptorProto", 12, 2, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[17], 4, 3), UPB_STRTABLE_INIT(3, 3, UPB_CTYPE_PTR, 2, &strentries[24]), false, UPB_SYNTAX_PROTO2, &reftables[6], &reftables[7]),
  UPB_MSGDEF_INIT("google.protobuf.EnumOptions", 9, 1, UPB_INTTABLE_INIT(1, 1, UPB_CTYPE_PTR, 1, &intentries[0], &arrays[21], 4, 2), UPB_STRTABLE_INIT(3, 3, UPB_CTYPE_PTR, 2, &strentries[28]), false, UPB_SYNTAX_PROTO2, &reftables[8], &reftables[9]),
  UPB_MSGDEF_INIT("google.protobuf.EnumValueDescriptorProto", 9, 1, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[25], 4, 3), UPB_STRTABLE_INIT(3, 3, UPB_CTYPE_PTR, 2, &strentries[32]), false, UPB_SYNTAX_PROTO2, &reftables[10], &reftables[11]),
  UPB_MSGDEF_INIT("google.protobuf.EnumValueOptions", 8, 1, UPB_INTTABLE_INIT(1, 1, UPB_CTYPE_PTR, 1, &intentries[2], &arrays[29], 2, 1), UPB_STRTABLE_INIT(2, 3, UPB_CTYPE_PTR, 2, &strentries[36]), false, UPB_SYNTAX_PROTO2, &reftables[12], &reftables[13]),
  UPB_MSGDEF_INIT("google.protobuf.FieldDescriptorProto", 24, 1, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[31], 11, 10), UPB_STRTABLE_INIT(10, 15, UPB_CTYPE_PTR, 4, &strentries[40]), false, UPB_SYNTAX_PROTO2, &reftables[14], &reftables[15]),
  UPB_MSGDEF_INIT("google.protobuf.FieldOptions", 13, 1, UPB_INTTABLE_INIT(1, 1, UPB_CTYPE_PTR, 1, &intentries[4], &arrays[42], 11, 6), UPB_STRTABLE_INIT(7, 15, UPB_CTYPE_PTR, 4, &strentries[56]), false, UPB_SYNTAX_PROTO2, &reftables[16], &reftables[17]),
  UPB_MSGDEF_INIT("google.protobuf.FileDescriptorProto", 43, 6, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[53], 13, 12), UPB_STRTABLE_INIT(12, 15, UPB_CTYPE_PTR, 4, &strentries[72]), false, UPB_SYNTAX_PROTO2, &reftables[18], &reftables[19]),
  UPB_MSGDEF_INIT("google.protobuf.FileDescriptorSet", 7, 1, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[66], 2, 1), UPB_STRTABLE_INIT(1, 3, UPB_CTYPE_PTR, 2, &strentries[88]), false, UPB_SYNTAX_PROTO2, &reftables[20], &reftables[21]),
  UPB_MSGDEF_INIT("google.protobuf.FileOptions", 38, 1, UPB_INTTABLE_INIT(1, 1, UPB_CTYPE_PTR, 1, &intentries[6], &arrays[68], 42, 17), UPB_STRTABLE_INIT(18, 31, UPB_CTYPE_PTR, 5, &strentries[92]), false, UPB_SYNTAX_PROTO2, &reftables[22], &reftables[23]),
  UPB_MSGDEF_INIT("google.protobuf.MessageOptions", 11, 1, UPB_INTTABLE_INIT(1, 1, UPB_CTYPE_PTR, 1, &intentries[8], &arrays[110], 8, 4), UPB_STRTABLE_INIT(5, 7, UPB_CTYPE_PTR, 3, &strentries[124]), false, UPB_SYNTAX_PROTO2, &reftables[24], &reftables[25]),
  UPB_MSGDEF_INIT("google.protobuf.MethodDescriptorProto", 16, 1, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[118], 7, 6), UPB_STRTABLE_INIT(6, 7, UPB_CTYPE_PTR, 3, &strentries[132]), false, UPB_SYNTAX_PROTO2, &reftables[26], &reftables[27]),
  UPB_MSGDEF_INIT("google.protobuf.MethodOptions", 8, 1, UPB_INTTABLE_INIT(2, 3, UPB_CTYPE_PTR, 2, &intentries[10], &arrays[125], 1, 0), UPB_STRTABLE_INIT(2, 3, UPB_CTYPE_PTR, 2, &strentries[140]), false, UPB_SYNTAX_PROTO2, &reftables[28], &reftables[29]),
  UPB_MSGDEF_INIT("google.protobuf.OneofDescriptorProto", 6, 0, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[126], 2, 1), UPB_STRTABLE_INIT(1, 3, UPB_CTYPE_PTR, 2, &strentries[144]), false, UPB_SYNTAX_PROTO2, &reftables[30], &reftables[31]),
  UPB_MSGDEF_INIT("google.protobuf.ServiceDescriptorProto", 12, 2, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[128], 4, 3), UPB_STRTABLE_INIT(3, 3, UPB_CTYPE_PTR, 2, &strentries[148]), false, UPB_SYNTAX_PROTO2, &reftables[32], &reftables[33]),
  UPB_MSGDEF_INIT("google.protobuf.ServiceOptions", 8, 1, UPB_INTTABLE_INIT(2, 3, UPB_CTYPE_PTR, 2, &intentries[14], &arrays[132], 1, 0), UPB_STRTABLE_INIT(2, 3, UPB_CTYPE_PTR, 2, &strentries[152]), false, UPB_SYNTAX_PROTO2, &reftables[34], &reftables[35]),
  UPB_MSGDEF_INIT("google.protobuf.SourceCodeInfo", 7, 1, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[133], 2, 1), UPB_STRTABLE_INIT(1, 3, UPB_CTYPE_PTR, 2, &strentries[156]), false, UPB_SYNTAX_PROTO2, &reftables[36], &reftables[37]),
  UPB_MSGDEF_INIT("google.protobuf.SourceCodeInfo.Location", 20, 0, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[135], 7, 5), UPB_STRTABLE_INIT(5, 7, UPB_CTYPE_PTR, 3, &strentries[160]), false, UPB_SYNTAX_PROTO2, &reftables[38], &reftables[39]),
  UPB_MSGDEF_INIT("google.protobuf.UninterpretedOption", 19, 1, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[142], 9, 7), UPB_STRTABLE_INIT(7, 15, UPB_CTYPE_PTR, 4, &strentries[168]), false, UPB_SYNTAX_PROTO2, &reftables[40], &reftables[41]),
  UPB_MSGDEF_INIT("google.protobuf.UninterpretedOption.NamePart", 7, 0, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[151], 3, 2), UPB_STRTABLE_INIT(2, 3, UPB_CTYPE_PTR, 2, &strentries[184]), false, UPB_SYNTAX_PROTO2, &reftables[42], &reftables[43]),
};

static const upb_fielddef fields[107] = {
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "aggregate_value", 8, &msgs[20], NULL, 16, 6, {0},&reftables[44], &reftables[45]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "allow_alias", 2, &msgs[4], NULL, 7, 1, {0},&reftables[46], &reftables[47]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "cc_enable_arenas", 31, &msgs[11], NULL, 24, 12, {0},&reftables[48], &reftables[49]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "cc_generic_services", 16, &msgs[11], NULL, 18, 6, {0},&reftables[50], &reftables[51]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "client_streaming", 5, &msgs[13], NULL, 14, 4, {0},&reftables[52], &reftables[53]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "csharp_namespace", 37, &msgs[11], NULL, 28, 14, {0},&reftables[54], &reftables[55]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_ENUM, 0, false, false, false, false, "ctype", 1, &msgs[8], (const upb_def*)(&enums[2]), 7, 1, {0},&reftables[56], &reftables[57]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "default_value", 7, &msgs[7], NULL, 17, 7, {0},&reftables[58], &reftables[59]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_STRING, 0, false, false, false, false, "dependency", 3, &msgs[9], NULL, 31, 8, {0},&reftables[60], &reftables[61]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "deprecated", 3, &msgs[8], NULL, 9, 3, {0},&reftables[62], &reftables[63]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "deprecated", 33, &msgs[14], NULL, 7, 1, {0},&reftables[64], &reftables[65]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "deprecated", 3, &msgs[12], NULL, 9, 3, {0},&reftables[66], &reftables[67]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "deprecated", 23, &msgs[11], NULL, 22, 10, {0},&reftables[68], &reftables[69]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "deprecated", 1, &msgs[6], NULL, 7, 1, {0},&reftables[70], &reftables[71]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "deprecated", 3, &msgs[4], NULL, 8, 2, {0},&reftables[72], &reftables[73]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "deprecated", 33, &msgs[17], NULL, 7, 1, {0},&reftables[74], &reftables[75]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_DOUBLE, 0, false, false, false, false, "double_value", 6, &msgs[20], NULL, 12, 4, {0},&reftables[76], &reftables[77]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, false, "end", 2, &msgs[2], NULL, 4, 1, {0},&reftables[78], &reftables[79]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, false, "end", 2, &msgs[1], NULL, 4, 1, {0},&reftables[80], &reftables[81]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "enum_type", 5, &msgs[9], (const upb_def*)(&msgs[3]), 14, 1, {0},&reftables[82], &reftables[83]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "enum_type", 4, &msgs[0], (const upb_def*)(&msgs[3]), 19, 2, {0},&reftables[84], &reftables[85]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "extendee", 2, &msgs[7], NULL, 8, 2, {0},&reftables[86], &reftables[87]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "extension", 6, &msgs[0], (const upb_def*)(&msgs[7]), 25, 4, {0},&reftables[88], &reftables[89]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "extension", 7, &msgs[9], (const upb_def*)(&msgs[7]), 20, 3, {0},&reftables[90], &reftables[91]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "extension_range", 5, &msgs[0], (const upb_def*)(&msgs[1]), 22, 3, {0},&reftables[92], &reftables[93]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "field", 2, &msgs[0], (const upb_def*)(&msgs[7]), 13, 0, {0},&reftables[94], &reftables[95]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "file", 1, &msgs[10], (const upb_def*)(&msgs[9]), 6, 0, {0},&reftables[96], &reftables[97]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "go_package", 11, &msgs[11], NULL, 15, 5, {0},&reftables[98], &reftables[99]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "identifier_value", 3, &msgs[20], NULL, 7, 1, {0},&reftables[100], &reftables[101]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "input_type", 2, &msgs[13], NULL, 8, 2, {0},&reftables[102], &reftables[103]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REQUIRED, UPB_TYPE_BOOL, 0, false, false, false, false, "is_extension", 2, &msgs[21], NULL, 6, 1, {0},&reftables[104], &reftables[105]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "java_generate_equals_and_hash", 20, &msgs[11], NULL, 21, 9, {0},&reftables[106], &reftables[107]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "java_generic_services", 17, &msgs[11], NULL, 19, 7, {0},&reftables[108], &reftables[109]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "java_multiple_files", 10, &msgs[11], NULL, 14, 4, {0},&reftables[110], &reftables[111]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "java_outer_classname", 8, &msgs[11], NULL, 10, 2, {0},&reftables[112], &reftables[113]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "java_package", 1, &msgs[11], NULL, 7, 1, {0},&reftables[114], &reftables[115]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "java_string_check_utf8", 27, &msgs[11], NULL, 23, 11, {0},&reftables[116], &reftables[117]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "javanano_use_deprecated_package", 38, &msgs[11], NULL, 31, 15, {0},&reftables[118], &reftables[119]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "json_name", 10, &msgs[7], NULL, 21, 9, {0},&reftables[120], &reftables[121]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_ENUM, 0, false, false, false, false, "jstype", 6, &msgs[8], (const upb_def*)(&enums[3]), 11, 5, {0},&reftables[122], &reftables[123]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_ENUM, 0, false, false, false, false, "label", 4, &msgs[7], (const upb_def*)(&enums[0]), 12, 4, {0},&reftables[124], &reftables[125]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "lazy", 5, &msgs[8], NULL, 10, 4, {0},&reftables[126], &reftables[127]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "leading_comments", 3, &msgs[19], NULL, 9, 2, {0},&reftables[128], &reftables[129]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_STRING, 0, false, false, false, false, "leading_detached_comments", 6, &msgs[19], NULL, 17, 4, {0},&reftables[130], &reftables[131]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "location", 1, &msgs[18], (const upb_def*)(&msgs[19]), 6, 0, {0},&reftables[132], &reftables[133]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "map_entry", 7, &msgs[12], NULL, 10, 4, {0},&reftables[134], &reftables[135]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "message_set_wire_format", 1, &msgs[12], NULL, 7, 1, {0},&reftables[136], &reftables[137]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "message_type", 4, &msgs[9], (const upb_def*)(&msgs[0]), 11, 0, {0},&reftables[138], &reftables[139]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "method", 2, &msgs[16], (const upb_def*)(&msgs[13]), 7, 0, {0},&reftables[140], &reftables[141]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "name", 2, &msgs[20], (const upb_def*)(&msgs[21]), 6, 0, {0},&reftables[142], &reftables[143]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "name", 1, &msgs[5], NULL, 5, 1, {0},&reftables[144], &reftables[145]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "name", 1, &msgs[9], NULL, 23, 6, {0},&reftables[146], &reftables[147]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "name", 1, &msgs[3], NULL, 9, 2, {0},&reftables[148], &reftables[149]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "name", 1, &msgs[16], NULL, 9, 2, {0},&reftables[150], &reftables[151]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "name", 1, &msgs[15], NULL, 3, 0, {0},&reftables[152], &reftables[153]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "name", 1, &msgs[13], NULL, 5, 1, {0},&reftables[154], &reftables[155]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "name", 1, &msgs[7], NULL, 5, 1, {0},&reftables[156], &reftables[157]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "name", 1, &msgs[0], NULL, 33, 8, {0},&reftables[158], &reftables[159]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REQUIRED, UPB_TYPE_STRING, 0, false, false, false, false, "name_part", 1, &msgs[21], NULL, 3, 0, {0},&reftables[160], &reftables[161]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_INT64, UPB_INTFMT_VARIABLE, false, false, false, false, "negative_int_value", 5, &msgs[20], NULL, 11, 3, {0},&reftables[162], &reftables[163]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "nested_type", 3, &msgs[0], (const upb_def*)(&msgs[0]), 16, 1, {0},&reftables[164], &reftables[165]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "no_standard_descriptor_accessor", 2, &msgs[12], NULL, 8, 2, {0},&reftables[166], &reftables[167]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, false, "number", 3, &msgs[7], NULL, 11, 3, {0},&reftables[168], &reftables[169]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, false, "number", 2, &msgs[5], NULL, 8, 2, {0},&reftables[170], &reftables[171]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "objc_class_prefix", 36, &msgs[11], NULL, 25, 13, {0},&reftables[172], &reftables[173]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "oneof_decl", 8, &msgs[0], (const upb_def*)(&msgs[15]), 29, 6, {0},&reftables[174], &reftables[175]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, false, "oneof_index", 9, &msgs[7], NULL, 20, 8, {0},&reftables[176], &reftables[177]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_ENUM, 0, false, false, false, false, "optimize_for", 9, &msgs[11], (const upb_def*)(&enums[4]), 13, 3, {0},&reftables[178], &reftables[179]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_MESSAGE, 0, false, false, false, false, "options", 7, &msgs[0], (const upb_def*)(&msgs[12]), 26, 5, {0},&reftables[180], &reftables[181]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_MESSAGE, 0, false, false, false, false, "options", 8, &msgs[9], (const upb_def*)(&msgs[11]), 21, 4, {0},&reftables[182], &reftables[183]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_MESSAGE, 0, false, false, false, false, "options", 8, &msgs[7], (const upb_def*)(&msgs[8]), 4, 0, {0},&reftables[184], &reftables[185]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_MESSAGE, 0, false, false, false, false, "options", 4, &msgs[13], (const upb_def*)(&msgs[14]), 4, 0, {0},&reftables[186], &reftables[187]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_MESSAGE, 0, false, false, false, false, "options", 3, &msgs[16], (const upb_def*)(&msgs[17]), 8, 1, {0},&reftables[188], &reftables[189]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_MESSAGE, 0, false, false, false, false, "options", 3, &msgs[3], (const upb_def*)(&msgs[4]), 8, 1, {0},&reftables[190], &reftables[191]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_MESSAGE, 0, false, false, false, false, "options", 3, &msgs[5], (const upb_def*)(&msgs[6]), 4, 0, {0},&reftables[192], &reftables[193]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "output_type", 3, &msgs[13], NULL, 11, 3, {0},&reftables[194], &reftables[195]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "package", 2, &msgs[9], NULL, 26, 7, {0},&reftables[196], &reftables[197]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "packed", 2, &msgs[8], NULL, 8, 2, {0},&reftables[198], &reftables[199]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, true, "path", 1, &msgs[19], NULL, 5, 0, {0},&reftables[200], &reftables[201]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "php_class_prefix", 40, &msgs[11], NULL, 32, 16, {0},&reftables[202], &reftables[203]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "php_namespace", 41, &msgs[11], NULL, 35, 17, {0},&reftables[204], &reftables[205]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_UINT64, UPB_INTFMT_VARIABLE, false, false, false, false, "positive_int_value", 4, &msgs[20], NULL, 10, 2, {0},&reftables[206], &reftables[207]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, false, "public_dependency", 10, &msgs[9], NULL, 36, 9, {0},&reftables[208], &reftables[209]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "py_generic_services", 18, &msgs[11], NULL, 20, 8, {0},&reftables[210], &reftables[211]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_STRING, 0, false, false, false, false, "reserved_name", 10, &msgs[0], NULL, 38, 9, {0},&reftables[212], &reftables[213]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "reserved_range", 9, &msgs[0], (const upb_def*)(&msgs[2]), 32, 7, {0},&reftables[214], &reftables[215]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "server_streaming", 6, &msgs[13], NULL, 15, 5, {0},&reftables[216], &reftables[217]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "service", 6, &msgs[9], (const upb_def*)(&msgs[16]), 17, 2, {0},&reftables[218], &reftables[219]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_MESSAGE, 0, false, false, false, false, "source_code_info", 9, &msgs[9], (const upb_def*)(&msgs[18]), 22, 5, {0},&reftables[220], &reftables[221]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, true, "span", 2, &msgs[19], NULL, 8, 1, {0},&reftables[222], &reftables[223]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, false, "start", 1, &msgs[2], NULL, 3, 0, {0},&reftables[224], &reftables[225]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, false, "start", 1, &msgs[1], NULL, 3, 0, {0},&reftables[226], &reftables[227]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BYTES, 0, false, false, false, false, "string_value", 7, &msgs[20], NULL, 13, 5, {0},&reftables[228], &reftables[229]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "syntax", 12, &msgs[9], NULL, 40, 11, {0},&reftables[230], &reftables[231]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "trailing_comments", 4, &msgs[19], NULL, 12, 3, {0},&reftables[232], &reftables[233]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_ENUM, 0, false, false, false, false, "type", 5, &msgs[7], (const upb_def*)(&enums[1]), 13, 5, {0},&reftables[234], &reftables[235]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "type_name", 6, &msgs[7], NULL, 14, 6, {0},&reftables[236], &reftables[237]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "uninterpreted_option", 999, &msgs[12], (const upb_def*)(&msgs[20]), 6, 0, {0},&reftables[238], &reftables[239]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "uninterpreted_option", 999, &msgs[17], (const upb_def*)(&msgs[20]), 6, 0, {0},&reftables[240], &reftables[241]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "uninterpreted_option", 999, &msgs[11], (const upb_def*)(&msgs[20]), 6, 0, {0},&reftables[242], &reftables[243]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "uninterpreted_option", 999, &msgs[14], (const upb_def*)(&msgs[20]), 6, 0, {0},&reftables[244], &reftables[245]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "uninterpreted_option", 999, &msgs[8], (const upb_def*)(&msgs[20]), 6, 0, {0},&reftables[246], &reftables[247]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "uninterpreted_option", 999, &msgs[6], (const upb_def*)(&msgs[20]), 6, 0, {0},&reftables[248], &reftables[249]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "uninterpreted_option", 999, &msgs[4], (const upb_def*)(&msgs[20]), 6, 0, {0},&reftables[250], &reftables[251]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "value", 2, &msgs[3], (const upb_def*)(&msgs[5]), 7, 0, {0},&reftables[252], &reftables[253]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "weak", 10, &msgs[8], NULL, 12, 6, {0},&reftables[254], &reftables[255]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, false, "weak_dependency", 11, &msgs[9], NULL, 39, 10, {0},&reftables[256], &reftables[257]),
};

static const upb_enumdef enums[5] = {
  UPB_ENUMDEF_INIT("google.protobuf.FieldDescriptorProto.Label", UPB_STRTABLE_INIT(3, 3, UPB_CTYPE_INT32, 2, &strentries[188]), UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_CSTR, 0, NULL, &arrays[154], 4, 3), 0, &reftables[258], &reftables[259]),
  UPB_ENUMDEF_INIT("google.protobuf.FieldDescriptorProto.Type", UPB_STRTABLE_INIT(18, 31, UPB_CTYPE_INT32, 5, &strentries[192]), UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_CSTR, 0, NULL, &arrays[158], 19, 18), 0, &reftables[260], &reftables[261]),
  UPB_ENUMDEF_INIT("google.protobuf.FieldOptions.CType", UPB_STRTABLE_INIT(3, 3, UPB_CTYPE_INT32, 2, &strentries[224]), UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_CSTR, 0, NULL, &arrays[177], 3, 3), 0, &reftables[262], &reftables[263]),
  UPB_ENUMDEF_INIT("google.protobuf.FieldOptions.JSType", UPB_STRTABLE_INIT(3, 3, UPB_CTYPE_INT32, 2, &strentries[228]), UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_CSTR, 0, NULL, &arrays[180], 3, 3), 0, &reftables[264], &reftables[265]),
  UPB_ENUMDEF_INIT("google.protobuf.FileOptions.OptimizeMode", UPB_STRTABLE_INIT(3, 3, UPB_CTYPE_INT32, 2, &strentries[232]), UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_CSTR, 0, NULL, &arrays[183], 4, 3), 0, &reftables[266], &reftables[267]),
};

static const upb_tabent strentries[236] = {
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "extension"), UPB_TABVALUE_PTR_INIT(&fields[22]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\015", "\000", "\000", "\000", "reserved_name"), UPB_TABVALUE_PTR_INIT(&fields[84]), NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[57]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\005", "\000", "\000", "\000", "field"), UPB_TABVALUE_PTR_INIT(&fields[25]), &strentries[12]},
  {UPB_TABKEY_STR("\017", "\000", "\000", "\000", "extension_range"), UPB_TABVALUE_PTR_INIT(&fields[24]), &strentries[14]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "nested_type"), UPB_TABVALUE_PTR_INIT(&fields[60]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\016", "\000", "\000", "\000", "reserved_range"), UPB_TABVALUE_PTR_INIT(&fields[85]), NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "options"), UPB_TABVALUE_PTR_INIT(&fields[68]), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "oneof_decl"), UPB_TABVALUE_PTR_INIT(&fields[65]), NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "enum_type"), UPB_TABVALUE_PTR_INIT(&fields[20]), &strentries[13]},
  {UPB_TABKEY_STR("\005", "\000", "\000", "\000", "start"), UPB_TABVALUE_PTR_INIT(&fields[91]), NULL},
  {UPB_TABKEY_STR("\003", "\000", "\000", "\000", "end"), UPB_TABVALUE_PTR_INIT(&fields[18]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\005", "\000", "\000", "\000", "start"), UPB_TABVALUE_PTR_INIT(&fields[90]), NULL},
  {UPB_TABKEY_STR("\003", "\000", "\000", "\000", "end"), UPB_TABVALUE_PTR_INIT(&fields[17]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\005", "\000", "\000", "\000", "value"), UPB_TABVALUE_PTR_INIT(&fields[104]), NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "options"), UPB_TABVALUE_PTR_INIT(&fields[73]), NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[52]), &strentries[26]},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "uninterpreted_option"), UPB_TABVALUE_PTR_INIT(&fields[103]), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "deprecated"), UPB_TABVALUE_PTR_INIT(&fields[14]), NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "allow_alias"), UPB_TABVALUE_PTR_INIT(&fields[1]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\006", "\000", "\000", "\000", "number"), UPB_TABVALUE_PTR_INIT(&fields[63]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "options"), UPB_TABVALUE_PTR_INIT(&fields[74]), NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[50]), &strentries[34]},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "uninterpreted_option"), UPB_TABVALUE_PTR_INIT(&fields[102]), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "deprecated"), UPB_TABVALUE_PTR_INIT(&fields[13]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "oneof_index"), UPB_TABVALUE_PTR_INIT(&fields[66]), NULL},
  {UPB_TABKEY_STR("\005", "\000", "\000", "\000", "label"), UPB_TABVALUE_PTR_INIT(&fields[40]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[56]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\006", "\000", "\000", "\000", "number"), UPB_TABVALUE_PTR_INIT(&fields[62]), &strentries[53]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\010", "\000", "\000", "\000", "extendee"), UPB_TABVALUE_PTR_INIT(&fields[21]), NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "type_name"), UPB_TABVALUE_PTR_INIT(&fields[96]), NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "json_name"), UPB_TABVALUE_PTR_INIT(&fields[38]), NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "type"), UPB_TABVALUE_PTR_INIT(&fields[95]), &strentries[50]},
  {UPB_TABKEY_STR("\015", "\000", "\000", "\000", "default_value"), UPB_TABVALUE_PTR_INIT(&fields[7]), NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "options"), UPB_TABVALUE_PTR_INIT(&fields[70]), NULL},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "uninterpreted_option"), UPB_TABVALUE_PTR_INIT(&fields[101]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "weak"), UPB_TABVALUE_PTR_INIT(&fields[105]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\006", "\000", "\000", "\000", "packed"), UPB_TABVALUE_PTR_INIT(&fields[77]), NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "lazy"), UPB_TABVALUE_PTR_INIT(&fields[41]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\005", "\000", "\000", "\000", "ctype"), UPB_TABVALUE_PTR_INIT(&fields[6]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\006", "\000", "\000", "\000", "jstype"), UPB_TABVALUE_PTR_INIT(&fields[39]), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "deprecated"), UPB_TABVALUE_PTR_INIT(&fields[9]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "extension"), UPB_TABVALUE_PTR_INIT(&fields[23]), NULL},
  {UPB_TABKEY_STR("\017", "\000", "\000", "\000", "weak_dependency"), UPB_TABVALUE_PTR_INIT(&fields[106]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[51]), NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "service"), UPB_TABVALUE_PTR_INIT(&fields[87]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\020", "\000", "\000", "\000", "source_code_info"), UPB_TABVALUE_PTR_INIT(&fields[88]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\006", "\000", "\000", "\000", "syntax"), UPB_TABVALUE_PTR_INIT(&fields[93]), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "dependency"), UPB_TABVALUE_PTR_INIT(&fields[8]), NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "message_type"), UPB_TABVALUE_PTR_INIT(&fields[47]), NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "package"), UPB_TABVALUE_PTR_INIT(&fields[76]), NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "options"), UPB_TABVALUE_PTR_INIT(&fields[69]), &strentries[86]},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "enum_type"), UPB_TABVALUE_PTR_INIT(&fields[19]), NULL},
  {UPB_TABKEY_STR("\021", "\000", "\000", "\000", "public_dependency"), UPB_TABVALUE_PTR_INIT(&fields[82]), &strentries[85]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "file"), UPB_TABVALUE_PTR_INIT(&fields[26]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\023", "\000", "\000", "\000", "cc_generic_services"), UPB_TABVALUE_PTR_INIT(&fields[3]), NULL},
  {UPB_TABKEY_STR("\020", "\000", "\000", "\000", "csharp_namespace"), UPB_TABVALUE_PTR_INIT(&fields[5]), &strentries[116]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "go_package"), UPB_TABVALUE_PTR_INIT(&fields[27]), NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "java_package"), UPB_TABVALUE_PTR_INIT(&fields[35]), &strentries[120]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "java_outer_classname"), UPB_TABVALUE_PTR_INIT(&fields[34]), NULL},
  {UPB_TABKEY_STR("\015", "\000", "\000", "\000", "php_namespace"), UPB_TABVALUE_PTR_INIT(&fields[80]), &strentries[113]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\023", "\000", "\000", "\000", "java_multiple_files"), UPB_TABVALUE_PTR_INIT(&fields[33]), &strentries[117]},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "uninterpreted_option"), UPB_TABVALUE_PTR_INIT(&fields[99]), NULL},
  {UPB_TABKEY_STR("\025", "\000", "\000", "\000", "java_generic_services"), UPB_TABVALUE_PTR_INIT(&fields[32]), &strentries[118]},
  {UPB_TABKEY_STR("\035", "\000", "\000", "\000", "java_generate_equals_and_hash"), UPB_TABVALUE_PTR_INIT(&fields[31]), NULL},
  {UPB_TABKEY_STR("\020", "\000", "\000", "\000", "php_class_prefix"), UPB_TABVALUE_PTR_INIT(&fields[79]), NULL},
  {UPB_TABKEY_STR("\037", "\000", "\000", "\000", "javanano_use_deprecated_package"), UPB_TABVALUE_PTR_INIT(&fields[37]), &strentries[123]},
  {UPB_TABKEY_STR("\023", "\000", "\000", "\000", "py_generic_services"), UPB_TABVALUE_PTR_INIT(&fields[83]), NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "optimize_for"), UPB_TABVALUE_PTR_INIT(&fields[67]), NULL},
  {UPB_TABKEY_STR("\026", "\000", "\000", "\000", "java_string_check_utf8"), UPB_TABVALUE_PTR_INIT(&fields[36]), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "deprecated"), UPB_TABVALUE_PTR_INIT(&fields[12]), &strentries[119]},
  {UPB_TABKEY_STR("\021", "\000", "\000", "\000", "objc_class_prefix"), UPB_TABVALUE_PTR_INIT(&fields[64]), NULL},
  {UPB_TABKEY_STR("\020", "\000", "\000", "\000", "cc_enable_arenas"), UPB_TABVALUE_PTR_INIT(&fields[2]), NULL},
  {UPB_TABKEY_STR("\027", "\000", "\000", "\000", "message_set_wire_format"), UPB_TABVALUE_PTR_INIT(&fields[46]), &strentries[128]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "uninterpreted_option"), UPB_TABVALUE_PTR_INIT(&fields[97]), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "deprecated"), UPB_TABVALUE_PTR_INIT(&fields[11]), NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "map_entry"), UPB_TABVALUE_PTR_INIT(&fields[45]), NULL},
  {UPB_TABKEY_STR("\037", "\000", "\000", "\000", "no_standard_descriptor_accessor"), UPB_TABVALUE_PTR_INIT(&fields[61]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\020", "\000", "\000", "\000", "client_streaming"), UPB_TABVALUE_PTR_INIT(&fields[4]), NULL},
  {UPB_TABKEY_STR("\020", "\000", "\000", "\000", "server_streaming"), UPB_TABVALUE_PTR_INIT(&fields[86]), NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[55]), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "input_type"), UPB_TABVALUE_PTR_INIT(&fields[29]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "output_type"), UPB_TABVALUE_PTR_INIT(&fields[75]), NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "options"), UPB_TABVALUE_PTR_INIT(&fields[71]), NULL},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "uninterpreted_option"), UPB_TABVALUE_PTR_INIT(&fields[100]), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "deprecated"), UPB_TABVALUE_PTR_INIT(&fields[10]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[54]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "options"), UPB_TABVALUE_PTR_INIT(&fields[72]), &strentries[150]},
  {UPB_TABKEY_STR("\006", "\000", "\000", "\000", "method"), UPB_TABVALUE_PTR_INIT(&fields[48]), NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[53]), &strentries[149]},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "uninterpreted_option"), UPB_TABVALUE_PTR_INIT(&fields[98]), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "deprecated"), UPB_TABVALUE_PTR_INIT(&fields[15]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\010", "\000", "\000", "\000", "location"), UPB_TABVALUE_PTR_INIT(&fields[44]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "span"), UPB_TABVALUE_PTR_INIT(&fields[89]), &strentries[167]},
  {UPB_TABKEY_STR("\031", "\000", "\000", "\000", "leading_detached_comments"), UPB_TABVALUE_PTR_INIT(&fields[43]), &strentries[165]},
  {UPB_TABKEY_STR("\021", "\000", "\000", "\000", "trailing_comments"), UPB_TABVALUE_PTR_INIT(&fields[94]), NULL},
  {UPB_TABKEY_STR("\020", "\000", "\000", "\000", "leading_comments"), UPB_TABVALUE_PTR_INIT(&fields[42]), &strentries[164]},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "path"), UPB_TABVALUE_PTR_INIT(&fields[78]), NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "double_value"), UPB_TABVALUE_PTR_INIT(&fields[16]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[49]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\022", "\000", "\000", "\000", "negative_int_value"), UPB_TABVALUE_PTR_INIT(&fields[59]), NULL},
  {UPB_TABKEY_STR("\017", "\000", "\000", "\000", "aggregate_value"), UPB_TABVALUE_PTR_INIT(&fields[0]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\022", "\000", "\000", "\000", "positive_int_value"), UPB_TABVALUE_PTR_INIT(&fields[81]), NULL},
  {UPB_TABKEY_STR("\020", "\000", "\000", "\000", "identifier_value"), UPB_TABVALUE_PTR_INIT(&fields[28]), NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "string_value"), UPB_TABVALUE_PTR_INIT(&fields[92]), &strentries[182]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "is_extension"), UPB_TABVALUE_PTR_INIT(&fields[30]), NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "name_part"), UPB_TABVALUE_PTR_INIT(&fields[58]), NULL},
  {UPB_TABKEY_STR("\016", "\000", "\000", "\000", "LABEL_REQUIRED"), UPB_TABVALUE_INT_INIT(2), &strentries[190]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\016", "\000", "\000", "\000", "LABEL_REPEATED"), UPB_TABVALUE_INT_INIT(3), NULL},
  {UPB_TABKEY_STR("\016", "\000", "\000", "\000", "LABEL_OPTIONAL"), UPB_TABVALUE_INT_INIT(1), NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "TYPE_FIXED64"), UPB_TABVALUE_INT_INIT(6), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "TYPE_STRING"), UPB_TABVALUE_INT_INIT(9), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "TYPE_FLOAT"), UPB_TABVALUE_INT_INIT(2), &strentries[221]},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "TYPE_DOUBLE"), UPB_TABVALUE_INT_INIT(1), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "TYPE_INT32"), UPB_TABVALUE_INT_INIT(5), NULL},
  {UPB_TABKEY_STR("\015", "\000", "\000", "\000", "TYPE_SFIXED32"), UPB_TABVALUE_INT_INIT(15), NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "TYPE_FIXED32"), UPB_TABVALUE_INT_INIT(7), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "TYPE_MESSAGE"), UPB_TABVALUE_INT_INIT(11), &strentries[222]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "TYPE_INT64"), UPB_TABVALUE_INT_INIT(3), &strentries[219]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "TYPE_ENUM"), UPB_TABVALUE_INT_INIT(14), NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "TYPE_UINT32"), UPB_TABVALUE_INT_INIT(13), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "TYPE_UINT64"), UPB_TABVALUE_INT_INIT(4), &strentries[218]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\015", "\000", "\000", "\000", "TYPE_SFIXED64"), UPB_TABVALUE_INT_INIT(16), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "TYPE_BYTES"), UPB_TABVALUE_INT_INIT(12), NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "TYPE_SINT64"), UPB_TABVALUE_INT_INIT(18), NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "TYPE_BOOL"), UPB_TABVALUE_INT_INIT(8), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "TYPE_GROUP"), UPB_TABVALUE_INT_INIT(10), NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "TYPE_SINT32"), UPB_TABVALUE_INT_INIT(17), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "CORD"), UPB_TABVALUE_INT_INIT(1), NULL},
  {UPB_TABKEY_STR("\006", "\000", "\000", "\000", "STRING"), UPB_TABVALUE_INT_INIT(0), &strentries[225]},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "STRING_PIECE"), UPB_TABVALUE_INT_INIT(2), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "JS_NORMAL"), UPB_TABVALUE_INT_INIT(0), NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "JS_NUMBER"), UPB_TABVALUE_INT_INIT(2), NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "JS_STRING"), UPB_TABVALUE_INT_INIT(1), NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "CODE_SIZE"), UPB_TABVALUE_INT_INIT(2), NULL},
  {UPB_TABKEY_STR("\005", "\000", "\000", "\000", "SPEED"), UPB_TABVALUE_INT_INIT(1), &strentries[235]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "LITE_RUNTIME"), UPB_TABVALUE_INT_INIT(3), NULL},
};

static const upb_tabent intentries[18] = {
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NUM(999), UPB_TABVALUE_PTR_INIT(&fields[103]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NUM(999), UPB_TABVALUE_PTR_INIT(&fields[102]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NUM(999), UPB_TABVALUE_PTR_INIT(&fields[101]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NUM(999), UPB_TABVALUE_PTR_INIT(&fields[99]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NUM(999), UPB_TABVALUE_PTR_INIT(&fields[97]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NUM(33), UPB_TABVALUE_PTR_INIT(&fields[10]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NUM(999), UPB_TABVALUE_PTR_INIT(&fields[100]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NUM(33), UPB_TABVALUE_PTR_INIT(&fields[15]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NUM(999), UPB_TABVALUE_PTR_INIT(&fields[98]), NULL},
};

static const upb_tabval arrays[187] = {
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[57]),
  UPB_TABVALUE_PTR_INIT(&fields[25]),
  UPB_TABVALUE_PTR_INIT(&fields[60]),
  UPB_TABVALUE_PTR_INIT(&fields[20]),
  UPB_TABVALUE_PTR_INIT(&fields[24]),
  UPB_TABVALUE_PTR_INIT(&fields[22]),
  UPB_TABVALUE_PTR_INIT(&fields[68]),
  UPB_TABVALUE_PTR_INIT(&fields[65]),
  UPB_TABVALUE_PTR_INIT(&fields[85]),
  UPB_TABVALUE_PTR_INIT(&fields[84]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[91]),
  UPB_TABVALUE_PTR_INIT(&fields[18]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[90]),
  UPB_TABVALUE_PTR_INIT(&fields[17]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[52]),
  UPB_TABVALUE_PTR_INIT(&fields[104]),
  UPB_TABVALUE_PTR_INIT(&fields[73]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[1]),
  UPB_TABVALUE_PTR_INIT(&fields[14]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[50]),
  UPB_TABVALUE_PTR_INIT(&fields[63]),
  UPB_TABVALUE_PTR_INIT(&fields[74]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[13]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[56]),
  UPB_TABVALUE_PTR_INIT(&fields[21]),
  UPB_TABVALUE_PTR_INIT(&fields[62]),
  UPB_TABVALUE_PTR_INIT(&fields[40]),
  UPB_TABVALUE_PTR_INIT(&fields[95]),
  UPB_TABVALUE_PTR_INIT(&fields[96]),
  UPB_TABVALUE_PTR_INIT(&fields[7]),
  UPB_TABVALUE_PTR_INIT(&fields[70]),
  UPB_TABVALUE_PTR_INIT(&fields[66]),
  UPB_TABVALUE_PTR_INIT(&fields[38]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[6]),
  UPB_TABVALUE_PTR_INIT(&fields[77]),
  UPB_TABVALUE_PTR_INIT(&fields[9]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[41]),
  UPB_TABVALUE_PTR_INIT(&fields[39]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[105]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[51]),
  UPB_TABVALUE_PTR_INIT(&fields[76]),
  UPB_TABVALUE_PTR_INIT(&fields[8]),
  UPB_TABVALUE_PTR_INIT(&fields[47]),
  UPB_TABVALUE_PTR_INIT(&fields[19]),
  UPB_TABVALUE_PTR_INIT(&fields[87]),
  UPB_TABVALUE_PTR_INIT(&fields[23]),
  UPB_TABVALUE_PTR_INIT(&fields[69]),
  UPB_TABVALUE_PTR_INIT(&fields[88]),
  UPB_TABVALUE_PTR_INIT(&fields[82]),
  UPB_TABVALUE_PTR_INIT(&fields[106]),
  UPB_TABVALUE_PTR_INIT(&fields[93]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[26]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[35]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[34]),
  UPB_TABVALUE_PTR_INIT(&fields[67]),
  UPB_TABVALUE_PTR_INIT(&fields[33]),
  UPB_TABVALUE_PTR_INIT(&fields[27]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[3]),
  UPB_TABVALUE_PTR_INIT(&fields[32]),
  UPB_TABVALUE_PTR_INIT(&fields[83]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[31]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[12]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[36]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[2]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[64]),
  UPB_TABVALUE_PTR_INIT(&fields[5]),
  UPB_TABVALUE_PTR_INIT(&fields[37]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[79]),
  UPB_TABVALUE_PTR_INIT(&fields[80]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[46]),
  UPB_TABVALUE_PTR_INIT(&fields[61]),
  UPB_TABVALUE_PTR_INIT(&fields[11]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[45]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[55]),
  UPB_TABVALUE_PTR_INIT(&fields[29]),
  UPB_TABVALUE_PTR_INIT(&fields[75]),
  UPB_TABVALUE_PTR_INIT(&fields[71]),
  UPB_TABVALUE_PTR_INIT(&fields[4]),
  UPB_TABVALUE_PTR_INIT(&fields[86]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[54]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[53]),
  UPB_TABVALUE_PTR_INIT(&fields[48]),
  UPB_TABVALUE_PTR_INIT(&fields[72]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[44]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[78]),
  UPB_TABVALUE_PTR_INIT(&fields[89]),
  UPB_TABVALUE_PTR_INIT(&fields[42]),
  UPB_TABVALUE_PTR_INIT(&fields[94]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[43]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[49]),
  UPB_TABVALUE_PTR_INIT(&fields[28]),
  UPB_TABVALUE_PTR_INIT(&fields[81]),
  UPB_TABVALUE_PTR_INIT(&fields[59]),
  UPB_TABVALUE_PTR_INIT(&fields[16]),
  UPB_TABVALUE_PTR_INIT(&fields[92]),
  UPB_TABVALUE_PTR_INIT(&fields[0]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[58]),
  UPB_TABVALUE_PTR_INIT(&fields[30]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT("LABEL_OPTIONAL"),
  UPB_TABVALUE_PTR_INIT("LABEL_REQUIRED"),
  UPB_TABVALUE_PTR_INIT("LABEL_REPEATED"),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT("TYPE_DOUBLE"),
  UPB_TABVALUE_PTR_INIT("TYPE_FLOAT"),
  UPB_TABVALUE_PTR_INIT("TYPE_INT64"),
  UPB_TABVALUE_PTR_INIT("TYPE_UINT64"),
  UPB_TABVALUE_PTR_INIT("TYPE_INT32"),
  UPB_TABVALUE_PTR_INIT("TYPE_FIXED64"),
  UPB_TABVALUE_PTR_INIT("TYPE_FIXED32"),
  UPB_TABVALUE_PTR_INIT("TYPE_BOOL"),
  UPB_TABVALUE_PTR_INIT("TYPE_STRING"),
  UPB_TABVALUE_PTR_INIT("TYPE_GROUP"),
  UPB_TABVALUE_PTR_INIT("TYPE_MESSAGE"),
  UPB_TABVALUE_PTR_INIT("TYPE_BYTES"),
  UPB_TABVALUE_PTR_INIT("TYPE_UINT32"),
  UPB_TABVALUE_PTR_INIT("TYPE_ENUM"),
  UPB_TABVALUE_PTR_INIT("TYPE_SFIXED32"),
  UPB_TABVALUE_PTR_INIT("TYPE_SFIXED64"),
  UPB_TABVALUE_PTR_INIT("TYPE_SINT32"),
  UPB_TABVALUE_PTR_INIT("TYPE_SINT64"),
  UPB_TABVALUE_PTR_INIT("STRING"),
  UPB_TABVALUE_PTR_INIT("CORD"),
  UPB_TABVALUE_PTR_INIT("STRING_PIECE"),
  UPB_TABVALUE_PTR_INIT("JS_NORMAL"),
  UPB_TABVALUE_PTR_INIT("JS_STRING"),
  UPB_TABVALUE_PTR_INIT("JS_NUMBER"),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT("SPEED"),
  UPB_TABVALUE_PTR_INIT("CODE_SIZE"),
  UPB_TABVALUE_PTR_INIT("LITE_RUNTIME"),
};

#ifdef UPB_DEBUG_REFS
static upb_inttable reftables[268] = {
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
};
#endif

static const upb_msgdef *refm(const upb_msgdef *m, const void *owner) {
  upb_msgdef_ref(m, owner);
  return m;
}

static const upb_enumdef *refe(const upb_enumdef *e, const void *owner) {
  upb_enumdef_ref(e, owner);
  return e;
}

/* Public API. */
const upb_msgdef *upbdefs_google_protobuf_DescriptorProto_get(const void *owner) { return refm(&msgs[0], owner); }
const upb_msgdef *upbdefs_google_protobuf_DescriptorProto_ExtensionRange_get(const void *owner) { return refm(&msgs[1], owner); }
const upb_msgdef *upbdefs_google_protobuf_DescriptorProto_ReservedRange_get(const void *owner) { return refm(&msgs[2], owner); }
const upb_msgdef *upbdefs_google_protobuf_EnumDescriptorProto_get(const void *owner) { return refm(&msgs[3], owner); }
const upb_msgdef *upbdefs_google_protobuf_EnumOptions_get(const void *owner) { return refm(&msgs[4], owner); }
const upb_msgdef *upbdefs_google_protobuf_EnumValueDescriptorProto_get(const void *owner) { return refm(&msgs[5], owner); }
const upb_msgdef *upbdefs_google_protobuf_EnumValueOptions_get(const void *owner) { return refm(&msgs[6], owner); }
const upb_msgdef *upbdefs_google_protobuf_FieldDescriptorProto_get(const void *owner) { return refm(&msgs[7], owner); }
const upb_msgdef *upbdefs_google_protobuf_FieldOptions_get(const void *owner) { return refm(&msgs[8], owner); }
const upb_msgdef *upbdefs_google_protobuf_FileDescriptorProto_get(const void *owner) { return refm(&msgs[9], owner); }
const upb_msgdef *upbdefs_google_protobuf_FileDescriptorSet_get(const void *owner) { return refm(&msgs[10], owner); }
const upb_msgdef *upbdefs_google_protobuf_FileOptions_get(const void *owner) { return refm(&msgs[11], owner); }
const upb_msgdef *upbdefs_google_protobuf_MessageOptions_get(const void *owner) { return refm(&msgs[12], owner); }
const upb_msgdef *upbdefs_google_protobuf_MethodDescriptorProto_get(const void *owner) { return refm(&msgs[13], owner); }
const upb_msgdef *upbdefs_google_protobuf_MethodOptions_get(const void *owner) { return refm(&msgs[14], owner); }
const upb_msgdef *upbdefs_google_protobuf_OneofDescriptorProto_get(const void *owner) { return refm(&msgs[15], owner); }
const upb_msgdef *upbdefs_google_protobuf_ServiceDescriptorProto_get(const void *owner) { return refm(&msgs[16], owner); }
const upb_msgdef *upbdefs_google_protobuf_ServiceOptions_get(const void *owner) { return refm(&msgs[17], owner); }
const upb_msgdef *upbdefs_google_protobuf_SourceCodeInfo_get(const void *owner) { return refm(&msgs[18], owner); }
const upb_msgdef *upbdefs_google_protobuf_SourceCodeInfo_Location_get(const void *owner) { return refm(&msgs[19], owner); }
const upb_msgdef *upbdefs_google_protobuf_UninterpretedOption_get(const void *owner) { return refm(&msgs[20], owner); }
const upb_msgdef *upbdefs_google_protobuf_UninterpretedOption_NamePart_get(const void *owner) { return refm(&msgs[21], owner); }

const upb_enumdef *upbdefs_google_protobuf_FieldDescriptorProto_Label_get(const void *owner) { return refe(&enums[0], owner); }
const upb_enumdef *upbdefs_google_protobuf_FieldDescriptorProto_Type_get(const void *owner) { return refe(&enums[1], owner); }
const upb_enumdef *upbdefs_google_protobuf_FieldOptions_CType_get(const void *owner) { return refe(&enums[2], owner); }
const upb_enumdef *upbdefs_google_protobuf_FieldOptions_JSType_get(const void *owner) { return refe(&enums[3], owner); }
const upb_enumdef *upbdefs_google_protobuf_FileOptions_OptimizeMode_get(const void *owner) { return refe(&enums[4], owner); }
/*
** XXX: The routines in this file that consume a string do not currently
** support having the string span buffers.  In the future, as upb_sink and
** its buffering/sharing functionality evolve there should be an easy and
** idiomatic way of correctly handling this case.  For now, we accept this
** limitation since we currently only parse descriptors from single strings.
*/


#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* Compares a NULL-terminated string with a non-NULL-terminated string. */
static bool upb_streq(const char *str, const char *buf, size_t n) {
  return strlen(str) == n && memcmp(str, buf, n) == 0;
}

/* We keep a stack of all the messages scopes we are currently in, as well as
 * the top-level file scope.  This is necessary to correctly qualify the
 * definitions that are contained inside.  "name" tracks the name of the
 * message or package (a bare name -- not qualified by any enclosing scopes). */
typedef struct {
  char *name;
  /* Index of the first def that is under this scope.  For msgdefs, the
   * msgdef itself is at start-1. */
  int start;
  uint32_t oneof_start;
  uint32_t oneof_index;
} upb_descreader_frame;

/* The maximum number of nested declarations that are allowed, ie.
 * message Foo {
 *   message Bar {
 *     message Baz {
 *     }
 *   }
 * }
 *
 * This is a resource limit that affects how big our runtime stack can grow.
 * TODO: make this a runtime-settable property of the Reader instance. */
#define UPB_MAX_MESSAGE_NESTING 64

struct upb_descreader {
  upb_sink sink;
  upb_inttable files;
  upb_strtable files_by_name;
  upb_filedef *file;  /* The last file in files. */
  upb_descreader_frame stack[UPB_MAX_MESSAGE_NESTING];
  int stack_len;
  upb_inttable oneofs;

  uint32_t number;
  char *name;
  bool saw_number;
  bool saw_name;

  char *default_string;

  upb_fielddef *f;
};

static char *upb_gstrndup(const char *buf, size_t n) {
  char *ret = upb_gmalloc(n + 1);
  if (!ret) return NULL;
  memcpy(ret, buf, n);
  ret[n] = '\0';
  return ret;
}

/* Returns a newly allocated string that joins input strings together, for
 * example:
 *   join("Foo.Bar", "Baz") -> "Foo.Bar.Baz"
 *   join("", "Baz") -> "Baz"
 * Caller owns a ref on the returned string. */
static char *upb_join(const char *base, const char *name) {
  if (!base || strlen(base) == 0) {
    return upb_gstrdup(name);
  } else {
    char *ret = upb_gmalloc(strlen(base) + strlen(name) + 2);
    if (!ret) {
      return NULL;
    }
    ret[0] = '\0';
    strcat(ret, base);
    strcat(ret, ".");
    strcat(ret, name);
    return ret;
  }
}

/* Qualify the defname for all defs starting with offset "start" with "str". */
static bool upb_descreader_qualify(upb_filedef *f, char *str, int32_t start) {
  size_t i;
  for (i = start; i < upb_filedef_defcount(f); i++) {
    upb_def *def = upb_filedef_mutabledef(f, i);
    char *name = upb_join(str, upb_def_fullname(def));
    if (!name) {
      /* Need better logic here; at this point we've qualified some names but
       * not others. */
      return false;
    }
    upb_def_setfullname(def, name, NULL);
    upb_gfree(name);
  }
  return true;
}


/* upb_descreader  ************************************************************/

static upb_msgdef *upb_descreader_top(upb_descreader *r) {
  int index;
  UPB_ASSERT(r->stack_len > 1);
  index = r->stack[r->stack_len-1].start - 1;
  UPB_ASSERT(index >= 0);
  return upb_downcast_msgdef_mutable(upb_filedef_mutabledef(r->file, index));
}

static upb_def *upb_descreader_last(upb_descreader *r) {
  return upb_filedef_mutabledef(r->file, upb_filedef_defcount(r->file) - 1);
}

/* Start/end handlers for FileDescriptorProto and DescriptorProto (the two
 * entities that have names and can contain sub-definitions. */
void upb_descreader_startcontainer(upb_descreader *r) {
  upb_descreader_frame *f = &r->stack[r->stack_len++];
  f->start = upb_filedef_defcount(r->file);
  f->oneof_start = upb_inttable_count(&r->oneofs);
  f->oneof_index = 0;
  f->name = NULL;
}

bool upb_descreader_endcontainer(upb_descreader *r) {
  upb_descreader_frame *f = &r->stack[r->stack_len - 1];

  while (upb_inttable_count(&r->oneofs) > f->oneof_start) {
    upb_oneofdef *o = upb_value_getptr(upb_inttable_pop(&r->oneofs));
    bool ok = upb_msgdef_addoneof(upb_descreader_top(r), o, &r->oneofs, NULL);
    UPB_ASSERT(ok);
  }

  if (!upb_descreader_qualify(r->file, f->name, f->start)) {
    return false;
  }
  upb_gfree(f->name);
  f->name = NULL;

  r->stack_len--;
  return true;
}

void upb_descreader_setscopename(upb_descreader *r, char *str) {
  upb_descreader_frame *f = &r->stack[r->stack_len-1];
  upb_gfree(f->name);
  f->name = str;
}

static upb_oneofdef *upb_descreader_getoneof(upb_descreader *r,
                                             uint32_t index) {
  bool found;
  upb_value val;
  upb_descreader_frame *f = &r->stack[r->stack_len-1];

  /* DescriptorProto messages can be nested, so we will see the nested messages
   * between when we see the FieldDescriptorProto and the OneofDescriptorProto.
   * We need to preserve the oneofs in between these two things. */
  index += f->oneof_start;

  while (upb_inttable_count(&r->oneofs) <= index) {
    upb_inttable_push(&r->oneofs, upb_value_ptr(upb_oneofdef_new(&r->oneofs)));
  }

  found = upb_inttable_lookup(&r->oneofs, index, &val);
  UPB_ASSERT(found);
  return upb_value_getptr(val);
}

/** Handlers for google.protobuf.FileDescriptorSet. ***************************/

static void *fileset_startfile(void *closure, const void *hd) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  r->file = upb_filedef_new(&r->files);
  upb_inttable_push(&r->files, upb_value_ptr(r->file));
  return r;
}

/** Handlers for google.protobuf.FileDescriptorProto. *************************/

static bool file_start(void *closure, const void *hd) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  upb_descreader_startcontainer(r);
  return true;
}

static bool file_end(void *closure, const void *hd, upb_status *status) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  UPB_UNUSED(status);
  return upb_descreader_endcontainer(r);
}

static size_t file_onname(void *closure, const void *hd, const char *buf,
                          size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *name;
  bool ok;
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  name = upb_gstrndup(buf, n);
  upb_strtable_insert(&r->files_by_name, name, upb_value_ptr(r->file));
  /* XXX: see comment at the top of the file. */
  ok = upb_filedef_setname(r->file, name, NULL);
  upb_gfree(name);
  UPB_ASSERT(ok);
  return n;
}

static size_t file_onpackage(void *closure, const void *hd, const char *buf,
                             size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *package;
  bool ok;
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  package = upb_gstrndup(buf, n);
  /* XXX: see comment at the top of the file. */
  upb_descreader_setscopename(r, package);
  ok = upb_filedef_setpackage(r->file, package, NULL);
  UPB_ASSERT(ok);
  return n;
}

static void *file_startphpnamespace(void *closure, const void *hd,
                                    size_t size_hint) {
  upb_descreader *r = closure;
  bool ok;
  UPB_UNUSED(hd);
  UPB_UNUSED(size_hint);

  ok = upb_filedef_setphpnamespace(r->file, "", NULL);
  UPB_ASSERT(ok);
  return closure;
}

static size_t file_onphpnamespace(void *closure, const void *hd,
                                  const char *buf, size_t n,
                                  const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *php_namespace;
  bool ok;
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  php_namespace = upb_gstrndup(buf, n);
  ok = upb_filedef_setphpnamespace(r->file, php_namespace, NULL);
  upb_gfree(php_namespace);
  UPB_ASSERT(ok);
  return n;
}

static size_t file_onphpprefix(void *closure, const void *hd, const char *buf,
                             size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *prefix;
  bool ok;
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  prefix = upb_gstrndup(buf, n);
  ok = upb_filedef_setphpprefix(r->file, prefix, NULL);
  upb_gfree(prefix);
  UPB_ASSERT(ok);
  return n;
}

static size_t file_onsyntax(void *closure, const void *hd, const char *buf,
                            size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  bool ok;
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);
  /* XXX: see comment at the top of the file. */
  if (upb_streq("proto2", buf, n)) {
    ok = upb_filedef_setsyntax(r->file, UPB_SYNTAX_PROTO2, NULL);
  } else if (upb_streq("proto3", buf, n)) {
    ok = upb_filedef_setsyntax(r->file, UPB_SYNTAX_PROTO3, NULL);
  } else {
    ok = false;
  }

  UPB_ASSERT(ok);
  return n;
}

static void *file_startmsg(void *closure, const void *hd) {
  upb_descreader *r = closure;
  upb_msgdef *m = upb_msgdef_new(&m);
  bool ok = upb_filedef_addmsg(r->file, m, &m, NULL);
  UPB_UNUSED(hd);
  UPB_ASSERT(ok);
  return r;
}

static void *file_startenum(void *closure, const void *hd) {
  upb_descreader *r = closure;
  upb_enumdef *e = upb_enumdef_new(&e);
  bool ok = upb_filedef_addenum(r->file, e, &e, NULL);
  UPB_UNUSED(hd);
  UPB_ASSERT(ok);
  return r;
}

static void *file_startext(void *closure, const void *hd) {
  upb_descreader *r = closure;
  bool ok;
  r->f = upb_fielddef_new(r);
  ok = upb_filedef_addext(r->file, r->f, r, NULL);
  UPB_UNUSED(hd);
  UPB_ASSERT(ok);
  return r;
}

static size_t file_ondep(void *closure, const void *hd, const char *buf,
                         size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  upb_value val;
  if (upb_strtable_lookup2(&r->files_by_name, buf, n, &val)) {
    upb_filedef_adddep(r->file, upb_value_getptr(val));
  }
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);
  return n;
}

/** Handlers for google.protobuf.EnumValueDescriptorProto. *********************/

static bool enumval_startmsg(void *closure, const void *hd) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  r->saw_number = false;
  r->saw_name = false;
  return true;
}

static size_t enumval_onname(void *closure, const void *hd, const char *buf,
                             size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);
  /* XXX: see comment at the top of the file. */
  upb_gfree(r->name);
  r->name = upb_gstrndup(buf, n);
  r->saw_name = true;
  return n;
}

static bool enumval_onnumber(void *closure, const void *hd, int32_t val) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  r->number = val;
  r->saw_number = true;
  return true;
}

static bool enumval_endmsg(void *closure, const void *hd, upb_status *status) {
  upb_descreader *r = closure;
  upb_enumdef *e;
  UPB_UNUSED(hd);

  if(!r->saw_number || !r->saw_name) {
    upb_status_seterrmsg(status, "Enum value missing name or number.");
    return false;
  }
  e = upb_downcast_enumdef_mutable(upb_descreader_last(r));
  upb_enumdef_addval(e, r->name, r->number, status);
  upb_gfree(r->name);
  r->name = NULL;
  return true;
}

/** Handlers for google.protobuf.EnumDescriptorProto. *************************/

static bool enum_endmsg(void *closure, const void *hd, upb_status *status) {
  upb_descreader *r = closure;
  upb_enumdef *e;
  UPB_UNUSED(hd);

  e = upb_downcast_enumdef_mutable(upb_descreader_last(r));
  if (upb_def_fullname(upb_descreader_last(r)) == NULL) {
    upb_status_seterrmsg(status, "Enum had no name.");
    return false;
  }
  if (upb_enumdef_numvals(e) == 0) {
    upb_status_seterrmsg(status, "Enum had no values.");
    return false;
  }
  return true;
}

static size_t enum_onname(void *closure, const void *hd, const char *buf,
                          size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *fullname = upb_gstrndup(buf, n);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);
  /* XXX: see comment at the top of the file. */
  upb_def_setfullname(upb_descreader_last(r), fullname, NULL);
  upb_gfree(fullname);
  return n;
}

/** Handlers for google.protobuf.FieldDescriptorProto *************************/

static bool field_startmsg(void *closure, const void *hd) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  UPB_ASSERT(r->f);
  upb_gfree(r->default_string);
  r->default_string = NULL;

  /* fielddefs default to packed, but descriptors default to non-packed. */
  upb_fielddef_setpacked(r->f, false);
  return true;
}

/* Converts the default value in string "str" into "d".  Passes a ref on str.
 * Returns true on success. */
static bool parse_default(char *str, upb_fielddef *f) {
  bool success = true;
  char *end;
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32: {
      long val = strtol(str, &end, 0);
      if (val > INT32_MAX || val < INT32_MIN || errno == ERANGE || *end)
        success = false;
      else
        upb_fielddef_setdefaultint32(f, val);
      break;
    }
    case UPB_TYPE_INT64: {
      /* XXX: Need to write our own strtoll, since it's not available in c89. */
      long long val = strtol(str, &end, 0);
      if (val > INT64_MAX || val < INT64_MIN || errno == ERANGE || *end)
        success = false;
      else
        upb_fielddef_setdefaultint64(f, val);
      break;
    }
    case UPB_TYPE_UINT32: {
      unsigned long val = strtoul(str, &end, 0);
      if (val > UINT32_MAX || errno == ERANGE || *end)
        success = false;
      else
        upb_fielddef_setdefaultuint32(f, val);
      break;
    }
    case UPB_TYPE_UINT64: {
      /* XXX: Need to write our own strtoull, since it's not available in c89. */
      unsigned long long val = strtoul(str, &end, 0);
      if (val > UINT64_MAX || errno == ERANGE || *end)
        success = false;
      else
        upb_fielddef_setdefaultuint64(f, val);
      break;
    }
    case UPB_TYPE_DOUBLE: {
      double val = strtod(str, &end);
      if (errno == ERANGE || *end)
        success = false;
      else
        upb_fielddef_setdefaultdouble(f, val);
      break;
    }
    case UPB_TYPE_FLOAT: {
      /* XXX: Need to write our own strtof, since it's not available in c89. */
      float val = strtod(str, &end);
      if (errno == ERANGE || *end)
        success = false;
      else
        upb_fielddef_setdefaultfloat(f, val);
      break;
    }
    case UPB_TYPE_BOOL: {
      if (strcmp(str, "false") == 0)
        upb_fielddef_setdefaultbool(f, false);
      else if (strcmp(str, "true") == 0)
        upb_fielddef_setdefaultbool(f, true);
      else
        success = false;
      break;
    }
    default: abort();
  }
  return success;
}

static bool field_endmsg(void *closure, const void *hd, upb_status *status) {
  upb_descreader *r = closure;
  upb_fielddef *f = r->f;
  UPB_UNUSED(hd);

  /* TODO: verify that all required fields were present. */
  UPB_ASSERT(upb_fielddef_number(f) != 0);
  UPB_ASSERT(upb_fielddef_name(f) != NULL);
  UPB_ASSERT((upb_fielddef_subdefname(f) != NULL) == upb_fielddef_hassubdef(f));

  if (r->default_string) {
    if (upb_fielddef_issubmsg(f)) {
      upb_status_seterrmsg(status, "Submessages cannot have defaults.");
      return false;
    }
    if (upb_fielddef_isstring(f) || upb_fielddef_type(f) == UPB_TYPE_ENUM) {
      upb_fielddef_setdefaultcstr(f, r->default_string, NULL);
    } else {
      if (r->default_string && !parse_default(r->default_string, f)) {
        /* We don't worry too much about giving a great error message since the
         * compiler should have ensured this was correct. */
        upb_status_seterrmsg(status, "Error converting default value.");
        return false;
      }
    }
  }
  return true;
}

static bool field_onlazy(void *closure, const void *hd, bool val) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);

  upb_fielddef_setlazy(r->f, val);
  return true;
}

static bool field_onpacked(void *closure, const void *hd, bool val) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);

  upb_fielddef_setpacked(r->f, val);
  return true;
}

static bool field_ontype(void *closure, const void *hd, int32_t val) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);

  upb_fielddef_setdescriptortype(r->f, val);
  return true;
}

static bool field_onlabel(void *closure, const void *hd, int32_t val) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);

  upb_fielddef_setlabel(r->f, val);
  return true;
}

static bool field_onnumber(void *closure, const void *hd, int32_t val) {
  upb_descreader *r = closure;
  bool ok;
  UPB_UNUSED(hd);

  ok = upb_fielddef_setnumber(r->f, val, NULL);
  UPB_ASSERT(ok);
  return true;
}

static size_t field_onname(void *closure, const void *hd, const char *buf,
                           size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *name = upb_gstrndup(buf, n);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  /* XXX: see comment at the top of the file. */
  upb_fielddef_setname(r->f, name, NULL);
  upb_gfree(name);
  return n;
}

static size_t field_ontypename(void *closure, const void *hd, const char *buf,
                               size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *name = upb_gstrndup(buf, n);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  /* XXX: see comment at the top of the file. */
  upb_fielddef_setsubdefname(r->f, name, NULL);
  upb_gfree(name);
  return n;
}

static size_t field_onextendee(void *closure, const void *hd, const char *buf,
                               size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *name = upb_gstrndup(buf, n);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  /* XXX: see comment at the top of the file. */
  upb_fielddef_setcontainingtypename(r->f, name, NULL);
  upb_gfree(name);
  return n;
}

static size_t field_ondefaultval(void *closure, const void *hd, const char *buf,
                                 size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  /* Have to convert from string to the correct type, but we might not know the
   * type yet, so we save it as a string until the end of the field.
   * XXX: see comment at the top of the file. */
  upb_gfree(r->default_string);
  r->default_string = upb_gstrndup(buf, n);
  return n;
}

static bool field_ononeofindex(void *closure, const void *hd, int32_t index) {
  upb_descreader *r = closure;
  upb_oneofdef *o = upb_descreader_getoneof(r, index);
  bool ok = upb_oneofdef_addfield(o, r->f, &r->f, NULL);
  UPB_UNUSED(hd);

  UPB_ASSERT(ok);
  return true;
}

/** Handlers for google.protobuf.OneofDescriptorProto. ************************/

static size_t oneof_name(void *closure, const void *hd, const char *buf,
                         size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  upb_descreader_frame *f = &r->stack[r->stack_len-1];
  upb_oneofdef *o = upb_descreader_getoneof(r, f->oneof_index++);
  char *name_null_terminated = upb_gstrndup(buf, n);
  bool ok = upb_oneofdef_setname(o, name_null_terminated, NULL);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  UPB_ASSERT(ok);
  free(name_null_terminated);
  return n;
}

/** Handlers for google.protobuf.DescriptorProto ******************************/

static bool msg_start(void *closure, const void *hd) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);

  upb_descreader_startcontainer(r);
  return true;
}

static bool msg_end(void *closure, const void *hd, upb_status *status) {
  upb_descreader *r = closure;
  upb_msgdef *m = upb_descreader_top(r);
  UPB_UNUSED(hd);

  if(!upb_def_fullname(upb_msgdef_upcast_mutable(m))) {
    upb_status_seterrmsg(status, "Encountered message with no name.");
    return false;
  }
  return upb_descreader_endcontainer(r);
}

static size_t msg_name(void *closure, const void *hd, const char *buf,
                       size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  upb_msgdef *m = upb_descreader_top(r);
  /* XXX: see comment at the top of the file. */
  char *name = upb_gstrndup(buf, n);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  upb_def_setfullname(upb_msgdef_upcast_mutable(m), name, NULL);
  upb_descreader_setscopename(r, name);  /* Passes ownership of name. */
  return n;
}

static void *msg_startmsg(void *closure, const void *hd) {
  upb_descreader *r = closure;
  upb_msgdef *m = upb_msgdef_new(&m);
  bool ok = upb_filedef_addmsg(r->file, m, &m, NULL);
  UPB_UNUSED(hd);
  UPB_ASSERT(ok);
  return r;
}

static void *msg_startext(void *closure, const void *hd) {
  upb_descreader *r = closure;
  upb_fielddef *f = upb_fielddef_new(&f);
  bool ok = upb_filedef_addext(r->file, f, &f, NULL);
  UPB_UNUSED(hd);
  UPB_ASSERT(ok);
  return r;
}

static void *msg_startfield(void *closure, const void *hd) {
  upb_descreader *r = closure;
  r->f = upb_fielddef_new(&r->f);
  /* We can't add the new field to the message until its name/number are
   * filled in. */
  UPB_UNUSED(hd);
  return r;
}

static bool msg_endfield(void *closure, const void *hd) {
  upb_descreader *r = closure;
  upb_msgdef *m = upb_descreader_top(r);
  bool ok;
  UPB_UNUSED(hd);

  /* Oneof fields are added to the msgdef through their oneof, so don't need to
   * be added here. */
  if (upb_fielddef_containingoneof(r->f) == NULL) {
    ok = upb_msgdef_addfield(m, r->f, &r->f, NULL);
    UPB_ASSERT(ok);
  }
  r->f = NULL;
  return true;
}

static bool msg_onmapentry(void *closure, const void *hd, bool mapentry) {
  upb_descreader *r = closure;
  upb_msgdef *m = upb_descreader_top(r);
  UPB_UNUSED(hd);

  upb_msgdef_setmapentry(m, mapentry);
  r->f = NULL;
  return true;
}



/** Code to register handlers *************************************************/

#define F(msg, field) upbdefs_google_protobuf_ ## msg ## _f_ ## field(m)

static void reghandlers(const void *closure, upb_handlers *h) {
  const upb_msgdef *m = upb_handlers_msgdef(h);
  UPB_UNUSED(closure);

  if (upbdefs_google_protobuf_FileDescriptorSet_is(m)) {
    upb_handlers_setstartsubmsg(h, F(FileDescriptorSet, file),
                                &fileset_startfile, NULL);
  } else if (upbdefs_google_protobuf_DescriptorProto_is(m)) {
    upb_handlers_setstartmsg(h, &msg_start, NULL);
    upb_handlers_setendmsg(h, &msg_end, NULL);
    upb_handlers_setstring(h, F(DescriptorProto, name), &msg_name, NULL);
    upb_handlers_setstartsubmsg(h, F(DescriptorProto, extension), &msg_startext,
                                NULL);
    upb_handlers_setstartsubmsg(h, F(DescriptorProto, nested_type),
                                &msg_startmsg, NULL);
    upb_handlers_setstartsubmsg(h, F(DescriptorProto, field),
                                &msg_startfield, NULL);
    upb_handlers_setendsubmsg(h, F(DescriptorProto, field),
                              &msg_endfield, NULL);
    upb_handlers_setstartsubmsg(h, F(DescriptorProto, enum_type),
                                &file_startenum, NULL);
  } else if (upbdefs_google_protobuf_FileDescriptorProto_is(m)) {
    upb_handlers_setstartmsg(h, &file_start, NULL);
    upb_handlers_setendmsg(h, &file_end, NULL);
    upb_handlers_setstring(h, F(FileDescriptorProto, name), &file_onname,
                           NULL);
    upb_handlers_setstring(h, F(FileDescriptorProto, package), &file_onpackage,
                           NULL);
    upb_handlers_setstring(h, F(FileDescriptorProto, syntax), &file_onsyntax,
                           NULL);
    upb_handlers_setstartsubmsg(h, F(FileDescriptorProto, message_type),
                                &file_startmsg, NULL);
    upb_handlers_setstartsubmsg(h, F(FileDescriptorProto, enum_type),
                                &file_startenum, NULL);
    upb_handlers_setstartsubmsg(h, F(FileDescriptorProto, extension),
                                &file_startext, NULL);
    upb_handlers_setstring(h, F(FileDescriptorProto, dependency),
                           &file_ondep, NULL);
  } else if (upbdefs_google_protobuf_EnumValueDescriptorProto_is(m)) {
    upb_handlers_setstartmsg(h, &enumval_startmsg, NULL);
    upb_handlers_setendmsg(h, &enumval_endmsg, NULL);
    upb_handlers_setstring(h, F(EnumValueDescriptorProto, name), &enumval_onname, NULL);
    upb_handlers_setint32(h, F(EnumValueDescriptorProto, number), &enumval_onnumber,
                          NULL);
  } else if (upbdefs_google_protobuf_EnumDescriptorProto_is(m)) {
    upb_handlers_setendmsg(h, &enum_endmsg, NULL);
    upb_handlers_setstring(h, F(EnumDescriptorProto, name), &enum_onname, NULL);
  } else if (upbdefs_google_protobuf_FieldDescriptorProto_is(m)) {
    upb_handlers_setstartmsg(h, &field_startmsg, NULL);
    upb_handlers_setendmsg(h, &field_endmsg, NULL);
    upb_handlers_setint32(h, F(FieldDescriptorProto, type), &field_ontype,
                          NULL);
    upb_handlers_setint32(h, F(FieldDescriptorProto, label), &field_onlabel,
                          NULL);
    upb_handlers_setint32(h, F(FieldDescriptorProto, number), &field_onnumber,
                          NULL);
    upb_handlers_setstring(h, F(FieldDescriptorProto, name), &field_onname,
                           NULL);
    upb_handlers_setstring(h, F(FieldDescriptorProto, type_name),
                           &field_ontypename, NULL);
    upb_handlers_setstring(h, F(FieldDescriptorProto, extendee),
                           &field_onextendee, NULL);
    upb_handlers_setstring(h, F(FieldDescriptorProto, default_value),
                           &field_ondefaultval, NULL);
    upb_handlers_setint32(h, F(FieldDescriptorProto, oneof_index),
                          &field_ononeofindex, NULL);
  } else if (upbdefs_google_protobuf_OneofDescriptorProto_is(m)) {
    upb_handlers_setstring(h, F(OneofDescriptorProto, name), &oneof_name, NULL);
  } else if (upbdefs_google_protobuf_FieldOptions_is(m)) {
    upb_handlers_setbool(h, F(FieldOptions, lazy), &field_onlazy, NULL);
    upb_handlers_setbool(h, F(FieldOptions, packed), &field_onpacked, NULL);
  } else if (upbdefs_google_protobuf_MessageOptions_is(m)) {
    upb_handlers_setbool(h, F(MessageOptions, map_entry), &msg_onmapentry, NULL);
  } else if (upbdefs_google_protobuf_FileOptions_is(m)) {
    upb_handlers_setstring(h, F(FileOptions, php_class_prefix),
                           &file_onphpprefix, NULL);
    upb_handlers_setstartstr(h, F(FileOptions, php_namespace),
                             &file_startphpnamespace, NULL);
    upb_handlers_setstring(h, F(FileOptions, php_namespace),
                           &file_onphpnamespace, NULL);
  }

  UPB_ASSERT(upb_ok(upb_handlers_status(h)));
}

#undef F

void descreader_cleanup(void *_r) {
  upb_descreader *r = _r;
  size_t i;

  for (i = 0; i < upb_descreader_filecount(r); i++) {
    upb_filedef_unref(upb_descreader_file(r, i), &r->files);
  }

  upb_gfree(r->name);
  upb_inttable_uninit(&r->files);
  upb_strtable_uninit(&r->files_by_name);
  upb_inttable_uninit(&r->oneofs);
  upb_gfree(r->default_string);
  while (r->stack_len > 0) {
    upb_descreader_frame *f = &r->stack[--r->stack_len];
    upb_gfree(f->name);
  }
}


/* Public API  ****************************************************************/

upb_descreader *upb_descreader_create(upb_env *e, const upb_handlers *h) {
  upb_descreader *r = upb_env_malloc(e, sizeof(upb_descreader));
  if (!r || !upb_env_addcleanup(e, descreader_cleanup, r)) {
    return NULL;
  }

  upb_inttable_init(&r->files, UPB_CTYPE_PTR);
  upb_strtable_init(&r->files_by_name, UPB_CTYPE_PTR);
  upb_inttable_init(&r->oneofs, UPB_CTYPE_PTR);
  upb_sink_reset(upb_descreader_input(r), h, r);
  r->stack_len = 0;
  r->name = NULL;
  r->default_string = NULL;

  return r;
}

size_t upb_descreader_filecount(const upb_descreader *r) {
  return upb_inttable_count(&r->files);
}

upb_filedef *upb_descreader_file(const upb_descreader *r, size_t i) {
  upb_value v;
  if (upb_inttable_lookup(&r->files, i, &v)) {
    return upb_value_getptr(v);
  } else {
    return NULL;
  }
}

upb_sink *upb_descreader_input(upb_descreader *r) {
  return &r->sink;
}

const upb_handlers *upb_descreader_newhandlers(const void *owner) {
  const upb_msgdef *m = upbdefs_google_protobuf_FileDescriptorSet_get(&m);
  const upb_handlers *h = upb_handlers_newfrozen(m, owner, reghandlers, NULL);
  upb_msgdef_unref(m, &m);
  return h;
}
/*
** protobuf decoder bytecode compiler
**
** Code to compile a upb::Handlers into bytecode for decoding a protobuf
** according to that specific schema and destination handlers.
**
** Compiling to bytecode is always the first step.  If we are using the
** interpreted decoder we leave it as bytecode and interpret that.  If we are
** using a JIT decoder we use a code generator to turn the bytecode into native
** code, LLVM IR, etc.
**
** Bytecode definition is in decoder.int.h.
*/

#include <stdarg.h>

#ifdef UPB_DUMP_BYTECODE
#include <stdio.h>
#endif

#define MAXLABEL 5
#define EMPTYLABEL -1

/* mgroup *********************************************************************/

static void freegroup(upb_refcounted *r) {
  mgroup *g = (mgroup*)r;
  upb_inttable_uninit(&g->methods);
#ifdef UPB_USE_JIT_X64
  upb_pbdecoder_freejit(g);
#endif
  upb_gfree(g->bytecode);
  upb_gfree(g);
}

static void visitgroup(const upb_refcounted *r, upb_refcounted_visit *visit,
                       void *closure) {
  const mgroup *g = (const mgroup*)r;
  upb_inttable_iter i;
  upb_inttable_begin(&i, &g->methods);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_pbdecodermethod *method = upb_value_getptr(upb_inttable_iter_value(&i));
    visit(r, upb_pbdecodermethod_upcast(method), closure);
  }
}

mgroup *newgroup(const void *owner) {
  mgroup *g = upb_gmalloc(sizeof(*g));
  static const struct upb_refcounted_vtbl vtbl = {visitgroup, freegroup};
  upb_refcounted_init(mgroup_upcast_mutable(g), &vtbl, owner);
  upb_inttable_init(&g->methods, UPB_CTYPE_PTR);
  g->bytecode = NULL;
  g->bytecode_end = NULL;
  return g;
}


/* upb_pbdecodermethod ********************************************************/

static void freemethod(upb_refcounted *r) {
  upb_pbdecodermethod *method = (upb_pbdecodermethod*)r;

  if (method->dest_handlers_) {
    upb_handlers_unref(method->dest_handlers_, method);
  }

  upb_inttable_uninit(&method->dispatch);
  upb_gfree(method);
}

static void visitmethod(const upb_refcounted *r, upb_refcounted_visit *visit,
                        void *closure) {
  const upb_pbdecodermethod *m = (const upb_pbdecodermethod*)r;
  visit(r, m->group, closure);
}

static upb_pbdecodermethod *newmethod(const upb_handlers *dest_handlers,
                                      mgroup *group) {
  static const struct upb_refcounted_vtbl vtbl = {visitmethod, freemethod};
  upb_pbdecodermethod *ret = upb_gmalloc(sizeof(*ret));
  upb_refcounted_init(upb_pbdecodermethod_upcast_mutable(ret), &vtbl, &ret);
  upb_byteshandler_init(&ret->input_handler_);

  /* The method references the group and vice-versa, in a circular reference. */
  upb_ref2(ret, group);
  upb_ref2(group, ret);
  upb_inttable_insertptr(&group->methods, dest_handlers, upb_value_ptr(ret));
  upb_pbdecodermethod_unref(ret, &ret);

  ret->group = mgroup_upcast_mutable(group);
  ret->dest_handlers_ = dest_handlers;
  ret->is_native_ = false;  /* If we JIT, it will update this later. */
  upb_inttable_init(&ret->dispatch, UPB_CTYPE_UINT64);

  if (ret->dest_handlers_) {
    upb_handlers_ref(ret->dest_handlers_, ret);
  }
  return ret;
}

const upb_handlers *upb_pbdecodermethod_desthandlers(
    const upb_pbdecodermethod *m) {
  return m->dest_handlers_;
}

const upb_byteshandler *upb_pbdecodermethod_inputhandler(
    const upb_pbdecodermethod *m) {
  return &m->input_handler_;
}

bool upb_pbdecodermethod_isnative(const upb_pbdecodermethod *m) {
  return m->is_native_;
}

const upb_pbdecodermethod *upb_pbdecodermethod_new(
    const upb_pbdecodermethodopts *opts, const void *owner) {
  const upb_pbdecodermethod *ret;
  upb_pbcodecache cache;

  upb_pbcodecache_init(&cache);
  ret = upb_pbcodecache_getdecodermethod(&cache, opts);
  upb_pbdecodermethod_ref(ret, owner);
  upb_pbcodecache_uninit(&cache);
  return ret;
}


/* bytecode compiler **********************************************************/

/* Data used only at compilation time. */
typedef struct {
  mgroup *group;

  uint32_t *pc;
  int fwd_labels[MAXLABEL];
  int back_labels[MAXLABEL];

  /* For fields marked "lazy", parse them lazily or eagerly? */
  bool lazy;
} compiler;

static compiler *newcompiler(mgroup *group, bool lazy) {
  compiler *ret = upb_gmalloc(sizeof(*ret));
  int i;

  ret->group = group;
  ret->lazy = lazy;
  for (i = 0; i < MAXLABEL; i++) {
    ret->fwd_labels[i] = EMPTYLABEL;
    ret->back_labels[i] = EMPTYLABEL;
  }
  return ret;
}

static void freecompiler(compiler *c) {
  upb_gfree(c);
}

const size_t ptr_words = sizeof(void*) / sizeof(uint32_t);

/* How many words an instruction is. */
static int instruction_len(uint32_t instr) {
  switch (getop(instr)) {
    case OP_SETDISPATCH: return 1 + ptr_words;
    case OP_TAGN: return 3;
    case OP_SETBIGGROUPNUM: return 2;
    default: return 1;
  }
}

bool op_has_longofs(int32_t instruction) {
  switch (getop(instruction)) {
    case OP_CALL:
    case OP_BRANCH:
    case OP_CHECKDELIM:
      return true;
    /* The "tag" instructions only have 8 bytes available for the jump target,
     * but that is ok because these opcodes only require short jumps. */
    case OP_TAG1:
    case OP_TAG2:
    case OP_TAGN:
      return false;
    default:
      UPB_ASSERT(false);
      return false;
  }
}

static int32_t getofs(uint32_t instruction) {
  if (op_has_longofs(instruction)) {
    return (int32_t)instruction >> 8;
  } else {
    return (int8_t)(instruction >> 8);
  }
}

static void setofs(uint32_t *instruction, int32_t ofs) {
  if (op_has_longofs(*instruction)) {
    *instruction = getop(*instruction) | ofs << 8;
  } else {
    *instruction = (*instruction & ~0xff00) | ((ofs & 0xff) << 8);
  }
  UPB_ASSERT(getofs(*instruction) == ofs);  /* Would fail in cases of overflow. */
}

static uint32_t pcofs(compiler *c) { return c->pc - c->group->bytecode; }

/* Defines a local label at the current PC location.  All previous forward
 * references are updated to point to this location.  The location is noted
 * for any future backward references. */
static void label(compiler *c, unsigned int label) {
  int val;
  uint32_t *codep;

  UPB_ASSERT(label < MAXLABEL);
  val = c->fwd_labels[label];
  codep = (val == EMPTYLABEL) ? NULL : c->group->bytecode + val;
  while (codep) {
    int ofs = getofs(*codep);
    setofs(codep, c->pc - codep - instruction_len(*codep));
    codep = ofs ? codep + ofs : NULL;
  }
  c->fwd_labels[label] = EMPTYLABEL;
  c->back_labels[label] = pcofs(c);
}

/* Creates a reference to a numbered label; either a forward reference
 * (positive arg) or backward reference (negative arg).  For forward references
 * the value returned now is actually a "next" pointer into a linked list of all
 * instructions that use this label and will be patched later when the label is
 * defined with label().
 *
 * The returned value is the offset that should be written into the instruction.
 */
static int32_t labelref(compiler *c, int label) {
  UPB_ASSERT(label < MAXLABEL);
  if (label == LABEL_DISPATCH) {
    /* No resolving required. */
    return 0;
  } else if (label < 0) {
    /* Backward local label.  Relative to the next instruction. */
    uint32_t from = (c->pc + 1) - c->group->bytecode;
    return c->back_labels[-label] - from;
  } else {
    /* Forward local label: prepend to (possibly-empty) linked list. */
    int *lptr = &c->fwd_labels[label];
    int32_t ret = (*lptr == EMPTYLABEL) ? 0 : *lptr - pcofs(c);
    *lptr = pcofs(c);
    return ret;
  }
}

static void put32(compiler *c, uint32_t v) {
  mgroup *g = c->group;
  if (c->pc == g->bytecode_end) {
    int ofs = pcofs(c);
    size_t oldsize = g->bytecode_end - g->bytecode;
    size_t newsize = UPB_MAX(oldsize * 2, 64);
    /* TODO(haberman): handle OOM. */
    g->bytecode = upb_grealloc(g->bytecode, oldsize * sizeof(uint32_t),
                                            newsize * sizeof(uint32_t));
    g->bytecode_end = g->bytecode + newsize;
    c->pc = g->bytecode + ofs;
  }
  *c->pc++ = v;
}

static void putop(compiler *c, int op, ...) {
  va_list ap;
  va_start(ap, op);

  switch (op) {
    case OP_SETDISPATCH: {
      uintptr_t ptr = (uintptr_t)va_arg(ap, void*);
      put32(c, OP_SETDISPATCH);
      put32(c, ptr);
      if (sizeof(uintptr_t) > sizeof(uint32_t))
        put32(c, (uint64_t)ptr >> 32);
      break;
    }
    case OP_STARTMSG:
    case OP_ENDMSG:
    case OP_PUSHLENDELIM:
    case OP_POP:
    case OP_SETDELIM:
    case OP_HALT:
    case OP_RET:
    case OP_DISPATCH:
      put32(c, op);
      break;
    case OP_PARSE_DOUBLE:
    case OP_PARSE_FLOAT:
    case OP_PARSE_INT64:
    case OP_PARSE_UINT64:
    case OP_PARSE_INT32:
    case OP_PARSE_FIXED64:
    case OP_PARSE_FIXED32:
    case OP_PARSE_BOOL:
    case OP_PARSE_UINT32:
    case OP_PARSE_SFIXED32:
    case OP_PARSE_SFIXED64:
    case OP_PARSE_SINT32:
    case OP_PARSE_SINT64:
    case OP_STARTSEQ:
    case OP_ENDSEQ:
    case OP_STARTSUBMSG:
    case OP_ENDSUBMSG:
    case OP_STARTSTR:
    case OP_STRING:
    case OP_ENDSTR:
    case OP_PUSHTAGDELIM:
      put32(c, op | va_arg(ap, upb_selector_t) << 8);
      break;
    case OP_SETBIGGROUPNUM:
      put32(c, op);
      put32(c, va_arg(ap, int));
      break;
    case OP_CALL: {
      const upb_pbdecodermethod *method = va_arg(ap, upb_pbdecodermethod *);
      put32(c, op | (method->code_base.ofs - (pcofs(c) + 1)) << 8);
      break;
    }
    case OP_CHECKDELIM:
    case OP_BRANCH: {
      uint32_t instruction = op;
      int label = va_arg(ap, int);
      setofs(&instruction, labelref(c, label));
      put32(c, instruction);
      break;
    }
    case OP_TAG1:
    case OP_TAG2: {
      int label = va_arg(ap, int);
      uint64_t tag = va_arg(ap, uint64_t);
      uint32_t instruction = op | (tag << 16);
      UPB_ASSERT(tag <= 0xffff);
      setofs(&instruction, labelref(c, label));
      put32(c, instruction);
      break;
    }
    case OP_TAGN: {
      int label = va_arg(ap, int);
      uint64_t tag = va_arg(ap, uint64_t);
      uint32_t instruction = op | (upb_value_size(tag) << 16);
      setofs(&instruction, labelref(c, label));
      put32(c, instruction);
      put32(c, tag);
      put32(c, tag >> 32);
      break;
    }
  }

  va_end(ap);
}

#if defined(UPB_USE_JIT_X64) || defined(UPB_DUMP_BYTECODE)

const char *upb_pbdecoder_getopname(unsigned int op) {
#define QUOTE(x) #x
#define EXPAND_AND_QUOTE(x) QUOTE(x)
#define OPNAME(x) OP_##x
#define OP(x) case OPNAME(x): return EXPAND_AND_QUOTE(OPNAME(x));
#define T(x) OP(PARSE_##x)
  /* Keep in sync with list in decoder.int.h. */
  switch ((opcode)op) {
    T(DOUBLE) T(FLOAT) T(INT64) T(UINT64) T(INT32) T(FIXED64) T(FIXED32)
    T(BOOL) T(UINT32) T(SFIXED32) T(SFIXED64) T(SINT32) T(SINT64)
    OP(STARTMSG) OP(ENDMSG) OP(STARTSEQ) OP(ENDSEQ) OP(STARTSUBMSG)
    OP(ENDSUBMSG) OP(STARTSTR) OP(STRING) OP(ENDSTR) OP(CALL) OP(RET)
    OP(PUSHLENDELIM) OP(PUSHTAGDELIM) OP(SETDELIM) OP(CHECKDELIM)
    OP(BRANCH) OP(TAG1) OP(TAG2) OP(TAGN) OP(SETDISPATCH) OP(POP)
    OP(SETBIGGROUPNUM) OP(DISPATCH) OP(HALT)
  }
  return "<unknown op>";
#undef OP
#undef T
}

#endif

#ifdef UPB_DUMP_BYTECODE

static void dumpbc(uint32_t *p, uint32_t *end, FILE *f) {

  uint32_t *begin = p;

  while (p < end) {
    fprintf(f, "%p  %8tx", p, p - begin);
    uint32_t instr = *p++;
    uint8_t op = getop(instr);
    fprintf(f, " %s", upb_pbdecoder_getopname(op));
    switch ((opcode)op) {
      case OP_SETDISPATCH: {
        const upb_inttable *dispatch;
        memcpy(&dispatch, p, sizeof(void*));
        p += ptr_words;
        const upb_pbdecodermethod *method =
            (void *)((char *)dispatch -
                     offsetof(upb_pbdecodermethod, dispatch));
        fprintf(f, " %s", upb_msgdef_fullname(
                              upb_handlers_msgdef(method->dest_handlers_)));
        break;
      }
      case OP_DISPATCH:
      case OP_STARTMSG:
      case OP_ENDMSG:
      case OP_PUSHLENDELIM:
      case OP_POP:
      case OP_SETDELIM:
      case OP_HALT:
      case OP_RET:
        break;
      case OP_PARSE_DOUBLE:
      case OP_PARSE_FLOAT:
      case OP_PARSE_INT64:
      case OP_PARSE_UINT64:
      case OP_PARSE_INT32:
      case OP_PARSE_FIXED64:
      case OP_PARSE_FIXED32:
      case OP_PARSE_BOOL:
      case OP_PARSE_UINT32:
      case OP_PARSE_SFIXED32:
      case OP_PARSE_SFIXED64:
      case OP_PARSE_SINT32:
      case OP_PARSE_SINT64:
      case OP_STARTSEQ:
      case OP_ENDSEQ:
      case OP_STARTSUBMSG:
      case OP_ENDSUBMSG:
      case OP_STARTSTR:
      case OP_STRING:
      case OP_ENDSTR:
      case OP_PUSHTAGDELIM:
        fprintf(f, " %d", instr >> 8);
        break;
      case OP_SETBIGGROUPNUM:
        fprintf(f, " %d", *p++);
        break;
      case OP_CHECKDELIM:
      case OP_CALL:
      case OP_BRANCH:
        fprintf(f, " =>0x%tx", p + getofs(instr) - begin);
        break;
      case OP_TAG1:
      case OP_TAG2: {
        fprintf(f, " tag:0x%x", instr >> 16);
        if (getofs(instr)) {
          fprintf(f, " =>0x%tx", p + getofs(instr) - begin);
        }
        break;
      }
      case OP_TAGN: {
        uint64_t tag = *p++;
        tag |= (uint64_t)*p++ << 32;
        fprintf(f, " tag:0x%llx", (long long)tag);
        fprintf(f, " n:%d", instr >> 16);
        if (getofs(instr)) {
          fprintf(f, " =>0x%tx", p + getofs(instr) - begin);
        }
        break;
      }
    }
    fputs("\n", f);
  }
}

#endif

static uint64_t get_encoded_tag(const upb_fielddef *f, int wire_type) {
  uint32_t tag = (upb_fielddef_number(f) << 3) | wire_type;
  uint64_t encoded_tag = upb_vencode32(tag);
  /* No tag should be greater than 5 bytes. */
  UPB_ASSERT(encoded_tag <= 0xffffffffff);
  return encoded_tag;
}

static void putchecktag(compiler *c, const upb_fielddef *f,
                        int wire_type, int dest) {
  uint64_t tag = get_encoded_tag(f, wire_type);
  switch (upb_value_size(tag)) {
    case 1:
      putop(c, OP_TAG1, dest, tag);
      break;
    case 2:
      putop(c, OP_TAG2, dest, tag);
      break;
    default:
      putop(c, OP_TAGN, dest, tag);
      break;
  }
}

static upb_selector_t getsel(const upb_fielddef *f, upb_handlertype_t type) {
  upb_selector_t selector;
  bool ok = upb_handlers_getselector(f, type, &selector);
  UPB_ASSERT(ok);
  return selector;
}

/* Takes an existing, primary dispatch table entry and repacks it with a
 * different alternate wire type.  Called when we are inserting a secondary
 * dispatch table entry for an alternate wire type. */
static uint64_t repack(uint64_t dispatch, int new_wt2) {
  uint64_t ofs;
  uint8_t wt1;
  uint8_t old_wt2;
  upb_pbdecoder_unpackdispatch(dispatch, &ofs, &wt1, &old_wt2);
  UPB_ASSERT(old_wt2 == NO_WIRE_TYPE);  /* wt2 should not be set yet. */
  return upb_pbdecoder_packdispatch(ofs, wt1, new_wt2);
}

/* Marks the current bytecode position as the dispatch target for this message,
 * field, and wire type. */
static void dispatchtarget(compiler *c, upb_pbdecodermethod *method,
                           const upb_fielddef *f, int wire_type) {
  /* Offset is relative to msg base. */
  uint64_t ofs = pcofs(c) - method->code_base.ofs;
  uint32_t fn = upb_fielddef_number(f);
  upb_inttable *d = &method->dispatch;
  upb_value v;
  if (upb_inttable_remove(d, fn, &v)) {
    /* TODO: prioritize based on packed setting in .proto file. */
    uint64_t repacked = repack(upb_value_getuint64(v), wire_type);
    upb_inttable_insert(d, fn, upb_value_uint64(repacked));
    upb_inttable_insert(d, fn + UPB_MAX_FIELDNUMBER, upb_value_uint64(ofs));
  } else {
    uint64_t val = upb_pbdecoder_packdispatch(ofs, wire_type, NO_WIRE_TYPE);
    upb_inttable_insert(d, fn, upb_value_uint64(val));
  }
}

static void putpush(compiler *c, const upb_fielddef *f) {
  if (upb_fielddef_descriptortype(f) == UPB_DESCRIPTOR_TYPE_MESSAGE) {
    putop(c, OP_PUSHLENDELIM);
  } else {
    uint32_t fn = upb_fielddef_number(f);
    if (fn >= 1 << 24) {
      putop(c, OP_PUSHTAGDELIM, 0);
      putop(c, OP_SETBIGGROUPNUM, fn);
    } else {
      putop(c, OP_PUSHTAGDELIM, fn);
    }
  }
}

static upb_pbdecodermethod *find_submethod(const compiler *c,
                                           const upb_pbdecodermethod *method,
                                           const upb_fielddef *f) {
  const upb_handlers *sub =
      upb_handlers_getsubhandlers(method->dest_handlers_, f);
  upb_value v;
  return upb_inttable_lookupptr(&c->group->methods, sub, &v)
             ? upb_value_getptr(v)
             : NULL;
}

static void putsel(compiler *c, opcode op, upb_selector_t sel,
                   const upb_handlers *h) {
  if (upb_handlers_gethandler(h, sel)) {
    putop(c, op, sel);
  }
}

/* Puts an opcode to call a callback, but only if a callback actually exists for
 * this field and handler type. */
static void maybeput(compiler *c, opcode op, const upb_handlers *h,
                     const upb_fielddef *f, upb_handlertype_t type) {
  putsel(c, op, getsel(f, type), h);
}

static bool haslazyhandlers(const upb_handlers *h, const upb_fielddef *f) {
  if (!upb_fielddef_lazy(f))
    return false;

  return upb_handlers_gethandler(h, getsel(f, UPB_HANDLER_STARTSTR)) ||
         upb_handlers_gethandler(h, getsel(f, UPB_HANDLER_STRING)) ||
         upb_handlers_gethandler(h, getsel(f, UPB_HANDLER_ENDSTR));
}


/* bytecode compiler code generation ******************************************/

/* Symbolic names for our local labels. */
#define LABEL_LOOPSTART 1  /* Top of a repeated field loop. */
#define LABEL_LOOPBREAK 2  /* To jump out of a repeated loop */
#define LABEL_FIELD     3  /* Jump backward to find the most recent field. */
#define LABEL_ENDMSG    4  /* To reach the OP_ENDMSG instr for this msg. */

/* Generates bytecode to parse a single non-lazy message field. */
static void generate_msgfield(compiler *c, const upb_fielddef *f,
                              upb_pbdecodermethod *method) {
  const upb_handlers *h = upb_pbdecodermethod_desthandlers(method);
  const upb_pbdecodermethod *sub_m = find_submethod(c, method, f);
  int wire_type;

  if (!sub_m) {
    /* Don't emit any code for this field at all; it will be parsed as an
     * unknown field.
     *
     * TODO(haberman): we should change this to parse it as a string field
     * instead.  It will probably be faster, but more importantly, once we
     * start vending unknown fields, a field shouldn't be treated as unknown
     * just because it doesn't have subhandlers registered. */
    return;
  }

  label(c, LABEL_FIELD);

  wire_type =
      (upb_fielddef_descriptortype(f) == UPB_DESCRIPTOR_TYPE_MESSAGE)
          ? UPB_WIRE_TYPE_DELIMITED
          : UPB_WIRE_TYPE_START_GROUP;

  if (upb_fielddef_isseq(f)) {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, wire_type, LABEL_DISPATCH);
   dispatchtarget(c, method, f, wire_type);
    putop(c, OP_PUSHTAGDELIM, 0);
    putop(c, OP_STARTSEQ, getsel(f, UPB_HANDLER_STARTSEQ));
   label(c, LABEL_LOOPSTART);
    putpush(c, f);
    putop(c, OP_STARTSUBMSG, getsel(f, UPB_HANDLER_STARTSUBMSG));
    putop(c, OP_CALL, sub_m);
    putop(c, OP_POP);
    maybeput(c, OP_ENDSUBMSG, h, f, UPB_HANDLER_ENDSUBMSG);
    if (wire_type == UPB_WIRE_TYPE_DELIMITED) {
      putop(c, OP_SETDELIM);
    }
    putop(c, OP_CHECKDELIM, LABEL_LOOPBREAK);
    putchecktag(c, f, wire_type, LABEL_LOOPBREAK);
    putop(c, OP_BRANCH, -LABEL_LOOPSTART);
   label(c, LABEL_LOOPBREAK);
    putop(c, OP_POP);
    maybeput(c, OP_ENDSEQ, h, f, UPB_HANDLER_ENDSEQ);
  } else {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, wire_type, LABEL_DISPATCH);
   dispatchtarget(c, method, f, wire_type);
    putpush(c, f);
    putop(c, OP_STARTSUBMSG, getsel(f, UPB_HANDLER_STARTSUBMSG));
    putop(c, OP_CALL, sub_m);
    putop(c, OP_POP);
    maybeput(c, OP_ENDSUBMSG, h, f, UPB_HANDLER_ENDSUBMSG);
    if (wire_type == UPB_WIRE_TYPE_DELIMITED) {
      putop(c, OP_SETDELIM);
    }
  }
}

/* Generates bytecode to parse a single string or lazy submessage field. */
static void generate_delimfield(compiler *c, const upb_fielddef *f,
                                upb_pbdecodermethod *method) {
  const upb_handlers *h = upb_pbdecodermethod_desthandlers(method);

  label(c, LABEL_FIELD);
  if (upb_fielddef_isseq(f)) {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, UPB_WIRE_TYPE_DELIMITED, LABEL_DISPATCH);
   dispatchtarget(c, method, f, UPB_WIRE_TYPE_DELIMITED);
    putop(c, OP_PUSHTAGDELIM, 0);
    putop(c, OP_STARTSEQ, getsel(f, UPB_HANDLER_STARTSEQ));
   label(c, LABEL_LOOPSTART);
    putop(c, OP_PUSHLENDELIM);
    putop(c, OP_STARTSTR, getsel(f, UPB_HANDLER_STARTSTR));
    /* Need to emit even if no handler to skip past the string. */
    putop(c, OP_STRING, getsel(f, UPB_HANDLER_STRING));
    putop(c, OP_POP);
    maybeput(c, OP_ENDSTR, h, f, UPB_HANDLER_ENDSTR);
    putop(c, OP_SETDELIM);
    putop(c, OP_CHECKDELIM, LABEL_LOOPBREAK);
    putchecktag(c, f, UPB_WIRE_TYPE_DELIMITED, LABEL_LOOPBREAK);
    putop(c, OP_BRANCH, -LABEL_LOOPSTART);
   label(c, LABEL_LOOPBREAK);
    putop(c, OP_POP);
    maybeput(c, OP_ENDSEQ, h, f, UPB_HANDLER_ENDSEQ);
  } else {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, UPB_WIRE_TYPE_DELIMITED, LABEL_DISPATCH);
   dispatchtarget(c, method, f, UPB_WIRE_TYPE_DELIMITED);
    putop(c, OP_PUSHLENDELIM);
    putop(c, OP_STARTSTR, getsel(f, UPB_HANDLER_STARTSTR));
    putop(c, OP_STRING, getsel(f, UPB_HANDLER_STRING));
    putop(c, OP_POP);
    maybeput(c, OP_ENDSTR, h, f, UPB_HANDLER_ENDSTR);
    putop(c, OP_SETDELIM);
  }
}

/* Generates bytecode to parse a single primitive field. */
static void generate_primitivefield(compiler *c, const upb_fielddef *f,
                                    upb_pbdecodermethod *method) {
  const upb_handlers *h = upb_pbdecodermethod_desthandlers(method);
  upb_descriptortype_t descriptor_type = upb_fielddef_descriptortype(f);
  opcode parse_type;
  upb_selector_t sel;
  int wire_type;

  label(c, LABEL_FIELD);

  /* From a decoding perspective, ENUM is the same as INT32. */
  if (descriptor_type == UPB_DESCRIPTOR_TYPE_ENUM)
    descriptor_type = UPB_DESCRIPTOR_TYPE_INT32;

  parse_type = (opcode)descriptor_type;

  /* TODO(haberman): generate packed or non-packed first depending on "packed"
   * setting in the fielddef.  This will favor (in speed) whichever was
   * specified. */

  UPB_ASSERT((int)parse_type >= 0 && parse_type <= OP_MAX);
  sel = getsel(f, upb_handlers_getprimitivehandlertype(f));
  wire_type = upb_pb_native_wire_types[upb_fielddef_descriptortype(f)];
  if (upb_fielddef_isseq(f)) {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, UPB_WIRE_TYPE_DELIMITED, LABEL_DISPATCH);
   dispatchtarget(c, method, f, UPB_WIRE_TYPE_DELIMITED);
    putop(c, OP_PUSHLENDELIM);
    putop(c, OP_STARTSEQ, getsel(f, UPB_HANDLER_STARTSEQ));  /* Packed */
   label(c, LABEL_LOOPSTART);
    putop(c, parse_type, sel);
    putop(c, OP_CHECKDELIM, LABEL_LOOPBREAK);
    putop(c, OP_BRANCH, -LABEL_LOOPSTART);
   dispatchtarget(c, method, f, wire_type);
    putop(c, OP_PUSHTAGDELIM, 0);
    putop(c, OP_STARTSEQ, getsel(f, UPB_HANDLER_STARTSEQ));  /* Non-packed */
   label(c, LABEL_LOOPSTART);
    putop(c, parse_type, sel);
    putop(c, OP_CHECKDELIM, LABEL_LOOPBREAK);
    putchecktag(c, f, wire_type, LABEL_LOOPBREAK);
    putop(c, OP_BRANCH, -LABEL_LOOPSTART);
   label(c, LABEL_LOOPBREAK);
    putop(c, OP_POP);  /* Packed and non-packed join. */
    maybeput(c, OP_ENDSEQ, h, f, UPB_HANDLER_ENDSEQ);
    putop(c, OP_SETDELIM);  /* Could remove for non-packed by dup ENDSEQ. */
  } else {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, wire_type, LABEL_DISPATCH);
   dispatchtarget(c, method, f, wire_type);
    putop(c, parse_type, sel);
  }
}

/* Adds bytecode for parsing the given message to the given decoderplan,
 * while adding all dispatch targets to this message's dispatch table. */
static void compile_method(compiler *c, upb_pbdecodermethod *method) {
  const upb_handlers *h;
  const upb_msgdef *md;
  uint32_t* start_pc;
  upb_msg_field_iter i;
  upb_value val;

  UPB_ASSERT(method);

  /* Clear all entries in the dispatch table. */
  upb_inttable_uninit(&method->dispatch);
  upb_inttable_init(&method->dispatch, UPB_CTYPE_UINT64);

  h = upb_pbdecodermethod_desthandlers(method);
  md = upb_handlers_msgdef(h);

 method->code_base.ofs = pcofs(c);
  putop(c, OP_SETDISPATCH, &method->dispatch);
  putsel(c, OP_STARTMSG, UPB_STARTMSG_SELECTOR, h);
 label(c, LABEL_FIELD);
  start_pc = c->pc;
  for(upb_msg_field_begin(&i, md);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    upb_fieldtype_t type = upb_fielddef_type(f);

    if (type == UPB_TYPE_MESSAGE && !(haslazyhandlers(h, f) && c->lazy)) {
      generate_msgfield(c, f, method);
    } else if (type == UPB_TYPE_STRING || type == UPB_TYPE_BYTES ||
               type == UPB_TYPE_MESSAGE) {
      generate_delimfield(c, f, method);
    } else {
      generate_primitivefield(c, f, method);
    }
  }

  /* If there were no fields, or if no handlers were defined, we need to
   * generate a non-empty loop body so that we can at least dispatch for unknown
   * fields and check for the end of the message. */
  if (c->pc == start_pc) {
    /* Check for end-of-message. */
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    /* Unconditionally dispatch. */
    putop(c, OP_DISPATCH, 0);
  }

  /* For now we just loop back to the last field of the message (or if none,
   * the DISPATCH opcode for the message). */
  putop(c, OP_BRANCH, -LABEL_FIELD);

  /* Insert both a label and a dispatch table entry for this end-of-msg. */
 label(c, LABEL_ENDMSG);
  val = upb_value_uint64(pcofs(c) - method->code_base.ofs);
  upb_inttable_insert(&method->dispatch, DISPATCH_ENDMSG, val);

  putsel(c, OP_ENDMSG, UPB_ENDMSG_SELECTOR, h);
  putop(c, OP_RET);

  upb_inttable_compact(&method->dispatch);
}

/* Populate "methods" with new upb_pbdecodermethod objects reachable from "h".
 * Returns the method for these handlers.
 *
 * Generates a new method for every destination handlers reachable from "h". */
static void find_methods(compiler *c, const upb_handlers *h) {
  upb_value v;
  upb_msg_field_iter i;
  const upb_msgdef *md;

  if (upb_inttable_lookupptr(&c->group->methods, h, &v))
    return;
  newmethod(h, c->group);

  /* Find submethods. */
  md = upb_handlers_msgdef(h);
  for(upb_msg_field_begin(&i, md);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    const upb_handlers *sub_h;
    if (upb_fielddef_type(f) == UPB_TYPE_MESSAGE &&
        (sub_h = upb_handlers_getsubhandlers(h, f)) != NULL) {
      /* We only generate a decoder method for submessages with handlers.
       * Others will be parsed as unknown fields. */
      find_methods(c, sub_h);
    }
  }
}

/* (Re-)compile bytecode for all messages in "msgs."
 * Overwrites any existing bytecode in "c". */
static void compile_methods(compiler *c) {
  upb_inttable_iter i;

  /* Start over at the beginning of the bytecode. */
  c->pc = c->group->bytecode;

  upb_inttable_begin(&i, &c->group->methods);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_pbdecodermethod *method = upb_value_getptr(upb_inttable_iter_value(&i));
    compile_method(c, method);
  }
}

static void set_bytecode_handlers(mgroup *g) {
  upb_inttable_iter i;
  upb_inttable_begin(&i, &g->methods);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_pbdecodermethod *m = upb_value_getptr(upb_inttable_iter_value(&i));
    upb_byteshandler *h = &m->input_handler_;

    m->code_base.ptr = g->bytecode + m->code_base.ofs;

    upb_byteshandler_setstartstr(h, upb_pbdecoder_startbc, m->code_base.ptr);
    upb_byteshandler_setstring(h, upb_pbdecoder_decode, g);
    upb_byteshandler_setendstr(h, upb_pbdecoder_end, m);
  }
}


/* JIT setup. *****************************************************************/

#ifdef UPB_USE_JIT_X64

static void sethandlers(mgroup *g, bool allowjit) {
  g->jit_code = NULL;
  if (allowjit) {
    /* Compile byte-code into machine code, create handlers. */
    upb_pbdecoder_jit(g);
  } else {
    set_bytecode_handlers(g);
  }
}

#else  /* UPB_USE_JIT_X64 */

static void sethandlers(mgroup *g, bool allowjit) {
  /* No JIT compiled in; use bytecode handlers unconditionally. */
  UPB_UNUSED(allowjit);
  set_bytecode_handlers(g);
}

#endif  /* UPB_USE_JIT_X64 */


/* TODO(haberman): allow this to be constructed for an arbitrary set of dest
 * handlers and other mgroups (but verify we have a transitive closure). */
const mgroup *mgroup_new(const upb_handlers *dest, bool allowjit, bool lazy,
                         const void *owner) {
  mgroup *g;
  compiler *c;

  UPB_UNUSED(allowjit);
  UPB_ASSERT(upb_handlers_isfrozen(dest));

  g = newgroup(owner);
  c = newcompiler(g, lazy);
  find_methods(c, dest);

  /* We compile in two passes:
   * 1. all messages are assigned relative offsets from the beginning of the
   *    bytecode (saved in method->code_base).
   * 2. forwards OP_CALL instructions can be correctly linked since message
   *    offsets have been previously assigned.
   *
   * Could avoid the second pass by linking OP_CALL instructions somehow. */
  compile_methods(c);
  compile_methods(c);
  g->bytecode_end = c->pc;
  freecompiler(c);

#ifdef UPB_DUMP_BYTECODE
  {
    FILE *f = fopen("/tmp/upb-bytecode", "w");
    UPB_ASSERT(f);
    dumpbc(g->bytecode, g->bytecode_end, stderr);
    dumpbc(g->bytecode, g->bytecode_end, f);
    fclose(f);

    f = fopen("/tmp/upb-bytecode.bin", "wb");
    UPB_ASSERT(f);
    fwrite(g->bytecode, 1, g->bytecode_end - g->bytecode, f);
    fclose(f);
  }
#endif

  sethandlers(g, allowjit);
  return g;
}


/* upb_pbcodecache ************************************************************/

void upb_pbcodecache_init(upb_pbcodecache *c) {
  upb_inttable_init(&c->groups, UPB_CTYPE_CONSTPTR);
  c->allow_jit_ = true;
}

void upb_pbcodecache_uninit(upb_pbcodecache *c) {
  upb_inttable_iter i;
  upb_inttable_begin(&i, &c->groups);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    const mgroup *group = upb_value_getconstptr(upb_inttable_iter_value(&i));
    mgroup_unref(group, c);
  }
  upb_inttable_uninit(&c->groups);
}

bool upb_pbcodecache_allowjit(const upb_pbcodecache *c) {
  return c->allow_jit_;
}

bool upb_pbcodecache_setallowjit(upb_pbcodecache *c, bool allow) {
  if (upb_inttable_count(&c->groups) > 0)
    return false;
  c->allow_jit_ = allow;
  return true;
}

const upb_pbdecodermethod *upb_pbcodecache_getdecodermethod(
    upb_pbcodecache *c, const upb_pbdecodermethodopts *opts) {
  upb_value v;
  bool ok;

  /* Right now we build a new DecoderMethod every time.
   * TODO(haberman): properly cache methods by their true key. */
  const mgroup *g = mgroup_new(opts->handlers, c->allow_jit_, opts->lazy, c);
  upb_inttable_push(&c->groups, upb_value_constptr(g));

  ok = upb_inttable_lookupptr(&g->methods, opts->handlers, &v);
  UPB_ASSERT(ok);
  return upb_value_getptr(v);
}


/* upb_pbdecodermethodopts ****************************************************/

void upb_pbdecodermethodopts_init(upb_pbdecodermethodopts *opts,
                                  const upb_handlers *h) {
  opts->handlers = h;
  opts->lazy = false;
}

void upb_pbdecodermethodopts_setlazy(upb_pbdecodermethodopts *opts, bool lazy) {
  opts->lazy = lazy;
}
/*
** upb::Decoder (Bytecode Decoder VM)
**
** Bytecode must previously have been generated using the bytecode compiler in
** compile_decoder.c.  This decoder then walks through the bytecode op-by-op to
** parse the input.
**
** Decoding is fully resumable; we just keep a pointer to the current bytecode
** instruction and resume from there.  A fair amount of the logic here is to
** handle the fact that values can span buffer seams and we have to be able to
** be capable of suspending/resuming from any byte in the stream.  This
** sometimes requires keeping a few trailing bytes from the last buffer around
** in the "residual" buffer.
*/

#include <inttypes.h>
#include <stddef.h>

#ifdef UPB_DUMP_BYTECODE
#include <stdio.h>
#endif

#define CHECK_SUSPEND(x) if (!(x)) return upb_pbdecoder_suspend(d);

/* Error messages that are shared between the bytecode and JIT decoders. */
const char *kPbDecoderStackOverflow = "Nesting too deep.";
const char *kPbDecoderSubmessageTooLong =
    "Submessage end extends past enclosing submessage.";

/* Error messages shared within this file. */
static const char *kUnterminatedVarint = "Unterminated varint.";

/* upb_pbdecoder **************************************************************/

static opcode halt = OP_HALT;

/* A dummy character we can point to when the user passes us a NULL buffer.
 * We need this because in C (NULL + 0) and (NULL - NULL) are undefined
 * behavior, which would invalidate functions like curbufleft(). */
static const char dummy_char;

/* Whether an op consumes any of the input buffer. */
static bool consumes_input(opcode op) {
  switch (op) {
    case OP_SETDISPATCH:
    case OP_STARTMSG:
    case OP_ENDMSG:
    case OP_STARTSEQ:
    case OP_ENDSEQ:
    case OP_STARTSUBMSG:
    case OP_ENDSUBMSG:
    case OP_STARTSTR:
    case OP_ENDSTR:
    case OP_PUSHTAGDELIM:
    case OP_POP:
    case OP_SETDELIM:
    case OP_SETBIGGROUPNUM:
    case OP_CHECKDELIM:
    case OP_CALL:
    case OP_RET:
    case OP_BRANCH:
      return false;
    default:
      return true;
  }
}

static size_t stacksize(upb_pbdecoder *d, size_t entries) {
  UPB_UNUSED(d);
  return entries * sizeof(upb_pbdecoder_frame);
}

static size_t callstacksize(upb_pbdecoder *d, size_t entries) {
  UPB_UNUSED(d);

#ifdef UPB_USE_JIT_X64
  if (d->method_->is_native_) {
    /* Each native stack frame needs two pointers, plus we need a few frames for
     * the enter/exit trampolines. */
    size_t ret = entries * sizeof(void*) * 2;
    ret += sizeof(void*) * 10;
    return ret;
  }
#endif

  return entries * sizeof(uint32_t*);
}


static bool in_residual_buf(const upb_pbdecoder *d, const char *p);

/* It's unfortunate that we have to micro-manage the compiler with
 * UPB_FORCEINLINE and UPB_NOINLINE, especially since this tuning is necessarily
 * specific to one hardware configuration.  But empirically on a Core i7,
 * performance increases 30-50% with these annotations.  Every instance where
 * these appear, gcc 4.2.1 made the wrong decision and degraded performance in
 * benchmarks. */

static void seterr(upb_pbdecoder *d, const char *msg) {
  upb_status status = UPB_STATUS_INIT;
  upb_status_seterrmsg(&status, msg);
  upb_env_reporterror(d->env, &status);
}

void upb_pbdecoder_seterr(upb_pbdecoder *d, const char *msg) {
  seterr(d, msg);
}


/* Buffering ******************************************************************/

/* We operate on one buffer at a time, which is either the user's buffer passed
 * to our "decode" callback or some residual bytes from the previous buffer. */

/* How many bytes can be safely read from d->ptr without reading past end-of-buf
 * or past the current delimited end. */
static size_t curbufleft(const upb_pbdecoder *d) {
  UPB_ASSERT(d->data_end >= d->ptr);
  return d->data_end - d->ptr;
}

/* How many bytes are available before end-of-buffer. */
static size_t bufleft(const upb_pbdecoder *d) {
  return d->end - d->ptr;
}

/* Overall stream offset of d->ptr. */
uint64_t offset(const upb_pbdecoder *d) {
  return d->bufstart_ofs + (d->ptr - d->buf);
}

/* How many bytes are available before the end of this delimited region. */
size_t delim_remaining(const upb_pbdecoder *d) {
  return d->top->end_ofs - offset(d);
}

/* Advances d->ptr. */
static void advance(upb_pbdecoder *d, size_t len) {
  UPB_ASSERT(curbufleft(d) >= len);
  d->ptr += len;
}

static bool in_buf(const char *p, const char *buf, const char *end) {
  return p >= buf && p <= end;
}

static bool in_residual_buf(const upb_pbdecoder *d, const char *p) {
  return in_buf(p, d->residual, d->residual_end);
}

/* Calculates the delim_end value, which is affected by both the current buffer
 * and the parsing stack, so must be called whenever either is updated. */
static void set_delim_end(upb_pbdecoder *d) {
  size_t delim_ofs = d->top->end_ofs - d->bufstart_ofs;
  if (delim_ofs <= (size_t)(d->end - d->buf)) {
    d->delim_end = d->buf + delim_ofs;
    d->data_end = d->delim_end;
  } else {
    d->data_end = d->end;
    d->delim_end = NULL;
  }
}

static void switchtobuf(upb_pbdecoder *d, const char *buf, const char *end) {
  d->ptr = buf;
  d->buf = buf;
  d->end = end;
  set_delim_end(d);
}

static void advancetobuf(upb_pbdecoder *d, const char *buf, size_t len) {
  UPB_ASSERT(curbufleft(d) == 0);
  d->bufstart_ofs += (d->end - d->buf);
  switchtobuf(d, buf, buf + len);
}

static void checkpoint(upb_pbdecoder *d) {
  /* The assertion here is in the interests of efficiency, not correctness.
   * We are trying to ensure that we don't checkpoint() more often than
   * necessary. */
  UPB_ASSERT(d->checkpoint != d->ptr);
  d->checkpoint = d->ptr;
}

/* Skips "bytes" bytes in the stream, which may be more than available.  If we
 * skip more bytes than are available, we return a long read count to the caller
 * indicating how many bytes can be skipped over before passing actual data
 * again.  Skipped bytes can pass a NULL buffer and the decoder guarantees they
 * won't actually be read.
 */
static int32_t skip(upb_pbdecoder *d, size_t bytes) {
  UPB_ASSERT(!in_residual_buf(d, d->ptr) || d->size_param == 0);
  UPB_ASSERT(d->skip == 0);
  if (bytes > delim_remaining(d)) {
    seterr(d, "Skipped value extended beyond enclosing submessage.");
    return upb_pbdecoder_suspend(d);
  } else if (bufleft(d) >= bytes) {
    /* Skipped data is all in current buffer, and more is still available. */
    advance(d, bytes);
    d->skip = 0;
    return DECODE_OK;
  } else {
    /* Skipped data extends beyond currently available buffers. */
    d->pc = d->last;
    d->skip = bytes - curbufleft(d);
    d->bufstart_ofs += (d->end - d->buf);
    d->residual_end = d->residual;
    switchtobuf(d, d->residual, d->residual_end);
    return d->size_param + d->skip;
  }
}


/* Resumes the decoder from an initial state or from a previous suspend. */
int32_t upb_pbdecoder_resume(upb_pbdecoder *d, void *p, const char *buf,
                             size_t size, const upb_bufhandle *handle) {
  UPB_UNUSED(p);  /* Useless; just for the benefit of the JIT. */

  /* d->skip and d->residual_end could probably elegantly be represented
   * as a single variable, to more easily represent this invariant. */
  UPB_ASSERT(!(d->skip && d->residual_end > d->residual));

  /* We need to remember the original size_param, so that the value we return
   * is relative to it, even if we do some skipping first. */
  d->size_param = size;
  d->handle = handle;

  /* Have to handle this case specially (ie. not with skip()) because the user
   * is allowed to pass a NULL buffer here, which won't allow us to safely
   * calculate a d->end or use our normal functions like curbufleft(). */
  if (d->skip && d->skip >= size) {
    d->skip -= size;
    d->bufstart_ofs += size;
    buf = &dummy_char;
    size = 0;

    /* We can't just return now, because we might need to execute some ops
     * like CHECKDELIM, which could call some callbacks and pop the stack. */
  }

  /* We need to pretend that this was the actual buffer param, since some of the
   * calculations assume that d->ptr/d->buf is relative to this. */
  d->buf_param = buf;

  if (!buf) {
    /* NULL buf is ok if its entire span is covered by the "skip" above, but
     * by this point we know that "skip" doesn't cover the buffer. */
    seterr(d, "Passed NULL buffer over non-skippable region.");
    return upb_pbdecoder_suspend(d);
  }

  if (d->residual_end > d->residual) {
    /* We have residual bytes from the last buffer. */
    UPB_ASSERT(d->ptr == d->residual);
  } else {
    switchtobuf(d, buf, buf + size);
  }

  d->checkpoint = d->ptr;

  /* Handle skips that don't cover the whole buffer (as above). */
  if (d->skip) {
    size_t skip_bytes = d->skip;
    d->skip = 0;
    CHECK_RETURN(skip(d, skip_bytes));
    checkpoint(d);
  }

  /* If we're inside an unknown group, continue to parse unknown values. */
  if (d->top->groupnum < 0) {
    CHECK_RETURN(upb_pbdecoder_skipunknown(d, -1, 0));
    checkpoint(d);
  }

  return DECODE_OK;
}

/* Suspends the decoder at the last checkpoint, without saving any residual
 * bytes.  If there are any unconsumed bytes, returns a short byte count. */
size_t upb_pbdecoder_suspend(upb_pbdecoder *d) {
  d->pc = d->last;
  if (d->checkpoint == d->residual) {
    /* Checkpoint was in residual buf; no user bytes were consumed. */
    d->ptr = d->residual;
    return 0;
  } else {
    size_t ret = d->size_param - (d->end - d->checkpoint);
    UPB_ASSERT(!in_residual_buf(d, d->checkpoint));
    UPB_ASSERT(d->buf == d->buf_param || d->buf == &dummy_char);

    d->bufstart_ofs += (d->checkpoint - d->buf);
    d->residual_end = d->residual;
    switchtobuf(d, d->residual, d->residual_end);
    return ret;
  }
}

/* Suspends the decoder at the last checkpoint, and saves any unconsumed
 * bytes in our residual buffer.  This is necessary if we need more user
 * bytes to form a complete value, which might not be contiguous in the
 * user's buffers.  Always consumes all user bytes. */
static size_t suspend_save(upb_pbdecoder *d) {
  /* We hit end-of-buffer before we could parse a full value.
   * Save any unconsumed bytes (if any) to the residual buffer. */
  d->pc = d->last;

  if (d->checkpoint == d->residual) {
    /* Checkpoint was in residual buf; append user byte(s) to residual buf. */
    UPB_ASSERT((d->residual_end - d->residual) + d->size_param <=
           sizeof(d->residual));
    if (!in_residual_buf(d, d->ptr)) {
      d->bufstart_ofs -= (d->residual_end - d->residual);
    }
    memcpy(d->residual_end, d->buf_param, d->size_param);
    d->residual_end += d->size_param;
  } else {
    /* Checkpoint was in user buf; old residual bytes not needed. */
    size_t save;
    UPB_ASSERT(!in_residual_buf(d, d->checkpoint));

    d->ptr = d->checkpoint;
    save = curbufleft(d);
    UPB_ASSERT(save <= sizeof(d->residual));
    memcpy(d->residual, d->ptr, save);
    d->residual_end = d->residual + save;
    d->bufstart_ofs = offset(d);
  }

  switchtobuf(d, d->residual, d->residual_end);
  return d->size_param;
}

/* Copies the next "bytes" bytes into "buf" and advances the stream.
 * Requires that this many bytes are available in the current buffer. */
UPB_FORCEINLINE static void consumebytes(upb_pbdecoder *d, void *buf,
                                         size_t bytes) {
  UPB_ASSERT(bytes <= curbufleft(d));
  memcpy(buf, d->ptr, bytes);
  advance(d, bytes);
}

/* Slow path for getting the next "bytes" bytes, regardless of whether they are
 * available in the current buffer or not.  Returns a status code as described
 * in decoder.int.h. */
UPB_NOINLINE static int32_t getbytes_slow(upb_pbdecoder *d, void *buf,
                                          size_t bytes) {
  const size_t avail = curbufleft(d);
  consumebytes(d, buf, avail);
  bytes -= avail;
  UPB_ASSERT(bytes > 0);
  if (in_residual_buf(d, d->ptr)) {
    advancetobuf(d, d->buf_param, d->size_param);
  }
  if (curbufleft(d) >= bytes) {
    consumebytes(d, (char *)buf + avail, bytes);
    return DECODE_OK;
  } else if (d->data_end == d->delim_end) {
    seterr(d, "Submessage ended in the middle of a value or group");
    return upb_pbdecoder_suspend(d);
  } else {
    return suspend_save(d);
  }
}

/* Gets the next "bytes" bytes, regardless of whether they are available in the
 * current buffer or not.  Returns a status code as described in decoder.int.h.
 */
UPB_FORCEINLINE static int32_t getbytes(upb_pbdecoder *d, void *buf,
                                        size_t bytes) {
  if (curbufleft(d) >= bytes) {
    /* Buffer has enough data to satisfy. */
    consumebytes(d, buf, bytes);
    return DECODE_OK;
  } else {
    return getbytes_slow(d, buf, bytes);
  }
}

UPB_NOINLINE static size_t peekbytes_slow(upb_pbdecoder *d, void *buf,
                                          size_t bytes) {
  size_t ret = curbufleft(d);
  memcpy(buf, d->ptr, ret);
  if (in_residual_buf(d, d->ptr)) {
    size_t copy = UPB_MIN(bytes - ret, d->size_param);
    memcpy((char *)buf + ret, d->buf_param, copy);
    ret += copy;
  }
  return ret;
}

UPB_FORCEINLINE static size_t peekbytes(upb_pbdecoder *d, void *buf,
                                        size_t bytes) {
  if (curbufleft(d) >= bytes) {
    memcpy(buf, d->ptr, bytes);
    return bytes;
  } else {
    return peekbytes_slow(d, buf, bytes);
  }
}


/* Decoding of wire types *****************************************************/

/* Slow path for decoding a varint from the current buffer position.
 * Returns a status code as described in decoder.int.h. */
UPB_NOINLINE int32_t upb_pbdecoder_decode_varint_slow(upb_pbdecoder *d,
                                                      uint64_t *u64) {
  uint8_t byte = 0x80;
  int bitpos;
  *u64 = 0;
  for(bitpos = 0; bitpos < 70 && (byte & 0x80); bitpos += 7) {
    CHECK_RETURN(getbytes(d, &byte, 1));
    *u64 |= (uint64_t)(byte & 0x7F) << bitpos;
  }
  if(bitpos == 70 && (byte & 0x80)) {
    seterr(d, kUnterminatedVarint);
    return upb_pbdecoder_suspend(d);
  }
  return DECODE_OK;
}

/* Decodes a varint from the current buffer position.
 * Returns a status code as described in decoder.int.h. */
UPB_FORCEINLINE static int32_t decode_varint(upb_pbdecoder *d, uint64_t *u64) {
  if (curbufleft(d) > 0 && !(*d->ptr & 0x80)) {
    *u64 = *d->ptr;
    advance(d, 1);
    return DECODE_OK;
  } else if (curbufleft(d) >= 10) {
    /* Fast case. */
    upb_decoderet r = upb_vdecode_fast(d->ptr);
    if (r.p == NULL) {
      seterr(d, kUnterminatedVarint);
      return upb_pbdecoder_suspend(d);
    }
    advance(d, r.p - d->ptr);
    *u64 = r.val;
    return DECODE_OK;
  } else {
    /* Slow case -- varint spans buffer seam. */
    return upb_pbdecoder_decode_varint_slow(d, u64);
  }
}

/* Decodes a 32-bit varint from the current buffer position.
 * Returns a status code as described in decoder.int.h. */
UPB_FORCEINLINE static int32_t decode_v32(upb_pbdecoder *d, uint32_t *u32) {
  uint64_t u64;
  int32_t ret = decode_varint(d, &u64);
  if (ret >= 0) return ret;
  if (u64 > UINT32_MAX) {
    seterr(d, "Unterminated 32-bit varint");
    /* TODO(haberman) guarantee that this function return is >= 0 somehow,
     * so we know this path will always be treated as error by our caller.
     * Right now the size_t -> int32_t can overflow and produce negative values.
     */
    *u32 = 0;
    return upb_pbdecoder_suspend(d);
  }
  *u32 = u64;
  return DECODE_OK;
}

/* Decodes a fixed32 from the current buffer position.
 * Returns a status code as described in decoder.int.h.
 * TODO: proper byte swapping for big-endian machines. */
UPB_FORCEINLINE static int32_t decode_fixed32(upb_pbdecoder *d, uint32_t *u32) {
  return getbytes(d, u32, 4);
}

/* Decodes a fixed64 from the current buffer position.
 * Returns a status code as described in decoder.int.h.
 * TODO: proper byte swapping for big-endian machines. */
UPB_FORCEINLINE static int32_t decode_fixed64(upb_pbdecoder *d, uint64_t *u64) {
  return getbytes(d, u64, 8);
}

/* Non-static versions of the above functions.
 * These are called by the JIT for fallback paths. */
int32_t upb_pbdecoder_decode_f32(upb_pbdecoder *d, uint32_t *u32) {
  return decode_fixed32(d, u32);
}

int32_t upb_pbdecoder_decode_f64(upb_pbdecoder *d, uint64_t *u64) {
  return decode_fixed64(d, u64);
}

static double as_double(uint64_t n) { double d; memcpy(&d, &n, 8); return d; }
static float  as_float(uint32_t n)  { float  f; memcpy(&f, &n, 4); return f; }

/* Pushes a frame onto the decoder stack. */
static bool decoder_push(upb_pbdecoder *d, uint64_t end) {
  upb_pbdecoder_frame *fr = d->top;

  if (end > fr->end_ofs) {
    seterr(d, kPbDecoderSubmessageTooLong);
    return false;
  } else if (fr == d->limit) {
    seterr(d, kPbDecoderStackOverflow);
    return false;
  }

  fr++;
  fr->end_ofs = end;
  fr->dispatch = NULL;
  fr->groupnum = 0;
  d->top = fr;
  return true;
}

static bool pushtagdelim(upb_pbdecoder *d, uint32_t arg) {
  /* While we expect to see an "end" tag (either ENDGROUP or a non-sequence
   * field number) prior to hitting any enclosing submessage end, pushing our
   * existing delim end prevents us from continuing to parse values from a
   * corrupt proto that doesn't give us an END tag in time. */
  if (!decoder_push(d, d->top->end_ofs))
    return false;
  d->top->groupnum = arg;
  return true;
}

/* Pops a frame from the decoder stack. */
static void decoder_pop(upb_pbdecoder *d) { d->top--; }

UPB_NOINLINE int32_t upb_pbdecoder_checktag_slow(upb_pbdecoder *d,
                                                 uint64_t expected) {
  uint64_t data = 0;
  size_t bytes = upb_value_size(expected);
  size_t read = peekbytes(d, &data, bytes);
  if (read == bytes && data == expected) {
    /* Advance past matched bytes. */
    int32_t ok = getbytes(d, &data, read);
    UPB_ASSERT(ok < 0);
    return DECODE_OK;
  } else if (read < bytes && memcmp(&data, &expected, read) == 0) {
    return suspend_save(d);
  } else {
    return DECODE_MISMATCH;
  }
}

int32_t upb_pbdecoder_skipunknown(upb_pbdecoder *d, int32_t fieldnum,
                                  uint8_t wire_type) {
  if (fieldnum >= 0)
    goto have_tag;

  while (true) {
    uint32_t tag;
    CHECK_RETURN(decode_v32(d, &tag));
    wire_type = tag & 0x7;
    fieldnum = tag >> 3;

have_tag:
    if (fieldnum == 0) {
      seterr(d, "Saw invalid field number (0)");
      return upb_pbdecoder_suspend(d);
    }

    switch (wire_type) {
      case UPB_WIRE_TYPE_32BIT:
        CHECK_RETURN(skip(d, 4));
        break;
      case UPB_WIRE_TYPE_64BIT:
        CHECK_RETURN(skip(d, 8));
        break;
      case UPB_WIRE_TYPE_VARINT: {
        uint64_t u64;
        CHECK_RETURN(decode_varint(d, &u64));
        break;
      }
      case UPB_WIRE_TYPE_DELIMITED: {
        uint32_t len;
        CHECK_RETURN(decode_v32(d, &len));
        CHECK_RETURN(skip(d, len));
        break;
      }
      case UPB_WIRE_TYPE_START_GROUP:
        CHECK_SUSPEND(pushtagdelim(d, -fieldnum));
        break;
      case UPB_WIRE_TYPE_END_GROUP:
        if (fieldnum == -d->top->groupnum) {
          decoder_pop(d);
        } else if (fieldnum == d->top->groupnum) {
          return DECODE_ENDGROUP;
        } else {
          seterr(d, "Unmatched ENDGROUP tag.");
          return upb_pbdecoder_suspend(d);
        }
        break;
      default:
        seterr(d, "Invalid wire type");
        return upb_pbdecoder_suspend(d);
    }

    if (d->top->groupnum >= 0) {
      /* TODO: More code needed for handling unknown groups. */
      upb_sink_putunknown(&d->top->sink, d->checkpoint, d->ptr - d->checkpoint);
      return DECODE_OK;
    }

    /* Unknown group -- continue looping over unknown fields. */
    checkpoint(d);
  }
}

static void goto_endmsg(upb_pbdecoder *d) {
  upb_value v;
  bool found = upb_inttable_lookup32(d->top->dispatch, DISPATCH_ENDMSG, &v);
  UPB_ASSERT(found);
  d->pc = d->top->base + upb_value_getuint64(v);
}

/* Parses a tag and jumps to the corresponding bytecode instruction for this
 * field.
 *
 * If the tag is unknown (or the wire type doesn't match), parses the field as
 * unknown.  If the tag is a valid ENDGROUP tag, jumps to the bytecode
 * instruction for the end of message. */
static int32_t dispatch(upb_pbdecoder *d) {
  upb_inttable *dispatch = d->top->dispatch;
  uint32_t tag;
  uint8_t wire_type;
  uint32_t fieldnum;
  upb_value val;
  int32_t retval;

  /* Decode tag. */
  CHECK_RETURN(decode_v32(d, &tag));
  wire_type = tag & 0x7;
  fieldnum = tag >> 3;

  /* Lookup tag.  Because of packed/non-packed compatibility, we have to
   * check the wire type against two possibilities. */
  if (fieldnum != DISPATCH_ENDMSG &&
      upb_inttable_lookup32(dispatch, fieldnum, &val)) {
    uint64_t v = upb_value_getuint64(val);
    if (wire_type == (v & 0xff)) {
      d->pc = d->top->base + (v >> 16);
      return DECODE_OK;
    } else if (wire_type == ((v >> 8) & 0xff)) {
      bool found =
          upb_inttable_lookup(dispatch, fieldnum + UPB_MAX_FIELDNUMBER, &val);
      UPB_ASSERT(found);
      d->pc = d->top->base + upb_value_getuint64(val);
      return DECODE_OK;
    }
  }

  /* We have some unknown fields (or ENDGROUP) to parse.  The DISPATCH or TAG
   * bytecode that triggered this is preceded by a CHECKDELIM bytecode which
   * we need to back up to, so that when we're done skipping unknown data we
   * can re-check the delimited end. */
  d->last--;  /* Necessary if we get suspended */
  d->pc = d->last;
  UPB_ASSERT(getop(*d->last) == OP_CHECKDELIM);

  /* Unknown field or ENDGROUP. */
  retval = upb_pbdecoder_skipunknown(d, fieldnum, wire_type);

  CHECK_RETURN(retval);

  if (retval == DECODE_ENDGROUP) {
    goto_endmsg(d);
    return DECODE_OK;
  }

  return DECODE_OK;
}

/* Callers know that the stack is more than one deep because the opcodes that
 * call this only occur after PUSH operations. */
upb_pbdecoder_frame *outer_frame(upb_pbdecoder *d) {
  UPB_ASSERT(d->top != d->stack);
  return d->top - 1;
}


/* The main decoding loop *****************************************************/

/* The main decoder VM function.  Uses traditional bytecode dispatch loop with a
 * switch() statement. */
size_t run_decoder_vm(upb_pbdecoder *d, const mgroup *group,
                      const upb_bufhandle* handle) {

#define VMCASE(op, code) \
  case op: { code; if (consumes_input(op)) checkpoint(d); break; }
#define PRIMITIVE_OP(type, wt, name, convfunc, ctype) \
  VMCASE(OP_PARSE_ ## type, { \
    ctype val; \
    CHECK_RETURN(decode_ ## wt(d, &val)); \
    upb_sink_put ## name(&d->top->sink, arg, (convfunc)(val)); \
  })

  while(1) {
    int32_t instruction;
    opcode op;
    uint32_t arg;
    int32_t longofs;

    d->last = d->pc;
    instruction = *d->pc++;
    op = getop(instruction);
    arg = instruction >> 8;
    longofs = arg;
    UPB_ASSERT(d->ptr != d->residual_end);
    UPB_UNUSED(group);
#ifdef UPB_DUMP_BYTECODE
    fprintf(stderr, "s_ofs=%d buf_ofs=%d data_rem=%d buf_rem=%d delim_rem=%d "
                    "%x %s (%d)\n",
            (int)offset(d),
            (int)(d->ptr - d->buf),
            (int)(d->data_end - d->ptr),
            (int)(d->end - d->ptr),
            (int)((d->top->end_ofs - d->bufstart_ofs) - (d->ptr - d->buf)),
            (int)(d->pc - 1 - group->bytecode),
            upb_pbdecoder_getopname(op),
            arg);
#endif
    switch (op) {
      /* Technically, we are losing data if we see a 32-bit varint that is not
       * properly sign-extended.  We could detect this and error about the data
       * loss, but proto2 does not do this, so we pass. */
      PRIMITIVE_OP(INT32,    varint,  int32,  int32_t,      uint64_t)
      PRIMITIVE_OP(INT64,    varint,  int64,  int64_t,      uint64_t)
      PRIMITIVE_OP(UINT32,   varint,  uint32, uint32_t,     uint64_t)
      PRIMITIVE_OP(UINT64,   varint,  uint64, uint64_t,     uint64_t)
      PRIMITIVE_OP(FIXED32,  fixed32, uint32, uint32_t,     uint32_t)
      PRIMITIVE_OP(FIXED64,  fixed64, uint64, uint64_t,     uint64_t)
      PRIMITIVE_OP(SFIXED32, fixed32, int32,  int32_t,      uint32_t)
      PRIMITIVE_OP(SFIXED64, fixed64, int64,  int64_t,      uint64_t)
      PRIMITIVE_OP(BOOL,     varint,  bool,   bool,         uint64_t)
      PRIMITIVE_OP(DOUBLE,   fixed64, double, as_double,    uint64_t)
      PRIMITIVE_OP(FLOAT,    fixed32, float,  as_float,     uint32_t)
      PRIMITIVE_OP(SINT32,   varint,  int32,  upb_zzdec_32, uint64_t)
      PRIMITIVE_OP(SINT64,   varint,  int64,  upb_zzdec_64, uint64_t)

      VMCASE(OP_SETDISPATCH,
        d->top->base = d->pc - 1;
        memcpy(&d->top->dispatch, d->pc, sizeof(void*));
        d->pc += sizeof(void*) / sizeof(uint32_t);
      )
      VMCASE(OP_STARTMSG,
        CHECK_SUSPEND(upb_sink_startmsg(&d->top->sink));
      )
      VMCASE(OP_ENDMSG,
        CHECK_SUSPEND(upb_sink_endmsg(&d->top->sink, d->status));
      )
      VMCASE(OP_STARTSEQ,
        upb_pbdecoder_frame *outer = outer_frame(d);
        CHECK_SUSPEND(upb_sink_startseq(&outer->sink, arg, &d->top->sink));
      )
      VMCASE(OP_ENDSEQ,
        CHECK_SUSPEND(upb_sink_endseq(&d->top->sink, arg));
      )
      VMCASE(OP_STARTSUBMSG,
        upb_pbdecoder_frame *outer = outer_frame(d);
        CHECK_SUSPEND(upb_sink_startsubmsg(&outer->sink, arg, &d->top->sink));
      )
      VMCASE(OP_ENDSUBMSG,
        CHECK_SUSPEND(upb_sink_endsubmsg(&d->top->sink, arg));
      )
      VMCASE(OP_STARTSTR,
        uint32_t len = delim_remaining(d);
        upb_pbdecoder_frame *outer = outer_frame(d);
        CHECK_SUSPEND(upb_sink_startstr(&outer->sink, arg, len, &d->top->sink));
        if (len == 0) {
          d->pc++;  /* Skip OP_STRING. */
        }
      )
      VMCASE(OP_STRING,
        uint32_t len = curbufleft(d);
        size_t n = upb_sink_putstring(&d->top->sink, arg, d->ptr, len, handle);
        if (n > len) {
          if (n > delim_remaining(d)) {
            seterr(d, "Tried to skip past end of string.");
            return upb_pbdecoder_suspend(d);
          } else {
            int32_t ret = skip(d, n);
            /* This shouldn't return DECODE_OK, because n > len. */
            UPB_ASSERT(ret >= 0);
            return ret;
          }
        }
        advance(d, n);
        if (n < len || d->delim_end == NULL) {
          /* We aren't finished with this string yet. */
          d->pc--;  /* Repeat OP_STRING. */
          if (n > 0) checkpoint(d);
          return upb_pbdecoder_suspend(d);
        }
      )
      VMCASE(OP_ENDSTR,
        CHECK_SUSPEND(upb_sink_endstr(&d->top->sink, arg));
      )
      VMCASE(OP_PUSHTAGDELIM,
        CHECK_SUSPEND(pushtagdelim(d, arg));
      )
      VMCASE(OP_SETBIGGROUPNUM,
        d->top->groupnum = *d->pc++;
      )
      VMCASE(OP_POP,
        UPB_ASSERT(d->top > d->stack);
        decoder_pop(d);
      )
      VMCASE(OP_PUSHLENDELIM,
        uint32_t len;
        CHECK_RETURN(decode_v32(d, &len));
        CHECK_SUSPEND(decoder_push(d, offset(d) + len));
        set_delim_end(d);
      )
      VMCASE(OP_SETDELIM,
        set_delim_end(d);
      )
      VMCASE(OP_CHECKDELIM,
        /* We are guaranteed of this assert because we never allow ourselves to
         * consume bytes beyond data_end, which covers delim_end when non-NULL.
         */
        UPB_ASSERT(!(d->delim_end && d->ptr > d->delim_end));
        if (d->ptr == d->delim_end)
          d->pc += longofs;
      )
      VMCASE(OP_CALL,
        d->callstack[d->call_len++] = d->pc;
        d->pc += longofs;
      )
      VMCASE(OP_RET,
        UPB_ASSERT(d->call_len > 0);
        d->pc = d->callstack[--d->call_len];
      )
      VMCASE(OP_BRANCH,
        d->pc += longofs;
      )
      VMCASE(OP_TAG1,
        uint8_t expected;
        CHECK_SUSPEND(curbufleft(d) > 0);
        expected = (arg >> 8) & 0xff;
        if (*d->ptr == expected) {
          advance(d, 1);
        } else {
          int8_t shortofs;
         badtag:
          shortofs = arg;
          if (shortofs == LABEL_DISPATCH) {
            CHECK_RETURN(dispatch(d));
          } else {
            d->pc += shortofs;
            break; /* Avoid checkpoint(). */
          }
        }
      )
      VMCASE(OP_TAG2,
        uint16_t expected;
        CHECK_SUSPEND(curbufleft(d) > 0);
        expected = (arg >> 8) & 0xffff;
        if (curbufleft(d) >= 2) {
          uint16_t actual;
          memcpy(&actual, d->ptr, 2);
          if (expected == actual) {
            advance(d, 2);
          } else {
            goto badtag;
          }
        } else {
          int32_t result = upb_pbdecoder_checktag_slow(d, expected);
          if (result == DECODE_MISMATCH) goto badtag;
          if (result >= 0) return result;
        }
      )
      VMCASE(OP_TAGN, {
        uint64_t expected;
        int32_t result;
        memcpy(&expected, d->pc, 8);
        d->pc += 2;
        result = upb_pbdecoder_checktag_slow(d, expected);
        if (result == DECODE_MISMATCH) goto badtag;
        if (result >= 0) return result;
      })
      VMCASE(OP_DISPATCH, {
        CHECK_RETURN(dispatch(d));
      })
      VMCASE(OP_HALT, {
        return d->size_param;
      })
    }
  }
}


/* BytesHandler handlers ******************************************************/

void *upb_pbdecoder_startbc(void *closure, const void *pc, size_t size_hint) {
  upb_pbdecoder *d = closure;
  UPB_UNUSED(size_hint);
  d->top->end_ofs = UINT64_MAX;
  d->bufstart_ofs = 0;
  d->call_len = 1;
  d->callstack[0] = &halt;
  d->pc = pc;
  d->skip = 0;
  return d;
}

void *upb_pbdecoder_startjit(void *closure, const void *hd, size_t size_hint) {
  upb_pbdecoder *d = closure;
  UPB_UNUSED(hd);
  UPB_UNUSED(size_hint);
  d->top->end_ofs = UINT64_MAX;
  d->bufstart_ofs = 0;
  d->call_len = 0;
  d->skip = 0;
  return d;
}

bool upb_pbdecoder_end(void *closure, const void *handler_data) {
  upb_pbdecoder *d = closure;
  const upb_pbdecodermethod *method = handler_data;
  uint64_t end;
  char dummy;

  if (d->residual_end > d->residual) {
    seterr(d, "Unexpected EOF: decoder still has buffered unparsed data");
    return false;
  }

  if (d->skip) {
    seterr(d, "Unexpected EOF inside skipped data");
    return false;
  }

  if (d->top->end_ofs != UINT64_MAX) {
    seterr(d, "Unexpected EOF inside delimited string");
    return false;
  }

  /* The user's end() call indicates that the message ends here. */
  end = offset(d);
  d->top->end_ofs = end;

#ifdef UPB_USE_JIT_X64
  if (method->is_native_) {
    const mgroup *group = (const mgroup*)method->group;
    if (d->top != d->stack)
      d->stack->end_ofs = 0;
    group->jit_code(closure, method->code_base.ptr, &dummy, 0, NULL);
  } else
#endif
  {
    const uint32_t *p = d->pc;
    d->stack->end_ofs = end;
    /* Check the previous bytecode, but guard against beginning. */
    if (p != method->code_base.ptr) p--;
    if (getop(*p) == OP_CHECKDELIM) {
      /* Rewind from OP_TAG* to OP_CHECKDELIM. */
      UPB_ASSERT(getop(*d->pc) == OP_TAG1 ||
             getop(*d->pc) == OP_TAG2 ||
             getop(*d->pc) == OP_TAGN ||
             getop(*d->pc) == OP_DISPATCH);
      d->pc = p;
    }
    upb_pbdecoder_decode(closure, handler_data, &dummy, 0, NULL);
  }

  if (d->call_len != 0) {
    seterr(d, "Unexpected EOF inside submessage or group");
    return false;
  }

  return true;
}

size_t upb_pbdecoder_decode(void *decoder, const void *group, const char *buf,
                            size_t size, const upb_bufhandle *handle) {
  int32_t result = upb_pbdecoder_resume(decoder, NULL, buf, size, handle);

  if (result == DECODE_ENDGROUP) goto_endmsg(decoder);
  CHECK_RETURN(result);

  return run_decoder_vm(decoder, group, handle);
}


/* Public API *****************************************************************/

void upb_pbdecoder_reset(upb_pbdecoder *d) {
  d->top = d->stack;
  d->top->groupnum = 0;
  d->ptr = d->residual;
  d->buf = d->residual;
  d->end = d->residual;
  d->residual_end = d->residual;
}

upb_pbdecoder *upb_pbdecoder_create(upb_env *e, const upb_pbdecodermethod *m,
                                    upb_sink *sink) {
  const size_t default_max_nesting = 64;
#ifndef NDEBUG
  size_t size_before = upb_env_bytesallocated(e);
#endif

  upb_pbdecoder *d = upb_env_malloc(e, sizeof(upb_pbdecoder));
  if (!d) return NULL;

  d->method_ = m;
  d->callstack = upb_env_malloc(e, callstacksize(d, default_max_nesting));
  d->stack = upb_env_malloc(e, stacksize(d, default_max_nesting));
  if (!d->stack || !d->callstack) {
    return NULL;
  }

  d->env = e;
  d->limit = d->stack + default_max_nesting - 1;
  d->stack_size = default_max_nesting;
  d->status = NULL;

  upb_pbdecoder_reset(d);
  upb_bytessink_reset(&d->input_, &m->input_handler_, d);

  UPB_ASSERT(sink);
  if (d->method_->dest_handlers_) {
    if (sink->handlers != d->method_->dest_handlers_)
      return NULL;
  }
  upb_sink_reset(&d->top->sink, sink->handlers, sink->closure);

  /* If this fails, increase the value in decoder.h. */
  UPB_ASSERT_DEBUGVAR(upb_env_bytesallocated(e) - size_before <=
                      UPB_PB_DECODER_SIZE);
  return d;
}

uint64_t upb_pbdecoder_bytesparsed(const upb_pbdecoder *d) {
  return offset(d);
}

const upb_pbdecodermethod *upb_pbdecoder_method(const upb_pbdecoder *d) {
  return d->method_;
}

upb_bytessink *upb_pbdecoder_input(upb_pbdecoder *d) {
  return &d->input_;
}

size_t upb_pbdecoder_maxnesting(const upb_pbdecoder *d) {
  return d->stack_size;
}

bool upb_pbdecoder_setmaxnesting(upb_pbdecoder *d, size_t max) {
  UPB_ASSERT(d->top >= d->stack);

  if (max < (size_t)(d->top - d->stack)) {
    /* Can't set a limit smaller than what we are currently at. */
    return false;
  }

  if (max > d->stack_size) {
    /* Need to reallocate stack and callstack to accommodate. */
    size_t old_size = stacksize(d, d->stack_size);
    size_t new_size = stacksize(d, max);
    void *p = upb_env_realloc(d->env, d->stack, old_size, new_size);
    if (!p) {
      return false;
    }
    d->stack = p;

    old_size = callstacksize(d, d->stack_size);
    new_size = callstacksize(d, max);
    p = upb_env_realloc(d->env, d->callstack, old_size, new_size);
    if (!p) {
      return false;
    }
    d->callstack = p;

    d->stack_size = max;
  }

  d->limit = d->stack + max - 1;
  return true;
}
/*
** upb::Encoder
**
** Since we are implementing pure handlers (ie. without any out-of-band access
** to pre-computed lengths), we have to buffer all submessages before we can
** emit even their first byte.
**
** Not knowing the size of submessages also means we can't write a perfect
** zero-copy implementation, even with buffering.  Lengths are stored as
** varints, which means that we don't know how many bytes to reserve for the
** length until we know what the length is.
**
** This leaves us with three main choices:
**
** 1. buffer all submessage data in a temporary buffer, then copy it exactly
**    once into the output buffer.
**
** 2. attempt to buffer data directly into the output buffer, estimating how
**    many bytes each length will take.  When our guesses are wrong, use
**    memmove() to grow or shrink the allotted space.
**
** 3. buffer directly into the output buffer, allocating a max length
**    ahead-of-time for each submessage length.  If we overallocated, we waste
**    space, but no memcpy() or memmove() is required.  This approach requires
**    defining a maximum size for submessages and rejecting submessages that
**    exceed that size.
**
** (2) and (3) have the potential to have better performance, but they are more
** complicated and subtle to implement:
**
**   (3) requires making an arbitrary choice of the maximum message size; it
**       wastes space when submessages are shorter than this and fails
**       completely when they are longer.  This makes it more finicky and
**       requires configuration based on the input.  It also makes it impossible
**       to perfectly match the output of reference encoders that always use the
**       optimal amount of space for each length.
**
**   (2) requires guessing the the size upfront, and if multiple lengths are
**       guessed wrong the minimum required number of memmove() operations may
**       be complicated to compute correctly.  Implemented properly, it may have
**       a useful amortized or average cost, but more investigation is required
**       to determine this and what the optimal algorithm is to achieve it.
**
**   (1) makes you always pay for exactly one copy, but its implementation is
**       the simplest and its performance is predictable.
**
** So for now, we implement (1) only.  If we wish to optimize later, we should
** be able to do it without affecting users.
**
** The strategy is to buffer the segments of data that do *not* depend on
** unknown lengths in one buffer, and keep a separate buffer of segment pointers
** and lengths.  When the top-level submessage ends, we can go beginning to end,
** alternating the writing of lengths with memcpy() of the rest of the data.
** At the top level though, no buffering is required.
*/



/* The output buffer is divided into segments; a segment is a string of data
 * that is "ready to go" -- it does not need any varint lengths inserted into
 * the middle.  The seams between segments are where varints will be inserted
 * once they are known.
 *
 * We also use the concept of a "run", which is a range of encoded bytes that
 * occur at a single submessage level.  Every segment contains one or more runs.
 *
 * A segment can span messages.  Consider:
 *
 *                  .--Submessage lengths---------.
 *                  |       |                     |
 *                  |       V                     V
 *                  V      | |---------------    | |-----------------
 * Submessages:    | |-----------------------------------------------
 * Top-level msg: ------------------------------------------------------------
 *
 * Segments:          -----   -------------------   -----------------
 * Runs:              *----   *--------------*---   *----------------
 * (* marks the start)
 *
 * Note that the top-level menssage is not in any segment because it does not
 * have any length preceding it.
 *
 * A segment is only interrupted when another length needs to be inserted.  So
 * observe how the second segment spans both the inner submessage and part of
 * the next enclosing message. */
typedef struct {
  uint32_t msglen;  /* The length to varint-encode before this segment. */
  uint32_t seglen;  /* Length of the segment. */
} upb_pb_encoder_segment;

struct upb_pb_encoder {
  upb_env *env;

  /* Our input and output. */
  upb_sink input_;
  upb_bytessink *output_;

  /* The "subclosure" -- used as the inner closure as part of the bytessink
   * protocol. */
  void *subc;

  /* The output buffer and limit, and our current write position.  "buf"
   * initially points to "initbuf", but is dynamically allocated if we need to
   * grow beyond the initial size. */
  char *buf, *ptr, *limit;

  /* The beginning of the current run, or undefined if we are at the top
   * level. */
  char *runbegin;

  /* The list of segments we are accumulating. */
  upb_pb_encoder_segment *segbuf, *segptr, *seglimit;

  /* The stack of enclosing submessages.  Each entry in the stack points to the
   * segment where this submessage's length is being accumulated. */
  int *stack, *top, *stacklimit;

  /* Depth of startmsg/endmsg calls. */
  int depth;
};

/* low-level buffering ********************************************************/

/* Low-level functions for interacting with the output buffer. */

/* TODO(haberman): handle pushback */
static void putbuf(upb_pb_encoder *e, const char *buf, size_t len) {
  size_t n = upb_bytessink_putbuf(e->output_, e->subc, buf, len, NULL);
  UPB_ASSERT(n == len);
}

static upb_pb_encoder_segment *top(upb_pb_encoder *e) {
  return &e->segbuf[*e->top];
}

/* Call to ensure that at least "bytes" bytes are available for writing at
 * e->ptr.  Returns false if the bytes could not be allocated. */
static bool reserve(upb_pb_encoder *e, size_t bytes) {
  if ((size_t)(e->limit - e->ptr) < bytes) {
    /* Grow buffer. */
    char *new_buf;
    size_t needed = bytes + (e->ptr - e->buf);
    size_t old_size = e->limit - e->buf;

    size_t new_size = old_size;

    while (new_size < needed) {
      new_size *= 2;
    }

    new_buf = upb_env_realloc(e->env, e->buf, old_size, new_size);

    if (new_buf == NULL) {
      return false;
    }

    e->ptr = new_buf + (e->ptr - e->buf);
    e->runbegin = new_buf + (e->runbegin - e->buf);
    e->limit = new_buf + new_size;
    e->buf = new_buf;
  }

  return true;
}

/* Call when "bytes" bytes have been writte at e->ptr.  The caller *must* have
 * previously called reserve() with at least this many bytes. */
static void encoder_advance(upb_pb_encoder *e, size_t bytes) {
  UPB_ASSERT((size_t)(e->limit - e->ptr) >= bytes);
  e->ptr += bytes;
}

/* Call when all of the bytes for a handler have been written.  Flushes the
 * bytes if possible and necessary, returning false if this failed. */
static bool commit(upb_pb_encoder *e) {
  if (!e->top) {
    /* We aren't inside a delimited region.  Flush our accumulated bytes to
     * the output.
     *
     * TODO(haberman): in the future we may want to delay flushing for
     * efficiency reasons. */
    putbuf(e, e->buf, e->ptr - e->buf);
    e->ptr = e->buf;
  }

  return true;
}

/* Writes the given bytes to the buffer, handling reserve/advance. */
static bool encode_bytes(upb_pb_encoder *e, const void *data, size_t len) {
  if (!reserve(e, len)) {
    return false;
  }

  memcpy(e->ptr, data, len);
  encoder_advance(e, len);
  return true;
}

/* Finish the current run by adding the run totals to the segment and message
 * length. */
static void accumulate(upb_pb_encoder *e) {
  size_t run_len;
  UPB_ASSERT(e->ptr >= e->runbegin);
  run_len = e->ptr - e->runbegin;
  e->segptr->seglen += run_len;
  top(e)->msglen += run_len;
  e->runbegin = e->ptr;
}

/* Call to indicate the start of delimited region for which the full length is
 * not yet known.  All data will be buffered until the length is known.
 * Delimited regions may be nested; their lengths will all be tracked properly. */
static bool start_delim(upb_pb_encoder *e) {
  if (e->top) {
    /* We are already buffering, advance to the next segment and push it on the
     * stack. */
    accumulate(e);

    if (++e->top == e->stacklimit) {
      /* TODO(haberman): grow stack? */
      return false;
    }

    if (++e->segptr == e->seglimit) {
      /* Grow segment buffer. */
      size_t old_size =
          (e->seglimit - e->segbuf) * sizeof(upb_pb_encoder_segment);
      size_t new_size = old_size * 2;
      upb_pb_encoder_segment *new_buf =
          upb_env_realloc(e->env, e->segbuf, old_size, new_size);

      if (new_buf == NULL) {
        return false;
      }

      e->segptr = new_buf + (e->segptr - e->segbuf);
      e->seglimit = new_buf + (new_size / sizeof(upb_pb_encoder_segment));
      e->segbuf = new_buf;
    }
  } else {
    /* We were previously at the top level, start buffering. */
    e->segptr = e->segbuf;
    e->top = e->stack;
    e->runbegin = e->ptr;
  }

  *e->top = e->segptr - e->segbuf;
  e->segptr->seglen = 0;
  e->segptr->msglen = 0;

  return true;
}

/* Call to indicate the end of a delimited region.  We now know the length of
 * the delimited region.  If we are not nested inside any other delimited
 * regions, we can now emit all of the buffered data we accumulated. */
static bool end_delim(upb_pb_encoder *e) {
  size_t msglen;
  accumulate(e);
  msglen = top(e)->msglen;

  if (e->top == e->stack) {
    /* All lengths are now available, emit all buffered data. */
    char buf[UPB_PB_VARINT_MAX_LEN];
    upb_pb_encoder_segment *s;
    const char *ptr = e->buf;
    for (s = e->segbuf; s <= e->segptr; s++) {
      size_t lenbytes = upb_vencode64(s->msglen, buf);
      putbuf(e, buf, lenbytes);
      putbuf(e, ptr, s->seglen);
      ptr += s->seglen;
    }

    e->ptr = e->buf;
    e->top = NULL;
  } else {
    /* Need to keep buffering; propagate length info into enclosing
     * submessages. */
    --e->top;
    top(e)->msglen += msglen + upb_varint_size(msglen);
  }

  return true;
}


/* tag_t **********************************************************************/

/* A precomputed (pre-encoded) tag and length. */

typedef struct {
  uint8_t bytes;
  char tag[7];
} tag_t;

/* Allocates a new tag for this field, and sets it in these handlerattr. */
static void new_tag(upb_handlers *h, const upb_fielddef *f, upb_wiretype_t wt,
                    upb_handlerattr *attr) {
  uint32_t n = upb_fielddef_number(f);

  tag_t *tag = upb_gmalloc(sizeof(tag_t));
  tag->bytes = upb_vencode64((n << 3) | wt, tag->tag);

  upb_handlerattr_init(attr);
  upb_handlerattr_sethandlerdata(attr, tag);
  upb_handlers_addcleanup(h, tag, upb_gfree);
}

static bool encode_tag(upb_pb_encoder *e, const tag_t *tag) {
  return encode_bytes(e, tag->tag, tag->bytes);
}


/* encoding of wire types *****************************************************/

static bool encode_fixed64(upb_pb_encoder *e, uint64_t val) {
  /* TODO(haberman): byte-swap for big endian. */
  return encode_bytes(e, &val, sizeof(uint64_t));
}

static bool encode_fixed32(upb_pb_encoder *e, uint32_t val) {
  /* TODO(haberman): byte-swap for big endian. */
  return encode_bytes(e, &val, sizeof(uint32_t));
}

static bool encode_varint(upb_pb_encoder *e, uint64_t val) {
  if (!reserve(e, UPB_PB_VARINT_MAX_LEN)) {
    return false;
  }

  encoder_advance(e, upb_vencode64(val, e->ptr));
  return true;
}

static uint64_t dbl2uint64(double d) {
  uint64_t ret;
  memcpy(&ret, &d, sizeof(uint64_t));
  return ret;
}

static uint32_t flt2uint32(float d) {
  uint32_t ret;
  memcpy(&ret, &d, sizeof(uint32_t));
  return ret;
}


/* encoding of proto types ****************************************************/

static bool startmsg(void *c, const void *hd) {
  upb_pb_encoder *e = c;
  UPB_UNUSED(hd);
  if (e->depth++ == 0) {
    upb_bytessink_start(e->output_, 0, &e->subc);
  }
  return true;
}

static bool endmsg(void *c, const void *hd, upb_status *status) {
  upb_pb_encoder *e = c;
  UPB_UNUSED(hd);
  UPB_UNUSED(status);
  if (--e->depth == 0) {
    upb_bytessink_end(e->output_);
  }
  return true;
}

static void *encode_startdelimfield(void *c, const void *hd) {
  bool ok = encode_tag(c, hd) && commit(c) && start_delim(c);
  return ok ? c : UPB_BREAK;
}

static bool encode_unknown(void *c, const void *hd, const char *buf,
                           size_t len) {
  UPB_UNUSED(hd);
  return encode_bytes(c, buf, len) && commit(c);
}

static bool encode_enddelimfield(void *c, const void *hd) {
  UPB_UNUSED(hd);
  return end_delim(c);
}

static void *encode_startgroup(void *c, const void *hd) {
  return (encode_tag(c, hd) && commit(c)) ? c : UPB_BREAK;
}

static bool encode_endgroup(void *c, const void *hd) {
  return encode_tag(c, hd) && commit(c);
}

static void *encode_startstr(void *c, const void *hd, size_t size_hint) {
  UPB_UNUSED(size_hint);
  return encode_startdelimfield(c, hd);
}

static size_t encode_strbuf(void *c, const void *hd, const char *buf,
                            size_t len, const upb_bufhandle *h) {
  UPB_UNUSED(hd);
  UPB_UNUSED(h);
  return encode_bytes(c, buf, len) ? len : 0;
}

#define T(type, ctype, convert, encode)                                  \
  static bool encode_scalar_##type(void *e, const void *hd, ctype val) { \
    return encode_tag(e, hd) && encode(e, (convert)(val)) && commit(e);  \
  }                                                                      \
  static bool encode_packed_##type(void *e, const void *hd, ctype val) { \
    UPB_UNUSED(hd);                                                      \
    return encode(e, (convert)(val));                                    \
  }

T(double,   double,   dbl2uint64,   encode_fixed64)
T(float,    float,    flt2uint32,   encode_fixed32)
T(int64,    int64_t,  uint64_t,     encode_varint)
T(int32,    int32_t,  int64_t,      encode_varint)
T(fixed64,  uint64_t, uint64_t,     encode_fixed64)
T(fixed32,  uint32_t, uint32_t,     encode_fixed32)
T(bool,     bool,     bool,         encode_varint)
T(uint32,   uint32_t, uint32_t,     encode_varint)
T(uint64,   uint64_t, uint64_t,     encode_varint)
T(enum,     int32_t,  uint32_t,     encode_varint)
T(sfixed32, int32_t,  uint32_t,     encode_fixed32)
T(sfixed64, int64_t,  uint64_t,     encode_fixed64)
T(sint32,   int32_t,  upb_zzenc_32, encode_varint)
T(sint64,   int64_t,  upb_zzenc_64, encode_varint)

#undef T


/* code to build the handlers *************************************************/

static void newhandlers_callback(const void *closure, upb_handlers *h) {
  const upb_msgdef *m;
  upb_msg_field_iter i;

  UPB_UNUSED(closure);

  upb_handlers_setstartmsg(h, startmsg, NULL);
  upb_handlers_setendmsg(h, endmsg, NULL);
  upb_handlers_setunknown(h, encode_unknown, NULL);

  m = upb_handlers_msgdef(h);
  for(upb_msg_field_begin(&i, m);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    bool packed = upb_fielddef_isseq(f) && upb_fielddef_isprimitive(f) &&
                  upb_fielddef_packed(f);
    upb_handlerattr attr;
    upb_wiretype_t wt =
        packed ? UPB_WIRE_TYPE_DELIMITED
               : upb_pb_native_wire_types[upb_fielddef_descriptortype(f)];

    /* Pre-encode the tag for this field. */
    new_tag(h, f, wt, &attr);

    if (packed) {
      upb_handlers_setstartseq(h, f, encode_startdelimfield, &attr);
      upb_handlers_setendseq(h, f, encode_enddelimfield, &attr);
    }

#define T(upper, lower, upbtype)                                     \
  case UPB_DESCRIPTOR_TYPE_##upper:                                  \
    if (packed) {                                                    \
      upb_handlers_set##upbtype(h, f, encode_packed_##lower, &attr); \
    } else {                                                         \
      upb_handlers_set##upbtype(h, f, encode_scalar_##lower, &attr); \
    }                                                                \
    break;

    switch (upb_fielddef_descriptortype(f)) {
      T(DOUBLE,   double,   double);
      T(FLOAT,    float,    float);
      T(INT64,    int64,    int64);
      T(INT32,    int32,    int32);
      T(FIXED64,  fixed64,  uint64);
      T(FIXED32,  fixed32,  uint32);
      T(BOOL,     bool,     bool);
      T(UINT32,   uint32,   uint32);
      T(UINT64,   uint64,   uint64);
      T(ENUM,     enum,     int32);
      T(SFIXED32, sfixed32, int32);
      T(SFIXED64, sfixed64, int64);
      T(SINT32,   sint32,   int32);
      T(SINT64,   sint64,   int64);
      case UPB_DESCRIPTOR_TYPE_STRING:
      case UPB_DESCRIPTOR_TYPE_BYTES:
        upb_handlers_setstartstr(h, f, encode_startstr, &attr);
        upb_handlers_setendstr(h, f, encode_enddelimfield, &attr);
        upb_handlers_setstring(h, f, encode_strbuf, &attr);
        break;
      case UPB_DESCRIPTOR_TYPE_MESSAGE:
        upb_handlers_setstartsubmsg(h, f, encode_startdelimfield, &attr);
        upb_handlers_setendsubmsg(h, f, encode_enddelimfield, &attr);
        break;
      case UPB_DESCRIPTOR_TYPE_GROUP: {
        /* Endgroup takes a different tag (wire_type = END_GROUP). */
        upb_handlerattr attr2;
        new_tag(h, f, UPB_WIRE_TYPE_END_GROUP, &attr2);

        upb_handlers_setstartsubmsg(h, f, encode_startgroup, &attr);
        upb_handlers_setendsubmsg(h, f, encode_endgroup, &attr2);

        upb_handlerattr_uninit(&attr2);
        break;
      }
    }

#undef T

    upb_handlerattr_uninit(&attr);
  }
}

void upb_pb_encoder_reset(upb_pb_encoder *e) {
  e->segptr = NULL;
  e->top = NULL;
  e->depth = 0;
}


/* public API *****************************************************************/

const upb_handlers *upb_pb_encoder_newhandlers(const upb_msgdef *m,
                                               const void *owner) {
  return upb_handlers_newfrozen(m, owner, newhandlers_callback, NULL);
}

upb_pb_encoder *upb_pb_encoder_create(upb_env *env, const upb_handlers *h,
                                      upb_bytessink *output) {
  const size_t initial_bufsize = 256;
  const size_t initial_segbufsize = 16;
  /* TODO(haberman): make this configurable. */
  const size_t stack_size = 64;
#ifndef NDEBUG
  const size_t size_before = upb_env_bytesallocated(env);
#endif

  upb_pb_encoder *e = upb_env_malloc(env, sizeof(upb_pb_encoder));
  if (!e) return NULL;

  e->buf = upb_env_malloc(env, initial_bufsize);
  e->segbuf = upb_env_malloc(env, initial_segbufsize * sizeof(*e->segbuf));
  e->stack = upb_env_malloc(env, stack_size * sizeof(*e->stack));

  if (!e->buf || !e->segbuf || !e->stack) {
    return NULL;
  }

  e->limit = e->buf + initial_bufsize;
  e->seglimit = e->segbuf + initial_segbufsize;
  e->stacklimit = e->stack + stack_size;

  upb_pb_encoder_reset(e);
  upb_sink_reset(&e->input_, h, e);

  e->env = env;
  e->output_ = output;
  e->subc = output->closure;
  e->ptr = e->buf;

  /* If this fails, increase the value in encoder.h. */
  UPB_ASSERT_DEBUGVAR(upb_env_bytesallocated(env) - size_before <=
                      UPB_PB_ENCODER_SIZE);
  return e;
}

upb_sink *upb_pb_encoder_input(upb_pb_encoder *e) { return &e->input_; }



upb_filedef **upb_loaddescriptor(const char *buf, size_t n, const void *owner,
                                 upb_status *status) {
  /* Create handlers. */
  const upb_pbdecodermethod *decoder_m;
  const upb_handlers *reader_h = upb_descreader_newhandlers(&reader_h);
  upb_env env;
  upb_pbdecodermethodopts opts;
  upb_pbdecoder *decoder;
  upb_descreader *reader;
  bool ok;
  size_t i;
  upb_filedef **ret = NULL;

  upb_pbdecodermethodopts_init(&opts, reader_h);
  decoder_m = upb_pbdecodermethod_new(&opts, &decoder_m);

  upb_env_init(&env);
  upb_env_reporterrorsto(&env, status);

  reader = upb_descreader_create(&env, reader_h);
  decoder = upb_pbdecoder_create(&env, decoder_m, upb_descreader_input(reader));

  /* Push input data. */
  ok = upb_bufsrc_putbuf(buf, n, upb_pbdecoder_input(decoder));

  if (!ok) {
    goto cleanup;
  }

  ret = upb_gmalloc(sizeof (*ret) * (upb_descreader_filecount(reader) + 1));

  if (!ret) {
    goto cleanup;
  }

  for (i = 0; i < upb_descreader_filecount(reader); i++) {
    ret[i] = upb_descreader_file(reader, i);
    upb_filedef_ref(ret[i], owner);
  }

  ret[i] = NULL;

cleanup:
  upb_env_uninit(&env);
  upb_handlers_unref(reader_h, &reader_h);
  upb_pbdecodermethod_unref(decoder_m, &decoder_m);
  return ret;
}
/*
 * upb::pb::TextPrinter
 *
 * OPT: This is not optimized at all.  It uses printf() which parses the format
 * string every time, and it allocates memory for every put.
 */


#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


struct upb_textprinter {
  upb_sink input_;
  upb_bytessink *output_;
  int indent_depth_;
  bool single_line_;
  void *subc;
};

#define CHECK(x) if ((x) < 0) goto err;

static const char *shortname(const char *longname) {
  const char *last = strrchr(longname, '.');
  return last ? last + 1 : longname;
}

static int indent(upb_textprinter *p) {
  int i;
  if (!p->single_line_)
    for (i = 0; i < p->indent_depth_; i++)
      upb_bytessink_putbuf(p->output_, p->subc, "  ", 2, NULL);
  return 0;
}

static int endfield(upb_textprinter *p) {
  const char ch = (p->single_line_ ? ' ' : '\n');
  upb_bytessink_putbuf(p->output_, p->subc, &ch, 1, NULL);
  return 0;
}

static int putescaped(upb_textprinter *p, const char *buf, size_t len,
                      bool preserve_utf8) {
  /* Based on CEscapeInternal() from Google's protobuf release. */
  char dstbuf[4096], *dst = dstbuf, *dstend = dstbuf + sizeof(dstbuf);
  const char *end = buf + len;

  /* I think hex is prettier and more useful, but proto2 uses octal; should
   * investigate whether it can parse hex also. */
  const bool use_hex = false;
  bool last_hex_escape = false; /* true if last output char was \xNN */

  for (; buf < end; buf++) {
    bool is_hex_escape;

    if (dstend - dst < 4) {
      upb_bytessink_putbuf(p->output_, p->subc, dstbuf, dst - dstbuf, NULL);
      dst = dstbuf;
    }

    is_hex_escape = false;
    switch (*buf) {
      case '\n': *(dst++) = '\\'; *(dst++) = 'n';  break;
      case '\r': *(dst++) = '\\'; *(dst++) = 'r';  break;
      case '\t': *(dst++) = '\\'; *(dst++) = 't';  break;
      case '\"': *(dst++) = '\\'; *(dst++) = '\"'; break;
      case '\'': *(dst++) = '\\'; *(dst++) = '\''; break;
      case '\\': *(dst++) = '\\'; *(dst++) = '\\'; break;
      default:
        /* Note that if we emit \xNN and the buf character after that is a hex
         * digit then that digit must be escaped too to prevent it being
         * interpreted as part of the character code by C. */
        if ((!preserve_utf8 || (uint8_t)*buf < 0x80) &&
            (!isprint(*buf) || (last_hex_escape && isxdigit(*buf)))) {
          sprintf(dst, (use_hex ? "\\x%02x" : "\\%03o"), (uint8_t)*buf);
          is_hex_escape = use_hex;
          dst += 4;
        } else {
          *(dst++) = *buf; break;
        }
    }
    last_hex_escape = is_hex_escape;
  }
  /* Flush remaining data. */
  upb_bytessink_putbuf(p->output_, p->subc, dstbuf, dst - dstbuf, NULL);
  return 0;
}

bool putf(upb_textprinter *p, const char *fmt, ...) {
  va_list args;
  va_list args_copy;
  char *str;
  int written;
  int len;
  bool ok;

  va_start(args, fmt);

  /* Run once to get the length of the string. */
  _upb_va_copy(args_copy, args);
  len = _upb_vsnprintf(NULL, 0, fmt, args_copy);
  va_end(args_copy);

  /* + 1 for NULL terminator (vsprintf() requires it even if we don't). */
  str = upb_gmalloc(len + 1);
  if (!str) return false;
  written = vsprintf(str, fmt, args);
  va_end(args);
  UPB_ASSERT(written == len);

  ok = upb_bytessink_putbuf(p->output_, p->subc, str, len, NULL);
  upb_gfree(str);
  return ok;
}


/* handlers *******************************************************************/

static bool textprinter_startmsg(void *c, const void *hd) {
  upb_textprinter *p = c;
  UPB_UNUSED(hd);
  if (p->indent_depth_ == 0) {
    upb_bytessink_start(p->output_, 0, &p->subc);
  }
  return true;
}

static bool textprinter_endmsg(void *c, const void *hd, upb_status *s) {
  upb_textprinter *p = c;
  UPB_UNUSED(hd);
  UPB_UNUSED(s);
  if (p->indent_depth_ == 0) {
    upb_bytessink_end(p->output_);
  }
  return true;
}

#define TYPE(name, ctype, fmt) \
  static bool textprinter_put ## name(void *closure, const void *handler_data, \
                                      ctype val) {                             \
    upb_textprinter *p = closure;                                              \
    const upb_fielddef *f = handler_data;                                      \
    CHECK(indent(p));                                                          \
    putf(p, "%s: " fmt, upb_fielddef_name(f), val);                            \
    CHECK(endfield(p));                                                        \
    return true;                                                               \
  err:                                                                         \
    return false;                                                              \
}

static bool textprinter_putbool(void *closure, const void *handler_data,
                                bool val) {
  upb_textprinter *p = closure;
  const upb_fielddef *f = handler_data;
  CHECK(indent(p));
  putf(p, "%s: %s", upb_fielddef_name(f), val ? "true" : "false");
  CHECK(endfield(p));
  return true;
err:
  return false;
}

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY_MACROVAL(x) STRINGIFY_HELPER(x)

TYPE(int32,  int32_t,  "%" PRId32)
TYPE(int64,  int64_t,  "%" PRId64)
TYPE(uint32, uint32_t, "%" PRIu32)
TYPE(uint64, uint64_t, "%" PRIu64)
TYPE(float,  float,    "%." STRINGIFY_MACROVAL(FLT_DIG) "g")
TYPE(double, double,   "%." STRINGIFY_MACROVAL(DBL_DIG) "g")

#undef TYPE

/* Output a symbolic value from the enum if found, else just print as int32. */
static bool textprinter_putenum(void *closure, const void *handler_data,
                                int32_t val) {
  upb_textprinter *p = closure;
  const upb_fielddef *f = handler_data;
  const upb_enumdef *enum_def = upb_downcast_enumdef(upb_fielddef_subdef(f));
  const char *label = upb_enumdef_iton(enum_def, val);
  if (label) {
    indent(p);
    putf(p, "%s: %s", upb_fielddef_name(f), label);
    endfield(p);
  } else {
    if (!textprinter_putint32(closure, handler_data, val))
      return false;
  }
  return true;
}

static void *textprinter_startstr(void *closure, const void *handler_data,
                      size_t size_hint) {
  upb_textprinter *p = closure;
  const upb_fielddef *f = handler_data;
  UPB_UNUSED(size_hint);
  indent(p);
  putf(p, "%s: \"", upb_fielddef_name(f));
  return p;
}

static bool textprinter_endstr(void *closure, const void *handler_data) {
  upb_textprinter *p = closure;
  UPB_UNUSED(handler_data);
  putf(p, "\"");
  endfield(p);
  return true;
}

static size_t textprinter_putstr(void *closure, const void *hd, const char *buf,
                                 size_t len, const upb_bufhandle *handle) {
  upb_textprinter *p = closure;
  const upb_fielddef *f = hd;
  UPB_UNUSED(handle);
  CHECK(putescaped(p, buf, len, upb_fielddef_type(f) == UPB_TYPE_STRING));
  return len;
err:
  return 0;
}

static void *textprinter_startsubmsg(void *closure, const void *handler_data) {
  upb_textprinter *p = closure;
  const char *name = handler_data;
  CHECK(indent(p));
  putf(p, "%s {%c", name, p->single_line_ ? ' ' : '\n');
  p->indent_depth_++;
  return p;
err:
  return UPB_BREAK;
}

static bool textprinter_endsubmsg(void *closure, const void *handler_data) {
  upb_textprinter *p = closure;
  UPB_UNUSED(handler_data);
  p->indent_depth_--;
  CHECK(indent(p));
  upb_bytessink_putbuf(p->output_, p->subc, "}", 1, NULL);
  CHECK(endfield(p));
  return true;
err:
  return false;
}

static void onmreg(const void *c, upb_handlers *h) {
  const upb_msgdef *m = upb_handlers_msgdef(h);
  upb_msg_field_iter i;
  UPB_UNUSED(c);

  upb_handlers_setstartmsg(h, textprinter_startmsg, NULL);
  upb_handlers_setendmsg(h, textprinter_endmsg, NULL);

  for(upb_msg_field_begin(&i, m);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
    upb_handlerattr_sethandlerdata(&attr, f);
    switch (upb_fielddef_type(f)) {
      case UPB_TYPE_INT32:
        upb_handlers_setint32(h, f, textprinter_putint32, &attr);
        break;
      case UPB_TYPE_INT64:
        upb_handlers_setint64(h, f, textprinter_putint64, &attr);
        break;
      case UPB_TYPE_UINT32:
        upb_handlers_setuint32(h, f, textprinter_putuint32, &attr);
        break;
      case UPB_TYPE_UINT64:
        upb_handlers_setuint64(h, f, textprinter_putuint64, &attr);
        break;
      case UPB_TYPE_FLOAT:
        upb_handlers_setfloat(h, f, textprinter_putfloat, &attr);
        break;
      case UPB_TYPE_DOUBLE:
        upb_handlers_setdouble(h, f, textprinter_putdouble, &attr);
        break;
      case UPB_TYPE_BOOL:
        upb_handlers_setbool(h, f, textprinter_putbool, &attr);
        break;
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES:
        upb_handlers_setstartstr(h, f, textprinter_startstr, &attr);
        upb_handlers_setstring(h, f, textprinter_putstr, &attr);
        upb_handlers_setendstr(h, f, textprinter_endstr, &attr);
        break;
      case UPB_TYPE_MESSAGE: {
        const char *name =
            upb_fielddef_istagdelim(f)
                ? shortname(upb_msgdef_fullname(upb_fielddef_msgsubdef(f)))
                : upb_fielddef_name(f);
        upb_handlerattr_sethandlerdata(&attr, name);
        upb_handlers_setstartsubmsg(h, f, textprinter_startsubmsg, &attr);
        upb_handlers_setendsubmsg(h, f, textprinter_endsubmsg, &attr);
        break;
      }
      case UPB_TYPE_ENUM:
        upb_handlers_setint32(h, f, textprinter_putenum, &attr);
        break;
    }
  }
}

static void textprinter_reset(upb_textprinter *p, bool single_line) {
  p->single_line_ = single_line;
  p->indent_depth_ = 0;
}


/* Public API *****************************************************************/

upb_textprinter *upb_textprinter_create(upb_env *env, const upb_handlers *h,
                                        upb_bytessink *output) {
  upb_textprinter *p = upb_env_malloc(env, sizeof(upb_textprinter));
  if (!p) return NULL;

  p->output_ = output;
  upb_sink_reset(&p->input_, h, p);
  textprinter_reset(p, false);

  return p;
}

const upb_handlers *upb_textprinter_newhandlers(const upb_msgdef *m,
                                                const void *owner) {
  return upb_handlers_newfrozen(m, owner, &onmreg, NULL);
}

upb_sink *upb_textprinter_input(upb_textprinter *p) { return &p->input_; }

void upb_textprinter_setsingleline(upb_textprinter *p, bool single_line) {
  p->single_line_ = single_line;
}


/* Index is descriptor type. */
const uint8_t upb_pb_native_wire_types[] = {
  UPB_WIRE_TYPE_END_GROUP,     /* ENDGROUP */
  UPB_WIRE_TYPE_64BIT,         /* DOUBLE */
  UPB_WIRE_TYPE_32BIT,         /* FLOAT */
  UPB_WIRE_TYPE_VARINT,        /* INT64 */
  UPB_WIRE_TYPE_VARINT,        /* UINT64 */
  UPB_WIRE_TYPE_VARINT,        /* INT32 */
  UPB_WIRE_TYPE_64BIT,         /* FIXED64 */
  UPB_WIRE_TYPE_32BIT,         /* FIXED32 */
  UPB_WIRE_TYPE_VARINT,        /* BOOL */
  UPB_WIRE_TYPE_DELIMITED,     /* STRING */
  UPB_WIRE_TYPE_START_GROUP,   /* GROUP */
  UPB_WIRE_TYPE_DELIMITED,     /* MESSAGE */
  UPB_WIRE_TYPE_DELIMITED,     /* BYTES */
  UPB_WIRE_TYPE_VARINT,        /* UINT32 */
  UPB_WIRE_TYPE_VARINT,        /* ENUM */
  UPB_WIRE_TYPE_32BIT,         /* SFIXED32 */
  UPB_WIRE_TYPE_64BIT,         /* SFIXED64 */
  UPB_WIRE_TYPE_VARINT,        /* SINT32 */
  UPB_WIRE_TYPE_VARINT,        /* SINT64 */
};

/* A basic branch-based decoder, uses 32-bit values to get good performance
 * on 32-bit architectures (but performs well on 64-bits also).
 * This scheme comes from the original Google Protobuf implementation
 * (proto2). */
upb_decoderet upb_vdecode_max8_branch32(upb_decoderet r) {
  upb_decoderet err = {NULL, 0};
  const char *p = r.p;
  uint32_t low = (uint32_t)r.val;
  uint32_t high = 0;
  uint32_t b;
  b = *(p++); low  |= (b & 0x7fU) << 14; if (!(b & 0x80)) goto done;
  b = *(p++); low  |= (b & 0x7fU) << 21; if (!(b & 0x80)) goto done;
  b = *(p++); low  |= (b & 0x7fU) << 28;
              high  = (b & 0x7fU) >>  4; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) <<  3; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) << 10; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) << 17; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) << 24; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) << 31; if (!(b & 0x80)) goto done;
  return err;

done:
  r.val = ((uint64_t)high << 32) | low;
  r.p = p;
  return r;
}

/* Like the previous, but uses 64-bit values. */
upb_decoderet upb_vdecode_max8_branch64(upb_decoderet r) {
  const char *p = r.p;
  uint64_t val = r.val;
  uint64_t b;
  upb_decoderet err = {NULL, 0};
  b = *(p++); val |= (b & 0x7fU) << 14; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 21; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 28; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 35; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 42; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 49; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 56; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 63; if (!(b & 0x80)) goto done;
  return err;

done:
  r.val = val;
  r.p = p;
  return r;
}

#line 1 "upb/json/parser.rl"
/*
** upb::json::Parser (upb_json_parser)
**
** A parser that uses the Ragel State Machine Compiler to generate
** the finite automata.
**
** Ragel only natively handles regular languages, but we can manually
** program it a bit to handle context-free languages like JSON, by using
** the "fcall" and "fret" constructs.
**
** This parser can handle the basics, but needs several things to be fleshed
** out:
**
** - handling of unicode escape sequences (including high surrogate pairs).
** - properly check and report errors for unknown fields, stack overflow,
**   improper array nesting (or lack of nesting).
** - handling of base64 sequences with padding characters.
** - handling of push-back (non-success returns from sink functions).
** - handling of keys/escape-sequences/etc that span input buffers.
*/

#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#define UPB_JSON_MAX_DEPTH 64

typedef struct {
  upb_sink sink;

  /* The current message in which we're parsing, and the field whose value we're
   * expecting next. */
  const upb_msgdef *m;
  const upb_fielddef *f;

  /* The table mapping json name to fielddef for this message. */
  upb_strtable *name_table;

  /* We are in a repeated-field context, ready to emit mapentries as
   * submessages. This flag alters the start-of-object (open-brace) behavior to
   * begin a sequence of mapentry messages rather than a single submessage. */
  bool is_map;

  /* We are in a map-entry message context. This flag is set when parsing the
   * value field of a single map entry and indicates to all value-field parsers
   * (subobjects, strings, numbers, and bools) that the map-entry submessage
   * should end as soon as the value is parsed. */
  bool is_mapentry;

  /* If |is_map| or |is_mapentry| is true, |mapfield| refers to the parent
   * message's map field that we're currently parsing. This differs from |f|
   * because |f| is the field in the *current* message (i.e., the map-entry
   * message itself), not the parent's field that leads to this map. */
  const upb_fielddef *mapfield;
} upb_jsonparser_frame;

struct upb_json_parser {
  upb_env *env;
  const upb_json_parsermethod *method;
  upb_bytessink input_;

  /* Stack to track the JSON scopes we are in. */
  upb_jsonparser_frame stack[UPB_JSON_MAX_DEPTH];
  upb_jsonparser_frame *top;
  upb_jsonparser_frame *limit;

  upb_status status;

  /* Ragel's internal parsing stack for the parsing state machine. */
  int current_state;
  int parser_stack[UPB_JSON_MAX_DEPTH];
  int parser_top;

  /* The handle for the current buffer. */
  const upb_bufhandle *handle;

  /* Accumulate buffer.  See details in parser.rl. */
  const char *accumulated;
  size_t accumulated_len;
  char *accumulate_buf;
  size_t accumulate_buf_size;

  /* Multi-part text data.  See details in parser.rl. */
  int multipart_state;
  upb_selector_t string_selector;

  /* Input capture.  See details in parser.rl. */
  const char *capture;

  /* Intermediate result of parsing a unicode escape sequence. */
  uint32_t digit;
};

struct upb_json_parsermethod {
  upb_refcounted base;

  upb_byteshandler input_handler_;

  /* Mainly for the purposes of refcounting, so all the fielddefs we point
   * to stay alive. */
  const upb_msgdef *msg;

  /* Keys are upb_msgdef*, values are upb_strtable (json_name -> fielddef) */
  upb_inttable name_tables;
};

#define PARSER_CHECK_RETURN(x) if (!(x)) return false

/* Used to signal that a capture has been suspended. */
static char suspend_capture;

static upb_selector_t getsel_for_handlertype(upb_json_parser *p,
                                             upb_handlertype_t type) {
  upb_selector_t sel;
  bool ok = upb_handlers_getselector(p->top->f, type, &sel);
  UPB_ASSERT(ok);
  return sel;
}

static upb_selector_t parser_getsel(upb_json_parser *p) {
  return getsel_for_handlertype(
      p, upb_handlers_getprimitivehandlertype(p->top->f));
}

static bool check_stack(upb_json_parser *p) {
  if ((p->top + 1) == p->limit) {
    upb_status_seterrmsg(&p->status, "Nesting too deep");
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  return true;
}

static void set_name_table(upb_json_parser *p, upb_jsonparser_frame *frame) {
  upb_value v;
  bool ok = upb_inttable_lookupptr(&p->method->name_tables, frame->m, &v);
  UPB_ASSERT(ok);
  frame->name_table = upb_value_getptr(v);
}

/* There are GCC/Clang built-ins for overflow checking which we could start
 * using if there was any performance benefit to it. */

static bool checked_add(size_t a, size_t b, size_t *c) {
  if (SIZE_MAX - a < b) return false;
  *c = a + b;
  return true;
}

static size_t saturating_multiply(size_t a, size_t b) {
  /* size_t is unsigned, so this is defined behavior even on overflow. */
  size_t ret = a * b;
  if (b != 0 && ret / b != a) {
    ret = SIZE_MAX;
  }
  return ret;
}


/* Base64 decoding ************************************************************/

/* TODO(haberman): make this streaming. */

static const signed char b64table[] = {
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      62/*+*/, -1,      -1,      -1,      63/*/ */,
  52/*0*/, 53/*1*/, 54/*2*/, 55/*3*/, 56/*4*/, 57/*5*/, 58/*6*/, 59/*7*/,
  60/*8*/, 61/*9*/, -1,      -1,      -1,      -1,      -1,      -1,
  -1,       0/*A*/,  1/*B*/,  2/*C*/,  3/*D*/,  4/*E*/,  5/*F*/,  6/*G*/,
  07/*H*/,  8/*I*/,  9/*J*/, 10/*K*/, 11/*L*/, 12/*M*/, 13/*N*/, 14/*O*/,
  15/*P*/, 16/*Q*/, 17/*R*/, 18/*S*/, 19/*T*/, 20/*U*/, 21/*V*/, 22/*W*/,
  23/*X*/, 24/*Y*/, 25/*Z*/, -1,      -1,      -1,      -1,      -1,
  -1,      26/*a*/, 27/*b*/, 28/*c*/, 29/*d*/, 30/*e*/, 31/*f*/, 32/*g*/,
  33/*h*/, 34/*i*/, 35/*j*/, 36/*k*/, 37/*l*/, 38/*m*/, 39/*n*/, 40/*o*/,
  41/*p*/, 42/*q*/, 43/*r*/, 44/*s*/, 45/*t*/, 46/*u*/, 47/*v*/, 48/*w*/,
  49/*x*/, 50/*y*/, 51/*z*/, -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1
};

/* Returns the table value sign-extended to 32 bits.  Knowing that the upper
 * bits will be 1 for unrecognized characters makes it easier to check for
 * this error condition later (see below). */
int32_t b64lookup(unsigned char ch) { return b64table[ch]; }

/* Returns true if the given character is not a valid base64 character or
 * padding. */
bool nonbase64(unsigned char ch) { return b64lookup(ch) == -1 && ch != '='; }

static bool base64_push(upb_json_parser *p, upb_selector_t sel, const char *ptr,
                        size_t len) {
  const char *limit = ptr + len;
  for (; ptr < limit; ptr += 4) {
    uint32_t val;
    char output[3];

    if (limit - ptr < 4) {
      upb_status_seterrf(&p->status,
                         "Base64 input for bytes field not a multiple of 4: %s",
                         upb_fielddef_name(p->top->f));
      upb_env_reporterror(p->env, &p->status);
      return false;
    }

    val = b64lookup(ptr[0]) << 18 |
          b64lookup(ptr[1]) << 12 |
          b64lookup(ptr[2]) << 6  |
          b64lookup(ptr[3]);

    /* Test the upper bit; returns true if any of the characters returned -1. */
    if (val & 0x80000000) {
      goto otherchar;
    }

    output[0] = val >> 16;
    output[1] = (val >> 8) & 0xff;
    output[2] = val & 0xff;
    upb_sink_putstring(&p->top->sink, sel, output, 3, NULL);
  }
  return true;

otherchar:
  if (nonbase64(ptr[0]) || nonbase64(ptr[1]) || nonbase64(ptr[2]) ||
      nonbase64(ptr[3]) ) {
    upb_status_seterrf(&p->status,
                       "Non-base64 characters in bytes field: %s",
                       upb_fielddef_name(p->top->f));
    upb_env_reporterror(p->env, &p->status);
    return false;
  } if (ptr[2] == '=') {
    uint32_t val;
    char output;

    /* Last group contains only two input bytes, one output byte. */
    if (ptr[0] == '=' || ptr[1] == '=' || ptr[3] != '=') {
      goto badpadding;
    }

    val = b64lookup(ptr[0]) << 18 |
          b64lookup(ptr[1]) << 12;

    UPB_ASSERT(!(val & 0x80000000));
    output = val >> 16;
    upb_sink_putstring(&p->top->sink, sel, &output, 1, NULL);
    return true;
  } else {
    uint32_t val;
    char output[2];

    /* Last group contains only three input bytes, two output bytes. */
    if (ptr[0] == '=' || ptr[1] == '=' || ptr[2] == '=') {
      goto badpadding;
    }

    val = b64lookup(ptr[0]) << 18 |
          b64lookup(ptr[1]) << 12 |
          b64lookup(ptr[2]) << 6;

    output[0] = val >> 16;
    output[1] = (val >> 8) & 0xff;
    upb_sink_putstring(&p->top->sink, sel, output, 2, NULL);
    return true;
  }

badpadding:
  upb_status_seterrf(&p->status,
                     "Incorrect base64 padding for field: %s (%.*s)",
                     upb_fielddef_name(p->top->f),
                     4, ptr);
  upb_env_reporterror(p->env, &p->status);
  return false;
}


/* Accumulate buffer **********************************************************/

/* Functionality for accumulating a buffer.
 *
 * Some parts of the parser need an entire value as a contiguous string.  For
 * example, to look up a member name in a hash table, or to turn a string into
 * a number, the relevant library routines need the input string to be in
 * contiguous memory, even if the value spanned two or more buffers in the
 * input.  These routines handle that.
 *
 * In the common case we can just point to the input buffer to get this
 * contiguous string and avoid any actual copy.  So we optimistically begin
 * this way.  But there are a few cases where we must instead copy into a
 * separate buffer:
 *
 *   1. The string was not contiguous in the input (it spanned buffers).
 *
 *   2. The string included escape sequences that need to be interpreted to get
 *      the true value in a contiguous buffer. */

static void assert_accumulate_empty(upb_json_parser *p) {
  UPB_ASSERT(p->accumulated == NULL);
  UPB_ASSERT(p->accumulated_len == 0);
}

static void accumulate_clear(upb_json_parser *p) {
  p->accumulated = NULL;
  p->accumulated_len = 0;
}

/* Used internally by accumulate_append(). */
static bool accumulate_realloc(upb_json_parser *p, size_t need) {
  void *mem;
  size_t old_size = p->accumulate_buf_size;
  size_t new_size = UPB_MAX(old_size, 128);
  while (new_size < need) {
    new_size = saturating_multiply(new_size, 2);
  }

  mem = upb_env_realloc(p->env, p->accumulate_buf, old_size, new_size);
  if (!mem) {
    upb_status_seterrmsg(&p->status, "Out of memory allocating buffer.");
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  p->accumulate_buf = mem;
  p->accumulate_buf_size = new_size;
  return true;
}

/* Logically appends the given data to the append buffer.
 * If "can_alias" is true, we will try to avoid actually copying, but the buffer
 * must be valid until the next accumulate_append() call (if any). */
static bool accumulate_append(upb_json_parser *p, const char *buf, size_t len,
                              bool can_alias) {
  size_t need;

  if (!p->accumulated && can_alias) {
    p->accumulated = buf;
    p->accumulated_len = len;
    return true;
  }

  if (!checked_add(p->accumulated_len, len, &need)) {
    upb_status_seterrmsg(&p->status, "Integer overflow.");
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  if (need > p->accumulate_buf_size && !accumulate_realloc(p, need)) {
    return false;
  }

  if (p->accumulated != p->accumulate_buf) {
    memcpy(p->accumulate_buf, p->accumulated, p->accumulated_len);
    p->accumulated = p->accumulate_buf;
  }

  memcpy(p->accumulate_buf + p->accumulated_len, buf, len);
  p->accumulated_len += len;
  return true;
}

/* Returns a pointer to the data accumulated since the last accumulate_clear()
 * call, and writes the length to *len.  This with point either to the input
 * buffer or a temporary accumulate buffer. */
static const char *accumulate_getptr(upb_json_parser *p, size_t *len) {
  UPB_ASSERT(p->accumulated);
  *len = p->accumulated_len;
  return p->accumulated;
}


/* Mult-part text data ********************************************************/

/* When we have text data in the input, it can often come in multiple segments.
 * For example, there may be some raw string data followed by an escape
 * sequence.  The two segments are processed with different logic.  Also buffer
 * seams in the input can cause multiple segments.
 *
 * As we see segments, there are two main cases for how we want to process them:
 *
 *  1. we want to push the captured input directly to string handlers.
 *
 *  2. we need to accumulate all the parts into a contiguous buffer for further
 *     processing (field name lookup, string->number conversion, etc). */

/* This is the set of states for p->multipart_state. */
enum {
  /* We are not currently processing multipart data. */
  MULTIPART_INACTIVE = 0,

  /* We are processing multipart data by accumulating it into a contiguous
   * buffer. */
  MULTIPART_ACCUMULATE = 1,

  /* We are processing multipart data by pushing each part directly to the
   * current string handlers. */
  MULTIPART_PUSHEAGERLY = 2
};

/* Start a multi-part text value where we accumulate the data for processing at
 * the end. */
static void multipart_startaccum(upb_json_parser *p) {
  assert_accumulate_empty(p);
  UPB_ASSERT(p->multipart_state == MULTIPART_INACTIVE);
  p->multipart_state = MULTIPART_ACCUMULATE;
}

/* Start a multi-part text value where we immediately push text data to a string
 * value with the given selector. */
static void multipart_start(upb_json_parser *p, upb_selector_t sel) {
  assert_accumulate_empty(p);
  UPB_ASSERT(p->multipart_state == MULTIPART_INACTIVE);
  p->multipart_state = MULTIPART_PUSHEAGERLY;
  p->string_selector = sel;
}

static bool multipart_text(upb_json_parser *p, const char *buf, size_t len,
                           bool can_alias) {
  switch (p->multipart_state) {
    case MULTIPART_INACTIVE:
      upb_status_seterrmsg(
          &p->status, "Internal error: unexpected state MULTIPART_INACTIVE");
      upb_env_reporterror(p->env, &p->status);
      return false;

    case MULTIPART_ACCUMULATE:
      if (!accumulate_append(p, buf, len, can_alias)) {
        return false;
      }
      break;

    case MULTIPART_PUSHEAGERLY: {
      const upb_bufhandle *handle = can_alias ? p->handle : NULL;
      upb_sink_putstring(&p->top->sink, p->string_selector, buf, len, handle);
      break;
    }
  }

  return true;
}

/* Note: this invalidates the accumulate buffer!  Call only after reading its
 * contents. */
static void multipart_end(upb_json_parser *p) {
  UPB_ASSERT(p->multipart_state != MULTIPART_INACTIVE);
  p->multipart_state = MULTIPART_INACTIVE;
  accumulate_clear(p);
}


/* Input capture **************************************************************/

/* Functionality for capturing a region of the input as text.  Gracefully
 * handles the case where a buffer seam occurs in the middle of the captured
 * region. */

static void capture_begin(upb_json_parser *p, const char *ptr) {
  UPB_ASSERT(p->multipart_state != MULTIPART_INACTIVE);
  UPB_ASSERT(p->capture == NULL);
  p->capture = ptr;
}

static bool capture_end(upb_json_parser *p, const char *ptr) {
  UPB_ASSERT(p->capture);
  if (multipart_text(p, p->capture, ptr - p->capture, true)) {
    p->capture = NULL;
    return true;
  } else {
    return false;
  }
}

/* This is called at the end of each input buffer (ie. when we have hit a
 * buffer seam).  If we are in the middle of capturing the input, this
 * processes the unprocessed capture region. */
static void capture_suspend(upb_json_parser *p, const char **ptr) {
  if (!p->capture) return;

  if (multipart_text(p, p->capture, *ptr - p->capture, false)) {
    /* We use this as a signal that we were in the middle of capturing, and
     * that capturing should resume at the beginning of the next buffer.
     * 
     * We can't use *ptr here, because we have no guarantee that this pointer
     * will be valid when we resume (if the underlying memory is freed, then
     * using the pointer at all, even to compare to NULL, is likely undefined
     * behavior). */
    p->capture = &suspend_capture;
  } else {
    /* Need to back up the pointer to the beginning of the capture, since
     * we were not able to actually preserve it. */
    *ptr = p->capture;
  }
}

static void capture_resume(upb_json_parser *p, const char *ptr) {
  if (p->capture) {
    UPB_ASSERT(p->capture == &suspend_capture);
    p->capture = ptr;
  }
}


/* Callbacks from the parser **************************************************/

/* These are the functions called directly from the parser itself.
 * We define these in the same order as their declarations in the parser. */

static char escape_char(char in) {
  switch (in) {
    case 'r': return '\r';
    case 't': return '\t';
    case 'n': return '\n';
    case 'f': return '\f';
    case 'b': return '\b';
    case '/': return '/';
    case '"': return '"';
    case '\\': return '\\';
    default:
      UPB_ASSERT(0);
      return 'x';
  }
}

static bool escape(upb_json_parser *p, const char *ptr) {
  char ch = escape_char(*ptr);
  return multipart_text(p, &ch, 1, false);
}

static void start_hex(upb_json_parser *p) {
  p->digit = 0;
}

static void hexdigit(upb_json_parser *p, const char *ptr) {
  char ch = *ptr;

  p->digit <<= 4;

  if (ch >= '0' && ch <= '9') {
    p->digit += (ch - '0');
  } else if (ch >= 'a' && ch <= 'f') {
    p->digit += ((ch - 'a') + 10);
  } else {
    UPB_ASSERT(ch >= 'A' && ch <= 'F');
    p->digit += ((ch - 'A') + 10);
  }
}

static bool end_hex(upb_json_parser *p) {
  uint32_t codepoint = p->digit;

  /* emit the codepoint as UTF-8. */
  char utf8[3]; /* support \u0000 -- \uFFFF -- need only three bytes. */
  int length = 0;
  if (codepoint <= 0x7F) {
    utf8[0] = codepoint;
    length = 1;
  } else if (codepoint <= 0x07FF) {
    utf8[1] = (codepoint & 0x3F) | 0x80;
    codepoint >>= 6;
    utf8[0] = (codepoint & 0x1F) | 0xC0;
    length = 2;
  } else /* codepoint <= 0xFFFF */ {
    utf8[2] = (codepoint & 0x3F) | 0x80;
    codepoint >>= 6;
    utf8[1] = (codepoint & 0x3F) | 0x80;
    codepoint >>= 6;
    utf8[0] = (codepoint & 0x0F) | 0xE0;
    length = 3;
  }
  /* TODO(haberman): Handle high surrogates: if codepoint is a high surrogate
   * we have to wait for the next escape to get the full code point). */

  return multipart_text(p, utf8, length, false);
}

static void start_text(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_text(upb_json_parser *p, const char *ptr) {
  return capture_end(p, ptr);
}

static void start_number(upb_json_parser *p, const char *ptr) {
  multipart_startaccum(p);
  capture_begin(p, ptr);
}

static bool parse_number(upb_json_parser *p, bool is_quoted);

static bool end_number(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }

  return parse_number(p, false);
}

/* |buf| is NULL-terminated. |buf| itself will never include quotes;
 * |is_quoted| tells us whether this text originally appeared inside quotes. */
static bool parse_number_from_buffer(upb_json_parser *p, const char *buf,
                                     bool is_quoted) {
  size_t len = strlen(buf);
  const char *bufend = buf + len;
  char *end;
  upb_fieldtype_t type = upb_fielddef_type(p->top->f);
  double val;
  double dummy;
  double inf = 1.0 / 0.0;  /* C89 does not have an INFINITY macro. */

  errno = 0;

  if (len == 0 || buf[0] == ' ') {
    return false;
  }

  /* For integer types, first try parsing with integer-specific routines.
   * If these succeed, they will be more accurate for int64/uint64 than
   * strtod().
   */
  switch (type) {
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32: {
      long val = strtol(buf, &end, 0);
      if (errno == ERANGE || end != bufend) {
        break;
      } else if (val > INT32_MAX || val < INT32_MIN) {
        return false;
      } else {
        upb_sink_putint32(&p->top->sink, parser_getsel(p), val);
        return true;
      }
    }
    case UPB_TYPE_UINT32: {
      unsigned long val = strtoul(buf, &end, 0);
      if (end != bufend) {
        break;
      } else if (val > UINT32_MAX || errno == ERANGE) {
        return false;
      } else {
        upb_sink_putuint32(&p->top->sink, parser_getsel(p), val);
        return true;
      }
    }
    /* XXX: We can't handle [u]int64 properly on 32-bit machines because
     * strto[u]ll isn't in C89. */
    case UPB_TYPE_INT64: {
      long val = strtol(buf, &end, 0);
      if (errno == ERANGE || end != bufend) {
        break;
      } else {
        upb_sink_putint64(&p->top->sink, parser_getsel(p), val);
        return true;
      }
    }
    case UPB_TYPE_UINT64: {
      unsigned long val = strtoul(p->accumulated, &end, 0);
      if (end != bufend) {
        break;
      } else if (errno == ERANGE) {
        return false;
      } else {
        upb_sink_putuint64(&p->top->sink, parser_getsel(p), val);
        return true;
      }
    }
    default:
      break;
  }

  if (type != UPB_TYPE_DOUBLE && type != UPB_TYPE_FLOAT && is_quoted) {
    /* Quoted numbers for integer types are not allowed to be in double form. */
    return false;
  }

  if (len == strlen("Infinity") && strcmp(buf, "Infinity") == 0) {
    /* C89 does not have an INFINITY macro. */
    val = inf;
  } else if (len == strlen("-Infinity") && strcmp(buf, "-Infinity") == 0) {
    val = -inf;
  } else {
    val = strtod(buf, &end);
    if (errno == ERANGE || end != bufend) {
      return false;
    }
  }

  switch (type) {
#define CASE(capitaltype, smalltype, ctype, min, max)                     \
    case UPB_TYPE_ ## capitaltype: {                                      \
      if (modf(val, &dummy) != 0 || val > max || val < min) {             \
        return false;                                                     \
      } else {                                                            \
        upb_sink_put ## smalltype(&p->top->sink, parser_getsel(p),        \
                                  (ctype)val);                            \
        return true;                                                      \
      }                                                                   \
      break;                                                              \
    }
    case UPB_TYPE_ENUM:
    CASE(INT32, int32, int32_t, INT32_MIN, INT32_MAX);
    CASE(INT64, int64, int64_t, INT64_MIN, INT64_MAX);
    CASE(UINT32, uint32, uint32_t, 0, UINT32_MAX);
    CASE(UINT64, uint64, uint64_t, 0, UINT64_MAX);
#undef CASE

    case UPB_TYPE_DOUBLE:
      upb_sink_putdouble(&p->top->sink, parser_getsel(p), val);
      return true;
    case UPB_TYPE_FLOAT:
      if ((val > FLT_MAX || val < -FLT_MAX) && val != inf && val != -inf) {
        return false;
      } else {
        upb_sink_putfloat(&p->top->sink, parser_getsel(p), val);
        return true;
      }
    default:
      return false;
  }
}

static bool parse_number(upb_json_parser *p, bool is_quoted) {
  size_t len;
  const char *buf;

  /* strtol() and friends unfortunately do not support specifying the length of
   * the input string, so we need to force a copy into a NULL-terminated buffer. */
  if (!multipart_text(p, "\0", 1, false)) {
    return false;
  }

  buf = accumulate_getptr(p, &len);

  if (parse_number_from_buffer(p, buf, is_quoted)) {
    multipart_end(p);
    return true;
  } else {
    upb_status_seterrf(&p->status, "error parsing number: %s", buf);
    upb_env_reporterror(p->env, &p->status);
    multipart_end(p);
    return false;
  }
}

static bool parser_putbool(upb_json_parser *p, bool val) {
  bool ok;

  if (upb_fielddef_type(p->top->f) != UPB_TYPE_BOOL) {
    upb_status_seterrf(&p->status,
                       "Boolean value specified for non-bool field: %s",
                       upb_fielddef_name(p->top->f));
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  ok = upb_sink_putbool(&p->top->sink, parser_getsel(p), val);
  UPB_ASSERT(ok);

  return true;
}

static bool start_stringval(upb_json_parser *p) {
  UPB_ASSERT(p->top->f);

  if (upb_fielddef_isstring(p->top->f)) {
    upb_jsonparser_frame *inner;
    upb_selector_t sel;

    if (!check_stack(p)) return false;

    /* Start a new parser frame: parser frames correspond one-to-one with
     * handler frames, and string events occur in a sub-frame. */
    inner = p->top + 1;
    sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
    upb_sink_startstr(&p->top->sink, sel, 0, &inner->sink);
    inner->m = p->top->m;
    inner->f = p->top->f;
    inner->name_table = NULL;
    inner->is_map = false;
    inner->is_mapentry = false;
    p->top = inner;

    if (upb_fielddef_type(p->top->f) == UPB_TYPE_STRING) {
      /* For STRING fields we push data directly to the handlers as it is
       * parsed.  We don't do this yet for BYTES fields, because our base64
       * decoder is not streaming.
       *
       * TODO(haberman): make base64 decoding streaming also. */
      multipart_start(p, getsel_for_handlertype(p, UPB_HANDLER_STRING));
      return true;
    } else {
      multipart_startaccum(p);
      return true;
    }
  } else if (upb_fielddef_type(p->top->f) != UPB_TYPE_BOOL &&
             upb_fielddef_type(p->top->f) != UPB_TYPE_MESSAGE) {
    /* No need to push a frame -- numeric values in quotes remain in the
     * current parser frame.  These values must accmulate so we can convert
     * them all at once at the end. */
    multipart_startaccum(p);
    return true;
  } else {
    upb_status_seterrf(&p->status,
                       "String specified for bool or submessage field: %s",
                       upb_fielddef_name(p->top->f));
    upb_env_reporterror(p->env, &p->status);
    return false;
  }
}

static bool end_stringval(upb_json_parser *p) {
  bool ok = true;

  switch (upb_fielddef_type(p->top->f)) {
    case UPB_TYPE_BYTES:
      if (!base64_push(p, getsel_for_handlertype(p, UPB_HANDLER_STRING),
                       p->accumulated, p->accumulated_len)) {
        return false;
      }
      /* Fall through. */

    case UPB_TYPE_STRING: {
      upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
      p->top--;
      upb_sink_endstr(&p->top->sink, sel);
      break;
    }

    case UPB_TYPE_ENUM: {
      /* Resolve enum symbolic name to integer value. */
      const upb_enumdef *enumdef =
          (const upb_enumdef*)upb_fielddef_subdef(p->top->f);

      size_t len;
      const char *buf = accumulate_getptr(p, &len);

      int32_t int_val = 0;
      ok = upb_enumdef_ntoi(enumdef, buf, len, &int_val);

      if (ok) {
        upb_selector_t sel = parser_getsel(p);
        upb_sink_putint32(&p->top->sink, sel, int_val);
      } else {
        upb_status_seterrf(&p->status, "Enum value unknown: '%.*s'", len, buf);
        upb_env_reporterror(p->env, &p->status);
      }

      break;
    }

    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
    case UPB_TYPE_DOUBLE:
    case UPB_TYPE_FLOAT:
      ok = parse_number(p, true);
      break;

    default:
      UPB_ASSERT(false);
      upb_status_seterrmsg(&p->status, "Internal error in JSON decoder");
      upb_env_reporterror(p->env, &p->status);
      ok = false;
      break;
  }

  multipart_end(p);

  return ok;
}

static void start_member(upb_json_parser *p) {
  UPB_ASSERT(!p->top->f);
  multipart_startaccum(p);
}

/* Helper: invoked during parse_mapentry() to emit the mapentry message's key
 * field based on the current contents of the accumulate buffer. */
static bool parse_mapentry_key(upb_json_parser *p) {

  size_t len;
  const char *buf = accumulate_getptr(p, &len);

  /* Emit the key field. We do a bit of ad-hoc parsing here because the
   * parser state machine has already decided that this is a string field
   * name, and we are reinterpreting it as some arbitrary key type. In
   * particular, integer and bool keys are quoted, so we need to parse the
   * quoted string contents here. */

  p->top->f = upb_msgdef_itof(p->top->m, UPB_MAPENTRY_KEY);
  if (p->top->f == NULL) {
    upb_status_seterrmsg(&p->status, "mapentry message has no key");
    upb_env_reporterror(p->env, &p->status);
    return false;
  }
  switch (upb_fielddef_type(p->top->f)) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
      /* Invoke end_number. The accum buffer has the number's text already. */
      if (!parse_number(p, true)) {
        return false;
      }
      break;
    case UPB_TYPE_BOOL:
      if (len == 4 && !strncmp(buf, "true", 4)) {
        if (!parser_putbool(p, true)) {
          return false;
        }
      } else if (len == 5 && !strncmp(buf, "false", 5)) {
        if (!parser_putbool(p, false)) {
          return false;
        }
      } else {
        upb_status_seterrmsg(&p->status,
                             "Map bool key not 'true' or 'false'");
        upb_env_reporterror(p->env, &p->status);
        return false;
      }
      multipart_end(p);
      break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      upb_sink subsink;
      upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
      upb_sink_startstr(&p->top->sink, sel, len, &subsink);
      sel = getsel_for_handlertype(p, UPB_HANDLER_STRING);
      upb_sink_putstring(&subsink, sel, buf, len, NULL);
      sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
      upb_sink_endstr(&p->top->sink, sel);
      multipart_end(p);
      break;
    }
    default:
      upb_status_seterrmsg(&p->status, "Invalid field type for map key");
      upb_env_reporterror(p->env, &p->status);
      return false;
  }

  return true;
}

/* Helper: emit one map entry (as a submessage in the map field sequence). This
 * is invoked from end_membername(), at the end of the map entry's key string,
 * with the map key in the accumulate buffer. It parses the key from that
 * buffer, emits the handler calls to start the mapentry submessage (setting up
 * its subframe in the process), and sets up state in the subframe so that the
 * value parser (invoked next) will emit the mapentry's value field and then
 * end the mapentry message. */

static bool handle_mapentry(upb_json_parser *p) {
  const upb_fielddef *mapfield;
  const upb_msgdef *mapentrymsg;
  upb_jsonparser_frame *inner;
  upb_selector_t sel;

  /* Map entry: p->top->sink is the seq frame, so we need to start a frame
   * for the mapentry itself, and then set |f| in that frame so that the map
   * value field is parsed, and also set a flag to end the frame after the
   * map-entry value is parsed. */
  if (!check_stack(p)) return false;

  mapfield = p->top->mapfield;
  mapentrymsg = upb_fielddef_msgsubdef(mapfield);

  inner = p->top + 1;
  p->top->f = mapfield;
  sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSUBMSG);
  upb_sink_startsubmsg(&p->top->sink, sel, &inner->sink);
  inner->m = mapentrymsg;
  inner->name_table = NULL;
  inner->mapfield = mapfield;
  inner->is_map = false;

  /* Don't set this to true *yet* -- we reuse parsing handlers below to push
   * the key field value to the sink, and these handlers will pop the frame
   * if they see is_mapentry (when invoked by the parser state machine, they
   * would have just seen the map-entry value, not key). */
  inner->is_mapentry = false;
  p->top = inner;

  /* send STARTMSG in submsg frame. */
  upb_sink_startmsg(&p->top->sink);

  parse_mapentry_key(p);

  /* Set up the value field to receive the map-entry value. */
  p->top->f = upb_msgdef_itof(p->top->m, UPB_MAPENTRY_VALUE);
  p->top->is_mapentry = true;  /* set up to pop frame after value is parsed. */
  p->top->mapfield = mapfield;
  if (p->top->f == NULL) {
    upb_status_seterrmsg(&p->status, "mapentry message has no value");
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  return true;
}

static bool end_membername(upb_json_parser *p) {
  UPB_ASSERT(!p->top->f);

  if (p->top->is_map) {
    return handle_mapentry(p);
  } else {
    size_t len;
    const char *buf = accumulate_getptr(p, &len);
    upb_value v;

    if (upb_strtable_lookup2(p->top->name_table, buf, len, &v)) {
      p->top->f = upb_value_getconstptr(v);
      multipart_end(p);

      return true;
    } else {
      /* TODO(haberman): Ignore unknown fields if requested/configured to do
       * so. */
      upb_status_seterrf(&p->status, "No such field: %.*s\n", (int)len, buf);
      upb_env_reporterror(p->env, &p->status);
      return false;
    }
  }
}

static void end_member(upb_json_parser *p) {
  /* If we just parsed a map-entry value, end that frame too. */
  if (p->top->is_mapentry) {
    upb_status s = UPB_STATUS_INIT;
    upb_selector_t sel;
    bool ok;
    const upb_fielddef *mapfield;

    UPB_ASSERT(p->top > p->stack);
    /* send ENDMSG on submsg. */
    upb_sink_endmsg(&p->top->sink, &s);
    mapfield = p->top->mapfield;

    /* send ENDSUBMSG in repeated-field-of-mapentries frame. */
    p->top--;
    ok = upb_handlers_getselector(mapfield, UPB_HANDLER_ENDSUBMSG, &sel);
    UPB_ASSERT(ok);
    upb_sink_endsubmsg(&p->top->sink, sel);
  }

  p->top->f = NULL;
}

static bool start_subobject(upb_json_parser *p) {
  UPB_ASSERT(p->top->f);

  if (upb_fielddef_ismap(p->top->f)) {
    upb_jsonparser_frame *inner;
    upb_selector_t sel;

    /* Beginning of a map. Start a new parser frame in a repeated-field
     * context. */
    if (!check_stack(p)) return false;

    inner = p->top + 1;
    sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSEQ);
    upb_sink_startseq(&p->top->sink, sel, &inner->sink);
    inner->m = upb_fielddef_msgsubdef(p->top->f);
    inner->name_table = NULL;
    inner->mapfield = p->top->f;
    inner->f = NULL;
    inner->is_map = true;
    inner->is_mapentry = false;
    p->top = inner;

    return true;
  } else if (upb_fielddef_issubmsg(p->top->f)) {
    upb_jsonparser_frame *inner;
    upb_selector_t sel;

    /* Beginning of a subobject. Start a new parser frame in the submsg
     * context. */
    if (!check_stack(p)) return false;

    inner = p->top + 1;

    sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSUBMSG);
    upb_sink_startsubmsg(&p->top->sink, sel, &inner->sink);
    inner->m = upb_fielddef_msgsubdef(p->top->f);
    set_name_table(p, inner);
    inner->f = NULL;
    inner->is_map = false;
    inner->is_mapentry = false;
    p->top = inner;

    return true;
  } else {
    upb_status_seterrf(&p->status,
                       "Object specified for non-message/group field: %s",
                       upb_fielddef_name(p->top->f));
    upb_env_reporterror(p->env, &p->status);
    return false;
  }
}

static void end_subobject(upb_json_parser *p) {
  if (p->top->is_map) {
    upb_selector_t sel;
    p->top--;
    sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSEQ);
    upb_sink_endseq(&p->top->sink, sel);
  } else {
    upb_selector_t sel;
    p->top--;
    sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSUBMSG);
    upb_sink_endsubmsg(&p->top->sink, sel);
  }
}

static bool start_array(upb_json_parser *p) {
  upb_jsonparser_frame *inner;
  upb_selector_t sel;

  UPB_ASSERT(p->top->f);

  if (!upb_fielddef_isseq(p->top->f)) {
    upb_status_seterrf(&p->status,
                       "Array specified for non-repeated field: %s",
                       upb_fielddef_name(p->top->f));
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  if (!check_stack(p)) return false;

  inner = p->top + 1;
  sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSEQ);
  upb_sink_startseq(&p->top->sink, sel, &inner->sink);
  inner->m = p->top->m;
  inner->name_table = NULL;
  inner->f = p->top->f;
  inner->is_map = false;
  inner->is_mapentry = false;
  p->top = inner;

  return true;
}

static void end_array(upb_json_parser *p) {
  upb_selector_t sel;

  UPB_ASSERT(p->top > p->stack);

  p->top--;
  sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSEQ);
  upb_sink_endseq(&p->top->sink, sel);
}

static void start_object(upb_json_parser *p) {
  if (!p->top->is_map) {
    upb_sink_startmsg(&p->top->sink);
  }
}

static void end_object(upb_json_parser *p) {
  if (!p->top->is_map) {
    upb_status status;
    upb_status_clear(&status);
    upb_sink_endmsg(&p->top->sink, &status);
    if (!upb_ok(&status)) {
      upb_env_reporterror(p->env, &status);
    }
  }
}


#define CHECK_RETURN_TOP(x) if (!(x)) goto error


/* The actual parser **********************************************************/

/* What follows is the Ragel parser itself.  The language is specified in Ragel
 * and the actions call our C functions above.
 *
 * Ragel has an extensive set of functionality, and we use only a small part of
 * it.  There are many action types but we only use a few:
 *
 *   ">" -- transition into a machine
 *   "%" -- transition out of a machine
 *   "@" -- transition into a final state of a machine.
 *
 * "@" transitions are tricky because a machine can transition into a final
 * state repeatedly.  But in some cases we know this can't happen, for example
 * a string which is delimited by a final '"' can only transition into its
 * final state once, when the closing '"' is seen. */


#line 1310 "upb/json/parser.rl"



#line 1222 "upb/json/parser.c"
static const char _json_actions[] = {
	0, 1, 0, 1, 2, 1, 3, 1, 
	5, 1, 6, 1, 7, 1, 8, 1, 
	10, 1, 12, 1, 13, 1, 14, 1, 
	15, 1, 16, 1, 17, 1, 21, 1, 
	25, 1, 27, 2, 3, 8, 2, 4, 
	5, 2, 6, 2, 2, 6, 8, 2, 
	11, 9, 2, 13, 15, 2, 14, 15, 
	2, 18, 1, 2, 19, 27, 2, 20, 
	9, 2, 22, 27, 2, 23, 27, 2, 
	24, 27, 2, 26, 27, 3, 14, 11, 
	9
};

static const unsigned char _json_key_offsets[] = {
	0, 0, 4, 9, 14, 15, 19, 24, 
	29, 34, 38, 42, 45, 48, 50, 54, 
	58, 60, 62, 67, 69, 71, 80, 86, 
	92, 98, 104, 106, 115, 116, 116, 116, 
	121, 126, 131, 132, 133, 134, 135, 135, 
	136, 137, 138, 138, 139, 140, 141, 141, 
	146, 151, 152, 156, 161, 166, 171, 175, 
	175, 178, 178, 178
};

static const char _json_trans_keys[] = {
	32, 123, 9, 13, 32, 34, 125, 9, 
	13, 32, 34, 125, 9, 13, 34, 32, 
	58, 9, 13, 32, 93, 125, 9, 13, 
	32, 44, 125, 9, 13, 32, 44, 125, 
	9, 13, 32, 34, 9, 13, 45, 48, 
	49, 57, 48, 49, 57, 46, 69, 101, 
	48, 57, 69, 101, 48, 57, 43, 45, 
	48, 57, 48, 57, 48, 57, 46, 69, 
	101, 48, 57, 34, 92, 34, 92, 34, 
	47, 92, 98, 102, 110, 114, 116, 117, 
	48, 57, 65, 70, 97, 102, 48, 57, 
	65, 70, 97, 102, 48, 57, 65, 70, 
	97, 102, 48, 57, 65, 70, 97, 102, 
	34, 92, 34, 45, 91, 102, 110, 116, 
	123, 48, 57, 34, 32, 93, 125, 9, 
	13, 32, 44, 93, 9, 13, 32, 93, 
	125, 9, 13, 97, 108, 115, 101, 117, 
	108, 108, 114, 117, 101, 32, 34, 125, 
	9, 13, 32, 34, 125, 9, 13, 34, 
	32, 58, 9, 13, 32, 93, 125, 9, 
	13, 32, 44, 125, 9, 13, 32, 44, 
	125, 9, 13, 32, 34, 9, 13, 32, 
	9, 13, 0
};

static const char _json_single_lengths[] = {
	0, 2, 3, 3, 1, 2, 3, 3, 
	3, 2, 2, 1, 3, 0, 2, 2, 
	0, 0, 3, 2, 2, 9, 0, 0, 
	0, 0, 2, 7, 1, 0, 0, 3, 
	3, 3, 1, 1, 1, 1, 0, 1, 
	1, 1, 0, 1, 1, 1, 0, 3, 
	3, 1, 2, 3, 3, 3, 2, 0, 
	1, 0, 0, 0
};

static const char _json_range_lengths[] = {
	0, 1, 1, 1, 0, 1, 1, 1, 
	1, 1, 1, 1, 0, 1, 1, 1, 
	1, 1, 1, 0, 0, 0, 3, 3, 
	3, 3, 0, 1, 0, 0, 0, 1, 
	1, 1, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	1, 0, 1, 1, 1, 1, 1, 0, 
	1, 0, 0, 0
};

static const short _json_index_offsets[] = {
	0, 0, 4, 9, 14, 16, 20, 25, 
	30, 35, 39, 43, 46, 50, 52, 56, 
	60, 62, 64, 69, 72, 75, 85, 89, 
	93, 97, 101, 104, 113, 115, 116, 117, 
	122, 127, 132, 134, 136, 138, 140, 141, 
	143, 145, 147, 148, 150, 152, 154, 155, 
	160, 165, 167, 171, 176, 181, 186, 190, 
	191, 194, 195, 196
};

static const char _json_indicies[] = {
	0, 2, 0, 1, 3, 4, 5, 3, 
	1, 6, 7, 8, 6, 1, 9, 1, 
	10, 11, 10, 1, 11, 1, 1, 11, 
	12, 13, 14, 15, 13, 1, 16, 17, 
	8, 16, 1, 17, 7, 17, 1, 18, 
	19, 20, 1, 19, 20, 1, 22, 23, 
	23, 21, 24, 1, 23, 23, 24, 21, 
	25, 25, 26, 1, 26, 1, 26, 21, 
	22, 23, 23, 20, 21, 28, 29, 27, 
	31, 32, 30, 33, 33, 33, 33, 33, 
	33, 33, 33, 34, 1, 35, 35, 35, 
	1, 36, 36, 36, 1, 37, 37, 37, 
	1, 38, 38, 38, 1, 40, 41, 39, 
	42, 43, 44, 45, 46, 47, 48, 43, 
	1, 49, 1, 50, 51, 53, 54, 1, 
	53, 52, 55, 56, 54, 55, 1, 56, 
	1, 1, 56, 52, 57, 1, 58, 1, 
	59, 1, 60, 1, 61, 62, 1, 63, 
	1, 64, 1, 65, 66, 1, 67, 1, 
	68, 1, 69, 70, 71, 72, 70, 1, 
	73, 74, 75, 73, 1, 76, 1, 77, 
	78, 77, 1, 78, 1, 1, 78, 79, 
	80, 81, 82, 80, 1, 83, 84, 75, 
	83, 1, 84, 74, 84, 1, 85, 86, 
	86, 1, 1, 1, 1, 0
};

static const char _json_trans_targs[] = {
	1, 0, 2, 3, 4, 56, 3, 4, 
	56, 5, 5, 6, 7, 8, 9, 56, 
	8, 9, 11, 12, 18, 57, 13, 15, 
	14, 16, 17, 20, 58, 21, 20, 58, 
	21, 19, 22, 23, 24, 25, 26, 20, 
	58, 21, 28, 30, 31, 34, 39, 43, 
	47, 29, 59, 59, 32, 31, 29, 32, 
	33, 35, 36, 37, 38, 59, 40, 41, 
	42, 59, 44, 45, 46, 59, 48, 49, 
	55, 48, 49, 55, 50, 50, 51, 52, 
	53, 54, 55, 53, 54, 59, 56
};

static const char _json_trans_actions[] = {
	0, 0, 0, 21, 77, 53, 0, 47, 
	23, 17, 0, 0, 15, 19, 19, 50, 
	0, 0, 0, 0, 0, 1, 0, 0, 
	0, 0, 0, 3, 13, 0, 0, 35, 
	5, 11, 0, 38, 7, 7, 7, 41, 
	44, 9, 62, 56, 25, 0, 0, 0, 
	31, 29, 33, 59, 15, 0, 27, 0, 
	0, 0, 0, 0, 0, 68, 0, 0, 
	0, 71, 0, 0, 0, 65, 21, 77, 
	53, 0, 47, 23, 17, 0, 0, 15, 
	19, 19, 50, 0, 0, 74, 0
};

static const int json_start = 1;

static const int json_en_number_machine = 10;
static const int json_en_string_machine = 19;
static const int json_en_value_machine = 27;
static const int json_en_main = 1;


#line 1313 "upb/json/parser.rl"

size_t parse(void *closure, const void *hd, const char *buf, size_t size,
             const upb_bufhandle *handle) {
  upb_json_parser *parser = closure;

  /* Variables used by Ragel's generated code. */
  int cs = parser->current_state;
  int *stack = parser->parser_stack;
  int top = parser->parser_top;

  const char *p = buf;
  const char *pe = buf + size;

  parser->handle = handle;

  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  capture_resume(parser, buf);

  
#line 1393 "upb/json/parser.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _json_trans_keys + _json_key_offsets[cs];
	_trans = _json_index_offsets[cs];

	_klen = _json_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _json_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = _json_indicies[_trans];
	cs = _json_trans_targs[_trans];

	if ( _json_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _json_actions + _json_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 1225 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
	case 1:
#line 1226 "upb/json/parser.rl"
	{ p--; {stack[top++] = cs; cs = 10; goto _again;} }
	break;
	case 2:
#line 1230 "upb/json/parser.rl"
	{ start_text(parser, p); }
	break;
	case 3:
#line 1231 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_text(parser, p)); }
	break;
	case 4:
#line 1237 "upb/json/parser.rl"
	{ start_hex(parser); }
	break;
	case 5:
#line 1238 "upb/json/parser.rl"
	{ hexdigit(parser, p); }
	break;
	case 6:
#line 1239 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_hex(parser)); }
	break;
	case 7:
#line 1245 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(escape(parser, p)); }
	break;
	case 8:
#line 1251 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
	case 9:
#line 1254 "upb/json/parser.rl"
	{ {stack[top++] = cs; cs = 19; goto _again;} }
	break;
	case 10:
#line 1256 "upb/json/parser.rl"
	{ p--; {stack[top++] = cs; cs = 27; goto _again;} }
	break;
	case 11:
#line 1261 "upb/json/parser.rl"
	{ start_member(parser); }
	break;
	case 12:
#line 1262 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_membername(parser)); }
	break;
	case 13:
#line 1265 "upb/json/parser.rl"
	{ end_member(parser); }
	break;
	case 14:
#line 1271 "upb/json/parser.rl"
	{ start_object(parser); }
	break;
	case 15:
#line 1274 "upb/json/parser.rl"
	{ end_object(parser); }
	break;
	case 16:
#line 1280 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(start_array(parser)); }
	break;
	case 17:
#line 1284 "upb/json/parser.rl"
	{ end_array(parser); }
	break;
	case 18:
#line 1289 "upb/json/parser.rl"
	{ start_number(parser, p); }
	break;
	case 19:
#line 1290 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_number(parser, p)); }
	break;
	case 20:
#line 1292 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(start_stringval(parser)); }
	break;
	case 21:
#line 1293 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_stringval(parser)); }
	break;
	case 22:
#line 1295 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(parser_putbool(parser, true)); }
	break;
	case 23:
#line 1297 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(parser_putbool(parser, false)); }
	break;
	case 24:
#line 1299 "upb/json/parser.rl"
	{ /* null value */ }
	break;
	case 25:
#line 1301 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(start_subobject(parser)); }
	break;
	case 26:
#line 1302 "upb/json/parser.rl"
	{ end_subobject(parser); }
	break;
	case 27:
#line 1307 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
#line 1579 "upb/json/parser.c"
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	_out: {}
	}

#line 1334 "upb/json/parser.rl"

  if (p != pe) {
    upb_status_seterrf(&parser->status, "Parse error at '%.*s'\n", pe - p, p);
    upb_env_reporterror(parser->env, &parser->status);
  } else {
    capture_suspend(parser, &p);
  }

error:
  /* Save parsing state back to parser. */
  parser->current_state = cs;
  parser->parser_top = top;

  return p - buf;
}

bool end(void *closure, const void *hd) {
  UPB_UNUSED(closure);
  UPB_UNUSED(hd);

  /* Prevent compile warning on unused static constants. */
  UPB_UNUSED(json_start);
  UPB_UNUSED(json_en_number_machine);
  UPB_UNUSED(json_en_string_machine);
  UPB_UNUSED(json_en_value_machine);
  UPB_UNUSED(json_en_main);
  return true;
}

static void json_parser_reset(upb_json_parser *p) {
  int cs;
  int top;

  p->top = p->stack;
  p->top->f = NULL;
  p->top->is_map = false;
  p->top->is_mapentry = false;

  /* Emit Ragel initialization of the parser. */
  
#line 1633 "upb/json/parser.c"
	{
	cs = json_start;
	top = 0;
	}

#line 1374 "upb/json/parser.rl"
  p->current_state = cs;
  p->parser_top = top;
  accumulate_clear(p);
  p->multipart_state = MULTIPART_INACTIVE;
  p->capture = NULL;
  p->accumulated = NULL;
  upb_status_clear(&p->status);
}

static void visit_json_parsermethod(const upb_refcounted *r,
                                    upb_refcounted_visit *visit,
                                    void *closure) {
  const upb_json_parsermethod *method = (upb_json_parsermethod*)r;
  visit(r, upb_msgdef_upcast2(method->msg), closure);
}

static void free_json_parsermethod(upb_refcounted *r) {
  upb_json_parsermethod *method = (upb_json_parsermethod*)r;

  upb_inttable_iter i;
  upb_inttable_begin(&i, &method->name_tables);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_value val = upb_inttable_iter_value(&i);
    upb_strtable *t = upb_value_getptr(val);
    upb_strtable_uninit(t);
    upb_gfree(t);
  }

  upb_inttable_uninit(&method->name_tables);

  upb_gfree(r);
}

static void add_jsonname_table(upb_json_parsermethod *m, const upb_msgdef* md) {
  upb_msg_field_iter i;
  upb_strtable *t;

  /* It would be nice to stack-allocate this, but protobufs do not limit the
   * length of fields to any reasonable limit. */
  char *buf = NULL;
  size_t len = 0;

  if (upb_inttable_lookupptr(&m->name_tables, md, NULL)) {
    return;
  }

  /* TODO(haberman): handle malloc failure. */
  t = upb_gmalloc(sizeof(*t));
  upb_strtable_init(t, UPB_CTYPE_CONSTPTR);
  upb_inttable_insertptr(&m->name_tables, md, upb_value_ptr(t));

  for(upb_msg_field_begin(&i, md);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);

    /* Add an entry for the JSON name. */
    size_t field_len = upb_fielddef_getjsonname(f, buf, len);
    if (field_len > len) {
      size_t len2;
      buf = upb_grealloc(buf, 0, field_len);
      len = field_len;
      len2 = upb_fielddef_getjsonname(f, buf, len);
      UPB_ASSERT(len == len2);
    }
    upb_strtable_insert(t, buf, upb_value_constptr(f));

    if (strcmp(buf, upb_fielddef_name(f)) != 0) {
      /* Since the JSON name is different from the regular field name, add an
       * entry for the raw name (compliant proto3 JSON parsers must accept
       * both). */
      upb_strtable_insert(t, upb_fielddef_name(f), upb_value_constptr(f));
    }

    if (upb_fielddef_issubmsg(f)) {
      add_jsonname_table(m, upb_fielddef_msgsubdef(f));
    }
  }

  upb_gfree(buf);
}

/* Public API *****************************************************************/

upb_json_parser *upb_json_parser_create(upb_env *env,
                                        const upb_json_parsermethod *method,
                                        upb_sink *output) {
#ifndef NDEBUG
  const size_t size_before = upb_env_bytesallocated(env);
#endif
  upb_json_parser *p = upb_env_malloc(env, sizeof(upb_json_parser));
  if (!p) return false;

  p->env = env;
  p->method = method;
  p->limit = p->stack + UPB_JSON_MAX_DEPTH;
  p->accumulate_buf = NULL;
  p->accumulate_buf_size = 0;
  upb_bytessink_reset(&p->input_, &method->input_handler_, p);

  json_parser_reset(p);
  upb_sink_reset(&p->top->sink, output->handlers, output->closure);
  p->top->m = upb_handlers_msgdef(output->handlers);
  set_name_table(p, p->top);

  /* If this fails, uncomment and increase the value in parser.h. */
  /* fprintf(stderr, "%zd\n", upb_env_bytesallocated(env) - size_before); */
  UPB_ASSERT_DEBUGVAR(upb_env_bytesallocated(env) - size_before <=
                      UPB_JSON_PARSER_SIZE);
  return p;
}

upb_bytessink *upb_json_parser_input(upb_json_parser *p) {
  return &p->input_;
}

upb_json_parsermethod *upb_json_parsermethod_new(const upb_msgdef* md,
                                                 const void* owner) {
  static const struct upb_refcounted_vtbl vtbl = {visit_json_parsermethod,
                                                  free_json_parsermethod};
  upb_json_parsermethod *ret = upb_gmalloc(sizeof(*ret));
  upb_refcounted_init(upb_json_parsermethod_upcast_mutable(ret), &vtbl, owner);

  ret->msg = md;
  upb_ref2(md, ret);

  upb_byteshandler_init(&ret->input_handler_);
  upb_byteshandler_setstring(&ret->input_handler_, parse, ret);
  upb_byteshandler_setendstr(&ret->input_handler_, end, ret);

  upb_inttable_init(&ret->name_tables, UPB_CTYPE_PTR);

  add_jsonname_table(ret, md);

  return ret;
}

const upb_byteshandler *upb_json_parsermethod_inputhandler(
    const upb_json_parsermethod *m) {
  return &m->input_handler_;
}
/*
** This currently uses snprintf() to format primitives, and could be optimized
** further.
*/


#include <string.h>
#include <stdint.h>

struct upb_json_printer {
  upb_sink input_;
  /* BytesSink closure. */
  void *subc_;
  upb_bytessink *output_;

  /* We track the depth so that we know when to emit startstr/endstr on the
   * output. */
  int depth_;

  /* Have we emitted the first element? This state is necessary to emit commas
   * without leaving a trailing comma in arrays/maps. We keep this state per
   * frame depth.
   *
   * Why max_depth * 2? UPB_MAX_HANDLER_DEPTH counts depth as nested messages.
   * We count frames (contexts in which we separate elements by commas) as both
   * repeated fields and messages (maps), and the worst case is a
   * message->repeated field->submessage->repeated field->... nesting. */
  bool first_elem_[UPB_MAX_HANDLER_DEPTH * 2];
};

/* StringPiece; a pointer plus a length. */
typedef struct {
  char *ptr;
  size_t len;
} strpc;

void freestrpc(void *ptr) {
  strpc *pc = ptr;
  upb_gfree(pc->ptr);
  upb_gfree(pc);
}

/* Convert fielddef name to JSON name and return as a string piece. */
strpc *newstrpc(upb_handlers *h, const upb_fielddef *f,
                bool preserve_fieldnames) {
  /* TODO(haberman): handle malloc failure. */
  strpc *ret = upb_gmalloc(sizeof(*ret));
  if (preserve_fieldnames) {
    ret->ptr = upb_gstrdup(upb_fielddef_name(f));
    ret->len = strlen(ret->ptr);
  } else {
    size_t len;
    ret->len = upb_fielddef_getjsonname(f, NULL, 0);
    ret->ptr = upb_gmalloc(ret->len);
    len = upb_fielddef_getjsonname(f, ret->ptr, ret->len);
    UPB_ASSERT(len == ret->len);
    ret->len--;  /* NULL */
  }

  upb_handlers_addcleanup(h, ret, freestrpc);
  return ret;
}

/* ------------ JSON string printing: values, maps, arrays ------------------ */

static void print_data(
    upb_json_printer *p, const char *buf, unsigned int len) {
  /* TODO: Will need to change if we support pushback from the sink. */
  size_t n = upb_bytessink_putbuf(p->output_, p->subc_, buf, len, NULL);
  UPB_ASSERT(n == len);
}

static void print_comma(upb_json_printer *p) {
  if (!p->first_elem_[p->depth_]) {
    print_data(p, ",", 1);
  }
  p->first_elem_[p->depth_] = false;
}

/* Helpers that print properly formatted elements to the JSON output stream. */

/* Used for escaping control chars in strings. */
static const char kControlCharLimit = 0x20;

UPB_INLINE bool is_json_escaped(char c) {
  /* See RFC 4627. */
  unsigned char uc = (unsigned char)c;
  return uc < kControlCharLimit || uc == '"' || uc == '\\';
}

UPB_INLINE const char* json_nice_escape(char c) {
  switch (c) {
    case '"':  return "\\\"";
    case '\\': return "\\\\";
    case '\b': return "\\b";
    case '\f': return "\\f";
    case '\n': return "\\n";
    case '\r': return "\\r";
    case '\t': return "\\t";
    default:   return NULL;
  }
}

/* Write a properly escaped string chunk. The surrounding quotes are *not*
 * printed; this is so that the caller has the option of emitting the string
 * content in chunks. */
static void putstring(upb_json_printer *p, const char *buf, unsigned int len) {
  const char* unescaped_run = NULL;
  unsigned int i;
  for (i = 0; i < len; i++) {
    char c = buf[i];
    /* Handle escaping. */
    if (is_json_escaped(c)) {
      /* Use a "nice" escape, like \n, if one exists for this character. */
      const char* escape = json_nice_escape(c);
      /* If we don't have a specific 'nice' escape code, use a \uXXXX-style
       * escape. */
      char escape_buf[8];
      if (!escape) {
        unsigned char byte = (unsigned char)c;
        _upb_snprintf(escape_buf, sizeof(escape_buf), "\\u%04x", (int)byte);
        escape = escape_buf;
      }

      /* N.B. that we assume that the input encoding is equal to the output
       * encoding (both UTF-8 for  now), so for chars >= 0x20 and != \, ", we
       * can simply pass the bytes through. */

      /* If there's a current run of unescaped chars, print that run first. */
      if (unescaped_run) {
        print_data(p, unescaped_run, &buf[i] - unescaped_run);
        unescaped_run = NULL;
      }
      /* Then print the escape code. */
      print_data(p, escape, strlen(escape));
    } else {
      /* Add to the current unescaped run of characters. */
      if (unescaped_run == NULL) {
        unescaped_run = &buf[i];
      }
    }
  }

  /* If the string ended in a run of unescaped characters, print that last run. */
  if (unescaped_run) {
    print_data(p, unescaped_run, &buf[len] - unescaped_run);
  }
}

#define CHKLENGTH(x) if (!(x)) return -1;

/* Helpers that format floating point values according to our custom formats.
 * Right now we use %.8g and %.17g for float/double, respectively, to match
 * proto2::util::JsonFormat's defaults.  May want to change this later. */

const char neginf[] = "\"-Infinity\"";
const char inf[] = "\"Infinity\"";

static size_t fmt_double(double val, char* buf, size_t length) {
  if (val == (1.0 / 0.0)) {
    CHKLENGTH(length >= strlen(inf));
    strcpy(buf, inf);
    return strlen(inf);
  } else if (val == (-1.0 / 0.0)) {
    CHKLENGTH(length >= strlen(neginf));
    strcpy(buf, neginf);
    return strlen(neginf);
  } else {
    size_t n = _upb_snprintf(buf, length, "%.17g", val);
    CHKLENGTH(n > 0 && n < length);
    return n;
  }
}

static size_t fmt_float(float val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%.8g", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_bool(bool val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%s", (val ? "true" : "false"));
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_int64(long val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%ld", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_uint64(unsigned long long val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%llu", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

/* Print a map key given a field name. Called by scalar field handlers and by
 * startseq for repeated fields. */
static bool putkey(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  const strpc *key = handler_data;
  print_comma(p);
  print_data(p, "\"", 1);
  putstring(p, key->ptr, key->len);
  print_data(p, "\":", 2);
  return true;
}

#define CHKFMT(val) if ((val) == (size_t)-1) return false;
#define CHK(val)    if (!(val)) return false;

#define TYPE_HANDLERS(type, fmt_func)                                        \
  static bool put##type(void *closure, const void *handler_data, type val) { \
    upb_json_printer *p = closure;                                           \
    char data[64];                                                           \
    size_t length = fmt_func(val, data, sizeof(data));                       \
    UPB_UNUSED(handler_data);                                                \
    CHKFMT(length);                                                          \
    print_data(p, data, length);                                             \
    return true;                                                             \
  }                                                                          \
  static bool scalar_##type(void *closure, const void *handler_data,         \
                            type val) {                                      \
    CHK(putkey(closure, handler_data));                                      \
    CHK(put##type(closure, handler_data, val));                              \
    return true;                                                             \
  }                                                                          \
  static bool repeated_##type(void *closure, const void *handler_data,       \
                              type val) {                                    \
    upb_json_printer *p = closure;                                           \
    print_comma(p);                                                          \
    CHK(put##type(closure, handler_data, val));                              \
    return true;                                                             \
  }

#define TYPE_HANDLERS_MAPKEY(type, fmt_func)                                 \
  static bool putmapkey_##type(void *closure, const void *handler_data,      \
                            type val) {                                      \
    upb_json_printer *p = closure;                                           \
    print_data(p, "\"", 1);                                                  \
    CHK(put##type(closure, handler_data, val));                              \
    print_data(p, "\":", 2);                                                 \
    return true;                                                             \
  }

TYPE_HANDLERS(double,   fmt_double)
TYPE_HANDLERS(float,    fmt_float)
TYPE_HANDLERS(bool,     fmt_bool)
TYPE_HANDLERS(int32_t,  fmt_int64)
TYPE_HANDLERS(uint32_t, fmt_int64)
TYPE_HANDLERS(int64_t,  fmt_int64)
TYPE_HANDLERS(uint64_t, fmt_uint64)

/* double and float are not allowed to be map keys. */
TYPE_HANDLERS_MAPKEY(bool,     fmt_bool)
TYPE_HANDLERS_MAPKEY(int32_t,  fmt_int64)
TYPE_HANDLERS_MAPKEY(uint32_t, fmt_int64)
TYPE_HANDLERS_MAPKEY(int64_t,  fmt_int64)
TYPE_HANDLERS_MAPKEY(uint64_t, fmt_uint64)

#undef TYPE_HANDLERS
#undef TYPE_HANDLERS_MAPKEY

typedef struct {
  void *keyname;
  const upb_enumdef *enumdef;
} EnumHandlerData;

static bool scalar_enum(void *closure, const void *handler_data,
                        int32_t val) {
  const EnumHandlerData *hd = handler_data;
  upb_json_printer *p = closure;
  const char *symbolic_name;

  CHK(putkey(closure, hd->keyname));

  symbolic_name = upb_enumdef_iton(hd->enumdef, val);
  if (symbolic_name) {
    print_data(p, "\"", 1);
    putstring(p, symbolic_name, strlen(symbolic_name));
    print_data(p, "\"", 1);
  } else {
    putint32_t(closure, NULL, val);
  }

  return true;
}

static void print_enum_symbolic_name(upb_json_printer *p,
                                     const upb_enumdef *def,
                                     int32_t val) {
  const char *symbolic_name = upb_enumdef_iton(def, val);
  if (symbolic_name) {
    print_data(p, "\"", 1);
    putstring(p, symbolic_name, strlen(symbolic_name));
    print_data(p, "\"", 1);
  } else {
    putint32_t(p, NULL, val);
  }
}

static bool repeated_enum(void *closure, const void *handler_data,
                          int32_t val) {
  const EnumHandlerData *hd = handler_data;
  upb_json_printer *p = closure;
  print_comma(p);

  print_enum_symbolic_name(p, hd->enumdef, val);

  return true;
}

static bool mapvalue_enum(void *closure, const void *handler_data,
                          int32_t val) {
  const EnumHandlerData *hd = handler_data;
  upb_json_printer *p = closure;

  print_enum_symbolic_name(p, hd->enumdef, val);

  return true;
}

static void *scalar_startsubmsg(void *closure, const void *handler_data) {
  return putkey(closure, handler_data) ? closure : UPB_BREAK;
}

static void *repeated_startsubmsg(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_comma(p);
  return closure;
}

static void start_frame(upb_json_printer *p) {
  p->depth_++;
  p->first_elem_[p->depth_] = true;
  print_data(p, "{", 1);
}

static void end_frame(upb_json_printer *p) {
  print_data(p, "}", 1);
  p->depth_--;
}

static bool printer_startmsg(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  if (p->depth_ == 0) {
    upb_bytessink_start(p->output_, 0, &p->subc_);
  }
  start_frame(p);
  return true;
}

static bool printer_endmsg(void *closure, const void *handler_data, upb_status *s) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(s);
  end_frame(p);
  if (p->depth_ == 0) {
    upb_bytessink_end(p->output_);
  }
  return true;
}

static void *startseq(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  CHK(putkey(closure, handler_data));
  p->depth_++;
  p->first_elem_[p->depth_] = true;
  print_data(p, "[", 1);
  return closure;
}

static bool endseq(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "]", 1);
  p->depth_--;
  return true;
}

static void *startmap(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  CHK(putkey(closure, handler_data));
  p->depth_++;
  p->first_elem_[p->depth_] = true;
  print_data(p, "{", 1);
  return closure;
}

static bool endmap(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "}", 1);
  p->depth_--;
  return true;
}

static size_t putstr(void *closure, const void *handler_data, const char *str,
                     size_t len, const upb_bufhandle *handle) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(handle);
  putstring(p, str, len);
  return len;
}

/* This has to Base64 encode the bytes, because JSON has no "bytes" type. */
static size_t putbytes(void *closure, const void *handler_data, const char *str,
                       size_t len, const upb_bufhandle *handle) {
  upb_json_printer *p = closure;

  /* This is the regular base64, not the "web-safe" version. */
  static const char base64[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  /* Base64-encode. */
  char data[16000];
  const char *limit = data + sizeof(data);
  const unsigned char *from = (const unsigned char*)str;
  char *to = data;
  size_t remaining = len;
  size_t bytes;

  UPB_UNUSED(handler_data);
  UPB_UNUSED(handle);

  while (remaining > 2) {
    /* TODO(haberman): handle encoded lengths > sizeof(data) */
    UPB_ASSERT((limit - to) >= 4);

    to[0] = base64[from[0] >> 2];
    to[1] = base64[((from[0] & 0x3) << 4) | (from[1] >> 4)];
    to[2] = base64[((from[1] & 0xf) << 2) | (from[2] >> 6)];
    to[3] = base64[from[2] & 0x3f];

    remaining -= 3;
    to += 4;
    from += 3;
  }

  switch (remaining) {
    case 2:
      to[0] = base64[from[0] >> 2];
      to[1] = base64[((from[0] & 0x3) << 4) | (from[1] >> 4)];
      to[2] = base64[(from[1] & 0xf) << 2];
      to[3] = '=';
      to += 4;
      from += 2;
      break;
    case 1:
      to[0] = base64[from[0] >> 2];
      to[1] = base64[((from[0] & 0x3) << 4)];
      to[2] = '=';
      to[3] = '=';
      to += 4;
      from += 1;
      break;
  }

  bytes = to - data;
  print_data(p, "\"", 1);
  putstring(p, data, bytes);
  print_data(p, "\"", 1);
  return len;
}

static void *scalar_startstr(void *closure, const void *handler_data,
                             size_t size_hint) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(size_hint);
  CHK(putkey(closure, handler_data));
  print_data(p, "\"", 1);
  return p;
}

static size_t scalar_str(void *closure, const void *handler_data,
                         const char *str, size_t len,
                         const upb_bufhandle *handle) {
  CHK(putstr(closure, handler_data, str, len, handle));
  return len;
}

static bool scalar_endstr(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "\"", 1);
  return true;
}

static void *repeated_startstr(void *closure, const void *handler_data,
                               size_t size_hint) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(size_hint);
  print_comma(p);
  print_data(p, "\"", 1);
  return p;
}

static size_t repeated_str(void *closure, const void *handler_data,
                           const char *str, size_t len,
                           const upb_bufhandle *handle) {
  CHK(putstr(closure, handler_data, str, len, handle));
  return len;
}

static bool repeated_endstr(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "\"", 1);
  return true;
}

static void *mapkeyval_startstr(void *closure, const void *handler_data,
                                size_t size_hint) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(size_hint);
  print_data(p, "\"", 1);
  return p;
}

static size_t mapkey_str(void *closure, const void *handler_data,
                         const char *str, size_t len,
                         const upb_bufhandle *handle) {
  CHK(putstr(closure, handler_data, str, len, handle));
  return len;
}

static bool mapkey_endstr(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "\":", 2);
  return true;
}

static bool mapvalue_endstr(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "\"", 1);
  return true;
}

static size_t scalar_bytes(void *closure, const void *handler_data,
                           const char *str, size_t len,
                           const upb_bufhandle *handle) {
  CHK(putkey(closure, handler_data));
  CHK(putbytes(closure, handler_data, str, len, handle));
  return len;
}

static size_t repeated_bytes(void *closure, const void *handler_data,
                             const char *str, size_t len,
                             const upb_bufhandle *handle) {
  upb_json_printer *p = closure;
  print_comma(p);
  CHK(putbytes(closure, handler_data, str, len, handle));
  return len;
}

static size_t mapkey_bytes(void *closure, const void *handler_data,
                           const char *str, size_t len,
                           const upb_bufhandle *handle) {
  upb_json_printer *p = closure;
  CHK(putbytes(closure, handler_data, str, len, handle));
  print_data(p, ":", 1);
  return len;
}

static void set_enum_hd(upb_handlers *h,
                        const upb_fielddef *f,
                        bool preserve_fieldnames,
                        upb_handlerattr *attr) {
  EnumHandlerData *hd = upb_gmalloc(sizeof(EnumHandlerData));
  hd->enumdef = (const upb_enumdef *)upb_fielddef_subdef(f);
  hd->keyname = newstrpc(h, f, preserve_fieldnames);
  upb_handlers_addcleanup(h, hd, upb_gfree);
  upb_handlerattr_sethandlerdata(attr, hd);
}

/* Set up handlers for a mapentry submessage (i.e., an individual key/value pair
 * in a map).
 *
 * TODO: Handle missing key, missing value, out-of-order key/value, or repeated
 * key or value cases properly. The right way to do this is to allocate a
 * temporary structure at the start of a mapentry submessage, store key and
 * value data in it as key and value handlers are called, and then print the
 * key/value pair once at the end of the submessage. If we don't do this, we
 * should at least detect the case and throw an error. However, so far all of
 * our sources that emit mapentry messages do so canonically (with one key
 * field, and then one value field), so this is not a pressing concern at the
 * moment. */
void printer_sethandlers_mapentry(const void *closure, bool preserve_fieldnames,
                                  upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);

  /* A mapentry message is printed simply as '"key": value'. Rather than
   * special-case key and value for every type below, we just handle both
   * fields explicitly here. */
  const upb_fielddef* key_field = upb_msgdef_itof(md, UPB_MAPENTRY_KEY);
  const upb_fielddef* value_field = upb_msgdef_itof(md, UPB_MAPENTRY_VALUE);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INITIALIZER;

  UPB_UNUSED(closure);

  switch (upb_fielddef_type(key_field)) {
    case UPB_TYPE_INT32:
      upb_handlers_setint32(h, key_field, putmapkey_int32_t, &empty_attr);
      break;
    case UPB_TYPE_INT64:
      upb_handlers_setint64(h, key_field, putmapkey_int64_t, &empty_attr);
      break;
    case UPB_TYPE_UINT32:
      upb_handlers_setuint32(h, key_field, putmapkey_uint32_t, &empty_attr);
      break;
    case UPB_TYPE_UINT64:
      upb_handlers_setuint64(h, key_field, putmapkey_uint64_t, &empty_attr);
      break;
    case UPB_TYPE_BOOL:
      upb_handlers_setbool(h, key_field, putmapkey_bool, &empty_attr);
      break;
    case UPB_TYPE_STRING:
      upb_handlers_setstartstr(h, key_field, mapkeyval_startstr, &empty_attr);
      upb_handlers_setstring(h, key_field, mapkey_str, &empty_attr);
      upb_handlers_setendstr(h, key_field, mapkey_endstr, &empty_attr);
      break;
    case UPB_TYPE_BYTES:
      upb_handlers_setstring(h, key_field, mapkey_bytes, &empty_attr);
      break;
    default:
      UPB_ASSERT(false);
      break;
  }

  switch (upb_fielddef_type(value_field)) {
    case UPB_TYPE_INT32:
      upb_handlers_setint32(h, value_field, putint32_t, &empty_attr);
      break;
    case UPB_TYPE_INT64:
      upb_handlers_setint64(h, value_field, putint64_t, &empty_attr);
      break;
    case UPB_TYPE_UINT32:
      upb_handlers_setuint32(h, value_field, putuint32_t, &empty_attr);
      break;
    case UPB_TYPE_UINT64:
      upb_handlers_setuint64(h, value_field, putuint64_t, &empty_attr);
      break;
    case UPB_TYPE_BOOL:
      upb_handlers_setbool(h, value_field, putbool, &empty_attr);
      break;
    case UPB_TYPE_FLOAT:
      upb_handlers_setfloat(h, value_field, putfloat, &empty_attr);
      break;
    case UPB_TYPE_DOUBLE:
      upb_handlers_setdouble(h, value_field, putdouble, &empty_attr);
      break;
    case UPB_TYPE_STRING:
      upb_handlers_setstartstr(h, value_field, mapkeyval_startstr, &empty_attr);
      upb_handlers_setstring(h, value_field, putstr, &empty_attr);
      upb_handlers_setendstr(h, value_field, mapvalue_endstr, &empty_attr);
      break;
    case UPB_TYPE_BYTES:
      upb_handlers_setstring(h, value_field, putbytes, &empty_attr);
      break;
    case UPB_TYPE_ENUM: {
      upb_handlerattr enum_attr = UPB_HANDLERATTR_INITIALIZER;
      set_enum_hd(h, value_field, preserve_fieldnames, &enum_attr);
      upb_handlers_setint32(h, value_field, mapvalue_enum, &enum_attr);
      upb_handlerattr_uninit(&enum_attr);
      break;
    }
    case UPB_TYPE_MESSAGE:
      /* No handler necessary -- the submsg handlers will print the message
       * as appropriate. */
      break;
  }

  upb_handlerattr_uninit(&empty_attr);
}

void printer_sethandlers(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);
  bool is_mapentry = upb_msgdef_mapentry(md);
  upb_handlerattr empty_attr = UPB_HANDLERATTR_INITIALIZER;
  upb_msg_field_iter i;
  const bool *preserve_fieldnames_ptr = closure;
  const bool preserve_fieldnames = *preserve_fieldnames_ptr;

  if (is_mapentry) {
    /* mapentry messages are sufficiently different that we handle them
     * separately. */
    printer_sethandlers_mapentry(closure, preserve_fieldnames, h);
    return;
  }

  upb_handlers_setstartmsg(h, printer_startmsg, &empty_attr);
  upb_handlers_setendmsg(h, printer_endmsg, &empty_attr);

#define TYPE(type, name, ctype)                                               \
  case type:                                                                  \
    if (upb_fielddef_isseq(f)) {                                              \
      upb_handlers_set##name(h, f, repeated_##ctype, &empty_attr);            \
    } else {                                                                  \
      upb_handlers_set##name(h, f, scalar_##ctype, &name_attr);               \
    }                                                                         \
    break;

  upb_msg_field_begin(&i, md);
  for(; !upb_msg_field_done(&i); upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);

    upb_handlerattr name_attr = UPB_HANDLERATTR_INITIALIZER;
    upb_handlerattr_sethandlerdata(&name_attr,
                                   newstrpc(h, f, preserve_fieldnames));

    if (upb_fielddef_ismap(f)) {
      upb_handlers_setstartseq(h, f, startmap, &name_attr);
      upb_handlers_setendseq(h, f, endmap, &name_attr);
    } else if (upb_fielddef_isseq(f)) {
      upb_handlers_setstartseq(h, f, startseq, &name_attr);
      upb_handlers_setendseq(h, f, endseq, &empty_attr);
    }

    switch (upb_fielddef_type(f)) {
      TYPE(UPB_TYPE_FLOAT,  float,  float);
      TYPE(UPB_TYPE_DOUBLE, double, double);
      TYPE(UPB_TYPE_BOOL,   bool,   bool);
      TYPE(UPB_TYPE_INT32,  int32,  int32_t);
      TYPE(UPB_TYPE_UINT32, uint32, uint32_t);
      TYPE(UPB_TYPE_INT64,  int64,  int64_t);
      TYPE(UPB_TYPE_UINT64, uint64, uint64_t);
      case UPB_TYPE_ENUM: {
        /* For now, we always emit symbolic names for enums. We may want an
         * option later to control this behavior, but we will wait for a real
         * need first. */
        upb_handlerattr enum_attr = UPB_HANDLERATTR_INITIALIZER;
        set_enum_hd(h, f, preserve_fieldnames, &enum_attr);

        if (upb_fielddef_isseq(f)) {
          upb_handlers_setint32(h, f, repeated_enum, &enum_attr);
        } else {
          upb_handlers_setint32(h, f, scalar_enum, &enum_attr);
        }

        upb_handlerattr_uninit(&enum_attr);
        break;
      }
      case UPB_TYPE_STRING:
        if (upb_fielddef_isseq(f)) {
          upb_handlers_setstartstr(h, f, repeated_startstr, &empty_attr);
          upb_handlers_setstring(h, f, repeated_str, &empty_attr);
          upb_handlers_setendstr(h, f, repeated_endstr, &empty_attr);
        } else {
          upb_handlers_setstartstr(h, f, scalar_startstr, &name_attr);
          upb_handlers_setstring(h, f, scalar_str, &empty_attr);
          upb_handlers_setendstr(h, f, scalar_endstr, &empty_attr);
        }
        break;
      case UPB_TYPE_BYTES:
        /* XXX: this doesn't support strings that span buffers yet. The base64
         * encoder will need to be made resumable for this to work properly. */
        if (upb_fielddef_isseq(f)) {
          upb_handlers_setstring(h, f, repeated_bytes, &empty_attr);
        } else {
          upb_handlers_setstring(h, f, scalar_bytes, &name_attr);
        }
        break;
      case UPB_TYPE_MESSAGE:
        if (upb_fielddef_isseq(f)) {
          upb_handlers_setstartsubmsg(h, f, repeated_startsubmsg, &name_attr);
        } else {
          upb_handlers_setstartsubmsg(h, f, scalar_startsubmsg, &name_attr);
        }
        break;
    }

    upb_handlerattr_uninit(&name_attr);
  }

  upb_handlerattr_uninit(&empty_attr);
#undef TYPE
}

static void json_printer_reset(upb_json_printer *p) {
  p->depth_ = 0;
}


/* Public API *****************************************************************/

upb_json_printer *upb_json_printer_create(upb_env *e, const upb_handlers *h,
                                          upb_bytessink *output) {
#ifndef NDEBUG
  size_t size_before = upb_env_bytesallocated(e);
#endif

  upb_json_printer *p = upb_env_malloc(e, sizeof(upb_json_printer));
  if (!p) return NULL;

  p->output_ = output;
  json_printer_reset(p);
  upb_sink_reset(&p->input_, h, p);

  /* If this fails, increase the value in printer.h. */
  UPB_ASSERT_DEBUGVAR(upb_env_bytesallocated(e) - size_before <=
                      UPB_JSON_PRINTER_SIZE);
  return p;
}

upb_sink *upb_json_printer_input(upb_json_printer *p) {
  return &p->input_;
}

const upb_handlers *upb_json_printer_newhandlers(const upb_msgdef *md,
                                                 bool preserve_fieldnames,
                                                 const void *owner) {
  return upb_handlers_newfrozen(
      md, owner, printer_sethandlers, &preserve_fieldnames);
}
