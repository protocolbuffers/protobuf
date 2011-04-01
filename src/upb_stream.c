/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <stdlib.h>
#include "upb_stream.h"


/* upb_handlers ***************************************************************/

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

upb_flow_t upb_unknownval_nop(void *closure, upb_field_number_t fieldnum,
                              upb_value val) {
  (void)closure;
  (void)fieldnum;
  (void)val;
  return UPB_CONTINUE;
}

static void upb_msgent_init(upb_handlers_msgent *e) {
  upb_inttable_init(&e->fieldtab, 8, sizeof(upb_handlers_fieldent));
  e->startmsg = &upb_startmsg_nop;
  e->endmsg = &upb_endmsg_nop;
  e->unknownval = &upb_unknownval_nop;
  e->is_group = false;
  e->tablearray = NULL;
}

void upb_handlers_init(upb_handlers *h, upb_msgdef *md) {
  h->msgs_len = 1;
  h->msgs_size = 4;
  h->msgs = malloc(h->msgs_size * sizeof(*h->msgs));
  h->top = &h->stack[0];
  h->limit = &h->stack[UPB_MAX_TYPE_DEPTH];
  h->toplevel_msgdef = md;
  h->should_jit = true;
  if (md) upb_msgdef_ref(md);

  h->top->msgent_index = 0;
  h->top->msgdef = md;
  h->msgent = &h->msgs[0];
  upb_msgent_init(h->msgent);
}

void upb_handlers_uninit(upb_handlers *h) {
  for (int i = 0; i < h->msgs_len; i++) {
    upb_inttable_free(&h->msgs[i].fieldtab);
    free(h->msgs[i].tablearray);
  }
  free(h->msgs);
  upb_msgdef_unref(h->toplevel_msgdef);
}

static upb_handlers_fieldent *upb_handlers_getorcreate_without_fval(
    upb_handlers *h, upb_field_number_t fieldnum, upb_fieldtype_t type, bool repeated) {
  uint32_t tag = fieldnum << 3 | upb_types[type].native_wire_type;
  upb_handlers_fieldent *f =
      upb_inttable_lookup(&h->msgent->fieldtab, tag);
  if (!f) {
    upb_handlers_fieldent new_f = {false, type, -1, UPB_NO_VALUE,
        {&upb_value_nop}, &upb_endsubmsg_nop, 0, 0, 0, repeated};
    if (upb_issubmsgtype(type)) new_f.cb.startsubmsg = &upb_startsubmsg_nop;
    upb_inttable_insert(&h->msgent->fieldtab, tag, &new_f);

    f = upb_inttable_lookup(&h->msgent->fieldtab, tag);
    assert(f);
  }
  assert(f->type == type);
  return f;
}

static upb_handlers_fieldent *upb_handlers_getorcreate(
    upb_handlers *h, upb_field_number_t fieldnum,
    upb_fieldtype_t type, bool repeated, upb_value fval) {
  upb_handlers_fieldent *f =
      upb_handlers_getorcreate_without_fval(h, fieldnum, type, repeated);
  f->fval = fval;
  return f;
}

void upb_register_startend(upb_handlers *h, upb_startmsg_handler_t startmsg,
                           upb_endmsg_handler_t endmsg) {
  h->msgent->startmsg = startmsg ? startmsg : &upb_startmsg_nop;
  h->msgent->endmsg = endmsg ? endmsg : &upb_endmsg_nop;
}

// TODO:
// void upb_register_unknownval(upb_handlers *h,
//                              upb_unknownval_handler_t unknown);
// bool upb_handlers_link(upb_handlers *h, upb_fielddef *f);
// void upb_register_path_value(upb_handlers *h, const char *path,
//                              upb_value_handler_t value, upb_value fval);

void upb_register_all(upb_handlers *h, upb_startmsg_handler_t start,
                      upb_endmsg_handler_t end,
                      upb_value_handler_t value,
                      upb_startsubmsg_handler_t startsubmsg,
                      upb_endsubmsg_handler_t endsubmsg,
                      upb_unknownval_handler_t unknown) {
  upb_register_startend(h, start, end);
  //upb_register_unknownval(h, unknown);
  upb_msgdef *m = h->top->msgdef;
  upb_msg_iter i;
  for(i = upb_msg_begin(m); !upb_msg_done(i); i = upb_msg_next(m, i)) {
    upb_fielddef *f = upb_msg_iter_field(i);
    upb_value fval;
    upb_value_setfielddef(&fval, f);
    if (upb_issubmsg(f)) {
      upb_handlers_push(h, f, startsubmsg, endsubmsg, fval, false);
      upb_register_all(h, start, end, value, startsubmsg, endsubmsg, unknown);
      upb_handlers_pop(h, f);
    } else {
      upb_register_value(h, f, value, fval);
    }
  }
}

void upb_register_typed_value(upb_handlers *h, upb_field_number_t fieldnum,
                              upb_fieldtype_t type, bool repeated,
                              upb_value_handler_t value, upb_value fval) {
  upb_handlers_getorcreate(h, fieldnum, type, repeated, fval)->cb.value =
      value ? value : &upb_value_nop;
}

void upb_register_value(upb_handlers *h, upb_fielddef *f,
                        upb_value_handler_t value, upb_value fval) {
  assert(f->msgdef == h->top->msgdef);
  upb_register_typed_value(h, f->number, f->type, upb_isarray(f), value, fval);
}

void upb_register_typed_submsg(upb_handlers *h, upb_field_number_t fieldnum,
                               upb_fieldtype_t type, bool repeated,
                               upb_startsubmsg_handler_t start,
                               upb_endsubmsg_handler_t end,
                               upb_value fval) {
  upb_handlers_fieldent *f = upb_handlers_getorcreate(h, fieldnum, type, repeated, fval);
  f->cb.startsubmsg = start ? start : &upb_startsubmsg_nop;
  f->endsubmsg = end ? end : &upb_endsubmsg_nop;
}

void upb_handlers_typed_link(upb_handlers *h, upb_field_number_t fieldnum,
                             upb_fieldtype_t type, bool repeated, int frames) {
  assert(frames <= (h->top - h->stack));
  upb_handlers_fieldent *f =
      upb_handlers_getorcreate_without_fval(h, fieldnum, type, repeated);
  f->msgent_index = (h->top - frames)->msgent_index;
}

void upb_handlers_typed_push(upb_handlers *h, upb_field_number_t fieldnum,
                             upb_fieldtype_t type, bool repeated) {
  upb_handlers_fieldent *f =
      upb_handlers_getorcreate_without_fval(h, fieldnum, type, repeated);
  if (h->top == h->limit) abort();  // TODO: make growable.
  ++h->top;
  if (f->msgent_index == -1) {
    // Need to push a new msgent.
    if (h->msgs_size == h->msgs_len) {
      h->msgs_size *= 2;
      h->msgs = realloc(h->msgs, h->msgs_size * sizeof(*h->msgs));
    }
    f->msgent_index = h->msgs_len++;
    h->msgent = &h->msgs[f->msgent_index];
    upb_msgent_init(h->msgent);
  } else {
    h->msgent = &h->msgs[f->msgent_index];
  }
  h->top->msgent_index = f->msgent_index;
  if (h->toplevel_msgdef) {
    upb_fielddef *f = upb_msgdef_itof((h->top - 1)->msgdef, fieldnum);
    assert(f);
    h->top->msgdef = upb_downcast_msgdef(f->def);
  }
}

void upb_handlers_push(upb_handlers *h, upb_fielddef *f,
                       upb_startsubmsg_handler_t start,
                       upb_endsubmsg_handler_t end, upb_value fval,
                       bool delegate) {
  assert(f->msgdef == h->top->msgdef);
  (void)delegate;  // TODO
  upb_register_typed_submsg(h, f->number, f->type, upb_isarray(f), start, end, fval);
  upb_handlers_typed_push(h, f->number, f->type, upb_isarray(f));
}

void upb_handlers_typed_pop(upb_handlers *h) {
  assert(h->top > h->stack);
  --h->top;
  h->msgent = &h->msgs[h->top->msgent_index];
}

void upb_handlers_pop(upb_handlers *h, upb_fielddef *f) {
  (void)f; // TODO: Check that this matches the corresponding push.
  upb_handlers_typed_pop(h);
}

/* upb_dispatcher *************************************************************/

static upb_handlers_fieldent toplevel_f = {
  false, UPB_TYPE(GROUP),
  0, // msgent_index
#ifdef NDEBUG
  {{0}},
#else
  {{0}, UPB_VALUETYPE_RAW},
#endif
  {NULL}, NULL, 0, 0, 0, false};

void upb_dispatcher_init(upb_dispatcher *d, upb_handlers *h) {
  d->handlers = h;
  for (int i = 0; i < h->msgs_len; i++)
    upb_inttable_compact(&h->msgs[i].fieldtab);
  d->stack[0].f = &toplevel_f;
  d->limit = &d->stack[UPB_MAX_NESTING];
  upb_status_init(&d->status);
}

void upb_dispatcher_reset(upb_dispatcher *d, void *top_closure, uint32_t top_end_offset) {
  d->msgent = &d->handlers->msgs[0];
  d->dispatch_table = &d->msgent->fieldtab;
  d->current_depth = 0;
  d->skip_depth = INT_MAX;
  d->noframe_depth = INT_MAX;
  d->delegated_depth = 0;
  d->top = d->stack;
  d->top->closure = top_closure;
  d->top->end_offset = top_end_offset;
}

void upb_dispatcher_uninit(upb_dispatcher *d) {
  upb_handlers_uninit(d->handlers);
  upb_status_uninit(&d->status);
}

void upb_dispatcher_break(upb_dispatcher *d) {
  assert(d->skip_depth == INT_MAX);
  assert(d->noframe_depth == INT_MAX);
  d->noframe_depth = d->current_depth;
}

upb_flow_t upb_dispatch_startmsg(upb_dispatcher *d) {
  upb_flow_t flow = d->msgent->startmsg(d->top->closure);
  if (flow != UPB_CONTINUE) {
    d->noframe_depth = d->current_depth + 1;
    d->skip_depth = (flow == UPB_BREAK) ? d->delegated_depth : d->current_depth;
    return UPB_SKIPSUBMSG;
  }
  return UPB_CONTINUE;
}

void upb_dispatch_endmsg(upb_dispatcher *d, upb_status *status) {
  assert(d->top == d->stack);
  d->msgent->endmsg(d->top->closure, &d->status);
  // TODO: should we avoid this copy by passing client's status obj to cbs?
  upb_copyerr(status, &d->status);
}

upb_flow_t upb_dispatch_startsubmsg(upb_dispatcher *d,
                                    upb_dispatcher_field *f,
                                    size_t userval) {
  ++d->current_depth;
  if (upb_dispatcher_skipping(d)) return UPB_SKIPSUBMSG;
  upb_sflow_t sflow = f->cb.startsubmsg(d->top->closure, f->fval);
  if (sflow.flow != UPB_CONTINUE) {
    d->noframe_depth = d->current_depth;
    d->skip_depth = (sflow.flow == UPB_BREAK) ?
        d->delegated_depth : d->current_depth;
    return UPB_SKIPSUBMSG;
  }

  ++d->top;
  if(d->top >= d->limit) {
    upb_seterr(&d->status, UPB_ERROR, "Nesting too deep.");
    d->noframe_depth = d->current_depth;
    d->skip_depth = d->delegated_depth;
    return UPB_SKIPSUBMSG;
  }
  d->top->f = f;
  d->top->end_offset = userval;
  d->top->closure = sflow.closure;
  d->msgent = upb_handlers_getmsgent(d->handlers, f);
  d->dispatch_table = &d->msgent->fieldtab;
  return upb_dispatch_startmsg(d);
}

upb_flow_t upb_dispatch_endsubmsg(upb_dispatcher *d) {
  upb_flow_t flow;
  if (upb_dispatcher_noframe(d)) {
    flow = UPB_SKIPSUBMSG;
  } else {
    assert(d->top > d->stack);
    upb_dispatcher_field *old_f = d->top->f;
    d->msgent->endmsg(d->top->closure, &d->status);
    --d->top;
    d->msgent = upb_handlers_getmsgent(d->handlers, d->top->f);
    d->dispatch_table = &d->msgent->fieldtab;
    d->noframe_depth = INT_MAX;
    if (!upb_dispatcher_skipping(d)) d->skip_depth = INT_MAX;
    // Deliver like a regular value.
    flow = old_f->endsubmsg(d->top->closure, old_f->fval);
  }
  --d->current_depth;
  return flow;
}
