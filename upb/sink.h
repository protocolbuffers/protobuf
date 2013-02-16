/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * A upb_sink is an object that binds a upb_handlers object to some runtime
 * state.  It is the object that can actually receive data via the upb_handlers
 * interface.
 *
 * Unlike upb_def and upb_handlers, upb_sink is never frozen, immutable, or
 * thread-safe.  You can create as many of them as you want, but each one may
 * only be used in a single thread at a time.
 *
 * If we compare with class-based OOP, a you can think of a upb_def as an
 * abstract base class, a upb_handlers as a concrete derived class, and a
 * upb_sink as an object (class instance).
 */

#ifndef UPB_SINK_H
#define UPB_SINK_H

#include "upb/handlers.h"

#ifdef __cplusplus
extern "C" {
#endif


/* upb_sink *******************************************************************/

typedef struct {
  upb_selector_t end;  // From the enclosing message (unused at top-level).
  const upb_handlers *h;
  void *closure;
} upb_sink_frame;

typedef struct {
  upb_sink_frame *top, *limit;
  upb_sink_frame stack[UPB_MAX_NESTING];
  upb_status status;
} upb_sink;

// Caller retains ownership of the handlers object.
void upb_sink_init(upb_sink *s, const upb_handlers *h);

// Resets the state of the sink so that it is ready to accept new input.
// Any state from previously received data is discarded.  "Closure" will be
// used as the top-level closure.
void upb_sink_reset(upb_sink *s, void *closure);

void upb_sink_uninit(upb_sink *s);

// Returns the handlers at the top of the stack.
const upb_handlers *upb_sink_tophandlers(upb_sink *s);

// Functions for pushing data into the sink.
// These return false if processing should stop (either due to error or just
// to suspend).
bool upb_sink_startmsg(upb_sink *s);
void upb_sink_endmsg(upb_sink *s, upb_status *status);
bool upb_sink_putint32(upb_sink *s, const upb_fielddef *f, int32_t val);
bool upb_sink_putint64(upb_sink *s, const upb_fielddef *f, int64_t val);
bool upb_sink_putuint32(upb_sink *s, const upb_fielddef *f, uint32_t val);
bool upb_sink_putuint64(upb_sink *s, const upb_fielddef *f, uint64_t val);
bool upb_sink_putfloat(upb_sink *s, const upb_fielddef *f, float val);
bool upb_sink_putdouble(upb_sink *s, const upb_fielddef *f, double val);
bool upb_sink_putbool(upb_sink *s, const upb_fielddef *f, bool val);
bool upb_sink_startstr(upb_sink *s, const upb_fielddef *f, size_t size_hint);
size_t upb_sink_putstring(upb_sink *s, const upb_fielddef *f, const char *buf,
                          size_t len);
bool upb_sink_endstr(upb_sink *s, const upb_fielddef *f);
bool upb_sink_startsubmsg(upb_sink *s, const upb_fielddef *f);
bool upb_sink_endsubmsg(upb_sink *s, const upb_fielddef *f);
bool upb_sink_startseq(upb_sink *s, const upb_fielddef *f);
bool upb_sink_endseq(upb_sink *s, const upb_fielddef *f);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
