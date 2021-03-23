/* Amalgamated source file */
#include "ruby-upb.h"
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

#if !((defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || \
      (defined(__cplusplus) && __cplusplus >= 201103L) ||           \
      (defined(_MSC_VER) && _MSC_VER >= 1900))
#error upb requires C99 or C++11 or MSVC >= 2015.
#endif

#include <stdint.h>
#include <stddef.h>

#if UINTPTR_MAX == 0xffffffff
#define UPB_SIZE(size32, size64) size32
#else
#define UPB_SIZE(size32, size64) size64
#endif

/* If we always read/write as a consistent type to each address, this shouldn't
 * violate aliasing.
 */
#define UPB_PTR_AT(msg, ofs, type) ((type*)((char*)(msg) + (ofs)))

#define UPB_READ_ONEOF(msg, fieldtype, offset, case_offset, case_val, default) \
  *UPB_PTR_AT(msg, case_offset, int) == case_val                              \
      ? *UPB_PTR_AT(msg, offset, fieldtype)                                   \
      : default

#define UPB_WRITE_ONEOF(msg, fieldtype, offset, value, case_offset, case_val) \
  *UPB_PTR_AT(msg, case_offset, int) = case_val;                             \
  *UPB_PTR_AT(msg, offset, fieldtype) = value;

#define UPB_MAPTYPE_STRING 0

/* UPB_INLINE: inline if possible, emit standalone code if required. */
#ifdef __cplusplus
#define UPB_INLINE inline
#elif defined (__GNUC__) || defined(__clang__)
#define UPB_INLINE static __inline__
#else
#define UPB_INLINE static
#endif

#define UPB_ALIGN_UP(size, align) (((size) + (align) - 1) / (align) * (align))
#define UPB_ALIGN_DOWN(size, align) ((size) / (align) * (align))
#define UPB_ALIGN_MALLOC(size) UPB_ALIGN_UP(size, 16)
#define UPB_ALIGN_OF(type) offsetof (struct { char c; type member; }, member)

/* Hints to the compiler about likely/unlikely branches. */
#if defined (__GNUC__) || defined(__clang__)
#define UPB_LIKELY(x) __builtin_expect((x),1)
#define UPB_UNLIKELY(x) __builtin_expect((x),0)
#else
#define UPB_LIKELY(x) (x)
#define UPB_UNLIKELY(x) (x)
#endif

/* Macros for function attributes on compilers that support them. */
#ifdef __GNUC__
#define UPB_FORCEINLINE __inline__ __attribute__((always_inline))
#define UPB_NOINLINE __attribute__((noinline))
#define UPB_NORETURN __attribute__((__noreturn__))
#define UPB_PRINTF(str, first_vararg) __attribute__((format (printf, str, first_vararg)))
#elif defined(_MSC_VER)
#define UPB_NOINLINE
#define UPB_FORCEINLINE
#define UPB_NORETURN __declspec(noreturn)
#define UPB_PRINTF(str, first_vararg)
#else  /* !defined(__GNUC__) */
#define UPB_FORCEINLINE
#define UPB_NOINLINE
#define UPB_NORETURN
#define UPB_PRINTF(str, first_vararg)
#endif

#define UPB_MAX(x, y) ((x) > (y) ? (x) : (y))
#define UPB_MIN(x, y) ((x) < (y) ? (x) : (y))

#define UPB_UNUSED(var) (void)var

/* UPB_ASSUME(): in release mode, we tell the compiler to assume this is true.
 */
#ifdef NDEBUG
#ifdef __GNUC__
#define UPB_ASSUME(expr) if (!(expr)) __builtin_unreachable()
#elif defined _MSC_VER
#define UPB_ASSUME(expr) if (!(expr)) __assume(0)
#else
#define UPB_ASSUME(expr) do {} while (false && (expr))
#endif
#else
#define UPB_ASSUME(expr) assert(expr)
#endif

/* UPB_ASSERT(): in release mode, we use the expression without letting it be
 * evaluated.  This prevents "unused variable" warnings. */
#ifdef NDEBUG
#define UPB_ASSERT(expr) do {} while (false && (expr))
#else
#define UPB_ASSERT(expr) assert(expr)
#endif

#if defined(__GNUC__) || defined(__clang__)
#define UPB_UNREACHABLE() do { assert(0); __builtin_unreachable(); } while(0)
#else
#define UPB_UNREACHABLE() do { assert(0); } while(0)
#endif

/* UPB_SETJMP() / UPB_LONGJMP(): avoid setting/restoring signal mask. */
#ifdef __APPLE__
#define UPB_SETJMP(buf) _setjmp(buf)
#define UPB_LONGJMP(buf, val) _longjmp(buf, val)
#else
#define UPB_SETJMP(buf) setjmp(buf)
#define UPB_LONGJMP(buf, val) longjmp(buf, val)
#endif

/* Configure whether fasttable is switched on or not. *************************/

#if defined(__x86_64__) && defined(__GNUC__)
#define UPB_FASTTABLE_SUPPORTED 1
#else
#define UPB_FASTTABLE_SUPPORTED 0
#endif

/* define UPB_ENABLE_FASTTABLE to force fast table support.
 * This is useful when we want to ensure we are really getting fasttable,
 * for example for testing or benchmarking. */
#if defined(UPB_ENABLE_FASTTABLE)
#if !UPB_FASTTABLE_SUPPORTED
#error fasttable is x86-64 + Clang/GCC only
#endif
#define UPB_FASTTABLE 1
/* Define UPB_TRY_ENABLE_FASTTABLE to use fasttable if possible.
 * This is useful for releasing code that might be used on multiple platforms,
 * for example the PHP or Ruby C extensions. */
#elif defined(UPB_TRY_ENABLE_FASTTABLE)
#define UPB_FASTTABLE UPB_FASTTABLE_SUPPORTED
#else
#define UPB_FASTTABLE 0
#endif

/* UPB_FASTTABLE_INIT() allows protos compiled for fasttable to gracefully
 * degrade to non-fasttable if we are using UPB_TRY_ENABLE_FASTTABLE. */
#if !UPB_FASTTABLE && defined(UPB_TRY_ENABLE_FASTTABLE)
#define UPB_FASTTABLE_INIT(...)
#else
#define UPB_FASTTABLE_INIT(...) __VA_ARGS__
#endif

#undef UPB_FASTTABLE_SUPPORTED

/* ASAN poisoning (for arena) *************************************************/

#if defined(__SANITIZE_ADDRESS__)
#define UPB_ASAN 1
#ifdef __cplusplus
extern "C" {
#endif
void __asan_poison_memory_region(void const volatile *addr, size_t size);
void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
#ifdef __cplusplus
}  /* extern "C" */
#endif
#define UPB_POISON_MEMORY_REGION(addr, size) \
  __asan_poison_memory_region((addr), (size))
#define UPB_UNPOISON_MEMORY_REGION(addr, size) \
  __asan_unpoison_memory_region((addr), (size))
#else
#define UPB_ASAN 0
#define UPB_POISON_MEMORY_REGION(addr, size) \
  ((void)(addr), (void)(size))
#define UPB_UNPOISON_MEMORY_REGION(addr, size) \
  ((void)(addr), (void)(size))
#endif 


#include <setjmp.h>
#include <string.h>


/* Must be last. */

/* Maps descriptor type -> elem_size_lg2.  */
static const uint8_t desctype_to_elem_size_lg2[] = {
    -1,               /* invalid descriptor type */
    3,  /* DOUBLE */
    2,   /* FLOAT */
    3,   /* INT64 */
    3,  /* UINT64 */
    2,   /* INT32 */
    3,  /* FIXED64 */
    2,  /* FIXED32 */
    0,    /* BOOL */
    UPB_SIZE(3, 4),  /* STRING */
    UPB_SIZE(2, 3),  /* GROUP */
    UPB_SIZE(2, 3),  /* MESSAGE */
    UPB_SIZE(3, 4),  /* BYTES */
    2,  /* UINT32 */
    2,    /* ENUM */
    2,   /* SFIXED32 */
    3,   /* SFIXED64 */
    2,   /* SINT32 */
    3,   /* SINT64 */
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

typedef union {
  bool bool_val;
  uint32_t uint32_val;
  uint64_t uint64_val;
  uint32_t size;
} wireval;

static const char *decode_msg(upb_decstate *d, const char *ptr, upb_msg *msg,
                              const upb_msglayout *layout);

UPB_NORETURN static void decode_err(upb_decstate *d) { UPB_LONGJMP(d->err, 1); }

// We don't want to mark this NORETURN, see comment in .h.
// Unfortunately this code to suppress the warning doesn't appear to be working.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wsuggest-attribute"
#endif

const char *fastdecode_err(upb_decstate *d) {
  longjmp(d->err, 1);
  return NULL;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

const uint8_t upb_utf8_offsets[] = {
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

static void decode_verifyutf8(upb_decstate *d, const char *buf, int len) {
  if (!decode_verifyutf8_inl(buf, len)) decode_err(d);
}

static bool decode_reserve(upb_decstate *d, upb_array *arr, size_t elem) {
  bool need_realloc = arr->size - arr->len < elem;
  if (need_realloc && !_upb_array_realloc(arr, arr->len + elem, &d->arena)) {
    decode_err(d);
  }
  return need_realloc;
}

typedef struct {
  const char *ptr;
  uint64_t val;
} decode_vret;

UPB_NOINLINE
static decode_vret decode_longvarint64(const char *ptr, uint64_t val) {
  decode_vret ret = {NULL, 0};
  uint64_t byte;
  int i;
  for (i = 1; i < 10; i++) {
    byte = (uint8_t)ptr[i];
    val += (byte - 1) << (i * 7);
    if (!(byte & 0x80)) {
      ret.ptr = ptr + i + 1;
      ret.val = val;
      return ret;
    }
  }
  return ret;
}

UPB_FORCEINLINE
static const char *decode_varint64(upb_decstate *d, const char *ptr,
                                   uint64_t *val) {
  uint64_t byte = (uint8_t)*ptr;
  if (UPB_LIKELY((byte & 0x80) == 0)) {
    *val = byte;
    return ptr + 1;
  } else {
    decode_vret res = decode_longvarint64(ptr, byte);
    if (!res.ptr) decode_err(d);
    *val = res.val;
    return res.ptr;
  }
}

UPB_FORCEINLINE
static const char *decode_tag(upb_decstate *d, const char *ptr,
                                   uint32_t *val) {
  uint64_t byte = (uint8_t)*ptr;
  if (UPB_LIKELY((byte & 0x80) == 0)) {
    *val = byte;
    return ptr + 1;
  } else {
    const char *start = ptr;
    decode_vret res = decode_longvarint64(ptr, byte);
    ptr = res.ptr;
    *val = res.val;
    if (!ptr || *val > UINT32_MAX || ptr - start > 5) decode_err(d);
    return ptr;
  }
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
  return _upb_msg_new_inl(subl, &d->arena);
}

UPB_NOINLINE
const char *decode_isdonefallback(upb_decstate *d, const char *ptr,
                                  int overrun) {
  ptr = decode_isdonefallback_inl(d, ptr, overrun);
  if (ptr == NULL) {
    decode_err(d);
  }
  return ptr;
}

static const char *decode_readstr(upb_decstate *d, const char *ptr, int size,
                                  upb_strview *str) {
  if (d->alias) {
    str->data = ptr;
  } else {
    char *data =  upb_arena_malloc(&d->arena, size);
    if (!data) decode_err(d);
    memcpy(data, ptr, size);
    str->data = data;
  }
  str->size = size;
  return ptr + size;
}

UPB_FORCEINLINE
static const char *decode_tosubmsg(upb_decstate *d, const char *ptr,
                                   upb_msg *submsg, const upb_msglayout *layout,
                                   const upb_msglayout_field *field, int size) {
  const upb_msglayout *subl = layout->submsgs[field->submsg_index];
  int saved_delta = decode_pushlimit(d, ptr, size);
  if (--d->depth < 0) decode_err(d);
  if (!decode_isdone(d, &ptr)) {
    ptr = decode_msg(d, ptr, submsg, subl);
  }
  if (d->end_group != DECODE_NOGROUP) decode_err(d);
  decode_poplimit(d, ptr, saved_delta);
  d->depth++;
  return ptr;
}

UPB_FORCEINLINE
static const char *decode_group(upb_decstate *d, const char *ptr,
                                upb_msg *submsg, const upb_msglayout *subl,
                                uint32_t number) {
  if (--d->depth < 0) decode_err(d);
  if (decode_isdone(d, &ptr)) {
    decode_err(d);
  }
  ptr = decode_msg(d, ptr, submsg, subl);
  if (d->end_group != number) decode_err(d);
  d->end_group = DECODE_NOGROUP;
  d->depth++;
  return ptr;
}

UPB_FORCEINLINE
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

  if (arr) {
    decode_reserve(d, arr, 1);
  } else {
    size_t lg2 = desctype_to_elem_size_lg2[field->descriptortype];
    arr = _upb_array_new(&d->arena, 4, lg2);
    if (!arr) decode_err(d);
    *arrp = arr;
  }

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
      decode_verifyutf8(d, ptr, val.size);
      /* Fallthrough. */
    case OP_BYTES: {
      /* Append bytes. */
      upb_strview *str = (upb_strview*)_upb_array_ptr(arr) + arr->len;
      arr->len++;
      return decode_readstr(d, ptr, val.size, str);
    }
    case OP_SUBMSG: {
      /* Append submessage / group. */
      upb_msg *submsg = decode_newsubmsg(d, layout, field);
      *UPB_PTR_AT(_upb_array_ptr(arr), arr->len * sizeof(void *), upb_msg *) =
          submsg;
      arr->len++;
      if (UPB_UNLIKELY(field->descriptortype == UPB_DTYPE_GROUP)) {
        return decode_togroup(d, ptr, submsg, layout, field);
      } else {
        return decode_tosubmsg(d, ptr, submsg, layout, field, val.size);
      }
    }
    case OP_FIXPCK_LG2(2):
    case OP_FIXPCK_LG2(3): {
      /* Fixed packed. */
      int lg2 = op - OP_FIXPCK_LG2(0);
      int mask = (1 << lg2) - 1;
      size_t count = val.size >> lg2;
      if ((val.size & mask) != 0) {
        decode_err(d); /* Length isn't a round multiple of elem size. */
      }
      decode_reserve(d, arr, count);
      mem = UPB_PTR_AT(_upb_array_ptr(arr), arr->len << lg2, void);
      arr->len += count;
      memcpy(mem, ptr, val.size);  /* XXX: ptr boundary. */
      return ptr + val.size;
    }
    case OP_VARPCK_LG2(0):
    case OP_VARPCK_LG2(2):
    case OP_VARPCK_LG2(3): {
      /* Varint packed. */
      int lg2 = op - OP_VARPCK_LG2(0);
      int scale = 1 << lg2;
      int saved_limit = decode_pushlimit(d, ptr, val.size);
      char *out = UPB_PTR_AT(_upb_array_ptr(arr), arr->len << lg2, void);
      while (!decode_isdone(d, &ptr)) {
        wireval elem;
        ptr = decode_varint64(d, ptr, &elem.uint64_val);
        decode_munge(field->descriptortype, &elem);
        if (decode_reserve(d, arr, 1)) {
          out = UPB_PTR_AT(_upb_array_ptr(arr), arr->len << lg2, void);
        }
        arr->len++;
        memcpy(out, &elem, scale);
        out += scale;
      }
      decode_poplimit(d, ptr, saved_limit);
      return ptr;
    }
    default:
      UPB_UNREACHABLE();
  }
}

static const char *decode_tomap(upb_decstate *d, const char *ptr, upb_msg *msg,
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
    map = _upb_map_new(&d->arena, key_size, val_size);
    *map_p = map;
  }

  /* Parse map entry. */
  memset(&ent, 0, sizeof(ent));

  if (entry->fields[1].descriptortype == UPB_DESCRIPTOR_TYPE_MESSAGE ||
      entry->fields[1].descriptortype == UPB_DESCRIPTOR_TYPE_GROUP) {
    /* Create proactively to handle the case where it doesn't appear. */
    ent.v.val = upb_value_ptr(_upb_msg_new(entry->submsgs[0], &d->arena));
  }

  ptr = decode_tosubmsg(d, ptr, &ent.k, layout, field, val.size);
  _upb_map_set(map, &ent.k, map->key_size, &ent.v, map->val_size, &d->arena);
  return ptr;
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
        ptr = decode_tosubmsg(d, ptr, submsg, layout, field, val.size);
      }
      break;
    }
    case OP_STRING:
      decode_verifyutf8(d, ptr, val.size);
      /* Fallthrough. */
    case OP_BYTES:
      return decode_readstr(d, ptr, val.size, mem);
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

UPB_FORCEINLINE
static bool decode_tryfastdispatch(upb_decstate *d, const char **ptr,
                                   upb_msg *msg, const upb_msglayout *layout) {
#if UPB_FASTTABLE
  if (layout && layout->table_mask != (unsigned char)-1) {
    uint16_t tag = fastdecode_loadtag(*ptr);
    intptr_t table = decode_totable(layout);
    *ptr = fastdecode_tagdispatch(d, *ptr, msg, table, 0, tag);
    return true;
  }
#endif
  return false;
}

UPB_NOINLINE
static const char *decode_msg(upb_decstate *d, const char *ptr, upb_msg *msg,
                              const upb_msglayout *layout) {
  while (true) {
    uint32_t tag;
    const upb_msglayout_field *field;
    int field_number;
    int wire_type;
    const char *field_start = ptr;
    wireval val;
    int op;

    UPB_ASSERT(ptr < d->limit_ptr);
    ptr = decode_tag(d, ptr, &tag);
    field_number = tag >> 3;
    wire_type = tag & 7;

    field = upb_find_field(layout, field_number);

    switch (wire_type) {
      case UPB_WIRE_TYPE_VARINT:
        ptr = decode_varint64(d, ptr, &val.uint64_val);
        op = varint_ops[field->descriptortype];
        decode_munge(field->descriptortype, &val);
        break;
      case UPB_WIRE_TYPE_32BIT:
        memcpy(&val.uint32_val, ptr, 4);
        val.uint32_val = _upb_be_swap32(val.uint32_val);
        ptr += 4;
        op = OP_SCALAR_LG2(2);
        if (((1 << field->descriptortype) & fixed32_ok) == 0) goto unknown;
        break;
      case UPB_WIRE_TYPE_64BIT:
        memcpy(&val.uint64_val, ptr, 8);
        val.uint64_val = _upb_be_swap64(val.uint64_val);
        ptr += 8;
        op = OP_SCALAR_LG2(3);
        if (((1 << field->descriptortype) & fixed64_ok) == 0) goto unknown;
        break;
      case UPB_WIRE_TYPE_DELIMITED: {
        int ndx = field->descriptortype;
        uint64_t size;
        if (_upb_isrepeated(field)) ndx += 18;
        ptr = decode_varint64(d, ptr, &size);
        if (size >= INT32_MAX ||
            ptr - d->end + (int32_t)size > d->limit) {
          decode_err(d); /* Length overflow. */
        }
        op = delim_ops[ndx];
        val.size = size;
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
          ptr = decode_tomap(d, ptr, msg, layout, field, val);
          break;
        default:
          ptr = decode_tomsg(d, ptr, msg, layout, field, val, op);
          break;
      }
    } else {
    unknown:
      /* Skip unknown field. */
      if (field_number == 0) decode_err(d);
      if (wire_type == UPB_WIRE_TYPE_DELIMITED) ptr += val.size;
      if (msg) {
        if (wire_type == UPB_WIRE_TYPE_START_GROUP) {
          d->unknown = field_start;
          d->unknown_msg = msg;
          ptr = decode_group(d, ptr, NULL, NULL, field_number);
          d->unknown_msg = NULL;
          field_start = d->unknown;
        }
        if (!_upb_msg_addunknown(msg, field_start, ptr - field_start,
                                 &d->arena)) {
          decode_err(d);
        }
      } else if (wire_type == UPB_WIRE_TYPE_START_GROUP) {
        ptr = decode_group(d, ptr, NULL, NULL, field_number);
      }
    }

    if (decode_isdone(d, &ptr)) return ptr;
    if (decode_tryfastdispatch(d, &ptr, msg, layout)) return ptr;
  }
}

const char *fastdecode_generic(struct upb_decstate *d, const char *ptr,
                               upb_msg *msg, intptr_t table, uint64_t hasbits,
                               uint64_t data) {
  (void)data;
  *(uint32_t*)msg |= hasbits;
  return decode_msg(d, ptr, msg, decode_totablep(table));
}

static bool decode_top(struct upb_decstate *d, const char *buf, void *msg,
                       const upb_msglayout *l) {
  if (!decode_tryfastdispatch(d, &buf, msg, l)) {
    decode_msg(d, buf, msg, l);
  }
  return d->end_group == DECODE_NOGROUP;
}

bool _upb_decode(const char *buf, size_t size, void *msg,
                 const upb_msglayout *l, upb_arena *arena, int options) {
  bool ok;
  upb_decstate state;
  unsigned depth = (unsigned)options >> 16;

  if (size == 0) {
    return true;
  } else if (size <= 16) {
    memset(&state.patch, 0, 32);
    memcpy(&state.patch, buf, size);
    buf = state.patch;
    state.end = buf + size;
    state.limit = 0;
    state.alias = false;
  } else {
    state.end = buf + size - 16;
    state.limit = 16;
    state.alias = options & UPB_DECODE_ALIAS;
  }

  state.limit_ptr = state.end;
  state.unknown_msg = NULL;
  state.depth = depth ? depth : 64;
  state.end_group = DECODE_NOGROUP;
  state.arena.head = arena->head;
  state.arena.last_size = arena->last_size;
  state.arena.cleanups = arena->cleanups;
  state.arena.parent = arena;

  if (UPB_UNLIKELY(UPB_SETJMP(state.err))) {
    ok = false;
  } else {
    ok = decode_top(&state, buf, msg, l);
  }

  arena->head.ptr = state.arena.head.ptr;
  arena->head.end = state.arena.head.end;
  arena->cleanups = state.arena.cleanups;
  return ok;
}

#undef OP_SCALAR_LG2
#undef OP_FIXPCK_LG2
#undef OP_VARPCK_LG2
#undef OP_STRING
#undef OP_SUBMSG
/* We encode backwards, to avoid pre-computing lengths (one-pass encode). */


#include <setjmp.h>
#include <string.h>


/* Must be last. */

#define UPB_PB_VARINT_MAX_LEN 10

UPB_NOINLINE
static size_t encode_varint64(uint64_t val, char *buf) {
  size_t i = 0;
  do {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    buf[i++] = byte;
  } while (val);
  return i;
}

static uint32_t encode_zz32(int32_t n) { return ((uint32_t)n << 1) ^ (n >> 31); }
static uint64_t encode_zz64(int64_t n) { return ((uint64_t)n << 1) ^ (n >> 63); }

typedef struct {
  jmp_buf err;
  upb_alloc *alloc;
  char *buf, *ptr, *limit;
  int options;
  int depth;
  _upb_mapsorter sorter;
} upb_encstate;

static size_t upb_roundup_pow2(size_t bytes) {
  size_t ret = 128;
  while (ret < bytes) {
    ret *= 2;
  }
  return ret;
}

UPB_NORETURN static void encode_err(upb_encstate *e) {
  UPB_LONGJMP(e->err, 1);
}

UPB_NOINLINE
static void encode_growbuffer(upb_encstate *e, size_t bytes) {
  size_t old_size = e->limit - e->buf;
  size_t new_size = upb_roundup_pow2(bytes + (e->limit - e->ptr));
  char *new_buf = upb_realloc(e->alloc, e->buf, old_size, new_size);

  if (!new_buf) encode_err(e);

  /* We want previous data at the end, realloc() put it at the beginning. */
  if (old_size > 0) {
    memmove(new_buf + new_size - old_size, e->buf, old_size);
  }

  e->ptr = new_buf + new_size - (e->limit - e->ptr);
  e->limit = new_buf + new_size;
  e->buf = new_buf;

  e->ptr -= bytes;
}

/* Call to ensure that at least "bytes" bytes are available for writing at
 * e->ptr.  Returns false if the bytes could not be allocated. */
UPB_FORCEINLINE
static void encode_reserve(upb_encstate *e, size_t bytes) {
  if ((size_t)(e->ptr - e->buf) < bytes) {
    encode_growbuffer(e, bytes);
    return;
  }

  e->ptr -= bytes;
}

/* Writes the given bytes to the buffer, handling reserve/advance. */
static void encode_bytes(upb_encstate *e, const void *data, size_t len) {
  if (len == 0) return;  /* memcpy() with zero size is UB */
  encode_reserve(e, len);
  memcpy(e->ptr, data, len);
}

static void encode_fixed64(upb_encstate *e, uint64_t val) {
  val = _upb_be_swap64(val);
  encode_bytes(e, &val, sizeof(uint64_t));
}

static void encode_fixed32(upb_encstate *e, uint32_t val) {
  val = _upb_be_swap32(val);
  encode_bytes(e, &val, sizeof(uint32_t));
}

UPB_NOINLINE
static void encode_longvarint(upb_encstate *e, uint64_t val) {
  size_t len;
  char *start;

  encode_reserve(e, UPB_PB_VARINT_MAX_LEN);
  len = encode_varint64(val, e->ptr);
  start = e->ptr + UPB_PB_VARINT_MAX_LEN - len;
  memmove(start, e->ptr, len);
  e->ptr = start;
}

UPB_FORCEINLINE
static void encode_varint(upb_encstate *e, uint64_t val) {
  if (val < 128 && e->ptr != e->buf) {
    --e->ptr;
    *e->ptr = val;
  } else {
    encode_longvarint(e, val);
  }
}

static void encode_double(upb_encstate *e, double d) {
  uint64_t u64;
  UPB_ASSERT(sizeof(double) == sizeof(uint64_t));
  memcpy(&u64, &d, sizeof(uint64_t));
  encode_fixed64(e, u64);
}

static void encode_float(upb_encstate *e, float d) {
  uint32_t u32;
  UPB_ASSERT(sizeof(float) == sizeof(uint32_t));
  memcpy(&u32, &d, sizeof(uint32_t));
  encode_fixed32(e, u32);
}

static void encode_tag(upb_encstate *e, uint32_t field_number,
                       uint8_t wire_type) {
  encode_varint(e, (field_number << 3) | wire_type);
}

static void encode_fixedarray(upb_encstate *e, const upb_array *arr,
                               size_t elem_size, uint32_t tag) {
  size_t bytes = arr->len * elem_size;
  const char* data = _upb_array_constptr(arr);
  const char* ptr = data + bytes - elem_size;
  if (tag) {
    while (true) {
      encode_bytes(e, ptr, elem_size);
      encode_varint(e, tag);
      if (ptr == data) break;
      ptr -= elem_size;
    }
  } else {
    encode_bytes(e, data, bytes);
  }
}

static void encode_message(upb_encstate *e, const char *msg,
                           const upb_msglayout *m, size_t *size);

static void encode_scalar(upb_encstate *e, const void *_field_mem,
                          const upb_msglayout *m, const upb_msglayout_field *f,
                          bool skip_zero_value) {
  const char *field_mem = _field_mem;
  int wire_type;

#define CASE(ctype, type, wtype, encodeval) \
  {                                         \
    ctype val = *(ctype *)field_mem;        \
    if (skip_zero_value && val == 0) {      \
      return;                               \
    }                                       \
    encode_##type(e, encodeval);            \
    wire_type = wtype;                      \
    break;                                  \
  }

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
      CASE(int32_t, varint, UPB_WIRE_TYPE_VARINT, encode_zz32(val));
    case UPB_DESCRIPTOR_TYPE_SINT64:
      CASE(int64_t, varint, UPB_WIRE_TYPE_VARINT, encode_zz64(val));
    case UPB_DESCRIPTOR_TYPE_STRING:
    case UPB_DESCRIPTOR_TYPE_BYTES: {
      upb_strview view = *(upb_strview*)field_mem;
      if (skip_zero_value && view.size == 0) {
        return;
      }
      encode_bytes(e, view.data, view.size);
      encode_varint(e, view.size);
      wire_type = UPB_WIRE_TYPE_DELIMITED;
      break;
    }
    case UPB_DESCRIPTOR_TYPE_GROUP: {
      size_t size;
      void *submsg = *(void **)field_mem;
      const upb_msglayout *subm = m->submsgs[f->submsg_index];
      if (submsg == NULL) {
        return;
      }
      if (--e->depth == 0) encode_err(e);
      encode_tag(e, f->number, UPB_WIRE_TYPE_END_GROUP);
      encode_message(e, submsg, subm, &size);
      wire_type = UPB_WIRE_TYPE_START_GROUP;
      e->depth++;
      break;
    }
    case UPB_DESCRIPTOR_TYPE_MESSAGE: {
      size_t size;
      void *submsg = *(void **)field_mem;
      const upb_msglayout *subm = m->submsgs[f->submsg_index];
      if (submsg == NULL) {
        return;
      }
      if (--e->depth == 0) encode_err(e);
      encode_message(e, submsg, subm, &size);
      encode_varint(e, size);
      wire_type = UPB_WIRE_TYPE_DELIMITED;
      e->depth++;
      break;
    }
    default:
      UPB_UNREACHABLE();
  }
#undef CASE

  encode_tag(e, f->number, wire_type);
}

static void encode_array(upb_encstate *e, const char *field_mem,
                         const upb_msglayout *m, const upb_msglayout_field *f) {
  const upb_array *arr = *(const upb_array**)field_mem;
  bool packed = f->label == _UPB_LABEL_PACKED;
  size_t pre_len = e->limit - e->ptr;

  if (arr == NULL || arr->len == 0) {
    return;
  }

#define VARINT_CASE(ctype, encode)                                       \
  {                                                                      \
    const ctype *start = _upb_array_constptr(arr);                       \
    const ctype *ptr = start + arr->len;                                 \
    uint32_t tag = packed ? 0 : (f->number << 3) | UPB_WIRE_TYPE_VARINT; \
    do {                                                                 \
      ptr--;                                                             \
      encode_varint(e, encode);                                          \
      if (tag) encode_varint(e, tag);                                    \
    } while (ptr != start);                                              \
  }                                                                      \
  break;

#define TAG(wire_type) (packed ? 0 : (f->number << 3 | wire_type))

  switch (f->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
      encode_fixedarray(e, arr, sizeof(double), TAG(UPB_WIRE_TYPE_64BIT));
      break;
    case UPB_DESCRIPTOR_TYPE_FLOAT:
      encode_fixedarray(e, arr, sizeof(float), TAG(UPB_WIRE_TYPE_32BIT));
      break;
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
      encode_fixedarray(e, arr, sizeof(uint64_t), TAG(UPB_WIRE_TYPE_64BIT));
      break;
    case UPB_DESCRIPTOR_TYPE_FIXED32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
      encode_fixedarray(e, arr, sizeof(uint32_t), TAG(UPB_WIRE_TYPE_32BIT));
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
      VARINT_CASE(int32_t, encode_zz32(*ptr));
    case UPB_DESCRIPTOR_TYPE_SINT64:
      VARINT_CASE(int64_t, encode_zz64(*ptr));
    case UPB_DESCRIPTOR_TYPE_STRING:
    case UPB_DESCRIPTOR_TYPE_BYTES: {
      const upb_strview *start = _upb_array_constptr(arr);
      const upb_strview *ptr = start + arr->len;
      do {
        ptr--;
        encode_bytes(e, ptr->data, ptr->size);
        encode_varint(e, ptr->size);
        encode_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED);
      } while (ptr != start);
      return;
    }
    case UPB_DESCRIPTOR_TYPE_GROUP: {
      const void *const*start = _upb_array_constptr(arr);
      const void *const*ptr = start + arr->len;
      const upb_msglayout *subm = m->submsgs[f->submsg_index];
      if (--e->depth == 0) encode_err(e);
      do {
        size_t size;
        ptr--;
        encode_tag(e, f->number, UPB_WIRE_TYPE_END_GROUP);
        encode_message(e, *ptr, subm, &size);
        encode_tag(e, f->number, UPB_WIRE_TYPE_START_GROUP);
      } while (ptr != start);
      e->depth++;
      return;
    }
    case UPB_DESCRIPTOR_TYPE_MESSAGE: {
      const void *const*start = _upb_array_constptr(arr);
      const void *const*ptr = start + arr->len;
      const upb_msglayout *subm = m->submsgs[f->submsg_index];
      if (--e->depth == 0) encode_err(e);
      do {
        size_t size;
        ptr--;
        encode_message(e, *ptr, subm, &size);
        encode_varint(e, size);
        encode_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED);
      } while (ptr != start);
      e->depth++;
      return;
    }
  }
#undef VARINT_CASE

  if (packed) {
    encode_varint(e, e->limit - e->ptr - pre_len);
    encode_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED);
  }
}

static void encode_mapentry(upb_encstate *e, uint32_t number,
                            const upb_msglayout *layout,
                            const upb_map_entry *ent) {
  const upb_msglayout_field *key_field = &layout->fields[0];
  const upb_msglayout_field *val_field = &layout->fields[1];
  size_t pre_len = e->limit - e->ptr;
  size_t size;
  encode_scalar(e, &ent->v, layout, val_field, false);
  encode_scalar(e, &ent->k, layout, key_field, false);
  size = (e->limit - e->ptr) - pre_len;
  encode_varint(e, size);
  encode_tag(e, number, UPB_WIRE_TYPE_DELIMITED);
}

static void encode_map(upb_encstate *e, const char *field_mem,
                       const upb_msglayout *m, const upb_msglayout_field *f) {
  const upb_map *map = *(const upb_map**)field_mem;
  const upb_msglayout *layout = m->submsgs[f->submsg_index];
  UPB_ASSERT(layout->field_count == 2);

  if (map == NULL) return;

  if (e->options & UPB_ENCODE_DETERMINISTIC) {
    _upb_sortedmap sorted;
    _upb_mapsorter_pushmap(&e->sorter, layout->fields[0].descriptortype, map,
                           &sorted);
    upb_map_entry ent;
    while (_upb_sortedmap_next(&e->sorter, map, &sorted, &ent)) {
      encode_mapentry(e, f->number, layout, &ent);
    }
    _upb_mapsorter_popmap(&e->sorter, &sorted);
  } else {
    upb_strtable_iter i;
    upb_strtable_begin(&i, &map->table);
    for(; !upb_strtable_done(&i); upb_strtable_next(&i)) {
      upb_strview key = upb_strtable_iter_key(&i);
      const upb_value val = upb_strtable_iter_value(&i);
      upb_map_entry ent;
      _upb_map_fromkey(key, &ent.k, map->key_size);
      _upb_map_fromvalue(val, &ent.v, map->val_size);
      encode_mapentry(e, f->number, layout, &ent);
    }
  }
}

static void encode_scalarfield(upb_encstate *e, const char *msg,
                               const upb_msglayout *m,
                               const upb_msglayout_field *f) {
  bool skip_empty = false;
  if (f->presence == 0) {
    /* Proto3 presence. */
    skip_empty = true;
  } else if (f->presence > 0) {
    /* Proto2 presence: hasbit. */
    if (!_upb_hasbit_field(msg, f)) return;
  } else {
    /* Field is in a oneof. */
    if (_upb_getoneofcase_field(msg, f) != f->number) return;
  }
  encode_scalar(e, msg + f->offset, m, f, skip_empty);
}

static void encode_message(upb_encstate *e, const char *msg,
                           const upb_msglayout *m, size_t *size) {
  size_t pre_len = e->limit - e->ptr;
  const upb_msglayout_field *f = &m->fields[m->field_count];
  const upb_msglayout_field *first = &m->fields[0];

  if ((e->options & UPB_ENCODE_SKIPUNKNOWN) == 0) {
    size_t unknown_size;
    const char *unknown = upb_msg_getunknown(msg, &unknown_size);

    if (unknown) {
      encode_bytes(e, unknown, unknown_size);
    }
  }

  while (f != first) {
    f--;
    if (_upb_isrepeated(f)) {
      encode_array(e, msg + f->offset, m, f);
    } else if (f->label == _UPB_LABEL_MAP) {
      encode_map(e, msg + f->offset, m, f);
    } else {
      encode_scalarfield(e, msg, m, f);
    }
  }

  *size = (e->limit - e->ptr) - pre_len;
}

char *upb_encode_ex(const void *msg, const upb_msglayout *l, int options,
                    upb_arena *arena, size_t *size) {
  upb_encstate e;
  unsigned depth = (unsigned)options >> 16;

  e.alloc = upb_arena_alloc(arena);
  e.buf = NULL;
  e.limit = NULL;
  e.ptr = NULL;
  e.depth = depth ? depth : 64;
  e.options = options;
  _upb_mapsorter_init(&e.sorter);
  char *ret = NULL;

  if (UPB_SETJMP(e.err)) {
    *size = 0;
    ret = NULL;
  } else {
    encode_message(&e, msg, l, size);
    *size = e.limit - e.ptr;
    if (*size == 0) {
      static char ch;
      ret = &ch;
    } else {
      UPB_ASSERT(e.ptr);
      ret = e.ptr;
    }
  }

  _upb_mapsorter_destroy(&e.sorter);
  return ret;
}




/** upb_msg *******************************************************************/

static const size_t overhead = sizeof(upb_msg_internal);

static const upb_msg_internal *upb_msg_getinternal_const(const upb_msg *msg) {
  ptrdiff_t size = sizeof(upb_msg_internal);
  return (upb_msg_internal*)((char*)msg - size);
}

upb_msg *_upb_msg_new(const upb_msglayout *l, upb_arena *a) {
  return _upb_msg_new_inl(l, a);
}

void _upb_msg_clear(upb_msg *msg, const upb_msglayout *l) {
  void *mem = UPB_PTR_AT(msg, -sizeof(upb_msg_internal), char);
  memset(mem, 0, upb_msg_sizeof(l));
}

bool _upb_msg_addunknown(upb_msg *msg, const char *data, size_t len,
                         upb_arena *arena) {

  upb_msg_internal *in = upb_msg_getinternal(msg);
  if (!in->unknown) {
    size_t size = 128;
    while (size < len) size *= 2;
    in->unknown = upb_arena_malloc(arena, size + overhead);
    if (!in->unknown) return false;
    in->unknown->size = size;
    in->unknown->len = 0;
  } else if (in->unknown->size - in->unknown->len < len) {
    size_t need = in->unknown->len + len;
    size_t size = in->unknown->size;
    while (size < need)  size *= 2;
    in->unknown = upb_arena_realloc(
        arena, in->unknown, in->unknown->size + overhead, size + overhead);
    if (!in->unknown) return false;
    in->unknown->size = size;
  }
  memcpy(UPB_PTR_AT(in->unknown + 1, in->unknown->len, char), data, len);
  in->unknown->len += len;
  return true;
}

void _upb_msg_discardunknown_shallow(upb_msg *msg) {
  upb_msg_internal *in = upb_msg_getinternal(msg);
  if (in->unknown) {
    in->unknown->len = 0;
  }
}

const char *upb_msg_getunknown(const upb_msg *msg, size_t *len) {
  const upb_msg_internal *in = upb_msg_getinternal_const(msg);
  if (in->unknown) {
    *len = in->unknown->len;
    return (char*)(in->unknown + 1);
  } else {
    *len = 0;
    return NULL;
  }
}

/** upb_array *****************************************************************/

bool _upb_array_realloc(upb_array *arr, size_t min_size, upb_arena *arena) {
  size_t new_size = UPB_MAX(arr->size, 4);
  int elem_size_lg2 = arr->data & 7;
  size_t old_bytes = arr->size << elem_size_lg2;
  size_t new_bytes;
  void* ptr = _upb_array_ptr(arr);

  /* Log2 ceiling of size. */
  while (new_size < min_size) new_size *= 2;

  new_bytes = new_size << elem_size_lg2;
  ptr = upb_arena_realloc(arena, ptr, old_bytes, new_bytes);

  if (!ptr) {
    return false;
  }

  arr->data = _upb_tag_arrptr(ptr, elem_size_lg2);
  arr->size = new_size;
  return true;
}

static upb_array *getorcreate_array(upb_array **arr_ptr, int elem_size_lg2,
                                    upb_arena *arena) {
  upb_array *arr = *arr_ptr;
  if (!arr) {
    arr = _upb_array_new(arena, 4, elem_size_lg2);
    if (!arr) return NULL;
    *arr_ptr = arr;
  }
  return arr;
}

void *_upb_array_resize_fallback(upb_array **arr_ptr, size_t size,
                                 int elem_size_lg2, upb_arena *arena) {
  upb_array *arr = getorcreate_array(arr_ptr, elem_size_lg2, arena);
  return arr && _upb_array_resize(arr, size, arena) ? _upb_array_ptr(arr)
                                                    : NULL;
}

bool _upb_array_append_fallback(upb_array **arr_ptr, const void *value,
                                int elem_size_lg2, upb_arena *arena) {
  upb_array *arr = getorcreate_array(arr_ptr, elem_size_lg2, arena);
  if (!arr) return false;

  size_t elems = arr->len;

  if (!_upb_array_resize(arr, elems + 1, arena)) {
    return false;
  }

  char *data = _upb_array_ptr(arr);
  memcpy(data + (elems << elem_size_lg2), value, 1 << elem_size_lg2);
  return true;
}

/** upb_map *******************************************************************/

upb_map *_upb_map_new(upb_arena *a, size_t key_size, size_t value_size) {
  upb_map *map = upb_arena_malloc(a, sizeof(upb_map));

  if (!map) {
    return NULL;
  }

  upb_strtable_init2(&map->table, UPB_CTYPE_INT32, 4, upb_arena_alloc(a));
  map->key_size = key_size;
  map->val_size = value_size;

  return map;
}

static void _upb_mapsorter_getkeys(const void *_a, const void *_b, void *a_key,
                                   void *b_key, size_t size) {
  const upb_tabent *const*a = _a;
  const upb_tabent *const*b = _b;
  upb_strview a_tabkey = upb_tabstrview((*a)->key);
  upb_strview b_tabkey = upb_tabstrview((*b)->key);
  _upb_map_fromkey(a_tabkey, a_key, size);
  _upb_map_fromkey(b_tabkey, b_key, size);
}

static int _upb_mapsorter_cmpi64(const void *_a, const void *_b) {
  int64_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 8);
  return a - b;
}

static int _upb_mapsorter_cmpu64(const void *_a, const void *_b) {
  uint64_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 8);
  return a - b;
}

static int _upb_mapsorter_cmpi32(const void *_a, const void *_b) {
  int32_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 4);
  return a - b;
}

static int _upb_mapsorter_cmpu32(const void *_a, const void *_b) {
  uint32_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 4);
  return a - b;
}

static int _upb_mapsorter_cmpbool(const void *_a, const void *_b) {
  bool a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 1);
  return a - b;
}

static int _upb_mapsorter_cmpstr(const void *_a, const void *_b) {
  upb_strview a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, UPB_MAPTYPE_STRING);
  size_t common_size = UPB_MIN(a.size, b.size);
  int cmp = memcmp(a.data, b.data, common_size);
  if (cmp) return cmp;
  return a.size - b.size;
}

bool _upb_mapsorter_pushmap(_upb_mapsorter *s, upb_descriptortype_t key_type,
                            const upb_map *map, _upb_sortedmap *sorted) {
  int map_size = _upb_map_size(map);
  sorted->start = s->size;
  sorted->pos = sorted->start;
  sorted->end = sorted->start + map_size;

  /* Grow s->entries if necessary. */
  if (sorted->end > s->cap) {
    s->cap = _upb_lg2ceilsize(sorted->end);
    s->entries = realloc(s->entries, s->cap * sizeof(*s->entries));
    if (!s->entries) return false;
  }

  s->size = sorted->end;

  /* Copy non-empty entries from the table to s->entries. */
  upb_tabent const**dst = &s->entries[sorted->start];
  const upb_tabent *src = map->table.t.entries;
  const upb_tabent *end = src + upb_table_size(&map->table.t);
  for (; src < end; src++) {
    if (!upb_tabent_isempty(src)) {
      *dst = src;
      dst++;
    }
  }
  UPB_ASSERT(dst == &s->entries[sorted->end]);

  /* Sort entries according to the key type. */

  int (*compar)(const void *, const void *);

  switch (key_type) {
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
    case UPB_DESCRIPTOR_TYPE_SINT64:
      compar = _upb_mapsorter_cmpi64;
      break;
    case UPB_DESCRIPTOR_TYPE_UINT64:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
      compar = _upb_mapsorter_cmpu64;
      break;
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_SINT32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
    case UPB_DESCRIPTOR_TYPE_ENUM:
      compar = _upb_mapsorter_cmpi32;
      break;
    case UPB_DESCRIPTOR_TYPE_UINT32:
    case UPB_DESCRIPTOR_TYPE_FIXED32:
      compar = _upb_mapsorter_cmpu32;
      break;
    case UPB_DESCRIPTOR_TYPE_BOOL:
      compar = _upb_mapsorter_cmpbool;
      break;
    case UPB_DESCRIPTOR_TYPE_STRING:
      compar = _upb_mapsorter_cmpstr;
      break;
    default:
      UPB_UNREACHABLE();
  }

  qsort(&s->entries[sorted->start], map_size, sizeof(*s->entries), compar);
  return true;
}
/*
** upb_table Implementation
**
** Implementation is heavily inspired by Lua's ltable.c.
*/

#include <string.h>

#include "third_party/wyhash/wyhash.h"

/* Must be last. */

#define UPB_MAXARRSIZE 16  /* 64k. */

/* From Chromium. */
#define ARRAY_SIZE(x) \
    ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

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
  return t->count == t->max_count;
}

static bool init(upb_table *t, uint8_t size_lg2, upb_alloc *a) {
  size_t bytes;

  t->count = 0;
  t->size_lg2 = size_lg2;
  t->mask = upb_table_size(t) ? upb_table_size(t) - 1 : 0;
  t->max_count = upb_table_size(t) * MAX_LOAD;
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
  upb_free(a, mutable_entries(t));
}

static upb_tabent *emptyent(upb_table *t, upb_tabent *e) {
  upb_tabent *begin = mutable_entries(t);
  upb_tabent *end = begin + upb_table_size(t);
  for (e = e + 1; e < end; e++) {
    if (upb_tabent_isempty(e)) return e;
  }
  for (e = begin; e < end; e++) {
    if (upb_tabent_isempty(e)) return e;
  }
  UPB_ASSERT(false);
  return NULL;
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
      _upb_value_setval(v, e->val.val);
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

  t->count++;
  mainpos_e = getentry_mutable(t, hash);
  our_e = mainpos_e;

  if (upb_tabent_isempty(mainpos_e)) {
    /* Our main position is empty; use it. */
    our_e->next = NULL;
  } else {
    /* Collision. */
    upb_tabent *new_e = emptyent(t, mainpos_e);
    /* Head of collider's chain. */
    upb_tabent *chain = getentry_mutable(t, hashfunc(mainpos_e->key));
    if (chain == mainpos_e) {
      /* Existing ent is in its main position (it has the same hash as us, and
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
    if (val) _upb_value_setval(val, chain->val.val);
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
      if (val) _upb_value_setval(val, chain->next->val.val);
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
      return SIZE_MAX - 1;  /* Distinct from -1. */
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
  if (k2.str.len) memcpy(str + sizeof(uint32_t), k2.str.str, k2.str.len);
  str[sizeof(uint32_t) + k2.str.len] = '\0';
  return (uintptr_t)str;
}

static uint32_t table_hash(const char *p, size_t n) {
  return wyhash(p, n, 0, _wyp);
}

static uint32_t strhash(upb_tabkey key) {
  uint32_t len;
  char *str = upb_tabstr(key, &len);
  return table_hash(str, len);
}

static bool streql(upb_tabkey k1, lookupkey_t k2) {
  uint32_t len;
  char *str = upb_tabstr(k1, &len);
  return len == k2.str.len && (len == 0 || memcmp(str, k2.str.str, len) == 0);
}

bool upb_strtable_init2(upb_strtable *t, upb_ctype_t ctype,
                        size_t expected_size, upb_alloc *a) {
  UPB_UNUSED(ctype);  /* TODO(haberman): rm */
  // Multiply by approximate reciprocal of MAX_LOAD (0.85), with pow2 denominator.
  size_t need_entries = (expected_size + 1) * 1204 / 1024;
  UPB_ASSERT(need_entries >= expected_size * 0.85);
  int size_lg2 = _upb_lg2ceil(need_entries);
  return init(&t->t, size_lg2, a);
}

void upb_strtable_clear(upb_strtable *t) {
  size_t bytes = upb_table_size(&t->t) * sizeof(upb_tabent);
  t->t.count = 0;
  memset((char*)t->t.entries, 0, bytes);
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

  if (!init(&new_table.t, size_lg2, a))
    return false;
  upb_strtable_begin(&i, t);
  for ( ; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    upb_strview key = upb_strtable_iter_key(&i);
    upb_strtable_insert3(
        &new_table, key.data, key.size,
        upb_strtable_iter_value(&i), a);
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

  if (isfull(&t->t)) {
    /* Need to resize.  New table of double the size, add old elements to it. */
    if (!upb_strtable_resize(t, t->t.size_lg2 + 1, a)) {
      return false;
    }
  }

  key = strkey2(k, len);
  tabkey = strcopy(key, a);
  if (tabkey == 0) return false;

  hash = table_hash(key.str.str, key.str.len);
  insert(&t->t, key, tabkey, v, hash, &strhash, &streql);
  return true;
}

bool upb_strtable_lookup2(const upb_strtable *t, const char *key, size_t len,
                          upb_value *v) {
  uint32_t hash = table_hash(key, len);
  return lookup(&t->t, strkey2(key, len), v, hash, &streql);
}

bool upb_strtable_remove3(upb_strtable *t, const char *key, size_t len,
                         upb_value *val, upb_alloc *alloc) {
  uint32_t hash = table_hash(key, len);
  upb_tabkey tabkey;
  if (rm(&t->t, strkey2(key, len), val, &tabkey, hash, &streql)) {
    if (alloc) {
      /* Arena-based allocs don't need to free and won't pass this. */
      upb_free(alloc, (void*)tabkey);
    }
    return true;
  } else {
    return false;
  }
}

/* Iteration */

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

upb_strview upb_strtable_iter_key(const upb_strtable_iter *i) {
  upb_strview key;
  uint32_t len;
  UPB_ASSERT(!upb_strtable_done(i));
  key.data = upb_tabstr(str_tabent(i)->key, &len);
  key.size = len;
  return key;
}

upb_value upb_strtable_iter_value(const upb_strtable_iter *i) {
  UPB_ASSERT(!upb_strtable_done(i));
  return _upb_value_val(str_tabent(i)->val.val);
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

bool upb_inttable_sizedinit(upb_inttable *t, size_t asize, int hsize_lg2,
                            upb_alloc *a) {
  size_t array_bytes;

  if (!init(&t->t, hsize_lg2, a)) return false;
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
  UPB_UNUSED(ctype);  /* TODO(haberman): rm */
  return upb_inttable_sizedinit(t, 0, 4, a);
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

  if (key < t->array_size) {
    UPB_ASSERT(!upb_arrhas(t->array[key]));
    t->array_count++;
    mutable_array(t)[key].val = val.val;
  } else {
    if (isfull(&t->t)) {
      /* Need to resize the hash part, but we re-use the array part. */
      size_t i;
      upb_table new_table;

      if (!init(&new_table, t->t.size_lg2 + 1, a)) {
        return false;
      }

      for (i = begin(&t->t); i < upb_table_size(&t->t); i = next(&t->t, i)) {
        const upb_tabent *e = &t->t.entries[i];
        uint32_t hash;
        upb_value v;

        _upb_value_setval(&v, e->val.val);
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
  if (v) _upb_value_setval(v, table_v->val);
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
        _upb_value_setval(val, t->array[key].val);
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

bool upb_inttable_insertptr2(upb_inttable *t, const void *key, upb_value val,
                             upb_alloc *a) {
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

    upb_inttable_sizedinit(&new_t, arr_size, hashsize_lg2, a);
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
      i->array_part ? i->t->array[i->index].val : int_tabent(i)->val.val);
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


#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
  strncpy(status->msg, msg, UPB_STATUS_MAX_MESSAGE - 1);
  status->msg[UPB_STATUS_MAX_MESSAGE - 1] = '\0';
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
  vsnprintf(status->msg, sizeof(status->msg), fmt, args);
  status->msg[UPB_STATUS_MAX_MESSAGE - 1] = '\0';
}

void upb_status_vappenderrf(upb_status *status, const char *fmt, va_list args) {
  size_t len;
  if (!status) return;
  status->ok = false;
  len = strlen(status->msg);
  vsnprintf(status->msg + len, sizeof(status->msg) - len, fmt, args);
  status->msg[UPB_STATUS_MAX_MESSAGE - 1] = '\0';
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

struct mem_block {
  struct mem_block *next;
  uint32_t size;
  uint32_t cleanups;
  /* Data follows. */
};

typedef struct cleanup_ent {
  upb_cleanup_func *cleanup;
  void *ud;
} cleanup_ent;

static const size_t memblock_reserve = UPB_ALIGN_UP(sizeof(mem_block), 16);

static upb_arena *arena_findroot(upb_arena *a) {
  /* Path splitting keeps time complexity down, see:
   *   https://en.wikipedia.org/wiki/Disjoint-set_data_structure */
  while (a->parent != a) {
    upb_arena *next = a->parent;
    a->parent = next->parent;
    a = next;
  }
  return a;
}

static void upb_arena_addblock(upb_arena *a, upb_arena *root, void *ptr,
                               size_t size) {
  mem_block *block = ptr;

  /* The block is for arena |a|, but should appear in the freelist of |root|. */
  block->next = root->freelist;
  block->size = (uint32_t)size;
  block->cleanups = 0;
  root->freelist = block;
  a->last_size = block->size;
  if (!root->freelist_tail) root->freelist_tail = block;

  a->head.ptr = UPB_PTR_AT(block, memblock_reserve, char);
  a->head.end = UPB_PTR_AT(block, size, char);
  a->cleanups = &block->cleanups;

  UPB_POISON_MEMORY_REGION(a->head.ptr, a->head.end - a->head.ptr);
}

static bool upb_arena_allocblock(upb_arena *a, size_t size) {
  upb_arena *root = arena_findroot(a);
  size_t block_size = UPB_MAX(size, a->last_size * 2) + memblock_reserve;
  mem_block *block = upb_malloc(root->block_alloc, block_size);

  if (!block) return false;
  upb_arena_addblock(a, root, block, block_size);
  return true;
}

void *_upb_arena_slowmalloc(upb_arena *a, size_t size) {
  if (!upb_arena_allocblock(a, size)) return NULL;  /* Out of memory. */
  UPB_ASSERT(_upb_arenahas(a) >= size);
  return upb_arena_malloc(a, size);
}

static void *upb_arena_doalloc(upb_alloc *alloc, void *ptr, size_t oldsize,
                               size_t size) {
  upb_arena *a = (upb_arena*)alloc;  /* upb_alloc is initial member. */
  return upb_arena_realloc(a, ptr, oldsize, size);
}

/* Public Arena API ***********************************************************/

upb_arena *arena_initslow(void *mem, size_t n, upb_alloc *alloc) {
  const size_t first_block_overhead = sizeof(upb_arena) + memblock_reserve;
  upb_arena *a;

  /* We need to malloc the initial block. */
  n = first_block_overhead + 256;
  if (!alloc || !(mem = upb_malloc(alloc, n))) {
    return NULL;
  }

  a = UPB_PTR_AT(mem, n - sizeof(*a), upb_arena);
  n -= sizeof(*a);

  a->head.alloc.func = &upb_arena_doalloc;
  a->block_alloc = alloc;
  a->parent = a;
  a->refcount = 1;
  a->freelist = NULL;
  a->freelist_tail = NULL;

  upb_arena_addblock(a, a, mem, n);

  return a;
}

upb_arena *upb_arena_init(void *mem, size_t n, upb_alloc *alloc) {
  upb_arena *a;

  /* Round block size down to alignof(*a) since we will allocate the arena
   * itself at the end. */
  n = UPB_ALIGN_DOWN(n, UPB_ALIGN_OF(upb_arena));

  if (UPB_UNLIKELY(n < sizeof(upb_arena))) {
    return arena_initslow(mem, n, alloc);
  }

  a = UPB_PTR_AT(mem, n - sizeof(*a), upb_arena);

  a->head.alloc.func = &upb_arena_doalloc;
  a->block_alloc = alloc;
  a->parent = a;
  a->refcount = 1;
  a->last_size = UPB_MAX(128, n);
  a->head.ptr = mem;
  a->head.end = UPB_PTR_AT(mem, n - sizeof(*a), char);
  a->freelist = NULL;
  a->cleanups = NULL;

  return a;
}

static void arena_dofree(upb_arena *a) {
  mem_block *block = a->freelist;
  UPB_ASSERT(a->parent == a);
  UPB_ASSERT(a->refcount == 0);

  while (block) {
    /* Load first since we are deleting block. */
    mem_block *next = block->next;

    if (block->cleanups > 0) {
      cleanup_ent *end = UPB_PTR_AT(block, block->size, void);
      cleanup_ent *ptr = end - block->cleanups;

      for (; ptr < end; ptr++) {
        ptr->cleanup(ptr->ud);
      }
    }

    upb_free(a->block_alloc, block);
    block = next;
  }
}

void upb_arena_free(upb_arena *a) {
  a = arena_findroot(a);
  if (--a->refcount == 0) arena_dofree(a);
}

bool upb_arena_addcleanup(upb_arena *a, void *ud, upb_cleanup_func *func) {
  cleanup_ent *ent;

  if (!a->cleanups || _upb_arenahas(a) < sizeof(cleanup_ent)) {
    if (!upb_arena_allocblock(a, 128)) return false;  /* Out of memory. */
    UPB_ASSERT(_upb_arenahas(a) >= sizeof(cleanup_ent));
  }

  a->head.end -= sizeof(cleanup_ent);
  ent = (cleanup_ent*)a->head.end;
  (*a->cleanups)++;
  UPB_UNPOISON_MEMORY_REGION(ent, sizeof(cleanup_ent));

  ent->cleanup = func;
  ent->ud = ud;

  return true;
}

void upb_arena_fuse(upb_arena *a1, upb_arena *a2) {
  upb_arena *r1 = arena_findroot(a1);
  upb_arena *r2 = arena_findroot(a2);

  if (r1 == r2) return;  /* Already fused. */

  /* We want to join the smaller tree to the larger tree.
   * So swap first if they are backwards. */
  if (r1->refcount < r2->refcount) {
    upb_arena *tmp = r1;
    r1 = r2;
    r2 = tmp;
  }

  /* r1 takes over r2's freelist and refcount. */
  r1->refcount += r2->refcount;
  if (r2->freelist_tail) {
    UPB_ASSERT(r2->freelist_tail->next == NULL);
    r2->freelist_tail->next = r1->freelist;
    r1->freelist = r2->freelist;
  }
  r2->parent = r1;
}
// Fast decoder: ~3x the speed of decode.c, but x86-64 specific.
// Also the table size grows by 2x.
//
// Could potentially be ported to ARM64 or other 64-bit archs that pass at
// least six arguments in registers.
//
// The overall design is to create specialized functions for every possible
// field type (eg. oneof boolean field with a 1 byte tag) and then dispatch
// to the specialized function as quickly as possible.



/* Must be last. */

#if UPB_FASTTABLE

// The standard set of arguments passed to each parsing function.
// Thanks to x86-64 calling conventions, these will stay in registers.
#define UPB_PARSE_PARAMS                                          \
  upb_decstate *d, const char *ptr, upb_msg *msg, intptr_t table, \
      uint64_t hasbits, uint64_t data

#define UPB_PARSE_ARGS d, ptr, msg, table, hasbits, data

#define RETURN_GENERIC(m)  \
  /* fprintf(stderr, m); */ \
  return fastdecode_generic(d, ptr, msg, table, hasbits, 0);

typedef enum {
  CARD_s = 0,  /* Singular (optional, non-repeated) */
  CARD_o = 1,  /* Oneof */
  CARD_r = 2,  /* Repeated */
  CARD_p = 3   /* Packed Repeated */
} upb_card;

UPB_NOINLINE
static const char *fastdecode_isdonefallback(upb_decstate *d, const char *ptr,
                                             upb_msg *msg, intptr_t table,
                                             uint64_t hasbits, int overrun) {
  ptr = decode_isdonefallback_inl(d, ptr, overrun);
  if (ptr == NULL) {
    return fastdecode_err(d);
  }
  uint16_t tag = fastdecode_loadtag(ptr);
  return fastdecode_tagdispatch(d, ptr, msg, table, hasbits, tag);
}

UPB_FORCEINLINE
static const char *fastdecode_dispatch(upb_decstate *d, const char *ptr,
                                       upb_msg *msg, intptr_t table,
                                       uint64_t hasbits) {
  if (UPB_UNLIKELY(ptr >= d->limit_ptr)) {
    int overrun = ptr - d->end;
    if (UPB_LIKELY(overrun == d->limit)) {
      // Parse is finished.
      *(uint32_t*)msg |= hasbits;  // Sync hasbits.
      return ptr;
    } else {
      return fastdecode_isdonefallback(d, ptr, msg, table, hasbits, overrun);
    }
  }

  // Read two bytes of tag data (for a one-byte tag, the high byte is junk).
  uint16_t tag = fastdecode_loadtag(ptr);
  return fastdecode_tagdispatch(d, ptr, msg, table, hasbits, tag);
}

UPB_FORCEINLINE
static bool fastdecode_checktag(uint64_t data, int tagbytes) {
  if (tagbytes == 1) {
    return (data & 0xff) == 0;
  } else {
    return (data & 0xffff) == 0;
  }
}

UPB_FORCEINLINE
static const char *fastdecode_longsize(const char *ptr, int *size) {
  int i;
  UPB_ASSERT(*size & 0x80);
  *size &= 0xff;
  for (i = 0; i < 3; i++) {
    ptr++;
    size_t byte = (uint8_t)ptr[-1];
    *size += (byte - 1) << (7 + 7 * i);
    if (UPB_LIKELY((byte & 0x80) == 0)) return ptr;
  }
  ptr++;
  size_t byte = (uint8_t)ptr[-1];
  // len is limited by 2gb not 4gb, hence 8 and not 16 as normally expected
  // for a 32 bit varint.
  if (UPB_UNLIKELY(byte >= 8)) return NULL;
  *size += (byte - 1) << 28;
  return ptr;
}

UPB_FORCEINLINE
static bool fastdecode_boundscheck(const char *ptr, size_t len,
                                   const char *end) {
  uintptr_t uptr = (uintptr_t)ptr;
  uintptr_t uend = (uintptr_t)end + 16;
  uintptr_t res = uptr + len;
  return res < uptr || res > uend;
}

UPB_FORCEINLINE
static bool fastdecode_boundscheck2(const char *ptr, size_t len,
                                    const char *end) {
  // This is one extra branch compared to the more normal:
  //   return (size_t)(end - ptr) < size;
  // However it is one less computation if we are just about to use "ptr + len":
  //   https://godbolt.org/z/35YGPz
  // In microbenchmarks this shows an overall 4% improvement.
  uintptr_t uptr = (uintptr_t)ptr;
  uintptr_t uend = (uintptr_t)end;
  uintptr_t res = uptr + len;
  return res < uptr || res > uend;
}

typedef const char *fastdecode_delimfunc(upb_decstate *d, const char *ptr,
                                         void *ctx);

UPB_FORCEINLINE
static const char *fastdecode_delimited(upb_decstate *d, const char *ptr,
                                        fastdecode_delimfunc *func, void *ctx) {
  ptr++;
  int len = (int8_t)ptr[-1];
  if (fastdecode_boundscheck2(ptr, len, d->limit_ptr)) {
    // Slow case: Sub-message is >=128 bytes and/or exceeds the current buffer.
    // If it exceeds the buffer limit, limit/limit_ptr will change during
    // sub-message parsing, so we need to preserve delta, not limit.
    if (UPB_UNLIKELY(len & 0x80)) {
      // Size varint >1 byte (length >= 128).
      ptr = fastdecode_longsize(ptr, &len);
      if (!ptr) {
        // Corrupt wire format: size exceeded INT_MAX.
        return NULL;
      }
    }
    if (ptr - d->end + (int)len > d->limit) {
      // Corrupt wire format: invalid limit.
      return NULL;
    }
    int delta = decode_pushlimit(d, ptr, len);
    ptr = func(d, ptr, ctx);
    decode_poplimit(d, ptr, delta);
  } else {
    // Fast case: Sub-message is <128 bytes and fits in the current buffer.
    // This means we can preserve limit/limit_ptr verbatim.
    const char *saved_limit_ptr = d->limit_ptr;
    int saved_limit = d->limit;
    d->limit_ptr = ptr + len;
    d->limit = d->limit_ptr - d->end;
    UPB_ASSERT(d->limit_ptr == d->end + UPB_MIN(0, d->limit));
    ptr = func(d, ptr, ctx);
    d->limit_ptr = saved_limit_ptr;
    d->limit = saved_limit;
    UPB_ASSERT(d->limit_ptr == d->end + UPB_MIN(0, d->limit));
  }
  return ptr;
}

/* singular, oneof, repeated field handling ***********************************/

typedef struct {
  upb_array *arr;
  void *end;
} fastdecode_arr;

typedef enum {
  FD_NEXT_ATLIMIT,
  FD_NEXT_SAMEFIELD,
  FD_NEXT_OTHERFIELD
} fastdecode_next;

typedef struct {
  void *dst;
  fastdecode_next next;
  uint32_t tag;
} fastdecode_nextret;

UPB_FORCEINLINE
static void *fastdecode_resizearr(upb_decstate *d, void *dst,
                                  fastdecode_arr *farr, int valbytes) {
  if (UPB_UNLIKELY(dst == farr->end)) {
    size_t old_size = farr->arr->size;
    size_t old_bytes = old_size * valbytes;
    size_t new_size = old_size * 2;
    size_t new_bytes = new_size * valbytes;
    char *old_ptr = _upb_array_ptr(farr->arr);
    char *new_ptr = upb_arena_realloc(&d->arena, old_ptr, old_bytes, new_bytes);
    uint8_t elem_size_lg2 = __builtin_ctz(valbytes);
    farr->arr->size = new_size;
    farr->arr->data = _upb_array_tagptr(new_ptr, elem_size_lg2);
    dst = (void*)(new_ptr + (old_size * valbytes));
    farr->end = (void*)(new_ptr + (new_size * valbytes));
  }
  return dst;
}

UPB_FORCEINLINE
static bool fastdecode_tagmatch(uint32_t tag, uint64_t data, int tagbytes) {
  if (tagbytes == 1) {
    return (uint8_t)tag == (uint8_t)data;
  } else {
    return (uint16_t)tag == (uint16_t)data;
  }
}

UPB_FORCEINLINE
static void fastdecode_commitarr(void *dst, fastdecode_arr *farr,
                                 int valbytes) {
  farr->arr->len =
      (size_t)((char *)dst - (char *)_upb_array_ptr(farr->arr)) / valbytes;
}

UPB_FORCEINLINE
static fastdecode_nextret fastdecode_nextrepeated(upb_decstate *d, void *dst,
                                                  const char **ptr,
                                                  fastdecode_arr *farr,
                                                  uint64_t data, int tagbytes,
                                                  int valbytes) {
  fastdecode_nextret ret;
  dst = (char *)dst + valbytes;

  if (UPB_LIKELY(!decode_isdone(d, ptr))) {
    ret.tag = fastdecode_loadtag(*ptr);
    if (fastdecode_tagmatch(ret.tag, data, tagbytes)) {
      ret.next = FD_NEXT_SAMEFIELD;
    } else {
      fastdecode_commitarr(dst, farr, valbytes);
      ret.next = FD_NEXT_OTHERFIELD;
    }
  } else {
    fastdecode_commitarr(dst, farr, valbytes);
    ret.next = FD_NEXT_ATLIMIT;
  }

  ret.dst = dst;
  return ret;
}

UPB_FORCEINLINE
static void *fastdecode_fieldmem(upb_msg *msg, uint64_t data) {
  size_t ofs = data >> 48;
  return (char *)msg + ofs;
}

UPB_FORCEINLINE
static void *fastdecode_getfield(upb_decstate *d, const char *ptr, upb_msg *msg,
                                 uint64_t *data, uint64_t *hasbits,
                                 fastdecode_arr *farr, int valbytes,
                                 upb_card card) {
  switch (card) {
    case CARD_s: {
      uint8_t hasbit_index = *data >> 24;
      // Set hasbit and return pointer to scalar field.
      *hasbits |= 1ull << hasbit_index;
      return fastdecode_fieldmem(msg, *data);
    }
    case CARD_o: {
      uint16_t case_ofs = *data >> 32;
      uint32_t *oneof_case = UPB_PTR_AT(msg, case_ofs, uint32_t);
      uint8_t field_number = *data >> 24;
      *oneof_case = field_number;
      return fastdecode_fieldmem(msg, *data);
    }
    case CARD_r: {
      // Get pointer to upb_array and allocate/expand if necessary.
      uint8_t elem_size_lg2 = __builtin_ctz(valbytes);
      upb_array **arr_p = fastdecode_fieldmem(msg, *data);
      char *begin;
      *(uint32_t*)msg |= *hasbits;
      *hasbits = 0;
      if (UPB_LIKELY(!*arr_p)) {
        farr->arr = _upb_array_new(&d->arena, 8, elem_size_lg2);
        *arr_p = farr->arr;
      } else {
        farr->arr = *arr_p;
      }
      begin = _upb_array_ptr(farr->arr);
      farr->end = begin + (farr->arr->size * valbytes);
      *data = fastdecode_loadtag(ptr);
      return begin + (farr->arr->len * valbytes);
    }
    default:
      UPB_UNREACHABLE();
  }
}

UPB_FORCEINLINE
static bool fastdecode_flippacked(uint64_t *data, int tagbytes) {
  *data ^= (0x2 ^ 0x0);  // Patch data to match packed wiretype.
  return fastdecode_checktag(*data, tagbytes);
}

/* varint fields **************************************************************/

UPB_FORCEINLINE
static uint64_t fastdecode_munge(uint64_t val, int valbytes, bool zigzag) {
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
static const char *fastdecode_varint64(const char *ptr, uint64_t *val) {
  ptr++;
  *val = (uint8_t)ptr[-1];
  if (UPB_UNLIKELY(*val & 0x80)) {
    int i;
    for (i = 0; i < 8; i++) {
      ptr++;
      uint64_t byte = (uint8_t)ptr[-1];
      *val += (byte - 1) << (7 + 7 * i);
      if (UPB_LIKELY((byte & 0x80) == 0)) goto done;
    }
    ptr++;
    uint64_t byte = (uint8_t)ptr[-1];
    if (byte > 1) {
      return NULL;
    }
    *val += (byte - 1) << 63;
  }
done:
  UPB_ASSUME(ptr != NULL);
  return ptr;
}

UPB_FORCEINLINE
static const char *fastdecode_unpackedvarint(UPB_PARSE_PARAMS, int tagbytes,
                                             int valbytes, upb_card card,
                                             bool zigzag,
                                             _upb_field_parser *packed) {
  uint64_t val;
  void *dst;
  fastdecode_arr farr;

  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    if (card == CARD_r && fastdecode_flippacked(&data, tagbytes)) {
      return packed(UPB_PARSE_ARGS);
    }
    RETURN_GENERIC("varint field tag mismatch\n");
  }

  dst =
      fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr, valbytes, card);
  if (card == CARD_r) {
    if (UPB_UNLIKELY(!dst)) {
      RETURN_GENERIC("need array resize\n");
    }
  }

again:
  if (card == CARD_r) {
    dst = fastdecode_resizearr(d, dst, &farr, valbytes);
  }

  ptr += tagbytes;
  ptr = fastdecode_varint64(ptr, &val);
  if (ptr == NULL) return fastdecode_err(d);
  val = fastdecode_munge(val, valbytes, zigzag);
  memcpy(dst, &val, valbytes);

  if (card == CARD_r) {
    fastdecode_nextret ret =
        fastdecode_nextrepeated(d, dst, &ptr, &farr, data, tagbytes, valbytes);
    switch (ret.next) {
      case FD_NEXT_SAMEFIELD:
        dst = ret.dst;
        goto again;
      case FD_NEXT_OTHERFIELD:
        return fastdecode_tagdispatch(d, ptr, msg, table, hasbits, ret.tag);
      case FD_NEXT_ATLIMIT:
        return ptr;
    }
  }

  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
}

typedef struct {
  uint8_t valbytes;
  bool zigzag;
  void *dst;
  fastdecode_arr farr;
} fastdecode_varintdata;

UPB_FORCEINLINE
static const char *fastdecode_topackedvarint(upb_decstate *d, const char *ptr,
                                             void *ctx) {
  fastdecode_varintdata *data = ctx;
  void *dst = data->dst;
  uint64_t val;

  while (!decode_isdone(d, &ptr)) {
    dst = fastdecode_resizearr(d, dst, &data->farr, data->valbytes);
    ptr = fastdecode_varint64(ptr, &val);
    if (ptr == NULL) return NULL;
    val = fastdecode_munge(val, data->valbytes, data->zigzag);
    memcpy(dst, &val, data->valbytes);
    dst = (char *)dst + data->valbytes;
  }

  fastdecode_commitarr(dst, &data->farr, data->valbytes);
  return ptr;
}

UPB_FORCEINLINE
static const char *fastdecode_packedvarint(UPB_PARSE_PARAMS, int tagbytes,
                                           int valbytes, bool zigzag,
                                           _upb_field_parser *unpacked) {
  fastdecode_varintdata ctx = {valbytes, zigzag};

  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    if (fastdecode_flippacked(&data, tagbytes)) {
      return unpacked(UPB_PARSE_ARGS);
    } else {
      RETURN_GENERIC("varint field tag mismatch\n");
    }
  }

  ctx.dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &ctx.farr,
                                valbytes, CARD_r);
  if (UPB_UNLIKELY(!ctx.dst)) {
    RETURN_GENERIC("need array resize\n");
  }

  ptr += tagbytes;
  ptr = fastdecode_delimited(d, ptr, &fastdecode_topackedvarint, &ctx);

  if (UPB_UNLIKELY(ptr == NULL)) {
    return fastdecode_err(d);
  }

  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
}

UPB_FORCEINLINE
static const char *fastdecode_varint(UPB_PARSE_PARAMS, int tagbytes,
                                     int valbytes, upb_card card, bool zigzag,
                                     _upb_field_parser *unpacked,
                                     _upb_field_parser *packed) {
  if (card == CARD_p) {
    return fastdecode_packedvarint(UPB_PARSE_ARGS, tagbytes, valbytes, zigzag,
                                   unpacked);
  } else {
    return fastdecode_unpackedvarint(UPB_PARSE_ARGS, tagbytes, valbytes, card,
                                     zigzag, packed);
  }
}

#define z_ZZ true
#define b_ZZ false
#define v_ZZ false

/* Generate all combinations:
 * {s,o,r,p} x {b1,v4,z4,v8,z8} x {1bt,2bt} */

#define F(card, type, valbytes, tagbytes)                                      \
  UPB_NOINLINE                                                                 \
  const char *upb_p##card##type##valbytes##_##tagbytes##bt(UPB_PARSE_PARAMS) { \
    return fastdecode_varint(UPB_PARSE_ARGS, tagbytes, valbytes, CARD_##card,  \
                             type##_ZZ,                                        \
                             &upb_pr##type##valbytes##_##tagbytes##bt,         \
                             &upb_pp##type##valbytes##_##tagbytes##bt);        \
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
TAGBYTES(r)
TAGBYTES(p)

#undef z_ZZ
#undef b_ZZ
#undef v_ZZ
#undef o_ONEOF
#undef s_ONEOF
#undef r_ONEOF
#undef F
#undef TYPES
#undef TAGBYTES


/* fixed fields ***************************************************************/

UPB_FORCEINLINE
static const char *fastdecode_unpackedfixed(UPB_PARSE_PARAMS, int tagbytes,
                                            int valbytes, upb_card card,
                                            _upb_field_parser *packed) {
  void *dst;
  fastdecode_arr farr;

  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    if (card == CARD_r && fastdecode_flippacked(&data, tagbytes)) {
      return packed(UPB_PARSE_ARGS);
    }
    RETURN_GENERIC("fixed field tag mismatch\n");
  }

  dst =
      fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr, valbytes, card);
  if (card == CARD_r) {
    if (UPB_UNLIKELY(!dst)) {
      RETURN_GENERIC("couldn't allocate array in arena\n");
    }
  }


again:
  if (card == CARD_r) {
    dst = fastdecode_resizearr(d, dst, &farr, valbytes);
  }

  ptr += tagbytes;
  memcpy(dst, ptr, valbytes);
  ptr += valbytes;

  if (card == CARD_r) {
    fastdecode_nextret ret =
        fastdecode_nextrepeated(d, dst, &ptr, &farr, data, tagbytes, valbytes);
    switch (ret.next) {
      case FD_NEXT_SAMEFIELD:
        dst = ret.dst;
        goto again;
      case FD_NEXT_OTHERFIELD:
        return fastdecode_tagdispatch(d, ptr, msg, table, hasbits, ret.tag);
      case FD_NEXT_ATLIMIT:
        return ptr;
    }
  }

  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
}

UPB_FORCEINLINE
static const char *fastdecode_packedfixed(UPB_PARSE_PARAMS, int tagbytes,
                                          int valbytes,
                                          _upb_field_parser *unpacked) {
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    if (fastdecode_flippacked(&data, tagbytes)) {
      return unpacked(UPB_PARSE_ARGS);
    } else {
      RETURN_GENERIC("varint field tag mismatch\n");
    }
  }

  ptr += tagbytes;
  int size = (uint8_t)ptr[0];
  ptr++;
  if (size & 0x80) {
    ptr = fastdecode_longsize(ptr, &size);
  }

  if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, size, d->limit_ptr)) ||
      (size % valbytes) != 0) {
    return fastdecode_err(d);
  }

  upb_array **arr_p = fastdecode_fieldmem(msg, data);
  upb_array *arr = *arr_p;
  uint8_t elem_size_lg2 = __builtin_ctz(valbytes);
  int elems = size / valbytes;

  if (UPB_LIKELY(!arr)) {
    *arr_p = arr = _upb_array_new(&d->arena, elems, elem_size_lg2);
    if (!arr) {
      return fastdecode_err(d);
    }
  } else {
    _upb_array_resize(arr, elems, &d->arena);
  }

  char *dst = _upb_array_ptr(arr);
  memcpy(dst, ptr, size);
  arr->len = elems;

  return fastdecode_dispatch(d, ptr + size, msg, table, hasbits);
}

UPB_FORCEINLINE
static const char *fastdecode_fixed(UPB_PARSE_PARAMS, int tagbytes,
                                    int valbytes, upb_card card,
                                    _upb_field_parser *unpacked,
                                    _upb_field_parser *packed) {
  if (card == CARD_p) {
    return fastdecode_packedfixed(UPB_PARSE_ARGS, tagbytes, valbytes, unpacked);
  } else {
    return fastdecode_unpackedfixed(UPB_PARSE_ARGS, tagbytes, valbytes, card,
                                    packed);
  }
}

/* Generate all combinations:
 * {s,o,r,p} x {f4,f8} x {1bt,2bt} */

#define F(card, valbytes, tagbytes)                                          \
  UPB_NOINLINE                                                               \
  const char *upb_p##card##f##valbytes##_##tagbytes##bt(UPB_PARSE_PARAMS) {  \
    return fastdecode_fixed(UPB_PARSE_ARGS, tagbytes, valbytes, CARD_##card, \
                            &upb_ppf##valbytes##_##tagbytes##bt,             \
                            &upb_prf##valbytes##_##tagbytes##bt);            \
  }

#define TYPES(card, tagbytes) \
  F(card, 4, tagbytes)        \
  F(card, 8, tagbytes)

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

typedef const char *fastdecode_copystr_func(struct upb_decstate *d,
                                            const char *ptr, upb_msg *msg,
                                            const upb_msglayout *table,
                                            uint64_t hasbits, upb_strview *dst);

UPB_NOINLINE
static const char *fastdecode_verifyutf8(upb_decstate *d, const char *ptr,
                                         upb_msg *msg, intptr_t table,
                                         uint64_t hasbits, upb_strview *dst) {
  if (!decode_verifyutf8_inl(dst->data, dst->size)) {
    return fastdecode_err(d);
  }
  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
}

UPB_FORCEINLINE
static const char *fastdecode_longstring(struct upb_decstate *d,
                                         const char *ptr, upb_msg *msg,
                                         intptr_t table, uint64_t hasbits,
                                         upb_strview *dst,
                                         bool validate_utf8) {
  int size = (uint8_t)ptr[0];  // Could plumb through hasbits.
  ptr++;
  if (size & 0x80) {
    ptr = fastdecode_longsize(ptr, &size);
  }

  if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, size, d->limit_ptr))) {
    dst->size = 0;
    return fastdecode_err(d);
  }

  if (d->alias) {
    dst->data = ptr;
    dst->size = size;
  } else {
    char *data = upb_arena_malloc(&d->arena, size);
    if (!data) {
      return fastdecode_err(d);
    }
    memcpy(data, ptr, size);
    dst->data = data;
    dst->size = size;
  }

  if (validate_utf8) {
    return fastdecode_verifyutf8(d, ptr + size, msg, table, hasbits, dst);
  } else {
    return fastdecode_dispatch(d, ptr + size, msg, table, hasbits);
  }
}

UPB_NOINLINE
static const char *fastdecode_longstring_utf8(struct upb_decstate *d,
                                         const char *ptr, upb_msg *msg,
                                         intptr_t table, uint64_t hasbits,
                                         upb_strview *dst) {
  return fastdecode_longstring(d, ptr, msg, table, hasbits, dst, true);
}

UPB_NOINLINE
static const char *fastdecode_longstring_noutf8(struct upb_decstate *d,
                                                const char *ptr, upb_msg *msg,
                                                intptr_t table,
                                                uint64_t hasbits,
                                                upb_strview *dst) {
  return fastdecode_longstring(d, ptr, msg, table, hasbits, dst, false);
}

UPB_FORCEINLINE
static void fastdecode_docopy(upb_decstate *d, const char *ptr, uint32_t size,
                              int copy, char *data, upb_strview *dst) {
  d->arena.head.ptr += copy;
  dst->data = data;
  UPB_UNPOISON_MEMORY_REGION(data, copy);
  memcpy(data, ptr, copy);
  UPB_POISON_MEMORY_REGION(data + size, copy - size);
}

UPB_FORCEINLINE
static const char *fastdecode_copystring(UPB_PARSE_PARAMS, int tagbytes,
                                         upb_card card, bool validate_utf8) {
  upb_strview *dst;
  fastdecode_arr farr;
  int64_t size;
  size_t arena_has;
  size_t common_has;
  char *buf;

  UPB_ASSERT(!d->alias);
  UPB_ASSERT(fastdecode_checktag(data, tagbytes));

  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr,
                            sizeof(upb_strview), card);

again:
  if (card == CARD_r) {
    dst = fastdecode_resizearr(d, dst, &farr, sizeof(upb_strview));
  }

  size = (uint8_t)ptr[tagbytes];
  ptr += tagbytes + 1;
  dst->size = size;

  buf = d->arena.head.ptr;
  arena_has = _upb_arenahas(&d->arena);
  common_has = UPB_MIN(arena_has, (d->end - ptr) + 16);

  if (UPB_LIKELY(size <= 15 - tagbytes)) {
    if (arena_has < 16) goto longstr;
    d->arena.head.ptr += 16;
    memcpy(buf, ptr - tagbytes - 1, 16);
    dst->data = buf + tagbytes + 1;
  } else if (UPB_LIKELY(size <= 32)) {
    if (UPB_UNLIKELY(common_has < 32)) goto longstr;
    fastdecode_docopy(d, ptr, size, 32, buf, dst);
  } else if (UPB_LIKELY(size <= 64)) {
    if (UPB_UNLIKELY(common_has < 64)) goto longstr;
    fastdecode_docopy(d, ptr, size, 64, buf, dst);
  } else if (UPB_LIKELY(size < 128)) {
    if (UPB_UNLIKELY(common_has < 128)) goto longstr;
    fastdecode_docopy(d, ptr, size, 128, buf, dst);
  } else {
    goto longstr;
  }

  ptr += size;

  if (card == CARD_r) {
    if (validate_utf8 && !decode_verifyutf8_inl(dst->data, dst->size)) {
      return fastdecode_err(d);
    }
    fastdecode_nextret ret = fastdecode_nextrepeated(
        d, dst, &ptr, &farr, data, tagbytes, sizeof(upb_strview));
    switch (ret.next) {
      case FD_NEXT_SAMEFIELD:
        dst = ret.dst;
        goto again;
      case FD_NEXT_OTHERFIELD:
        return fastdecode_tagdispatch(d, ptr, msg, table, hasbits, ret.tag);
      case FD_NEXT_ATLIMIT:
        return ptr;
    }
  }

  if (card != CARD_r && validate_utf8) {
    return fastdecode_verifyutf8(d, ptr, msg, table, hasbits, dst);
  }

  return fastdecode_dispatch(d, ptr, msg, table, hasbits);

longstr:
  ptr--;
  if (validate_utf8) {
    return fastdecode_longstring_utf8(d, ptr, msg, table, hasbits, dst);
  } else {
    return fastdecode_longstring_noutf8(d, ptr, msg, table, hasbits, dst);
  }
}

UPB_FORCEINLINE
static const char *fastdecode_string(UPB_PARSE_PARAMS, int tagbytes,
                                     upb_card card, _upb_field_parser *copyfunc,
                                     bool validate_utf8) {
  upb_strview *dst;
  fastdecode_arr farr;
  int64_t size;

  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    RETURN_GENERIC("string field tag mismatch\n");
  }

  if (UPB_UNLIKELY(!d->alias)) {
    return copyfunc(UPB_PARSE_ARGS);
  }

  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr,
                            sizeof(upb_strview), card);

again:
  if (card == CARD_r) {
    dst = fastdecode_resizearr(d, dst, &farr, sizeof(upb_strview));
  }

  size = (int8_t)ptr[tagbytes];
  ptr += tagbytes + 1;
  dst->data = ptr;
  dst->size = size;

  if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, size, d->end))) {
    ptr--;
    if (validate_utf8) {
      return fastdecode_longstring_utf8(d, ptr, msg, table, hasbits, dst);
    } else {
      return fastdecode_longstring_noutf8(d, ptr, msg, table, hasbits, dst);
    }
  }

  ptr += size;

  if (card == CARD_r) {
    if (validate_utf8 && !decode_verifyutf8_inl(dst->data, dst->size)) {
      return fastdecode_err(d);
    }
    fastdecode_nextret ret = fastdecode_nextrepeated(
        d, dst, &ptr, &farr, data, tagbytes, sizeof(upb_strview));
    switch (ret.next) {
      case FD_NEXT_SAMEFIELD:
        dst = ret.dst;
        if (UPB_UNLIKELY(!d->alias)) {
          // Buffer flipped and we can't alias any more. Bounce to copyfunc(),
          // but via dispatch since we need to reload table data also.
          fastdecode_commitarr(dst, &farr, sizeof(upb_strview));
          return fastdecode_tagdispatch(d, ptr, msg, table, hasbits, ret.tag);
        }
        goto again;
      case FD_NEXT_OTHERFIELD:
        return fastdecode_tagdispatch(d, ptr, msg, table, hasbits, ret.tag);
      case FD_NEXT_ATLIMIT:
        return ptr;
    }
  }

  if (card != CARD_r && validate_utf8) {
    return fastdecode_verifyutf8(d, ptr, msg, table, hasbits, dst);
  }

  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
}

/* Generate all combinations:
 * {p,c} x {s,o,r} x {s, b} x {1bt,2bt} */

#define s_VALIDATE true
#define b_VALIDATE false

#define F(card, tagbytes, type)                                         \
  UPB_NOINLINE                                                          \
  const char *upb_c##card##type##_##tagbytes##bt(UPB_PARSE_PARAMS) {    \
    return fastdecode_copystring(UPB_PARSE_ARGS, tagbytes, CARD_##card, \
                                 type##_VALIDATE);                      \
  }                                                                     \
  const char *upb_p##card##type##_##tagbytes##bt(UPB_PARSE_PARAMS) {    \
    return fastdecode_string(UPB_PARSE_ARGS, tagbytes, CARD_##card,     \
                             &upb_c##card##type##_##tagbytes##bt,       \
                             type##_VALIDATE);                          \
  }

#define UTF8(card, tagbytes) \
  F(card, tagbytes, s)       \
  F(card, tagbytes, b)

#define TAGBYTES(card) \
  UTF8(card, 1)        \
  UTF8(card, 2)

TAGBYTES(s)
TAGBYTES(o)
TAGBYTES(r)

#undef s_VALIDATE
#undef b_VALIDATE
#undef F
#undef TAGBYTES

/* message fields *************************************************************/

UPB_INLINE
upb_msg *decode_newmsg_ceil(upb_decstate *d, const upb_msglayout *l,
                            int msg_ceil_bytes) {
  size_t size = l->size + sizeof(upb_msg_internal);
  char *msg_data;
  if (UPB_LIKELY(msg_ceil_bytes > 0 &&
                 _upb_arenahas(&d->arena) >= msg_ceil_bytes)) {
    UPB_ASSERT(size <= (size_t)msg_ceil_bytes);
    msg_data = d->arena.head.ptr;
    d->arena.head.ptr += size;
    UPB_UNPOISON_MEMORY_REGION(msg_data, msg_ceil_bytes);
    memset(msg_data, 0, msg_ceil_bytes);
    UPB_POISON_MEMORY_REGION(msg_data + size, msg_ceil_bytes - size);
  } else {
    msg_data = (char*)upb_arena_malloc(&d->arena, size);
    memset(msg_data, 0, size);
  }
  return msg_data + sizeof(upb_msg_internal);
}

typedef struct {
  intptr_t table;
  upb_msg *msg;
} fastdecode_submsgdata;

UPB_FORCEINLINE
static const char *fastdecode_tosubmsg(upb_decstate *d, const char *ptr,
                                       void *ctx) {
  fastdecode_submsgdata *submsg = ctx;
  ptr = fastdecode_dispatch(d, ptr, submsg->msg, submsg->table, 0);
  UPB_ASSUME(ptr != NULL);
  return ptr;
}

UPB_FORCEINLINE
static const char *fastdecode_submsg(UPB_PARSE_PARAMS, int tagbytes,
                                     int msg_ceil_bytes, upb_card card) {

  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    RETURN_GENERIC("submessage field tag mismatch\n");
  }

  if (--d->depth == 0) return fastdecode_err(d);

  upb_msg **dst;
  uint32_t submsg_idx = (data >> 16) & 0xff;
  const upb_msglayout *tablep = decode_totablep(table);
  const upb_msglayout *subtablep = tablep->submsgs[submsg_idx];
  fastdecode_submsgdata submsg = {decode_totable(subtablep)};
  fastdecode_arr farr;

  if (subtablep->table_mask == (uint8_t)-1) {
    RETURN_GENERIC("submessage doesn't have fast tables.");
  }

  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr,
                            sizeof(upb_msg *), card);

  if (card == CARD_s) {
    *(uint32_t*)msg |= hasbits;
    hasbits = 0;
  }

again:
  if (card == CARD_r) {
    dst = fastdecode_resizearr(d, dst, &farr, sizeof(upb_msg*));
  }

  submsg.msg = *dst;

  if (card == CARD_r || UPB_LIKELY(!submsg.msg)) {
    *dst = submsg.msg = decode_newmsg_ceil(d, subtablep, msg_ceil_bytes);
  }

  ptr += tagbytes;
  ptr = fastdecode_delimited(d, ptr, fastdecode_tosubmsg, &submsg);

  if (UPB_UNLIKELY(ptr == NULL || d->end_group != DECODE_NOGROUP)) {
    return fastdecode_err(d);
  }

  if (card == CARD_r) {
    fastdecode_nextret ret = fastdecode_nextrepeated(
        d, dst, &ptr, &farr, data, tagbytes, sizeof(upb_msg *));
    switch (ret.next) {
      case FD_NEXT_SAMEFIELD:
        dst = ret.dst;
        goto again;
      case FD_NEXT_OTHERFIELD:
        d->depth++;
        return fastdecode_tagdispatch(d, ptr, msg, table, hasbits, ret.tag);
      case FD_NEXT_ATLIMIT:
        d->depth++;
        return ptr;
    }
  }

  d->depth++;
  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
}

#define F(card, tagbytes, size_ceil, ceil_arg)                                 \
  const char *upb_p##card##m_##tagbytes##bt_max##size_ceil##b(                 \
      UPB_PARSE_PARAMS) {                                                      \
    return fastdecode_submsg(UPB_PARSE_ARGS, tagbytes, ceil_arg, CARD_##card); \
  }

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

#endif  /* UPB_FASTTABLE */
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
  UPB_SIZE(8, 8), 1, false, 255,
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
  {1, UPB_SIZE(4, 8), 1, 0, 12, 1},
  {2, UPB_SIZE(12, 24), 2, 0, 12, 1},
  {3, UPB_SIZE(36, 72), 0, 0, 12, 3},
  {4, UPB_SIZE(40, 80), 0, 0, 11, 3},
  {5, UPB_SIZE(44, 88), 0, 1, 11, 3},
  {6, UPB_SIZE(48, 96), 0, 4, 11, 3},
  {7, UPB_SIZE(52, 104), 0, 2, 11, 3},
  {8, UPB_SIZE(28, 56), 3, 3, 11, 1},
  {9, UPB_SIZE(32, 64), 4, 5, 11, 1},
  {10, UPB_SIZE(56, 112), 0, 0, 5, 3},
  {11, UPB_SIZE(60, 120), 0, 0, 5, 3},
  {12, UPB_SIZE(20, 40), 5, 0, 12, 1},
};

const upb_msglayout google_protobuf_FileDescriptorProto_msginit = {
  &google_protobuf_FileDescriptorProto_submsgs[0],
  &google_protobuf_FileDescriptorProto__fields[0],
  UPB_SIZE(64, 128), 12, false, 255,
};

static const upb_msglayout *const google_protobuf_DescriptorProto_submsgs[7] = {
  &google_protobuf_DescriptorProto_msginit,
  &google_protobuf_DescriptorProto_ExtensionRange_msginit,
  &google_protobuf_DescriptorProto_ReservedRange_msginit,
  &google_protobuf_EnumDescriptorProto_msginit,
  &google_protobuf_FieldDescriptorProto_msginit,
  &google_protobuf_MessageOptions_msginit,
  &google_protobuf_OneofDescriptorProto_msginit,
};

static const upb_msglayout_field google_protobuf_DescriptorProto__fields[10] = {
  {1, UPB_SIZE(4, 8), 1, 0, 12, 1},
  {2, UPB_SIZE(16, 32), 0, 4, 11, 3},
  {3, UPB_SIZE(20, 40), 0, 0, 11, 3},
  {4, UPB_SIZE(24, 48), 0, 3, 11, 3},
  {5, UPB_SIZE(28, 56), 0, 1, 11, 3},
  {6, UPB_SIZE(32, 64), 0, 4, 11, 3},
  {7, UPB_SIZE(12, 24), 2, 5, 11, 1},
  {8, UPB_SIZE(36, 72), 0, 6, 11, 3},
  {9, UPB_SIZE(40, 80), 0, 2, 11, 3},
  {10, UPB_SIZE(44, 88), 0, 0, 12, 3},
};

const upb_msglayout google_protobuf_DescriptorProto_msginit = {
  &google_protobuf_DescriptorProto_submsgs[0],
  &google_protobuf_DescriptorProto__fields[0],
  UPB_SIZE(48, 96), 10, false, 255,
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
  UPB_SIZE(16, 24), 3, false, 255,
};

static const upb_msglayout_field google_protobuf_DescriptorProto_ReservedRange__fields[2] = {
  {1, UPB_SIZE(4, 4), 1, 0, 5, 1},
  {2, UPB_SIZE(8, 8), 2, 0, 5, 1},
};

const upb_msglayout google_protobuf_DescriptorProto_ReservedRange_msginit = {
  NULL,
  &google_protobuf_DescriptorProto_ReservedRange__fields[0],
  UPB_SIZE(16, 16), 2, false, 255,
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
  UPB_SIZE(8, 8), 1, false, 255,
};

static const upb_msglayout *const google_protobuf_FieldDescriptorProto_submsgs[1] = {
  &google_protobuf_FieldOptions_msginit,
};

static const upb_msglayout_field google_protobuf_FieldDescriptorProto__fields[11] = {
  {1, UPB_SIZE(24, 24), 1, 0, 12, 1},
  {2, UPB_SIZE(32, 40), 2, 0, 12, 1},
  {3, UPB_SIZE(12, 12), 3, 0, 5, 1},
  {4, UPB_SIZE(4, 4), 4, 0, 14, 1},
  {5, UPB_SIZE(8, 8), 5, 0, 14, 1},
  {6, UPB_SIZE(40, 56), 6, 0, 12, 1},
  {7, UPB_SIZE(48, 72), 7, 0, 12, 1},
  {8, UPB_SIZE(64, 104), 8, 0, 11, 1},
  {9, UPB_SIZE(16, 16), 9, 0, 5, 1},
  {10, UPB_SIZE(56, 88), 10, 0, 12, 1},
  {17, UPB_SIZE(20, 20), 11, 0, 8, 1},
};

const upb_msglayout google_protobuf_FieldDescriptorProto_msginit = {
  &google_protobuf_FieldDescriptorProto_submsgs[0],
  &google_protobuf_FieldDescriptorProto__fields[0],
  UPB_SIZE(72, 112), 11, false, 255,
};

static const upb_msglayout *const google_protobuf_OneofDescriptorProto_submsgs[1] = {
  &google_protobuf_OneofOptions_msginit,
};

static const upb_msglayout_field google_protobuf_OneofDescriptorProto__fields[2] = {
  {1, UPB_SIZE(4, 8), 1, 0, 12, 1},
  {2, UPB_SIZE(12, 24), 2, 0, 11, 1},
};

const upb_msglayout google_protobuf_OneofDescriptorProto_msginit = {
  &google_protobuf_OneofDescriptorProto_submsgs[0],
  &google_protobuf_OneofDescriptorProto__fields[0],
  UPB_SIZE(16, 32), 2, false, 255,
};

static const upb_msglayout *const google_protobuf_EnumDescriptorProto_submsgs[3] = {
  &google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit,
  &google_protobuf_EnumOptions_msginit,
  &google_protobuf_EnumValueDescriptorProto_msginit,
};

static const upb_msglayout_field google_protobuf_EnumDescriptorProto__fields[5] = {
  {1, UPB_SIZE(4, 8), 1, 0, 12, 1},
  {2, UPB_SIZE(16, 32), 0, 2, 11, 3},
  {3, UPB_SIZE(12, 24), 2, 1, 11, 1},
  {4, UPB_SIZE(20, 40), 0, 0, 11, 3},
  {5, UPB_SIZE(24, 48), 0, 0, 12, 3},
};

const upb_msglayout google_protobuf_EnumDescriptorProto_msginit = {
  &google_protobuf_EnumDescriptorProto_submsgs[0],
  &google_protobuf_EnumDescriptorProto__fields[0],
  UPB_SIZE(32, 64), 5, false, 255,
};

static const upb_msglayout_field google_protobuf_EnumDescriptorProto_EnumReservedRange__fields[2] = {
  {1, UPB_SIZE(4, 4), 1, 0, 5, 1},
  {2, UPB_SIZE(8, 8), 2, 0, 5, 1},
};

const upb_msglayout google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit = {
  NULL,
  &google_protobuf_EnumDescriptorProto_EnumReservedRange__fields[0],
  UPB_SIZE(16, 16), 2, false, 255,
};

static const upb_msglayout *const google_protobuf_EnumValueDescriptorProto_submsgs[1] = {
  &google_protobuf_EnumValueOptions_msginit,
};

static const upb_msglayout_field google_protobuf_EnumValueDescriptorProto__fields[3] = {
  {1, UPB_SIZE(8, 8), 1, 0, 12, 1},
  {2, UPB_SIZE(4, 4), 2, 0, 5, 1},
  {3, UPB_SIZE(16, 24), 3, 0, 11, 1},
};

const upb_msglayout google_protobuf_EnumValueDescriptorProto_msginit = {
  &google_protobuf_EnumValueDescriptorProto_submsgs[0],
  &google_protobuf_EnumValueDescriptorProto__fields[0],
  UPB_SIZE(24, 32), 3, false, 255,
};

static const upb_msglayout *const google_protobuf_ServiceDescriptorProto_submsgs[2] = {
  &google_protobuf_MethodDescriptorProto_msginit,
  &google_protobuf_ServiceOptions_msginit,
};

static const upb_msglayout_field google_protobuf_ServiceDescriptorProto__fields[3] = {
  {1, UPB_SIZE(4, 8), 1, 0, 12, 1},
  {2, UPB_SIZE(16, 32), 0, 0, 11, 3},
  {3, UPB_SIZE(12, 24), 2, 1, 11, 1},
};

const upb_msglayout google_protobuf_ServiceDescriptorProto_msginit = {
  &google_protobuf_ServiceDescriptorProto_submsgs[0],
  &google_protobuf_ServiceDescriptorProto__fields[0],
  UPB_SIZE(24, 48), 3, false, 255,
};

static const upb_msglayout *const google_protobuf_MethodDescriptorProto_submsgs[1] = {
  &google_protobuf_MethodOptions_msginit,
};

static const upb_msglayout_field google_protobuf_MethodDescriptorProto__fields[6] = {
  {1, UPB_SIZE(4, 8), 1, 0, 12, 1},
  {2, UPB_SIZE(12, 24), 2, 0, 12, 1},
  {3, UPB_SIZE(20, 40), 3, 0, 12, 1},
  {4, UPB_SIZE(28, 56), 4, 0, 11, 1},
  {5, UPB_SIZE(1, 1), 5, 0, 8, 1},
  {6, UPB_SIZE(2, 2), 6, 0, 8, 1},
};

const upb_msglayout google_protobuf_MethodDescriptorProto_msginit = {
  &google_protobuf_MethodDescriptorProto_submsgs[0],
  &google_protobuf_MethodDescriptorProto__fields[0],
  UPB_SIZE(32, 64), 6, false, 255,
};

static const upb_msglayout *const google_protobuf_FileOptions_submsgs[1] = {
  &google_protobuf_UninterpretedOption_msginit,
};

static const upb_msglayout_field google_protobuf_FileOptions__fields[21] = {
  {1, UPB_SIZE(20, 24), 1, 0, 12, 1},
  {8, UPB_SIZE(28, 40), 2, 0, 12, 1},
  {9, UPB_SIZE(4, 4), 3, 0, 14, 1},
  {10, UPB_SIZE(8, 8), 4, 0, 8, 1},
  {11, UPB_SIZE(36, 56), 5, 0, 12, 1},
  {16, UPB_SIZE(9, 9), 6, 0, 8, 1},
  {17, UPB_SIZE(10, 10), 7, 0, 8, 1},
  {18, UPB_SIZE(11, 11), 8, 0, 8, 1},
  {20, UPB_SIZE(12, 12), 9, 0, 8, 1},
  {23, UPB_SIZE(13, 13), 10, 0, 8, 1},
  {27, UPB_SIZE(14, 14), 11, 0, 8, 1},
  {31, UPB_SIZE(15, 15), 12, 0, 8, 1},
  {36, UPB_SIZE(44, 72), 13, 0, 12, 1},
  {37, UPB_SIZE(52, 88), 14, 0, 12, 1},
  {39, UPB_SIZE(60, 104), 15, 0, 12, 1},
  {40, UPB_SIZE(68, 120), 16, 0, 12, 1},
  {41, UPB_SIZE(76, 136), 17, 0, 12, 1},
  {42, UPB_SIZE(16, 16), 18, 0, 8, 1},
  {44, UPB_SIZE(84, 152), 19, 0, 12, 1},
  {45, UPB_SIZE(92, 168), 20, 0, 12, 1},
  {999, UPB_SIZE(100, 184), 0, 0, 11, 3},
};

const upb_msglayout google_protobuf_FileOptions_msginit = {
  &google_protobuf_FileOptions_submsgs[0],
  &google_protobuf_FileOptions__fields[0],
  UPB_SIZE(104, 192), 21, false, 255,
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
  UPB_SIZE(16, 16), 5, false, 255,
};

static const upb_msglayout *const google_protobuf_FieldOptions_submsgs[1] = {
  &google_protobuf_UninterpretedOption_msginit,
};

static const upb_msglayout_field google_protobuf_FieldOptions__fields[7] = {
  {1, UPB_SIZE(4, 4), 1, 0, 14, 1},
  {2, UPB_SIZE(12, 12), 2, 0, 8, 1},
  {3, UPB_SIZE(13, 13), 3, 0, 8, 1},
  {5, UPB_SIZE(14, 14), 4, 0, 8, 1},
  {6, UPB_SIZE(8, 8), 5, 0, 14, 1},
  {10, UPB_SIZE(15, 15), 6, 0, 8, 1},
  {999, UPB_SIZE(16, 16), 0, 0, 11, 3},
};

const upb_msglayout google_protobuf_FieldOptions_msginit = {
  &google_protobuf_FieldOptions_submsgs[0],
  &google_protobuf_FieldOptions__fields[0],
  UPB_SIZE(24, 24), 7, false, 255,
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
  UPB_SIZE(8, 8), 1, false, 255,
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
  UPB_SIZE(8, 16), 3, false, 255,
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
  UPB_SIZE(8, 16), 2, false, 255,
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
  UPB_SIZE(8, 16), 2, false, 255,
};

static const upb_msglayout *const google_protobuf_MethodOptions_submsgs[1] = {
  &google_protobuf_UninterpretedOption_msginit,
};

static const upb_msglayout_field google_protobuf_MethodOptions__fields[3] = {
  {33, UPB_SIZE(8, 8), 1, 0, 8, 1},
  {34, UPB_SIZE(4, 4), 2, 0, 14, 1},
  {999, UPB_SIZE(12, 16), 0, 0, 11, 3},
};

const upb_msglayout google_protobuf_MethodOptions_msginit = {
  &google_protobuf_MethodOptions_submsgs[0],
  &google_protobuf_MethodOptions__fields[0],
  UPB_SIZE(16, 24), 3, false, 255,
};

static const upb_msglayout *const google_protobuf_UninterpretedOption_submsgs[1] = {
  &google_protobuf_UninterpretedOption_NamePart_msginit,
};

static const upb_msglayout_field google_protobuf_UninterpretedOption__fields[7] = {
  {2, UPB_SIZE(56, 80), 0, 0, 11, 3},
  {3, UPB_SIZE(32, 32), 1, 0, 12, 1},
  {4, UPB_SIZE(8, 8), 2, 0, 4, 1},
  {5, UPB_SIZE(16, 16), 3, 0, 3, 1},
  {6, UPB_SIZE(24, 24), 4, 0, 1, 1},
  {7, UPB_SIZE(40, 48), 5, 0, 12, 1},
  {8, UPB_SIZE(48, 64), 6, 0, 12, 1},
};

const upb_msglayout google_protobuf_UninterpretedOption_msginit = {
  &google_protobuf_UninterpretedOption_submsgs[0],
  &google_protobuf_UninterpretedOption__fields[0],
  UPB_SIZE(64, 96), 7, false, 255,
};

static const upb_msglayout_field google_protobuf_UninterpretedOption_NamePart__fields[2] = {
  {1, UPB_SIZE(4, 8), 1, 0, 12, 2},
  {2, UPB_SIZE(1, 1), 2, 0, 8, 2},
};

const upb_msglayout google_protobuf_UninterpretedOption_NamePart_msginit = {
  NULL,
  &google_protobuf_UninterpretedOption_NamePart__fields[0],
  UPB_SIZE(16, 32), 2, false, 255,
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
  UPB_SIZE(8, 8), 1, false, 255,
};

static const upb_msglayout_field google_protobuf_SourceCodeInfo_Location__fields[5] = {
  {1, UPB_SIZE(20, 40), 0, 0, 5, _UPB_LABEL_PACKED},
  {2, UPB_SIZE(24, 48), 0, 0, 5, _UPB_LABEL_PACKED},
  {3, UPB_SIZE(4, 8), 1, 0, 12, 1},
  {4, UPB_SIZE(12, 24), 2, 0, 12, 1},
  {6, UPB_SIZE(28, 56), 0, 0, 12, 3},
};

const upb_msglayout google_protobuf_SourceCodeInfo_Location_msginit = {
  NULL,
  &google_protobuf_SourceCodeInfo_Location__fields[0],
  UPB_SIZE(32, 64), 5, false, 255,
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
  UPB_SIZE(8, 8), 1, false, 255,
};

static const upb_msglayout_field google_protobuf_GeneratedCodeInfo_Annotation__fields[4] = {
  {1, UPB_SIZE(20, 32), 0, 0, 5, _UPB_LABEL_PACKED},
  {2, UPB_SIZE(12, 16), 1, 0, 12, 1},
  {3, UPB_SIZE(4, 4), 2, 0, 5, 1},
  {4, UPB_SIZE(8, 8), 3, 0, 5, 1},
};

const upb_msglayout google_protobuf_GeneratedCodeInfo_Annotation_msginit = {
  NULL,
  &google_protobuf_GeneratedCodeInfo_Annotation__fields[0],
  UPB_SIZE(24, 48), 4, false, 255,
};




#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>


/* Must be last. */

typedef struct {
  size_t len;
  char str[1];  /* Null-terminated string data follows. */
} str_t;

struct upb_fielddef {
  const upb_filedef *file;
  const upb_msgdef *msgdef;
  const char *full_name;
  const char *json_name;
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
  uint16_t index_;
  uint16_t layout_index;
  uint32_t selector_base;  /* Used to index into a upb::Handlers table. */
  bool is_extension_;
  bool lazy_;
  bool packed_;
  bool proto3_optional_;
  upb_descriptortype_t type_;
  upb_label_t label_;
};

struct upb_msgdef {
  const upb_msglayout *layout;
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
  int real_oneof_count;

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
  int field_count;
  bool synthetic;
  const upb_fielddef **fields;
  upb_strtable ntof;
  upb_inttable itof;
};

struct upb_filedef {
  const char *name;
  const char *package;
  const char *phpprefix;
  const char *phpnamespace;

  const upb_filedef **deps;
  const upb_msgdef *msgs;
  const upb_enumdef *enums;
  const upb_fielddef *exts;
  const upb_symtab *symtab;

  int dep_count;
  int msg_count;
  int enum_count;
  int ext_count;
  upb_syntax_t syntax;
};

struct upb_symtab {
  upb_arena *arena;
  upb_strtable syms;  /* full_name -> packed def ptr */
  upb_strtable files;  /* file_name -> upb_filedef* */
  size_t bytes_loaded;
};

/* Inside a symtab we store tagged pointers to specific def types. */
typedef enum {
  UPB_DEFTYPE_FIELD = 0,

  /* Only inside symtab table. */
  UPB_DEFTYPE_MSG = 1,
  UPB_DEFTYPE_ENUM = 2,

  /* Only inside message table. */
  UPB_DEFTYPE_ONEOF = 1,
  UPB_DEFTYPE_FIELD_JSONNAME = 2
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

static void upb_status_setoom(upb_status *status) {
  upb_status_seterrmsg(status, "out of memory");
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
  return (int)upb_strtable_count(&e->ntoi);
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
  return upb_strtable_iter_key(iter).data;
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

const char *upb_fielddef_jsonname(const upb_fielddef *f) {
  return f->json_name;
}

uint32_t upb_fielddef_selectorbase(const upb_fielddef *f) {
  return f->selector_base;
}

const upb_filedef *upb_fielddef_file(const upb_fielddef *f) {
  return f->file;
}

const upb_msgdef *upb_fielddef_containingtype(const upb_fielddef *f) {
  return f->msgdef;
}

const upb_oneofdef *upb_fielddef_containingoneof(const upb_fielddef *f) {
  return f->oneof;
}

const upb_oneofdef *upb_fielddef_realcontainingoneof(const upb_fielddef *f) {
  if (!f->oneof || upb_oneofdef_issynthetic(f->oneof)) return NULL;
  return f->oneof;
}

upb_msgval upb_fielddef_default(const upb_fielddef *f) {
  UPB_ASSERT(!upb_fielddef_issubmsg(f));
  upb_msgval ret;
  if (upb_fielddef_isstring(f)) {
    str_t *str = f->defaultval.str;
    if (str) {
      ret.str_val.data = str->str;
      ret.str_val.size = str->len;
    } else {
      ret.str_val.size = 0;
    }
  } else {
    memcpy(&ret, &f->defaultval, 8);
  }
  return ret;
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
  return (int32_t)f->defaultval.sint;
}

uint64_t upb_fielddef_defaultuint64(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_UINT64);
  return f->defaultval.uint;
}

uint32_t upb_fielddef_defaultuint32(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_UINT32);
  return (uint32_t)f->defaultval.uint;
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
  return upb_fielddef_type(f) == UPB_TYPE_MESSAGE ? f->sub.msgdef : NULL;
}

const upb_enumdef *upb_fielddef_enumsubdef(const upb_fielddef *f) {
  return upb_fielddef_type(f) == UPB_TYPE_ENUM ? f->sub.enumdef : NULL;
}

const upb_msglayout_field *upb_fielddef_layout(const upb_fielddef *f) {
  return &f->msgdef->layout->fields[f->layout_index];
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
  return upb_fielddef_issubmsg(f) || upb_fielddef_containingoneof(f) ||
         f->file->syntax == UPB_SYNTAX_PROTO2;
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
  return *o || *f;  /* False if this was a JSON name. */
}

const upb_fielddef *upb_msgdef_lookupjsonname(const upb_msgdef *m,
                                              const char *name, size_t len) {
  upb_value val;
  const upb_fielddef* f;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return NULL;
  }

  f = unpack_def(val, UPB_DEFTYPE_FIELD);
  if (!f) f = unpack_def(val, UPB_DEFTYPE_FIELD_JSONNAME);

  return f;
}

int upb_msgdef_numfields(const upb_msgdef *m) {
  return m->field_count;
}

int upb_msgdef_numoneofs(const upb_msgdef *m) {
  return m->oneof_count;
}

int upb_msgdef_numrealoneofs(const upb_msgdef *m) {
  return m->real_oneof_count;
}

int upb_msgdef_fieldcount(const upb_msgdef *m) {
  return m->field_count;
}

int upb_msgdef_oneofcount(const upb_msgdef *m) {
  return m->oneof_count;
}

int upb_msgdef_realoneofcount(const upb_msgdef *m) {
  return m->real_oneof_count;
}

const upb_msglayout *upb_msgdef_layout(const upb_msgdef *m) {
  return m->layout;
}

const upb_fielddef *upb_msgdef_field(const upb_msgdef *m, int i) {
  UPB_ASSERT(i >= 0 && i < m->field_count);
  return &m->fields[i];
}

const upb_oneofdef *upb_msgdef_oneof(const upb_msgdef *m, int i) {
  UPB_ASSERT(i >= 0 && i < m->oneof_count);
  return &m->oneofs[i];
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

bool upb_msgdef_iswrapper(const upb_msgdef *m) {
  upb_wellknowntype_t type = upb_msgdef_wellknowntype(m);
  return type >= UPB_WELLKNOWN_DOUBLEVALUE &&
         type <= UPB_WELLKNOWN_BOOLVALUE;
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

int upb_oneofdef_fieldcount(const upb_oneofdef *o) {
  return o->field_count;
}

const upb_fielddef *upb_oneofdef_field(const upb_oneofdef *o, int i) {
  UPB_ASSERT(i < o->field_count);
  return o->fields[i];
}

int upb_oneofdef_numfields(const upb_oneofdef *o) {
  return o->field_count;
}

uint32_t upb_oneofdef_index(const upb_oneofdef *o) {
  return o - o->parent->oneofs;
}

bool upb_oneofdef_issynthetic(const upb_oneofdef *o) {
  return o->synthetic;
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

const upb_symtab *upb_filedef_symtab(const upb_filedef *f) {
  return f->symtab;
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
  s->bytes_loaded = 0;
  alloc = upb_arena_alloc(s->arena);

  if (!upb_strtable_init2(&s->syms, UPB_CTYPE_CONSTPTR, 32, alloc) ||
      !upb_strtable_init2(&s->files, UPB_CTYPE_CONSTPTR, 4, alloc)) {
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

const upb_filedef *upb_symtab_lookupfile2(
    const upb_symtab *s, const char *name, size_t len) {
  upb_value v;
  return upb_strtable_lookup2(&s->files, name, len, &v) ?
      upb_value_getconstptr(v) : NULL;
}

int upb_symtab_filecount(const upb_symtab *s) {
  return (int)upb_strtable_count(&s->files);
}

/* Code to build defs from descriptor protos. *********************************/

/* There is a question of how much validation to do here.  It will be difficult
 * to perfectly match the amount of validation performed by proto2.  But since
 * this code is used to directly build defs from Ruby (for example) we do need
 * to validate important constraints like uniqueness of names and numbers. */

#define CHK_OOM(x) if (!(x)) { symtab_oomerr(ctx); }

typedef struct {
  upb_symtab *symtab;
  upb_filedef *file;              /* File we are building. */
  upb_arena *file_arena;          /* Allocate defs here. */
  upb_alloc *alloc;               /* Alloc of file_arena, for tables. */
  const upb_msglayout **layouts;  /* NULL if we should build layouts. */
  upb_status *status;             /* Record errors here. */
  jmp_buf err;                    /* longjmp() on error. */
} symtab_addctx;

UPB_NORETURN UPB_NOINLINE UPB_PRINTF(2, 3)
static void symtab_errf(symtab_addctx *ctx, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  upb_status_vseterrf(ctx->status, fmt, argp);
  va_end(argp);
  UPB_LONGJMP(ctx->err, 1);
}

UPB_NORETURN UPB_NOINLINE
static void symtab_oomerr(symtab_addctx *ctx) {
  upb_status_setoom(ctx->status);
  UPB_LONGJMP(ctx->err, 1);
}

void *symtab_alloc(symtab_addctx *ctx, size_t bytes) {
  void *ret = upb_arena_malloc(ctx->file_arena, bytes);
  if (!ret) symtab_oomerr(ctx);
  return ret;
}

static void check_ident(symtab_addctx *ctx, upb_strview name, bool full) {
  const char *str = name.data;
  size_t len = name.size;
  bool start = true;
  size_t i;
  for (i = 0; i < len; i++) {
    char c = str[i];
    if (c == '.') {
      if (start || !full) {
        symtab_errf(ctx, "invalid name: unexpected '.' (%.*s)", (int)len, str);
      }
      start = true;
    } else if (start) {
      if (!upb_isletter(c)) {
        symtab_errf(
            ctx,
            "invalid name: path components must start with a letter (%.*s)",
            (int)len, str);
      }
      start = false;
    } else {
      if (!upb_isalphanum(c)) {
        symtab_errf(ctx, "invalid name: non-alphanumeric character (%.*s)",
                    (int)len, str);
      }
    }
  }
  if (start) {
    symtab_errf(ctx, "invalid name: empty part (%.*s)", (int)len, str);
  }
}

static size_t div_round_up(size_t n, size_t d) {
  return (n + d - 1) / d;
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
    case UPB_TYPE_MESSAGE:
      return sizeof(void*);
    case UPB_TYPE_BYTES:
    case UPB_TYPE_STRING:
      return sizeof(upb_strview);
  }
  UPB_UNREACHABLE();
}

static uint8_t upb_msg_fielddefsize(const upb_fielddef *f) {
  if (upb_msgdef_mapentry(upb_fielddef_containingtype(f))) {
    upb_map_entry ent;
    UPB_ASSERT(sizeof(ent.k) == sizeof(ent.v));
    return sizeof(ent.k);
  } else if (upb_fielddef_isseq(f)) {
    return sizeof(void*);
  } else {
    return upb_msgval_sizeof(upb_fielddef_type(f));
  }
}

static uint32_t upb_msglayout_place(upb_msglayout *l, size_t size) {
  uint32_t ret;

  l->size = UPB_ALIGN_UP(l->size, size);
  ret = l->size;
  l->size += size;
  return ret;
}

static int field_number_cmp(const void *p1, const void *p2) {
  const upb_msglayout_field *f1 = p1;
  const upb_msglayout_field *f2 = p2;
  return f1->number - f2->number;
}

static void assign_layout_indices(const upb_msgdef *m, upb_msglayout_field *fields) {
  int i;
  int n = upb_msgdef_numfields(m);
  for (i = 0; i < n; i++) {
    upb_fielddef *f = (upb_fielddef*)upb_msgdef_itof(m, fields[i].number);
    UPB_ASSERT(f);
    f->layout_index = i;
  }
}

/* This function is the dynamic equivalent of message_layout.{cc,h} in upbc.
 * It computes a dynamic layout for all of the fields in |m|. */
static void make_layout(symtab_addctx *ctx, const upb_msgdef *m) {
  upb_msglayout *l = (upb_msglayout*)m->layout;
  upb_msg_field_iter it;
  upb_msg_oneof_iter oit;
  size_t hasbit;
  size_t submsg_count = m->submsg_field_count;
  const upb_msglayout **submsgs;
  upb_msglayout_field *fields;

  memset(l, 0, sizeof(*l) + sizeof(_upb_fasttable_entry));

  fields = symtab_alloc(ctx, upb_msgdef_numfields(m) * sizeof(*fields));
  submsgs = symtab_alloc(ctx, submsg_count * sizeof(*submsgs));

  l->field_count = upb_msgdef_numfields(m);
  l->fields = fields;
  l->submsgs = submsgs;
  l->table_mask = 0;

  /* TODO(haberman): initialize fast tables so that reflection-based parsing
   * can get the same speeds as linked-in types. */
  l->fasttable[0].field_parser = &fastdecode_generic;
  l->fasttable[0].field_data = 0;

  if (upb_msgdef_mapentry(m)) {
    /* TODO(haberman): refactor this method so this special case is more
     * elegant. */
    const upb_fielddef *key = upb_msgdef_itof(m, 1);
    const upb_fielddef *val = upb_msgdef_itof(m, 2);
    fields[0].number = 1;
    fields[1].number = 2;
    fields[0].label = UPB_LABEL_OPTIONAL;
    fields[1].label = UPB_LABEL_OPTIONAL;
    fields[0].presence = 0;
    fields[1].presence = 0;
    fields[0].descriptortype = upb_fielddef_descriptortype(key);
    fields[1].descriptortype = upb_fielddef_descriptortype(val);
    fields[0].offset = 0;
    fields[1].offset = sizeof(upb_strview);
    fields[1].submsg_index = 0;

    if (upb_fielddef_type(val) == UPB_TYPE_MESSAGE) {
      submsgs[0] = upb_fielddef_msgsubdef(val)->layout;
    }

    l->field_count = 2;
    l->size = 2 * sizeof(upb_strview);
    l->size = UPB_ALIGN_UP(l->size, 8);
    return;
  }

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
    upb_fielddef* f = upb_msg_iter_field(&it);
    upb_msglayout_field *field = &fields[upb_fielddef_index(f)];

    field->number = upb_fielddef_number(f);
    field->descriptortype = upb_fielddef_descriptortype(f);
    field->label = upb_fielddef_label(f);

    if (field->descriptortype == UPB_DTYPE_STRING &&
        f->file->syntax == UPB_SYNTAX_PROTO2) {
      /* See TableDescriptorType() in upbc/generator.cc for details and
       * rationale. */
      field->descriptortype = UPB_DTYPE_BYTES;
    }

    if (upb_fielddef_ismap(f)) {
      field->label = _UPB_LABEL_MAP;
    } else if (upb_fielddef_packed(f)) {
      field->label = _UPB_LABEL_PACKED;
    }

    if (upb_fielddef_issubmsg(f)) {
      const upb_msgdef *subm = upb_fielddef_msgsubdef(f);
      field->submsg_index = submsg_count++;
      submsgs[field->submsg_index] = subm->layout;
    }

    if (upb_fielddef_haspresence(f) && !upb_fielddef_realcontainingoneof(f)) {
      /* We don't use hasbit 0, so that 0 can indicate "no presence" in the
       * table. This wastes one hasbit, but we don't worry about it for now. */
      field->presence = ++hasbit;
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

    if (upb_fielddef_realcontainingoneof(f)) {
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

    if (upb_oneofdef_issynthetic(o)) continue;

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
  l->size = UPB_ALIGN_UP(l->size, 8);

  /* Sort fields by number. */
  qsort(fields, upb_msgdef_numfields(m), sizeof(*fields), field_number_cmp);
  assign_layout_indices(m, fields);
}

static void assign_msg_indices(symtab_addctx *ctx, upb_msgdef *m) {
  /* Sort fields.  upb internally relies on UPB_TYPE_MESSAGE fields having the
   * lowest indexes, but we do not publicly guarantee this. */
  upb_msg_field_iter j;
  int i;
  uint32_t selector;
  int n = upb_msgdef_numfields(m);
  upb_fielddef **fields;

  if (n == 0) {
    m->selector_count = UPB_STATIC_SELECTOR_COUNT;
    m->submsg_field_count = 0;
    return;
  }

  fields = upb_gmalloc(n * sizeof(*fields));

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

  upb_gfree(fields);
}

static char *strviewdup(symtab_addctx *ctx, upb_strview view) {
  return upb_strdup2(view.data, view.size, ctx->alloc);
}

static bool streql2(const char *a, size_t n, const char *b) {
  return n == strlen(b) && memcmp(a, b, n) == 0;
}

static bool streql_view(upb_strview view, const char *b) {
  return streql2(view.data, view.size, b);
}

static const char *makefullname(symtab_addctx *ctx, const char *prefix,
                                upb_strview name) {
  if (prefix) {
    /* ret = prefix + '.' + name; */
    size_t n = strlen(prefix);
    char *ret = symtab_alloc(ctx, n + name.size + 2);
    strcpy(ret, prefix);
    ret[n] = '.';
    memcpy(&ret[n + 1], name.data, name.size);
    ret[n + 1 + name.size] = '\0';
    return ret;
  } else {
    return strviewdup(ctx, name);
  }
}

static void finalize_oneofs(symtab_addctx *ctx, upb_msgdef *m) {
  int i;
  int synthetic_count = 0;
  upb_oneofdef *mutable_oneofs = (upb_oneofdef*)m->oneofs;

  for (i = 0; i < m->oneof_count; i++) {
    upb_oneofdef *o = &mutable_oneofs[i];

    if (o->synthetic && o->field_count != 1) {
      symtab_errf(ctx, "Synthetic oneofs must have one field, not %d: %s",
                  o->field_count, upb_oneofdef_name(o));
    }

    if (o->synthetic) {
      synthetic_count++;
    } else if (synthetic_count != 0) {
      symtab_errf(ctx, "Synthetic oneofs must be after all other oneofs: %s",
                  upb_oneofdef_name(o));
    }

    o->fields = symtab_alloc(ctx, sizeof(upb_fielddef *) * o->field_count);
    o->field_count = 0;
  }

  for (i = 0; i < m->field_count; i++) {
    const upb_fielddef *f = &m->fields[i];
    upb_oneofdef *o = (upb_oneofdef*)f->oneof;
    if (o) {
      o->fields[o->field_count++] = f;
    }
  }

  m->real_oneof_count = m->oneof_count - synthetic_count;
}

size_t getjsonname(const char *name, char *buf, size_t len) {
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

static char* makejsonname(symtab_addctx *ctx, const char* name) {
  size_t size = getjsonname(name, NULL, 0);
  char* json_name = symtab_alloc(ctx, size);
  getjsonname(name, json_name, size);
  return json_name;
}

static void symtab_add(symtab_addctx *ctx, const char *name, upb_value v) {
  if (upb_strtable_lookup(&ctx->symtab->syms, name, NULL)) {
    symtab_errf(ctx, "duplicate symbol '%s'", name);
  }
  upb_alloc *alloc = upb_arena_alloc(ctx->symtab->arena);
  size_t len = strlen(name);
  CHK_OOM(upb_strtable_insert3(&ctx->symtab->syms, name, len, v, alloc));
}

/* Given a symbol and the base symbol inside which it is defined, find the
 * symbol's definition in t. */
static const void *symtab_resolve(symtab_addctx *ctx, const upb_fielddef *f,
                                  const char *base, upb_strview sym,
                                  upb_deftype_t type) {
  const upb_strtable *t = &ctx->symtab->syms;
  if(sym.size == 0) goto notfound;
  if(sym.data[0] == '.') {
    /* Symbols starting with '.' are absolute, so we do a single lookup.
     * Slice to omit the leading '.' */
    upb_value v;
    if (!upb_strtable_lookup2(t, sym.data + 1, sym.size - 1, &v)) {
      goto notfound;
    }

    const void *ret = unpack_def(v, type);
    if (!ret) {
      symtab_errf(ctx, "type mismatch when resolving field %s, name %s",
                  f->full_name, sym.data);
    }
    return ret;
  } else {
    /* Remove components from base until we find an entry or run out.
     * TODO: This branch is totally broken, but currently not used. */
    (void)base;
    UPB_ASSERT(false);
    goto notfound;
  }

notfound:
  symtab_errf(ctx, "couldn't resolve name '%s'", sym.data);
}

static void create_oneofdef(
    symtab_addctx *ctx, upb_msgdef *m,
    const google_protobuf_OneofDescriptorProto *oneof_proto) {
  upb_oneofdef *o;
  upb_strview name = google_protobuf_OneofDescriptorProto_name(oneof_proto);
  upb_value v;

  o = (upb_oneofdef*)&m->oneofs[m->oneof_count++];
  o->parent = m;
  o->full_name = makefullname(ctx, m->full_name, name);
  o->field_count = 0;
  o->synthetic = false;

  v = pack_def(o, UPB_DEFTYPE_ONEOF);
  symtab_add(ctx, o->full_name, v);
  CHK_OOM(upb_strtable_insert3(&m->ntof, name.data, name.size, v, ctx->alloc));

  CHK_OOM(upb_inttable_init2(&o->itof, UPB_CTYPE_CONSTPTR, ctx->alloc));
  CHK_OOM(upb_strtable_init2(&o->ntof, UPB_CTYPE_CONSTPTR, 4, ctx->alloc));
}

static str_t *newstr(symtab_addctx *ctx, const char *data, size_t len) {
  str_t *ret = symtab_alloc(ctx, sizeof(*ret) + len);
  if (!ret) return NULL;
  ret->len = len;
  if (len) memcpy(ret->str, data, len);
  ret->str[len] = '\0';
  return ret;
}

static void parse_default(symtab_addctx *ctx, const char *str, size_t len,
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
        symtab_errf(ctx, "Default too long: %.*s", (int)len, str);
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
      if (val > INT32_MAX || val < INT32_MIN || errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.sint = val;
      break;
    }
    case UPB_TYPE_ENUM: {
      const upb_enumdef *e = f->sub.enumdef;
      int32_t val;
      if (!upb_enumdef_ntoi(e, str, len, &val)) {
        goto invalid;
      }
      f->defaultval.sint = val;
      break;
    }
    case UPB_TYPE_INT64: {
      /* XXX: Need to write our own strtoll, since it's not available in c89. */
      int64_t val = strtol(str, &end, 0);
      if (val > INT64_MAX || val < INT64_MIN || errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.sint = val;
      break;
    }
    case UPB_TYPE_UINT32: {
      unsigned long val = strtoul(str, &end, 0);
      if (val > UINT32_MAX || errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.uint = val;
      break;
    }
    case UPB_TYPE_UINT64: {
      /* XXX: Need to write our own strtoull, since it's not available in c89. */
      uint64_t val = strtoul(str, &end, 0);
      if (val > UINT64_MAX || errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.uint = val;
      break;
    }
    case UPB_TYPE_DOUBLE: {
      double val = strtod(str, &end);
      if (errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.dbl = val;
      break;
    }
    case UPB_TYPE_FLOAT: {
      /* XXX: Need to write our own strtof, since it's not available in c89. */
      float val = strtod(str, &end);
      if (errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.flt = val;
      break;
    }
    case UPB_TYPE_BOOL: {
      if (streql2(str, len, "false")) {
        f->defaultval.boolean = false;
      } else if (streql2(str, len, "true")) {
        f->defaultval.boolean = true;
      } else {
      }
      break;
    }
    case UPB_TYPE_STRING:
      f->defaultval.str = newstr(ctx, str, len);
      break;
    case UPB_TYPE_BYTES:
      /* XXX: need to interpret the C-escaped value. */
      f->defaultval.str = newstr(ctx, str, len);
      break;
    case UPB_TYPE_MESSAGE:
      /* Should not have a default value. */
      symtab_errf(ctx, "Message should not have a default (%s)",
                  upb_fielddef_fullname(f));
  }

  return;

invalid:
  symtab_errf(ctx, "Invalid default '%.*s' for field %s", (int)len, str,
              upb_fielddef_fullname(f));
}

static void set_default_default(symtab_addctx *ctx, upb_fielddef *f) {
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
      f->defaultval.str = newstr(ctx, NULL, 0);
      break;
    case UPB_TYPE_BOOL:
      f->defaultval.boolean = false;
      break;
    case UPB_TYPE_MESSAGE:
      break;
  }
}

static void create_fielddef(
    symtab_addctx *ctx, const char *prefix, upb_msgdef *m,
    const google_protobuf_FieldDescriptorProto *field_proto) {
  upb_alloc *alloc = ctx->alloc;
  upb_fielddef *f;
  const google_protobuf_FieldOptions *options;
  upb_strview name;
  const char *full_name;
  const char *json_name;
  const char *shortname;
  uint32_t field_number;

  if (!google_protobuf_FieldDescriptorProto_has_name(field_proto)) {
    symtab_errf(ctx, "field has no name (%s)", upb_msgdef_fullname(m));
  }

  name = google_protobuf_FieldDescriptorProto_name(field_proto);
  check_ident(ctx, name, false);
  full_name = makefullname(ctx, prefix, name);
  shortname = shortdefname(full_name);

  if (google_protobuf_FieldDescriptorProto_has_json_name(field_proto)) {
    json_name = strviewdup(
        ctx, google_protobuf_FieldDescriptorProto_json_name(field_proto));
  } else {
    json_name = makejsonname(ctx, shortname);
  }

  field_number = google_protobuf_FieldDescriptorProto_number(field_proto);

  if (field_number == 0 || field_number > UPB_MAX_FIELDNUMBER) {
    symtab_errf(ctx, "invalid field number (%u)", field_number);
  }

  if (m) {
    /* direct message field. */
    upb_value v, field_v, json_v;
    size_t json_size;

    f = (upb_fielddef*)&m->fields[m->field_count++];
    f->msgdef = m;
    f->is_extension_ = false;

    if (upb_strtable_lookup(&m->ntof, shortname, NULL)) {
      symtab_errf(ctx, "duplicate field name (%s)", shortname);
    }

    if (upb_strtable_lookup(&m->ntof, json_name, NULL)) {
      symtab_errf(ctx, "duplicate json_name (%s)", json_name);
    }

    if (upb_inttable_lookup(&m->itof, field_number, NULL)) {
      symtab_errf(ctx, "duplicate field number (%u)", field_number);
    }

    field_v = pack_def(f, UPB_DEFTYPE_FIELD);
    json_v = pack_def(f, UPB_DEFTYPE_FIELD_JSONNAME);
    v = upb_value_constptr(f);
    json_size = strlen(json_name);

    CHK_OOM(
        upb_strtable_insert3(&m->ntof, name.data, name.size, field_v, alloc));
    CHK_OOM(upb_inttable_insert2(&m->itof, field_number, v, alloc));

    if (strcmp(shortname, json_name) != 0) {
      upb_strtable_insert3(&m->ntof, json_name, json_size, json_v, alloc);
    }

    if (ctx->layouts) {
      const upb_msglayout_field *fields = m->layout->fields;
      int count = m->layout->field_count;
      bool found = false;
      int i;
      for (i = 0; i < count; i++) {
        if (fields[i].number == field_number) {
          f->layout_index = i;
          found = true;
          break;
        }
      }
      UPB_ASSERT(found);
    }
  } else {
    /* extension field. */
    f = (upb_fielddef*)&ctx->file->exts[ctx->file->ext_count++];
    f->is_extension_ = true;
    symtab_add(ctx, full_name, pack_def(f, UPB_DEFTYPE_FIELD));
  }

  f->full_name = full_name;
  f->json_name = json_name;
  f->file = ctx->file;
  f->type_ = (int)google_protobuf_FieldDescriptorProto_type(field_proto);
  f->label_ = (int)google_protobuf_FieldDescriptorProto_label(field_proto);
  f->number_ = field_number;
  f->oneof = NULL;
  f->proto3_optional_ =
      google_protobuf_FieldDescriptorProto_proto3_optional(field_proto);

  /* We can't resolve the subdef or (in the case of extensions) the containing
   * message yet, because it may not have been defined yet.  We stash a pointer
   * to the field_proto until later when we can properly resolve it. */
  f->sub.unresolved = field_proto;

  if (f->label_ == UPB_LABEL_REQUIRED && f->file->syntax == UPB_SYNTAX_PROTO3) {
    symtab_errf(ctx, "proto3 fields cannot be required (%s)", f->full_name);
  }

  if (google_protobuf_FieldDescriptorProto_has_oneof_index(field_proto)) {
    int oneof_index =
        google_protobuf_FieldDescriptorProto_oneof_index(field_proto);
    upb_oneofdef *oneof;
    upb_value v = upb_value_constptr(f);

    if (upb_fielddef_label(f) != UPB_LABEL_OPTIONAL) {
      symtab_errf(ctx, "fields in oneof must have OPTIONAL label (%s)",
                  f->full_name);
    }

    if (!m) {
      symtab_errf(ctx, "oneof_index provided for extension field (%s)",
                  f->full_name);
    }

    if (oneof_index >= m->oneof_count) {
      symtab_errf(ctx, "oneof_index out of range (%s)", f->full_name);
    }

    oneof = (upb_oneofdef*)&m->oneofs[oneof_index];
    f->oneof = oneof;

    oneof->field_count++;
    if (f->proto3_optional_) {
      oneof->synthetic = true;
    }
    CHK_OOM(upb_inttable_insert2(&oneof->itof, f->number_, v, alloc));
    CHK_OOM(upb_strtable_insert3(&oneof->ntof, name.data, name.size, v, alloc));
  } else {
    f->oneof = NULL;
    if (f->proto3_optional_) {
      symtab_errf(ctx, "field with proto3_optional was not in a oneof (%s)",
                  f->full_name);
    }
  }

  options = google_protobuf_FieldDescriptorProto_has_options(field_proto) ?
    google_protobuf_FieldDescriptorProto_options(field_proto) : NULL;

  if (options && google_protobuf_FieldOptions_has_packed(options)) {
    f->packed_ = google_protobuf_FieldOptions_packed(options);
  } else {
    /* Repeated fields default to packed for proto3 only. */
    f->packed_ = upb_fielddef_isprimitive(f) &&
        f->label_ == UPB_LABEL_REPEATED && f->file->syntax == UPB_SYNTAX_PROTO3;
  }

  if (options) {
    f->lazy_ = google_protobuf_FieldOptions_lazy(options);
  } else {
    f->lazy_ = false;
  }
}

static void create_enumdef(
    symtab_addctx *ctx, const char *prefix,
    const google_protobuf_EnumDescriptorProto *enum_proto) {
  upb_enumdef *e;
  const google_protobuf_EnumValueDescriptorProto *const *values;
  upb_strview name;
  size_t i, n;

  name = google_protobuf_EnumDescriptorProto_name(enum_proto);
  check_ident(ctx, name, false);

  e = (upb_enumdef*)&ctx->file->enums[ctx->file->enum_count++];
  e->full_name = makefullname(ctx, prefix, name);
  symtab_add(ctx, e->full_name, pack_def(e, UPB_DEFTYPE_ENUM));

  values = google_protobuf_EnumDescriptorProto_value(enum_proto, &n);
  CHK_OOM(upb_strtable_init2(&e->ntoi, UPB_CTYPE_INT32, n, ctx->alloc));
  CHK_OOM(upb_inttable_init2(&e->iton, UPB_CTYPE_CSTR, ctx->alloc));

  e->file = ctx->file;
  e->defaultval = 0;

  if (n == 0) {
    symtab_errf(ctx, "enums must contain at least one value (%s)",
                e->full_name);
  }

  for (i = 0; i < n; i++) {
    const google_protobuf_EnumValueDescriptorProto *value = values[i];
    upb_strview name = google_protobuf_EnumValueDescriptorProto_name(value);
    char *name2 = strviewdup(ctx, name);
    int32_t num = google_protobuf_EnumValueDescriptorProto_number(value);
    upb_value v = upb_value_int32(num);

    if (i == 0 && e->file->syntax == UPB_SYNTAX_PROTO3 && num != 0) {
      symtab_errf(ctx, "for proto3, the first enum value must be zero (%s)",
                  e->full_name);
    }

    if (upb_strtable_lookup(&e->ntoi, name2, NULL)) {
      symtab_errf(ctx, "duplicate enum label '%s'", name2);
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
}

static void create_msgdef(symtab_addctx *ctx, const char *prefix,
                          const google_protobuf_DescriptorProto *msg_proto) {
  upb_msgdef *m;
  const google_protobuf_MessageOptions *options;
  const google_protobuf_OneofDescriptorProto *const *oneofs;
  const google_protobuf_FieldDescriptorProto *const *fields;
  const google_protobuf_EnumDescriptorProto *const *enums;
  const google_protobuf_DescriptorProto *const *msgs;
  size_t i, n_oneof, n_field, n;
  upb_strview name;

  name = google_protobuf_DescriptorProto_name(msg_proto);
  check_ident(ctx, name, false);

  m = (upb_msgdef*)&ctx->file->msgs[ctx->file->msg_count++];
  m->full_name = makefullname(ctx, prefix, name);
  symtab_add(ctx, m->full_name, pack_def(m, UPB_DEFTYPE_MSG));

  oneofs = google_protobuf_DescriptorProto_oneof_decl(msg_proto, &n_oneof);
  fields = google_protobuf_DescriptorProto_field(msg_proto, &n_field);

  CHK_OOM(upb_inttable_init2(&m->itof, UPB_CTYPE_CONSTPTR, ctx->alloc));
  CHK_OOM(upb_strtable_init2(&m->ntof, UPB_CTYPE_CONSTPTR, n_oneof + n_field,
                             ctx->alloc));

  m->file = ctx->file;
  m->map_entry = false;

  options = google_protobuf_DescriptorProto_options(msg_proto);

  if (options) {
    m->map_entry = google_protobuf_MessageOptions_map_entry(options);
  }

  if (ctx->layouts) {
    m->layout = *ctx->layouts;
    ctx->layouts++;
  } else {
    /* Allocate now (to allow cross-linking), populate later. */
    m->layout = symtab_alloc(
        ctx, sizeof(*m->layout) + sizeof(_upb_fasttable_entry));
  }

  m->oneof_count = 0;
  m->oneofs = symtab_alloc(ctx, sizeof(*m->oneofs) * n_oneof);
  for (i = 0; i < n_oneof; i++) {
    create_oneofdef(ctx, m, oneofs[i]);
  }

  m->field_count = 0;
  m->fields = symtab_alloc(ctx, sizeof(*m->fields) * n_field);
  for (i = 0; i < n_field; i++) {
    create_fielddef(ctx, m->full_name, m, fields[i]);
  }

  assign_msg_indices(ctx, m);
  finalize_oneofs(ctx, m);
  assign_msg_wellknowntype(m);
  upb_inttable_compact2(&m->itof, ctx->alloc);

  /* This message is built.  Now build nested messages and enums. */

  enums = google_protobuf_DescriptorProto_enum_type(msg_proto, &n);
  for (i = 0; i < n; i++) {
    create_enumdef(ctx, m->full_name, enums[i]);
  }

  msgs = google_protobuf_DescriptorProto_nested_type(msg_proto, &n);
  for (i = 0; i < n; i++) {
    create_msgdef(ctx, m->full_name, msgs[i]);
  }
}

static void count_types_in_msg(const google_protobuf_DescriptorProto *msg_proto,
                               upb_filedef *file) {
  const google_protobuf_DescriptorProto *const *msgs;
  size_t i, n;

  file->msg_count++;

  msgs = google_protobuf_DescriptorProto_nested_type(msg_proto, &n);
  for (i = 0; i < n; i++) {
    count_types_in_msg(msgs[i], file);
  }

  google_protobuf_DescriptorProto_enum_type(msg_proto, &n);
  file->enum_count += n;

  google_protobuf_DescriptorProto_extension(msg_proto, &n);
  file->ext_count += n;
}

static void count_types_in_file(
    const google_protobuf_FileDescriptorProto *file_proto,
    upb_filedef *file) {
  const google_protobuf_DescriptorProto *const *msgs;
  size_t i, n;

  msgs = google_protobuf_FileDescriptorProto_message_type(file_proto, &n);
  for (i = 0; i < n; i++) {
    count_types_in_msg(msgs[i], file);
  }

  google_protobuf_FileDescriptorProto_enum_type(file_proto, &n);
  file->enum_count += n;

  google_protobuf_FileDescriptorProto_extension(file_proto, &n);
  file->ext_count += n;
}

static void resolve_fielddef(symtab_addctx *ctx, const char *prefix,
                             upb_fielddef *f) {
  upb_strview name;
  const google_protobuf_FieldDescriptorProto *field_proto = f->sub.unresolved;

  if (f->is_extension_) {
    if (!google_protobuf_FieldDescriptorProto_has_extendee(field_proto)) {
      symtab_errf(ctx, "extension for field '%s' had no extendee",
                  f->full_name);
    }

    name = google_protobuf_FieldDescriptorProto_extendee(field_proto);
    f->msgdef = symtab_resolve(ctx, f, prefix, name, UPB_DEFTYPE_MSG);
  }

  if ((upb_fielddef_issubmsg(f) || f->type_ == UPB_DESCRIPTOR_TYPE_ENUM) &&
      !google_protobuf_FieldDescriptorProto_has_type_name(field_proto)) {
    symtab_errf(ctx, "field '%s' is missing type name", f->full_name);
  }

  name = google_protobuf_FieldDescriptorProto_type_name(field_proto);

  if (upb_fielddef_issubmsg(f)) {
    f->sub.msgdef = symtab_resolve(ctx, f, prefix, name, UPB_DEFTYPE_MSG);
  } else if (f->type_ == UPB_DESCRIPTOR_TYPE_ENUM) {
    f->sub.enumdef = symtab_resolve(ctx, f, prefix, name, UPB_DEFTYPE_ENUM);
  }

  /* Have to delay resolving of the default value until now because of the enum
   * case, since enum defaults are specified with a label. */
  if (google_protobuf_FieldDescriptorProto_has_default_value(field_proto)) {
    upb_strview defaultval =
        google_protobuf_FieldDescriptorProto_default_value(field_proto);

    if (f->file->syntax == UPB_SYNTAX_PROTO3) {
      symtab_errf(ctx, "proto3 fields cannot have explicit defaults (%s)",
                  f->full_name);
    }

    if (upb_fielddef_issubmsg(f)) {
      symtab_errf(ctx, "message fields cannot have explicit defaults (%s)",
                  f->full_name);
    }

    parse_default(ctx, defaultval.data, defaultval.size, f);
  } else {
    set_default_default(ctx, f);
  }
}

static void build_filedef(
    symtab_addctx *ctx, upb_filedef *file,
    const google_protobuf_FileDescriptorProto *file_proto) {
  const google_protobuf_FileOptions *file_options_proto;
  const google_protobuf_DescriptorProto *const *msgs;
  const google_protobuf_EnumDescriptorProto *const *enums;
  const google_protobuf_FieldDescriptorProto *const *exts;
  const upb_strview* strs;
  size_t i, n;

  count_types_in_file(file_proto, file);

  file->msgs = symtab_alloc(ctx, sizeof(*file->msgs) * file->msg_count);
  file->enums = symtab_alloc(ctx, sizeof(*file->enums) * file->enum_count);
  file->exts = symtab_alloc(ctx, sizeof(*file->exts) * file->ext_count);

  /* We increment these as defs are added. */
  file->msg_count = 0;
  file->enum_count = 0;
  file->ext_count = 0;

  if (!google_protobuf_FileDescriptorProto_has_name(file_proto)) {
    symtab_errf(ctx, "File has no name");
  }

  file->name =
      strviewdup(ctx, google_protobuf_FileDescriptorProto_name(file_proto));
  file->phpprefix = NULL;
  file->phpnamespace = NULL;

  if (google_protobuf_FileDescriptorProto_has_package(file_proto)) {
    upb_strview package =
        google_protobuf_FileDescriptorProto_package(file_proto);
    check_ident(ctx, package, true);
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
      symtab_errf(ctx, "Invalid syntax '" UPB_STRVIEW_FORMAT "'",
                  UPB_STRVIEW_ARGS(syntax));
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
  file->deps = symtab_alloc(ctx, sizeof(*file->deps) * n);

  for (i = 0; i < n; i++) {
    upb_strview dep_name = strs[i];
    upb_value v;
    if (!upb_strtable_lookup2(&ctx->symtab->files, dep_name.data,
                              dep_name.size, &v)) {
      symtab_errf(ctx,
                  "Depends on file '" UPB_STRVIEW_FORMAT
                  "', but it has not been loaded",
                  UPB_STRVIEW_ARGS(dep_name));
    }
    file->deps[i] = upb_value_getconstptr(v);
  }

  /* Create messages. */
  msgs = google_protobuf_FileDescriptorProto_message_type(file_proto, &n);
  for (i = 0; i < n; i++) {
    create_msgdef(ctx, file->package, msgs[i]);
  }

  /* Create enums. */
  enums = google_protobuf_FileDescriptorProto_enum_type(file_proto, &n);
  for (i = 0; i < n; i++) {
    create_enumdef(ctx, file->package, enums[i]);
  }

  /* Create extensions. */
  exts = google_protobuf_FileDescriptorProto_extension(file_proto, &n);
  file->exts = symtab_alloc(ctx, sizeof(*file->exts) * n);
  for (i = 0; i < n; i++) {
    create_fielddef(ctx, file->package, NULL, exts[i]);
  }

  /* Now that all names are in the table, build layouts and resolve refs. */
  for (i = 0; i < (size_t)file->ext_count; i++) {
    resolve_fielddef(ctx, file->package, (upb_fielddef*)&file->exts[i]);
  }

  for (i = 0; i < (size_t)file->msg_count; i++) {
    const upb_msgdef *m = &file->msgs[i];
    int j;
    for (j = 0; j < m->field_count; j++) {
      resolve_fielddef(ctx, m->full_name, (upb_fielddef*)&m->fields[j]);
    }
  }

  if (!ctx->layouts) {
    for (i = 0; i < (size_t)file->msg_count; i++) {
      const upb_msgdef *m = &file->msgs[i];
      make_layout(ctx, m);
    }
  }
}

static void remove_filedef(upb_symtab *s, upb_filedef *file) {
  upb_alloc *alloc = upb_arena_alloc(s->arena);
  int i;
  for (i = 0; i < file->msg_count; i++) {
    const char *name = file->msgs[i].full_name;
    upb_strtable_remove3(&s->syms, name, strlen(name), NULL, alloc);
  }
  for (i = 0; i < file->enum_count; i++) {
    const char *name = file->enums[i].full_name;
    upb_strtable_remove3(&s->syms, name, strlen(name), NULL, alloc);
  }
  for (i = 0; i < file->ext_count; i++) {
    const char *name = file->exts[i].full_name;
    upb_strtable_remove3(&s->syms, name, strlen(name), NULL, alloc);
  }
}

static const upb_filedef *_upb_symtab_addfile(
    upb_symtab *s, const google_protobuf_FileDescriptorProto *file_proto,
    const upb_msglayout **layouts, upb_status *status) {
  upb_arena *file_arena = upb_arena_new();
  upb_filedef *file;
  symtab_addctx ctx;

  if (!file_arena) return NULL;

  file = upb_arena_malloc(file_arena, sizeof(*file));
  if (!file) goto done;

  ctx.file = file;
  ctx.symtab = s;
  ctx.file_arena = file_arena;
  ctx.alloc = upb_arena_alloc(file_arena);
  ctx.layouts = layouts;
  ctx.status = status;

  file->msg_count = 0;
  file->enum_count = 0;
  file->ext_count = 0;
  file->symtab = s;

  if (UPB_UNLIKELY(UPB_SETJMP(ctx.err))) {
    UPB_ASSERT(!upb_ok(status));
    remove_filedef(s, file);
    file = NULL;
  } else {
    build_filedef(&ctx, file, file_proto);
    upb_strtable_insert3(&s->files, file->name, strlen(file->name),
                         upb_value_constptr(file), ctx.alloc);
    UPB_ASSERT(upb_ok(status));
    upb_arena_fuse(s->arena, file_arena);
  }

done:
  upb_arena_free(file_arena);
  return file;
}

const upb_filedef *upb_symtab_addfile(
    upb_symtab *s, const google_protobuf_FileDescriptorProto *file_proto,
    upb_status *status) {
  return _upb_symtab_addfile(s, file_proto, NULL, status);
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

  file = google_protobuf_FileDescriptorProto_parse_ex(
      init->descriptor.data, init->descriptor.size, arena, UPB_DECODE_ALIAS);
  s->bytes_loaded += init->descriptor.size;

  if (!file) {
    upb_status_seterrf(
        &status,
        "Failed to parse compiled-in descriptor for file '%s'. This should "
        "never happen.",
        init->filename);
    goto err;
  }

  if (!_upb_symtab_addfile(s, file, init->layouts, &status)) goto err;

  upb_arena_free(arena);
  return true;

err:
  fprintf(stderr, "Error loading compiled-in descriptor: %s\n",
          upb_status_errmsg(&status));
  upb_arena_free(arena);
  return false;
}

size_t _upb_symtab_bytesloaded(const upb_symtab *s) {
  return s->bytes_loaded;
}

upb_arena *_upb_symtab_arena(const upb_symtab *s) {
  return s->arena;
}

#undef CHK_OOM


#include <string.h>


static size_t get_field_size(const upb_msglayout_field *f) {
  static unsigned char sizes[] = {
    0,/* 0 */
    8, /* UPB_DESCRIPTOR_TYPE_DOUBLE */
    4, /* UPB_DESCRIPTOR_TYPE_FLOAT */
    8, /* UPB_DESCRIPTOR_TYPE_INT64 */
    8, /* UPB_DESCRIPTOR_TYPE_UINT64 */
    4, /* UPB_DESCRIPTOR_TYPE_INT32 */
    8, /* UPB_DESCRIPTOR_TYPE_FIXED64 */
    4, /* UPB_DESCRIPTOR_TYPE_FIXED32 */
    1, /* UPB_DESCRIPTOR_TYPE_BOOL */
    sizeof(upb_strview), /* UPB_DESCRIPTOR_TYPE_STRING */
    sizeof(void*), /* UPB_DESCRIPTOR_TYPE_GROUP */
    sizeof(void*), /* UPB_DESCRIPTOR_TYPE_MESSAGE */
    sizeof(upb_strview), /* UPB_DESCRIPTOR_TYPE_BYTES */
    4, /* UPB_DESCRIPTOR_TYPE_UINT32 */
    4, /* UPB_DESCRIPTOR_TYPE_ENUM */
    4, /* UPB_DESCRIPTOR_TYPE_SFIXED32 */
    8, /* UPB_DESCRIPTOR_TYPE_SFIXED64 */
    4, /* UPB_DESCRIPTOR_TYPE_SINT32 */
    8, /* UPB_DESCRIPTOR_TYPE_SINT64 */
  };
  return _upb_repeated_or_map(f) ? sizeof(void *) : sizes[f->descriptortype];
}

/* Strings/bytes are special-cased in maps. */
static char _upb_fieldtype_to_mapsize[12] = {
  0,
  1,  /* UPB_TYPE_BOOL */
  4,  /* UPB_TYPE_FLOAT */
  4,  /* UPB_TYPE_INT32 */
  4,  /* UPB_TYPE_UINT32 */
  4,  /* UPB_TYPE_ENUM */
  sizeof(void*),  /* UPB_TYPE_MESSAGE */
  8,  /* UPB_TYPE_DOUBLE */
  8,  /* UPB_TYPE_INT64 */
  8,  /* UPB_TYPE_UINT64 */
  0,  /* UPB_TYPE_STRING */
  0,  /* UPB_TYPE_BYTES */
};

static const char _upb_fieldtype_to_sizelg2[12] = {
  0,
  0,  /* UPB_TYPE_BOOL */
  2,  /* UPB_TYPE_FLOAT */
  2,  /* UPB_TYPE_INT32 */
  2,  /* UPB_TYPE_UINT32 */
  2,  /* UPB_TYPE_ENUM */
  UPB_SIZE(2, 3),  /* UPB_TYPE_MESSAGE */
  3,  /* UPB_TYPE_DOUBLE */
  3,  /* UPB_TYPE_INT64 */
  3,  /* UPB_TYPE_UINT64 */
  UPB_SIZE(3, 4),  /* UPB_TYPE_STRING */
  UPB_SIZE(3, 4),  /* UPB_TYPE_BYTES */
};

/** upb_msg *******************************************************************/

upb_msg *upb_msg_new(const upb_msgdef *m, upb_arena *a) {
  return _upb_msg_new(upb_msgdef_layout(m), a);
}

static bool in_oneof(const upb_msglayout_field *field) {
  return field->presence < 0;
}

static upb_msgval _upb_msg_getraw(const upb_msg *msg, const upb_fielddef *f) {
  const upb_msglayout_field *field = upb_fielddef_layout(f);
  const char *mem = UPB_PTR_AT(msg, field->offset, char);
  upb_msgval val = {0};
  memcpy(&val, mem, get_field_size(field));
  return val;
}

bool upb_msg_has(const upb_msg *msg, const upb_fielddef *f) {
  const upb_msglayout_field *field = upb_fielddef_layout(f);
  if (in_oneof(field)) {
    return _upb_getoneofcase_field(msg, field) == field->number;
  } else if (field->presence > 0) {
    return _upb_hasbit_field(msg, field);
  } else {
    UPB_ASSERT(field->descriptortype == UPB_DESCRIPTOR_TYPE_MESSAGE ||
               field->descriptortype == UPB_DESCRIPTOR_TYPE_GROUP);
    return _upb_msg_getraw(msg, f).msg_val != NULL;
  }
}

const upb_fielddef *upb_msg_whichoneof(const upb_msg *msg,
                                       const upb_oneofdef *o) {
  const upb_fielddef *f = upb_oneofdef_field(o, 0);
  if (upb_oneofdef_issynthetic(o)) {
    UPB_ASSERT(upb_oneofdef_fieldcount(o) == 1);
    return upb_msg_has(msg, f) ? f : NULL;
  } else {
    const upb_msglayout_field *field = upb_fielddef_layout(f);
    uint32_t oneof_case = _upb_getoneofcase_field(msg, field);
    f = oneof_case ? upb_oneofdef_itof(o, oneof_case) : NULL;
    UPB_ASSERT((f != NULL) == (oneof_case != 0));
    return f;
  }
}

upb_msgval upb_msg_get(const upb_msg *msg, const upb_fielddef *f) {
  if (!upb_fielddef_haspresence(f) || upb_msg_has(msg, f)) {
    return _upb_msg_getraw(msg, f);
  } else {
    /* TODO(haberman): change upb_fielddef to not require this switch(). */
    upb_msgval val = {0};
    switch (upb_fielddef_type(f)) {
      case UPB_TYPE_INT32:
      case UPB_TYPE_ENUM:
        val.int32_val = upb_fielddef_defaultint32(f);
        break;
      case UPB_TYPE_INT64:
        val.int64_val = upb_fielddef_defaultint64(f);
        break;
      case UPB_TYPE_UINT32:
        val.uint32_val = upb_fielddef_defaultuint32(f);
        break;
      case UPB_TYPE_UINT64:
        val.uint64_val = upb_fielddef_defaultuint64(f);
        break;
      case UPB_TYPE_FLOAT:
        val.float_val = upb_fielddef_defaultfloat(f);
        break;
      case UPB_TYPE_DOUBLE:
        val.double_val = upb_fielddef_defaultdouble(f);
        break;
      case UPB_TYPE_BOOL:
        val.bool_val = upb_fielddef_defaultbool(f);
        break;
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES:
        val.str_val.data = upb_fielddef_defaultstr(f, &val.str_val.size);
        break;
      case UPB_TYPE_MESSAGE:
        val.msg_val = NULL;
        break;
    }
    return val;
  }
}

upb_mutmsgval upb_msg_mutable(upb_msg *msg, const upb_fielddef *f,
                              upb_arena *a) {
  const upb_msglayout_field *field = upb_fielddef_layout(f);
  upb_mutmsgval ret;
  char *mem = UPB_PTR_AT(msg, field->offset, char);
  bool wrong_oneof =
      in_oneof(field) && _upb_getoneofcase_field(msg, field) != field->number;

  memcpy(&ret, mem, sizeof(void*));

  if (a && (!ret.msg || wrong_oneof)) {
    if (upb_fielddef_ismap(f)) {
      const upb_msgdef *entry = upb_fielddef_msgsubdef(f);
      const upb_fielddef *key = upb_msgdef_itof(entry, UPB_MAPENTRY_KEY);
      const upb_fielddef *value = upb_msgdef_itof(entry, UPB_MAPENTRY_VALUE);
      ret.map = upb_map_new(a, upb_fielddef_type(key), upb_fielddef_type(value));
    } else if (upb_fielddef_isseq(f)) {
      ret.array = upb_array_new(a, upb_fielddef_type(f));
    } else {
      UPB_ASSERT(upb_fielddef_issubmsg(f));
      ret.msg = upb_msg_new(upb_fielddef_msgsubdef(f), a);
    }

    memcpy(mem, &ret, sizeof(void*));

    if (wrong_oneof) {
      *_upb_oneofcase_field(msg, field) = field->number;
    } else if (field->presence > 0) {
      _upb_sethas_field(msg, field);
    }
  }
  return ret;
}

void upb_msg_set(upb_msg *msg, const upb_fielddef *f, upb_msgval val,
                 upb_arena *a) {
  const upb_msglayout_field *field = upb_fielddef_layout(f);
  char *mem = UPB_PTR_AT(msg, field->offset, char);
  UPB_UNUSED(a);  /* We reserve the right to make set insert into a map. */
  memcpy(mem, &val, get_field_size(field));
  if (field->presence > 0) {
    _upb_sethas_field(msg, field);
  } else if (in_oneof(field)) {
    *_upb_oneofcase_field(msg, field) = field->number;
  }
}

void upb_msg_clearfield(upb_msg *msg, const upb_fielddef *f) {
  const upb_msglayout_field *field = upb_fielddef_layout(f);
  char *mem = UPB_PTR_AT(msg, field->offset, char);

  if (field->presence > 0) {
    _upb_clearhas_field(msg, field);
  } else if (in_oneof(field)) {
    uint32_t *oneof_case = _upb_oneofcase_field(msg, field);
    if (*oneof_case != field->number) return;
    *oneof_case = 0;
  }

  memset(mem, 0, get_field_size(field));
}

void upb_msg_clear(upb_msg *msg, const upb_msgdef *m) {
  _upb_msg_clear(msg, upb_msgdef_layout(m));
}

bool upb_msg_next(const upb_msg *msg, const upb_msgdef *m,
                  const upb_symtab *ext_pool, const upb_fielddef **out_f,
                  upb_msgval *out_val, size_t *iter) {
  int i = *iter;
  int n = upb_msgdef_fieldcount(m);
  const upb_msgval zero = {0};
  UPB_UNUSED(ext_pool);
  while (++i < n) {
    const upb_fielddef *f = upb_msgdef_field(m, i);
    upb_msgval val = _upb_msg_getraw(msg, f);

    /* Skip field if unset or empty. */
    if (upb_fielddef_haspresence(f)) {
      if (!upb_msg_has(msg, f)) continue;
    } else {
      upb_msgval test = val;
      if (upb_fielddef_isstring(f) && !upb_fielddef_isseq(f)) {
        /* Clear string pointer, only size matters (ptr could be non-NULL). */
        test.str_val.data = NULL;
      }
      /* Continue if NULL or 0. */
      if (memcmp(&test, &zero, sizeof(test)) == 0) continue;

      /* Continue on empty array or map. */
      if (upb_fielddef_ismap(f)) {
        if (upb_map_size(test.map_val) == 0) continue;
      } else if (upb_fielddef_isseq(f)) {
        if (upb_array_size(test.array_val) == 0) continue;
      }
    }

    *out_val = val;
    *out_f = f;
    *iter = i;
    return true;
  }
  *iter = i;
  return false;
}

bool _upb_msg_discardunknown(upb_msg *msg, const upb_msgdef *m, int depth) {
  size_t iter = UPB_MSG_BEGIN;
  const upb_fielddef *f;
  upb_msgval val;
  bool ret = true;

  if (--depth == 0) return false;

  _upb_msg_discardunknown_shallow(msg);

  while (upb_msg_next(msg, m, NULL /*ext_pool*/, &f, &val, &iter)) {
    const upb_msgdef *subm = upb_fielddef_msgsubdef(f);
    if (!subm) continue;
    if (upb_fielddef_ismap(f)) {
      const upb_fielddef *val_f = upb_msgdef_itof(subm, 2);
      const upb_msgdef *val_m = upb_fielddef_msgsubdef(val_f);
      upb_map *map = (upb_map*)val.map_val;
      size_t iter = UPB_MAP_BEGIN;

      if (!val_m) continue;

      while (upb_mapiter_next(map, &iter)) {
        upb_msgval map_val = upb_mapiter_value(map, iter);
        if (!_upb_msg_discardunknown((upb_msg*)map_val.msg_val, val_m, depth)) {
          ret = false;
        }
      }
    } else if (upb_fielddef_isseq(f)) {
      const upb_array *arr = val.array_val;
      size_t i, n = upb_array_size(arr);
      for (i = 0; i < n; i++) {
        upb_msgval elem = upb_array_get(arr, i);
        if (!_upb_msg_discardunknown((upb_msg*)elem.msg_val, subm, depth)) {
          ret = false;
        }
      }
    } else {
      if (!_upb_msg_discardunknown((upb_msg*)val.msg_val, subm, depth)) {
        ret = false;
      }
    }
  }

  return ret;
}

bool upb_msg_discardunknown(upb_msg *msg, const upb_msgdef *m, int maxdepth) {
  return _upb_msg_discardunknown(msg, m, maxdepth);
}

/** upb_array *****************************************************************/

upb_array *upb_array_new(upb_arena *a, upb_fieldtype_t type) {
  return _upb_array_new(a, 4, _upb_fieldtype_to_sizelg2[type]);
}

size_t upb_array_size(const upb_array *arr) {
  return arr->len;
}

upb_msgval upb_array_get(const upb_array *arr, size_t i) {
  upb_msgval ret;
  const char* data = _upb_array_constptr(arr);
  int lg2 = arr->data & 7;
  UPB_ASSERT(i < arr->len);
  memcpy(&ret, data + (i << lg2), 1 << lg2);
  return ret;
}

void upb_array_set(upb_array *arr, size_t i, upb_msgval val) {
  char* data = _upb_array_ptr(arr);
  int lg2 = arr->data & 7;
  UPB_ASSERT(i < arr->len);
  memcpy(data + (i << lg2), &val, 1 << lg2);
}

bool upb_array_append(upb_array *arr, upb_msgval val, upb_arena *arena) {
  if (!upb_array_resize(arr, arr->len + 1, arena)) {
    return false;
  }
  upb_array_set(arr, arr->len - 1, val);
  return true;
}

bool upb_array_resize(upb_array *arr, size_t size, upb_arena *arena) {
  return _upb_array_resize(arr, size, arena);
}

/** upb_map *******************************************************************/

upb_map *upb_map_new(upb_arena *a, upb_fieldtype_t key_type,
                     upb_fieldtype_t value_type) {
  return _upb_map_new(a, _upb_fieldtype_to_mapsize[key_type],
                      _upb_fieldtype_to_mapsize[value_type]);
}

size_t upb_map_size(const upb_map *map) {
  return _upb_map_size(map);
}

bool upb_map_get(const upb_map *map, upb_msgval key, upb_msgval *val) {
  return _upb_map_get(map, &key, map->key_size, val, map->val_size);
}

void upb_map_clear(upb_map *map) {
  _upb_map_clear(map);
}

bool upb_map_set(upb_map *map, upb_msgval key, upb_msgval val,
                 upb_arena *arena) {
  return _upb_map_set(map, &key, map->key_size, &val, map->val_size, arena);
}

bool upb_map_delete(upb_map *map, upb_msgval key) {
  return _upb_map_delete(map, &key, map->key_size);
}

bool upb_mapiter_next(const upb_map *map, size_t *iter) {
  return _upb_map_next(map, iter);
}

bool upb_mapiter_done(const upb_map *map, size_t iter) {
  upb_strtable_iter i;
  UPB_ASSERT(iter != UPB_MAP_BEGIN);
  i.t = &map->table;
  i.index = iter;
  return upb_strtable_done(&i);
}

/* Returns the key and value for this entry of the map. */
upb_msgval upb_mapiter_key(const upb_map *map, size_t iter) {
  upb_strtable_iter i;
  upb_msgval ret;
  i.t = &map->table;
  i.index = iter;
  _upb_map_fromkey(upb_strtable_iter_key(&i), &ret, map->key_size);
  return ret;
}

upb_msgval upb_mapiter_value(const upb_map *map, size_t iter) {
  upb_strtable_iter i;
  upb_msgval ret;
  i.t = &map->table;
  i.index = iter;
  _upb_map_fromvalue(upb_strtable_iter_value(&i), &ret, map->val_size);
  return ret;
}

/* void upb_mapiter_setvalue(upb_map *map, size_t iter, upb_msgval value); */


#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>


/* Special header, must be included last. */

typedef struct {
  const char *ptr, *end;
  upb_arena *arena;  /* TODO: should we have a tmp arena for tmp data? */
  const upb_symtab *any_pool;
  int depth;
  upb_status *status;
  jmp_buf err;
  int line;
  const char *line_begin;
  bool is_first;
  int options;
  const upb_fielddef *debug_field;
} jsondec;

enum { JD_OBJECT, JD_ARRAY, JD_STRING, JD_NUMBER, JD_TRUE, JD_FALSE, JD_NULL };

/* Forward declarations of mutually-recursive functions. */
static void jsondec_wellknown(jsondec *d, upb_msg *msg, const upb_msgdef *m);
static upb_msgval jsondec_value(jsondec *d, const upb_fielddef *f);
static void jsondec_wellknownvalue(jsondec *d, upb_msg *msg,
                                   const upb_msgdef *m);
static void jsondec_object(jsondec *d, upb_msg *msg, const upb_msgdef *m);

static bool jsondec_streql(upb_strview str, const char *lit) {
  return str.size == strlen(lit) && memcmp(str.data, lit, str.size) == 0;
}

static bool jsondec_isnullvalue(const upb_fielddef *f) {
  return upb_fielddef_type(f) == UPB_TYPE_ENUM &&
         strcmp(upb_enumdef_fullname(upb_fielddef_enumsubdef(f)),
                "google.protobuf.NullValue") == 0;
}

static bool jsondec_isvalue(const upb_fielddef *f) {
  return (upb_fielddef_type(f) == UPB_TYPE_MESSAGE &&
          upb_msgdef_wellknowntype(upb_fielddef_msgsubdef(f)) ==
              UPB_WELLKNOWN_VALUE) ||
         jsondec_isnullvalue(f);
}

UPB_NORETURN static void jsondec_err(jsondec *d, const char *msg) {
  upb_status_seterrf(d->status, "Error parsing JSON @%d:%d: %s", d->line,
                     (int)(d->ptr - d->line_begin), msg);
  UPB_LONGJMP(d->err, 1);
}

UPB_PRINTF(2, 3)
UPB_NORETURN static void jsondec_errf(jsondec *d, const char *fmt, ...) {
  va_list argp;
  upb_status_seterrf(d->status, "Error parsing JSON @%d:%d: ", d->line,
                     (int)(d->ptr - d->line_begin));
  va_start(argp, fmt);
  upb_status_vappenderrf(d->status, fmt, argp);
  va_end(argp);
  UPB_LONGJMP(d->err, 1);
}

static void jsondec_skipws(jsondec *d) {
  while (d->ptr != d->end) {
    switch (*d->ptr) {
      case '\n':
        d->line++;
        d->line_begin = d->ptr;
        /* Fallthrough. */
      case '\r':
      case '\t':
      case ' ':
        d->ptr++;
        break;
      default:
        return;
    }
  }
  jsondec_err(d, "Unexpected EOF");
}

static bool jsondec_tryparsech(jsondec *d, char ch) {
  if (d->ptr == d->end || *d->ptr != ch) return false;
  d->ptr++;
  return true;
}

static void jsondec_parselit(jsondec *d, const char *lit) {
  size_t avail = d->end - d->ptr;
  size_t len = strlen(lit);
  if (avail < len || memcmp(d->ptr, lit, len) != 0) {
    jsondec_errf(d, "Expected: '%s'", lit);
  }
  d->ptr += len;
}

static void jsondec_wsch(jsondec *d, char ch) {
  jsondec_skipws(d);
  if (!jsondec_tryparsech(d, ch)) {
    jsondec_errf(d, "Expected: '%c'", ch);
  }
}

static void jsondec_true(jsondec *d) { jsondec_parselit(d, "true"); }
static void jsondec_false(jsondec *d) { jsondec_parselit(d, "false"); }
static void jsondec_null(jsondec *d) { jsondec_parselit(d, "null"); }

static void jsondec_entrysep(jsondec *d) {
  jsondec_skipws(d);
  jsondec_parselit(d, ":");
}

static int jsondec_rawpeek(jsondec *d) {
  switch (*d->ptr) {
    case '{':
      return JD_OBJECT;
    case '[':
      return JD_ARRAY;
    case '"':
      return JD_STRING;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return JD_NUMBER;
    case 't':
      return JD_TRUE;
    case 'f':
      return JD_FALSE;
    case 'n':
      return JD_NULL;
    default:
      jsondec_errf(d, "Unexpected character: '%c'", *d->ptr);
  }
}

/* JSON object/array **********************************************************/

/* These are used like so:
 *
 * jsondec_objstart(d);
 * while (jsondec_objnext(d)) {
 *   ...
 * }
 * jsondec_objend(d) */

static int jsondec_peek(jsondec *d) {
  jsondec_skipws(d);
  return jsondec_rawpeek(d);
}

static void jsondec_push(jsondec *d) {
  if (--d->depth < 0) {
    jsondec_err(d, "Recursion limit exceeded");
  }
  d->is_first = true;
}

static bool jsondec_seqnext(jsondec *d, char end_ch) {
  bool is_first = d->is_first;
  d->is_first = false;
  jsondec_skipws(d);
  if (*d->ptr == end_ch) return false;
  if (!is_first) jsondec_parselit(d, ",");
  return true;
}

static void jsondec_arrstart(jsondec *d) {
  jsondec_push(d);
  jsondec_wsch(d, '[');
}

static void jsondec_arrend(jsondec *d) {
  d->depth++;
  jsondec_wsch(d, ']');
}

static bool jsondec_arrnext(jsondec *d) {
  return jsondec_seqnext(d, ']');
}

static void jsondec_objstart(jsondec *d) {
  jsondec_push(d);
  jsondec_wsch(d, '{');
}

static void jsondec_objend(jsondec *d) {
  d->depth++;
  jsondec_wsch(d, '}');
}

static bool jsondec_objnext(jsondec *d) {
  if (!jsondec_seqnext(d, '}')) return false;
  if (jsondec_peek(d) != JD_STRING) {
    jsondec_err(d, "Object must start with string");
  }
  return true;
}

/* JSON number ****************************************************************/

static bool jsondec_tryskipdigits(jsondec *d) {
  const char *start = d->ptr;

  while (d->ptr < d->end) {
    if (*d->ptr < '0' || *d->ptr > '9') {
      break;
    }
    d->ptr++;
  }

  return d->ptr != start;
}

static void jsondec_skipdigits(jsondec *d) {
  if (!jsondec_tryskipdigits(d)) {
    jsondec_err(d, "Expected one or more digits");
  }
}

static double jsondec_number(jsondec *d) {
  const char *start = d->ptr;

  assert(jsondec_rawpeek(d) == JD_NUMBER);

  /* Skip over the syntax of a number, as specified by JSON. */
  if (*d->ptr == '-') d->ptr++;

  if (jsondec_tryparsech(d, '0')) {
    if (jsondec_tryskipdigits(d)) {
      jsondec_err(d, "number cannot have leading zero");
    }
  } else {
    jsondec_skipdigits(d);
  }

  if (d->ptr == d->end) goto parse;
  if (jsondec_tryparsech(d, '.')) {
    jsondec_skipdigits(d);
  }
  if (d->ptr == d->end) goto parse;

  if (*d->ptr == 'e' || *d->ptr == 'E') {
    d->ptr++;
    if (d->ptr == d->end) {
      jsondec_err(d, "Unexpected EOF in number");
    }
    if (*d->ptr == '+' || *d->ptr == '-') {
      d->ptr++;
    }
    jsondec_skipdigits(d);
  }

parse:
  /* Having verified the syntax of a JSON number, use strtod() to parse
   * (strtod() accepts a superset of JSON syntax). */
  errno = 0;
  {
    char* end;
    double val = strtod(start, &end);
    assert(end == d->ptr);

    /* Currently the min/max-val conformance tests fail if we check this.  Does
     * this mean the conformance tests are wrong or strtod() is wrong, or
     * something else?  Investigate further. */
    /*
    if (errno == ERANGE) {
      jsondec_err(d, "Number out of range");
    }
    */

    if (val > DBL_MAX || val < -DBL_MAX) {
      jsondec_err(d, "Number out of range");
    }

    return val;
  }
}

/* JSON string ****************************************************************/

static char jsondec_escape(jsondec *d) {
  switch (*d->ptr++) {
    case '"':
      return '\"';
    case '\\':
      return '\\';
    case '/':
      return '/';
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    default:
      jsondec_err(d, "Invalid escape char");
  }
}

static uint32_t jsondec_codepoint(jsondec *d) {
  uint32_t cp = 0;
  const char *end;

  if (d->end - d->ptr < 4) {
    jsondec_err(d, "EOF inside string");
  }

  end = d->ptr + 4;
  while (d->ptr < end) {
    char ch = *d->ptr++;
    if (ch >= '0' && ch <= '9') {
      ch -= '0';
    } else if (ch >= 'a' && ch <= 'f') {
      ch = ch - 'a' + 10;
    } else if (ch >= 'A' && ch <= 'F') {
      ch = ch - 'A' + 10;
    } else {
      jsondec_err(d, "Invalid hex digit");
    }
    cp = (cp << 4) | ch;
  }

  return cp;
}

/* Parses a \uXXXX unicode escape (possibly a surrogate pair). */
static size_t jsondec_unicode(jsondec *d, char* out) {
  uint32_t cp = jsondec_codepoint(d);
  if (cp >= 0xd800 && cp <= 0xdbff) {
    /* Surrogate pair: two 16-bit codepoints become a 32-bit codepoint. */
    uint32_t high = cp;
    uint32_t low;
    jsondec_parselit(d, "\\u");
    low = jsondec_codepoint(d);
    if (low < 0xdc00 || low > 0xdfff) {
      jsondec_err(d, "Invalid low surrogate");
    }
    cp = (high & 0x3ff) << 10;
    cp |= (low & 0x3ff);
    cp += 0x10000;
  } else if (cp >= 0xdc00 && cp <= 0xdfff) {
    jsondec_err(d, "Unpaired low surrogate");
  }

  /* Write to UTF-8 */
  if (cp <= 0x7f) {
    out[0] = cp;
    return 1;
  } else if (cp <= 0x07FF) {
    out[0] = ((cp >> 6) & 0x1F) | 0xC0;
    out[1] = ((cp >> 0) & 0x3F) | 0x80;
    return 2;
  } else if (cp <= 0xFFFF) {
    out[0] = ((cp >> 12) & 0x0F) | 0xE0;
    out[1] = ((cp >> 6) & 0x3F) | 0x80;
    out[2] = ((cp >> 0) & 0x3F) | 0x80;
    return 3;
  } else if (cp < 0x10FFFF) {
    out[0] = ((cp >> 18) & 0x07) | 0xF0;
    out[1] = ((cp >> 12) & 0x3f) | 0x80;
    out[2] = ((cp >> 6) & 0x3f) | 0x80;
    out[3] = ((cp >> 0) & 0x3f) | 0x80;
    return 4;
  } else {
    jsondec_err(d, "Invalid codepoint");
  }
}

static void jsondec_resize(jsondec *d, char **buf, char **end, char **buf_end) {
  size_t oldsize = *buf_end - *buf;
  size_t len = *end - *buf;
  size_t size = UPB_MAX(8, 2 * oldsize);

  *buf = upb_arena_realloc(d->arena, *buf, len, size);
  if (!*buf) jsondec_err(d, "Out of memory");

  *end = *buf + len;
  *buf_end = *buf + size;
}

static upb_strview jsondec_string(jsondec *d) {
  char *buf = NULL;
  char *end = NULL;
  char *buf_end = NULL;

  jsondec_skipws(d);

  if (*d->ptr++ != '"') {
    jsondec_err(d, "Expected string");
  }

  while (d->ptr < d->end) {
    char ch = *d->ptr++;

    if (end == buf_end) {
      jsondec_resize(d, &buf, &end, &buf_end);
    }

    switch (ch) {
      case '"': {
        upb_strview ret;
        ret.data = buf;
        ret.size = end - buf;
        *end = '\0';  /* Needed for possible strtod(). */
        return ret;
      }
      case '\\':
        if (d->ptr == d->end) goto eof;
        if (*d->ptr == 'u') {
          d->ptr++;
          if (buf_end - end < 4) {
            /* Allow space for maximum-sized code point (4 bytes). */
            jsondec_resize(d, &buf, &end, &buf_end);
          }
          end += jsondec_unicode(d, end);
        } else {
          *end++ = jsondec_escape(d);
        }
        break;
      default:
        if ((unsigned char)*d->ptr < 0x20) {
          jsondec_err(d, "Invalid char in JSON string");
        }
        *end++ = ch;
        break;
    }
  }

eof:
  jsondec_err(d, "EOF inside string");
}

static void jsondec_skipval(jsondec *d) {
  switch (jsondec_peek(d)) {
    case JD_OBJECT:
      jsondec_objstart(d);
      while (jsondec_objnext(d)) {
        jsondec_string(d);
        jsondec_entrysep(d);
        jsondec_skipval(d);
      }
      jsondec_objend(d);
      break;
    case JD_ARRAY:
      jsondec_arrstart(d);
      while (jsondec_arrnext(d)) {
        jsondec_skipval(d);
      }
      jsondec_arrend(d);
      break;
    case JD_TRUE:
      jsondec_true(d);
      break;
    case JD_FALSE:
      jsondec_false(d);
      break;
    case JD_NULL:
      jsondec_null(d);
      break;
    case JD_STRING:
      jsondec_string(d);
      break;
    case JD_NUMBER:
      jsondec_number(d);
      break;
  }
}

/* Base64 decoding for bytes fields. ******************************************/

static unsigned int jsondec_base64_tablelookup(const char ch) {
  /* Table includes the normal base64 chars plus the URL-safe variant. */
  const signed char table[256] = {
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       62 /*+*/, -1,       62 /*-*/, -1,       63 /*/ */, 52 /*0*/,
      53 /*1*/, 54 /*2*/, 55 /*3*/, 56 /*4*/, 57 /*5*/, 58 /*6*/,  59 /*7*/,
      60 /*8*/, 61 /*9*/, -1,       -1,       -1,       -1,        -1,
      -1,       -1,       0 /*A*/,  1 /*B*/,  2 /*C*/,  3 /*D*/,   4 /*E*/,
      5 /*F*/,  6 /*G*/,  07 /*H*/, 8 /*I*/,  9 /*J*/,  10 /*K*/,  11 /*L*/,
      12 /*M*/, 13 /*N*/, 14 /*O*/, 15 /*P*/, 16 /*Q*/, 17 /*R*/,  18 /*S*/,
      19 /*T*/, 20 /*U*/, 21 /*V*/, 22 /*W*/, 23 /*X*/, 24 /*Y*/,  25 /*Z*/,
      -1,       -1,       -1,       -1,       63 /*_*/, -1,        26 /*a*/,
      27 /*b*/, 28 /*c*/, 29 /*d*/, 30 /*e*/, 31 /*f*/, 32 /*g*/,  33 /*h*/,
      34 /*i*/, 35 /*j*/, 36 /*k*/, 37 /*l*/, 38 /*m*/, 39 /*n*/,  40 /*o*/,
      41 /*p*/, 42 /*q*/, 43 /*r*/, 44 /*s*/, 45 /*t*/, 46 /*u*/,  47 /*v*/,
      48 /*w*/, 49 /*x*/, 50 /*y*/, 51 /*z*/, -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1};

  /* Sign-extend return value so high bit will be set on any unexpected char. */
  return table[(unsigned)ch];
}

static char *jsondec_partialbase64(jsondec *d, const char *ptr, const char *end,
                                   char *out) {
  int32_t val = -1;

  switch (end - ptr) {
    case 2:
      val = jsondec_base64_tablelookup(ptr[0]) << 18 |
            jsondec_base64_tablelookup(ptr[1]) << 12;
      out[0] = val >> 16;
      out += 1;
      break;
    case 3:
      val = jsondec_base64_tablelookup(ptr[0]) << 18 |
            jsondec_base64_tablelookup(ptr[1]) << 12 |
            jsondec_base64_tablelookup(ptr[2]) << 6;
      out[0] = val >> 16;
      out[1] = (val >> 8) & 0xff;
      out += 2;
      break;
  }

  if (val < 0) {
    jsondec_err(d, "Corrupt base64");
  }

  return out;
}

static size_t jsondec_base64(jsondec *d, upb_strview str) {
  /* We decode in place. This is safe because this is a new buffer (not
   * aliasing the input) and because base64 decoding shrinks 4 bytes into 3. */
  char *out = (char*)str.data;
  const char *ptr = str.data;
  const char *end = ptr + str.size;
  const char *end4 = ptr + (str.size & -4);  /* Round down to multiple of 4. */

  for (; ptr < end4; ptr += 4, out += 3) {
    int val = jsondec_base64_tablelookup(ptr[0]) << 18 |
              jsondec_base64_tablelookup(ptr[1]) << 12 |
              jsondec_base64_tablelookup(ptr[2]) << 6 |
              jsondec_base64_tablelookup(ptr[3]) << 0;

    if (val < 0) {
      /* Junk chars or padding. Remove trailing padding, if any. */
      if (end - ptr == 4 && ptr[3] == '=') {
        if (ptr[2] == '=') {
          end -= 2;
        } else {
          end -= 1;
        }
      }
      break;
    }

    out[0] = val >> 16;
    out[1] = (val >> 8) & 0xff;
    out[2] = val & 0xff;
  }

  if (ptr < end) {
    /* Process remaining chars. We do not require padding. */
    out = jsondec_partialbase64(d, ptr, end, out);
  }

  return out - str.data;
}

/* Low-level integer parsing **************************************************/

/* We use these hand-written routines instead of strto[u]l() because the "long
 * long" variants aren't in c89. Also our version allows setting a ptr limit. */

static const char *jsondec_buftouint64(jsondec *d, const char *ptr,
                                       const char *end, uint64_t *val) {
  uint64_t u64 = 0;
  while (ptr < end) {
    unsigned ch = *ptr - '0';
    if (ch >= 10) break;
    if (u64 > UINT64_MAX / 10 || u64 * 10 > UINT64_MAX - ch) {
      jsondec_err(d, "Integer overflow");
    }
    u64 *= 10;
    u64 += ch;
    ptr++;
  }

  *val = u64;
  return ptr;
}

static const char *jsondec_buftoint64(jsondec *d, const char *ptr,
                                      const char *end, int64_t *val) {
  bool neg = false;
  uint64_t u64;

  if (ptr != end && *ptr == '-') {
    ptr++;
    neg = true;
  }

  ptr = jsondec_buftouint64(d, ptr, end, &u64);
  if (u64 > (uint64_t)INT64_MAX + neg) {
    jsondec_err(d, "Integer overflow");
  }

  *val = neg ? -u64 : u64;
  return ptr;
}

static uint64_t jsondec_strtouint64(jsondec *d, upb_strview str) {
  const char *end = str.data + str.size;
  uint64_t ret;
  if (jsondec_buftouint64(d, str.data, end, &ret) != end) {
    jsondec_err(d, "Non-number characters in quoted integer");
  }
  return ret;
}

static int64_t jsondec_strtoint64(jsondec *d, upb_strview str) {
  const char *end = str.data + str.size;
  int64_t ret;
  if (jsondec_buftoint64(d, str.data, end, &ret) != end) {
    jsondec_err(d, "Non-number characters in quoted integer");
  }
  return ret;
}

/* Primitive value types ******************************************************/

/* Parse INT32 or INT64 value. */
static upb_msgval jsondec_int(jsondec *d, const upb_fielddef *f) {
  upb_msgval val;

  switch (jsondec_peek(d)) {
    case JD_NUMBER: {
      double dbl = jsondec_number(d);
      if (dbl > 9223372036854774784.0 || dbl < -9223372036854775808.0) {
        jsondec_err(d, "JSON number is out of range.");
      }
      val.int64_val = dbl;  /* must be guarded, overflow here is UB */
      if (val.int64_val != dbl) {
        jsondec_errf(d, "JSON number was not integral (%f != %" PRId64 ")", dbl,
                     val.int64_val);
      }
      break;
    }
    case JD_STRING: {
      upb_strview str = jsondec_string(d);
      val.int64_val = jsondec_strtoint64(d, str);
      break;
    }
    default:
      jsondec_err(d, "Expected number or string");
  }

  if (upb_fielddef_type(f) == UPB_TYPE_INT32) {
    if (val.int64_val > INT32_MAX || val.int64_val < INT32_MIN) {
      jsondec_err(d, "Integer out of range.");
    }
    val.int32_val = (int32_t)val.int64_val;
  }

  return val;
}

/* Parse UINT32 or UINT64 value. */
static upb_msgval jsondec_uint(jsondec *d, const upb_fielddef *f) {
  upb_msgval val = {0};

  switch (jsondec_peek(d)) {
    case JD_NUMBER: {
      double dbl = jsondec_number(d);
      if (dbl > 18446744073709549568.0 || dbl < 0) {
        jsondec_err(d, "JSON number is out of range.");
      }
      val.uint64_val = dbl;  /* must be guarded, overflow here is UB */
      if (val.uint64_val != dbl) {
        jsondec_errf(d, "JSON number was not integral (%f != %" PRIu64 ")", dbl,
                     val.uint64_val);
      }
      break;
    }
    case JD_STRING: {
      upb_strview str = jsondec_string(d);
      val.uint64_val = jsondec_strtouint64(d, str);
      break;
    }
    default:
      jsondec_err(d, "Expected number or string");
  }

  if (upb_fielddef_type(f) == UPB_TYPE_UINT32) {
    if (val.uint64_val > UINT32_MAX) {
      jsondec_err(d, "Integer out of range.");
    }
    val.uint32_val = (uint32_t)val.uint64_val;
  }

  return val;
}

/* Parse DOUBLE or FLOAT value. */
static upb_msgval jsondec_double(jsondec *d, const upb_fielddef *f) {
  upb_strview str;
  upb_msgval val = {0};

  switch (jsondec_peek(d)) {
    case JD_NUMBER:
      val.double_val = jsondec_number(d);
      break;
    case JD_STRING:
      str = jsondec_string(d);
      if (jsondec_streql(str, "NaN")) {
        val.double_val = NAN;
      } else if (jsondec_streql(str, "Infinity")) {
        val.double_val = INFINITY;
      } else if (jsondec_streql(str, "-Infinity")) {
        val.double_val = -INFINITY;
      } else {
        val.double_val = strtod(str.data, NULL);
      }
      break;
    default:
      jsondec_err(d, "Expected number or string");
  }

  if (upb_fielddef_type(f) == UPB_TYPE_FLOAT) {
    if (val.double_val != INFINITY && val.double_val != -INFINITY &&
        (val.double_val > FLT_MAX || val.double_val < -FLT_MAX)) {
      jsondec_err(d, "Float out of range");
    }
    val.float_val = val.double_val;
  }

  return val;
}

/* Parse STRING or BYTES value. */
static upb_msgval jsondec_strfield(jsondec *d, const upb_fielddef *f) {
  upb_msgval val;
  val.str_val = jsondec_string(d);
  if (upb_fielddef_type(f) == UPB_TYPE_BYTES) {
    val.str_val.size = jsondec_base64(d, val.str_val);
  }
  return val;
}

static upb_msgval jsondec_enum(jsondec *d, const upb_fielddef *f) {
  switch (jsondec_peek(d)) {
    case JD_STRING: {
      const upb_enumdef *e = upb_fielddef_enumsubdef(f);
      upb_strview str = jsondec_string(d);
      upb_msgval val;
      if (!upb_enumdef_ntoi(e, str.data, str.size, &val.int32_val)) {
        if (d->options & UPB_JSONDEC_IGNOREUNKNOWN) {
          val.int32_val = 0;
        } else {
          jsondec_errf(d, "Unknown enumerator: '" UPB_STRVIEW_FORMAT "'",
                       UPB_STRVIEW_ARGS(str));
        }
      }
      return val;
    }
    case JD_NULL: {
      if (jsondec_isnullvalue(f)) {
        upb_msgval val;
        jsondec_null(d);
        val.int32_val = 0;
        return val;
      }
    }
      /* Fallthrough. */
    default:
      return jsondec_int(d, f);
  }
}

static upb_msgval jsondec_bool(jsondec *d, const upb_fielddef *f) {
  bool is_map_key = upb_fielddef_number(f) == 1 &&
                    upb_msgdef_mapentry(upb_fielddef_containingtype(f));
  upb_msgval val;

  if (is_map_key) {
    upb_strview str = jsondec_string(d);
    if (jsondec_streql(str, "true")) {
      val.bool_val = true;
    } else if (jsondec_streql(str, "false")) {
      val.bool_val = false;
    } else {
      jsondec_err(d, "Invalid boolean map key");
    }
  } else {
    switch (jsondec_peek(d)) {
      case JD_TRUE:
        val.bool_val = true;
        jsondec_true(d);
        break;
      case JD_FALSE:
        val.bool_val = false;
        jsondec_false(d);
        break;
      default:
        jsondec_err(d, "Expected true or false");
    }
  }

  return val;
}

/* Composite types (array/message/map) ****************************************/

static void jsondec_array(jsondec *d, upb_msg *msg, const upb_fielddef *f) {
  upb_array *arr = upb_msg_mutable(msg, f, d->arena).array;

  jsondec_arrstart(d);
  while (jsondec_arrnext(d)) {
    upb_msgval elem = jsondec_value(d, f);
    upb_array_append(arr, elem, d->arena);
  }
  jsondec_arrend(d);
}

static void jsondec_map(jsondec *d, upb_msg *msg, const upb_fielddef *f) {
  upb_map *map = upb_msg_mutable(msg, f, d->arena).map;
  const upb_msgdef *entry = upb_fielddef_msgsubdef(f);
  const upb_fielddef *key_f = upb_msgdef_itof(entry, 1);
  const upb_fielddef *val_f = upb_msgdef_itof(entry, 2);

  jsondec_objstart(d);
  while (jsondec_objnext(d)) {
    upb_msgval key, val;
    key = jsondec_value(d, key_f);
    jsondec_entrysep(d);
    val = jsondec_value(d, val_f);
    upb_map_set(map, key, val, d->arena);
  }
  jsondec_objend(d);
}

static void jsondec_tomsg(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  if (upb_msgdef_wellknowntype(m) == UPB_WELLKNOWN_UNSPECIFIED) {
    jsondec_object(d, msg, m);
  } else {
    jsondec_wellknown(d, msg, m);
  }
}

static upb_msgval jsondec_msg(jsondec *d, const upb_fielddef *f) {
  const upb_msgdef *m = upb_fielddef_msgsubdef(f);
  upb_msg *msg = upb_msg_new(m, d->arena);
  upb_msgval val;

  jsondec_tomsg(d, msg, m);
  val.msg_val = msg;
  return val;
}

static void jsondec_field(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  upb_strview name;
  const upb_fielddef *f;
  const upb_fielddef *preserved;

  name = jsondec_string(d);
  jsondec_entrysep(d);
  f = upb_msgdef_lookupjsonname(m, name.data, name.size);

  if (!f) {
    if ((d->options & UPB_JSONDEC_IGNOREUNKNOWN) == 0) {
      jsondec_errf(d, "No such field: " UPB_STRVIEW_FORMAT,
                   UPB_STRVIEW_ARGS(name));
    }
    jsondec_skipval(d);
    return;
  }

  if (upb_fielddef_realcontainingoneof(f) &&
      upb_msg_whichoneof(msg, upb_fielddef_containingoneof(f))) {
    jsondec_err(d, "More than one field for this oneof.");
  }

  if (jsondec_peek(d) == JD_NULL && !jsondec_isvalue(f)) {
    /* JSON "null" indicates a default value, so no need to set anything. */
    jsondec_null(d);
    return;
  }

  preserved = d->debug_field;
  d->debug_field = f;

  if (upb_fielddef_ismap(f)) {
    jsondec_map(d, msg, f);
  } else if (upb_fielddef_isseq(f)) {
    jsondec_array(d, msg, f);
  } else if (upb_fielddef_issubmsg(f)) {
    upb_msg *submsg = upb_msg_mutable(msg, f, d->arena).msg;
    const upb_msgdef *subm = upb_fielddef_msgsubdef(f);
    jsondec_tomsg(d, submsg, subm);
  } else {
    upb_msgval val = jsondec_value(d, f);
    upb_msg_set(msg, f, val, d->arena);
  }

  d->debug_field = preserved;
}

static void jsondec_object(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  jsondec_objstart(d);
  while (jsondec_objnext(d)) {
    jsondec_field(d, msg, m);
  }
  jsondec_objend(d);
}

static upb_msgval jsondec_value(jsondec *d, const upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_BOOL:
      return jsondec_bool(d, f);
    case UPB_TYPE_FLOAT:
    case UPB_TYPE_DOUBLE:
      return jsondec_double(d, f);
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
      return jsondec_uint(d, f);
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
      return jsondec_int(d, f);
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      return jsondec_strfield(d, f);
    case UPB_TYPE_ENUM:
      return jsondec_enum(d, f);
    case UPB_TYPE_MESSAGE:
      return jsondec_msg(d, f);
    default:
      UPB_UNREACHABLE();
  }
}

/* Well-known types ***********************************************************/

static int jsondec_tsdigits(jsondec *d, const char **ptr, size_t digits,
                            const char *after) {
  uint64_t val;
  const char *p = *ptr;
  const char *end = p + digits;
  size_t after_len = after ? strlen(after) : 0;

  UPB_ASSERT(digits <= 9);  /* int can't overflow. */

  if (jsondec_buftouint64(d, p, end, &val) != end ||
      (after_len && memcmp(end, after, after_len) != 0)) {
    jsondec_err(d, "Malformed timestamp");
  }

  UPB_ASSERT(val < INT_MAX);

  *ptr = end + after_len;
  return (int)val;
}

static int jsondec_nanos(jsondec *d, const char **ptr, const char *end) {
  uint64_t nanos = 0;
  const char *p = *ptr;

  if (p != end && *p == '.') {
    const char *nano_end = jsondec_buftouint64(d, p + 1, end, &nanos);
    int digits = (int)(nano_end - p - 1);
    int exp_lg10 = 9 - digits;
    if (digits > 9) {
      jsondec_err(d, "Too many digits for partial seconds");
    }
    while (exp_lg10--) nanos *= 10;
    *ptr = nano_end;
  }

  UPB_ASSERT(nanos < INT_MAX);

  return (int)nanos;
}

/* jsondec_epochdays(1970, 1, 1) == 1970-01-01 == 0. */
int jsondec_epochdays(int y, int m, int d) {
  const uint32_t year_base = 4800;    /* Before min year, multiple of 400. */
  const uint32_t m_adj = m - 3;       /* March-based month. */
  const uint32_t carry = m_adj > (uint32_t)m ? 1 : 0;
  const uint32_t adjust = carry ? 12 : 0;
  const uint32_t y_adj = y + year_base - carry;
  const uint32_t month_days = ((m_adj + adjust) * 62719 + 769) / 2048;
  const uint32_t leap_days = y_adj / 4 - y_adj / 100 + y_adj / 400;
  return y_adj * 365 + leap_days + month_days + (d - 1) - 2472632;
}

static int64_t jsondec_unixtime(int y, int m, int d, int h, int min, int s) {
  return (int64_t)jsondec_epochdays(y, m, d) * 86400 + h * 3600 + min * 60 + s;
}

static void jsondec_timestamp(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  upb_msgval seconds;
  upb_msgval nanos;
  upb_strview str = jsondec_string(d);
  const char *ptr = str.data;
  const char *end = ptr + str.size;

  if (str.size < 20) goto malformed;

  {
    /* 1972-01-01T01:00:00 */
    int year = jsondec_tsdigits(d, &ptr, 4, "-");
    int mon = jsondec_tsdigits(d, &ptr, 2, "-");
    int day = jsondec_tsdigits(d, &ptr, 2, "T");
    int hour = jsondec_tsdigits(d, &ptr, 2, ":");
    int min = jsondec_tsdigits(d, &ptr, 2, ":");
    int sec = jsondec_tsdigits(d, &ptr, 2, NULL);

    seconds.int64_val = jsondec_unixtime(year, mon, day, hour, min, sec);
  }

  nanos.int32_val = jsondec_nanos(d, &ptr, end);

  {
    /* [+-]08:00 or Z */
    int ofs_hour = 0;
    int ofs_min = 0;
    bool neg = false;

    if (ptr == end) goto malformed;

    switch (*ptr++) {
      case '-':
        neg = true;
        /* fallthrough */
      case '+':
        if ((end - ptr) != 5) goto malformed;
        ofs_hour = jsondec_tsdigits(d, &ptr, 2, ":");
        ofs_min = jsondec_tsdigits(d, &ptr, 2, NULL);
        ofs_min = ((ofs_hour * 60) + ofs_min) * 60;
        seconds.int64_val += (neg ? ofs_min : -ofs_min);
        break;
      case 'Z':
        if (ptr != end) goto malformed;
        break;
      default:
        goto malformed;
    }
  }

  if (seconds.int64_val < -62135596800) {
    jsondec_err(d, "Timestamp out of range");
  }

  upb_msg_set(msg, upb_msgdef_itof(m, 1), seconds, d->arena);
  upb_msg_set(msg, upb_msgdef_itof(m, 2), nanos, d->arena);
  return;

malformed:
  jsondec_err(d, "Malformed timestamp");
}

static void jsondec_duration(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  upb_msgval seconds;
  upb_msgval nanos;
  upb_strview str = jsondec_string(d);
  const char *ptr = str.data;
  const char *end = ptr + str.size;
  const int64_t max = (uint64_t)3652500 * 86400;

  /* "3.000000001s", "3s", etc. */
  ptr = jsondec_buftoint64(d, ptr, end, &seconds.int64_val);
  nanos.int32_val = jsondec_nanos(d, &ptr, end);

  if (end - ptr != 1 || *ptr != 's') {
    jsondec_err(d, "Malformed duration");
  }

  if (seconds.int64_val < -max || seconds.int64_val > max) {
    jsondec_err(d, "Duration out of range");
  }

  if (seconds.int64_val < 0) {
    nanos.int32_val = - nanos.int32_val;
  }

  upb_msg_set(msg, upb_msgdef_itof(m, 1), seconds, d->arena);
  upb_msg_set(msg, upb_msgdef_itof(m, 2), nanos, d->arena);
}

static void jsondec_listvalue(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  const upb_fielddef *values_f = upb_msgdef_itof(m, 1);
  const upb_msgdef *value_m = upb_fielddef_msgsubdef(values_f);
  upb_array *values = upb_msg_mutable(msg, values_f, d->arena).array;

  jsondec_arrstart(d);
  while (jsondec_arrnext(d)) {
    upb_msg *value_msg = upb_msg_new(value_m, d->arena);
    upb_msgval value;
    value.msg_val = value_msg;
    upb_array_append(values, value, d->arena);
    jsondec_wellknownvalue(d, value_msg, value_m);
  }
  jsondec_arrend(d);
}

static void jsondec_struct(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  const upb_fielddef *fields_f = upb_msgdef_itof(m, 1);
  const upb_msgdef *entry_m = upb_fielddef_msgsubdef(fields_f);
  const upb_fielddef *value_f = upb_msgdef_itof(entry_m, 2);
  const upb_msgdef *value_m = upb_fielddef_msgsubdef(value_f);
  upb_map *fields = upb_msg_mutable(msg, fields_f, d->arena).map;

  jsondec_objstart(d);
  while (jsondec_objnext(d)) {
    upb_msgval key, value;
    upb_msg *value_msg = upb_msg_new(value_m, d->arena);
    key.str_val = jsondec_string(d);
    value.msg_val = value_msg;
    upb_map_set(fields, key, value, d->arena);
    jsondec_entrysep(d);
    jsondec_wellknownvalue(d, value_msg, value_m);
  }
  jsondec_objend(d);
}

static void jsondec_wellknownvalue(jsondec *d, upb_msg *msg,
                                   const upb_msgdef *m) {
  upb_msgval val;
  const upb_fielddef *f;
  upb_msg *submsg;

  switch (jsondec_peek(d)) {
    case JD_NUMBER:
      /* double number_value = 2; */
      f = upb_msgdef_itof(m, 2);
      val.double_val = jsondec_number(d);
      break;
    case JD_STRING:
      /* string string_value = 3; */
      f = upb_msgdef_itof(m, 3);
      val.str_val = jsondec_string(d);
      break;
    case JD_FALSE:
      /* bool bool_value = 4; */
      f = upb_msgdef_itof(m, 4);
      val.bool_val = false;
      jsondec_false(d);
      break;
    case JD_TRUE:
      /* bool bool_value = 4; */
      f = upb_msgdef_itof(m, 4);
      val.bool_val = true;
      jsondec_true(d);
      break;
    case JD_NULL:
      /* NullValue null_value = 1; */
      f = upb_msgdef_itof(m, 1);
      val.int32_val = 0;
      jsondec_null(d);
      break;
    /* Note: these cases return, because upb_msg_mutable() is enough. */
    case JD_OBJECT:
      /* Struct struct_value = 5; */
      f = upb_msgdef_itof(m, 5);
      submsg = upb_msg_mutable(msg, f, d->arena).msg;
      jsondec_struct(d, submsg, upb_fielddef_msgsubdef(f));
      return;
    case JD_ARRAY:
      /* ListValue list_value = 6; */
      f = upb_msgdef_itof(m, 6);
      submsg = upb_msg_mutable(msg, f, d->arena).msg;
      jsondec_listvalue(d, submsg, upb_fielddef_msgsubdef(f));
      return;
    default:
      UPB_UNREACHABLE();
  }

  upb_msg_set(msg, f, val, d->arena);
}

static upb_strview jsondec_mask(jsondec *d, const char *buf, const char *end) {
  /* FieldMask fields grow due to inserted '_' characters, so we can't do the
   * transform in place. */
  const char *ptr = buf;
  upb_strview ret;
  char *out;

  ret.size = end - ptr;
  while (ptr < end) {
    ret.size += (*ptr >= 'A' && *ptr <= 'Z');
    ptr++;
  }

  out = upb_arena_malloc(d->arena, ret.size);
  ptr = buf;
  ret.data = out;

  while (ptr < end) {
    char ch = *ptr++;
    if (ch >= 'A' && ch <= 'Z') {
      *out++ = '_';
      *out++ = ch + 32;
    } else if (ch == '_') {
      jsondec_err(d, "field mask may not contain '_'");
    } else {
      *out++ = ch;
    }
  }

  return ret;
}

static void jsondec_fieldmask(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  /* repeated string paths = 1; */
  const upb_fielddef *paths_f = upb_msgdef_itof(m, 1);
  upb_array *arr = upb_msg_mutable(msg, paths_f, d->arena).array;
  upb_strview str = jsondec_string(d);
  const char *ptr = str.data;
  const char *end = ptr + str.size;
  upb_msgval val;

  while (ptr < end) {
    const char *elem_end = memchr(ptr, ',', end - ptr);
    if (elem_end) {
      val.str_val = jsondec_mask(d, ptr, elem_end);
      ptr = elem_end + 1;
    } else {
      val.str_val = jsondec_mask(d, ptr, end);
      ptr = end;
    }
    upb_array_append(arr, val, d->arena);
  }
}

static void jsondec_anyfield(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  if (upb_msgdef_wellknowntype(m) == UPB_WELLKNOWN_UNSPECIFIED) {
    /* For regular types: {"@type": "[user type]", "f1": <V1>, "f2": <V2>}
     * where f1, f2, etc. are the normal fields of this type. */
    jsondec_field(d, msg, m);
  } else {
    /* For well-known types: {"@type": "[well-known type]", "value": <X>}
     * where <X> is whatever encoding the WKT normally uses. */
    upb_strview str = jsondec_string(d);
    jsondec_entrysep(d);
    if (!jsondec_streql(str, "value")) {
      jsondec_err(d, "Key for well-known type must be 'value'");
    }
    jsondec_wellknown(d, msg, m);
  }
}

static const upb_msgdef *jsondec_typeurl(jsondec *d, upb_msg *msg,
                                         const upb_msgdef *m) {
  const upb_fielddef *type_url_f = upb_msgdef_itof(m, 1);
  const upb_msgdef *type_m;
  upb_strview type_url = jsondec_string(d);
  const char *end = type_url.data + type_url.size;
  const char *ptr = end;
  upb_msgval val;

  val.str_val = type_url;
  upb_msg_set(msg, type_url_f, val, d->arena);

  /* Find message name after the last '/' */
  while (ptr > type_url.data && *--ptr != '/') {}

  if (ptr == type_url.data || ptr == end) {
    jsondec_err(d, "Type url must have at least one '/' and non-empty host");
  }

  ptr++;
  type_m = upb_symtab_lookupmsg2(d->any_pool, ptr, end - ptr);

  if (!type_m) {
    jsondec_err(d, "Type was not found");
  }

  return type_m;
}

static void jsondec_any(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  /* string type_url = 1;
   * bytes value = 2; */
  const upb_fielddef *value_f = upb_msgdef_itof(m, 2);
  upb_msg *any_msg;
  const upb_msgdef *any_m = NULL;
  const char *pre_type_data = NULL;
  const char *pre_type_end = NULL;
  upb_msgval encoded;

  jsondec_objstart(d);

  /* Scan looking for "@type", which is not necessarily first. */
  while (!any_m && jsondec_objnext(d)) {
    const char *start = d->ptr;
    upb_strview name = jsondec_string(d);
    jsondec_entrysep(d);
    if (jsondec_streql(name, "@type")) {
      any_m = jsondec_typeurl(d, msg, m);
      if (pre_type_data) {
        pre_type_end = start;
        while (*pre_type_end != ',') pre_type_end--;
      }
    } else {
      if (!pre_type_data) pre_type_data = start;
      jsondec_skipval(d);
    }
  }

  if (!any_m) {
    jsondec_err(d, "Any object didn't contain a '@type' field");
  }

  any_msg = upb_msg_new(any_m, d->arena);

  if (pre_type_data) {
    size_t len = pre_type_end - pre_type_data + 1;
    char *tmp = upb_arena_malloc(d->arena, len);
    const char *saved_ptr = d->ptr;
    const char *saved_end = d->end;
    memcpy(tmp, pre_type_data, len - 1);
    tmp[len - 1] = '}';
    d->ptr = tmp;
    d->end = tmp + len;
    d->is_first = true;
    while (jsondec_objnext(d)) {
      jsondec_anyfield(d, any_msg, any_m);
    }
    d->ptr = saved_ptr;
    d->end = saved_end;
  }

  while (jsondec_objnext(d)) {
    jsondec_anyfield(d, any_msg, any_m);
  }

  jsondec_objend(d);

  encoded.str_val.data = upb_encode(any_msg, upb_msgdef_layout(any_m), d->arena,
                                    &encoded.str_val.size);
  upb_msg_set(msg, value_f, encoded, d->arena);
}

static void jsondec_wrapper(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  const upb_fielddef *value_f = upb_msgdef_itof(m, 1);
  upb_msgval val = jsondec_value(d, value_f);
  upb_msg_set(msg, value_f, val, d->arena);
}

static void jsondec_wellknown(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  switch (upb_msgdef_wellknowntype(m)) {
    case UPB_WELLKNOWN_ANY:
      jsondec_any(d, msg, m);
      break;
    case UPB_WELLKNOWN_FIELDMASK:
      jsondec_fieldmask(d, msg, m);
      break;
    case UPB_WELLKNOWN_DURATION:
      jsondec_duration(d, msg, m);
      break;
    case UPB_WELLKNOWN_TIMESTAMP:
      jsondec_timestamp(d, msg, m);
      break;
    case UPB_WELLKNOWN_VALUE:
      jsondec_wellknownvalue(d, msg, m);
      break;
    case UPB_WELLKNOWN_LISTVALUE:
      jsondec_listvalue(d, msg, m);
      break;
    case UPB_WELLKNOWN_STRUCT:
      jsondec_struct(d, msg, m);
      break;
    case UPB_WELLKNOWN_DOUBLEVALUE:
    case UPB_WELLKNOWN_FLOATVALUE:
    case UPB_WELLKNOWN_INT64VALUE:
    case UPB_WELLKNOWN_UINT64VALUE:
    case UPB_WELLKNOWN_INT32VALUE:
    case UPB_WELLKNOWN_UINT32VALUE:
    case UPB_WELLKNOWN_STRINGVALUE:
    case UPB_WELLKNOWN_BYTESVALUE:
    case UPB_WELLKNOWN_BOOLVALUE:
      jsondec_wrapper(d, msg, m);
      break;
    default:
      UPB_UNREACHABLE();
  }
}

bool upb_json_decode(const char *buf, size_t size, upb_msg *msg,
                     const upb_msgdef *m, const upb_symtab *any_pool,
                     int options, upb_arena *arena, upb_status *status) {
  jsondec d;
  d.ptr = buf;
  d.end = buf + size;
  d.arena = arena;
  d.any_pool = any_pool;
  d.status = status;
  d.options = options;
  d.depth = 64;
  d.line = 1;
  d.line_begin = d.ptr;
  d.debug_field = NULL;
  d.is_first = false;

  if (UPB_SETJMP(d.err)) return false;

  jsondec_tomsg(&d, msg, m);
  return true;
}


#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


/* Must be last. */

typedef struct {
  char *buf, *ptr, *end;
  size_t overflow;
  int indent_depth;
  int options;
  const upb_symtab *ext_pool;
  jmp_buf err;
  upb_status *status;
  upb_arena *arena;
} jsonenc;

static void jsonenc_msg(jsonenc *e, const upb_msg *msg, const upb_msgdef *m);
static void jsonenc_scalar(jsonenc *e, upb_msgval val, const upb_fielddef *f);
static void jsonenc_msgfield(jsonenc *e, const upb_msg *msg,
                             const upb_msgdef *m);
static void jsonenc_msgfields(jsonenc *e, const upb_msg *msg,
                              const upb_msgdef *m);
static void jsonenc_value(jsonenc *e, const upb_msg *msg, const upb_msgdef *m);

UPB_NORETURN static void jsonenc_err(jsonenc *e, const char *msg) {
  upb_status_seterrmsg(e->status, msg);
  longjmp(e->err, 1);
}

UPB_PRINTF(2, 3)
UPB_NORETURN static void jsonenc_errf(jsonenc *e, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  upb_status_vseterrf(e->status, fmt, argp);
  va_end(argp);
  longjmp(e->err, 1);
}

static upb_arena *jsonenc_arena(jsonenc *e) {
  /* Create lazily, since it's only needed for Any */
  if (!e->arena) {
    e->arena = upb_arena_new();
  }
  return e->arena;
}

static void jsonenc_putbytes(jsonenc *e, const void *data, size_t len) {
  size_t have = e->end - e->ptr;
  if (UPB_LIKELY(have >= len)) {
    memcpy(e->ptr, data, len);
    e->ptr += len;
  } else {
    if (have) memcpy(e->ptr, data, have);
    e->ptr += have;
    e->overflow += (len - have);
  }
}

static void jsonenc_putstr(jsonenc *e, const char *str) {
  jsonenc_putbytes(e, str, strlen(str));
}

UPB_PRINTF(2, 3)
static void jsonenc_printf(jsonenc *e, const char *fmt, ...) {
  size_t n;
  size_t have = e->end - e->ptr;
  va_list args;

  va_start(args, fmt);
  n = vsnprintf(e->ptr, have, fmt, args);
  va_end(args);

  if (UPB_LIKELY(have > n)) {
    e->ptr += n;
  } else {
    e->ptr += have;
    e->overflow += (n - have);
  }
}

static void jsonenc_nanos(jsonenc *e, int32_t nanos) {
  int digits = 9;

  if (nanos == 0) return;
  if (nanos < 0 || nanos >= 1000000000) {
    jsonenc_err(e, "error formatting timestamp as JSON: invalid nanos");
  }

  while (nanos % 1000 == 0) {
    nanos /= 1000;
    digits -= 3;
  }

  jsonenc_printf(e, ".%.*" PRId32, digits, nanos);
}

static void jsonenc_timestamp(jsonenc *e, const upb_msg *msg,
                              const upb_msgdef *m) {
  const upb_fielddef *seconds_f = upb_msgdef_itof(m, 1);
  const upb_fielddef *nanos_f = upb_msgdef_itof(m, 2);
  int64_t seconds = upb_msg_get(msg, seconds_f).int64_val;
  int32_t nanos = upb_msg_get(msg, nanos_f).int32_val;
  int L, N, I, J, K, hour, min, sec;

  if (seconds < -62135596800) {
    jsonenc_err(e,
                "error formatting timestamp as JSON: minimum acceptable value "
                "is 0001-01-01T00:00:00Z");
  } else if (seconds > 253402300799) {
    jsonenc_err(e,
                "error formatting timestamp as JSON: maximum acceptable value "
                "is 9999-12-31T23:59:59Z");
  }

  /* Julian Day -> Y/M/D, Algorithm from:
   * Fliegel, H. F., and Van Flandern, T. C., "A Machine Algorithm for
   *   Processing Calendar Dates," Communications of the Association of
   *   Computing Machines, vol. 11 (1968), p. 657.  */
  L = (int)(seconds / 86400) + 68569 + 2440588;
  N = 4 * L / 146097;
  L = L - (146097 * N + 3) / 4;
  I = 4000 * (L + 1) / 1461001;
  L = L - 1461 * I / 4 + 31;
  J = 80 * L / 2447;
  K = L - 2447 * J / 80;
  L = J / 11;
  J = J + 2 - 12 * L;
  I = 100 * (N - 49) + I + L;

  sec = seconds % 60;
  min = (seconds / 60) % 60;
  hour = (seconds / 3600) % 24;

  jsonenc_printf(e, "\"%04d-%02d-%02dT%02d:%02d:%02d", I, J, K, hour, min, sec);
  jsonenc_nanos(e, nanos);
  jsonenc_putstr(e, "Z\"");
}

static void jsonenc_duration(jsonenc *e, const upb_msg *msg, const upb_msgdef *m) {
  const upb_fielddef *seconds_f = upb_msgdef_itof(m, 1);
  const upb_fielddef *nanos_f = upb_msgdef_itof(m, 2);
  int64_t seconds = upb_msg_get(msg, seconds_f).int64_val;
  int32_t nanos = upb_msg_get(msg, nanos_f).int32_val;

  if (seconds > 315576000000 || seconds < -315576000000 ||
      (seconds < 0) != (nanos < 0)) {
    jsonenc_err(e, "bad duration");
  }

  if (nanos < 0) {
    nanos = -nanos;
  }

  jsonenc_printf(e, "\"%" PRId64, seconds);
  jsonenc_nanos(e, nanos);
  jsonenc_putstr(e, "s\"");
}

static void jsonenc_enum(int32_t val, const upb_fielddef *f, jsonenc *e) {
  const upb_enumdef *e_def = upb_fielddef_enumsubdef(f);

  if (strcmp(upb_enumdef_fullname(e_def), "google.protobuf.NullValue") == 0) {
    jsonenc_putstr(e, "null");
  } else {
    const char *name = upb_enumdef_iton(e_def, val);

    if (name) {
      jsonenc_printf(e, "\"%s\"", name);
    } else {
      jsonenc_printf(e, "%" PRId32, val);
    }
  }
}

static void jsonenc_bytes(jsonenc *e, upb_strview str) {
  /* This is the regular base64, not the "web-safe" version. */
  static const char base64[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const unsigned char *ptr = (unsigned char*)str.data;
  const unsigned char *end = ptr + str.size;
  char buf[4];

  jsonenc_putstr(e, "\"");

  while (end - ptr >= 3) {
    buf[0] = base64[ptr[0] >> 2];
    buf[1] = base64[((ptr[0] & 0x3) << 4) | (ptr[1] >> 4)];
    buf[2] = base64[((ptr[1] & 0xf) << 2) | (ptr[2] >> 6)];
    buf[3] = base64[ptr[2] & 0x3f];
    jsonenc_putbytes(e, buf, 4);
    ptr += 3;
  }

  switch (end - ptr) {
    case 2:
      buf[0] = base64[ptr[0] >> 2];
      buf[1] = base64[((ptr[0] & 0x3) << 4) | (ptr[1] >> 4)];
      buf[2] = base64[(ptr[1] & 0xf) << 2];
      buf[3] = '=';
      jsonenc_putbytes(e, buf, 4);
      break;
    case 1:
      buf[0] = base64[ptr[0] >> 2];
      buf[1] = base64[((ptr[0] & 0x3) << 4)];
      buf[2] = '=';
      buf[3] = '=';
      jsonenc_putbytes(e, buf, 4);
      break;
  }

  jsonenc_putstr(e, "\"");
}

static void jsonenc_stringbody(jsonenc *e, upb_strview str) {
  const char *ptr = str.data;
  const char *end = ptr + str.size;

  while (ptr < end) {
    switch (*ptr) {
      case '\n':
        jsonenc_putstr(e, "\\n");
        break;
      case '\r':
        jsonenc_putstr(e, "\\r");
        break;
      case '\t':
        jsonenc_putstr(e, "\\t");
        break;
      case '\"':
        jsonenc_putstr(e, "\\\"");
        break;
      case '\f':
        jsonenc_putstr(e, "\\f");
        break;
      case '\b':
        jsonenc_putstr(e, "\\b");
        break;
      case '\\':
        jsonenc_putstr(e, "\\\\");
        break;
      default:
        if ((uint8_t)*ptr < 0x20) {
          jsonenc_printf(e, "\\u%04x", (int)(uint8_t)*ptr);
        } else {
          /* This could be a non-ASCII byte.  We rely on the string being valid
           * UTF-8. */
          jsonenc_putbytes(e, ptr, 1);
        }
        break;
    }
    ptr++;
  }
}

static void jsonenc_string(jsonenc *e, upb_strview str) {
  jsonenc_putstr(e, "\"");
  jsonenc_stringbody(e, str);
  jsonenc_putstr(e, "\"");
}

static void jsonenc_double(jsonenc *e, const char *fmt, double val) {
  if (val == INFINITY) {
    jsonenc_putstr(e, "\"Infinity\"");
  } else if (val == -INFINITY) {
    jsonenc_putstr(e, "\"-Infinity\"");
  } else if (val != val) {
    jsonenc_putstr(e, "\"NaN\"");
  } else {
    jsonenc_printf(e, fmt, val);
  }
}

static void jsonenc_wrapper(jsonenc *e, const upb_msg *msg,
                            const upb_msgdef *m) {
  const upb_fielddef *val_f = upb_msgdef_itof(m, 1);
  upb_msgval val = upb_msg_get(msg, val_f);
  jsonenc_scalar(e, val, val_f);
}

static const upb_msgdef *jsonenc_getanymsg(jsonenc *e, upb_strview type_url) {
  /* Find last '/', if any. */
  const char *end = type_url.data + type_url.size;
  const char *ptr = end;
  const upb_msgdef *ret;

  if (!e->ext_pool) {
    jsonenc_err(e, "Tried to encode Any, but no symtab was provided");
  }

  if (type_url.size == 0) goto badurl;

  while (true) {
    if (--ptr == type_url.data) {
      /* Type URL must contain at least one '/', with host before. */
      goto badurl;
    }
    if (*ptr == '/') {
      ptr++;
      break;
    }
  }

  ret = upb_symtab_lookupmsg2(e->ext_pool, ptr, end - ptr);

  if (!ret) {
    jsonenc_errf(e, "Couldn't find Any type: %.*s", (int)(end - ptr), ptr);
  }

  return ret;

badurl:
  jsonenc_errf(
      e, "Bad type URL: " UPB_STRVIEW_FORMAT, UPB_STRVIEW_ARGS(type_url));
}

static void jsonenc_any(jsonenc *e, const upb_msg *msg, const upb_msgdef *m) {
  const upb_fielddef *type_url_f = upb_msgdef_itof(m, 1);
  const upb_fielddef *value_f = upb_msgdef_itof(m, 2);
  upb_strview type_url = upb_msg_get(msg, type_url_f).str_val;
  upb_strview value = upb_msg_get(msg, value_f).str_val;
  const upb_msgdef *any_m = jsonenc_getanymsg(e, type_url);
  const upb_msglayout *any_layout = upb_msgdef_layout(any_m);
  upb_arena *arena = jsonenc_arena(e);
  upb_msg *any = upb_msg_new(any_m, arena);

  if (!upb_decode(value.data, value.size, any, any_layout, arena)) {
    jsonenc_err(e, "Error decoding message in Any");
  }

  jsonenc_putstr(e, "{\"@type\":");
  jsonenc_string(e, type_url);
  jsonenc_putstr(e, ",");

  if (upb_msgdef_wellknowntype(any_m) == UPB_WELLKNOWN_UNSPECIFIED) {
    /* Regular messages: {"@type": "...","foo": 1, "bar": 2} */
    jsonenc_msgfields(e, any, any_m);
  } else {
    /* Well-known type: {"@type": "...","value": <well-known encoding>} */
    jsonenc_putstr(e, "\"value\":");
    jsonenc_msgfield(e, any, any_m);
  }

  jsonenc_putstr(e, "}");
}

static void jsonenc_putsep(jsonenc *e, const char *str, bool *first) {
  if (*first) {
    *first = false;
  } else {
    jsonenc_putstr(e, str);
  }
}

static void jsonenc_fieldpath(jsonenc *e, upb_strview path) {
  const char *ptr = path.data;
  const char *end = ptr + path.size;

  while (ptr < end) {
    char ch = *ptr;

    if (ch >= 'A' && ch <= 'Z') {
      jsonenc_err(e, "Field mask element may not have upper-case letter.");
    } else if (ch == '_') {
      if (ptr == end - 1 || *(ptr + 1) < 'a' || *(ptr + 1) > 'z') {
        jsonenc_err(e, "Underscore must be followed by a lowercase letter.");
      }
      ch = *++ptr - 32;
    }

    jsonenc_putbytes(e, &ch, 1);
    ptr++;
  }
}

static void jsonenc_fieldmask(jsonenc *e, const upb_msg *msg,
                              const upb_msgdef *m) {
  const upb_fielddef *paths_f = upb_msgdef_itof(m, 1);
  const upb_array *paths = upb_msg_get(msg, paths_f).array_val;
  bool first = true;
  size_t i, n = 0;

  if (paths) n = upb_array_size(paths);

  jsonenc_putstr(e, "\"");

  for (i = 0; i < n; i++) {
    jsonenc_putsep(e, ",", &first);
    jsonenc_fieldpath(e, upb_array_get(paths, i).str_val);
  }

  jsonenc_putstr(e, "\"");
}

static void jsonenc_struct(jsonenc *e, const upb_msg *msg,
                           const upb_msgdef *m) {
  const upb_fielddef *fields_f = upb_msgdef_itof(m, 1);
  const upb_map *fields = upb_msg_get(msg, fields_f).map_val;
  const upb_msgdef *entry_m = upb_fielddef_msgsubdef(fields_f);
  const upb_fielddef *value_f = upb_msgdef_itof(entry_m, 2);
  size_t iter = UPB_MAP_BEGIN;
  bool first = true;

  jsonenc_putstr(e, "{");

  if (fields) {
    while (upb_mapiter_next(fields, &iter)) {
      upb_msgval key = upb_mapiter_key(fields, iter);
      upb_msgval val = upb_mapiter_value(fields, iter);

      jsonenc_putsep(e, ",", &first);
      jsonenc_string(e, key.str_val);
      jsonenc_putstr(e, ":");
      jsonenc_value(e, val.msg_val, upb_fielddef_msgsubdef(value_f));
    }
  }

  jsonenc_putstr(e, "}");
}

static void jsonenc_listvalue(jsonenc *e, const upb_msg *msg,
                              const upb_msgdef *m) {
  const upb_fielddef *values_f = upb_msgdef_itof(m, 1);
  const upb_msgdef *values_m = upb_fielddef_msgsubdef(values_f);
  const upb_array *values = upb_msg_get(msg, values_f).array_val;
  size_t i;
  bool first = true;

  jsonenc_putstr(e, "[");

  if (values) {
    const size_t size = upb_array_size(values);
    for (i = 0; i < size; i++) {
      upb_msgval elem = upb_array_get(values, i);

      jsonenc_putsep(e, ",", &first);
      jsonenc_value(e, elem.msg_val, values_m);
    }
  }

  jsonenc_putstr(e, "]");
}

static void jsonenc_value(jsonenc *e, const upb_msg *msg, const upb_msgdef *m) {
  /* TODO(haberman): do we want a reflection method to get oneof case? */
  size_t iter = UPB_MSG_BEGIN;
  const upb_fielddef *f;
  upb_msgval val;

  if (!upb_msg_next(msg, m, NULL,  &f, &val, &iter)) {
    jsonenc_err(e, "No value set in Value proto");
  }

  switch (upb_fielddef_number(f)) {
    case 1:
      jsonenc_putstr(e, "null");
      break;
    case 2:
      jsonenc_double(e, "%.17g", val.double_val);
      break;
    case 3:
      jsonenc_string(e, val.str_val);
      break;
    case 4:
      jsonenc_putstr(e, val.bool_val ? "true" : "false");
      break;
    case 5:
      jsonenc_struct(e, val.msg_val, upb_fielddef_msgsubdef(f));
      break;
    case 6:
      jsonenc_listvalue(e, val.msg_val, upb_fielddef_msgsubdef(f));
      break;
  }
}

static void jsonenc_msgfield(jsonenc *e, const upb_msg *msg,
                             const upb_msgdef *m) {
  switch (upb_msgdef_wellknowntype(m)) {
    case UPB_WELLKNOWN_UNSPECIFIED:
      jsonenc_msg(e, msg, m);
      break;
    case UPB_WELLKNOWN_ANY:
      jsonenc_any(e, msg, m);
      break;
    case UPB_WELLKNOWN_FIELDMASK:
      jsonenc_fieldmask(e, msg, m);
      break;
    case UPB_WELLKNOWN_DURATION:
      jsonenc_duration(e, msg, m);
      break;
    case UPB_WELLKNOWN_TIMESTAMP:
      jsonenc_timestamp(e, msg, m);
      break;
    case UPB_WELLKNOWN_DOUBLEVALUE:
    case UPB_WELLKNOWN_FLOATVALUE:
    case UPB_WELLKNOWN_INT64VALUE:
    case UPB_WELLKNOWN_UINT64VALUE:
    case UPB_WELLKNOWN_INT32VALUE:
    case UPB_WELLKNOWN_UINT32VALUE:
    case UPB_WELLKNOWN_STRINGVALUE:
    case UPB_WELLKNOWN_BYTESVALUE:
    case UPB_WELLKNOWN_BOOLVALUE:
      jsonenc_wrapper(e, msg, m);
      break;
    case UPB_WELLKNOWN_VALUE:
      jsonenc_value(e, msg, m);
      break;
    case UPB_WELLKNOWN_LISTVALUE:
      jsonenc_listvalue(e, msg, m);
      break;
    case UPB_WELLKNOWN_STRUCT:
      jsonenc_struct(e, msg, m);
      break;
  }
}

static void jsonenc_scalar(jsonenc *e, upb_msgval val, const upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_BOOL:
      jsonenc_putstr(e, val.bool_val ? "true" : "false");
      break;
    case UPB_TYPE_FLOAT:
      jsonenc_double(e, "%.9g", val.float_val);
      break;
    case UPB_TYPE_DOUBLE:
      jsonenc_double(e, "%.17g", val.double_val);
      break;
    case UPB_TYPE_INT32:
      jsonenc_printf(e, "%" PRId32, val.int32_val);
      break;
    case UPB_TYPE_UINT32:
      jsonenc_printf(e, "%" PRIu32, val.uint32_val);
      break;
    case UPB_TYPE_INT64:
      jsonenc_printf(e, "\"%" PRId64 "\"", val.int64_val);
      break;
    case UPB_TYPE_UINT64:
      jsonenc_printf(e, "\"%" PRIu64 "\"", val.uint64_val);
      break;
    case UPB_TYPE_STRING:
      jsonenc_string(e, val.str_val);
      break;
    case UPB_TYPE_BYTES:
      jsonenc_bytes(e, val.str_val);
      break;
    case UPB_TYPE_ENUM:
      jsonenc_enum(val.int32_val, f, e);
      break;
    case UPB_TYPE_MESSAGE:
      jsonenc_msgfield(e, val.msg_val, upb_fielddef_msgsubdef(f));
      break;
  }
}

static void jsonenc_mapkey(jsonenc *e, upb_msgval val, const upb_fielddef *f) {
  jsonenc_putstr(e, "\"");

  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_BOOL:
      jsonenc_putstr(e, val.bool_val ? "true" : "false");
      break;
    case UPB_TYPE_INT32:
      jsonenc_printf(e, "%" PRId32, val.int32_val);
      break;
    case UPB_TYPE_UINT32:
      jsonenc_printf(e, "%" PRIu32, val.uint32_val);
      break;
    case UPB_TYPE_INT64:
      jsonenc_printf(e, "%" PRId64, val.int64_val);
      break;
    case UPB_TYPE_UINT64:
      jsonenc_printf(e, "%" PRIu64, val.uint64_val);
      break;
    case UPB_TYPE_STRING:
      jsonenc_stringbody(e, val.str_val);
      break;
    default:
      UPB_UNREACHABLE();
  }

  jsonenc_putstr(e, "\":");
}

static void jsonenc_array(jsonenc *e, const upb_array *arr,
                         const upb_fielddef *f) {
  size_t i;
  size_t size = arr ? upb_array_size(arr) : 0;
  bool first = true;

  jsonenc_putstr(e, "[");

  for (i = 0; i < size; i++) {
    jsonenc_putsep(e, ",", &first);
    jsonenc_scalar(e, upb_array_get(arr, i), f);
  }

  jsonenc_putstr(e, "]");
}

static void jsonenc_map(jsonenc *e, const upb_map *map, const upb_fielddef *f) {
  const upb_msgdef *entry = upb_fielddef_msgsubdef(f);
  const upb_fielddef *key_f = upb_msgdef_itof(entry, 1);
  const upb_fielddef *val_f = upb_msgdef_itof(entry, 2);
  size_t iter = UPB_MAP_BEGIN;
  bool first = true;

  jsonenc_putstr(e, "{");

  if (map) {
    while (upb_mapiter_next(map, &iter)) {
      jsonenc_putsep(e, ",", &first);
      jsonenc_mapkey(e, upb_mapiter_key(map, iter), key_f);
      jsonenc_scalar(e, upb_mapiter_value(map, iter), val_f);
    }
  }

  jsonenc_putstr(e, "}");
}

static void jsonenc_fieldval(jsonenc *e, const upb_fielddef *f,
                             upb_msgval val, bool *first) {
  const char *name;

  if (e->options & UPB_JSONENC_PROTONAMES) {
    name = upb_fielddef_name(f);
  } else {
    name = upb_fielddef_jsonname(f);
  }

  jsonenc_putsep(e, ",", first);
  jsonenc_printf(e, "\"%s\":", name);

  if (upb_fielddef_ismap(f)) {
    jsonenc_map(e, val.map_val, f);
  } else if (upb_fielddef_isseq(f)) {
    jsonenc_array(e, val.array_val, f);
  } else {
    jsonenc_scalar(e, val, f);
  }
}

static void jsonenc_msgfields(jsonenc *e, const upb_msg *msg,
                              const upb_msgdef *m) {
  upb_msgval val;
  const upb_fielddef *f;
  bool first = true;

  if (e->options & UPB_JSONENC_EMITDEFAULTS) {
    /* Iterate over all fields. */
    int i = 0;
    int n = upb_msgdef_fieldcount(m);
    for (i = 0; i < n; i++) {
      f = upb_msgdef_field(m, i);
      if (!upb_fielddef_haspresence(f) || upb_msg_has(msg, f)) {
        jsonenc_fieldval(e, f, upb_msg_get(msg, f), &first);
      }
    }
  } else {
    /* Iterate over non-empty fields. */
    size_t iter = UPB_MSG_BEGIN;
    while (upb_msg_next(msg, m, e->ext_pool, &f, &val, &iter)) {
      jsonenc_fieldval(e, f, val, &first);
    }
  }
}

static void jsonenc_msg(jsonenc *e, const upb_msg *msg, const upb_msgdef *m) {
  jsonenc_putstr(e, "{");
  jsonenc_msgfields(e, msg, m);
  jsonenc_putstr(e, "}");
}

static size_t jsonenc_nullz(jsonenc *e, size_t size) {
  size_t ret = e->ptr - e->buf + e->overflow;

  if (size > 0) {
    if (e->ptr == e->end) e->ptr--;
    *e->ptr = '\0';
  }

  return ret;
}

size_t upb_json_encode(const upb_msg *msg, const upb_msgdef *m,
                       const upb_symtab *ext_pool, int options, char *buf,
                       size_t size, upb_status *status) {
  jsonenc e;

  e.buf = buf;
  e.ptr = buf;
  e.end = buf + size;
  e.overflow = 0;
  e.options = options;
  e.ext_pool = ext_pool;
  e.status = status;
  e.arena = NULL;

  if (setjmp(e.err)) return -1;

  jsonenc_msgfield(&e, msg, m);
  if (e.arena) upb_arena_free(e.arena);
  return jsonenc_nullz(&e, size);
}
/* See port_def.inc.  This should #undef all macros #defined there. */

#undef UPB_MAPTYPE_STRING
#undef UPB_SIZE
#undef UPB_PTR_AT
#undef UPB_READ_ONEOF
#undef UPB_WRITE_ONEOF
#undef UPB_INLINE
#undef UPB_ALIGN_UP
#undef UPB_ALIGN_DOWN
#undef UPB_ALIGN_MALLOC
#undef UPB_ALIGN_OF
#undef UPB_FORCEINLINE
#undef UPB_NOINLINE
#undef UPB_NORETURN
#undef UPB_MAX
#undef UPB_MIN
#undef UPB_UNUSED
#undef UPB_ASSUME
#undef UPB_ASSERT
#undef UPB_UNREACHABLE
#undef UPB_POISON_MEMORY_REGION
#undef UPB_UNPOISON_MEMORY_REGION
#undef UPB_ASAN
