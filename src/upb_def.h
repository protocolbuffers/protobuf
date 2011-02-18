/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009-2011 Joshua Haberman.  See LICENSE for details.
 *
 * Provides a mechanism for loading proto definitions from descriptors, and
 * data structures to represent those definitions.  These form the protobuf
 * schema, and are used extensively throughout upb:
 * - upb_msgdef: describes a "message" construct.
 * - upb_fielddef: describes a message field.
 * - upb_enumdef: describes an enum.
 * (TODO: definitions of extensions and services).
 *
 * Defs are obtained from a upb_symtab object.  A upb_symtab is empty when
 * constructed, and definitions can be added by supplying descriptors.
 *
 * Defs are immutable and reference-counted.  Symbol tables reference any defs
 * that are the "current" definitions.  If an extension is loaded that adds a
 * field to an existing message, a new msgdef is constructed that includes the
 * new field and the old msgdef is unref'd.  The old msgdef will still be ref'd
 * by messages (if any) that were constructed with that msgdef.
 *
 * This file contains routines for creating and manipulating the definitions
 * themselves.  To create and manipulate actual messages, see upb_msg.h.
 */

#ifndef UPB_DEF_H_
#define UPB_DEF_H_

#include "upb_atomic.h"
#include "upb_stream.h"
#include "upb_table.h"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_def: base class for defs  **********************************************/

// All the different kind of defs we support.  These correspond 1:1 with
// declarations in a .proto file.
typedef enum {
  UPB_DEF_MSG = 0,
  UPB_DEF_ENUM,
  UPB_DEF_SVC,
  UPB_DEF_EXT,
  // Internal-only, placeholder for a def that hasn't be resolved yet.
  UPB_DEF_UNRESOLVED,

  // For specifying that defs of any type are requsted from getdefs.
  UPB_DEF_ANY = -1
} upb_deftype;

// This typedef is more space-efficient than declaring an enum var directly.
typedef int8_t upb_deftype_t;

typedef struct {
  upb_string *fqname;  // Fully qualified.
  upb_atomic_refcount_t refcount;
  upb_deftype_t type;

  // The is_cyclic flag could go in upb_msgdef instead of here, because only
  // messages can be involved in cycles.  However, putting them here is free
  // from a space perspective because structure alignment will otherwise leave
  // three bytes empty after type.  It is also makes ref and unref more
  // efficient, because we don't have to downcast to msgdef before checking the
  // is_cyclic flag.
  bool is_cyclic;
  uint16_t search_depth;  // Used during initialization dfs.
} upb_def;

// These must not be called directly!
void _upb_def_cyclic_ref(upb_def *def);
void _upb_def_reftozero(upb_def *def);

// Call to ref/deref a def.
INLINE void upb_def_ref(upb_def *def) {
  if(upb_atomic_ref(&def->refcount) && def->is_cyclic) _upb_def_cyclic_ref(def);
}
INLINE void upb_def_unref(upb_def *def) {
  if(def && upb_atomic_unref(&def->refcount)) _upb_def_reftozero(def);
}

/* upb_fielddef ***************************************************************/

// A upb_fielddef describes a single field in a message.  It isn't a full def
// in the sense that it derives from upb_def.  It cannot stand on its own; it
// is either a field of a upb_msgdef or contained inside a upb_extensiondef.
// It is also reference-counted.
typedef struct _upb_fielddef {
  uint8_t type;
  uint8_t label;
  // True if we own a ref on "def" (above).  This is true unless this edge is
  // part of a cycle.
  bool owned;
  uint8_t set_bit_mask;

  int32_t number;
  int16_t field_index;  // Indicates set bit.

  uint16_t set_bit_offset;
  uint32_t byte_offset;           // Where in a upb_msg to find the data.

  upb_value default_value;
  upb_string *name;
  struct _upb_msgdef *msgdef;

  // For the case of an enum or a submessage, points to the def for that type.
  upb_def *def;
  upb_atomic_refcount_t refcount;
} upb_fielddef;

// A variety of tests about the type of a field.
INLINE bool upb_issubmsg(upb_fielddef *f) {
  return f->type == UPB_TYPE(GROUP) || f->type == UPB_TYPE(MESSAGE);
}
INLINE bool upb_isstring(upb_fielddef *f) {
  return f->type == UPB_TYPE(STRING) || f->type == UPB_TYPE(BYTES);
}
INLINE bool upb_isarray(upb_fielddef *f) {
  return f->label == UPB_LABEL(REPEATED);
}
// Does the type of this field imply that it should contain an associated def?
INLINE bool upb_hasdef(upb_fielddef *f) {
  return upb_issubmsg(f) || f->type == UPB_TYPE(ENUM);
}

INLINE upb_valuetype_t upb_field_valuetype(upb_fielddef *f) {
  if (upb_isarray(f)) {
    return UPB_VALUETYPE_ARRAY;
  } else {
    return f->type;
  }
}

INLINE upb_valuetype_t upb_elem_valuetype(upb_fielddef *f) {
  assert(upb_isarray(f));
  return f->type;
}

INLINE bool upb_field_ismm(upb_fielddef *f) {
  return upb_isarray(f) || upb_isstring(f) || upb_issubmsg(f);
}

INLINE bool upb_elem_ismm(upb_fielddef *f) {
  return upb_isstring(f) || upb_issubmsg(f);
}

/* upb_msgdef *****************************************************************/

// Structure that describes a single .proto message type.
typedef struct _upb_msgdef {
  upb_def base;
  upb_atomic_refcount_t cycle_refcount;
  uint32_t size;
  uint32_t set_flags_bytes;

  // Tables for looking up fields by number and name.
  upb_inttable itof;  // int to field
  upb_strtable ntof;  // name to field
} upb_msgdef;

// Hash table entries for looking up fields by name or number.
typedef struct {
  bool junk;
  upb_wire_type_t native_wire_type;
  upb_fieldtype_t field_type;
  upb_fielddef *f;
} upb_itof_ent;
typedef struct {
  upb_strtable_entry e;
  upb_fielddef *f;
} upb_ntof_ent;

// Looks up a field by name or number.  While these are written to be as fast
// as possible, it will still be faster to cache the results of this lookup if
// possible.  These return NULL if no such field is found.
INLINE upb_itof_ent *upb_msgdef_itofent(upb_msgdef *m, uint32_t num) {
  return (upb_itof_ent*)upb_inttable_fastlookup(
      &m->itof, num, sizeof(upb_itof_ent));
}

INLINE upb_fielddef *upb_msgdef_itof(upb_msgdef *m, uint32_t num) {
  upb_itof_ent *e = upb_msgdef_itofent(m, num);
  return e ? e->f : NULL;
}

INLINE upb_fielddef *upb_msgdef_ntof(upb_msgdef *m, upb_string *name) {
  upb_ntof_ent *e = (upb_ntof_ent*)upb_strtable_lookup(&m->ntof, name);
  return e ? e->f : NULL;
}

INLINE upb_field_count_t upb_msgdef_numfields(upb_msgdef *m) {
  return upb_strtable_count(&m->ntof);
}

// Iteration over fields.  The order is undefined.
//   upb_msg_iter i;
//   for(i = upb_msg_begin(m); !upb_msg_done(i); i = upb_msg_next(m, i)) {
//     upb_fielddef *f = upb_msg_iter_field(i);
//     // ...
//   }
typedef upb_inttable_iter upb_msg_iter;

upb_msg_iter upb_msg_begin(upb_msgdef *m);
upb_msg_iter upb_msg_next(upb_msgdef *m, upb_msg_iter iter);
INLINE bool upb_msg_done(upb_msg_iter iter) { return upb_inttable_done(iter); }

INLINE upb_fielddef *upb_msg_iter_field(upb_msg_iter iter) {
  upb_itof_ent *ent = (upb_itof_ent*)upb_inttable_iter_value(iter);
  return ent->f;
}

/* upb_enumdef ****************************************************************/

typedef struct _upb_enumdef {
  upb_def base;
  upb_strtable ntoi;
  upb_inttable iton;
} upb_enumdef;

typedef struct {
  upb_strtable_entry e;
  uint32_t value;
} upb_ntoi_ent;

typedef struct {
  bool junk;
  upb_string *string;
} upb_iton_ent;

typedef int32_t upb_enumval_t;

// Lookups from name to integer and vice-versa.
bool upb_enumdef_ntoi(upb_enumdef *e, upb_string *name, upb_enumval_t *num);
// Caller does not own a ref on the returned string.
upb_string *upb_enumdef_iton(upb_enumdef *e, upb_enumval_t num);

// Iteration over name/value pairs.  The order is undefined.
//   upb_enum_iter i;
//   for(i = upb_enum_begin(e); !upb_enum_done(i); i = upb_enum_next(e, i)) {
//     // ...
//   }
typedef upb_inttable_iter upb_enum_iter;

upb_enum_iter upb_enum_begin(upb_enumdef *e);
upb_enum_iter upb_enum_next(upb_enumdef *e, upb_enum_iter iter);
INLINE bool upb_enum_done(upb_enum_iter iter) { return upb_inttable_done(iter); }

INLINE upb_string *upb_enum_iter_name(upb_enum_iter iter) {
  upb_iton_ent *e = (upb_iton_ent*)upb_inttable_iter_value(iter);
  return e->string;
}
INLINE int32_t upb_enum_iter_number(upb_enum_iter iter) {
  return upb_inttable_iter_key(iter);
}


/* upb_symtab *****************************************************************/

// A SymbolTable is where upb_defs live.  It is empty when first constructed.
// Clients add definitions to the symtab by supplying unserialized or
// serialized descriptors (as defined in descriptor.proto).
struct _upb_symtab {
  upb_atomic_refcount_t refcount;
  upb_rwlock_t lock;       // Protects all members except the refcount.
  upb_strtable symtab;     // The symbol table.
  upb_msgdef *fds_msgdef;  // Msgdef for google.protobuf.FileDescriptorSet.
};
typedef struct _upb_symtab upb_symtab;

// Initializes a upb_symtab.  Contexts are not freed explicitly, but unref'd
// when the caller is done with them.
upb_symtab *upb_symtab_new(void);
void _upb_symtab_free(upb_symtab *s);  // Must not be called directly!

INLINE void upb_symtab_ref(upb_symtab *s) { upb_atomic_ref(&s->refcount); }
INLINE void upb_symtab_unref(upb_symtab *s) {
  if(upb_atomic_unref(&s->refcount)) _upb_symtab_free(s);
}

// Resolves the given symbol using the rules described in descriptor.proto,
// namely:
//
//    If the name starts with a '.', it is fully-qualified.  Otherwise, C++-like
//    scoping rules are used to find the type (i.e. first the nested types
//    within this message are searched, then within the parent, on up to the
//    root namespace).
//
// If a def is found, the caller owns one ref on the returned def.  Otherwise
// returns NULL.
upb_def *upb_symtab_resolve(upb_symtab *s, upb_string *base, upb_string *sym);

// Find an entry in the symbol table with this exact name.  If a def is found,
// the caller owns one ref on the returned def.  Otherwise returns NULL.
upb_def *upb_symtab_lookup(upb_symtab *s, upb_string *sym);

// Gets an array of pointers to all currently active defs in this symtab.  The
// caller owns the returned array (which is of length *count) as well as a ref
// to each symbol inside.  If type is UPB_DEF_ANY then defs of all types are
// returned, otherwise only defs of the required type are returned.
upb_def **upb_symtab_getdefs(upb_symtab *s, int *count, upb_deftype_t type);

// "fds" is a upb_src that will yield data from the
// google.protobuf.FileDescriptorSet message type.  It is not necessary that
// the upb_def for FileDescriptorSet came from this symtab, but it must be
// compatible with the official descriptor.proto, as published by Google.
//
// upb_symtab_addfds() adds all the definitions from the given
// FileDescriptorSet and adds them to the symtab.  status indicates whether the
// operation was successful or not, and the error message (if any).
//
// TODO: should this allow redefinition?  Either is possible, but which is
// more useful?  Maybe it should be an option.
void upb_symtab_addfds(upb_symtab *s, upb_src *desc, upb_status *status);

// Adds defs for google.protobuf.FileDescriptorSet and friends to this symtab.
// This is necessary for bootstrapping, since these are the upb_defs that
// specify other defs and allow them to be loaded.
void upb_symtab_add_descriptorproto(upb_symtab *s);

// Returns the upb_msgdef for google.protobuf.FileDescriptorSet, which the
// caller owns a ref on.  This is a convenience method that is equivalent to
// looking up the symbol called "google.protobuf.FileDescriptorSet" yourself,
// except that it only will return a def that was added by
// upb_symtab_add_descriptorproto().
upb_msgdef *upb_symtab_fds_def(upb_symtab *s);


/* upb_def casts **************************************************************/

// Dynamic casts, for determining if a def is of a particular type at runtime.
#define UPB_DYNAMIC_CAST_DEF(lower, upper) \
  struct _upb_ ## lower;  /* Forward-declare. */ \
  INLINE struct _upb_ ## lower *upb_dyncast_ ## lower(upb_def *def) { \
    if(def->type != UPB_DEF_ ## upper) return NULL; \
    return (struct _upb_ ## lower*)def; \
  }
UPB_DYNAMIC_CAST_DEF(msgdef, MSG);
UPB_DYNAMIC_CAST_DEF(enumdef, ENUM);
UPB_DYNAMIC_CAST_DEF(svcdef, SVC);
UPB_DYNAMIC_CAST_DEF(extdef, EXT);
UPB_DYNAMIC_CAST_DEF(unresolveddef, UNRESOLVED);
#undef UPB_DYNAMIC_CAST_DEF

// Downcasts, for when some wants to assert that a def is of a particular type.
// These are only checked if we are building debug.
#define UPB_DOWNCAST_DEF(lower, upper) \
  struct _upb_ ## lower;  /* Forward-declare. */ \
  INLINE struct _upb_ ## lower *upb_downcast_ ## lower(upb_def *def) { \
    assert(def->type == UPB_DEF_ ## upper); \
    return (struct _upb_ ## lower*)def; \
  }
UPB_DOWNCAST_DEF(msgdef, MSG);
UPB_DOWNCAST_DEF(enumdef, ENUM);
UPB_DOWNCAST_DEF(svcdef, SVC);
UPB_DOWNCAST_DEF(extdef, EXT);
UPB_DOWNCAST_DEF(unresolveddef, UNRESOLVED);
#undef UPB_DOWNCAST_DEF

#define UPB_UPCAST(ptr) (&(ptr)->base)

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DEF_H_ */
