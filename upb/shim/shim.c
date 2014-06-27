/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2013 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb/shim/shim.h"

#include <stdlib.h>

// Fallback implementation if the shim is not specialized by the JIT.
#define SHIM_WRITER(type, ctype)                                              \
  bool upb_shim_set ## type (void *c, const void *hd, ctype val) {            \
    uint8_t *m = c;                                                           \
    const upb_shim_data *d = hd;                                              \
    if (d->hasbit > 0)                                                        \
      *(uint8_t*)&m[d->hasbit / 8] |= 1 << (d->hasbit % 8);                   \
    *(ctype*)&m[d->offset] = val;                                             \
    return true;                                                              \
  }                                                                           \

SHIM_WRITER(double, double)
SHIM_WRITER(float,  float)
SHIM_WRITER(int32,  int32_t)
SHIM_WRITER(int64,  int64_t)
SHIM_WRITER(uint32, uint32_t)
SHIM_WRITER(uint64, uint64_t)
SHIM_WRITER(bool,   bool)
#undef SHIM_WRITER

bool upb_shim_set(upb_handlers *h, const upb_fielddef *f, size_t offset,
                  int32_t hasbit) {
  upb_shim_data *d = malloc(sizeof(*d));
  if (!d) return false;
  d->offset = offset;
  d->hasbit = hasbit;

  upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
  upb_handlerattr_sethandlerdata(&attr, d);
  upb_handlerattr_setalwaysok(&attr, true);
  upb_handlers_addcleanup(h, d, free);

#define TYPE(u, l) \
  case UPB_TYPE_##u: \
    ok = upb_handlers_set##l(h, f, upb_shim_set##l, &attr); break;

  bool ok = false;

  switch (upb_fielddef_type(f)) {
    TYPE(INT64,  int64);
    TYPE(INT32,  int32);
    TYPE(ENUM,   int32);
    TYPE(UINT64, uint64);
    TYPE(UINT32, uint32);
    TYPE(DOUBLE, double);
    TYPE(FLOAT,  float);
    TYPE(BOOL,   bool);
    default: assert(false); break;
  }
#undef TYPE

  upb_handlerattr_uninit(&attr);
  return ok;
}

const upb_shim_data *upb_shim_getdata(const upb_handlers *h, upb_selector_t s,
                                      upb_fieldtype_t *type) {
  upb_func *f = upb_handlers_gethandler(h, s);

  if ((upb_int64_handlerfunc*)f == upb_shim_setint64) {
    *type = UPB_TYPE_INT64;
  } else if ((upb_int32_handlerfunc*)f == upb_shim_setint32) {
    *type = UPB_TYPE_INT32;
  } else if ((upb_uint64_handlerfunc*)f == upb_shim_setuint64) {
    *type = UPB_TYPE_UINT64;
  } else if ((upb_uint32_handlerfunc*)f == upb_shim_setuint32) {
    *type = UPB_TYPE_UINT32;
  } else if ((upb_double_handlerfunc*)f == upb_shim_setdouble) {
    *type = UPB_TYPE_DOUBLE;
  } else if ((upb_float_handlerfunc*)f == upb_shim_setfloat) {
    *type = UPB_TYPE_FLOAT;
  } else if ((upb_bool_handlerfunc*)f == upb_shim_setbool) {
    *type = UPB_TYPE_BOOL;
  } else {
    return NULL;
  }

  return (const upb_shim_data*)upb_handlers_gethandlerdata(h, s);
}
