/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <stdlib.h>
#include "upb_handlers.h"


upb_flow_t upb_startmsg_nop(void *closure) {
  (void)closure;
  return UPB_CONTINUE;
}

void upb_endmsg_nop(void *closure, upb_status *status) {
  (void)closure;
  (void)status;
}

upb_flow_t upb_value_nop(void *closure, upb_value fval, upb_value val) {
  (void)closure;
  (void)fval;
  (void)val;
  return UPB_CONTINUE;
}

upb_sflow_t upb_startsubmsg_nop(void *closure, upb_value fval) {
  (void)fval;
  return UPB_CONTINUE_WITH(closure);
}

upb_flow_t upb_endsubmsg_nop(void *closure, upb_value fval) {
  (void)closure;
  (void)fval;
  return UPB_CONTINUE;
}


/* upb_mhandlers **************************************************************/

static upb_mhandlers *upb_mhandlers_new() {
  upb_mhandlers *m = malloc(sizeof(*m));
  upb_inttable_init(&m->fieldtab, 8, sizeof(upb_fhandlers));
  m->startmsg = &upb_startmsg_nop;
  m->endmsg = &upb_endmsg_nop;
  m->tablearray = NULL;
  m->is_group = false;
  return m;
}

static upb_fhandlers *_upb_mhandlers_newfield(upb_mhandlers *m, uint32_t n,
                                              upb_fieldtype_t type,
                                              bool repeated) {
  uint32_t tag = n << 3 | upb_types[type].native_wire_type;
  upb_fhandlers *f = upb_inttable_lookup(&m->fieldtab, tag);
  if (f) abort();
  upb_fhandlers new_f = {false, type, repeated,
      repeated && upb_isprimitivetype(type), n, NULL, UPB_NO_VALUE,
      &upb_value_nop, &upb_startsubmsg_nop, &upb_endsubmsg_nop, 0, 0, 0, NULL};
  if (upb_issubmsgtype(type)) new_f.startsubmsg = &upb_startsubmsg_nop;
  upb_inttable_insert(&m->fieldtab, tag, &new_f);
  f = upb_inttable_lookup(&m->fieldtab, tag);
  assert(f);
  assert(f->type == type);
  return f;
}

upb_fhandlers *upb_mhandlers_newfield(upb_mhandlers *m, uint32_t n,
                                      upb_fieldtype_t type, bool repeated) {
  assert(type != UPB_TYPE(MESSAGE));
  assert(type != UPB_TYPE(GROUP));
  return _upb_mhandlers_newfield(m, n, type, repeated);
}

upb_fhandlers *upb_mhandlers_newsubmsgfield(upb_mhandlers *m, uint32_t n,
                                            upb_fieldtype_t type, bool repeated,
                                            upb_mhandlers *subm) {
  assert(type == UPB_TYPE(MESSAGE) || type == UPB_TYPE(GROUP));
  assert(subm);
  upb_fhandlers *f = _upb_mhandlers_newfield(m, n, type, repeated);
  f->submsg = subm;
  if (type == UPB_TYPE(GROUP))
    _upb_mhandlers_newfield(subm, n, UPB_TYPE_ENDGROUP, false);
  return f;
}

typedef struct {
  upb_strtable_entry e;
  upb_mhandlers *mh;
} upb_mtab_ent;

static upb_mhandlers *upb_regmsg_dfs(upb_handlers *h, upb_msgdef *m,
                                     upb_onmsgreg *msgreg_cb,
                                     upb_onfieldreg *fieldreg_cb,
                                     void *closure, upb_strtable *mtab) {
  upb_mhandlers *mh = upb_handlers_newmsg(h);
  upb_mtab_ent e = {{m->base.fqname, 0}, mh};
  upb_strtable_insert(mtab, &e.e);
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
      fh = upb_mhandlers_newsubmsgfield(
          mh, f->number, f->type, upb_isarray(f), sub_mh);
    } else {
      fh = upb_mhandlers_newfield(mh, f->number, f->type, upb_isarray(f));
    }
    if (fieldreg_cb) fieldreg_cb(closure, fh, f);
  }
  return mh;
}

upb_mhandlers *upb_handlers_regmsgdef(upb_handlers *h, upb_msgdef *m,
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


/* upb_handlers ***************************************************************/

void upb_handlers_init(upb_handlers *h) {
  h->msgs_len = 0;
  h->msgs_size = 4;
  h->msgs = malloc(h->msgs_size * sizeof(*h->msgs));
  h->should_jit = true;
}

void upb_handlers_uninit(upb_handlers *h) {
  for (int i = 0; i < h->msgs_len; i++) {
    upb_mhandlers *mh = h->msgs[i];
    upb_inttable_free(&mh->fieldtab);
    free(mh->tablearray);
    free(mh);
  }
  free(h->msgs);
}

upb_mhandlers *upb_handlers_newmsg(upb_handlers *h) {
  if (h->msgs_len == h->msgs_size) {
    h->msgs_size *= 2;
    h->msgs = realloc(h->msgs, h->msgs_size * sizeof(*h->msgs));
  }
  upb_mhandlers *mh = upb_mhandlers_new();
  h->msgs[h->msgs_len++] = mh;
  return mh;
}


/* upb_dispatcher *************************************************************/

static upb_fhandlers toplevel_f = {
  false, UPB_TYPE(GROUP), false, false, 0,
  NULL, // submsg
#ifdef NDEBUG
  {{0}},
#else
  {{0}, UPB_VALUETYPE_RAW},
#endif
  NULL, NULL, NULL, 0, 0, 0, NULL};

void upb_dispatcher_init(upb_dispatcher *d, upb_handlers *h,
                         upb_skip_handler *skip, upb_exit_handler *exit,
                         void *srcclosure) {
  d->handlers = h;
  for (int i = 0; i < h->msgs_len; i++) {
    upb_mhandlers *m = h->msgs[i];
    upb_inttable_compact(&m->fieldtab);
  }
  d->stack[0].f = &toplevel_f;
  d->limit = &d->stack[UPB_MAX_NESTING];
  d->skip = skip;
  d->exit = exit;
  d->srcclosure = srcclosure;
  upb_status_init(&d->status);
}

upb_dispatcher_frame *upb_dispatcher_reset(upb_dispatcher *d, void *closure) {
  d->msgent = d->handlers->msgs[0];
  d->dispatch_table = &d->msgent->fieldtab;
  d->top = d->stack;
  d->top->closure = closure;
  return d->top;
}

void upb_dispatcher_uninit(upb_dispatcher *d) {
  upb_handlers_uninit(d->handlers);
  upb_status_uninit(&d->status);
}

void upb_dispatch_startmsg(upb_dispatcher *d) {
  upb_flow_t flow = d->msgent->startmsg(d->top->closure);
  if (flow != UPB_CONTINUE) _upb_dispatcher_unwind(d, flow);
}

void upb_dispatch_endmsg(upb_dispatcher *d, upb_status *status) {
  assert(d->top == d->stack);
  d->msgent->endmsg(d->top->closure, &d->status);
  // TODO: should we avoid this copy by passing client's status obj to cbs?
  upb_copyerr(status, &d->status);
}

upb_dispatcher_frame *upb_dispatch_startsubmsg(upb_dispatcher *d,
                                               upb_fhandlers *f) {
  if((d->top+1) >= d->limit) {
    upb_seterr(&d->status, UPB_ERROR, "Nesting too deep.");
    _upb_dispatcher_unwind(d, UPB_BREAK);
    return d->top;  // Dummy.
  }

  upb_sflow_t sflow = f->startsubmsg(d->top->closure, f->fval);
  if (sflow.flow != UPB_CONTINUE) {
    _upb_dispatcher_unwind(d, sflow.flow);
    return d->top;  // Dummy.
  }

  ++d->top;
  d->top->f = f;
  d->top->is_sequence = false;
  d->top->closure = sflow.closure;
  d->msgent = f->submsg;
  d->dispatch_table = &d->msgent->fieldtab;
  upb_dispatch_startmsg(d);
  return d->top;
}

upb_dispatcher_frame *upb_dispatch_endsubmsg(upb_dispatcher *d) {
  assert(d->top > d->stack);
  void *c = d->top->closure;
  upb_fhandlers *f = d->top->f;
  --d->top;
  d->msgent->endmsg(c, &d->status);
  upb_flow_t flow = f->endsubmsg(d->top->closure, f->fval);
  d->msgent = d->top->f->submsg ? d->top->f->submsg : d->handlers->msgs[0];
  d->dispatch_table = &d->msgent->fieldtab;
  if (flow != UPB_CONTINUE) _upb_dispatcher_unwind(d, flow);
  return d->top;
}

bool upb_dispatcher_stackempty(upb_dispatcher *d) {
  return d->top == d->stack;
}

void _upb_dispatcher_unwind(upb_dispatcher *d, upb_flow_t flow) {
  upb_dispatcher_frame *frame = d->top;
  while (1) {
    frame->f->submsg->endmsg(frame->closure, &d->status);
    frame->f->endsubmsg(frame->closure, frame->f->fval);
    --frame;
    if (frame < d->stack) { d->exit(d->srcclosure); return; }
    d->top = frame;
    if (flow == UPB_SKIPSUBMSG) return;
  }
}
