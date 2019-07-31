
#include "upb/msg.h"

#include "upb/table.int.h"

#include "upb/port_def.inc"

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
