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

#include "upb_handlers.h"

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
  upb_symtabtxn *txn;
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
void upb_descreader_init(upb_descreader *r, upb_symtabtxn *txn);
void upb_descreader_uninit(upb_descreader *r);

// Registers handlers that will load descriptor data into a symtabtxn.
// Pass the descreader as the closure.  The messages will have
// upb_msgdef_layout() called on them before adding to the txn.
upb_mhandlers *upb_descreader_reghandlers(upb_handlers *h);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
