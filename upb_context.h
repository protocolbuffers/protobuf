/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * A context represents a namespace of proto definitions, sort of like an
 * interpreter's symbol table.  It is empty when first constructed.  Clients
 * add definitions to the context by supplying unserialized or serialized
 * descriptors (as defined in descriptor.proto).
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_CONTEXT_H_
#define UPB_CONTEXT_H_

#include "upb.h"
#include "upb_table.h"

struct google_protobuf_FileDescriptorProto;

#ifdef __cplusplus
extern "C" {
#endif

/* Definitions. ***************************************************************/

/* The symbol table maps names to various kinds of symbols. */
enum upb_symbol_type {
  UPB_SYM_MESSAGE,
  UPB_SYM_ENUM,
  UPB_SYM_SERVICE,
  UPB_SYM_EXTENSION
};

struct upb_symtab_entry {
  struct upb_strtable_entry e;
  enum upb_symbol_type type;
  union upb_symbol_ref ref;
};

struct upb_context {
  struct upb_strtable symtab;   /* The context's symbol table. */
  struct upb_strtable psymtab;  /* Private symbols, for internal use. */
  struct upb_msg *fds_msg;   /* This is in psymtab, ptr here for convenience. */

  /* A list of the FileDescriptorProtos we own (from having parsed them
   * ourselves) and must free on destruction. */
  size_t fds_size, fds_len;
  struct google_protobuf_FileDescriptorSet **fds;
};

/* Initializes and frees a upb_context, respectively. */
bool upb_context_init(struct upb_context *c);
void upb_context_free(struct upb_context *c);

/* Looking up symbols. ********************************************************/

/* Resolves the given symbol using the rules described in descriptor.proto,
 * namely:
 *
 *    If the name starts with a '.', it is fully-qualified.  Otherwise, C++-like
 *    scoping rules are used to find the type (i.e. first the nested types
 *    within this message are searched, then within the parent, on up to the
 *    root namespace).
 *
 * Returns NULL if the symbol has not been defined. */
struct upb_symtab_entry *upb_context_resolve(struct upb_context *c,
                                             struct upb_string *base,
                                             struct upb_string *symbol);

/* Find an entry in the symbol table with this exact name.  Returns NULL if no
 * such symbol name exists. */
struct upb_symtab_entry *upb_context_lookup(struct upb_context *c,
                                            struct upb_string *symbol);

INLINE struct upb_symtab_entry *upb_context_symbegin(struct upb_context *c) {
  return upb_strtable_begin(&c->symtab);
}

INLINE struct upb_symtab_entry *upb_context_symnext(
    struct upb_context *c, struct upb_symtab_entry *cur) {
  return upb_strtable_next(&c->symtab, &cur->e);
}

/* Adding symbols. ************************************************************/

/* Adds the definitions in the given file descriptor to this context.  All
 * types that are referenced from fd must have previously been defined (or be
 * defined in fd).  fd may not attempt to define any names that are already
 * defined in this context.
 *
 * Caller retains ownership of fd, but the context will contain references to
 * it, so it must outlive the context.
 *
 * upb_context_addfd only returns true or false; it does not give any hint
 * about what happened in the case of failure.  This is because the descriptor
 * is expected to have been validated at the time it was parsed/generated. */
bool upb_context_addfd(struct upb_context *c,
                       struct google_protobuf_FileDescriptorProto *fd);

bool upb_context_parsefds(struct upb_context *c, struct upb_string *fds);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_PARSE_H_ */
