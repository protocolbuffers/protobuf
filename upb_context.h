/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * A context represents a namespace of proto definitions, sort of like
 * an interpreter's symbol table.  It is empty when first constructed.
 * Clients add definitions to the context by supplying serialized
 * descriptors (as defined in descriptor.proto).
 *
 * Copyright (c) 2008 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_PARSE_H_
#define UPB_PARSE_H_

#include "upb.h"
#include "upb_table.h"

#ifdef __cplusplus
extern "C" {
#endif

/* High-level parsing interface. **********************************************/

enum upb_symbol_type {
  UPB_SYM_MESSAGE,
  UPB_SYM_ENUM,
  UPB_SYM_SERVICE,
  UPB_SYM_EXTENSION
};

struct upb_symtab_entry {
  struct upb_strtable_entry e;
  enum upb_symbol_type type;
  union {
    struct upb_msg *msg;
    struct upb_enum *_enum;
    struct upb_svc *svc;
  } p;
}

struct upb_context {
  upb_strtable *symtab;
};

struct upb_symtab_entry *upb_context_findsym(struct upb_context *c, struct upb_string *s);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_PARSE_H_ */
