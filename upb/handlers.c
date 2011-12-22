/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <stdlib.h>
#include "upb/handlers.h"


/* upb_mhandlers **************************************************************/

static upb_mhandlers *upb_mhandlers_new() {
  upb_mhandlers *m = malloc(sizeof(*m));
  upb_inttable_init(&m->fieldtab, 8, sizeof(upb_itofhandlers_ent));
  m->startmsg = NULL;
  m->endmsg = NULL;
  m->is_group = false;
#ifdef UPB_USE_JIT_X64
  m->tablearray = NULL;
#endif
  return m;
}

static upb_fhandlers *_upb_mhandlers_newfhandlers(upb_mhandlers *m, uint32_t n,
                                                  upb_fieldtype_t type,
                                                  bool repeated) {
  upb_itofhandlers_ent *e = upb_inttable_lookup(&m->fieldtab, n);
  // TODO: design/refine the API for changing the set of fields or modifying
  // existing handlers.
  if (e) return NULL;
  upb_fhandlers new_f = {type, repeated, UPB_ATOMIC_INIT(0),
      n, -1, m, NULL, UPB_NO_VALUE, NULL, NULL, NULL, NULL, NULL,
#ifdef UPB_USE_JIT_X64
      0, 0, 0,
#endif
      NULL};
  upb_fhandlers *ptr = malloc(sizeof(*ptr));
  memcpy(ptr, &new_f, sizeof(upb_fhandlers));
  upb_itofhandlers_ent ent = {false, ptr};
  upb_inttable_insert(&m->fieldtab, n, &ent);
  return ptr;
}

upb_fhandlers *upb_mhandlers_newfhandlers(upb_mhandlers *m, uint32_t n,
                                          upb_fieldtype_t type, bool repeated) {
  assert(type != UPB_TYPE(MESSAGE));
  assert(type != UPB_TYPE(GROUP));
  return _upb_mhandlers_newfhandlers(m, n, type, repeated);
}

upb_fhandlers *upb_mhandlers_newfhandlers_subm(upb_mhandlers *m, uint32_t n,
                                               upb_fieldtype_t type,
                                               bool repeated,
                                               upb_mhandlers *subm) {
  assert(type == UPB_TYPE(MESSAGE) || type == UPB_TYPE(GROUP));
  assert(subm);
  upb_fhandlers *f = _upb_mhandlers_newfhandlers(m, n, type, repeated);
  if (!f) return NULL;
  f->submsg = subm;
  if (type == UPB_TYPE(GROUP))
    _upb_mhandlers_newfhandlers(subm, n, UPB_TYPE_ENDGROUP, false);
  return f;
}


/* upb_handlers ***************************************************************/

upb_handlers *upb_handlers_new() {
  upb_handlers *h = malloc(sizeof(*h));
  upb_atomic_init(&h->refcount, 1);
  h->msgs_len = 0;
  h->msgs_size = 4;
  h->msgs = malloc(h->msgs_size * sizeof(*h->msgs));
  h->should_jit = true;
  return h;
}

void upb_handlers_ref(upb_handlers *h) { upb_atomic_ref(&h->refcount); }

void upb_handlers_unref(upb_handlers *h) {
  if (upb_atomic_unref(&h->refcount)) {
    for (int i = 0; i < h->msgs_len; i++) {
      upb_mhandlers *mh = h->msgs[i];
      for(upb_inttable_iter j = upb_inttable_begin(&mh->fieldtab);
          !upb_inttable_done(j);
          j = upb_inttable_next(&mh->fieldtab, j)) {
        upb_itofhandlers_ent *e = upb_inttable_iter_value(j);
        free(e->f);
      }
      upb_inttable_free(&mh->fieldtab);
#ifdef UPB_USE_JIT_X64
      free(mh->tablearray);
#endif
      free(mh);
    }
    free(h->msgs);
    free(h);
  }
}

upb_mhandlers *upb_handlers_newmhandlers(upb_handlers *h) {
  if (h->msgs_len == h->msgs_size) {
    h->msgs_size *= 2;
    h->msgs = realloc(h->msgs, h->msgs_size * sizeof(*h->msgs));
  }
  upb_mhandlers *mh = upb_mhandlers_new();
  h->msgs[h->msgs_len++] = mh;
  return mh;
}

typedef struct {
  upb_mhandlers *mh;
} upb_mtab_ent;

static upb_mhandlers *upb_regmsg_dfs(upb_handlers *h, const upb_msgdef *m,
                                     upb_onmsgreg *msgreg_cb,
                                     upb_onfieldreg *fieldreg_cb,
                                     void *closure, upb_strtable *mtab) {
  upb_mhandlers *mh = upb_handlers_newmhandlers(h);
  upb_mtab_ent e = {mh};
  upb_strtable_insert(mtab, m->base.fqname, &e);
  if (msgreg_cb) msgreg_cb(closure, mh, m);
  upb_msg_iter i;
  for(i = upb_msg_begin(m); !upb_msg_done(i); i = upb_msg_next(m, i)) {
    upb_fielddef *f = upb_msg_iter_field(i);
    upb_fhandlers *fh;
    if (upb_issubmsg(f)) {
      upb_mhandlers *sub_mh;
      upb_mtab_ent *subm_ent;
      // The table lookup is necessary to break the DFS for type cycles.
      if ((subm_ent = upb_strtable_lookup(mtab, f->def->fqname)) != NULL) {
        sub_mh = subm_ent->mh;
      } else {
        sub_mh = upb_regmsg_dfs(h, upb_downcast_msgdef(f->def), msgreg_cb,
                                fieldreg_cb, closure, mtab);
      }
      fh = upb_mhandlers_newfhandlers_subm(
          mh, f->number, f->type, upb_isseq(f), sub_mh);
    } else {
      fh = upb_mhandlers_newfhandlers(mh, f->number, f->type, upb_isseq(f));
    }
    if (fieldreg_cb) fieldreg_cb(closure, fh, f);
  }
  return mh;
}

upb_mhandlers *upb_handlers_regmsgdef(upb_handlers *h, const upb_msgdef *m,
                                      upb_onmsgreg *msgreg_cb,
                                      upb_onfieldreg *fieldreg_cb,
                                      void *closure) {
  upb_strtable mtab;
  upb_strtable_init(&mtab, 8, sizeof(upb_mtab_ent));
  upb_mhandlers *ret =
      upb_regmsg_dfs(h, m, msgreg_cb, fieldreg_cb, closure, &mtab);
  upb_strtable_free(&mtab);
  return ret;
}


/* upb_dispatcher *************************************************************/

void upb_dispatcher_init(upb_dispatcher *d, upb_status *status,
                         upb_exit_handler UPB_NORETURN *exit,
                         void *srcclosure) {
  d->stack[0].f = NULL;  // Should never be read.
  d->limit = &d->stack[UPB_MAX_NESTING];
  d->exitjmp = exit;
  d->srcclosure = srcclosure;
  d->top_is_implicit = false;
  d->msgent = NULL;
  d->top = NULL;
  d->toplevel_msgent = NULL;
  d->status = status;
}

upb_dispatcher_frame *upb_dispatcher_reset(upb_dispatcher *d, void *closure,
                                           upb_mhandlers *top) {
  d->msgent = top;
  d->toplevel_msgent = top;
  d->top = d->stack;
  d->top->closure = closure;
  d->top->is_sequence = false;
  d->top->is_packed = false;
  return d->top;
}

void upb_dispatcher_uninit(upb_dispatcher *d) {
  (void)d;
}

void upb_dispatch_startmsg(upb_dispatcher *d) {
  upb_flow_t flow = UPB_CONTINUE;
  if (d->msgent->startmsg) d->msgent->startmsg(d->top->closure);
  if (flow != UPB_CONTINUE) _upb_dispatcher_abortjmp(d);
}

void upb_dispatch_endmsg(upb_dispatcher *d, upb_status *status) {
  assert(d->top == d->stack);
  if (d->msgent->endmsg) d->msgent->endmsg(d->top->closure, d->status);
  // TODO: should we avoid this copy by passing client's status obj to cbs?
  upb_status_copy(status, d->status);
}

upb_dispatcher_frame *upb_dispatch_startseq(upb_dispatcher *d,
                                            upb_fhandlers *f) {
  if (d->top + 1 >= d->limit) {
    upb_status_seterrliteral(d->status, "Nesting too deep.");
    _upb_dispatcher_abortjmp(d);
  }

  upb_sflow_t sflow = UPB_CONTINUE_WITH(d->top->closure);
  if (f->startseq) sflow = f->startseq(d->top->closure, f->fval);
  if (sflow.flow != UPB_CONTINUE) {
    _upb_dispatcher_abortjmp(d);
  }

  ++d->top;
  d->top->f = f;
  d->top->is_sequence = true;
  d->top->is_packed = false;
  d->top->closure = sflow.closure;
  return d->top;
}

upb_dispatcher_frame *upb_dispatch_endseq(upb_dispatcher *d) {
  assert(d->top > d->stack);
  assert(d->top->is_sequence);
  upb_fhandlers *f = d->top->f;
  --d->top;
  upb_flow_t flow = UPB_CONTINUE;
  if (f->endseq) flow = f->endseq(d->top->closure, f->fval);
  if (flow != UPB_CONTINUE) {
    _upb_dispatcher_abortjmp(d);
  }
  d->msgent = d->top->f ? d->top->f->submsg : d->toplevel_msgent;
  return d->top;
}

upb_dispatcher_frame *upb_dispatch_startsubmsg(upb_dispatcher *d,
                                               upb_fhandlers *f) {
  if (d->top + 1 >= d->limit) {
    upb_status_seterrliteral(d->status, "Nesting too deep.");
    _upb_dispatcher_abortjmp(d);
  }

  upb_sflow_t sflow = UPB_CONTINUE_WITH(d->top->closure);
  if (f->startsubmsg) sflow = f->startsubmsg(d->top->closure, f->fval);
  if (sflow.flow != UPB_CONTINUE) {
    _upb_dispatcher_abortjmp(d);
  }

  ++d->top;
  d->top->f = f;
  d->top->is_sequence = false;
  d->top->is_packed = false;
  d->top->closure = sflow.closure;
  d->msgent = f->submsg;
  upb_dispatch_startmsg(d);
  return d->top;
}

upb_dispatcher_frame *upb_dispatch_endsubmsg(upb_dispatcher *d) {
  assert(d->top > d->stack);
  assert(!d->top->is_sequence);
  upb_fhandlers *f = d->top->f;
  if (d->msgent->endmsg) d->msgent->endmsg(d->top->closure, d->status);
  d->msgent = d->top->f->msg;
  --d->top;
  upb_flow_t flow = UPB_CONTINUE;
  if (f->endsubmsg) f->endsubmsg(d->top->closure, f->fval);
  if (flow != UPB_CONTINUE) _upb_dispatcher_abortjmp(d);
  return d->top;
}

bool upb_dispatcher_stackempty(upb_dispatcher *d) {
  return d->top == d->stack;
}
bool upb_dispatcher_islegalend(upb_dispatcher *d) {
  if (d->top == d->stack) return true;
  if (d->top - 1 == d->stack &&
      d->top->is_sequence && !d->top->is_packed) return true;
  return false;
}

void _upb_dispatcher_abortjmp(upb_dispatcher *d) {
  d->exitjmp(d->srcclosure);
  assert(false);  // Never returns.
}
