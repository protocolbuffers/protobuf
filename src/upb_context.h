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
#include "upb_atomic.h"

struct google_protobuf_FileDescriptorProto;

#ifdef __cplusplus
extern "C" {
#endif

/* Definitions. ***************************************************************/

struct upb_context {
  upb_atomic_refcount_t refcount;
  upb_rwlock_t lock;  // Protects all members except the refcount.
  struct upb_msgdef *fds_msgdef;  // In psymtab, ptr here for convenience.

  // Our symbol tables; we own refs to the defs therein.
  struct upb_strtable symtab;     // The context's symbol table.
  struct upb_strtable psymtab;    // Private symbols, for internal use.
};

// Initializes a upb_context.  Contexts are not freed explicitly, but unref'd
// when the caller is done with them.
struct upb_context *upb_context_new(void);
INLINE void upb_context_ref(struct upb_context *c) {
  upb_atomic_ref(&c->refcount);
}
void upb_context_unref(struct upb_context *c);

/* Looking up symbols. ********************************************************/

// Resolves the given symbol using the rules described in descriptor.proto,
// namely:
//
//    If the name starts with a '.', it is fully-qualified.  Otherwise, C++-like
//    scoping rules are used to find the type (i.e. first the nested types
//    within this message are searched, then within the parent, on up to the
//    root namespace).
//
// Returns NULL if no such symbol has been defined.
struct upb_def *upb_context_resolve(struct upb_context *c,
                                    struct upb_string *base,
                                    struct upb_string *symbol);

// Find an entry in the symbol table with this exact name.  Returns NULL if no
// such symbol name has been defined.
struct upb_def *upb_context_lookup(struct upb_context *c,
                                   struct upb_string *sym);

// Gets an array of pointers to all currently active defs in this context.  The
// caller owns the returned array (which is of length *count) as well as a ref
// to each symbol inside.
struct upb_def **upb_context_getandref_defs(struct upb_context *c, int *count);

/* Adding symbols. ************************************************************/

// Adds the definitions in the given file descriptor to this context.  All
// types that are referenced from fd must have previously been defined (or be
// defined in fd).  fd may not attempt to define any names that are already
// defined in this context.  Caller retains ownership of fd.  status indicates
// whether the operation was successful or not, and the error message (if any).
struct google_protobuf_FileDescriptorSet;
void upb_context_addfds(struct upb_context *c,
                        struct google_protobuf_FileDescriptorSet *fds,
                        struct upb_status *status);
// Like the above, but also parses the FileDescriptorSet from fds.
void upb_context_parsefds(struct upb_context *c, struct upb_string *fds,
                          struct upb_status *status);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_PARSE_H_ */
