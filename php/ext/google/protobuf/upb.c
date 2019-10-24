/* Amalgamated source file */
#include "upb.h"
/*
* This is where we define macros used across upb.
*
* All of these macros are undef'd in port_undef.inc to avoid leaking them to
* users.
*
* The correct usage is:
*
*   #include "upb/foobar.h"
*   #include "upb/baz.h"
*
*   // MUST be last included header.
*   #include "upb/port_def.inc"
*
*   // Code for this file.
*   // <...>
*
*   // Can be omitted for .c files, required for .h.
*   #include "upb/port_undef.inc"
*
* This file is private and must not be included by users!
*/
#ifndef UINTPTR_MAX
#error must include stdint.h first
#endif

#if UINTPTR_MAX == 0xffffffff
#define UPB_SIZE(size32, size64) size32
#else
#define UPB_SIZE(size32, size64) size64
#endif

#define UPB_FIELD_AT(msg, fieldtype, offset) \
  *(fieldtype*)((const char*)(msg) + offset)

#define UPB_READ_ONEOF(msg, fieldtype, offset, case_offset, case_val, default) \
  UPB_FIELD_AT(msg, int, case_offset) == case_val                              \
      ? UPB_FIELD_AT(msg, fieldtype, offset)                                   \
      : default

#define UPB_WRITE_ONEOF(msg, fieldtype, offset, value, case_offset, case_val) \
  UPB_FIELD_AT(msg, int, case_offset) = case_val;                             \
  UPB_FIELD_AT(msg, fieldtype, offset) = value;

/* UPB_INLINE: inline if possible, emit standalone code if required. */
#ifdef __cplusplus
#define UPB_INLINE inline
#elif defined (__GNUC__) || defined(__clang__)
#define UPB_INLINE static __inline__
#else
#define UPB_INLINE static
#endif

/* Hints to the compiler about likely/unlikely branches. */
#if defined (__GNUC__) || defined(__clang__)
#define UPB_LIKELY(x) __builtin_expect((x),1)
#define UPB_UNLIKELY(x) __builtin_expect((x),0)
#else
#define UPB_LIKELY(x) (x)
#define UPB_UNLIKELY(x) (x)
#endif

/* Define UPB_BIG_ENDIAN manually if you're on big endian and your compiler
 * doesn't provide these preprocessor symbols. */
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define UPB_BIG_ENDIAN
#endif

/* Macros for function attributes on compilers that support them. */
#ifdef __GNUC__
#define UPB_FORCEINLINE __inline__ __attribute__((always_inline))
#define UPB_NOINLINE __attribute__((noinline))
#define UPB_NORETURN __attribute__((__noreturn__))
#else  /* !defined(__GNUC__) */
#define UPB_FORCEINLINE
#define UPB_NOINLINE
#define UPB_NORETURN
#endif

#if __STDC_VERSION__ >= 199901L || __cplusplus >= 201103L
/* C99/C++11 versions. */
#include <stdio.h>
#define _upb_snprintf snprintf
#define _upb_vsnprintf vsnprintf
#define _upb_va_copy(a, b) va_copy(a, b)
#elif defined(_MSC_VER)
/* Microsoft C/C++ versions. */
#include <stdarg.h>
#include <stdio.h>
#if _MSC_VER < 1900
int msvc_snprintf(char* s, size_t n, const char* format, ...);
int msvc_vsnprintf(char* s, size_t n, const char* format, va_list arg);
#define UPB_MSVC_VSNPRINTF
#define _upb_snprintf msvc_snprintf
#define _upb_vsnprintf msvc_vsnprintf
#else
#define _upb_snprintf snprintf
#define _upb_vsnprintf vsnprintf
#endif
#define _upb_va_copy(a, b) va_copy(a, b)
#elif defined __GNUC__
/* A few hacky workarounds for functions not in C89.
 * For internal use only!
 * TODO(haberman): fix these by including our own implementations, or finding
 * another workaround.
 */
#define _upb_snprintf __builtin_snprintf
#define _upb_vsnprintf __builtin_vsnprintf
#define _upb_va_copy(a, b) __va_copy(a, b)
#else
#error Need implementations of [v]snprintf and va_copy
#endif

#ifdef __cplusplus
#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__) || \
    (defined(_MSC_VER) && _MSC_VER >= 1900)
// C++11 is present
#else
#error upb requires C++11 for C++ support
#endif
#endif

#define UPB_MAX(x, y) ((x) > (y) ? (x) : (y))
#define UPB_MIN(x, y) ((x) < (y) ? (x) : (y))

#define UPB_UNUSED(var) (void)var

/* UPB_ASSERT(): in release mode, we use the expression without letting it be
 * evaluated.  This prevents "unused variable" warnings. */
#ifdef NDEBUG
#define UPB_ASSERT(expr) do {} while (false && (expr))
#else
#define UPB_ASSERT(expr) assert(expr)
#endif

/* UPB_ASSERT_DEBUGVAR(): assert that uses functions or variables that only
 * exist in debug mode.  This turns into regular assert. */
#define UPB_ASSERT_DEBUGVAR(expr) assert(expr)

#if defined(__GNUC__) || defined(__clang__)
#define UPB_UNREACHABLE() do { assert(0); __builtin_unreachable(); } while(0)
#else
#define UPB_UNREACHABLE() do { assert(0); } while(0)
#endif

/* UPB_INFINITY representing floating-point positive infinity. */
#include <math.h>
#ifdef INFINITY
#define UPB_INFINITY INFINITY
#else
#define UPB_INFINITY (1.0 / 0.0)
#endif

#include <string.h>


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
/* We encode backwards, to avoid pre-computing lengths (one-pass encode). */


#include <string.h>



#define UPB_PB_VARINT_MAX_LEN 10
#define CHK(x) do { if (!(x)) { return false; } } while(0)

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

static uint32_t upb_zzencode_32(int32_t n) { return ((uint32_t)n << 1) ^ (n >> 31); }
static uint64_t upb_zzencode_64(int64_t n) { return ((uint64_t)n << 1) ^ (n >> 63); }

typedef struct {
  upb_alloc *alloc;
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
  char *new_buf = upb_realloc(e->alloc, e->buf, old_size, new_size);
  CHK(new_buf);

  /* We want previous data at the end, realloc() put it at the beginning. */
  if (old_size > 0) {
    memmove(new_buf + new_size - old_size, e->buf, old_size);
  }

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

static uint32_t upb_readcase(const char *msg, const upb_msglayout_field *f) {
  uint32_t ret;
  uint32_t offset = ~f->presence;
  memcpy(&ret, msg + offset, sizeof(ret));
  return ret;
}

static bool upb_readhasbit(const char *msg, const upb_msglayout_field *f) {
  uint32_t hasbit = f->presence;
  UPB_ASSERT(f->presence > 0);
  return msg[hasbit / 8] & (1 << (hasbit % 8));
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
                        const upb_msglayout *m, size_t *size);

static bool upb_encode_array(upb_encstate *e, const char *field_mem,
                             const upb_msglayout *m,
                             const upb_msglayout_field *f) {
  const upb_array *arr = *(const upb_array**)field_mem;

  if (arr == NULL || arr->len == 0) {
    return true;
  }

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

  switch (f->descriptortype) {
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
      VARINT_CASE(uint32_t, *ptr);
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_ENUM:
      VARINT_CASE(int32_t, (int64_t)*ptr);
    case UPB_DESCRIPTOR_TYPE_BOOL:
      VARINT_CASE(bool, *ptr);
    case UPB_DESCRIPTOR_TYPE_SINT32:
      VARINT_CASE(int32_t, upb_zzencode_32(*ptr));
    case UPB_DESCRIPTOR_TYPE_SINT64:
      VARINT_CASE(int64_t, upb_zzencode_64(*ptr));
    case UPB_DESCRIPTOR_TYPE_STRING:
    case UPB_DESCRIPTOR_TYPE_BYTES: {
      upb_strview *start = arr->data;
      upb_strview *ptr = start + arr->len;
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
      const upb_msglayout *subm = m->submsgs[f->submsg_index];
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
      const upb_msglayout *subm = m->submsgs[f->submsg_index];
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
                                   const upb_msglayout *m,
                                   const upb_msglayout_field *f,
                                   bool skip_zero_value) {
#define CASE(ctype, type, wire_type, encodeval) do { \
  ctype val = *(ctype*)field_mem; \
  if (skip_zero_value && val == 0) { \
    return true; \
  } \
  return upb_put_ ## type(e, encodeval) && \
      upb_put_tag(e, f->number, wire_type); \
} while(0)

  switch (f->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
      CASE(double, double, UPB_WIRE_TYPE_64BIT, val);
    case UPB_DESCRIPTOR_TYPE_FLOAT:
      CASE(float, float, UPB_WIRE_TYPE_32BIT, val);
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_UINT64:
      CASE(uint64_t, varint, UPB_WIRE_TYPE_VARINT, val);
    case UPB_DESCRIPTOR_TYPE_UINT32:
      CASE(uint32_t, varint, UPB_WIRE_TYPE_VARINT, val);
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_ENUM:
      CASE(int32_t, varint, UPB_WIRE_TYPE_VARINT, (int64_t)val);
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
      upb_strview view = *(upb_strview*)field_mem;
      if (skip_zero_value && view.size == 0) {
        return true;
      }
      return upb_put_bytes(e, view.data, view.size) &&
          upb_put_varint(e, view.size) &&
          upb_put_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED);
    }
    case UPB_DESCRIPTOR_TYPE_GROUP: {
      size_t size;
      void *submsg = *(void **)field_mem;
      const upb_msglayout *subm = m->submsgs[f->submsg_index];
      if (submsg == NULL) {
        return true;
      }
      return upb_put_tag(e, f->number, UPB_WIRE_TYPE_END_GROUP) &&
          upb_encode_message(e, submsg, subm, &size) &&
          upb_put_tag(e, f->number, UPB_WIRE_TYPE_START_GROUP);
    }
    case UPB_DESCRIPTOR_TYPE_MESSAGE: {
      size_t size;
      void *submsg = *(void **)field_mem;
      const upb_msglayout *subm = m->submsgs[f->submsg_index];
      if (submsg == NULL) {
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

bool upb_encode_message(upb_encstate *e, const char *msg,
                        const upb_msglayout *m, size_t *size) {
  int i;
  size_t pre_len = e->limit - e->ptr;
  const char *unknown;
  size_t unknown_size;

  for (i = m->field_count - 1; i >= 0; i--) {
    const upb_msglayout_field *f = &m->fields[i];

    if (f->label == UPB_LABEL_REPEATED) {
      CHK(upb_encode_array(e, msg + f->offset, m, f));
    } else {
      bool skip_empty = false;
      if (f->presence == 0) {
        /* Proto3 presence. */
        skip_empty = true;
      } else if (f->presence > 0) {
        /* Proto2 presence: hasbit. */
        if (!upb_readhasbit(msg, f)) {
          continue;
        }
      } else {
        /* Field is in a oneof. */
        if (upb_readcase(msg, f) != f->number) {
          continue;
        }
      }
      CHK(upb_encode_scalarfield(e, msg + f->offset, m, f, skip_empty));
    }
  }

  unknown = upb_msg_getunknown(msg, &unknown_size);

  if (unknown) {
    upb_put_bytes(e, unknown, unknown_size);
  }

  *size = (e->limit - e->ptr) - pre_len;
  return true;
}

char *upb_encode(const void *msg, const upb_msglayout *m, upb_arena *arena,
                 size_t *size) {
  upb_encstate e;
  e.alloc = upb_arena_alloc(arena);
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




#define VOIDPTR_AT(msg, ofs) (void*)((char*)msg + (int)ofs)

/* Internal members of a upb_msg.  We can change this without breaking binary
 * compatibility.  We put these before the user's data.  The user's upb_msg*
 * points after the upb_msg_internal. */

/* Used when a message is not extendable. */
typedef struct {
  char *unknown;
  size_t unknown_len;
  size_t unknown_size;
} upb_msg_internal;

/* Used when a message is extendable. */
typedef struct {
  upb_inttable *extdict;
  upb_msg_internal base;
} upb_msg_internal_withext;

static int upb_msg_internalsize(const upb_msglayout *l) {
  return sizeof(upb_msg_internal) - l->extendable * sizeof(void *);
}

static size_t upb_msg_sizeof(const upb_msglayout *l) {
  return l->size + upb_msg_internalsize(l);
}

static upb_msg_internal *upb_msg_getinternal(upb_msg *msg) {
  return VOIDPTR_AT(msg, -sizeof(upb_msg_internal));
}

static const upb_msg_internal *upb_msg_getinternal_const(const upb_msg *msg) {
  return VOIDPTR_AT(msg, -sizeof(upb_msg_internal));
}

static upb_msg_internal_withext *upb_msg_getinternalwithext(
    upb_msg *msg, const upb_msglayout *l) {
  UPB_ASSERT(l->extendable);
  return VOIDPTR_AT(msg, -sizeof(upb_msg_internal_withext));
}

upb_msg *upb_msg_new(const upb_msglayout *l, upb_arena *a) {
  upb_alloc *alloc = upb_arena_alloc(a);
  void *mem = upb_malloc(alloc, upb_msg_sizeof(l));
  upb_msg_internal *in;
  upb_msg *msg;

  if (!mem) {
    return NULL;
  }

  msg = VOIDPTR_AT(mem, upb_msg_internalsize(l));

  /* Initialize normal members. */
  memset(msg, 0, l->size);

  /* Initialize internal members. */
  in = upb_msg_getinternal(msg);
  in->unknown = NULL;
  in->unknown_len = 0;
  in->unknown_size = 0;

  if (l->extendable) {
    upb_msg_getinternalwithext(msg, l)->extdict = NULL;
  }

  return msg;
}

upb_array *upb_array_new(upb_arena *a) {
  upb_array *ret = upb_arena_malloc(a, sizeof(upb_array));

  if (!ret) {
    return NULL;
  }

  ret->data = NULL;
  ret->len = 0;
  ret->size = 0;

  return ret;
}

void upb_msg_addunknown(upb_msg *msg, const char *data, size_t len,
                        upb_arena *arena) {
  upb_msg_internal *in = upb_msg_getinternal(msg);
  if (len > in->unknown_size - in->unknown_len) {
    upb_alloc *alloc = upb_arena_alloc(arena);
    size_t need = in->unknown_size + len;
    size_t newsize = UPB_MAX(in->unknown_size * 2, need);
    in->unknown = upb_realloc(alloc, in->unknown, in->unknown_size, newsize);
    in->unknown_size = newsize;
  }
  memcpy(in->unknown + in->unknown_len, data, len);
  in->unknown_len += len;
}

const char *upb_msg_getunknown(const upb_msg *msg, size_t *len) {
  const upb_msg_internal* in = upb_msg_getinternal_const(msg);
  *len = in->unknown_len;
  return in->unknown;
}

#undef VOIDPTR_AT


#ifdef UPB_MSVC_VSNPRINTF
/* Visual C++ earlier than 2015 doesn't have standard C99 snprintf and
 * vsnprintf. To support them, missing functions are manually implemented
 * using the existing secure functions. */
int msvc_vsnprintf(char* s, size_t n, const char* format, va_list arg) {
  if (!s) {
    return _vscprintf(format, arg);
  }
  int ret = _vsnprintf_s(s, n, _TRUNCATE, format, arg);
  if (ret < 0) {
	ret = _vscprintf(format, arg);
  }
  return ret;
}

int msvc_snprintf(char* s, size_t n, const char* format, ...) {
  va_list arg;
  va_start(arg, format);
  int ret = msvc_vsnprintf(s, n, format, arg);
  va_end(arg);
  return ret;
}
#endif
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
  uint32_t len = (uint32_t) k2.str.len;
  char *str = upb_malloc(a, k2.str.len + sizeof(uint32_t) + 1);
  if (str == NULL) return 0;
  memcpy(str, &len, sizeof(uint32_t));
  memcpy(str + sizeof(uint32_t), k2.str.str, k2.str.len);
  str[sizeof(uint32_t) + k2.str.len] = '\0';
  return (uintptr_t)str;
}

static uint32_t strhash(upb_tabkey key) {
  uint32_t len;
  char *str = upb_tabstr(key, &len);
  return upb_murmur_hash2(str, len, 0);
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

  hash = upb_murmur_hash2(key.str.str, key.str.len, 0);
  insert(&t->t, key, tabkey, v, hash, &strhash, &streql);
  return true;
}

bool upb_strtable_lookup2(const upb_strtable *t, const char *key, size_t len,
                          upb_value *v) {
  uint32_t hash = upb_murmur_hash2(key, len, 0);
  return lookup(&t->t, strkey2(key, len), v, hash, &streql);
}

bool upb_strtable_remove3(upb_strtable *t, const char *key, size_t len,
                         upb_value *val, upb_alloc *alloc) {
  uint32_t hash = upb_murmur_hash2(key, len, 0);
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
  if (!i->t) return true;
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
  i->t = NULL;
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
    int hashsize_lg2 = log2ceil(hash_size);

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
  if (!i->t) return true;
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
  i->t = NULL;
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

#if defined(UPB_UNALIGNED_READS_OK) || defined(__s390x__)
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
uint32_t upb_murmur_hash2(const void *key, size_t len, uint32_t seed) {
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

uint32_t upb_murmur_hash2(const void * key, size_t len, uint32_t seed) {
  const uint32_t m = 0x5bd1e995;
  const int32_t r = 24;
  const uint8_t * data = (const uint8_t *)key;
  uint32_t h = (uint32_t)(seed ^ len);
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

/* upb_status *****************************************************************/

void upb_status_clear(upb_status *status) {
  if (!status) return;
  status->ok = true;
  status->msg[0] = '\0';
}

bool upb_ok(const upb_status *status) { return status->ok; }

const char *upb_status_errmsg(const upb_status *status) { return status->msg; }

void upb_status_seterrmsg(upb_status *status, const char *msg) {
  if (!status) return;
  status->ok = false;
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
  status->ok = false;
  _upb_vsnprintf(status->msg, sizeof(status->msg), fmt, args);
  nullz(status);
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

struct upb_arena {
  /* We implement the allocator interface.
   * This must be the first member of upb_arena! */
  upb_alloc alloc;

  /* Allocator to allocate arena blocks.  We are responsible for freeing these
   * when we are destroyed. */
  upb_alloc *block_alloc;

  size_t bytes_allocated;
  size_t next_block_size;
  size_t max_block_size;

  /* Linked list of blocks.  Points to an arena_block, defined in env.c */
  void *block_head;

  /* Cleanup entries.  Pointer to a cleanup_ent, defined in env.c */
  void *cleanup_head;
};

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

#define upb_alignof(type) offsetof (struct { char c; type member; }, member)

upb_arena *upb_arena_init(void *mem, size_t n, upb_alloc *alloc) {
  const size_t first_block_overhead = sizeof(upb_arena) + sizeof(mem_block);
  upb_arena *a;
  bool owned = false;

  /* Round block size down to alignof(*a) since we will allocate the arena
   * itself at the end. */
  n &= ~(upb_alignof(upb_arena) - 1);

  if (n < first_block_overhead) {
    /* We need to malloc the initial block. */
    n = first_block_overhead + 256;
    owned = true;
    if (!alloc || !(mem = upb_malloc(alloc, n))) {
      return NULL;
    }
  }

  a = (void*)((char*)mem + n - sizeof(*a));
  n -= sizeof(*a);

  a->alloc.func = &upb_arena_doalloc;
  a->block_alloc = &upb_alloc_global;
  a->bytes_allocated = 0;
  a->next_block_size = 256;
  a->max_block_size = 16384;
  a->cleanup_head = NULL;
  a->block_head = NULL;
  a->block_alloc = alloc;

  upb_arena_addblock(a, mem, n, owned);

  return a;
}

#undef upb_alignof

void upb_arena_free(upb_arena *a) {
  cleanup_ent *ent = a->cleanup_head;
  mem_block *block = a->block_head;

  while (ent) {
    ent->cleanup(ent->ud);
    ent = ent->next;
  }

  /* Must do this after running cleanup functions, because this will delete
   * the memory we store our cleanup entries in! */
  while (block) {
    /* Load first since we are deleting block. */
    mem_block *next = block->next;

    if (block->owned) {
      upb_free(a->block_alloc, block);
    }

    block = next;
  }
}

bool upb_arena_addcleanup(upb_arena *a, void *ud, upb_cleanup_func *func) {
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
/* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     google/protobuf/descriptor.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */

#include <stddef.h>


static const upb_msglayout *const google_protobuf_FileDescriptorSet_submsgs[1] = {
  &google_protobuf_FileDescriptorProto_msginit,
};

static const upb_msglayout_field google_protobuf_FileDescriptorSet__fields[1] = {
  {1, UPB_SIZE(0, 0), 0, 0, 11, 3},
};

const upb_msglayout google_protobuf_FileDescriptorSet_msginit = {
  &google_protobuf_FileDescriptorSet_submsgs[0],
  &google_protobuf_FileDescriptorSet__fields[0],
  UPB_SIZE(4, 8), 1, false,
};

static const upb_msglayout *const google_protobuf_FileDescriptorProto_submsgs[6] = {
  &google_protobuf_DescriptorProto_msginit,
  &google_protobuf_EnumDescriptorProto_msginit,
  &google_protobuf_FieldDescriptorProto_msginit,
  &google_protobuf_FileOptions_msginit,
  &google_protobuf_ServiceDescriptorProto_msginit,
  &google_protobuf_SourceCodeInfo_msginit,
};

static const upb_msglayout_field google_protobuf_FileDescriptorProto__fields[12] = {
  {1, UPB_SIZE(4, 8), 1, 0, 9, 1},
  {2, UPB_SIZE(12, 24), 2, 0, 9, 1},
  {3, UPB_SIZE(36, 72), 0, 0, 9, 3},
  {4, UPB_SIZE(40, 80), 0, 0, 11, 3},
  {5, UPB_SIZE(44, 88), 0, 1, 11, 3},
  {6, UPB_SIZE(48, 96), 0, 4, 11, 3},
  {7, UPB_SIZE(52, 104), 0, 2, 11, 3},
  {8, UPB_SIZE(28, 56), 4, 3, 11, 1},
  {9, UPB_SIZE(32, 64), 5, 5, 11, 1},
  {10, UPB_SIZE(56, 112), 0, 0, 5, 3},
  {11, UPB_SIZE(60, 120), 0, 0, 5, 3},
  {12, UPB_SIZE(20, 40), 3, 0, 9, 1},
};

const upb_msglayout google_protobuf_FileDescriptorProto_msginit = {
  &google_protobuf_FileDescriptorProto_submsgs[0],
  &google_protobuf_FileDescriptorProto__fields[0],
  UPB_SIZE(64, 128), 12, false,
};

static const upb_msglayout *const google_protobuf_DescriptorProto_submsgs[8] = {
  &google_protobuf_DescriptorProto_msginit,
  &google_protobuf_DescriptorProto_ExtensionRange_msginit,
  &google_protobuf_DescriptorProto_ReservedRange_msginit,
  &google_protobuf_EnumDescriptorProto_msginit,
  &google_protobuf_FieldDescriptorProto_msginit,
  &google_protobuf_MessageOptions_msginit,
  &google_protobuf_OneofDescriptorProto_msginit,
};

static const upb_msglayout_field google_protobuf_DescriptorProto__fields[10] = {
  {1, UPB_SIZE(4, 8), 1, 0, 9, 1},
  {2, UPB_SIZE(16, 32), 0, 4, 11, 3},
  {3, UPB_SIZE(20, 40), 0, 0, 11, 3},
  {4, UPB_SIZE(24, 48), 0, 3, 11, 3},
  {5, UPB_SIZE(28, 56), 0, 1, 11, 3},
  {6, UPB_SIZE(32, 64), 0, 4, 11, 3},
  {7, UPB_SIZE(12, 24), 2, 5, 11, 1},
  {8, UPB_SIZE(36, 72), 0, 6, 11, 3},
  {9, UPB_SIZE(40, 80), 0, 2, 11, 3},
  {10, UPB_SIZE(44, 88), 0, 0, 9, 3},
};

const upb_msglayout google_protobuf_DescriptorProto_msginit = {
  &google_protobuf_DescriptorProto_submsgs[0],
  &google_protobuf_DescriptorProto__fields[0],
  UPB_SIZE(48, 96), 10, false,
};

static const upb_msglayout *const google_protobuf_DescriptorProto_ExtensionRange_submsgs[1] = {
  &google_protobuf_ExtensionRangeOptions_msginit,
};

static const upb_msglayout_field google_protobuf_DescriptorProto_ExtensionRange__fields[3] = {
  {1, UPB_SIZE(4, 4), 1, 0, 5, 1},
  {2, UPB_SIZE(8, 8), 2, 0, 5, 1},
  {3, UPB_SIZE(12, 16), 3, 0, 11, 1},
};

const upb_msglayout google_protobuf_DescriptorProto_ExtensionRange_msginit = {
  &google_protobuf_DescriptorProto_ExtensionRange_submsgs[0],
  &google_protobuf_DescriptorProto_ExtensionRange__fields[0],
  UPB_SIZE(16, 24), 3, false,
};

static const upb_msglayout_field google_protobuf_DescriptorProto_ReservedRange__fields[2] = {
  {1, UPB_SIZE(4, 4), 1, 0, 5, 1},
  {2, UPB_SIZE(8, 8), 2, 0, 5, 1},
};

const upb_msglayout google_protobuf_DescriptorProto_ReservedRange_msginit = {
  NULL,
  &google_protobuf_DescriptorProto_ReservedRange__fields[0],
  UPB_SIZE(12, 12), 2, false,
};

static const upb_msglayout *const google_protobuf_ExtensionRangeOptions_submsgs[1] = {
  &google_protobuf_UninterpretedOption_msginit,
};

static const upb_msglayout_field google_protobuf_ExtensionRangeOptions__fields[1] = {
  {999, UPB_SIZE(0, 0), 0, 0, 11, 3},
};

const upb_msglayout google_protobuf_ExtensionRangeOptions_msginit = {
  &google_protobuf_ExtensionRangeOptions_submsgs[0],
  &google_protobuf_ExtensionRangeOptions__fields[0],
  UPB_SIZE(4, 8), 1, false,
};

static const upb_msglayout *const google_protobuf_FieldDescriptorProto_submsgs[1] = {
  &google_protobuf_FieldOptions_msginit,
};

static const upb_msglayout_field google_protobuf_FieldDescriptorProto__fields[10] = {
  {1, UPB_SIZE(32, 32), 5, 0, 9, 1},
  {2, UPB_SIZE(40, 48), 6, 0, 9, 1},
  {3, UPB_SIZE(24, 24), 3, 0, 5, 1},
  {4, UPB_SIZE(8, 8), 1, 0, 14, 1},
  {5, UPB_SIZE(16, 16), 2, 0, 14, 1},
  {6, UPB_SIZE(48, 64), 7, 0, 9, 1},
  {7, UPB_SIZE(56, 80), 8, 0, 9, 1},
  {8, UPB_SIZE(72, 112), 10, 0, 11, 1},
  {9, UPB_SIZE(28, 28), 4, 0, 5, 1},
  {10, UPB_SIZE(64, 96), 9, 0, 9, 1},
};

const upb_msglayout google_protobuf_FieldDescriptorProto_msginit = {
  &google_protobuf_FieldDescriptorProto_submsgs[0],
  &google_protobuf_FieldDescriptorProto__fields[0],
  UPB_SIZE(80, 128), 10, false,
};

static const upb_msglayout *const google_protobuf_OneofDescriptorProto_submsgs[1] = {
  &google_protobuf_OneofOptions_msginit,
};

static const upb_msglayout_field google_protobuf_OneofDescriptorProto__fields[2] = {
  {1, UPB_SIZE(4, 8), 1, 0, 9, 1},
  {2, UPB_SIZE(12, 24), 2, 0, 11, 1},
};

const upb_msglayout google_protobuf_OneofDescriptorProto_msginit = {
  &google_protobuf_OneofDescriptorProto_submsgs[0],
  &google_protobuf_OneofDescriptorProto__fields[0],
  UPB_SIZE(16, 32), 2, false,
};

static const upb_msglayout *const google_protobuf_EnumDescriptorProto_submsgs[3] = {
  &google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit,
  &google_protobuf_EnumOptions_msginit,
  &google_protobuf_EnumValueDescriptorProto_msginit,
};

static const upb_msglayout_field google_protobuf_EnumDescriptorProto__fields[5] = {
  {1, UPB_SIZE(4, 8), 1, 0, 9, 1},
  {2, UPB_SIZE(16, 32), 0, 2, 11, 3},
  {3, UPB_SIZE(12, 24), 2, 1, 11, 1},
  {4, UPB_SIZE(20, 40), 0, 0, 11, 3},
  {5, UPB_SIZE(24, 48), 0, 0, 9, 3},
};

const upb_msglayout google_protobuf_EnumDescriptorProto_msginit = {
  &google_protobuf_EnumDescriptorProto_submsgs[0],
  &google_protobuf_EnumDescriptorProto__fields[0],
  UPB_SIZE(32, 64), 5, false,
};

static const upb_msglayout_field google_protobuf_EnumDescriptorProto_EnumReservedRange__fields[2] = {
  {1, UPB_SIZE(4, 4), 1, 0, 5, 1},
  {2, UPB_SIZE(8, 8), 2, 0, 5, 1},
};

const upb_msglayout google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit = {
  NULL,
  &google_protobuf_EnumDescriptorProto_EnumReservedRange__fields[0],
  UPB_SIZE(12, 12), 2, false,
};

static const upb_msglayout *const google_protobuf_EnumValueDescriptorProto_submsgs[1] = {
  &google_protobuf_EnumValueOptions_msginit,
};

static const upb_msglayout_field google_protobuf_EnumValueDescriptorProto__fields[3] = {
  {1, UPB_SIZE(8, 8), 2, 0, 9, 1},
  {2, UPB_SIZE(4, 4), 1, 0, 5, 1},
  {3, UPB_SIZE(16, 24), 3, 0, 11, 1},
};

const upb_msglayout google_protobuf_EnumValueDescriptorProto_msginit = {
  &google_protobuf_EnumValueDescriptorProto_submsgs[0],
  &google_protobuf_EnumValueDescriptorProto__fields[0],
  UPB_SIZE(24, 32), 3, false,
};

static const upb_msglayout *const google_protobuf_ServiceDescriptorProto_submsgs[2] = {
  &google_protobuf_MethodDescriptorProto_msginit,
  &google_protobuf_ServiceOptions_msginit,
};

static const upb_msglayout_field google_protobuf_ServiceDescriptorProto__fields[3] = {
  {1, UPB_SIZE(4, 8), 1, 0, 9, 1},
  {2, UPB_SIZE(16, 32), 0, 0, 11, 3},
  {3, UPB_SIZE(12, 24), 2, 1, 11, 1},
};

const upb_msglayout google_protobuf_ServiceDescriptorProto_msginit = {
  &google_protobuf_ServiceDescriptorProto_submsgs[0],
  &google_protobuf_ServiceDescriptorProto__fields[0],
  UPB_SIZE(24, 48), 3, false,
};

static const upb_msglayout *const google_protobuf_MethodDescriptorProto_submsgs[1] = {
  &google_protobuf_MethodOptions_msginit,
};

static const upb_msglayout_field google_protobuf_MethodDescriptorProto__fields[6] = {
  {1, UPB_SIZE(4, 8), 3, 0, 9, 1},
  {2, UPB_SIZE(12, 24), 4, 0, 9, 1},
  {3, UPB_SIZE(20, 40), 5, 0, 9, 1},
  {4, UPB_SIZE(28, 56), 6, 0, 11, 1},
  {5, UPB_SIZE(1, 1), 1, 0, 8, 1},
  {6, UPB_SIZE(2, 2), 2, 0, 8, 1},
};

const upb_msglayout google_protobuf_MethodDescriptorProto_msginit = {
  &google_protobuf_MethodDescriptorProto_submsgs[0],
  &google_protobuf_MethodDescriptorProto__fields[0],
  UPB_SIZE(32, 64), 6, false,
};

static const upb_msglayout *const google_protobuf_FileOptions_submsgs[1] = {
  &google_protobuf_UninterpretedOption_msginit,
};

static const upb_msglayout_field google_protobuf_FileOptions__fields[21] = {
  {1, UPB_SIZE(28, 32), 11, 0, 9, 1},
  {8, UPB_SIZE(36, 48), 12, 0, 9, 1},
  {9, UPB_SIZE(8, 8), 1, 0, 14, 1},
  {10, UPB_SIZE(16, 16), 2, 0, 8, 1},
  {11, UPB_SIZE(44, 64), 13, 0, 9, 1},
  {16, UPB_SIZE(17, 17), 3, 0, 8, 1},
  {17, UPB_SIZE(18, 18), 4, 0, 8, 1},
  {18, UPB_SIZE(19, 19), 5, 0, 8, 1},
  {20, UPB_SIZE(20, 20), 6, 0, 8, 1},
  {23, UPB_SIZE(21, 21), 7, 0, 8, 1},
  {27, UPB_SIZE(22, 22), 8, 0, 8, 1},
  {31, UPB_SIZE(23, 23), 9, 0, 8, 1},
  {36, UPB_SIZE(52, 80), 14, 0, 9, 1},
  {37, UPB_SIZE(60, 96), 15, 0, 9, 1},
  {39, UPB_SIZE(68, 112), 16, 0, 9, 1},
  {40, UPB_SIZE(76, 128), 17, 0, 9, 1},
  {41, UPB_SIZE(84, 144), 18, 0, 9, 1},
  {42, UPB_SIZE(24, 24), 10, 0, 8, 1},
  {44, UPB_SIZE(92, 160), 19, 0, 9, 1},
  {45, UPB_SIZE(100, 176), 20, 0, 9, 1},
  {999, UPB_SIZE(108, 192), 0, 0, 11, 3},
};

const upb_msglayout google_protobuf_FileOptions_msginit = {
  &google_protobuf_FileOptions_submsgs[0],
  &google_protobuf_FileOptions__fields[0],
  UPB_SIZE(112, 208), 21, false,
};

static const upb_msglayout *const google_protobuf_MessageOptions_submsgs[1] = {
  &google_protobuf_UninterpretedOption_msginit,
};

static const upb_msglayout_field google_protobuf_MessageOptions__fields[5] = {
  {1, UPB_SIZE(1, 1), 1, 0, 8, 1},
  {2, UPB_SIZE(2, 2), 2, 0, 8, 1},
  {3, UPB_SIZE(3, 3), 3, 0, 8, 1},
  {7, UPB_SIZE(4, 4), 4, 0, 8, 1},
  {999, UPB_SIZE(8, 8), 0, 0, 11, 3},
};

const upb_msglayout google_protobuf_MessageOptions_msginit = {
  &google_protobuf_MessageOptions_submsgs[0],
  &google_protobuf_MessageOptions__fields[0],
  UPB_SIZE(12, 16), 5, false,
};

static const upb_msglayout *const google_protobuf_FieldOptions_submsgs[1] = {
  &google_protobuf_UninterpretedOption_msginit,
};

static const upb_msglayout_field google_protobuf_FieldOptions__fields[7] = {
  {1, UPB_SIZE(8, 8), 1, 0, 14, 1},
  {2, UPB_SIZE(24, 24), 3, 0, 8, 1},
  {3, UPB_SIZE(25, 25), 4, 0, 8, 1},
  {5, UPB_SIZE(26, 26), 5, 0, 8, 1},
  {6, UPB_SIZE(16, 16), 2, 0, 14, 1},
  {10, UPB_SIZE(27, 27), 6, 0, 8, 1},
  {999, UPB_SIZE(28, 32), 0, 0, 11, 3},
};

const upb_msglayout google_protobuf_FieldOptions_msginit = {
  &google_protobuf_FieldOptions_submsgs[0],
  &google_protobuf_FieldOptions__fields[0],
  UPB_SIZE(32, 40), 7, false,
};

static const upb_msglayout *const google_protobuf_OneofOptions_submsgs[1] = {
  &google_protobuf_UninterpretedOption_msginit,
};

static const upb_msglayout_field google_protobuf_OneofOptions__fields[1] = {
  {999, UPB_SIZE(0, 0), 0, 0, 11, 3},
};

const upb_msglayout google_protobuf_OneofOptions_msginit = {
  &google_protobuf_OneofOptions_submsgs[0],
  &google_protobuf_OneofOptions__fields[0],
  UPB_SIZE(4, 8), 1, false,
};

static const upb_msglayout *const google_protobuf_EnumOptions_submsgs[1] = {
  &google_protobuf_UninterpretedOption_msginit,
};

static const upb_msglayout_field google_protobuf_EnumOptions__fields[3] = {
  {2, UPB_SIZE(1, 1), 1, 0, 8, 1},
  {3, UPB_SIZE(2, 2), 2, 0, 8, 1},
  {999, UPB_SIZE(4, 8), 0, 0, 11, 3},
};

const upb_msglayout google_protobuf_EnumOptions_msginit = {
  &google_protobuf_EnumOptions_submsgs[0],
  &google_protobuf_EnumOptions__fields[0],
  UPB_SIZE(8, 16), 3, false,
};

static const upb_msglayout *const google_protobuf_EnumValueOptions_submsgs[1] = {
  &google_protobuf_UninterpretedOption_msginit,
};

static const upb_msglayout_field google_protobuf_EnumValueOptions__fields[2] = {
  {1, UPB_SIZE(1, 1), 1, 0, 8, 1},
  {999, UPB_SIZE(4, 8), 0, 0, 11, 3},
};

const upb_msglayout google_protobuf_EnumValueOptions_msginit = {
  &google_protobuf_EnumValueOptions_submsgs[0],
  &google_protobuf_EnumValueOptions__fields[0],
  UPB_SIZE(8, 16), 2, false,
};

static const upb_msglayout *const google_protobuf_ServiceOptions_submsgs[1] = {
  &google_protobuf_UninterpretedOption_msginit,
};

static const upb_msglayout_field google_protobuf_ServiceOptions__fields[2] = {
  {33, UPB_SIZE(1, 1), 1, 0, 8, 1},
  {999, UPB_SIZE(4, 8), 0, 0, 11, 3},
};

const upb_msglayout google_protobuf_ServiceOptions_msginit = {
  &google_protobuf_ServiceOptions_submsgs[0],
  &google_protobuf_ServiceOptions__fields[0],
  UPB_SIZE(8, 16), 2, false,
};

static const upb_msglayout *const google_protobuf_MethodOptions_submsgs[1] = {
  &google_protobuf_UninterpretedOption_msginit,
};

static const upb_msglayout_field google_protobuf_MethodOptions__fields[3] = {
  {33, UPB_SIZE(16, 16), 2, 0, 8, 1},
  {34, UPB_SIZE(8, 8), 1, 0, 14, 1},
  {999, UPB_SIZE(20, 24), 0, 0, 11, 3},
};

const upb_msglayout google_protobuf_MethodOptions_msginit = {
  &google_protobuf_MethodOptions_submsgs[0],
  &google_protobuf_MethodOptions__fields[0],
  UPB_SIZE(24, 32), 3, false,
};

static const upb_msglayout *const google_protobuf_UninterpretedOption_submsgs[1] = {
  &google_protobuf_UninterpretedOption_NamePart_msginit,
};

static const upb_msglayout_field google_protobuf_UninterpretedOption__fields[7] = {
  {2, UPB_SIZE(56, 80), 0, 0, 11, 3},
  {3, UPB_SIZE(32, 32), 4, 0, 9, 1},
  {4, UPB_SIZE(8, 8), 1, 0, 4, 1},
  {5, UPB_SIZE(16, 16), 2, 0, 3, 1},
  {6, UPB_SIZE(24, 24), 3, 0, 1, 1},
  {7, UPB_SIZE(40, 48), 5, 0, 12, 1},
  {8, UPB_SIZE(48, 64), 6, 0, 9, 1},
};

const upb_msglayout google_protobuf_UninterpretedOption_msginit = {
  &google_protobuf_UninterpretedOption_submsgs[0],
  &google_protobuf_UninterpretedOption__fields[0],
  UPB_SIZE(64, 96), 7, false,
};

static const upb_msglayout_field google_protobuf_UninterpretedOption_NamePart__fields[2] = {
  {1, UPB_SIZE(4, 8), 2, 0, 9, 2},
  {2, UPB_SIZE(1, 1), 1, 0, 8, 2},
};

const upb_msglayout google_protobuf_UninterpretedOption_NamePart_msginit = {
  NULL,
  &google_protobuf_UninterpretedOption_NamePart__fields[0],
  UPB_SIZE(16, 32), 2, false,
};

static const upb_msglayout *const google_protobuf_SourceCodeInfo_submsgs[1] = {
  &google_protobuf_SourceCodeInfo_Location_msginit,
};

static const upb_msglayout_field google_protobuf_SourceCodeInfo__fields[1] = {
  {1, UPB_SIZE(0, 0), 0, 0, 11, 3},
};

const upb_msglayout google_protobuf_SourceCodeInfo_msginit = {
  &google_protobuf_SourceCodeInfo_submsgs[0],
  &google_protobuf_SourceCodeInfo__fields[0],
  UPB_SIZE(4, 8), 1, false,
};

static const upb_msglayout_field google_protobuf_SourceCodeInfo_Location__fields[5] = {
  {1, UPB_SIZE(20, 40), 0, 0, 5, 3},
  {2, UPB_SIZE(24, 48), 0, 0, 5, 3},
  {3, UPB_SIZE(4, 8), 1, 0, 9, 1},
  {4, UPB_SIZE(12, 24), 2, 0, 9, 1},
  {6, UPB_SIZE(28, 56), 0, 0, 9, 3},
};

const upb_msglayout google_protobuf_SourceCodeInfo_Location_msginit = {
  NULL,
  &google_protobuf_SourceCodeInfo_Location__fields[0],
  UPB_SIZE(32, 64), 5, false,
};

static const upb_msglayout *const google_protobuf_GeneratedCodeInfo_submsgs[1] = {
  &google_protobuf_GeneratedCodeInfo_Annotation_msginit,
};

static const upb_msglayout_field google_protobuf_GeneratedCodeInfo__fields[1] = {
  {1, UPB_SIZE(0, 0), 0, 0, 11, 3},
};

const upb_msglayout google_protobuf_GeneratedCodeInfo_msginit = {
  &google_protobuf_GeneratedCodeInfo_submsgs[0],
  &google_protobuf_GeneratedCodeInfo__fields[0],
  UPB_SIZE(4, 8), 1, false,
};

static const upb_msglayout_field google_protobuf_GeneratedCodeInfo_Annotation__fields[4] = {
  {1, UPB_SIZE(20, 32), 0, 0, 5, 3},
  {2, UPB_SIZE(12, 16), 3, 0, 9, 1},
  {3, UPB_SIZE(4, 4), 1, 0, 5, 1},
  {4, UPB_SIZE(8, 8), 2, 0, 5, 1},
};

const upb_msglayout google_protobuf_GeneratedCodeInfo_Annotation_msginit = {
  NULL,
  &google_protobuf_GeneratedCodeInfo_Annotation__fields[0],
  UPB_SIZE(24, 48), 4, false,
};




#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
  size_t len;
  char str[1];  /* Null-terminated string data follows. */
} str_t;

static str_t *newstr(upb_alloc *alloc, const char *data, size_t len) {
  str_t *ret = upb_malloc(alloc, sizeof(*ret) + len);
  if (!ret) return NULL;
  ret->len = len;
  memcpy(ret->str, data, len);
  ret->str[len] = '\0';
  return ret;
}

struct upb_fielddef {
  const upb_filedef *file;
  const upb_msgdef *msgdef;
  const char *full_name;
  union {
    int64_t sint;
    uint64_t uint;
    double dbl;
    float flt;
    bool boolean;
    str_t *str;
  } defaultval;
  const upb_oneofdef *oneof;
  union {
    const upb_msgdef *msgdef;
    const upb_enumdef *enumdef;
    const google_protobuf_FieldDescriptorProto *unresolved;
  } sub;
  uint32_t number_;
  uint32_t index_;
  uint32_t selector_base;  /* Used to index into a upb::Handlers table. */
  bool is_extension_;
  bool lazy_;
  bool packed_;
  upb_descriptortype_t type_;
  upb_label_t label_;
};

struct upb_msgdef {
  const upb_filedef *file;
  const char *full_name;
  uint32_t selector_count;
  uint32_t submsg_field_count;

  /* Tables for looking up fields by number and name. */
  upb_inttable itof;
  upb_strtable ntof;

  const upb_fielddef *fields;
  const upb_oneofdef *oneofs;
  int field_count;
  int oneof_count;

  /* Is this a map-entry message? */
  bool map_entry;
  upb_wellknowntype_t well_known_type;

  /* TODO(haberman): proper extension ranges (there can be multiple). */
};

struct upb_enumdef {
  const upb_filedef *file;
  const char *full_name;
  upb_strtable ntoi;
  upb_inttable iton;
  int32_t defaultval;
};

struct upb_oneofdef {
  const upb_msgdef *parent;
  const char *full_name;
  uint32_t index;
  upb_strtable ntof;
  upb_inttable itof;
};

struct upb_filedef {
  const char *name;
  const char *package;
  const char *phpprefix;
  const char *phpnamespace;
  upb_syntax_t syntax;

  const upb_filedef **deps;
  const upb_msgdef *msgs;
  const upb_enumdef *enums;
  const upb_fielddef *exts;

  int dep_count;
  int msg_count;
  int enum_count;
  int ext_count;
};

struct upb_symtab {
  upb_arena *arena;
  upb_strtable syms;  /* full_name -> packed def ptr */
  upb_strtable files;  /* file_name -> upb_filedef* */
};

/* Inside a symtab we store tagged pointers to specific def types. */
typedef enum {
  UPB_DEFTYPE_MSG = 0,
  UPB_DEFTYPE_ENUM = 1,
  UPB_DEFTYPE_FIELD = 2,
  UPB_DEFTYPE_ONEOF = 3
} upb_deftype_t;

static const void *unpack_def(upb_value v, upb_deftype_t type) {
  uintptr_t num = (uintptr_t)upb_value_getconstptr(v);
  return (num & 3) == type ? (const void*)(num & ~3) : NULL;
}

static upb_value pack_def(const void *ptr, upb_deftype_t type) {
  uintptr_t num = (uintptr_t)ptr | type;
  return upb_value_constptr((const void*)num);
}

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

static bool upb_isident(upb_strview name, bool full, upb_status *s) {
  const char *str = name.data;
  size_t len = name.size;
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

static const char *shortdefname(const char *fullname) {
  const char *p;

  if (fullname == NULL) {
    return NULL;
  } else if ((p = strrchr(fullname, '.')) == NULL) {
    /* No '.' in the name, return the full string. */
    return fullname;
  } else {
    /* Return one past the last '.'. */
    return p + 1;
  }
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

/* A few implementation details of handlers.  We put these here to avoid
 * a def -> handlers dependency. */

#define UPB_STATIC_SELECTOR_COUNT 3  /* Warning: also in upb/handlers.h. */

static uint32_t upb_handlers_selectorbaseoffset(const upb_fielddef *f) {
  return upb_fielddef_isseq(f) ? 2 : 0;
}

static uint32_t upb_handlers_selectorcount(const upb_fielddef *f) {
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
    upb_status_setoom(s);
    return false;
  }

  m->submsg_field_count = 0;
  for(i = 0, upb_msg_field_begin(&j, m);
      !upb_msg_field_done(&j);
      upb_msg_field_next(&j), i++) {
    upb_fielddef *f = upb_msg_iter_field(&j);
    UPB_ASSERT(f->msgdef == m);
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

  for(upb_msg_oneof_begin(&k, m), i = 0;
      !upb_msg_oneof_done(&k);
      upb_msg_oneof_next(&k), i++) {
    upb_oneofdef *o = (upb_oneofdef*)upb_msg_iter_oneof(&k);
    o->index = i;
  }

  upb_gfree(fields);
  return true;
}

static void assign_msg_wellknowntype(upb_msgdef *m) {
  const char *name = upb_msgdef_fullname(m);
  if (name == NULL) {
    m->well_known_type = UPB_WELLKNOWN_UNSPECIFIED;
    return;
  }
  if (!strcmp(name, "google.protobuf.Any")) {
    m->well_known_type = UPB_WELLKNOWN_ANY;
  } else if (!strcmp(name, "google.protobuf.FieldMask")) {
    m->well_known_type = UPB_WELLKNOWN_FIELDMASK;
  } else if (!strcmp(name, "google.protobuf.Duration")) {
    m->well_known_type = UPB_WELLKNOWN_DURATION;
  } else if (!strcmp(name, "google.protobuf.Timestamp")) {
    m->well_known_type = UPB_WELLKNOWN_TIMESTAMP;
  } else if (!strcmp(name, "google.protobuf.DoubleValue")) {
    m->well_known_type = UPB_WELLKNOWN_DOUBLEVALUE;
  } else if (!strcmp(name, "google.protobuf.FloatValue")) {
    m->well_known_type = UPB_WELLKNOWN_FLOATVALUE;
  } else if (!strcmp(name, "google.protobuf.Int64Value")) {
    m->well_known_type = UPB_WELLKNOWN_INT64VALUE;
  } else if (!strcmp(name, "google.protobuf.UInt64Value")) {
    m->well_known_type = UPB_WELLKNOWN_UINT64VALUE;
  } else if (!strcmp(name, "google.protobuf.Int32Value")) {
    m->well_known_type = UPB_WELLKNOWN_INT32VALUE;
  } else if (!strcmp(name, "google.protobuf.UInt32Value")) {
    m->well_known_type = UPB_WELLKNOWN_UINT32VALUE;
  } else if (!strcmp(name, "google.protobuf.BoolValue")) {
    m->well_known_type = UPB_WELLKNOWN_BOOLVALUE;
  } else if (!strcmp(name, "google.protobuf.StringValue")) {
    m->well_known_type = UPB_WELLKNOWN_STRINGVALUE;
  } else if (!strcmp(name, "google.protobuf.BytesValue")) {
    m->well_known_type = UPB_WELLKNOWN_BYTESVALUE;
  } else if (!strcmp(name, "google.protobuf.Value")) {
    m->well_known_type = UPB_WELLKNOWN_VALUE;
  } else if (!strcmp(name, "google.protobuf.ListValue")) {
    m->well_known_type = UPB_WELLKNOWN_LISTVALUE;
  } else if (!strcmp(name, "google.protobuf.Struct")) {
    m->well_known_type = UPB_WELLKNOWN_STRUCT;
  } else {
    m->well_known_type = UPB_WELLKNOWN_UNSPECIFIED;
  }
}


/* upb_enumdef ****************************************************************/

const char *upb_enumdef_fullname(const upb_enumdef *e) {
  return e->full_name;
}

const char *upb_enumdef_name(const upb_enumdef *e) {
  return shortdefname(e->full_name);
}

const upb_filedef *upb_enumdef_file(const upb_enumdef *e) {
  return e->file;
}

int32_t upb_enumdef_default(const upb_enumdef *e) {
  UPB_ASSERT(upb_enumdef_iton(e, e->defaultval));
  return e->defaultval;
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

const char *upb_fielddef_fullname(const upb_fielddef *f) {
  return f->full_name;
}

upb_fieldtype_t upb_fielddef_type(const upb_fielddef *f) {
  switch (f->type_) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
      return UPB_TYPE_DOUBLE;
    case UPB_DESCRIPTOR_TYPE_FLOAT:
      return UPB_TYPE_FLOAT;
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_SINT64:
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
      return UPB_TYPE_INT64;
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
    case UPB_DESCRIPTOR_TYPE_SINT32:
      return UPB_TYPE_INT32;
    case UPB_DESCRIPTOR_TYPE_UINT64:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
      return UPB_TYPE_UINT64;
    case UPB_DESCRIPTOR_TYPE_UINT32:
    case UPB_DESCRIPTOR_TYPE_FIXED32:
      return UPB_TYPE_UINT32;
    case UPB_DESCRIPTOR_TYPE_ENUM:
      return UPB_TYPE_ENUM;
    case UPB_DESCRIPTOR_TYPE_BOOL:
      return UPB_TYPE_BOOL;
    case UPB_DESCRIPTOR_TYPE_STRING:
      return UPB_TYPE_STRING;
    case UPB_DESCRIPTOR_TYPE_BYTES:
      return UPB_TYPE_BYTES;
    case UPB_DESCRIPTOR_TYPE_GROUP:
    case UPB_DESCRIPTOR_TYPE_MESSAGE:
      return UPB_TYPE_MESSAGE;
  }
  UPB_UNREACHABLE();
}

upb_descriptortype_t upb_fielddef_descriptortype(const upb_fielddef *f) {
  return f->type_;
}

uint32_t upb_fielddef_index(const upb_fielddef *f) {
  return f->index_;
}

upb_label_t upb_fielddef_label(const upb_fielddef *f) {
  return f->label_;
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
  return shortdefname(f->full_name);
}

uint32_t upb_fielddef_selectorbase(const upb_fielddef *f) {
  return f->selector_base;
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
  return f->msgdef;
}

const upb_oneofdef *upb_fielddef_containingoneof(const upb_fielddef *f) {
  return f->oneof;
}

static void chkdefaulttype(const upb_fielddef *f, int ctype) {
  UPB_UNUSED(f);
  UPB_UNUSED(ctype);
}

int64_t upb_fielddef_defaultint64(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_INT64);
  return f->defaultval.sint;
}

int32_t upb_fielddef_defaultint32(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_INT32);
  return f->defaultval.sint;
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
  return f->defaultval.boolean;
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
  str_t *str = f->defaultval.str;
  UPB_ASSERT(upb_fielddef_type(f) == UPB_TYPE_STRING ||
         upb_fielddef_type(f) == UPB_TYPE_BYTES ||
         upb_fielddef_type(f) == UPB_TYPE_ENUM);
  if (str) {
    if (len) *len = str->len;
    return str->str;
  } else {
    if (len) *len = 0;
    return NULL;
  }
}

const upb_msgdef *upb_fielddef_msgsubdef(const upb_fielddef *f) {
  UPB_ASSERT(upb_fielddef_type(f) == UPB_TYPE_MESSAGE);
  return f->sub.msgdef;
}

const upb_enumdef *upb_fielddef_enumsubdef(const upb_fielddef *f) {
  UPB_ASSERT(upb_fielddef_type(f) == UPB_TYPE_ENUM);
  return f->sub.enumdef;
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

bool upb_fielddef_hassubdef(const upb_fielddef *f) {
  return upb_fielddef_issubmsg(f) || upb_fielddef_type(f) == UPB_TYPE_ENUM;
}

bool upb_fielddef_haspresence(const upb_fielddef *f) {
  if (upb_fielddef_isseq(f)) return false;
  if (upb_fielddef_issubmsg(f)) return true;
  return f->file->syntax == UPB_SYNTAX_PROTO2;
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

const char *upb_msgdef_fullname(const upb_msgdef *m) {
  return m->full_name;
}

const upb_filedef *upb_msgdef_file(const upb_msgdef *m) {
  return m->file;
}

const char *upb_msgdef_name(const upb_msgdef *m) {
  return shortdefname(m->full_name);
}

upb_syntax_t upb_msgdef_syntax(const upb_msgdef *m) {
  return m->file->syntax;
}

size_t upb_msgdef_selectorcount(const upb_msgdef *m) {
  return m->selector_count;
}

uint32_t upb_msgdef_submsgfieldcount(const upb_msgdef *m) {
  return m->submsg_field_count;
}

const upb_fielddef *upb_msgdef_itof(const upb_msgdef *m, uint32_t i) {
  upb_value val;
  return upb_inttable_lookup32(&m->itof, i, &val) ?
      upb_value_getconstptr(val) : NULL;
}

const upb_fielddef *upb_msgdef_ntof(const upb_msgdef *m, const char *name,
                                    size_t len) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return NULL;
  }

  return unpack_def(val, UPB_DEFTYPE_FIELD);
}

const upb_oneofdef *upb_msgdef_ntoo(const upb_msgdef *m, const char *name,
                                    size_t len) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return NULL;
  }

  return unpack_def(val, UPB_DEFTYPE_ONEOF);
}

bool upb_msgdef_lookupname(const upb_msgdef *m, const char *name, size_t len,
                           const upb_fielddef **f, const upb_oneofdef **o) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return false;
  }

  *o = unpack_def(val, UPB_DEFTYPE_ONEOF);
  *f = unpack_def(val, UPB_DEFTYPE_FIELD);
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

bool upb_msgdef_mapentry(const upb_msgdef *m) {
  return m->map_entry;
}

upb_wellknowntype_t upb_msgdef_wellknowntype(const upb_msgdef *m) {
  return m->well_known_type;
}

bool upb_msgdef_isnumberwrapper(const upb_msgdef *m) {
  upb_wellknowntype_t type = upb_msgdef_wellknowntype(m);
  return type >= UPB_WELLKNOWN_DOUBLEVALUE &&
         type <= UPB_WELLKNOWN_UINT32VALUE;
}

void upb_msg_field_begin(upb_msg_field_iter *iter, const upb_msgdef *m) {
  upb_inttable_begin(iter, &m->itof);
}

void upb_msg_field_next(upb_msg_field_iter *iter) { upb_inttable_next(iter); }

bool upb_msg_field_done(const upb_msg_field_iter *iter) {
  return upb_inttable_done(iter);
}

upb_fielddef *upb_msg_iter_field(const upb_msg_field_iter *iter) {
  return (upb_fielddef *)upb_value_getconstptr(upb_inttable_iter_value(iter));
}

void upb_msg_field_iter_setdone(upb_msg_field_iter *iter) {
  upb_inttable_iter_setdone(iter);
}

bool upb_msg_field_iter_isequal(const upb_msg_field_iter * iter1,
                                const upb_msg_field_iter * iter2) {
  return upb_inttable_iter_isequal(iter1, iter2);
}

void upb_msg_oneof_begin(upb_msg_oneof_iter *iter, const upb_msgdef *m) {
  upb_strtable_begin(iter, &m->ntof);
  /* We need to skip past any initial fields. */
  while (!upb_strtable_done(iter) &&
         !unpack_def(upb_strtable_iter_value(iter), UPB_DEFTYPE_ONEOF)) {
    upb_strtable_next(iter);
  }
}

void upb_msg_oneof_next(upb_msg_oneof_iter *iter) {
  /* We need to skip past fields to return only oneofs. */
  do {
    upb_strtable_next(iter);
  } while (!upb_strtable_done(iter) &&
           !unpack_def(upb_strtable_iter_value(iter), UPB_DEFTYPE_ONEOF));
}

bool upb_msg_oneof_done(const upb_msg_oneof_iter *iter) {
  return upb_strtable_done(iter);
}

const upb_oneofdef *upb_msg_iter_oneof(const upb_msg_oneof_iter *iter) {
  return unpack_def(upb_strtable_iter_value(iter), UPB_DEFTYPE_ONEOF);
}

void upb_msg_oneof_iter_setdone(upb_msg_oneof_iter *iter) {
  upb_strtable_iter_setdone(iter);
}

bool upb_msg_oneof_iter_isequal(const upb_msg_oneof_iter *iter1,
                                const upb_msg_oneof_iter *iter2) {
  return upb_strtable_iter_isequal(iter1, iter2);
}

/* upb_oneofdef ***************************************************************/

const char *upb_oneofdef_name(const upb_oneofdef *o) {
  return shortdefname(o->full_name);
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
  return (upb_fielddef *)upb_value_getconstptr(upb_inttable_iter_value(iter));
}

void upb_oneof_iter_setdone(upb_oneof_iter *iter) {
  upb_inttable_iter_setdone(iter);
}

/* Code to build defs from descriptor protos. *********************************/

/* There is a question of how much validation to do here.  It will be difficult
 * to perfectly match the amount of validation performed by proto2.  But since
 * this code is used to directly build defs from Ruby (for example) we do need
 * to validate important constraints like uniqueness of names and numbers. */

#define CHK(x) if (!(x)) { return false; }
#define CHK_OOM(x) if (!(x)) { upb_status_setoom(ctx->status); return false; }

typedef struct {
  const upb_symtab *symtab;
  upb_filedef *file;  /* File we are building. */
  upb_alloc *alloc;    /* Allocate defs here. */
  upb_alloc *tmp;      /* Alloc for addtab and any other tmp data. */
  upb_strtable *addtab;  /* full_name -> packed def ptr for new defs. */
  upb_status *status;  /* Record errors here. */
} symtab_addctx;

static char* strviewdup(const symtab_addctx *ctx, upb_strview view) {
  return upb_strdup2(view.data, view.size, ctx->alloc);
}

static bool streql2(const char *a, size_t n, const char *b) {
  return n == strlen(b) && memcmp(a, b, n) == 0;
}

static bool streql_view(upb_strview view, const char *b) {
  return streql2(view.data, view.size, b);
}

static const char *makefullname(const symtab_addctx *ctx, const char *prefix,
                                upb_strview name) {
  if (prefix) {
    /* ret = prefix + '.' + name; */
    size_t n = strlen(prefix);
    char *ret = upb_malloc(ctx->alloc, n + name.size + 2);
    CHK_OOM(ret);
    strcpy(ret, prefix);
    ret[n] = '.';
    memcpy(&ret[n + 1], name.data, name.size);
    ret[n + 1 + name.size] = '\0';
    return ret;
  } else {
    return strviewdup(ctx, name);
  }
}

static bool symtab_add(const symtab_addctx *ctx, const char *name,
                       upb_value v) {
  upb_value tmp;
  if (upb_strtable_lookup(ctx->addtab, name, &tmp) ||
      upb_strtable_lookup(&ctx->symtab->syms, name, &tmp)) {
    upb_status_seterrf(ctx->status, "duplicate symbol '%s'", name);
    return false;
  }

  CHK_OOM(upb_strtable_insert3(ctx->addtab, name, strlen(name), v, ctx->tmp));
  return true;
}

/* Given a symbol and the base symbol inside which it is defined, find the
 * symbol's definition in t. */
static bool resolvename(const upb_strtable *t, const upb_fielddef *f,
                        const char *base, upb_strview sym,
                        upb_deftype_t type, upb_status *status,
                        const void **def) {
  if(sym.size == 0) return NULL;
  if(sym.data[0] == '.') {
    /* Symbols starting with '.' are absolute, so we do a single lookup.
     * Slice to omit the leading '.' */
    upb_value v;
    if (!upb_strtable_lookup2(t, sym.data + 1, sym.size - 1, &v)) {
      return false;
    }

    *def = unpack_def(v, type);

    if (!*def) {
      upb_status_seterrf(status,
                         "type mismatch when resolving field %s, name %s",
                         f->full_name, sym.data);
      return false;
    }

    return true;
  } else {
    /* Remove components from base until we find an entry or run out.
     * TODO: This branch is totally broken, but currently not used. */
    (void)base;
    UPB_ASSERT(false);
    return false;
  }
}

const void *symtab_resolve(const symtab_addctx *ctx, const upb_fielddef *f,
                           const char *base, upb_strview sym,
                           upb_deftype_t type) {
  const void *ret;
  if (!resolvename(ctx->addtab, f, base, sym, type, ctx->status, &ret) &&
      !resolvename(&ctx->symtab->syms, f, base, sym, type, ctx->status, &ret)) {
    if (upb_ok(ctx->status)) {
      upb_status_seterrf(ctx->status, "couldn't resolve name '%s'", sym.data);
    }
    return false;
  }
  return ret;
}

static bool create_oneofdef(
    const symtab_addctx *ctx, upb_msgdef *m,
    const google_protobuf_OneofDescriptorProto *oneof_proto) {
  upb_oneofdef *o;
  upb_strview name = google_protobuf_OneofDescriptorProto_name(oneof_proto);
  upb_value v;

  o = (upb_oneofdef*)&m->oneofs[m->oneof_count++];
  o->parent = m;
  o->full_name = makefullname(ctx, m->full_name, name);

  v = pack_def(o, UPB_DEFTYPE_ONEOF);
  CHK_OOM(symtab_add(ctx, o->full_name, v));
  CHK_OOM(upb_strtable_insert3(&m->ntof, name.data, name.size, v, ctx->alloc));

  CHK_OOM(upb_inttable_init2(&o->itof, UPB_CTYPE_CONSTPTR, ctx->alloc));
  CHK_OOM(upb_strtable_init2(&o->ntof, UPB_CTYPE_CONSTPTR, ctx->alloc));

  return true;
}

static bool parse_default(const symtab_addctx *ctx, const char *str, size_t len,
                          upb_fielddef *f) {
  char *end;
  char nullz[64];
  errno = 0;

  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
    case UPB_TYPE_DOUBLE:
    case UPB_TYPE_FLOAT:
      /* Standard C number parsing functions expect null-terminated strings. */
      if (len >= sizeof(nullz) - 1) {
        return false;
      }
      memcpy(nullz, str, len);
      nullz[len] = '\0';
      str = nullz;
      break;
    default:
      break;
  }

  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32: {
      long val = strtol(str, &end, 0);
      CHK(val <= INT32_MAX && val >= INT32_MIN && errno != ERANGE && !*end);
      f->defaultval.sint = val;
      break;
    }
    case UPB_TYPE_ENUM: {
      const upb_enumdef *e = f->sub.enumdef;
      int32_t val;
      CHK(upb_enumdef_ntoi(e, str, len, &val));
      f->defaultval.sint = val;
      break;
    }
    case UPB_TYPE_INT64: {
      /* XXX: Need to write our own strtoll, since it's not available in c89. */
      long long val = strtol(str, &end, 0);
      CHK(val <= INT64_MAX && val >= INT64_MIN && errno != ERANGE && !*end);
      f->defaultval.sint = val;
      break;
    }
    case UPB_TYPE_UINT32: {
      unsigned long val = strtoul(str, &end, 0);
      CHK(val <= UINT32_MAX && errno != ERANGE && !*end);
      f->defaultval.uint = val;
      break;
    }
    case UPB_TYPE_UINT64: {
      /* XXX: Need to write our own strtoull, since it's not available in c89. */
      unsigned long long val = strtoul(str, &end, 0);
      CHK(val <= UINT64_MAX && errno != ERANGE && !*end);
      f->defaultval.uint = val;
      break;
    }
    case UPB_TYPE_DOUBLE: {
      double val = strtod(str, &end);
      CHK(errno != ERANGE && !*end);
      f->defaultval.dbl = val;
      break;
    }
    case UPB_TYPE_FLOAT: {
      /* XXX: Need to write our own strtof, since it's not available in c89. */
      float val = strtod(str, &end);
      CHK(errno != ERANGE && !*end);
      f->defaultval.flt = val;
      break;
    }
    case UPB_TYPE_BOOL: {
      if (streql2(str, len, "false")) {
        f->defaultval.boolean = false;
      } else if (streql2(str, len, "true")) {
        f->defaultval.boolean = true;
      } else {
        return false;
      }
      break;
    }
    case UPB_TYPE_STRING:
      f->defaultval.str = newstr(ctx->alloc, str, len);
      break;
    case UPB_TYPE_BYTES:
      /* XXX: need to interpret the C-escaped value. */
      f->defaultval.str = newstr(ctx->alloc, str, len);
      break;
    case UPB_TYPE_MESSAGE:
      /* Should not have a default value. */
      return false;
  }
  return true;
}

static void set_default_default(const symtab_addctx *ctx, upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_ENUM:
      f->defaultval.sint = 0;
      break;
    case UPB_TYPE_UINT64:
    case UPB_TYPE_UINT32:
      f->defaultval.uint = 0;
      break;
    case UPB_TYPE_DOUBLE:
    case UPB_TYPE_FLOAT:
      f->defaultval.dbl = 0;
      break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      f->defaultval.str = newstr(ctx->alloc, NULL, 0);
      break;
    case UPB_TYPE_BOOL:
      f->defaultval.boolean = false;
      break;
    case UPB_TYPE_MESSAGE:
      break;
  }
}

static bool create_fielddef(
    const symtab_addctx *ctx, const char *prefix, upb_msgdef *m,
    const google_protobuf_FieldDescriptorProto *field_proto) {
  upb_alloc *alloc = ctx->alloc;
  upb_fielddef *f;
  const google_protobuf_FieldOptions *options;
  upb_strview name;
  const char *full_name;
  const char *shortname;
  uint32_t field_number;

  if (!google_protobuf_FieldDescriptorProto_has_name(field_proto)) {
    upb_status_seterrmsg(ctx->status, "field has no name");
    return false;
  }

  name = google_protobuf_FieldDescriptorProto_name(field_proto);
  CHK(upb_isident(name, false, ctx->status));
  full_name = makefullname(ctx, prefix, name);
  shortname = shortdefname(full_name);

  field_number = google_protobuf_FieldDescriptorProto_number(field_proto);

  if (field_number == 0 || field_number > UPB_MAX_FIELDNUMBER) {
    upb_status_seterrf(ctx->status, "invalid field number (%u)", field_number);
    return false;
  }

  if (m) {
    /* direct message field. */
    upb_value v, packed_v;

    f = (upb_fielddef*)&m->fields[m->field_count++];
    f->msgdef = m;
    f->is_extension_ = false;

    packed_v = pack_def(f, UPB_DEFTYPE_FIELD);
    v = upb_value_constptr(f);

    if (!upb_strtable_insert3(&m->ntof, name.data, name.size, packed_v, alloc)) {
      upb_status_seterrf(ctx->status, "duplicate field name (%s)", shortname);
      return false;
    }

    if (!upb_inttable_insert2(&m->itof, field_number, v, alloc)) {
      upb_status_seterrf(ctx->status, "duplicate field number (%u)",
                         field_number);
      return false;
    }
  } else {
    /* extension field. */
    f = (upb_fielddef*)&ctx->file->exts[ctx->file->ext_count];
    f->is_extension_ = true;
    CHK_OOM(symtab_add(ctx, full_name, pack_def(f, UPB_DEFTYPE_FIELD)));
  }

  f->full_name = full_name;
  f->file = ctx->file;
  f->type_ = (int)google_protobuf_FieldDescriptorProto_type(field_proto);
  f->label_ = (int)google_protobuf_FieldDescriptorProto_label(field_proto);
  f->number_ = field_number;
  f->oneof = NULL;

  /* We can't resolve the subdef or (in the case of extensions) the containing
   * message yet, because it may not have been defined yet.  We stash a pointer
   * to the field_proto until later when we can properly resolve it. */
  f->sub.unresolved = field_proto;

  if (f->label_ == UPB_LABEL_REQUIRED && f->file->syntax == UPB_SYNTAX_PROTO3) {
    upb_status_seterrf(ctx->status, "proto3 fields cannot be required (%s)",
                       f->full_name);
    return false;
  }

  if (google_protobuf_FieldDescriptorProto_has_oneof_index(field_proto)) {
    int oneof_index =
        google_protobuf_FieldDescriptorProto_oneof_index(field_proto);
    upb_oneofdef *oneof;
    upb_value v = upb_value_constptr(f);

    if (upb_fielddef_label(f) != UPB_LABEL_OPTIONAL) {
      upb_status_seterrf(ctx->status,
                         "fields in oneof must have OPTIONAL label (%s)",
                         f->full_name);
      return false;
    }

    if (!m) {
      upb_status_seterrf(ctx->status,
                         "oneof_index provided for extension field (%s)",
                         f->full_name);
      return false;
    }

    if (oneof_index >= m->oneof_count) {
      upb_status_seterrf(ctx->status, "oneof_index out of range (%s)",
                         f->full_name);
      return false;
    }

    oneof = (upb_oneofdef*)&m->oneofs[oneof_index];
    f->oneof = oneof;

    CHK(upb_inttable_insert2(&oneof->itof, f->number_, v, alloc));
    CHK(upb_strtable_insert3(&oneof->ntof, name.data, name.size, v, alloc));
  } else {
    f->oneof = NULL;
  }

  if (google_protobuf_FieldDescriptorProto_has_options(field_proto)) {
    options = google_protobuf_FieldDescriptorProto_options(field_proto);
    f->lazy_ = google_protobuf_FieldOptions_lazy(options);
    f->packed_ = google_protobuf_FieldOptions_packed(options);
  } else {
    f->lazy_ = false;
    f->packed_ = false;
  }

  return true;
}

static bool create_enumdef(
    const symtab_addctx *ctx, const char *prefix,
    const google_protobuf_EnumDescriptorProto *enum_proto) {
  upb_enumdef *e;
  const google_protobuf_EnumValueDescriptorProto *const *values;
  upb_strview name;
  size_t i, n;

  name = google_protobuf_EnumDescriptorProto_name(enum_proto);
  CHK(upb_isident(name, false, ctx->status));

  e = (upb_enumdef*)&ctx->file->enums[ctx->file->enum_count++];
  e->full_name = makefullname(ctx, prefix, name);
  CHK_OOM(symtab_add(ctx, e->full_name, pack_def(e, UPB_DEFTYPE_ENUM)));

  CHK_OOM(upb_strtable_init2(&e->ntoi, UPB_CTYPE_INT32, ctx->alloc));
  CHK_OOM(upb_inttable_init2(&e->iton, UPB_CTYPE_CSTR, ctx->alloc));

  e->file = ctx->file;
  e->defaultval = 0;

  values = google_protobuf_EnumDescriptorProto_value(enum_proto, &n);

  if (n == 0) {
    upb_status_seterrf(ctx->status,
                       "enums must contain at least one value (%s)",
                       e->full_name);
    return false;
  }

  for (i = 0; i < n; i++) {
    const google_protobuf_EnumValueDescriptorProto *value = values[i];
    upb_strview name = google_protobuf_EnumValueDescriptorProto_name(value);
    char *name2 = strviewdup(ctx, name);
    int32_t num = google_protobuf_EnumValueDescriptorProto_number(value);
    upb_value v = upb_value_int32(num);

    if (i == 0 && e->file->syntax == UPB_SYNTAX_PROTO3 && num != 0) {
      upb_status_seterrf(ctx->status,
                         "for proto3, the first enum value must be zero (%s)",
                         e->full_name);
      return false;
    }

    if (upb_strtable_lookup(&e->ntoi, name2, NULL)) {
      upb_status_seterrf(ctx->status, "duplicate enum label '%s'", name2);
      return false;
    }

    CHK_OOM(name2)
    CHK_OOM(
        upb_strtable_insert3(&e->ntoi, name2, strlen(name2), v, ctx->alloc));

    if (!upb_inttable_lookup(&e->iton, num, NULL)) {
      upb_value v = upb_value_cstr(name2);
      CHK_OOM(upb_inttable_insert2(&e->iton, num, v, ctx->alloc));
    }
  }

  upb_inttable_compact2(&e->iton, ctx->alloc);

  return true;
}

static bool create_msgdef(const symtab_addctx *ctx, const char *prefix,
                          const google_protobuf_DescriptorProto *msg_proto) {
  upb_msgdef *m;
  const google_protobuf_MessageOptions *options;
  const google_protobuf_OneofDescriptorProto *const *oneofs;
  const google_protobuf_FieldDescriptorProto *const *fields;
  const google_protobuf_EnumDescriptorProto *const *enums;
  const google_protobuf_DescriptorProto *const *msgs;
  size_t i, n;
  upb_strview name;

  name = google_protobuf_DescriptorProto_name(msg_proto);
  CHK(upb_isident(name, false, ctx->status));

  m = (upb_msgdef*)&ctx->file->msgs[ctx->file->msg_count++];
  m->full_name = makefullname(ctx, prefix, name);
  CHK_OOM(symtab_add(ctx, m->full_name, pack_def(m, UPB_DEFTYPE_MSG)));

  CHK_OOM(upb_inttable_init2(&m->itof, UPB_CTYPE_CONSTPTR, ctx->alloc));
  CHK_OOM(upb_strtable_init2(&m->ntof, UPB_CTYPE_CONSTPTR, ctx->alloc));

  m->file = ctx->file;
  m->map_entry = false;

  options = google_protobuf_DescriptorProto_options(msg_proto);

  if (options) {
    m->map_entry = google_protobuf_MessageOptions_map_entry(options);
  }

  oneofs = google_protobuf_DescriptorProto_oneof_decl(msg_proto, &n);
  m->oneof_count = 0;
  m->oneofs = upb_malloc(ctx->alloc, sizeof(*m->oneofs) * n);
  for (i = 0; i < n; i++) {
    CHK(create_oneofdef(ctx, m, oneofs[i]));
  }

  fields = google_protobuf_DescriptorProto_field(msg_proto, &n);
  m->field_count = 0;
  m->fields = upb_malloc(ctx->alloc, sizeof(*m->fields) * n);
  for (i = 0; i < n; i++) {
    CHK(create_fielddef(ctx, m->full_name, m, fields[i]));
  }

  CHK(assign_msg_indices(m, ctx->status));
  assign_msg_wellknowntype(m);
  upb_inttable_compact2(&m->itof, ctx->alloc);

  /* This message is built.  Now build nested messages and enums. */

  enums = google_protobuf_DescriptorProto_enum_type(msg_proto, &n);
  for (i = 0; i < n; i++) {
    CHK(create_enumdef(ctx, m->full_name, enums[i]));
  }

  msgs = google_protobuf_DescriptorProto_nested_type(msg_proto, &n);
  for (i = 0; i < n; i++) {
    CHK(create_msgdef(ctx, m->full_name, msgs[i]));
  }

  return true;
}

typedef struct {
  int msg_count;
  int enum_count;
  int ext_count;
} decl_counts;

static void count_types_in_msg(const google_protobuf_DescriptorProto *msg_proto,
                               decl_counts *counts) {
  const google_protobuf_DescriptorProto *const *msgs;
  size_t i, n;

  counts->msg_count++;

  msgs = google_protobuf_DescriptorProto_nested_type(msg_proto, &n);
  for (i = 0; i < n; i++) {
    count_types_in_msg(msgs[i], counts);
  }

  google_protobuf_DescriptorProto_enum_type(msg_proto, &n);
  counts->enum_count += n;

  google_protobuf_DescriptorProto_extension(msg_proto, &n);
  counts->ext_count += n;
}

static void count_types_in_file(
    const google_protobuf_FileDescriptorProto *file_proto,
    decl_counts *counts) {
  const google_protobuf_DescriptorProto *const *msgs;
  size_t i, n;

  msgs = google_protobuf_FileDescriptorProto_message_type(file_proto, &n);
  for (i = 0; i < n; i++) {
    count_types_in_msg(msgs[i], counts);
  }

  google_protobuf_FileDescriptorProto_enum_type(file_proto, &n);
  counts->enum_count += n;

  google_protobuf_FileDescriptorProto_extension(file_proto, &n);
  counts->ext_count += n;
}

static bool resolve_fielddef(const symtab_addctx *ctx, const char *prefix,
                             upb_fielddef *f) {
  upb_strview name;
  const google_protobuf_FieldDescriptorProto *field_proto = f->sub.unresolved;

  if (f->is_extension_) {
    if (!google_protobuf_FieldDescriptorProto_has_extendee(field_proto)) {
      upb_status_seterrf(ctx->status,
                         "extension for field '%s' had no extendee",
                         f->full_name);
      return false;
    }

    name = google_protobuf_FieldDescriptorProto_extendee(field_proto);
    f->msgdef = symtab_resolve(ctx, f, prefix, name, UPB_DEFTYPE_MSG);
    CHK(f->msgdef);
  }

  if ((upb_fielddef_issubmsg(f) || f->type_ == UPB_DESCRIPTOR_TYPE_ENUM) &&
      !google_protobuf_FieldDescriptorProto_has_type_name(field_proto)) {
    upb_status_seterrf(ctx->status, "field '%s' is missing type name",
                       f->full_name);
    return false;
  }

  name = google_protobuf_FieldDescriptorProto_type_name(field_proto);

  if (upb_fielddef_issubmsg(f)) {
    f->sub.msgdef = symtab_resolve(ctx, f, prefix, name, UPB_DEFTYPE_MSG);
    CHK(f->sub.msgdef);
  } else if (f->type_ == UPB_DESCRIPTOR_TYPE_ENUM) {
    f->sub.enumdef = symtab_resolve(ctx, f, prefix, name, UPB_DEFTYPE_ENUM);
    CHK(f->sub.enumdef);
  }

  /* Have to delay resolving of the default value until now because of the enum
   * case, since enum defaults are specified with a label. */
  if (google_protobuf_FieldDescriptorProto_has_default_value(field_proto)) {
    upb_strview defaultval =
        google_protobuf_FieldDescriptorProto_default_value(field_proto);

    if (f->file->syntax == UPB_SYNTAX_PROTO3) {
      upb_status_seterrf(ctx->status,
                         "proto3 fields cannot have explicit defaults (%s)",
                         f->full_name);
      return false;
    }

    if (upb_fielddef_issubmsg(f)) {
      upb_status_seterrf(ctx->status,
                         "message fields cannot have explicit defaults (%s)",
                         f->full_name);
      return false;
    }

    if (!parse_default(ctx, defaultval.data, defaultval.size, f)) {
      upb_status_seterrf(ctx->status,
                         "couldn't parse default '" UPB_STRVIEW_FORMAT
                         "' for field (%s)",
                         UPB_STRVIEW_ARGS(defaultval), f->full_name);
      return false;
    }
  } else {
    set_default_default(ctx, f);
  }

  return true;
}

static bool build_filedef(
    const symtab_addctx *ctx, upb_filedef *file,
    const google_protobuf_FileDescriptorProto *file_proto) {
  upb_alloc *alloc = ctx->alloc;
  const google_protobuf_FileOptions *file_options_proto;
  const google_protobuf_DescriptorProto *const *msgs;
  const google_protobuf_EnumDescriptorProto *const *enums;
  const google_protobuf_FieldDescriptorProto *const *exts;
  const upb_strview* strs;
  size_t i, n;
  decl_counts counts = {0};

  count_types_in_file(file_proto, &counts);

  file->msgs = upb_malloc(alloc, sizeof(*file->msgs) * counts.msg_count);
  file->enums = upb_malloc(alloc, sizeof(*file->enums) * counts.enum_count);
  file->exts = upb_malloc(alloc, sizeof(*file->exts) * counts.ext_count);

  CHK_OOM(counts.msg_count == 0 || file->msgs);
  CHK_OOM(counts.enum_count == 0 || file->enums);
  CHK_OOM(counts.ext_count == 0 || file->exts);

  /* We increment these as defs are added. */
  file->msg_count = 0;
  file->enum_count = 0;
  file->ext_count = 0;

  if (!google_protobuf_FileDescriptorProto_has_name(file_proto)) {
    upb_status_seterrmsg(ctx->status, "File has no name");
    return false;
  }

  file->name =
      strviewdup(ctx, google_protobuf_FileDescriptorProto_name(file_proto));
  file->phpprefix = NULL;
  file->phpnamespace = NULL;

  if (google_protobuf_FileDescriptorProto_has_package(file_proto)) {
    upb_strview package =
        google_protobuf_FileDescriptorProto_package(file_proto);
    CHK(upb_isident(package, true, ctx->status));
    file->package = strviewdup(ctx, package);
  } else {
    file->package = NULL;
  }

  if (google_protobuf_FileDescriptorProto_has_syntax(file_proto)) {
    upb_strview syntax =
        google_protobuf_FileDescriptorProto_syntax(file_proto);

    if (streql_view(syntax, "proto2")) {
      file->syntax = UPB_SYNTAX_PROTO2;
    } else if (streql_view(syntax, "proto3")) {
      file->syntax = UPB_SYNTAX_PROTO3;
    } else {
      upb_status_seterrf(ctx->status, "Invalid syntax '%s'", syntax);
      return false;
    }
  } else {
    file->syntax = UPB_SYNTAX_PROTO2;
  }

  /* Read options. */
  file_options_proto = google_protobuf_FileDescriptorProto_options(file_proto);
  if (file_options_proto) {
    if (google_protobuf_FileOptions_has_php_class_prefix(file_options_proto)) {
      file->phpprefix = strviewdup(
          ctx,
          google_protobuf_FileOptions_php_class_prefix(file_options_proto));
    }
    if (google_protobuf_FileOptions_has_php_namespace(file_options_proto)) {
      file->phpnamespace = strviewdup(
          ctx, google_protobuf_FileOptions_php_namespace(file_options_proto));
    }
  }

  /* Verify dependencies. */
  strs = google_protobuf_FileDescriptorProto_dependency(file_proto, &n);
  file->deps = upb_malloc(alloc, sizeof(*file->deps) * n) ;
  CHK_OOM(n == 0 || file->deps);

  for (i = 0; i < n; i++) {
    upb_strview dep_name = strs[i];
    upb_value v;
    if (!upb_strtable_lookup2(&ctx->symtab->files, dep_name.data,
                              dep_name.size, &v)) {
      upb_status_seterrf(ctx->status,
                         "Depends on file '" UPB_STRVIEW_FORMAT
                         "', but it has not been loaded",
                         UPB_STRVIEW_ARGS(dep_name));
      return false;
    }
    file->deps[i] = upb_value_getconstptr(v);
  }

  /* Create messages. */
  msgs = google_protobuf_FileDescriptorProto_message_type(file_proto, &n);
  for (i = 0; i < n; i++) {
    CHK(create_msgdef(ctx, file->package, msgs[i]));
  }

  /* Create enums. */
  enums = google_protobuf_FileDescriptorProto_enum_type(file_proto, &n);
  for (i = 0; i < n; i++) {
    CHK(create_enumdef(ctx, file->package, enums[i]));
  }

  /* Create extensions. */
  exts = google_protobuf_FileDescriptorProto_extension(file_proto, &n);
  file->exts = upb_malloc(alloc, sizeof(*file->exts) * n);
  CHK_OOM(n == 0 || file->exts);
  for (i = 0; i < n; i++) {
    CHK(create_fielddef(ctx, file->package, NULL, exts[i]));
  }

  /* Now that all names are in the table, resolve references. */
  for (i = 0; i < file->ext_count; i++) {
    CHK(resolve_fielddef(ctx, file->package, (upb_fielddef*)&file->exts[i]));
  }

  for (i = 0; i < file->msg_count; i++) {
    const upb_msgdef *m = &file->msgs[i];
    int j;
    for (j = 0; j < m->field_count; j++) {
      CHK(resolve_fielddef(ctx, m->full_name, (upb_fielddef*)&m->fields[j]));
    }
  }

  return true;
 }

static bool upb_symtab_addtotabs(upb_symtab *s, symtab_addctx *ctx,
                                 upb_status *status) {
  const upb_filedef *file = ctx->file;
  upb_alloc *alloc = upb_arena_alloc(s->arena);
  upb_strtable_iter iter;

  CHK_OOM(upb_strtable_insert3(&s->files, file->name, strlen(file->name),
                               upb_value_constptr(file), alloc));

  upb_strtable_begin(&iter, ctx->addtab);
  for (; !upb_strtable_done(&iter); upb_strtable_next(&iter)) {
    const char *key = upb_strtable_iter_key(&iter);
    size_t keylen = upb_strtable_iter_keylength(&iter);
    upb_value value = upb_strtable_iter_value(&iter);
    CHK_OOM(upb_strtable_insert3(&s->syms, key, keylen, value, alloc));
  }

  return true;
}

/* upb_filedef ****************************************************************/

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

int upb_filedef_msgcount(const upb_filedef *f) {
  return f->msg_count;
}

int upb_filedef_depcount(const upb_filedef *f) {
  return f->dep_count;
}

int upb_filedef_enumcount(const upb_filedef *f) {
  return f->enum_count;
}

const upb_filedef *upb_filedef_dep(const upb_filedef *f, int i) {
  return i < 0 || i >= f->dep_count ? NULL : f->deps[i];
}

const upb_msgdef *upb_filedef_msg(const upb_filedef *f, int i) {
  return i < 0 || i >= f->msg_count ? NULL : &f->msgs[i];
}

const upb_enumdef *upb_filedef_enum(const upb_filedef *f, int i) {
  return i < 0 || i >= f->enum_count ? NULL : &f->enums[i];
}

void upb_symtab_free(upb_symtab *s) {
  upb_arena_free(s->arena);
  upb_gfree(s);
}

upb_symtab *upb_symtab_new(void) {
  upb_symtab *s = upb_gmalloc(sizeof(*s));
  upb_alloc *alloc;

  if (!s) {
    return NULL;
  }

  s->arena = upb_arena_new();
  alloc = upb_arena_alloc(s->arena);

  if (!upb_strtable_init2(&s->syms, UPB_CTYPE_CONSTPTR, alloc) ||
      !upb_strtable_init2(&s->files, UPB_CTYPE_CONSTPTR, alloc)) {
    upb_arena_free(s->arena);
    upb_gfree(s);
    s = NULL;
  }
  return s;
}

const upb_msgdef *upb_symtab_lookupmsg(const upb_symtab *s, const char *sym) {
  upb_value v;
  return upb_strtable_lookup(&s->syms, sym, &v) ?
      unpack_def(v, UPB_DEFTYPE_MSG) : NULL;
}

const upb_msgdef *upb_symtab_lookupmsg2(const upb_symtab *s, const char *sym,
                                        size_t len) {
  upb_value v;
  return upb_strtable_lookup2(&s->syms, sym, len, &v) ?
      unpack_def(v, UPB_DEFTYPE_MSG) : NULL;
}

const upb_enumdef *upb_symtab_lookupenum(const upb_symtab *s, const char *sym) {
  upb_value v;
  return upb_strtable_lookup(&s->syms, sym, &v) ?
      unpack_def(v, UPB_DEFTYPE_ENUM) : NULL;
}

const upb_filedef *upb_symtab_lookupfile(const upb_symtab *s, const char *name) {
  upb_value v;
  return upb_strtable_lookup(&s->files, name, &v) ? upb_value_getconstptr(v)
                                                  : NULL;
}

const upb_filedef *upb_symtab_addfile(
    upb_symtab *s, const google_protobuf_FileDescriptorProto *file_proto,
    upb_status *status) {
  upb_arena *tmparena = upb_arena_new();
  upb_strtable addtab;
  upb_alloc *alloc = upb_arena_alloc(s->arena);
  upb_filedef *file = upb_malloc(alloc, sizeof(*file));
  bool ok;
  symtab_addctx ctx;

  ctx.file = file;
  ctx.symtab = s;
  ctx.alloc = alloc;
  ctx.tmp = upb_arena_alloc(tmparena);
  ctx.addtab = &addtab;
  ctx.status = status;

  ok = file &&
      upb_strtable_init2(&addtab, UPB_CTYPE_CONSTPTR, ctx.tmp) &&
      build_filedef(&ctx, file, file_proto) &&
      upb_symtab_addtotabs(s, &ctx, status);

  upb_arena_free(tmparena);
  return ok ? file : NULL;
}

/* Include here since we want most of this file to be stdio-free. */
#include <stdio.h>

bool _upb_symtab_loaddefinit(upb_symtab *s, const upb_def_init *init) {
  /* Since this function should never fail (it would indicate a bug in upb) we
   * print errors to stderr instead of returning error status to the user. */
  upb_def_init **deps = init->deps;
  google_protobuf_FileDescriptorProto *file;
  upb_arena *arena;
  upb_status status;

  upb_status_clear(&status);

  if (upb_strtable_lookup(&s->files, init->filename, NULL)) {
    return true;
  }

  arena = upb_arena_new();

  for (; *deps; deps++) {
    if (!_upb_symtab_loaddefinit(s, *deps)) goto err;
  }

  file = google_protobuf_FileDescriptorProto_parse(
      init->descriptor.data, init->descriptor.size, arena);

  if (!file) {
    upb_status_seterrf(
        &status,
        "Failed to parse compiled-in descriptor for file '%s'. This should "
        "never happen.",
        init->filename);
    goto err;
  }

  if (!upb_symtab_addfile(s, file, &status)) goto err;

  upb_arena_free(arena);
  return true;

err:
  fprintf(stderr, "Error loading compiled-in descriptor: %s\n",
          upb_status_errmsg(&status));
  upb_arena_free(arena);
  return false;
}

#undef CHK
#undef CHK_OOM



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

static size_t upb_msgval_sizeof2(upb_fieldtype_t type) {
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
    case UPB_TYPE_MESSAGE:
      return sizeof(void*);
    case UPB_TYPE_BYTES:
    case UPB_TYPE_STRING:
      return sizeof(upb_strview);
  }
  UPB_UNREACHABLE();
}

static uint8_t upb_msg_fielddefsize(const upb_fielddef *f) {
  if (upb_fielddef_isseq(f)) {
    return sizeof(void*);
  } else {
    return upb_msgval_sizeof2(upb_fielddef_type(f));
  }
}


/** upb_msglayout *************************************************************/

static void upb_msglayout_free(upb_msglayout *l) {
  upb_gfree(l);
}

static size_t upb_msglayout_place(upb_msglayout *l, size_t size) {
  size_t ret;

  l->size = align_up(l->size, size);
  ret = l->size;
  l->size += size;
  return ret;
}

static bool upb_msglayout_init(const upb_msgdef *m,
                               upb_msglayout *l,
                               upb_msgfactory *factory) {
  upb_msg_field_iter it;
  upb_msg_oneof_iter oit;
  size_t hasbit;
  size_t submsg_count = 0;
  const upb_msglayout **submsgs;
  upb_msglayout_field *fields;

  for (upb_msg_field_begin(&it, m);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* f = upb_msg_iter_field(&it);
    if (upb_fielddef_issubmsg(f)) {
      submsg_count++;
    }
  }

  memset(l, 0, sizeof(*l));

  fields = upb_gmalloc(upb_msgdef_numfields(m) * sizeof(*fields));
  submsgs = upb_gmalloc(submsg_count * sizeof(*submsgs));

  if ((!fields && upb_msgdef_numfields(m)) ||
      (!submsgs && submsg_count)) {
    /* OOM. */
    upb_gfree(fields);
    upb_gfree(submsgs);
    return false;
  }

  l->field_count = upb_msgdef_numfields(m);
  l->fields = fields;
  l->submsgs = submsgs;

  /* Allocate data offsets in three stages:
   *
   * 1. hasbits.
   * 2. regular fields.
   * 3. oneof fields.
   *
   * OPT: There is a lot of room for optimization here to minimize the size.
   */

  /* Allocate hasbits and set basic field attributes. */
  submsg_count = 0;
  for (upb_msg_field_begin(&it, m), hasbit = 0;
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* f = upb_msg_iter_field(&it);
    upb_msglayout_field *field = &fields[upb_fielddef_index(f)];

    field->number = upb_fielddef_number(f);
    field->descriptortype = upb_fielddef_descriptortype(f);
    field->label = upb_fielddef_label(f);

    if (upb_fielddef_issubmsg(f)) {
      const upb_msglayout *sub_layout =
          upb_msgfactory_getlayout(factory, upb_fielddef_msgsubdef(f));
      field->submsg_index = submsg_count++;
      submsgs[field->submsg_index] = sub_layout;
    }

    if (upb_fielddef_haspresence(f) && !upb_fielddef_containingoneof(f)) {
      field->presence = (hasbit++);
    } else {
      field->presence = 0;
    }
  }

  /* Account for space used by hasbits. */
  l->size = div_round_up(hasbit, 8);

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
    size_t field_size = 0;
    uint32_t case_offset;
    uint32_t data_offset;

    /* Calculate field size: the max of all field sizes. */
    for (upb_oneof_begin(&fit, o);
         !upb_oneof_done(&fit);
         upb_oneof_next(&fit)) {
      const upb_fielddef* f = upb_oneof_iter_field(&fit);
      field_size = UPB_MAX(field_size, upb_msg_fielddefsize(f));
    }

    /* Align and allocate case offset. */
    case_offset = upb_msglayout_place(l, case_size);
    data_offset = upb_msglayout_place(l, field_size);

    for (upb_oneof_begin(&fit, o);
         !upb_oneof_done(&fit);
         upb_oneof_next(&fit)) {
      const upb_fielddef* f = upb_oneof_iter_field(&fit);
      fields[upb_fielddef_index(f)].offset = data_offset;
      fields[upb_fielddef_index(f)].presence = ~case_offset;
    }
  }

  /* Size of the entire structure should be a multiple of its greatest
   * alignment.  TODO: track overall alignment for real? */
  l->size = align_up(l->size, 8);

  return true;
}


/** upb_msgfactory ************************************************************/

struct upb_msgfactory {
  const upb_symtab *symtab;  /* We own a ref. */
  upb_inttable layouts;
};

upb_msgfactory *upb_msgfactory_new(const upb_symtab *symtab) {
  upb_msgfactory *ret = upb_gmalloc(sizeof(*ret));

  ret->symtab = symtab;
  upb_inttable_init(&ret->layouts, UPB_CTYPE_PTR);

  return ret;
}

void upb_msgfactory_free(upb_msgfactory *f) {
  upb_inttable_iter i;
  upb_inttable_begin(&i, &f->layouts);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_msglayout *l = upb_value_getptr(upb_inttable_iter_value(&i));
    upb_msglayout_free(l);
  }

  upb_inttable_uninit(&f->layouts);
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
    /* In case of circular dependency, layout has to be inserted first. */
    upb_msglayout *l = upb_gmalloc(sizeof(*l));
    upb_msgfactory *mutable_f = (void*)f;
    upb_inttable_insertptr(&mutable_f->layouts, m, upb_value_ptr(l));
    UPB_ASSERT(l);
    if (!upb_msglayout_init(m, l, f)) {
      upb_msglayout_free(l);
    }
    return l;
  }
}
/*
** TODO(haberman): it's unclear whether a lot of the consistency checks should
** UPB_ASSERT() or return false.
*/


#include <string.h>



struct upb_handlers {
  upb_handlercache *cache;
  const upb_msgdef *msg;
  const upb_handlers **sub;
  const void *top_closure_type;
  upb_handlers_tabent table[1];  /* Dynamically-sized field handler array. */
};

static void *upb_calloc(upb_arena *arena, size_t size) {
  void *mem = upb_malloc(upb_arena_alloc(arena), size);
  if (mem) {
    memset(mem, 0, size);
  }
  return mem;
}

/* Defined for the sole purpose of having a unique pointer value for
 * UPB_NO_CLOSURE. */
char _upb_noclosure;

/* Given a selector for a STARTSUBMSG handler, resolves to a pointer to the
 * subhandlers for this submessage field. */
#define SUBH(h, selector) (h->sub[selector])

/* The selector for a submessage field is the field index. */
#define SUBH_F(h, f) SUBH(h, upb_fielddef_index(f))

static int32_t trygetsel(upb_handlers *h, const upb_fielddef *f,
                         upb_handlertype_t type) {
  upb_selector_t sel;
  bool ok;

  ok = upb_handlers_getselector(f, type, &sel);

  UPB_ASSERT(upb_handlers_msgdef(h) == upb_fielddef_containingtype(f));
  UPB_ASSERT(ok);

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
  return &h->table[handlers_getsel(h, f, type)].attr.return_closure_type;
}

static bool doset(upb_handlers *h, int32_t sel, const upb_fielddef *f,
                  upb_handlertype_t type, upb_func *func,
                  const upb_handlerattr *attr) {
  upb_handlerattr set_attr = UPB_HANDLERATTR_INIT;
  const void *closure_type;
  const void **context_closure_type;

  UPB_ASSERT(!h->table[sel].func);

  if (attr) {
    set_attr = *attr;
  }

  /* Check that the given closure type matches the closure type that has been
   * established for this context (if any). */
  closure_type = set_attr.closure_type;

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
    return false;
  }

  if (closure_type)
    *context_closure_type = closure_type;

  /* If this is a STARTSEQ or STARTSTR handler, check that the returned pointer
   * matches any pre-existing expectations about what type is expected. */
  if (type == UPB_HANDLER_STARTSEQ || type == UPB_HANDLER_STARTSTR) {
    const void *return_type = set_attr.return_closure_type;
    const void *table_return_type = h->table[sel].attr.return_closure_type;
    if (return_type && table_return_type && return_type != table_return_type) {
      return false;
    }

    if (table_return_type && !return_type) {
      set_attr.return_closure_type = table_return_type;
    }
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
    ret = h->table[sel].attr.return_closure_type;
  }

  if (type == UPB_HANDLER_STRING &&
      h->table[sel = handlers_getsel(h, f, UPB_HANDLER_STARTSTR)].func) {
    ret = h->table[sel].attr.return_closure_type;
  }

  /* The effective type of the submessage; not used yet.
   * if (type == SUBMESSAGE &&
   *     h->table[sel = handlers_getsel(h, f, UPB_HANDLER_STARTSUBMSG)].func) {
   *   ret = h->table[sel].attr.return_closure_type;
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
  return_closure_type = attr->return_closure_type;
  if (closure_type && return_closure_type &&
      closure_type != return_closure_type) {
    return false;
  }
  return true;
}

static upb_handlers *upb_handlers_new(const upb_msgdef *md,
                                      upb_handlercache *cache,
                                      upb_arena *arena) {
  int extra;
  upb_handlers *h;

  extra = sizeof(upb_handlers_tabent) * (upb_msgdef_selectorcount(md) - 1);
  h = upb_calloc(arena, sizeof(*h) + extra);
  if (!h) return NULL;

  h->cache = cache;
  h->msg = md;

  if (upb_msgdef_submsgfieldcount(md) > 0) {
    size_t bytes = upb_msgdef_submsgfieldcount(md) * sizeof(*h->sub);
    h->sub = upb_calloc(arena, bytes);
    if (!h->sub) return NULL;
  } else {
    h->sub = 0;
  }

  /* calloc() above initialized all handlers to NULL. */
  return h;
}

/* Public interface ***********************************************************/

#define SETTER(name, handlerctype, handlertype)                       \
  bool upb_handlers_set##name(upb_handlers *h, const upb_fielddef *f, \
                              handlerctype func,                      \
                              const upb_handlerattr *attr) {          \
    int32_t sel = trygetsel(h, f, handlertype);                       \
    return doset(h, sel, f, handlertype, (upb_func *)func, attr);     \
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
                             const upb_handlerattr *attr) {
  return doset(h, UPB_UNKNOWN_SELECTOR, NULL, UPB_HANDLER_INT32,
               (upb_func *)func, attr);
}

bool upb_handlers_setstartmsg(upb_handlers *h, upb_startmsg_handlerfunc *func,
                              const upb_handlerattr *attr) {
  return doset(h, UPB_STARTMSG_SELECTOR, NULL, UPB_HANDLER_INT32,
               (upb_func *)func, attr);
}

bool upb_handlers_setendmsg(upb_handlers *h, upb_endmsg_handlerfunc *func,
                            const upb_handlerattr *attr) {
  return doset(h, UPB_ENDMSG_SELECTOR, NULL, UPB_HANDLER_INT32,
               (upb_func *)func, attr);
}

bool upb_handlers_setsubhandlers(upb_handlers *h, const upb_fielddef *f,
                                 const upb_handlers *sub) {
  UPB_ASSERT(sub);
  UPB_ASSERT(upb_fielddef_issubmsg(f));
  if (SUBH_F(h, f)) return false;  /* Can't reset. */
  if (upb_handlers_msgdef(sub) != upb_fielddef_msgsubdef(f)) {
    return false;
  }
  SUBH_F(h, f) = sub;
  return true;
}

const upb_handlers *upb_handlers_getsubhandlers(const upb_handlers *h,
                                                const upb_fielddef *f) {
  UPB_ASSERT(upb_fielddef_issubmsg(f));
  return SUBH_F(h, f);
}

upb_func *upb_handlers_gethandler(const upb_handlers *h, upb_selector_t s,
                                  const void **handler_data) {
  upb_func *ret = (upb_func *)h->table[s].func;
  if (ret && handler_data) {
    *handler_data = h->table[s].attr.handler_data;
  }
  return ret;
}

bool upb_handlers_getattr(const upb_handlers *h, upb_selector_t sel,
                          upb_handlerattr *attr) {
  if (!upb_handlers_gethandler(h, sel, NULL))
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
  return upb_handlercache_addcleanup(h->cache, p, func);
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
  uint32_t selector_base = upb_fielddef_selectorbase(f);
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
      *s = selector_base;
      break;
    case UPB_HANDLER_STRING:
      if (upb_fielddef_isstring(f)) {
        *s = selector_base;
      } else if (upb_fielddef_lazy(f)) {
        *s = selector_base + 3;
      } else {
        return false;
      }
      break;
    case UPB_HANDLER_STARTSTR:
      if (upb_fielddef_isstring(f) || upb_fielddef_lazy(f)) {
        *s = selector_base + 1;
      } else {
        return false;
      }
      break;
    case UPB_HANDLER_ENDSTR:
      if (upb_fielddef_isstring(f) || upb_fielddef_lazy(f)) {
        *s = selector_base + 2;
      } else {
        return false;
      }
      break;
    case UPB_HANDLER_STARTSEQ:
      if (!upb_fielddef_isseq(f)) return false;
      *s = selector_base - 2;
      break;
    case UPB_HANDLER_ENDSEQ:
      if (!upb_fielddef_isseq(f)) return false;
      *s = selector_base - 1;
      break;
    case UPB_HANDLER_STARTSUBMSG:
      if (!upb_fielddef_issubmsg(f)) return false;
      /* Selectors for STARTSUBMSG are at the beginning of the table so that the
       * selector can also be used as an index into the "sub" array of
       * subhandlers.  The indexes for the two into these two tables are the
       * same, except that in the handler table the static selectors come first. */
      *s = upb_fielddef_index(f) + UPB_STATIC_SELECTOR_COUNT;
      break;
    case UPB_HANDLER_ENDSUBMSG:
      if (!upb_fielddef_issubmsg(f)) return false;
      *s = selector_base;
      break;
  }
  UPB_ASSERT((size_t)*s < upb_msgdef_selectorcount(upb_fielddef_containingtype(f)));
  return true;
}

/* upb_handlercache ***********************************************************/

struct upb_handlercache {
  upb_arena *arena;
  upb_inttable tab;  /* maps upb_msgdef* -> upb_handlers*. */
  upb_handlers_callback *callback;
  const void *closure;
};

const upb_handlers *upb_handlercache_get(upb_handlercache *c,
                                         const upb_msgdef *md) {
  upb_msg_field_iter i;
  upb_value v;
  upb_handlers *h;

  if (upb_inttable_lookupptr(&c->tab, md, &v)) {
    return upb_value_getptr(v);
  }

  h = upb_handlers_new(md, c, c->arena);
  v = upb_value_ptr(h);

  if (!h) return NULL;
  if (!upb_inttable_insertptr(&c->tab, md, v)) return NULL;

  c->callback(c->closure, h);

  /* For each submessage field, get or create a handlers object and set it as
   * the subhandlers. */
  for(upb_msg_field_begin(&i, md);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);

    if (upb_fielddef_issubmsg(f)) {
      const upb_msgdef *subdef = upb_fielddef_msgsubdef(f);
      const upb_handlers *sub_mh = upb_handlercache_get(c, subdef);

      if (!sub_mh) return NULL;

      upb_handlers_setsubhandlers(h, f, sub_mh);
    }
  }

  return h;
}


upb_handlercache *upb_handlercache_new(upb_handlers_callback *callback,
                                       const void *closure) {
  upb_handlercache *cache = upb_gmalloc(sizeof(*cache));

  if (!cache) return NULL;

  cache->arena = upb_arena_new();

  cache->callback = callback;
  cache->closure = closure;

  if (!upb_inttable_init(&cache->tab, UPB_CTYPE_PTR)) goto oom;

  return cache;

oom:
  upb_gfree(cache);
  return NULL;
}

void upb_handlercache_free(upb_handlercache *cache) {
  upb_inttable_uninit(&cache->tab);
  upb_arena_free(cache->arena);
  upb_gfree(cache);
}

bool upb_handlercache_addcleanup(upb_handlercache *c, void *p,
                                 upb_handlerfree *func) {
  return upb_arena_addcleanup(c->arena, p, func);
}

/* upb_byteshandler ***********************************************************/

bool upb_byteshandler_setstartstr(upb_byteshandler *h,
                                  upb_startstr_handlerfunc *func, void *d) {
  h->table[UPB_STARTSTR_SELECTOR].func = (upb_func*)func;
  h->table[UPB_STARTSTR_SELECTOR].attr.handler_data = d;
  return true;
}

bool upb_byteshandler_setstring(upb_byteshandler *h,
                                upb_string_handlerfunc *func, void *d) {
  h->table[UPB_STRING_SELECTOR].func = (upb_func*)func;
  h->table[UPB_STRING_SELECTOR].attr.handler_data = d;
  return true;
}

bool upb_byteshandler_setendstr(upb_byteshandler *h,
                                upb_endfield_handlerfunc *func, void *d) {
  h->table[UPB_ENDSTR_SELECTOR].func = (upb_func*)func;
  h->table[UPB_ENDSTR_SELECTOR].attr.handler_data = d;
  return true;
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
  upb_handlerattr attr = UPB_HANDLERATTR_INIT;
  bool ok;

  upb_msg_handlerdata *d = upb_gmalloc(sizeof(*d));
  if (!d) return false;
  d->offset = offset;
  d->hasbit = hasbit;

  attr.handler_data = d;
  attr.alwaysok = true;
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

  return ok;
}

bool upb_msg_getscalarhandlerdata(const upb_handlers *h,
                                  upb_selector_t s,
                                  upb_fieldtype_t *type,
                                  size_t *offset,
                                  int32_t *hasbit) {
  const upb_msg_handlerdata *d;
  const void *p;
  upb_func *f = upb_handlers_gethandler(h, s, &p);

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

  d = p;
  *offset = d->offset;
  *hasbit = d->hasbit;
  return true;
}


bool upb_bufsrc_putbuf(const char *buf, size_t len, upb_bytessink sink) {
  void *subc;
  bool ret;
  upb_bufhandle handle = UPB_BUFHANDLE_INIT;
  handle.buf = buf;
  ret = upb_bytessink_start(sink, len, &subc);
  if (ret && len != 0) {
    ret = (upb_bytessink_putbuf(sink, subc, buf, len, &handle) >= len);
  }
  if (ret) {
    ret = upb_bytessink_end(sink);
  }
  return ret;
}
/*
** protobuf decoder bytecode compiler
**
** Code to compile a upb::Handlers into bytecode for decoding a protobuf
** according to that specific schema and destination handlers.
**
** Bytecode definition is in decoder.int.h.
*/

#include <stdarg.h>

#ifdef UPB_DUMP_BYTECODE
#include <stdio.h>
#endif


#define MAXLABEL 5
#define EMPTYLABEL -1

/* upb_pbdecodermethod ********************************************************/

static void freemethod(upb_pbdecodermethod *method) {
  upb_inttable_uninit(&method->dispatch);
  upb_gfree(method);
}

static upb_pbdecodermethod *newmethod(const upb_handlers *dest_handlers,
                                      mgroup *group) {
  upb_pbdecodermethod *ret = upb_gmalloc(sizeof(*ret));
  upb_byteshandler_init(&ret->input_handler_);

  ret->group = group;
  ret->dest_handlers_ = dest_handlers;
  upb_inttable_init(&ret->dispatch, UPB_CTYPE_UINT64);

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


/* mgroup *********************************************************************/

static void freegroup(mgroup *g) {
  upb_inttable_iter i;

  upb_inttable_begin(&i, &g->methods);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    freemethod(upb_value_getptr(upb_inttable_iter_value(&i)));
  }

  upb_inttable_uninit(&g->methods);
  upb_gfree(g->bytecode);
  upb_gfree(g);
}

mgroup *newgroup(void) {
  mgroup *g = upb_gmalloc(sizeof(*g));
  upb_inttable_init(&g->methods, UPB_CTYPE_PTR);
  g->bytecode = NULL;
  g->bytecode_end = NULL;
  return g;
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
    *instruction = getop(*instruction) | (uint32_t)ofs << 8;
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

#if defined(UPB_DUMP_BYTECODE)

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
  if (upb_handlers_gethandler(h, sel, NULL)) {
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

  return upb_handlers_gethandler(h, getsel(f, UPB_HANDLER_STARTSTR), NULL) ||
         upb_handlers_gethandler(h, getsel(f, UPB_HANDLER_STRING), NULL) ||
         upb_handlers_gethandler(h, getsel(f, UPB_HANDLER_ENDSTR), NULL);
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
    maybeput(c, OP_ENDSTR, h, f, UPB_HANDLER_ENDSTR);
    putop(c, OP_POP);
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
    maybeput(c, OP_ENDSTR, h, f, UPB_HANDLER_ENDSTR);
    putop(c, OP_POP);
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
  upb_pbdecodermethod *method;

  if (upb_inttable_lookupptr(&c->group->methods, h, &v))
    return;

  method = newmethod(h, c->group);
  upb_inttable_insertptr(&c->group->methods, h, upb_value_ptr(method));

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


/* TODO(haberman): allow this to be constructed for an arbitrary set of dest
 * handlers and other mgroups (but verify we have a transitive closure). */
const mgroup *mgroup_new(const upb_handlers *dest, bool lazy) {
  mgroup *g;
  compiler *c;

  g = newgroup();
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

  set_bytecode_handlers(g);
  return g;
}


/* upb_pbcodecache ************************************************************/

upb_pbcodecache *upb_pbcodecache_new(upb_handlercache *dest) {
  upb_pbcodecache *c = upb_gmalloc(sizeof(*c));

  if (!c) return NULL;

  c->dest = dest;
  c->lazy = false;

  c->arena = upb_arena_new();
  if (!upb_inttable_init(&c->groups, UPB_CTYPE_CONSTPTR)) return NULL;

  return c;
}

void upb_pbcodecache_free(upb_pbcodecache *c) {
  upb_inttable_iter i;

  upb_inttable_begin(&i, &c->groups);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_value val = upb_inttable_iter_value(&i);
    freegroup((void*)upb_value_getconstptr(val));
  }

  upb_inttable_uninit(&c->groups);
  upb_arena_free(c->arena);
  upb_gfree(c);
}

void upb_pbdecodermethodopts_setlazy(upb_pbcodecache *c, bool lazy) {
  UPB_ASSERT(upb_inttable_count(&c->groups) == 0);
  c->lazy = lazy;
}

const upb_pbdecodermethod *upb_pbcodecache_get(upb_pbcodecache *c,
                                               const upb_msgdef *md) {
  upb_value v;
  bool ok;
  const upb_handlers *h;
  const mgroup *g;

  h = upb_handlercache_get(c->dest, md);
  if (upb_inttable_lookupptr(&c->groups, md, &v)) {
    g = upb_value_getconstptr(v);
  } else {
    g = mgroup_new(h, c->lazy);
    ok = upb_inttable_insertptr(&c->groups, md, upb_value_constptr(g));
    UPB_ASSERT(ok);
  }

  ok = upb_inttable_lookupptr(&g->methods, h, &v);
  UPB_ASSERT(ok);
  return upb_value_getptr(v);
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
  upb_status_seterrmsg(d->status, msg);
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
      upb_sink_putunknown(d->top->sink, d->checkpoint, d->ptr - d->checkpoint);
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
    upb_sink_put ## name(d->top->sink, arg, (convfunc)(val)); \
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
        CHECK_SUSPEND(upb_sink_startmsg(d->top->sink));
      )
      VMCASE(OP_ENDMSG,
        CHECK_SUSPEND(upb_sink_endmsg(d->top->sink, d->status));
      )
      VMCASE(OP_STARTSEQ,
        upb_pbdecoder_frame *outer = outer_frame(d);
        CHECK_SUSPEND(upb_sink_startseq(outer->sink, arg, &d->top->sink));
      )
      VMCASE(OP_ENDSEQ,
        CHECK_SUSPEND(upb_sink_endseq(d->top->sink, arg));
      )
      VMCASE(OP_STARTSUBMSG,
        upb_pbdecoder_frame *outer = outer_frame(d);
        CHECK_SUSPEND(upb_sink_startsubmsg(outer->sink, arg, &d->top->sink));
      )
      VMCASE(OP_ENDSUBMSG,
        CHECK_SUSPEND(upb_sink_endsubmsg(d->top->sink, arg));
      )
      VMCASE(OP_STARTSTR,
        uint32_t len = delim_remaining(d);
        upb_pbdecoder_frame *outer = outer_frame(d);
        CHECK_SUSPEND(upb_sink_startstr(outer->sink, arg, len, &d->top->sink));
        if (len == 0) {
          d->pc++;  /* Skip OP_STRING. */
        }
      )
      VMCASE(OP_STRING,
        uint32_t len = curbufleft(d);
        size_t n = upb_sink_putstring(d->top->sink, arg, d->ptr, len, handle);
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
        CHECK_SUSPEND(upb_sink_endstr(d->top->sink, arg));
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

upb_pbdecoder *upb_pbdecoder_create(upb_arena *a, const upb_pbdecodermethod *m,
                                    upb_sink sink, upb_status *status) {
  const size_t default_max_nesting = 64;
#ifndef NDEBUG
  size_t size_before = upb_arena_bytesallocated(a);
#endif

  upb_pbdecoder *d = upb_arena_malloc(a, sizeof(upb_pbdecoder));
  if (!d) return NULL;

  d->method_ = m;
  d->callstack = upb_arena_malloc(a, callstacksize(d, default_max_nesting));
  d->stack = upb_arena_malloc(a, stacksize(d, default_max_nesting));
  if (!d->stack || !d->callstack) {
    return NULL;
  }

  d->arena = a;
  d->limit = d->stack + default_max_nesting - 1;
  d->stack_size = default_max_nesting;
  d->status = status;

  upb_pbdecoder_reset(d);
  upb_bytessink_reset(&d->input_, &m->input_handler_, d);

  if (d->method_->dest_handlers_) {
    if (sink.handlers != d->method_->dest_handlers_)
      return NULL;
  }
  d->top->sink = sink;

  /* If this fails, increase the value in decoder.h. */
  UPB_ASSERT_DEBUGVAR(upb_arena_bytesallocated(a) - size_before <=
                      UPB_PB_DECODER_SIZE);
  return d;
}

uint64_t upb_pbdecoder_bytesparsed(const upb_pbdecoder *d) {
  return offset(d);
}

const upb_pbdecodermethod *upb_pbdecoder_method(const upb_pbdecoder *d) {
  return d->method_;
}

upb_bytessink upb_pbdecoder_input(upb_pbdecoder *d) {
  return d->input_;
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
    void *p = upb_arena_realloc(d->arena, d->stack, old_size, new_size);
    if (!p) {
      return false;
    }
    d->stack = p;

    old_size = callstacksize(d, d->stack_size);
    new_size = callstacksize(d, max);
    p = upb_arena_realloc(d->arena, d->callstack, old_size, new_size);
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
  upb_arena *arena;

  /* Our input and output. */
  upb_sink input_;
  upb_bytessink output_;

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

    new_buf = upb_arena_realloc(e->arena, e->buf, old_size, new_size);

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
          upb_arena_realloc(e->arena, e->segbuf, old_size, new_size);

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

  attr->handler_data = tag;
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

#include <stdio.h>
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
    upb_handlerattr attr = UPB_HANDLERATTR_INIT;
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
        upb_handlerattr attr2 = UPB_HANDLERATTR_INIT;
        new_tag(h, f, UPB_WIRE_TYPE_END_GROUP, &attr2);

        upb_handlers_setstartsubmsg(h, f, encode_startgroup, &attr);
        upb_handlers_setendsubmsg(h, f, encode_endgroup, &attr2);

        break;
      }
    }

#undef T
  }
}

void upb_pb_encoder_reset(upb_pb_encoder *e) {
  e->segptr = NULL;
  e->top = NULL;
  e->depth = 0;
}


/* public API *****************************************************************/

upb_handlercache *upb_pb_encoder_newcache(void) {
  return upb_handlercache_new(newhandlers_callback, NULL);
}

upb_pb_encoder *upb_pb_encoder_create(upb_arena *arena, const upb_handlers *h,
                                      upb_bytessink output) {
  const size_t initial_bufsize = 256;
  const size_t initial_segbufsize = 16;
  /* TODO(haberman): make this configurable. */
  const size_t stack_size = 64;
#ifndef NDEBUG
  const size_t size_before = upb_arena_bytesallocated(arena);
#endif

  upb_pb_encoder *e = upb_arena_malloc(arena, sizeof(upb_pb_encoder));
  if (!e) return NULL;

  e->buf = upb_arena_malloc(arena, initial_bufsize);
  e->segbuf = upb_arena_malloc(arena, initial_segbufsize * sizeof(*e->segbuf));
  e->stack = upb_arena_malloc(arena, stack_size * sizeof(*e->stack));

  if (!e->buf || !e->segbuf || !e->stack) {
    return NULL;
  }

  e->limit = e->buf + initial_bufsize;
  e->seglimit = e->segbuf + initial_segbufsize;
  e->stacklimit = e->stack + stack_size;

  upb_pb_encoder_reset(e);
  upb_sink_reset(&e->input_, h, e);

  e->arena = arena;
  e->output_ = output;
  e->subc = output.closure;
  e->ptr = e->buf;

  /* If this fails, increase the value in encoder.h. */
  UPB_ASSERT_DEBUGVAR(upb_arena_bytesallocated(arena) - size_before <=
                      UPB_PB_ENCODER_SIZE);
  return e;
}

upb_sink upb_pb_encoder_input(upb_pb_encoder *e) { return e->input_; }
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
  upb_bytessink output_;
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
  const upb_enumdef *enum_def = upb_fielddef_enumsubdef(f);
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
    upb_handlerattr attr = UPB_HANDLERATTR_INIT;
    attr.handler_data = f;
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
            upb_fielddef_descriptortype(f) == UPB_DESCRIPTOR_TYPE_GROUP
                ? shortname(upb_msgdef_fullname(upb_fielddef_msgsubdef(f)))
                : upb_fielddef_name(f);
        attr.handler_data = name;
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

upb_textprinter *upb_textprinter_create(upb_arena *arena, const upb_handlers *h,
                                        upb_bytessink output) {
  upb_textprinter *p = upb_arena_malloc(arena, sizeof(upb_textprinter));
  if (!p) return NULL;

  p->output_ = output;
  upb_sink_reset(&p->input_, h, p);
  textprinter_reset(p, false);

  return p;
}

upb_handlercache *upb_textprinter_newcache(void) {
  return upb_handlercache_new(&onmreg, NULL);
}

upb_sink upb_textprinter_input(upb_textprinter *p) { return p->input_; }

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

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>



#define UPB_JSON_MAX_DEPTH 64

/* Type of value message */
enum {
  VALUE_NULLVALUE   = 0,
  VALUE_NUMBERVALUE = 1,
  VALUE_STRINGVALUE = 2,
  VALUE_BOOLVALUE   = 3,
  VALUE_STRUCTVALUE = 4,
  VALUE_LISTVALUE   = 5
};

/* Forward declare */
static bool is_top_level(upb_json_parser *p);
static bool is_wellknown_msg(upb_json_parser *p, upb_wellknowntype_t type);
static bool is_wellknown_field(upb_json_parser *p, upb_wellknowntype_t type);

static bool is_number_wrapper_object(upb_json_parser *p);
static bool does_number_wrapper_start(upb_json_parser *p);
static bool does_number_wrapper_end(upb_json_parser *p);

static bool is_string_wrapper_object(upb_json_parser *p);
static bool does_string_wrapper_start(upb_json_parser *p);
static bool does_string_wrapper_end(upb_json_parser *p);

static bool does_fieldmask_start(upb_json_parser *p);
static bool does_fieldmask_end(upb_json_parser *p);
static void start_fieldmask_object(upb_json_parser *p);
static void end_fieldmask_object(upb_json_parser *p);

static void start_wrapper_object(upb_json_parser *p);
static void end_wrapper_object(upb_json_parser *p);

static void start_value_object(upb_json_parser *p, int value_type);
static void end_value_object(upb_json_parser *p);

static void start_listvalue_object(upb_json_parser *p);
static void end_listvalue_object(upb_json_parser *p);

static void start_structvalue_object(upb_json_parser *p);
static void end_structvalue_object(upb_json_parser *p);

static void start_object(upb_json_parser *p);
static void end_object(upb_json_parser *p);

static void start_any_object(upb_json_parser *p, const char *ptr);
static bool end_any_object(upb_json_parser *p, const char *ptr);

static bool start_subobject(upb_json_parser *p);
static void end_subobject(upb_json_parser *p);

static void start_member(upb_json_parser *p);
static void end_member(upb_json_parser *p);
static bool end_membername(upb_json_parser *p);

static void start_any_member(upb_json_parser *p, const char *ptr);
static void end_any_member(upb_json_parser *p, const char *ptr);
static bool end_any_membername(upb_json_parser *p);

size_t parse(void *closure, const void *hd, const char *buf, size_t size,
             const upb_bufhandle *handle);
static bool end(void *closure, const void *hd);

static const char eof_ch = 'e';

/* stringsink */
typedef struct {
  upb_byteshandler handler;
  upb_bytessink sink;
  char *ptr;
  size_t len, size;
} upb_stringsink;


static void *stringsink_start(void *_sink, const void *hd, size_t size_hint) {
  upb_stringsink *sink = _sink;
  sink->len = 0;
  UPB_UNUSED(hd);
  UPB_UNUSED(size_hint);
  return sink;
}

static size_t stringsink_string(void *_sink, const void *hd, const char *ptr,
                                size_t len, const upb_bufhandle *handle) {
  upb_stringsink *sink = _sink;
  size_t new_size = sink->size;

  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  while (sink->len + len > new_size) {
    new_size *= 2;
  }

  if (new_size != sink->size) {
    sink->ptr = realloc(sink->ptr, new_size);
    sink->size = new_size;
  }

  memcpy(sink->ptr + sink->len, ptr, len);
  sink->len += len;

  return len;
}

void upb_stringsink_init(upb_stringsink *sink) {
  upb_byteshandler_init(&sink->handler);
  upb_byteshandler_setstartstr(&sink->handler, stringsink_start, NULL);
  upb_byteshandler_setstring(&sink->handler, stringsink_string, NULL);

  upb_bytessink_reset(&sink->sink, &sink->handler, sink);

  sink->size = 32;
  sink->ptr = malloc(sink->size);
  sink->len = 0;
}

void upb_stringsink_uninit(upb_stringsink *sink) { free(sink->ptr); }

typedef struct {
  /* For encoding Any value field in binary format. */
  upb_handlercache *encoder_handlercache;
  upb_stringsink stringsink;

  /* For decoding Any value field in json format. */
  upb_json_codecache *parser_codecache;
  upb_sink sink;
  upb_json_parser *parser;

  /* Mark the range of uninterpreted values in json input before type url. */
  const char *before_type_url_start;
  const char *before_type_url_end;

  /* Mark the range of uninterpreted values in json input after type url. */
  const char *after_type_url_start;
} upb_jsonparser_any_frame;

typedef struct {
  upb_sink sink;

  /* The current message in which we're parsing, and the field whose value we're
   * expecting next. */
  const upb_msgdef *m;
  const upb_fielddef *f;

  /* The table mapping json name to fielddef for this message. */
  const upb_strtable *name_table;

  /* We are in a repeated-field context. We need this flag to decide whether to
   * handle the array as a normal repeated field or a
   * google.protobuf.ListValue/google.protobuf.Value. */
  bool is_repeated;

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

  /* We are in an Any message context. This flag is set when parsing the Any
   * message and indicates to all field parsers (subobjects, strings, numbers,
   * and bools) that the parsed field should be serialized as binary data or
   * cached (type url not found yet). */
  bool is_any;

  /* The type of packed message in Any. */
  upb_jsonparser_any_frame *any_frame;

  /* True if the field to be parsed is unknown. */
  bool is_unknown_field;
} upb_jsonparser_frame;

static void init_frame(upb_jsonparser_frame* frame) {
  frame->m = NULL;
  frame->f = NULL;
  frame->name_table = NULL;
  frame->is_repeated = false;
  frame->is_map = false;
  frame->is_mapentry = false;
  frame->mapfield = NULL;
  frame->is_any = false;
  frame->any_frame = NULL;
  frame->is_unknown_field = false;
}

struct upb_json_parser {
  upb_arena *arena;
  const upb_json_parsermethod *method;
  upb_bytessink input_;

  /* Stack to track the JSON scopes we are in. */
  upb_jsonparser_frame stack[UPB_JSON_MAX_DEPTH];
  upb_jsonparser_frame *top;
  upb_jsonparser_frame *limit;

  upb_status *status;

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

  /* For resolve type url in Any. */
  const upb_symtab *symtab;

  /* Whether to proceed if unknown field is met. */
  bool ignore_json_unknown;

  /* Cache for parsing timestamp due to base and zone are handled in different
   * handlers. */
  struct tm tm;
};

static upb_jsonparser_frame* start_jsonparser_frame(upb_json_parser *p) {
  upb_jsonparser_frame *inner;
  inner = p->top + 1;
  init_frame(inner);
  return inner;
}

struct upb_json_codecache {
  upb_arena *arena;
  upb_inttable methods;   /* upb_msgdef* -> upb_json_parsermethod* */
};

struct upb_json_parsermethod {
  const upb_json_codecache *cache;
  upb_byteshandler input_handler_;

  /* Maps json_name -> fielddef */
  upb_strtable name_table;
};

#define PARSER_CHECK_RETURN(x) if (!(x)) return false

static upb_jsonparser_any_frame *json_parser_any_frame_new(
    upb_json_parser *p) {
  upb_jsonparser_any_frame *frame;

  frame = upb_arena_malloc(p->arena, sizeof(upb_jsonparser_any_frame));

  frame->encoder_handlercache = upb_pb_encoder_newcache();
  frame->parser_codecache = upb_json_codecache_new();
  frame->parser = NULL;
  frame->before_type_url_start = NULL;
  frame->before_type_url_end = NULL;
  frame->after_type_url_start = NULL;

  upb_stringsink_init(&frame->stringsink);

  return frame;
}

static void json_parser_any_frame_set_payload_type(
    upb_json_parser *p,
    upb_jsonparser_any_frame *frame,
    const upb_msgdef *payload_type) {
  const upb_handlers *h;
  const upb_json_parsermethod *parser_method;
  upb_pb_encoder *encoder;

  /* Initialize encoder. */
  h = upb_handlercache_get(frame->encoder_handlercache, payload_type);
  encoder = upb_pb_encoder_create(p->arena, h, frame->stringsink.sink);

  /* Initialize parser. */
  parser_method = upb_json_codecache_get(frame->parser_codecache, payload_type);
  upb_sink_reset(&frame->sink, h, encoder);
  frame->parser =
      upb_json_parser_create(p->arena, parser_method, p->symtab, frame->sink,
                             p->status, p->ignore_json_unknown);
}

static void json_parser_any_frame_free(upb_jsonparser_any_frame *frame) {
  upb_handlercache_free(frame->encoder_handlercache);
  upb_json_codecache_free(frame->parser_codecache);
  upb_stringsink_uninit(&frame->stringsink);
}

static bool json_parser_any_frame_has_type_url(
  upb_jsonparser_any_frame *frame) {
  return frame->parser != NULL;
}

static bool json_parser_any_frame_has_value_before_type_url(
  upb_jsonparser_any_frame *frame) {
  return frame->before_type_url_start != frame->before_type_url_end;
}

static bool json_parser_any_frame_has_value_after_type_url(
  upb_jsonparser_any_frame *frame) {
  return frame->after_type_url_start != NULL;
}

static bool json_parser_any_frame_has_value(
  upb_jsonparser_any_frame *frame) {
  return json_parser_any_frame_has_value_before_type_url(frame) ||
         json_parser_any_frame_has_value_after_type_url(frame);
}

static void json_parser_any_frame_set_before_type_url_end(
    upb_jsonparser_any_frame *frame,
    const char *ptr) {
  if (frame->parser == NULL) {
    frame->before_type_url_end = ptr;
  }
}

static void json_parser_any_frame_set_after_type_url_start_once(
    upb_jsonparser_any_frame *frame,
    const char *ptr) {
  if (json_parser_any_frame_has_type_url(frame) &&
      frame->after_type_url_start == NULL) {
    frame->after_type_url_start = ptr;
  }
}

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
    upb_status_seterrmsg(p->status, "Nesting too deep");
    return false;
  }

  return true;
}

static void set_name_table(upb_json_parser *p, upb_jsonparser_frame *frame) {
  upb_value v;
  const upb_json_codecache *cache = p->method->cache;
  bool ok;
  const upb_json_parsermethod *method;

  ok = upb_inttable_lookupptr(&cache->methods, frame->m, &v);
  UPB_ASSERT(ok);
  method = upb_value_getconstptr(v);

  frame->name_table = &method->name_table;
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
      upb_status_seterrf(p->status,
                         "Base64 input for bytes field not a multiple of 4: %s",
                         upb_fielddef_name(p->top->f));
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
    upb_sink_putstring(p->top->sink, sel, output, 3, NULL);
  }
  return true;

otherchar:
  if (nonbase64(ptr[0]) || nonbase64(ptr[1]) || nonbase64(ptr[2]) ||
      nonbase64(ptr[3]) ) {
    upb_status_seterrf(p->status,
                       "Non-base64 characters in bytes field: %s",
                       upb_fielddef_name(p->top->f));
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
    upb_sink_putstring(p->top->sink, sel, &output, 1, NULL);
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
    upb_sink_putstring(p->top->sink, sel, output, 2, NULL);
    return true;
  }

badpadding:
  upb_status_seterrf(p->status,
                     "Incorrect base64 padding for field: %s (%.*s)",
                     upb_fielddef_name(p->top->f),
                     4, ptr);
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

  mem = upb_arena_realloc(p->arena, p->accumulate_buf, old_size, new_size);
  if (!mem) {
    upb_status_seterrmsg(p->status, "Out of memory allocating buffer.");
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
    upb_status_seterrmsg(p->status, "Integer overflow.");
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
          p->status, "Internal error: unexpected state MULTIPART_INACTIVE");
      return false;

    case MULTIPART_ACCUMULATE:
      if (!accumulate_append(p, buf, len, can_alias)) {
        return false;
      }
      break;

    case MULTIPART_PUSHEAGERLY: {
      const upb_bufhandle *handle = can_alias ? p->handle : NULL;
      upb_sink_putstring(p->top->sink, p->string_selector, buf, len, handle);
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

static bool start_number(upb_json_parser *p, const char *ptr) {
  if (is_top_level(p)) {
    if (is_number_wrapper_object(p)) {
      start_wrapper_object(p);
    } else if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
      start_value_object(p, VALUE_NUMBERVALUE);
    } else {
      return false;
    }
  } else if (does_number_wrapper_start(p)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_wrapper_object(p);
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_VALUE)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_value_object(p, VALUE_NUMBERVALUE);
  }

  multipart_startaccum(p);
  capture_begin(p, ptr);
  return true;
}

static bool parse_number(upb_json_parser *p, bool is_quoted);

static bool end_number_nontop(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }

  if (p->top->f == NULL) {
    multipart_end(p);
    return true;
  }

  return parse_number(p, false);
}

static bool end_number(upb_json_parser *p, const char *ptr) {
  if (!end_number_nontop(p, ptr)) {
    return false;
  }

  if (does_number_wrapper_end(p)) {
    end_wrapper_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
    return true;
  }

  if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
    end_value_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
    return true;
  }

  return true;
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
  double inf = UPB_INFINITY;

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
        upb_sink_putint32(p->top->sink, parser_getsel(p), val);
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
        upb_sink_putuint32(p->top->sink, parser_getsel(p), val);
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
        upb_sink_putint64(p->top->sink, parser_getsel(p), val);
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
        upb_sink_putuint64(p->top->sink, parser_getsel(p), val);
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
        upb_sink_put ## smalltype(p->top->sink, parser_getsel(p),        \
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
      upb_sink_putdouble(p->top->sink, parser_getsel(p), val);
      return true;
    case UPB_TYPE_FLOAT:
      if ((val > FLT_MAX || val < -FLT_MAX) && val != inf && val != -inf) {
        return false;
      } else {
        upb_sink_putfloat(p->top->sink, parser_getsel(p), val);
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
    upb_status_seterrf(p->status, "error parsing number: %s", buf);
    multipart_end(p);
    return false;
  }
}

static bool parser_putbool(upb_json_parser *p, bool val) {
  bool ok;

  if (p->top->f == NULL) {
    return true;
  }

  if (upb_fielddef_type(p->top->f) != UPB_TYPE_BOOL) {
    upb_status_seterrf(p->status,
                       "Boolean value specified for non-bool field: %s",
                       upb_fielddef_name(p->top->f));
    return false;
  }

  ok = upb_sink_putbool(p->top->sink, parser_getsel(p), val);
  UPB_ASSERT(ok);

  return true;
}

static bool end_bool(upb_json_parser *p, bool val) {
  if (is_top_level(p)) {
    if (is_wellknown_msg(p, UPB_WELLKNOWN_BOOLVALUE)) {
      start_wrapper_object(p);
    } else if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
      start_value_object(p, VALUE_BOOLVALUE);
    } else {
      return false;
    }
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_BOOLVALUE)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_wrapper_object(p);
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_VALUE)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_value_object(p, VALUE_BOOLVALUE);
  }

  if (p->top->is_unknown_field) {
    return true;
  }

  if (!parser_putbool(p, val)) {
    return false;
  }

  if (is_wellknown_msg(p, UPB_WELLKNOWN_BOOLVALUE)) {
    end_wrapper_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
    return true;
  }

  if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
    end_value_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
    return true;
  }

  return true;
}

static bool end_null(upb_json_parser *p) {
  const char *zero_ptr = "0";

  if (is_top_level(p)) {
    if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
      start_value_object(p, VALUE_NULLVALUE);
    } else {
      return true;
    }
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_VALUE)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_value_object(p, VALUE_NULLVALUE);
  } else {
    return true;
  }

  /* Fill null_value field. */
  multipart_startaccum(p);
  capture_begin(p, zero_ptr);
  capture_end(p, zero_ptr + 1);
  parse_number(p, false);

  end_value_object(p);
  if (!is_top_level(p)) {
    end_subobject(p);
  }

  return true;
}

static bool start_any_stringval(upb_json_parser *p) {
  multipart_startaccum(p);
  return true;
}

static bool start_stringval(upb_json_parser *p) {
  if (is_top_level(p)) {
    if (is_string_wrapper_object(p) ||
        is_number_wrapper_object(p)) {
      start_wrapper_object(p);
    } else if (is_wellknown_msg(p, UPB_WELLKNOWN_FIELDMASK)) {
      start_fieldmask_object(p);
      return true;
    } else if (is_wellknown_msg(p, UPB_WELLKNOWN_TIMESTAMP) ||
               is_wellknown_msg(p, UPB_WELLKNOWN_DURATION)) {
      start_object(p);
    } else if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
      start_value_object(p, VALUE_STRINGVALUE);
    } else {
      return false;
    }
  } else if (does_string_wrapper_start(p) ||
             does_number_wrapper_start(p)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_wrapper_object(p);
  } else if (does_fieldmask_start(p)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_fieldmask_object(p);
    return true;
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_TIMESTAMP) ||
             is_wellknown_field(p, UPB_WELLKNOWN_DURATION)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_object(p);
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_VALUE)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_value_object(p, VALUE_STRINGVALUE);
  }

  if (p->top->f == NULL) {
    multipart_startaccum(p);
    return true;
  }

  if (p->top->is_any) {
    return start_any_stringval(p);
  }

  if (upb_fielddef_isstring(p->top->f)) {
    upb_jsonparser_frame *inner;
    upb_selector_t sel;

    if (!check_stack(p)) return false;

    /* Start a new parser frame: parser frames correspond one-to-one with
     * handler frames, and string events occur in a sub-frame. */
    inner = start_jsonparser_frame(p);
    sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
    upb_sink_startstr(p->top->sink, sel, 0, &inner->sink);
    inner->m = p->top->m;
    inner->f = p->top->f;
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
    upb_status_seterrf(p->status,
                       "String specified for bool or submessage field: %s",
                       upb_fielddef_name(p->top->f));
    return false;
  }
}

static bool end_any_stringval(upb_json_parser *p) {
  size_t len;
  const char *buf = accumulate_getptr(p, &len);

  /* Set type_url */
  upb_selector_t sel;
  upb_jsonparser_frame *inner;
  if (!check_stack(p)) return false;
  inner = p->top + 1;

  sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
  upb_sink_startstr(p->top->sink, sel, 0, &inner->sink);
  sel = getsel_for_handlertype(p, UPB_HANDLER_STRING);
  upb_sink_putstring(inner->sink, sel, buf, len, NULL);
  sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
  upb_sink_endstr(inner->sink, sel);

  multipart_end(p);

  /* Resolve type url */
  if (strncmp(buf, "type.googleapis.com/", 20) == 0 && len > 20) {
    const upb_msgdef *payload_type = NULL;
    buf += 20;
    len -= 20;

    payload_type = upb_symtab_lookupmsg2(p->symtab, buf, len);
    if (payload_type == NULL) {
      upb_status_seterrf(
          p->status, "Cannot find packed type: %.*s\n", (int)len, buf);
      return false;
    }

    json_parser_any_frame_set_payload_type(p, p->top->any_frame, payload_type);

    return true;
  } else {
    upb_status_seterrf(
        p->status, "Invalid type url: %.*s\n", (int)len, buf);
    return false;
  }
}

static bool end_stringval_nontop(upb_json_parser *p) {
  bool ok = true;

  if (is_wellknown_msg(p, UPB_WELLKNOWN_TIMESTAMP) ||
      is_wellknown_msg(p, UPB_WELLKNOWN_DURATION)) {
    multipart_end(p);
    return true;
  }

  if (p->top->f == NULL) {
    multipart_end(p);
    return true;
  }

  if (p->top->is_any) {
    return end_any_stringval(p);
  }

  switch (upb_fielddef_type(p->top->f)) {
    case UPB_TYPE_BYTES:
      if (!base64_push(p, getsel_for_handlertype(p, UPB_HANDLER_STRING),
                       p->accumulated, p->accumulated_len)) {
        return false;
      }
      /* Fall through. */

    case UPB_TYPE_STRING: {
      upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
      upb_sink_endstr(p->top->sink, sel);
      p->top--;
      break;
    }

    case UPB_TYPE_ENUM: {
      /* Resolve enum symbolic name to integer value. */
      const upb_enumdef *enumdef = upb_fielddef_enumsubdef(p->top->f);

      size_t len;
      const char *buf = accumulate_getptr(p, &len);

      int32_t int_val = 0;
      ok = upb_enumdef_ntoi(enumdef, buf, len, &int_val);

      if (ok) {
        upb_selector_t sel = parser_getsel(p);
        upb_sink_putint32(p->top->sink, sel, int_val);
      } else {
        upb_status_seterrf(p->status, "Enum value unknown: '%.*s'", len, buf);
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
      upb_status_seterrmsg(p->status, "Internal error in JSON decoder");
      ok = false;
      break;
  }

  multipart_end(p);

  return ok;
}

static bool end_stringval(upb_json_parser *p) {
  /* FieldMask's stringvals have been ended when handling them. Only need to
   * close FieldMask here.*/
  if (does_fieldmask_end(p)) {
    end_fieldmask_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
    return true;
  }

  if (!end_stringval_nontop(p)) {
    return false;
  }

  if (does_string_wrapper_end(p) ||
      does_number_wrapper_end(p)) {
    end_wrapper_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
    return true;
  }

  if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
    end_value_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
    return true;
  }

  if (is_wellknown_msg(p, UPB_WELLKNOWN_TIMESTAMP) ||
      is_wellknown_msg(p, UPB_WELLKNOWN_DURATION) ||
      is_wellknown_msg(p, UPB_WELLKNOWN_FIELDMASK)) {
    end_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
    return true;
  }

  return true;
}

static void start_duration_base(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_duration_base(upb_json_parser *p, const char *ptr) {
  size_t len;
  const char *buf;
  char seconds_buf[14];
  char nanos_buf[12];
  char *end;
  int64_t seconds = 0;
  int32_t nanos = 0;
  double val = 0.0;
  const char *seconds_membername = "seconds";
  const char *nanos_membername = "nanos";
  size_t fraction_start;

  if (!capture_end(p, ptr)) {
    return false;
  }

  buf = accumulate_getptr(p, &len);

  memset(seconds_buf, 0, 14);
  memset(nanos_buf, 0, 12);

  /* Find out base end. The maximus duration is 315576000000, which cannot be
   * represented by double without losing precision. Thus, we need to handle
   * fraction and base separately. */
  for (fraction_start = 0; fraction_start < len && buf[fraction_start] != '.';
       fraction_start++);

  /* Parse base */
  memcpy(seconds_buf, buf, fraction_start);
  seconds = strtol(seconds_buf, &end, 10);
  if (errno == ERANGE || end != seconds_buf + fraction_start) {
    upb_status_seterrf(p->status, "error parsing duration: %s",
                       seconds_buf);
    return false;
  }

  if (seconds > 315576000000) {
    upb_status_seterrf(p->status, "error parsing duration: "
                                   "maximum acceptable value is "
                                   "315576000000");
    return false;
  }

  if (seconds < -315576000000) {
    upb_status_seterrf(p->status, "error parsing duration: "
                                   "minimum acceptable value is "
                                   "-315576000000");
    return false;
  }

  /* Parse fraction */
  nanos_buf[0] = '0';
  memcpy(nanos_buf + 1, buf + fraction_start, len - fraction_start);
  val = strtod(nanos_buf, &end);
  if (errno == ERANGE || end != nanos_buf + len - fraction_start + 1) {
    upb_status_seterrf(p->status, "error parsing duration: %s",
                       nanos_buf);
    return false;
  }

  nanos = val * 1000000000;
  if (seconds < 0) nanos = -nanos;

  /* Clean up buffer */
  multipart_end(p);

  /* Set seconds */
  start_member(p);
  capture_begin(p, seconds_membername);
  capture_end(p, seconds_membername + 7);
  end_membername(p);
  upb_sink_putint64(p->top->sink, parser_getsel(p), seconds);
  end_member(p);

  /* Set nanos */
  start_member(p);
  capture_begin(p, nanos_membername);
  capture_end(p, nanos_membername + 5);
  end_membername(p);
  upb_sink_putint32(p->top->sink, parser_getsel(p), nanos);
  end_member(p);

  /* Continue previous arena */
  multipart_startaccum(p);

  return true;
}

static int parse_timestamp_number(upb_json_parser *p) {
  size_t len;
  const char *buf;
  int val;

  /* atoi() and friends unfortunately do not support specifying the length of
   * the input string, so we need to force a copy into a NULL-terminated buffer. */
  multipart_text(p, "\0", 1, false);

  buf = accumulate_getptr(p, &len);
  val = atoi(buf);
  multipart_end(p);
  multipart_startaccum(p);

  return val;
}

static void start_year(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_year(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }
  p->tm.tm_year = parse_timestamp_number(p) - 1900;
  return true;
}

static void start_month(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_month(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }
  p->tm.tm_mon = parse_timestamp_number(p) - 1;
  return true;
}

static void start_day(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_day(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }
  p->tm.tm_mday = parse_timestamp_number(p);
  return true;
}

static void start_hour(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_hour(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }
  p->tm.tm_hour = parse_timestamp_number(p);
  return true;
}

static void start_minute(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_minute(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }
  p->tm.tm_min = parse_timestamp_number(p);
  return true;
}

static void start_second(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_second(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }
  p->tm.tm_sec = parse_timestamp_number(p);
  return true;
}

static void start_timestamp_base(upb_json_parser *p) {
  memset(&p->tm, 0, sizeof(struct tm));
}

static void start_timestamp_fraction(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_timestamp_fraction(upb_json_parser *p, const char *ptr) {
  size_t len;
  const char *buf;
  char nanos_buf[12];
  char *end;
  double val = 0.0;
  int32_t nanos;
  const char *nanos_membername = "nanos";

  memset(nanos_buf, 0, 12);

  if (!capture_end(p, ptr)) {
    return false;
  }

  buf = accumulate_getptr(p, &len);

  if (len > 10) {
    upb_status_seterrf(p->status,
        "error parsing timestamp: at most 9-digit fraction.");
    return false;
  }

  /* Parse nanos */
  nanos_buf[0] = '0';
  memcpy(nanos_buf + 1, buf, len);
  val = strtod(nanos_buf, &end);

  if (errno == ERANGE || end != nanos_buf + len + 1) {
    upb_status_seterrf(p->status, "error parsing timestamp nanos: %s",
                       nanos_buf);
    return false;
  }

  nanos = val * 1000000000;

  /* Clean up previous environment */
  multipart_end(p);

  /* Set nanos */
  start_member(p);
  capture_begin(p, nanos_membername);
  capture_end(p, nanos_membername + 5);
  end_membername(p);
  upb_sink_putint32(p->top->sink, parser_getsel(p), nanos);
  end_member(p);

  /* Continue previous environment */
  multipart_startaccum(p);

  return true;
}

static void start_timestamp_zone(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

#define EPOCH_YEAR 1970
#define TM_YEAR_BASE 1900

static bool isleap(int year) {
  return (year % 4) == 0 && (year % 100 != 0 || (year % 400) == 0);
}

const unsigned short int __mon_yday[2][13] = {
    /* Normal years.  */
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
    /* Leap years.  */
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

/* epoch_days(1970, 1, 1) == 1970-01-01 == 0. */
static int epoch_days(int year, int month, int day) {
  static const uint16_t month_yday[12] = {0,   31,  59,  90,  120, 151,
                                          181, 212, 243, 273, 304, 334};
  int febs_since_0 = month > 2 ? year + 1 : year;
  int leap_days_since_0 = div_round_up(febs_since_0, 4) -
                          div_round_up(febs_since_0, 100) +
                          div_round_up(febs_since_0, 400);
  int days_since_0 =
      365 * year + month_yday[month - 1] + (day - 1) + leap_days_since_0;

  /* Convert from 0-epoch (0001-01-01 BC) to Unix Epoch (1970-01-01 AD).
   * Since the "BC" system does not have a year zero, 1 BC == year zero. */
  return days_since_0 - 719528;
}

static int64_t upb_timegm(const struct tm *tp) {
  int64_t ret = epoch_days(tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday);
  ret = (ret * 24) + tp->tm_hour;
  ret = (ret * 60) + tp->tm_min;
  ret = (ret * 60) + tp->tm_sec;
  return ret;
}

static bool end_timestamp_zone(upb_json_parser *p, const char *ptr) {
  size_t len;
  const char *buf;
  int hours;
  int64_t seconds;
  const char *seconds_membername = "seconds";

  if (!capture_end(p, ptr)) {
    return false;
  }

  buf = accumulate_getptr(p, &len);

  if (buf[0] != 'Z') {
    if (sscanf(buf + 1, "%2d:00", &hours) != 1) {
      upb_status_seterrf(p->status, "error parsing timestamp offset");
      return false;
    }

    if (buf[0] == '+') {
      hours = -hours;
    }

    p->tm.tm_hour += hours;
  }

  /* Normalize tm */
  seconds = upb_timegm(&p->tm);

  /* Check timestamp boundary */
  if (seconds < -62135596800) {
    upb_status_seterrf(p->status, "error parsing timestamp: "
                                   "minimum acceptable value is "
                                   "0001-01-01T00:00:00Z");
    return false;
  }

  /* Clean up previous environment */
  multipart_end(p);

  /* Set seconds */
  start_member(p);
  capture_begin(p, seconds_membername);
  capture_end(p, seconds_membername + 7);
  end_membername(p);
  upb_sink_putint64(p->top->sink, parser_getsel(p), seconds);
  end_member(p);

  /* Continue previous environment */
  multipart_startaccum(p);

  return true;
}

static void start_fieldmask_path_text(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_fieldmask_path_text(upb_json_parser *p, const char *ptr) {
  return capture_end(p, ptr);
}

static bool start_fieldmask_path(upb_json_parser *p) {
  upb_jsonparser_frame *inner;
  upb_selector_t sel;

  if (!check_stack(p)) return false;

  /* Start a new parser frame: parser frames correspond one-to-one with
   * handler frames, and string events occur in a sub-frame. */
  inner = start_jsonparser_frame(p);
  sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
  upb_sink_startstr(p->top->sink, sel, 0, &inner->sink);
  inner->m = p->top->m;
  inner->f = p->top->f;
  p->top = inner;

  multipart_startaccum(p);
  return true;
}

static bool lower_camel_push(
    upb_json_parser *p, upb_selector_t sel, const char *ptr, size_t len) {
  const char *limit = ptr + len;
  bool first = true;
  for (;ptr < limit; ptr++) {
    if (*ptr >= 'A' && *ptr <= 'Z' && !first) {
      char lower = tolower(*ptr);
      upb_sink_putstring(p->top->sink, sel, "_", 1, NULL);
      upb_sink_putstring(p->top->sink, sel, &lower, 1, NULL);
    } else {
      upb_sink_putstring(p->top->sink, sel, ptr, 1, NULL);
    }
    first = false;
  }
  return true;
}

static bool end_fieldmask_path(upb_json_parser *p) {
  upb_selector_t sel;

  if (!lower_camel_push(
           p, getsel_for_handlertype(p, UPB_HANDLER_STRING),
           p->accumulated, p->accumulated_len)) {
    return false;
  }

  sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
  upb_sink_endstr(p->top->sink, sel);
  p->top--;

  multipart_end(p);
  return true;
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
    upb_status_seterrmsg(p->status, "mapentry message has no key");
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
        upb_status_seterrmsg(p->status,
                             "Map bool key not 'true' or 'false'");
        return false;
      }
      multipart_end(p);
      break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      upb_sink subsink;
      upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
      upb_sink_startstr(p->top->sink, sel, len, &subsink);
      sel = getsel_for_handlertype(p, UPB_HANDLER_STRING);
      upb_sink_putstring(subsink, sel, buf, len, NULL);
      sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
      upb_sink_endstr(subsink, sel);
      multipart_end(p);
      break;
    }
    default:
      upb_status_seterrmsg(p->status, "Invalid field type for map key");
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

  inner = start_jsonparser_frame(p);
  p->top->f = mapfield;
  sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSUBMSG);
  upb_sink_startsubmsg(p->top->sink, sel, &inner->sink);
  inner->m = mapentrymsg;
  inner->mapfield = mapfield;

  /* Don't set this to true *yet* -- we reuse parsing handlers below to push
   * the key field value to the sink, and these handlers will pop the frame
   * if they see is_mapentry (when invoked by the parser state machine, they
   * would have just seen the map-entry value, not key). */
  inner->is_mapentry = false;
  p->top = inner;

  /* send STARTMSG in submsg frame. */
  upb_sink_startmsg(p->top->sink);

  parse_mapentry_key(p);

  /* Set up the value field to receive the map-entry value. */
  p->top->f = upb_msgdef_itof(p->top->m, UPB_MAPENTRY_VALUE);
  p->top->is_mapentry = true;  /* set up to pop frame after value is parsed. */
  p->top->mapfield = mapfield;
  if (p->top->f == NULL) {
    upb_status_seterrmsg(p->status, "mapentry message has no value");
    return false;
  }

  return true;
}

static bool end_membername(upb_json_parser *p) {
  UPB_ASSERT(!p->top->f);

  if (!p->top->m) {
    p->top->is_unknown_field = true;
    multipart_end(p);
    return true;
  }

  if (p->top->is_any) {
    return end_any_membername(p);
  } else if (p->top->is_map) {
    return handle_mapentry(p);
  } else {
    size_t len;
    const char *buf = accumulate_getptr(p, &len);
    upb_value v;

    if (upb_strtable_lookup2(p->top->name_table, buf, len, &v)) {
      p->top->f = upb_value_getconstptr(v);
      multipart_end(p);

      return true;
    } else if (p->ignore_json_unknown) {
      p->top->is_unknown_field = true;
      multipart_end(p);
      return true;
    } else {
      upb_status_seterrf(p->status, "No such field: %.*s\n", (int)len, buf);
      return false;
    }
  }
}

static bool end_any_membername(upb_json_parser *p) {
  size_t len;
  const char *buf = accumulate_getptr(p, &len);
  upb_value v;

  if (len == 5 && strncmp(buf, "@type", len) == 0) {
    upb_strtable_lookup2(p->top->name_table, "type_url", 8, &v);
    p->top->f = upb_value_getconstptr(v);
    multipart_end(p);
    return true;
  } else {
    p->top->is_unknown_field = true;
    multipart_end(p);
    return true;
  }
}

static void end_member(upb_json_parser *p) {
  /* If we just parsed a map-entry value, end that frame too. */
  if (p->top->is_mapentry) {
    upb_selector_t sel;
    bool ok;
    const upb_fielddef *mapfield;

    UPB_ASSERT(p->top > p->stack);
    /* send ENDMSG on submsg. */
    upb_sink_endmsg(p->top->sink, p->status);
    mapfield = p->top->mapfield;

    /* send ENDSUBMSG in repeated-field-of-mapentries frame. */
    p->top--;
    ok = upb_handlers_getselector(mapfield, UPB_HANDLER_ENDSUBMSG, &sel);
    UPB_ASSERT(ok);
    upb_sink_endsubmsg(p->top->sink, sel);
  }

  p->top->f = NULL;
  p->top->is_unknown_field = false;
}

static void start_any_member(upb_json_parser *p, const char *ptr) {
  start_member(p);
  json_parser_any_frame_set_after_type_url_start_once(p->top->any_frame, ptr);
}

static void end_any_member(upb_json_parser *p, const char *ptr) {
  json_parser_any_frame_set_before_type_url_end(p->top->any_frame, ptr);
  end_member(p);
}

static bool start_subobject(upb_json_parser *p) {
  if (p->top->is_unknown_field) {
    if (!check_stack(p)) return false;

    p->top = start_jsonparser_frame(p);
    return true;
  }

  if (upb_fielddef_ismap(p->top->f)) {
    upb_jsonparser_frame *inner;
    upb_selector_t sel;

    /* Beginning of a map. Start a new parser frame in a repeated-field
     * context. */
    if (!check_stack(p)) return false;

    inner = start_jsonparser_frame(p);
    sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSEQ);
    upb_sink_startseq(p->top->sink, sel, &inner->sink);
    inner->m = upb_fielddef_msgsubdef(p->top->f);
    inner->mapfield = p->top->f;
    inner->is_map = true;
    p->top = inner;

    return true;
  } else if (upb_fielddef_issubmsg(p->top->f)) {
    upb_jsonparser_frame *inner;
    upb_selector_t sel;

    /* Beginning of a subobject. Start a new parser frame in the submsg
     * context. */
    if (!check_stack(p)) return false;

    inner = start_jsonparser_frame(p);
    sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSUBMSG);
    upb_sink_startsubmsg(p->top->sink, sel, &inner->sink);
    inner->m = upb_fielddef_msgsubdef(p->top->f);
    set_name_table(p, inner);
    p->top = inner;

    if (is_wellknown_msg(p, UPB_WELLKNOWN_ANY)) {
      p->top->is_any = true;
      p->top->any_frame = json_parser_any_frame_new(p);
    } else {
      p->top->is_any = false;
      p->top->any_frame = NULL;
    }

    return true;
  } else {
    upb_status_seterrf(p->status,
                       "Object specified for non-message/group field: %s",
                       upb_fielddef_name(p->top->f));
    return false;
  }
}

static bool start_subobject_full(upb_json_parser *p) {
  if (is_top_level(p)) {
    if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
      start_value_object(p, VALUE_STRUCTVALUE);
      if (!start_subobject(p)) return false;
      start_structvalue_object(p);
    } else if (is_wellknown_msg(p, UPB_WELLKNOWN_STRUCT)) {
      start_structvalue_object(p);
    } else {
      return true;
    }
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_STRUCT)) {
    if (!start_subobject(p)) return false;
    start_structvalue_object(p);
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_VALUE)) {
    if (!start_subobject(p)) return false;
    start_value_object(p, VALUE_STRUCTVALUE);
    if (!start_subobject(p)) return false;
    start_structvalue_object(p);
  }

  return start_subobject(p);
}

static void end_subobject(upb_json_parser *p) {
  if (is_top_level(p)) {
    return;
  }

  if (p->top->is_map) {
    upb_selector_t sel;
    p->top--;
    sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSEQ);
    upb_sink_endseq(p->top->sink, sel);
  } else {
    upb_selector_t sel;
    bool is_unknown = p->top->m == NULL;
    p->top--;
    if (!is_unknown) {
      sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSUBMSG);
      upb_sink_endsubmsg(p->top->sink, sel);
    }
  }
}

static void end_subobject_full(upb_json_parser *p) {
  end_subobject(p);

  if (is_wellknown_msg(p, UPB_WELLKNOWN_STRUCT)) {
    end_structvalue_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
  }

  if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
    end_value_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
  }
}

static bool start_array(upb_json_parser *p) {
  upb_jsonparser_frame *inner;
  upb_selector_t sel;

  if (is_top_level(p)) {
    if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
      start_value_object(p, VALUE_LISTVALUE);
      if (!start_subobject(p)) return false;
      start_listvalue_object(p);
    } else if (is_wellknown_msg(p, UPB_WELLKNOWN_LISTVALUE)) {
      start_listvalue_object(p);
    } else {
      return false;
    }
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_LISTVALUE) &&
             (!upb_fielddef_isseq(p->top->f) ||
              p->top->is_repeated)) {
    if (!start_subobject(p)) return false;
    start_listvalue_object(p);
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_VALUE) &&
             (!upb_fielddef_isseq(p->top->f) ||
              p->top->is_repeated)) {
    if (!start_subobject(p)) return false;
    start_value_object(p, VALUE_LISTVALUE);
    if (!start_subobject(p)) return false;
    start_listvalue_object(p);
  }

  if (p->top->is_unknown_field) {
    inner = start_jsonparser_frame(p);
    inner->is_unknown_field = true;
    p->top = inner;

    return true;
  }

  if (!upb_fielddef_isseq(p->top->f)) {
    upb_status_seterrf(p->status,
                       "Array specified for non-repeated field: %s",
                       upb_fielddef_name(p->top->f));
    return false;
  }

  if (!check_stack(p)) return false;

  inner = start_jsonparser_frame(p);
  sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSEQ);
  upb_sink_startseq(p->top->sink, sel, &inner->sink);
  inner->m = p->top->m;
  inner->f = p->top->f;
  inner->is_repeated = true;
  p->top = inner;

  return true;
}

static void end_array(upb_json_parser *p) {
  upb_selector_t sel;

  UPB_ASSERT(p->top > p->stack);

  p->top--;

  if (p->top->is_unknown_field) {
    return;
  }

  sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSEQ);
  upb_sink_endseq(p->top->sink, sel);

  if (is_wellknown_msg(p, UPB_WELLKNOWN_LISTVALUE)) {
    end_listvalue_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
  }

  if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
    end_value_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
  }
}

static void start_object(upb_json_parser *p) {
  if (!p->top->is_map && p->top->m != NULL) {
    upb_sink_startmsg(p->top->sink);
  }
}

static void end_object(upb_json_parser *p) {
  if (!p->top->is_map && p->top->m != NULL) {
    upb_sink_endmsg(p->top->sink, p->status);
  }
}

static void start_any_object(upb_json_parser *p, const char *ptr) {
  start_object(p);
  p->top->any_frame->before_type_url_start = ptr;
  p->top->any_frame->before_type_url_end = ptr;
}

static bool end_any_object(upb_json_parser *p, const char *ptr) {
  const char *value_membername = "value";
  bool is_well_known_packed = false;
  const char *packed_end = ptr + 1;
  upb_selector_t sel;
  upb_jsonparser_frame *inner;

  if (json_parser_any_frame_has_value(p->top->any_frame) &&
      !json_parser_any_frame_has_type_url(p->top->any_frame)) {
    upb_status_seterrmsg(p->status, "No valid type url");
    return false;
  }

  /* Well known types data is represented as value field. */
  if (upb_msgdef_wellknowntype(p->top->any_frame->parser->top->m) !=
          UPB_WELLKNOWN_UNSPECIFIED) {
    is_well_known_packed = true;

    if (json_parser_any_frame_has_value_before_type_url(p->top->any_frame)) {
      p->top->any_frame->before_type_url_start =
          memchr(p->top->any_frame->before_type_url_start, ':',
                 p->top->any_frame->before_type_url_end -
                 p->top->any_frame->before_type_url_start);
      if (p->top->any_frame->before_type_url_start == NULL) {
        upb_status_seterrmsg(p->status, "invalid data for well known type.");
        return false;
      }
      p->top->any_frame->before_type_url_start++;
    }

    if (json_parser_any_frame_has_value_after_type_url(p->top->any_frame)) {
      p->top->any_frame->after_type_url_start =
          memchr(p->top->any_frame->after_type_url_start, ':',
                 (ptr + 1) -
                 p->top->any_frame->after_type_url_start);
      if (p->top->any_frame->after_type_url_start == NULL) {
        upb_status_seterrmsg(p->status, "Invalid data for well known type.");
        return false;
      }
      p->top->any_frame->after_type_url_start++;
      packed_end = ptr;
    }
  }

  if (json_parser_any_frame_has_value_before_type_url(p->top->any_frame)) {
    if (!parse(p->top->any_frame->parser, NULL,
               p->top->any_frame->before_type_url_start,
               p->top->any_frame->before_type_url_end -
               p->top->any_frame->before_type_url_start, NULL)) {
      return false;
    }
  } else {
    if (!is_well_known_packed) {
      if (!parse(p->top->any_frame->parser, NULL, "{", 1, NULL)) {
        return false;
      }
    }
  }

  if (json_parser_any_frame_has_value_before_type_url(p->top->any_frame) &&
      json_parser_any_frame_has_value_after_type_url(p->top->any_frame)) {
    if (!parse(p->top->any_frame->parser, NULL, ",", 1, NULL)) {
      return false;
    }
  }

  if (json_parser_any_frame_has_value_after_type_url(p->top->any_frame)) {
    if (!parse(p->top->any_frame->parser, NULL,
               p->top->any_frame->after_type_url_start,
               packed_end - p->top->any_frame->after_type_url_start, NULL)) {
      return false;
    }
  } else {
    if (!is_well_known_packed) {
      if (!parse(p->top->any_frame->parser, NULL, "}", 1, NULL)) {
        return false;
      }
    }
  }

  if (!end(p->top->any_frame->parser, NULL)) {
    return false;
  }

  p->top->is_any = false;

  /* Set value */
  start_member(p);
  capture_begin(p, value_membername);
  capture_end(p, value_membername + 5);
  end_membername(p);

  if (!check_stack(p)) return false;
  inner = p->top + 1;

  sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
  upb_sink_startstr(p->top->sink, sel, 0, &inner->sink);
  sel = getsel_for_handlertype(p, UPB_HANDLER_STRING);
  upb_sink_putstring(inner->sink, sel, p->top->any_frame->stringsink.ptr,
                     p->top->any_frame->stringsink.len, NULL);
  sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
  upb_sink_endstr(inner->sink, sel);

  end_member(p);

  end_object(p);

  /* Deallocate any parse frame. */
  json_parser_any_frame_free(p->top->any_frame);

  return true;
}

static bool is_string_wrapper(const upb_msgdef *m) {
  upb_wellknowntype_t type = upb_msgdef_wellknowntype(m);
  return type == UPB_WELLKNOWN_STRINGVALUE ||
         type == UPB_WELLKNOWN_BYTESVALUE;
}

static bool is_fieldmask(const upb_msgdef *m) {
  upb_wellknowntype_t type = upb_msgdef_wellknowntype(m);
  return type == UPB_WELLKNOWN_FIELDMASK;
}

static void start_fieldmask_object(upb_json_parser *p) {
  const char *membername = "paths";

  start_object(p);

  /* Set up context for parsing value */
  start_member(p);
  capture_begin(p, membername);
  capture_end(p, membername + 5);
  end_membername(p);

  start_array(p);
}

static void end_fieldmask_object(upb_json_parser *p) {
  end_array(p);
  end_member(p);
  end_object(p);
}

static void start_wrapper_object(upb_json_parser *p) {
  const char *membername = "value";

  start_object(p);

  /* Set up context for parsing value */
  start_member(p);
  capture_begin(p, membername);
  capture_end(p, membername + 5);
  end_membername(p);
}

static void end_wrapper_object(upb_json_parser *p) {
  end_member(p);
  end_object(p);
}

static void start_value_object(upb_json_parser *p, int value_type) {
  const char *nullmember = "null_value";
  const char *numbermember = "number_value";
  const char *stringmember = "string_value";
  const char *boolmember = "bool_value";
  const char *structmember = "struct_value";
  const char *listmember = "list_value";
  const char *membername = "";

  switch (value_type) {
    case VALUE_NULLVALUE:
      membername = nullmember;
      break;
    case VALUE_NUMBERVALUE:
      membername = numbermember;
      break;
    case VALUE_STRINGVALUE:
      membername = stringmember;
      break;
    case VALUE_BOOLVALUE:
      membername = boolmember;
      break;
    case VALUE_STRUCTVALUE:
      membername = structmember;
      break;
    case VALUE_LISTVALUE:
      membername = listmember;
      break;
  }

  start_object(p);

  /* Set up context for parsing value */
  start_member(p);
  capture_begin(p, membername);
  capture_end(p, membername + strlen(membername));
  end_membername(p);
}

static void end_value_object(upb_json_parser *p) {
  end_member(p);
  end_object(p);
}

static void start_listvalue_object(upb_json_parser *p) {
  const char *membername = "values";

  start_object(p);

  /* Set up context for parsing value */
  start_member(p);
  capture_begin(p, membername);
  capture_end(p, membername + strlen(membername));
  end_membername(p);
}

static void end_listvalue_object(upb_json_parser *p) {
  end_member(p);
  end_object(p);
}

static void start_structvalue_object(upb_json_parser *p) {
  const char *membername = "fields";

  start_object(p);

  /* Set up context for parsing value */
  start_member(p);
  capture_begin(p, membername);
  capture_end(p, membername + strlen(membername));
  end_membername(p);
}

static void end_structvalue_object(upb_json_parser *p) {
  end_member(p);
  end_object(p);
}

static bool is_top_level(upb_json_parser *p) {
  return p->top == p->stack && p->top->f == NULL && !p->top->is_unknown_field;
}

static bool is_wellknown_msg(upb_json_parser *p, upb_wellknowntype_t type) {
  return p->top->m != NULL && upb_msgdef_wellknowntype(p->top->m) == type;
}

static bool is_wellknown_field(upb_json_parser *p, upb_wellknowntype_t type) {
  return p->top->f != NULL &&
         upb_fielddef_issubmsg(p->top->f) &&
         (upb_msgdef_wellknowntype(upb_fielddef_msgsubdef(p->top->f))
              == type);
}

static bool does_number_wrapper_start(upb_json_parser *p) {
  return p->top->f != NULL &&
         upb_fielddef_issubmsg(p->top->f) &&
         upb_msgdef_isnumberwrapper(upb_fielddef_msgsubdef(p->top->f));
}

static bool does_number_wrapper_end(upb_json_parser *p) {
  return p->top->m != NULL && upb_msgdef_isnumberwrapper(p->top->m);
}

static bool is_number_wrapper_object(upb_json_parser *p) {
  return p->top->m != NULL && upb_msgdef_isnumberwrapper(p->top->m);
}

static bool does_string_wrapper_start(upb_json_parser *p) {
  return p->top->f != NULL &&
         upb_fielddef_issubmsg(p->top->f) &&
         is_string_wrapper(upb_fielddef_msgsubdef(p->top->f));
}

static bool does_string_wrapper_end(upb_json_parser *p) {
  return p->top->m != NULL && is_string_wrapper(p->top->m);
}

static bool is_string_wrapper_object(upb_json_parser *p) {
  return p->top->m != NULL && is_string_wrapper(p->top->m);
}

static bool does_fieldmask_start(upb_json_parser *p) {
  return p->top->f != NULL &&
         upb_fielddef_issubmsg(p->top->f) &&
         is_fieldmask(upb_fielddef_msgsubdef(p->top->f));
}

static bool does_fieldmask_end(upb_json_parser *p) {
  return p->top->m != NULL && is_fieldmask(p->top->m);
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


#line 2794 "upb/json/parser.rl"



#line 2597 "upb/json/parser.c"
static const char _json_actions[] = {
	0, 1, 0, 1, 1, 1, 3, 1, 
	4, 1, 6, 1, 7, 1, 8, 1, 
	9, 1, 11, 1, 12, 1, 13, 1, 
	14, 1, 15, 1, 16, 1, 17, 1, 
	18, 1, 19, 1, 20, 1, 22, 1, 
	23, 1, 24, 1, 35, 1, 37, 1, 
	39, 1, 40, 1, 42, 1, 43, 1, 
	44, 1, 46, 1, 48, 1, 49, 1, 
	50, 1, 51, 1, 53, 1, 54, 2, 
	4, 9, 2, 5, 6, 2, 7, 3, 
	2, 7, 9, 2, 21, 26, 2, 25, 
	10, 2, 27, 28, 2, 29, 30, 2, 
	32, 34, 2, 33, 31, 2, 38, 36, 
	2, 40, 42, 2, 45, 2, 2, 46, 
	54, 2, 47, 36, 2, 49, 54, 2, 
	50, 54, 2, 51, 54, 2, 52, 41, 
	2, 53, 54, 3, 32, 34, 35, 4, 
	21, 26, 27, 28
};

static const short _json_key_offsets[] = {
	0, 0, 12, 13, 18, 23, 28, 29, 
	30, 31, 32, 33, 34, 35, 36, 37, 
	38, 43, 44, 48, 53, 58, 63, 67, 
	71, 74, 77, 79, 83, 87, 89, 91, 
	96, 98, 100, 109, 115, 121, 127, 133, 
	135, 139, 142, 144, 146, 149, 150, 154, 
	156, 158, 160, 162, 163, 165, 167, 168, 
	170, 172, 173, 175, 177, 178, 180, 182, 
	183, 185, 187, 191, 193, 195, 196, 197, 
	198, 199, 201, 206, 208, 210, 212, 221, 
	222, 222, 222, 227, 232, 237, 238, 239, 
	240, 241, 241, 242, 243, 244, 244, 245, 
	246, 247, 247, 252, 253, 257, 262, 267, 
	272, 276, 276, 279, 282, 285, 288, 291, 
	294, 294, 294, 294, 294, 294
};

static const char _json_trans_keys[] = {
	32, 34, 45, 91, 102, 110, 116, 123, 
	9, 13, 48, 57, 34, 32, 93, 125, 
	9, 13, 32, 44, 93, 9, 13, 32, 
	93, 125, 9, 13, 97, 108, 115, 101, 
	117, 108, 108, 114, 117, 101, 32, 34, 
	125, 9, 13, 34, 32, 58, 9, 13, 
	32, 93, 125, 9, 13, 32, 44, 125, 
	9, 13, 32, 44, 125, 9, 13, 32, 
	34, 9, 13, 45, 48, 49, 57, 48, 
	49, 57, 46, 69, 101, 48, 57, 69, 
	101, 48, 57, 43, 45, 48, 57, 48, 
	57, 48, 57, 46, 69, 101, 48, 57, 
	34, 92, 34, 92, 34, 47, 92, 98, 
	102, 110, 114, 116, 117, 48, 57, 65, 
	70, 97, 102, 48, 57, 65, 70, 97, 
	102, 48, 57, 65, 70, 97, 102, 48, 
	57, 65, 70, 97, 102, 34, 92, 45, 
	48, 49, 57, 48, 49, 57, 46, 115, 
	48, 57, 115, 48, 57, 34, 46, 115, 
	48, 57, 48, 57, 48, 57, 48, 57, 
	48, 57, 45, 48, 57, 48, 57, 45, 
	48, 57, 48, 57, 84, 48, 57, 48, 
	57, 58, 48, 57, 48, 57, 58, 48, 
	57, 48, 57, 43, 45, 46, 90, 48, 
	57, 48, 57, 58, 48, 48, 34, 48, 
	57, 43, 45, 90, 48, 57, 34, 44, 
	34, 44, 34, 44, 34, 45, 91, 102, 
	110, 116, 123, 48, 57, 34, 32, 93, 
	125, 9, 13, 32, 44, 93, 9, 13, 
	32, 93, 125, 9, 13, 97, 108, 115, 
	101, 117, 108, 108, 114, 117, 101, 32, 
	34, 125, 9, 13, 34, 32, 58, 9, 
	13, 32, 93, 125, 9, 13, 32, 44, 
	125, 9, 13, 32, 44, 125, 9, 13, 
	32, 34, 9, 13, 32, 9, 13, 32, 
	9, 13, 32, 9, 13, 32, 9, 13, 
	32, 9, 13, 32, 9, 13, 0
};

static const char _json_single_lengths[] = {
	0, 8, 1, 3, 3, 3, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	3, 1, 2, 3, 3, 3, 2, 2, 
	1, 3, 0, 2, 2, 0, 0, 3, 
	2, 2, 9, 0, 0, 0, 0, 2, 
	2, 1, 2, 0, 1, 1, 2, 0, 
	0, 0, 0, 1, 0, 0, 1, 0, 
	0, 1, 0, 0, 1, 0, 0, 1, 
	0, 0, 4, 0, 0, 1, 1, 1, 
	1, 0, 3, 2, 2, 2, 7, 1, 
	0, 0, 3, 3, 3, 1, 1, 1, 
	1, 0, 1, 1, 1, 0, 1, 1, 
	1, 0, 3, 1, 2, 3, 3, 3, 
	2, 0, 1, 1, 1, 1, 1, 1, 
	0, 0, 0, 0, 0, 0
};

static const char _json_range_lengths[] = {
	0, 2, 0, 1, 1, 1, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 0, 1, 1, 1, 1, 1, 1, 
	1, 0, 1, 1, 1, 1, 1, 1, 
	0, 0, 0, 3, 3, 3, 3, 0, 
	1, 1, 0, 1, 1, 0, 1, 1, 
	1, 1, 1, 0, 1, 1, 0, 1, 
	1, 0, 1, 1, 0, 1, 1, 0, 
	1, 1, 0, 1, 1, 0, 0, 0, 
	0, 1, 1, 0, 0, 0, 1, 0, 
	0, 0, 1, 1, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 1, 0, 1, 1, 1, 1, 
	1, 0, 1, 1, 1, 1, 1, 1, 
	0, 0, 0, 0, 0, 0
};

static const short _json_index_offsets[] = {
	0, 0, 11, 13, 18, 23, 28, 30, 
	32, 34, 36, 38, 40, 42, 44, 46, 
	48, 53, 55, 59, 64, 69, 74, 78, 
	82, 85, 89, 91, 95, 99, 101, 103, 
	108, 111, 114, 124, 128, 132, 136, 140, 
	143, 147, 150, 153, 155, 158, 160, 164, 
	166, 168, 170, 172, 174, 176, 178, 180, 
	182, 184, 186, 188, 190, 192, 194, 196, 
	198, 200, 202, 207, 209, 211, 213, 215, 
	217, 219, 221, 226, 229, 232, 235, 244, 
	246, 247, 248, 253, 258, 263, 265, 267, 
	269, 271, 272, 274, 276, 278, 279, 281, 
	283, 285, 286, 291, 293, 297, 302, 307, 
	312, 316, 317, 320, 323, 326, 329, 332, 
	335, 336, 337, 338, 339, 340
};

static const unsigned char _json_indicies[] = {
	0, 2, 3, 4, 5, 6, 7, 8, 
	0, 3, 1, 9, 1, 11, 12, 1, 
	11, 10, 13, 14, 12, 13, 1, 14, 
	1, 1, 14, 10, 15, 1, 16, 1, 
	17, 1, 18, 1, 19, 1, 20, 1, 
	21, 1, 22, 1, 23, 1, 24, 1, 
	25, 26, 27, 25, 1, 28, 1, 29, 
	30, 29, 1, 30, 1, 1, 30, 31, 
	32, 33, 34, 32, 1, 35, 36, 27, 
	35, 1, 36, 26, 36, 1, 37, 38, 
	39, 1, 38, 39, 1, 41, 42, 42, 
	40, 43, 1, 42, 42, 43, 40, 44, 
	44, 45, 1, 45, 1, 45, 40, 41, 
	42, 42, 39, 40, 47, 48, 46, 50, 
	51, 49, 52, 52, 52, 52, 52, 52, 
	52, 52, 53, 1, 54, 54, 54, 1, 
	55, 55, 55, 1, 56, 56, 56, 1, 
	57, 57, 57, 1, 59, 60, 58, 61, 
	62, 63, 1, 64, 65, 1, 66, 67, 
	1, 68, 1, 67, 68, 1, 69, 1, 
	66, 67, 65, 1, 70, 1, 71, 1, 
	72, 1, 73, 1, 74, 1, 75, 1, 
	76, 1, 77, 1, 78, 1, 79, 1, 
	80, 1, 81, 1, 82, 1, 83, 1, 
	84, 1, 85, 1, 86, 1, 87, 1, 
	88, 1, 89, 89, 90, 91, 1, 92, 
	1, 93, 1, 94, 1, 95, 1, 96, 
	1, 97, 1, 98, 1, 99, 99, 100, 
	98, 1, 102, 1, 101, 104, 105, 103, 
	1, 1, 101, 106, 107, 108, 109, 110, 
	111, 112, 107, 1, 113, 1, 114, 115, 
	117, 118, 1, 117, 116, 119, 120, 118, 
	119, 1, 120, 1, 1, 120, 116, 121, 
	1, 122, 1, 123, 1, 124, 1, 125, 
	126, 1, 127, 1, 128, 1, 129, 130, 
	1, 131, 1, 132, 1, 133, 134, 135, 
	136, 134, 1, 137, 1, 138, 139, 138, 
	1, 139, 1, 1, 139, 140, 141, 142, 
	143, 141, 1, 144, 145, 136, 144, 1, 
	145, 135, 145, 1, 146, 147, 147, 1, 
	148, 148, 1, 149, 149, 1, 150, 150, 
	1, 151, 151, 1, 152, 152, 1, 1, 
	1, 1, 1, 1, 1, 0
};

static const char _json_trans_targs[] = {
	1, 0, 2, 107, 3, 6, 10, 13, 
	16, 106, 4, 3, 106, 4, 5, 7, 
	8, 9, 108, 11, 12, 109, 14, 15, 
	110, 16, 17, 111, 18, 18, 19, 20, 
	21, 22, 111, 21, 22, 24, 25, 31, 
	112, 26, 28, 27, 29, 30, 33, 113, 
	34, 33, 113, 34, 32, 35, 36, 37, 
	38, 39, 33, 113, 34, 41, 42, 46, 
	42, 46, 43, 45, 44, 114, 48, 49, 
	50, 51, 52, 53, 54, 55, 56, 57, 
	58, 59, 60, 61, 62, 63, 64, 65, 
	66, 67, 73, 72, 68, 69, 70, 71, 
	72, 115, 74, 67, 72, 76, 116, 76, 
	116, 77, 79, 81, 82, 85, 90, 94, 
	98, 80, 117, 117, 83, 82, 80, 83, 
	84, 86, 87, 88, 89, 117, 91, 92, 
	93, 117, 95, 96, 97, 117, 98, 99, 
	105, 100, 100, 101, 102, 103, 104, 105, 
	103, 104, 117, 106, 106, 106, 106, 106, 
	106
};

static const unsigned char _json_trans_actions[] = {
	0, 0, 113, 107, 53, 0, 0, 0, 
	125, 59, 45, 0, 55, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 101, 51, 47, 0, 0, 45, 
	49, 49, 104, 0, 0, 0, 0, 0, 
	3, 0, 0, 0, 0, 0, 5, 15, 
	0, 0, 71, 7, 13, 0, 74, 9, 
	9, 9, 77, 80, 11, 37, 37, 37, 
	0, 0, 0, 39, 0, 41, 86, 0, 
	0, 0, 17, 19, 0, 21, 23, 0, 
	25, 27, 0, 29, 31, 0, 33, 35, 
	0, 135, 83, 135, 0, 0, 0, 0, 
	0, 92, 0, 89, 89, 98, 43, 0, 
	131, 95, 113, 107, 53, 0, 0, 0, 
	125, 59, 69, 110, 45, 0, 55, 0, 
	0, 0, 0, 0, 0, 119, 0, 0, 
	0, 122, 0, 0, 0, 116, 0, 101, 
	51, 47, 0, 0, 45, 49, 49, 104, 
	0, 0, 128, 0, 57, 63, 65, 61, 
	67
};

static const unsigned char _json_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 1, 0, 1, 0, 0, 1, 1, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 57, 63, 65, 61, 67, 
	0, 0, 0, 0, 0, 0
};

static const int json_start = 1;

static const int json_en_number_machine = 23;
static const int json_en_string_machine = 32;
static const int json_en_duration_machine = 40;
static const int json_en_timestamp_machine = 47;
static const int json_en_fieldmask_machine = 75;
static const int json_en_value_machine = 78;
static const int json_en_main = 1;


#line 2797 "upb/json/parser.rl"

size_t parse(void *closure, const void *hd, const char *buf, size_t size,
             const upb_bufhandle *handle) {
  upb_json_parser *parser = closure;

  /* Variables used by Ragel's generated code. */
  int cs = parser->current_state;
  int *stack = parser->parser_stack;
  int top = parser->parser_top;

  const char *p = buf;
  const char *pe = buf + size;
  const char *eof = &eof_ch;

  parser->handle = handle;

  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  capture_resume(parser, buf);

  
#line 2875 "upb/json/parser.c"
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
	case 1:
#line 2602 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
	case 2:
#line 2604 "upb/json/parser.rl"
	{ p--; {stack[top++] = cs; cs = 23;goto _again;} }
	break;
	case 3:
#line 2608 "upb/json/parser.rl"
	{ start_text(parser, p); }
	break;
	case 4:
#line 2609 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_text(parser, p)); }
	break;
	case 5:
#line 2615 "upb/json/parser.rl"
	{ start_hex(parser); }
	break;
	case 6:
#line 2616 "upb/json/parser.rl"
	{ hexdigit(parser, p); }
	break;
	case 7:
#line 2617 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_hex(parser)); }
	break;
	case 8:
#line 2623 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(escape(parser, p)); }
	break;
	case 9:
#line 2629 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
	case 10:
#line 2634 "upb/json/parser.rl"
	{ start_year(parser, p); }
	break;
	case 11:
#line 2635 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_year(parser, p)); }
	break;
	case 12:
#line 2639 "upb/json/parser.rl"
	{ start_month(parser, p); }
	break;
	case 13:
#line 2640 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_month(parser, p)); }
	break;
	case 14:
#line 2644 "upb/json/parser.rl"
	{ start_day(parser, p); }
	break;
	case 15:
#line 2645 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_day(parser, p)); }
	break;
	case 16:
#line 2649 "upb/json/parser.rl"
	{ start_hour(parser, p); }
	break;
	case 17:
#line 2650 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_hour(parser, p)); }
	break;
	case 18:
#line 2654 "upb/json/parser.rl"
	{ start_minute(parser, p); }
	break;
	case 19:
#line 2655 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_minute(parser, p)); }
	break;
	case 20:
#line 2659 "upb/json/parser.rl"
	{ start_second(parser, p); }
	break;
	case 21:
#line 2660 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_second(parser, p)); }
	break;
	case 22:
#line 2665 "upb/json/parser.rl"
	{ start_duration_base(parser, p); }
	break;
	case 23:
#line 2666 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_duration_base(parser, p)); }
	break;
	case 24:
#line 2668 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
	case 25:
#line 2673 "upb/json/parser.rl"
	{ start_timestamp_base(parser); }
	break;
	case 26:
#line 2675 "upb/json/parser.rl"
	{ start_timestamp_fraction(parser, p); }
	break;
	case 27:
#line 2676 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_timestamp_fraction(parser, p)); }
	break;
	case 28:
#line 2678 "upb/json/parser.rl"
	{ start_timestamp_zone(parser, p); }
	break;
	case 29:
#line 2679 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_timestamp_zone(parser, p)); }
	break;
	case 30:
#line 2681 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
	case 31:
#line 2686 "upb/json/parser.rl"
	{ start_fieldmask_path_text(parser, p); }
	break;
	case 32:
#line 2687 "upb/json/parser.rl"
	{ end_fieldmask_path_text(parser, p); }
	break;
	case 33:
#line 2692 "upb/json/parser.rl"
	{ start_fieldmask_path(parser); }
	break;
	case 34:
#line 2693 "upb/json/parser.rl"
	{ end_fieldmask_path(parser); }
	break;
	case 35:
#line 2699 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
	case 36:
#line 2704 "upb/json/parser.rl"
	{
        if (is_wellknown_msg(parser, UPB_WELLKNOWN_TIMESTAMP)) {
          {stack[top++] = cs; cs = 47;goto _again;}
        } else if (is_wellknown_msg(parser, UPB_WELLKNOWN_DURATION)) {
          {stack[top++] = cs; cs = 40;goto _again;}
        } else if (is_wellknown_msg(parser, UPB_WELLKNOWN_FIELDMASK)) {
          {stack[top++] = cs; cs = 75;goto _again;}
        } else {
          {stack[top++] = cs; cs = 32;goto _again;}
        }
      }
	break;
	case 37:
#line 2717 "upb/json/parser.rl"
	{ p--; {stack[top++] = cs; cs = 78;goto _again;} }
	break;
	case 38:
#line 2722 "upb/json/parser.rl"
	{
        if (is_wellknown_msg(parser, UPB_WELLKNOWN_ANY)) {
          start_any_member(parser, p);
        } else {
          start_member(parser);
        }
      }
	break;
	case 39:
#line 2729 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_membername(parser)); }
	break;
	case 40:
#line 2732 "upb/json/parser.rl"
	{
        if (is_wellknown_msg(parser, UPB_WELLKNOWN_ANY)) {
          end_any_member(parser, p);
        } else {
          end_member(parser);
        }
      }
	break;
	case 41:
#line 2743 "upb/json/parser.rl"
	{
        if (is_wellknown_msg(parser, UPB_WELLKNOWN_ANY)) {
          start_any_object(parser, p);
        } else {
          start_object(parser);
        }
      }
	break;
	case 42:
#line 2752 "upb/json/parser.rl"
	{
        if (is_wellknown_msg(parser, UPB_WELLKNOWN_ANY)) {
          CHECK_RETURN_TOP(end_any_object(parser, p));
        } else {
          end_object(parser);
        }
      }
	break;
	case 43:
#line 2764 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(start_array(parser)); }
	break;
	case 44:
#line 2768 "upb/json/parser.rl"
	{ end_array(parser); }
	break;
	case 45:
#line 2773 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(start_number(parser, p)); }
	break;
	case 46:
#line 2774 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_number(parser, p)); }
	break;
	case 47:
#line 2776 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(start_stringval(parser)); }
	break;
	case 48:
#line 2777 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_stringval(parser)); }
	break;
	case 49:
#line 2779 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_bool(parser, true)); }
	break;
	case 50:
#line 2781 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_bool(parser, false)); }
	break;
	case 51:
#line 2783 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_null(parser)); }
	break;
	case 52:
#line 2785 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(start_subobject_full(parser)); }
	break;
	case 53:
#line 2786 "upb/json/parser.rl"
	{ end_subobject_full(parser); }
	break;
	case 54:
#line 2791 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
#line 3199 "upb/json/parser.c"
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = _json_actions + _json_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 0:
#line 2600 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; 	if ( p == pe )
		goto _test_eof;
goto _again;} }
	break;
	case 46:
#line 2774 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_number(parser, p)); }
	break;
	case 49:
#line 2779 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_bool(parser, true)); }
	break;
	case 50:
#line 2781 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_bool(parser, false)); }
	break;
	case 51:
#line 2783 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_null(parser)); }
	break;
	case 53:
#line 2786 "upb/json/parser.rl"
	{ end_subobject_full(parser); }
	break;
#line 3241 "upb/json/parser.c"
		}
	}
	}

	_out: {}
	}

#line 2819 "upb/json/parser.rl"

  if (p != pe) {
    upb_status_seterrf(parser->status, "Parse error at '%.*s'\n", pe - p, p);
  } else {
    capture_suspend(parser, &p);
  }

error:
  /* Save parsing state back to parser. */
  parser->current_state = cs;
  parser->parser_top = top;

  return p - buf;
}

static bool end(void *closure, const void *hd) {
  upb_json_parser *parser = closure;

  /* Prevent compile warning on unused static constants. */
  UPB_UNUSED(json_start);
  UPB_UNUSED(json_en_duration_machine);
  UPB_UNUSED(json_en_fieldmask_machine);
  UPB_UNUSED(json_en_number_machine);
  UPB_UNUSED(json_en_string_machine);
  UPB_UNUSED(json_en_timestamp_machine);
  UPB_UNUSED(json_en_value_machine);
  UPB_UNUSED(json_en_main);

  parse(parser, hd, &eof_ch, 0, NULL);

  return parser->current_state >= 106;
}

static void json_parser_reset(upb_json_parser *p) {
  int cs;
  int top;

  p->top = p->stack;
  init_frame(p->top);

  /* Emit Ragel initialization of the parser. */
  
#line 3292 "upb/json/parser.c"
	{
	cs = json_start;
	top = 0;
	}

#line 2861 "upb/json/parser.rl"
  p->current_state = cs;
  p->parser_top = top;
  accumulate_clear(p);
  p->multipart_state = MULTIPART_INACTIVE;
  p->capture = NULL;
  p->accumulated = NULL;
}

static upb_json_parsermethod *parsermethod_new(upb_json_codecache *c,
                                               const upb_msgdef *md) {
  upb_msg_field_iter i;
  upb_alloc *alloc = upb_arena_alloc(c->arena);

  upb_json_parsermethod *m = upb_malloc(alloc, sizeof(*m));

  m->cache = c;

  upb_byteshandler_init(&m->input_handler_);
  upb_byteshandler_setstring(&m->input_handler_, parse, m);
  upb_byteshandler_setendstr(&m->input_handler_, end, m);

  upb_strtable_init2(&m->name_table, UPB_CTYPE_CONSTPTR, alloc);

  /* Build name_table */

  for(upb_msg_field_begin(&i, md);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    upb_value v = upb_value_constptr(f);
    char *buf;

    /* Add an entry for the JSON name. */
    size_t len = upb_fielddef_getjsonname(f, NULL, 0);
    buf = upb_malloc(alloc, len);
    upb_fielddef_getjsonname(f, buf, len);
    upb_strtable_insert3(&m->name_table, buf, strlen(buf), v, alloc);

    if (strcmp(buf, upb_fielddef_name(f)) != 0) {
      /* Since the JSON name is different from the regular field name, add an
       * entry for the raw name (compliant proto3 JSON parsers must accept
       * both). */
      const char *name = upb_fielddef_name(f);
      upb_strtable_insert3(&m->name_table, name, strlen(name), v, alloc);
    }
  }

  return m;
}

/* Public API *****************************************************************/

upb_json_parser *upb_json_parser_create(upb_arena *arena,
                                        const upb_json_parsermethod *method,
                                        const upb_symtab* symtab,
                                        upb_sink output,
                                        upb_status *status,
                                        bool ignore_json_unknown) {
#ifndef NDEBUG
  const size_t size_before = upb_arena_bytesallocated(arena);
#endif
  upb_json_parser *p = upb_arena_malloc(arena, sizeof(upb_json_parser));
  if (!p) return false;

  p->arena = arena;
  p->method = method;
  p->status = status;
  p->limit = p->stack + UPB_JSON_MAX_DEPTH;
  p->accumulate_buf = NULL;
  p->accumulate_buf_size = 0;
  upb_bytessink_reset(&p->input_, &method->input_handler_, p);

  json_parser_reset(p);
  p->top->sink = output;
  p->top->m = upb_handlers_msgdef(output.handlers);
  if (is_wellknown_msg(p, UPB_WELLKNOWN_ANY)) {
    p->top->is_any = true;
    p->top->any_frame = json_parser_any_frame_new(p);
  } else {
    p->top->is_any = false;
    p->top->any_frame = NULL;
  }
  set_name_table(p, p->top);
  p->symtab = symtab;

  p->ignore_json_unknown = ignore_json_unknown;

  /* If this fails, uncomment and increase the value in parser.h. */
  /* fprintf(stderr, "%zd\n", upb_arena_bytesallocated(arena) - size_before); */
  UPB_ASSERT_DEBUGVAR(upb_arena_bytesallocated(arena) - size_before <=
                      UPB_JSON_PARSER_SIZE);
  return p;
}

upb_bytessink upb_json_parser_input(upb_json_parser *p) {
  return p->input_;
}

const upb_byteshandler *upb_json_parsermethod_inputhandler(
    const upb_json_parsermethod *m) {
  return &m->input_handler_;
}

upb_json_codecache *upb_json_codecache_new(void) {
  upb_alloc *alloc;
  upb_json_codecache *c;

  c = upb_gmalloc(sizeof(*c));

  c->arena = upb_arena_new();
  alloc = upb_arena_alloc(c->arena);

  upb_inttable_init2(&c->methods, UPB_CTYPE_CONSTPTR, alloc);

  return c;
}

void upb_json_codecache_free(upb_json_codecache *c) {
  upb_arena_free(c->arena);
  upb_gfree(c);
}

const upb_json_parsermethod *upb_json_codecache_get(upb_json_codecache *c,
                                                    const upb_msgdef *md) {
  upb_json_parsermethod *m;
  upb_value v;
  upb_msg_field_iter i;
  upb_alloc *alloc = upb_arena_alloc(c->arena);

  if (upb_inttable_lookupptr(&c->methods, md, &v)) {
    return upb_value_getconstptr(v);
  }

  m = parsermethod_new(c, md);
  v = upb_value_constptr(m);

  if (!m) return NULL;
  if (!upb_inttable_insertptr2(&c->methods, md, v, alloc)) return NULL;

  /* Populate parser methods for all submessages, so the name tables will
   * be available during parsing. */
  for(upb_msg_field_begin(&i, md);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);

    if (upb_fielddef_issubmsg(f)) {
      const upb_msgdef *subdef = upb_fielddef_msgsubdef(f);
      const upb_json_parsermethod *sub_method =
          upb_json_codecache_get(c, subdef);

      if (!sub_method) return NULL;
    }
  }

  return m;
}
/*
** This currently uses snprintf() to format primitives, and could be optimized
** further.
*/


#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <time.h>


struct upb_json_printer {
  upb_sink input_;
  /* BytesSink closure. */
  void *subc_;
  upb_bytessink output_;

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

  /* To print timestamp, printer needs to cache its seconds and nanos values
   * and convert them when ending timestamp message. See comments of
   * printer_sethandlers_timestamp for more detail. */
  int64_t seconds;
  int32_t nanos;
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

typedef struct {
  bool preserve_fieldnames;
} upb_json_printercache;

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

/* Convert a null-terminated const char* to a string piece. */
strpc *newstrpc_str(upb_handlers *h, const char * str) {
  strpc * ret = upb_gmalloc(sizeof(*ret));
  ret->ptr = upb_gstrdup(str);
  ret->len = strlen(str);
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
  if (val == UPB_INFINITY) {
    CHKLENGTH(length >= strlen(inf));
    strcpy(buf, inf);
    return strlen(inf);
  } else if (val == -UPB_INFINITY) {
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

static size_t fmt_int64_as_number(long long val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%lld", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_uint64_as_number(
    unsigned long long val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%llu", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_int64_as_string(long long val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "\"%lld\"", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_uint64_as_string(
    unsigned long long val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "\"%llu\"", val);
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
    char data[64];                                                           \
    size_t length = fmt_func(val, data, sizeof(data));                       \
    UPB_UNUSED(handler_data);                                                \
    print_data(p, "\"", 1);                                                  \
    print_data(p, data, length);                                             \
    print_data(p, "\":", 2);                                                 \
    return true;                                                             \
  }

TYPE_HANDLERS(double,   fmt_double)
TYPE_HANDLERS(float,    fmt_float)
TYPE_HANDLERS(bool,     fmt_bool)
TYPE_HANDLERS(int32_t,  fmt_int64_as_number)
TYPE_HANDLERS(uint32_t, fmt_int64_as_number)
TYPE_HANDLERS(int64_t,  fmt_int64_as_string)
TYPE_HANDLERS(uint64_t, fmt_uint64_as_string)

/* double and float are not allowed to be map keys. */
TYPE_HANDLERS_MAPKEY(bool,     fmt_bool)
TYPE_HANDLERS_MAPKEY(int32_t,  fmt_int64_as_number)
TYPE_HANDLERS_MAPKEY(uint32_t, fmt_int64_as_number)
TYPE_HANDLERS_MAPKEY(int64_t,  fmt_int64_as_number)
TYPE_HANDLERS_MAPKEY(uint64_t, fmt_uint64_as_number)

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

  print_data(p, "\"", 1);

  while (remaining > 2) {
    if (limit - to < 4) {
      bytes = to - data;
      putstring(p, data, bytes);
      to = data;
    }

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
  hd->enumdef = upb_fielddef_enumsubdef(f);
  hd->keyname = newstrpc(h, f, preserve_fieldnames);
  upb_handlers_addcleanup(h, hd, upb_gfree);
  attr->handler_data = hd;
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

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;

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
      upb_handlerattr enum_attr = UPB_HANDLERATTR_INIT;
      set_enum_hd(h, value_field, preserve_fieldnames, &enum_attr);
      upb_handlers_setint32(h, value_field, mapvalue_enum, &enum_attr);
      break;
    }
    case UPB_TYPE_MESSAGE:
      /* No handler necessary -- the submsg handlers will print the message
       * as appropriate. */
      break;
  }
}

static bool putseconds(void *closure, const void *handler_data,
                       int64_t seconds) {
  upb_json_printer *p = closure;
  p->seconds = seconds;
  UPB_UNUSED(handler_data);
  return true;
}

static bool putnanos(void *closure, const void *handler_data,
                     int32_t nanos) {
  upb_json_printer *p = closure;
  p->nanos = nanos;
  UPB_UNUSED(handler_data);
  return true;
}

static void *scalar_startstr_nokey(void *closure, const void *handler_data,
                                   size_t size_hint) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(size_hint);
  print_data(p, "\"", 1);
  return p;
}

static size_t putstr_nokey(void *closure, const void *handler_data,
                           const char *str, size_t len,
                           const upb_bufhandle *handle) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(handle);
  print_data(p, "\"", 1);
  putstring(p, str, len);
  print_data(p, "\"", 1);
  return len + 2;
}

static void *startseq_nokey(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  p->depth_++;
  p->first_elem_[p->depth_] = true;
  print_data(p, "[", 1);
  return closure;
}

static void *startseq_fieldmask(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  p->depth_++;
  p->first_elem_[p->depth_] = true;
  return closure;
}

static bool endseq_fieldmask(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  p->depth_--;
  return true;
}

static void *repeated_startstr_fieldmask(
    void *closure, const void *handler_data,
    size_t size_hint) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(size_hint);
  print_comma(p);
  return p;
}

static size_t repeated_str_fieldmask(
    void *closure, const void *handler_data,
    const char *str, size_t len,
    const upb_bufhandle *handle) {
  const char* limit = str + len;
  bool upper = false;
  size_t result_len = 0;
  for (; str < limit; str++) {
    if (*str == '_') {
      upper = true;
      continue;
    }
    if (upper && *str >= 'a' && *str <= 'z') {
      char upper_char = toupper(*str);
      CHK(putstr(closure, handler_data, &upper_char, 1, handle));
    } else {
      CHK(putstr(closure, handler_data, str, 1, handle));
    }
    upper = false;
    result_len++;
  }
  return result_len;
}

static void *startmap_nokey(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  p->depth_++;
  p->first_elem_[p->depth_] = true;
  print_data(p, "{", 1);
  return closure;
}

static bool putnull(void *closure, const void *handler_data,
                    int32_t null) {
  upb_json_printer *p = closure;
  print_data(p, "null", 4);
  UPB_UNUSED(handler_data);
  UPB_UNUSED(null);
  return true;
}

static bool printer_startdurationmsg(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  if (p->depth_ == 0) {
    upb_bytessink_start(p->output_, 0, &p->subc_);
  }
  return true;
}

#define UPB_DURATION_MAX_JSON_LEN 23
#define UPB_DURATION_MAX_NANO_LEN 9

static bool printer_enddurationmsg(void *closure, const void *handler_data,
                                   upb_status *s) {
  upb_json_printer *p = closure;
  char buffer[UPB_DURATION_MAX_JSON_LEN];
  size_t base_len;
  size_t curr;
  size_t i;

  memset(buffer, 0, UPB_DURATION_MAX_JSON_LEN);

  if (p->seconds < -315576000000) {
    upb_status_seterrf(s, "error parsing duration: "
                          "minimum acceptable value is "
                          "-315576000000");
    return false;
  }

  if (p->seconds > 315576000000) {
    upb_status_seterrf(s, "error serializing duration: "
                          "maximum acceptable value is "
                          "315576000000");
    return false;
  }

  _upb_snprintf(buffer, sizeof(buffer), "%ld", (long)p->seconds);
  base_len = strlen(buffer);

  if (p->nanos != 0) {
    char nanos_buffer[UPB_DURATION_MAX_NANO_LEN + 3];
    _upb_snprintf(nanos_buffer, sizeof(nanos_buffer), "%.9f",
                  p->nanos / 1000000000.0);
    /* Remove trailing 0. */
    for (i = UPB_DURATION_MAX_NANO_LEN + 2;
         nanos_buffer[i] == '0'; i--) {
      nanos_buffer[i] = 0;
    }
    strcpy(buffer + base_len, nanos_buffer + 1);
  }

  curr = strlen(buffer);
  strcpy(buffer + curr, "s");

  p->seconds = 0;
  p->nanos = 0;

  print_data(p, "\"", 1);
  print_data(p, buffer, strlen(buffer));
  print_data(p, "\"", 1);

  if (p->depth_ == 0) {
    upb_bytessink_end(p->output_);
  }

  UPB_UNUSED(handler_data);
  return true;
}

static bool printer_starttimestampmsg(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  if (p->depth_ == 0) {
    upb_bytessink_start(p->output_, 0, &p->subc_);
  }
  return true;
}

#define UPB_TIMESTAMP_MAX_JSON_LEN 31
#define UPB_TIMESTAMP_BEFORE_NANO_LEN 19
#define UPB_TIMESTAMP_MAX_NANO_LEN 9

static bool printer_endtimestampmsg(void *closure, const void *handler_data,
                                    upb_status *s) {
  upb_json_printer *p = closure;
  char buffer[UPB_TIMESTAMP_MAX_JSON_LEN];
  time_t time = p->seconds;
  size_t curr;
  size_t i;
  size_t year_length =
      strftime(buffer, UPB_TIMESTAMP_MAX_JSON_LEN, "%Y", gmtime(&time));

  if (p->seconds < -62135596800) {
    upb_status_seterrf(s, "error parsing timestamp: "
                          "minimum acceptable value is "
                          "0001-01-01T00:00:00Z");
    return false;
  }

  if (p->seconds > 253402300799) {
    upb_status_seterrf(s, "error parsing timestamp: "
                          "maximum acceptable value is "
                          "9999-12-31T23:59:59Z");
    return false;
  }

  /* strftime doesn't guarantee 4 digits for year. Prepend 0 by ourselves. */
  for (i = 0; i < 4 - year_length; i++) {
    buffer[i] = '0';
  }

  strftime(buffer + (4 - year_length), UPB_TIMESTAMP_MAX_JSON_LEN,
           "%Y-%m-%dT%H:%M:%S", gmtime(&time));
  if (p->nanos != 0) {
    char nanos_buffer[UPB_TIMESTAMP_MAX_NANO_LEN + 3];
    _upb_snprintf(nanos_buffer, sizeof(nanos_buffer), "%.9f",
                  p->nanos / 1000000000.0);
    /* Remove trailing 0. */
    for (i = UPB_TIMESTAMP_MAX_NANO_LEN + 2;
         nanos_buffer[i] == '0'; i--) {
      nanos_buffer[i] = 0;
    }
    strcpy(buffer + UPB_TIMESTAMP_BEFORE_NANO_LEN, nanos_buffer + 1);
  }

  curr = strlen(buffer);
  strcpy(buffer + curr, "Z");

  p->seconds = 0;
  p->nanos = 0;

  print_data(p, "\"", 1);
  print_data(p, buffer, strlen(buffer));
  print_data(p, "\"", 1);

  if (p->depth_ == 0) {
    upb_bytessink_end(p->output_);
  }

  UPB_UNUSED(handler_data);
  UPB_UNUSED(s);
  return true;
}

static bool printer_startmsg_noframe(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  if (p->depth_ == 0) {
    upb_bytessink_start(p->output_, 0, &p->subc_);
  }
  return true;
}

static bool printer_endmsg_noframe(
    void *closure, const void *handler_data, upb_status *s) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(s);
  if (p->depth_ == 0) {
    upb_bytessink_end(p->output_);
  }
  return true;
}

static bool printer_startmsg_fieldmask(
    void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  if (p->depth_ == 0) {
    upb_bytessink_start(p->output_, 0, &p->subc_);
  }
  print_data(p, "\"", 1);
  return true;
}

static bool printer_endmsg_fieldmask(
    void *closure, const void *handler_data, upb_status *s) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(s);
  print_data(p, "\"", 1);
  if (p->depth_ == 0) {
    upb_bytessink_end(p->output_);
  }
  return true;
}

static void *scalar_startstr_onlykey(
    void *closure, const void *handler_data, size_t size_hint) {
  upb_json_printer *p = closure;
  UPB_UNUSED(size_hint);
  CHK(putkey(closure, handler_data));
  return p;
}

/* Set up handlers for an Any submessage. */
void printer_sethandlers_any(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);

  const upb_fielddef* type_field = upb_msgdef_itof(md, UPB_ANY_TYPE);
  const upb_fielddef* value_field = upb_msgdef_itof(md, UPB_ANY_VALUE);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;

  /* type_url's json name is "@type" */
  upb_handlerattr type_name_attr = UPB_HANDLERATTR_INIT;
  upb_handlerattr value_name_attr = UPB_HANDLERATTR_INIT;
  strpc *type_url_json_name = newstrpc_str(h, "@type");
  strpc *value_json_name = newstrpc_str(h, "value");

  type_name_attr.handler_data = type_url_json_name;
  value_name_attr.handler_data = value_json_name;

  /* Set up handlers. */
  upb_handlers_setstartmsg(h, printer_startmsg, &empty_attr);
  upb_handlers_setendmsg(h, printer_endmsg, &empty_attr);

  upb_handlers_setstartstr(h, type_field, scalar_startstr, &type_name_attr);
  upb_handlers_setstring(h, type_field, scalar_str, &empty_attr);
  upb_handlers_setendstr(h, type_field, scalar_endstr, &empty_attr);

  /* This is not the full and correct JSON encoding for the Any value field. It
   * requires further processing by the wrapper code based on the type URL.
   */
  upb_handlers_setstartstr(h, value_field, scalar_startstr_onlykey,
                           &value_name_attr);

  UPB_UNUSED(closure);
}

/* Set up handlers for a fieldmask submessage. */
void printer_sethandlers_fieldmask(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);
  const upb_fielddef* f = upb_msgdef_itof(md, 1);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;

  upb_handlers_setstartseq(h, f, startseq_fieldmask, &empty_attr);
  upb_handlers_setendseq(h, f, endseq_fieldmask, &empty_attr);

  upb_handlers_setstartmsg(h, printer_startmsg_fieldmask, &empty_attr);
  upb_handlers_setendmsg(h, printer_endmsg_fieldmask, &empty_attr);

  upb_handlers_setstartstr(h, f, repeated_startstr_fieldmask, &empty_attr);
  upb_handlers_setstring(h, f, repeated_str_fieldmask, &empty_attr);

  UPB_UNUSED(closure);
}

/* Set up handlers for a duration submessage. */
void printer_sethandlers_duration(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);

  const upb_fielddef* seconds_field =
      upb_msgdef_itof(md, UPB_DURATION_SECONDS);
  const upb_fielddef* nanos_field =
      upb_msgdef_itof(md, UPB_DURATION_NANOS);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;

  upb_handlers_setstartmsg(h, printer_startdurationmsg, &empty_attr);
  upb_handlers_setint64(h, seconds_field, putseconds, &empty_attr);
  upb_handlers_setint32(h, nanos_field, putnanos, &empty_attr);
  upb_handlers_setendmsg(h, printer_enddurationmsg, &empty_attr);

  UPB_UNUSED(closure);
}

/* Set up handlers for a timestamp submessage. Instead of printing fields
 * separately, the json representation of timestamp follows RFC 3339 */
void printer_sethandlers_timestamp(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);

  const upb_fielddef* seconds_field =
      upb_msgdef_itof(md, UPB_TIMESTAMP_SECONDS);
  const upb_fielddef* nanos_field =
      upb_msgdef_itof(md, UPB_TIMESTAMP_NANOS);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;

  upb_handlers_setstartmsg(h, printer_starttimestampmsg, &empty_attr);
  upb_handlers_setint64(h, seconds_field, putseconds, &empty_attr);
  upb_handlers_setint32(h, nanos_field, putnanos, &empty_attr);
  upb_handlers_setendmsg(h, printer_endtimestampmsg, &empty_attr);

  UPB_UNUSED(closure);
}

void printer_sethandlers_value(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);
  upb_msg_field_iter i;

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;

  upb_handlers_setstartmsg(h, printer_startmsg_noframe, &empty_attr);
  upb_handlers_setendmsg(h, printer_endmsg_noframe, &empty_attr);

  upb_msg_field_begin(&i, md);
  for(; !upb_msg_field_done(&i); upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);

    switch (upb_fielddef_type(f)) {
      case UPB_TYPE_ENUM:
        upb_handlers_setint32(h, f, putnull, &empty_attr);
        break;
      case UPB_TYPE_DOUBLE:
        upb_handlers_setdouble(h, f, putdouble, &empty_attr);
        break;
      case UPB_TYPE_STRING:
        upb_handlers_setstartstr(h, f, scalar_startstr_nokey, &empty_attr);
        upb_handlers_setstring(h, f, scalar_str, &empty_attr);
        upb_handlers_setendstr(h, f, scalar_endstr, &empty_attr);
        break;
      case UPB_TYPE_BOOL:
        upb_handlers_setbool(h, f, putbool, &empty_attr);
        break;
      case UPB_TYPE_MESSAGE:
        break;
      default:
        UPB_ASSERT(false);
        break;
    }
  }

  UPB_UNUSED(closure);
}

#define WRAPPER_SETHANDLERS(wrapper, type, putmethod)                      \
void printer_sethandlers_##wrapper(const void *closure, upb_handlers *h) { \
  const upb_msgdef *md = upb_handlers_msgdef(h);                           \
  const upb_fielddef* f = upb_msgdef_itof(md, 1);                          \
  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;                \
  upb_handlers_setstartmsg(h, printer_startmsg_noframe, &empty_attr);      \
  upb_handlers_setendmsg(h, printer_endmsg_noframe, &empty_attr);          \
  upb_handlers_set##type(h, f, putmethod, &empty_attr);                    \
  UPB_UNUSED(closure);                                                     \
}

WRAPPER_SETHANDLERS(doublevalue, double, putdouble)
WRAPPER_SETHANDLERS(floatvalue,  float,  putfloat)
WRAPPER_SETHANDLERS(int64value,  int64,  putint64_t)
WRAPPER_SETHANDLERS(uint64value, uint64, putuint64_t)
WRAPPER_SETHANDLERS(int32value,  int32,  putint32_t)
WRAPPER_SETHANDLERS(uint32value, uint32, putuint32_t)
WRAPPER_SETHANDLERS(boolvalue,   bool,   putbool)
WRAPPER_SETHANDLERS(stringvalue, string, putstr_nokey)
WRAPPER_SETHANDLERS(bytesvalue,  string, putbytes)

#undef WRAPPER_SETHANDLERS

void printer_sethandlers_listvalue(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);
  const upb_fielddef* f = upb_msgdef_itof(md, 1);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;

  upb_handlers_setstartseq(h, f, startseq_nokey, &empty_attr);
  upb_handlers_setendseq(h, f, endseq, &empty_attr);

  upb_handlers_setstartmsg(h, printer_startmsg_noframe, &empty_attr);
  upb_handlers_setendmsg(h, printer_endmsg_noframe, &empty_attr);

  upb_handlers_setstartsubmsg(h, f, repeated_startsubmsg, &empty_attr);

  UPB_UNUSED(closure);
}

void printer_sethandlers_structvalue(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);
  const upb_fielddef* f = upb_msgdef_itof(md, 1);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;

  upb_handlers_setstartseq(h, f, startmap_nokey, &empty_attr);
  upb_handlers_setendseq(h, f, endmap, &empty_attr);

  upb_handlers_setstartmsg(h, printer_startmsg_noframe, &empty_attr);
  upb_handlers_setendmsg(h, printer_endmsg_noframe, &empty_attr);

  upb_handlers_setstartsubmsg(h, f, repeated_startsubmsg, &empty_attr);

  UPB_UNUSED(closure);
}

void printer_sethandlers(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);
  bool is_mapentry = upb_msgdef_mapentry(md);
  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;
  upb_msg_field_iter i;
  const upb_json_printercache *cache = closure;
  const bool preserve_fieldnames = cache->preserve_fieldnames;

  if (is_mapentry) {
    /* mapentry messages are sufficiently different that we handle them
     * separately. */
    printer_sethandlers_mapentry(closure, preserve_fieldnames, h);
    return;
  }

  switch (upb_msgdef_wellknowntype(md)) {
    case UPB_WELLKNOWN_UNSPECIFIED:
      break;
    case UPB_WELLKNOWN_ANY:
      printer_sethandlers_any(closure, h);
      return;
    case UPB_WELLKNOWN_FIELDMASK:
      printer_sethandlers_fieldmask(closure, h);
      return;
    case UPB_WELLKNOWN_DURATION:
      printer_sethandlers_duration(closure, h);
      return;
    case UPB_WELLKNOWN_TIMESTAMP:
      printer_sethandlers_timestamp(closure, h);
      return;
    case UPB_WELLKNOWN_VALUE:
      printer_sethandlers_value(closure, h);
      return;
    case UPB_WELLKNOWN_LISTVALUE:
      printer_sethandlers_listvalue(closure, h);
      return;
    case UPB_WELLKNOWN_STRUCT:
      printer_sethandlers_structvalue(closure, h);
      return;
#define WRAPPER(wellknowntype, name)        \
  case wellknowntype:                       \
    printer_sethandlers_##name(closure, h); \
    return;                                 \

    WRAPPER(UPB_WELLKNOWN_DOUBLEVALUE, doublevalue);
    WRAPPER(UPB_WELLKNOWN_FLOATVALUE, floatvalue);
    WRAPPER(UPB_WELLKNOWN_INT64VALUE, int64value);
    WRAPPER(UPB_WELLKNOWN_UINT64VALUE, uint64value);
    WRAPPER(UPB_WELLKNOWN_INT32VALUE, int32value);
    WRAPPER(UPB_WELLKNOWN_UINT32VALUE, uint32value);
    WRAPPER(UPB_WELLKNOWN_BOOLVALUE, boolvalue);
    WRAPPER(UPB_WELLKNOWN_STRINGVALUE, stringvalue);
    WRAPPER(UPB_WELLKNOWN_BYTESVALUE, bytesvalue);

#undef WRAPPER
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

    upb_handlerattr name_attr = UPB_HANDLERATTR_INIT;
    name_attr.handler_data = newstrpc(h, f, preserve_fieldnames);

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
        upb_handlerattr enum_attr = UPB_HANDLERATTR_INIT;
        set_enum_hd(h, f, preserve_fieldnames, &enum_attr);

        if (upb_fielddef_isseq(f)) {
          upb_handlers_setint32(h, f, repeated_enum, &enum_attr);
        } else {
          upb_handlers_setint32(h, f, scalar_enum, &enum_attr);
        }

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
  }

#undef TYPE
}

static void json_printer_reset(upb_json_printer *p) {
  p->depth_ = 0;
}


/* Public API *****************************************************************/

upb_json_printer *upb_json_printer_create(upb_arena *a, const upb_handlers *h,
                                          upb_bytessink output) {
#ifndef NDEBUG
  size_t size_before = upb_arena_bytesallocated(a);
#endif

  upb_json_printer *p = upb_arena_malloc(a, sizeof(upb_json_printer));
  if (!p) return NULL;

  p->output_ = output;
  json_printer_reset(p);
  upb_sink_reset(&p->input_, h, p);
  p->seconds = 0;
  p->nanos = 0;

  /* If this fails, increase the value in printer.h. */
  UPB_ASSERT_DEBUGVAR(upb_arena_bytesallocated(a) - size_before <=
                      UPB_JSON_PRINTER_SIZE);
  return p;
}

upb_sink upb_json_printer_input(upb_json_printer *p) {
  return p->input_;
}

upb_handlercache *upb_json_printer_newcache(bool preserve_proto_fieldnames) {
  upb_json_printercache *cache = upb_gmalloc(sizeof(*cache));
  upb_handlercache *ret = upb_handlercache_new(printer_sethandlers, cache);

  cache->preserve_fieldnames = preserve_proto_fieldnames;
  upb_handlercache_addcleanup(ret, cache, upb_gfree);

  return ret;
}
/* See port_def.inc.  This should #undef all macros #defined there. */

#undef UPB_SIZE
#undef UPB_FIELD_AT
#undef UPB_READ_ONEOF
#undef UPB_WRITE_ONEOF
#undef UPB_INLINE
#undef UPB_FORCEINLINE
#undef UPB_NOINLINE
#undef UPB_NORETURN
#undef UPB_MAX
#undef UPB_MIN
#undef UPB_UNUSED
#undef UPB_ASSERT
#undef UPB_ASSERT_DEBUGVAR
#undef UPB_UNREACHABLE
#undef UPB_INFINITY
#undef UPB_MSVC_VSNPRINTF
#undef _upb_snprintf
#undef _upb_vsnprintf
#undef _upb_va_copy
