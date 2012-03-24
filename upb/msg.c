/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 */

#include "upb/upb.h"
#include "upb/msg.h"

#define UPB_ACCESSOR(type, ctype)                                             \
  upb_flow_t upb_stdmsg_set ## type (void *_m, upb_value fval,                \
                                     upb_value val) {                         \
    assert(_m != NULL);                                                       \
    const upb_fielddef *f = upb_value_getfielddef(fval);                      \
    uint8_t *m = _m;                                                          \
    /* Hasbit is set automatically by the handlers. */                        \
    *(ctype*)&m[f->offset] = upb_value_get ## type(val);                      \
    return UPB_CONTINUE;                                                      \
  }                                                                           \

UPB_ACCESSOR(double, double)
UPB_ACCESSOR(float, float)
UPB_ACCESSOR(int32, int32_t)
UPB_ACCESSOR(int64, int64_t)
UPB_ACCESSOR(uint32, uint32_t)
UPB_ACCESSOR(uint64, uint64_t)
UPB_ACCESSOR(bool, bool)
UPB_ACCESSOR(ptr, void*)
#undef UPB_ACCESSORS

static void upb_accessors_onfreg(void *c, upb_fhandlers *fh,
                                 const upb_fielddef *f) {
  (void)c;
  if (f->accessor) {
    upb_fhandlers_setfval(fh, f->fval);
    if (upb_isseq(f)) {
      upb_fhandlers_setstartseq(fh, f->accessor->startseq);
      upb_fhandlers_setvalue(fh, f->accessor->append);
      upb_fhandlers_setstartsubmsg(fh, f->accessor->appendsubmsg);
    } else {
      upb_fhandlers_setvalue(fh, f->accessor->set);
      upb_fhandlers_setstartsubmsg(fh, f->accessor->startsubmsg);
      upb_fhandlers_sethasbit(fh, f->hasbit);
    }
  }
}

upb_mhandlers *upb_accessors_reghandlers(upb_handlers *h, const upb_msgdef *m) {
  return upb_handlers_regmsgdef(h, m, NULL, &upb_accessors_onfreg, NULL);
}
