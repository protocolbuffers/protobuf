/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * A context represents a namespace of proto definitions, sort of like an
 * interpreter's symbol table.  It is mostly empty when first constructed.
 * Clients add definitions to the context by supplying unserialized or
 * serialized descriptors (as defined in descriptor.proto).
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_CONTEXT_H_
#define UPB_CONTEXT_H_

#include "upb.h"
#include "upb_table.h"

#ifdef __cplusplus
extern "C" {
#endif

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
};

struct upb_context {
  struct upb_strtable symtab;
};

/* Initializes and frees a upb_context, respectively.  Newly initialized
 * contexts will always have the types in descriptor.proto defined. */
bool upb_context_init(struct upb_context *c);
void upb_context_free(struct upb_context *c);

/* Looking up symbols. ********************************************************/

/* Nested type names are separated by periods. */
#define UPB_CONTEXT_SEPARATOR '.'
#define UPB_SYM_MAX_LENGTH 256

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

/* TODO: let the client enumerate the symbols. */

/* Adding symbols. ************************************************************/

/* Enum controlling what happens if a symbol is redefined. */
enum upb_onredef {
  UPB_ONREDEF_REPLACE, /* Replace existing definition (must be same type). */
  UPB_ONREDEF_KEEP,    /* Keep existing definition, ignore new one. */
  UPB_ONREDEF_ERROR    /* Error on redefinition. */
};

/* Adds the definitions in the given file descriptor to this context.  All
 * types that are referenced from fd must have previously been defined (or be
 * defined in fd).  onredef controls the behavior in the case that fd attempts
 * to define a type that is already defined.
 *
 * Caller retains ownership of fd, but the context will contain references to
 * it, so it must outlive the context. */
bool upb_context_addfd(struct upb_context *c,
                       google_protobuf_FileDescriptorProto *fd,
                       int onredef);

/* Adds the serialized FileDescriptorSet proto contained in fdss to the context,
 * and adds symbol table entries for all the objects defined therein.  onredef
 * controls the behavior in the case the fds attempts to define a type that is
 * already defined.
 *
 * Returns true if the protobuf in fds was parsed successfully and all
 * references were successfully resolved.  If false is returned, the context is
 * unmodified.  */
bool upb_context_parsefds(struct upb_context *c, struct upb_string *fds,
                          int onredef);

/* Like the previous, but for a single FileDescriptorProto instead of a set. */
bool upb_context_parsefd(struct upb_context *c, struct upb_string *fds,
                         int onredef);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_PARSE_H_ */
