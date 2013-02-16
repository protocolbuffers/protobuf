/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * upb_descreader provides a set of sink handlers that will build defs from a
 * data source that uses the descriptor.proto schema (like a protobuf binary
 * descriptor).
 */

#ifndef UPB_DESCRIPTOR_H
#define UPB_DESCRIPTOR_H

#include "upb/handlers.h"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_deflist ****************************************************************/

// upb_deflist is an internal-only dynamic array for storing a growing list of
// upb_defs.
typedef struct {
  upb_def **defs;
  size_t len;
  size_t size;
  bool owned;
} upb_deflist;

void upb_deflist_init(upb_deflist *l);
void upb_deflist_uninit(upb_deflist *l);
void upb_deflist_push(upb_deflist *l, upb_def *d);

/* upb_descreader  ************************************************************/

// We keep a stack of all the messages scopes we are currently in, as well as
// the top-level file scope.  This is necessary to correctly qualify the
// definitions that are contained inside.  "name" tracks the name of the
// message or package (a bare name -- not qualified by any enclosing scopes).
typedef struct {
  char *name;
  // Index of the first def that is under this scope.  For msgdefs, the
  // msgdef itself is at start-1.
  int start;
} upb_descreader_frame;

typedef struct {
  upb_deflist defs;
  upb_descreader_frame stack[UPB_MAX_TYPE_DEPTH];
  int stack_len;
  upb_status status;

  uint32_t number;
  char *name;
  bool saw_number;
  bool saw_name;

  char *default_string;

  upb_fielddef *f;
} upb_descreader;

void upb_descreader_init(upb_descreader *r);
void upb_descreader_uninit(upb_descreader *r);

// Registers handlers that will build the defs.  Pass the descreader as the
// closure.
const upb_handlers *upb_descreader_newhandlers(const void *owner);

// Gets the array of defs that have been parsed and removes them from the
// descreader.  Ownership of the defs is passed to the caller using the given
// owner), but the ownership of the returned array is retained and is
// invalidated by any other call into the descreader.  The defs will not have
// been resolved, and are ready to be added to a symtab.
upb_def **upb_descreader_getdefs(upb_descreader *r, void *owner, int *n);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
