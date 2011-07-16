/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Routines for building defs by parsing descriptors in descriptor.proto format.
 * This only needs to use the public API of upb_symtab.  Later we may also
 * add routines for dumping a symtab to a descriptor.
 */

#ifndef UPB_DESCRIPTOR_H
#define UPB_DESCRIPTOR_H

#include "upb/handlers.h"

#ifdef __cplusplus
extern "C" {
#endif


/* upb_descreader  ************************************************************/

// upb_descreader reads a descriptor and puts defs in a upb_symtabtxn.

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

// Creates a new descriptor builder that will add defs to the given txn.
void upb_descreader_init(upb_descreader *r);
void upb_descreader_uninit(upb_descreader *r);

// Registers handlers that will load descriptor data into a symtabtxn.
// Pass the descreader as the closure.  The messages will have
// upb_msgdef_layout() called on them before adding to the txn.
upb_mhandlers *upb_descreader_reghandlers(upb_handlers *h);

// Gets the array of defs that have been parsed and removes them from the
// descreader.  Ownership of the defs is passed to the caller, but the
// ownership of the returned array is retained and is invalidated by any other
// call into the descreader.  The defs will not have been resolved, and are
// ready to be added to a symtab.
upb_def **upb_descreader_getdefs(upb_descreader *r, int *n);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
