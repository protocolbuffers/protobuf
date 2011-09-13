/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009-2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Provides a mechanism for creating and linking proto definitions.
 * These form the protobuf schema, and are used extensively throughout upb:
 * - upb_msgdef: describes a "message" construct.
 * - upb_fielddef: describes a message field.
 * - upb_enumdef: describes an enum.
 * (TODO: definitions of services).
 *
 *
 * Defs go through two distinct phases of life:
 *
 * 1. MUTABLE: when first created, the properties of the def can be set freely
 *    (for example a message's name, its list of fields, the name/number of
 *    fields, etc).  During this phase the def is *not* thread-safe, and may
 *    not be used for any purpose except to set its properties (it can't be
 *    used to parse anything, create any messages in memory, etc).
 *
 * 2. FINALIZED: after being added to a symtab (which links the defs together)
 *    the defs become finalized (thread-safe and immutable).  Programs may only
 *    access defs through a CONST POINTER during this stage -- upb_symtab will
 *    help you out with this requirement by only vending const pointers, but
 *    you need to make sure not to use any non-const pointers you still have
 *    sitting around.  In practice this means that you may not call any setters
 *    on the defs (or functions that themselves call the setters).  If you want
 *    to modify an existing immutable def, copy it with upb_*_dup(), modify the
 *    copy, and add the modified def to the symtab (replacing the existing
 *    def).
 *
 * You can test for which stage of life a def is in by calling
 * upb_def_ismutable().  This is particularly useful for dynamic language
 * bindings, which must properly guarantee that the dynamic language cannot
 * break the rules laid out above.
 *
 * It would be possible to make the defs thread-safe during stage 1 by using
 * mutexes internally and changing any methods returning pointers to return
 * copies instead.  This could be important if we are integrating with a VM or
 * interpreter that does not naturally serialize access to wrapped objects (for
 * example, in the case of Python this is not necessary because of the GIL).
 */

#ifndef UPB_DEF_H_
#define UPB_DEF_H_

#include "upb/atomic.h"
#include "upb/table.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _upb_symtab;
typedef struct _upb_symtab upb_symtab;

// All the different kind of defs we support.  These correspond 1:1 with
// declarations in a .proto file.
typedef enum {
  UPB_DEF_MSG = 1,
  UPB_DEF_ENUM,
  UPB_DEF_SERVICE,          // Not yet implemented.

  UPB_DEF_ANY = -1,         // Wildcard for upb_symtab_get*()
  UPB_DEF_UNRESOLVED = 99,  // Internal-only.
} upb_deftype_t;


/* upb_def: base class for defs  **********************************************/

typedef struct {
  char *fqname;     // Fully qualified.
  upb_symtab *symtab;     // Def is mutable iff symtab == NULL.
  upb_atomic_t refcount;  // Owns a ref on symtab iff (symtab && refcount > 0).
  upb_deftype_t type;
} upb_def;

// Call to ref/unref a def.  Can be used at any time, but is not thread-safe
// until the def is in a symtab.  While a def is in a symtab, everything
// reachable from that def (the symtab and all defs in the symtab) are
// guaranteed to be alive.
void upb_def_ref(const upb_def *def);
void upb_def_unref(const upb_def *def);
upb_def *upb_def_dup(const upb_def *def);

// A def is mutable until it has been added to a symtab.
bool upb_def_ismutable(const upb_def *def);
INLINE const char *upb_def_fqname(const upb_def *def) { return def->fqname; }
bool upb_def_setfqname(upb_def *def, const char *fqname);  // Only if mutable.

#define UPB_UPCAST(ptr) (&(ptr)->base)


/* upb_fielddef ***************************************************************/

// A upb_fielddef describes a single field in a message.  It isn't a full def
// in the sense that it derives from upb_def.  It cannot stand on its own; it
// must be part of a upb_msgdef.  It is also reference-counted.
typedef struct _upb_fielddef {
  struct _upb_msgdef *msgdef;
  upb_def *def;  // if upb_hasdef(f)
  upb_atomic_t refcount;
  bool finalized;

  // The following fields may be modified until the def is finalized.
  uint8_t type;          // Use UPB_TYPE() constants.
  uint8_t label;         // Use UPB_LABEL() constants.
  int16_t hasbit;
  uint16_t offset;
  bool default_is_symbolic;
  bool active;
  int32_t number;
  char *name;
  upb_value defaultval;  // Only meaningful for non-repeated scalars and strings.
  upb_value fval;
  struct _upb_accessor_vtbl *accessor;
  const void *default_ptr;
  const void *prototype;
} upb_fielddef;

upb_fielddef *upb_fielddef_new(void);
void upb_fielddef_ref(upb_fielddef *f);
void upb_fielddef_unref(upb_fielddef *f);
upb_fielddef *upb_fielddef_dup(upb_fielddef *f);

// A fielddef is mutable until its msgdef has been added to a symtab.
bool upb_fielddef_ismutable(const upb_fielddef *f);

// Read accessors.  May be called any time.
INLINE uint8_t upb_fielddef_type(const upb_fielddef *f) { return f->type; }
INLINE uint8_t upb_fielddef_label(const upb_fielddef *f) { return f->label; }
INLINE int32_t upb_fielddef_number(const upb_fielddef *f) { return f->number; }
INLINE char *upb_fielddef_name(const upb_fielddef *f) { return f->name; }
INLINE upb_value upb_fielddef_fval(const upb_fielddef *f) { return f->fval; }
INLINE bool upb_fielddef_finalized(const upb_fielddef *f) { return f->finalized; }
INLINE struct _upb_msgdef *upb_fielddef_msgdef(const upb_fielddef *f) {
  return f->msgdef;
}
INLINE struct _upb_accessor_vtbl *upb_fielddef_accessor(const upb_fielddef *f) {
  return f->accessor;
}
INLINE const char *upb_fielddef_typename(const upb_fielddef *f) {
  return f->def ? f->def->fqname : NULL;
}

// Returns the default value for this fielddef, which may either be something
// the client set explicitly or the "default default" (0 for numbers, empty for
// strings).  The field's type indicates the type of the returned value, except
// for enums.   For enums the default can be set either numerically or
// symbolically -- the upb_fielddef_default_is_symbolic() function below will
// indicate which it is.  For string defaults, the value will be a upb_strref
// which is invalidated by any other call on this object.
INLINE upb_value upb_fielddef_default(const upb_fielddef *f) {
  return f->defaultval;
}

// The results of this function are only meaningful for enum fields, which can
// have a default specified either as an integer or as a string.  If this
// returns true, the default returned from upb_fielddef_default() is a string,
// otherwise it is an integer.
INLINE bool upb_fielddef_default_is_symbolic(const upb_fielddef *f) {
  return f->default_is_symbolic;
}

// The enum or submessage def for this field, if any.  Only meaningful for
// submessage, group, and enum fields (ie. when upb_hassubdef(f) is true).
// Since defs are not linked together until they are in a symtab, this
// will return NULL until the msgdef is in a symtab.
upb_def *upb_fielddef_subdef(const upb_fielddef *f);

// Write accessors.  "Number" and "name" must be set before the fielddef is
// added to a msgdef.  For the moment we do not allow these to be set once
// the fielddef is added to a msgdef -- this could be relaxed in the future.
bool upb_fielddef_setnumber(upb_fielddef *f, int32_t number);
bool upb_fielddef_setname(upb_fielddef *f, const char *name);

// These writers may be called at any time prior to being put in a symtab.
bool upb_fielddef_settype(upb_fielddef *f, uint8_t type);
bool upb_fielddef_setlabel(upb_fielddef *f, uint8_t label);
void upb_fielddef_setfval(upb_fielddef *f, upb_value fval);
void upb_fielddef_setaccessor(upb_fielddef *f, struct _upb_accessor_vtbl *vtbl);

// The name of the message or enum this field is referring to.  Must be found
// at name resolution time (when upb_symtab_add() is called).
//
// NOTE: May only be called for fields whose type has already been set to
// be a submessage, group, or enum!  Also, will be reset to empty if the
// field's type is set again.
bool upb_fielddef_settypename(upb_fielddef *f, const char *name);

// The default value for the field.  For numeric types, use
// upb_fielddef_setdefault(), and "value" must match the type of the field.
// For string/bytes types, use upb_fielddef_setdefaultstr().
// Enum types may use either, since the default may be set either numerically
// or symbolically.
//
// NOTE: May only be called for fields whose type has already been set.
// Also, will be reset to default if the field's type is set again.
void upb_fielddef_setdefault(upb_fielddef *f, upb_value value);
void upb_fielddef_setdefaultstr(upb_fielddef *f, const void *str, size_t len);
void upb_fielddef_setdefaultcstr(upb_fielddef *f, const char *str);

// A variety of tests about the type of a field.
INLINE bool upb_issubmsgtype(upb_fieldtype_t type) {
  return type == UPB_TYPE(GROUP) || type == UPB_TYPE(MESSAGE);
}
INLINE bool upb_isstringtype(upb_fieldtype_t type) {
  return type == UPB_TYPE(STRING) || type == UPB_TYPE(BYTES);
}
INLINE bool upb_isprimitivetype(upb_fieldtype_t type) {
  return !upb_issubmsgtype(type) && !upb_isstringtype(type);
}
INLINE bool upb_issubmsg(const upb_fielddef *f) { return upb_issubmsgtype(f->type); }
INLINE bool upb_isstring(const upb_fielddef *f) { return upb_isstringtype(f->type); }
INLINE bool upb_isseq(const upb_fielddef *f) { return f->label == UPB_LABEL(REPEATED); }

// Does the type of this field imply that it should contain an associated def?
INLINE bool upb_hassubdef(const upb_fielddef *f) {
  return upb_issubmsg(f) || f->type == UPB_TYPE(ENUM);
}


/* upb_msgdef *****************************************************************/

// Structure that describes a single .proto message type.
typedef struct _upb_msgdef {
  upb_def base;

  // Tables for looking up fields by number and name.
  upb_inttable itof;  // int to field
  upb_strtable ntof;  // name to field

  // The following fields may be modified until finalized.
  uint16_t size;
  uint8_t hasbit_bytes;
  // The range of tag numbers used to store extensions.
  uint32_t extstart, extend;
} upb_msgdef;

// Hash table entries for looking up fields by name or number.
typedef struct {
  bool junk;
  upb_fielddef *f;
} upb_itof_ent;
typedef struct {
  upb_fielddef *f;
} upb_ntof_ent;

upb_msgdef *upb_msgdef_new(void);
INLINE void upb_msgdef_unref(const upb_msgdef *md) { upb_def_unref(UPB_UPCAST(md)); }
INLINE void upb_msgdef_ref(const upb_msgdef *md) { upb_def_ref(UPB_UPCAST(md)); }

// Returns a new msgdef that is a copy of the given msgdef (and a copy of all
// the fields) but with any references to submessages broken and replaced with
// just the name of the submessage.  This can be put back into another symtab
// and the names will be re-resolved in the new context.
upb_msgdef *upb_msgdef_dup(const upb_msgdef *m);

// Read accessors.  May be called at any time.
INLINE size_t upb_msgdef_size(const upb_msgdef *m) { return m->size; }
INLINE uint8_t upb_msgdef_hasbit_bytes(const upb_msgdef *m) {
  return m->hasbit_bytes;
}
INLINE uint32_t upb_msgdef_extstart(const upb_msgdef *m) { return m->extstart; }
INLINE uint32_t upb_msgdef_extend(const upb_msgdef *m) { return m->extend; }

// Write accessors.  May only be called before the msgdef is in a symtab.
void upb_msgdef_setsize(upb_msgdef *m, uint16_t size);
void upb_msgdef_sethasbit_bytes(upb_msgdef *m, uint16_t bytes);
bool upb_msgdef_setextrange(upb_msgdef *m, uint32_t start, uint32_t end);

// Adds a set of fields (upb_fielddef objects) to a msgdef.  Caller retains its
// ref on the fielddef.  May only be done before the msgdef is in a symtab
// (requires upb_def_ismutable(m) for the msgdef).  The fielddef's name and
// number must be set, and the message may not already contain any field with
// this name or number, and this fielddef may not be part of another message,
// otherwise false is returned and no action is performed.
bool upb_msgdef_addfields(upb_msgdef *m, upb_fielddef **f, int n);
INLINE bool upb_msgdef_addfield(upb_msgdef *m, upb_fielddef *f) {
  return upb_msgdef_addfields(m, &f, 1);
}

// Sets the layout of all fields according to default rules:
// 1. Hasbits for required fields come first, then optional fields.
// 2. Values are laid out in a way that respects alignment rules.
// 3. The order is chosen to minimize memory usage.
// This should only be called once all fielddefs have been added.
// TODO: will likely want the ability to exclude strings/submessages/arrays.
// TODO: will likely want the ability to define a header size.
void upb_msgdef_layout(upb_msgdef *m);

// Looks up a field by name or number.  While these are written to be as fast
// as possible, it will still be faster to cache the results of this lookup if
// possible.  These return NULL if no such field is found.
INLINE upb_fielddef *upb_msgdef_itof(const upb_msgdef *m, uint32_t i) {
  upb_itof_ent *e = (upb_itof_ent*)
      upb_inttable_fastlookup(&m->itof, i, sizeof(upb_itof_ent));
  return e ? e->f : NULL;
}

INLINE upb_fielddef *upb_msgdef_ntof(const upb_msgdef *m, const char *name) {
  upb_ntof_ent *e = (upb_ntof_ent*)upb_strtable_lookup(&m->ntof, name);
  return e ? e->f : NULL;
}

INLINE int upb_msgdef_numfields(const upb_msgdef *m) {
  return upb_strtable_count(&m->ntof);
}

// Iteration over fields.  The order is undefined.
// TODO: the iteration should be in field order.
// Iterators are invalidated when a field is added or removed.
//   upb_msg_iter i;
//   for(i = upb_msg_begin(m); !upb_msg_done(i); i = upb_msg_next(m, i)) {
//     upb_fielddef *f = upb_msg_iter_field(i);
//     // ...
//   }
typedef upb_inttable_iter upb_msg_iter;

upb_msg_iter upb_msg_begin(const upb_msgdef *m);
upb_msg_iter upb_msg_next(const upb_msgdef *m, upb_msg_iter iter);
INLINE bool upb_msg_done(upb_msg_iter iter) { return upb_inttable_done(iter); }

// Iterator accessor.
INLINE upb_fielddef *upb_msg_iter_field(upb_msg_iter iter) {
  upb_itof_ent *ent = (upb_itof_ent*)upb_inttable_iter_value(iter);
  return ent->f;
}


/* upb_enumdef ****************************************************************/

typedef struct _upb_enumdef {
  upb_def base;
  upb_strtable ntoi;
  upb_inttable iton;
  int32_t defaultval;
} upb_enumdef;

typedef struct {
  uint32_t value;
} upb_ntoi_ent;

typedef struct {
  bool junk;
  char *str;
} upb_iton_ent;

upb_enumdef *upb_enumdef_new(void);
INLINE void upb_enumdef_ref(const upb_enumdef *e) { upb_def_ref(UPB_UPCAST(e)); }
INLINE void upb_enumdef_unref(const upb_enumdef *e) { upb_def_unref(UPB_UPCAST(e)); }
upb_enumdef *upb_enumdef_dup(const upb_enumdef *e);

INLINE int32_t upb_enumdef_default(upb_enumdef *e) { return e->defaultval; }

// May only be set if upb_def_ismutable(e).
void upb_enumdef_setdefault(upb_enumdef *e, int32_t val);

// Adds a value to the enumdef.  Requires that no existing val has this
// name or number (returns false and does not add if there is).  May only
// be called before the enumdef is in a symtab.
bool upb_enumdef_addval(upb_enumdef *e, char *name, int32_t num);

// Lookups from name to integer and vice-versa.
bool upb_enumdef_ntoil(upb_enumdef *e, const char *name, size_t len, int32_t *num);
bool upb_enumdef_ntoi(upb_enumdef *e, const char *name, int32_t *num);
// Caller does not own the returned string.
const char *upb_enumdef_iton(upb_enumdef *e, int32_t num);

// Iteration over name/value pairs.  The order is undefined.
// Adding an enum val invalidates any iterators.
//   upb_enum_iter i;
//   for(i = upb_enum_begin(e); !upb_enum_done(i); i = upb_enum_next(e, i)) {
//     // ...
//   }
typedef upb_inttable_iter upb_enum_iter;

upb_enum_iter upb_enum_begin(const upb_enumdef *e);
upb_enum_iter upb_enum_next(const upb_enumdef *e, upb_enum_iter iter);
INLINE bool upb_enum_done(upb_enum_iter iter) { return upb_inttable_done(iter); }

// Iterator accessors.
INLINE char *upb_enum_iter_name(upb_enum_iter iter) {
  upb_iton_ent *e = (upb_iton_ent*)upb_inttable_iter_value(iter);
  return e->str;
}
INLINE int32_t upb_enum_iter_number(upb_enum_iter iter) {
  return upb_inttable_iter_key(iter);
}


/* upb_deflist ****************************************************************/

// upb_deflist is an internal-only dynamic array for storing a growing list of
// upb_defs.
typedef struct {
  upb_def **defs;
  uint32_t len;
  uint32_t size;
} upb_deflist;

void upb_deflist_init(upb_deflist *l);
void upb_deflist_uninit(upb_deflist *l);
void upb_deflist_push(upb_deflist *l, upb_def *d);


/* upb_symtab *****************************************************************/

// A symtab (symbol table) is where upb_defs live.  It is empty when first
// constructed.  Clients add definitions to the symtab (or replace existing
// definitions) by calling upb_symtab_add().
struct _upb_symtab {
  upb_atomic_t refcount;
  upb_rwlock_t lock;       // Protects all members except the refcount.
  upb_strtable symtab;     // The symbol table.
  upb_deflist olddefs;
};

upb_symtab *upb_symtab_new(void);
void upb_symtab_ref(const upb_symtab *s);
void upb_symtab_unref(const upb_symtab *s);

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
const upb_def *upb_symtab_resolve(const upb_symtab *s, const char *base,
                                  const char *sym);

// Find an entry in the symbol table with this exact name.  If a def is found,
// the caller owns one ref on the returned def.  Otherwise returns NULL.
const upb_def *upb_symtab_lookup(const upb_symtab *s, const char *sym);
const upb_msgdef *upb_symtab_lookupmsg(const upb_symtab *s, const char *sym);

// Gets an array of pointers to all currently active defs in this symtab.  The
// caller owns the returned array (which is of length *count) as well as a ref
// to each symbol inside.  If type is UPB_DEF_ANY then defs of all types are
// returned, otherwise only defs of the required type are returned.
const upb_def **upb_symtab_getdefs(const upb_symtab *s, int *n, upb_deftype_t type);

// Adds the given defs to the symtab, resolving all symbols.  Only one def per
// name may be in the list, but defs can replace existing defs in the symtab.
// The entire operation either succeeds or fails.  If the operation fails, the
// symtab is unchanged, false is returned, and status indicates the error.  The
// caller retains its ref on all defs in all cases.
bool upb_symtab_add(upb_symtab *s, upb_def **defs, int n, upb_status *status);

// Frees defs that are no longer active in the symtab and are no longer
// reachable.  Such defs are not freed when they are replaced in the symtab
// if they are still reachable from defs that are still referenced.
void upb_symtab_gc(upb_symtab *s);


/* upb_def casts **************************************************************/

// Dynamic casts, for determining if a def is of a particular type at runtime.
// Downcasts, for when some wants to assert that a def is of a particular type.
// These are only checked if we are building debug.
#define UPB_DEF_CASTS(lower, upper) \
  struct _upb_ ## lower;  /* Forward-declare. */ \
  INLINE struct _upb_ ## lower *upb_dyncast_ ## lower(upb_def *def) { \
    if(def->type != UPB_DEF_ ## upper) return NULL; \
    return (struct _upb_ ## lower*)def; \
  } \
  INLINE const struct _upb_ ## lower *upb_dyncast_ ## lower ## _const(const upb_def *def) { \
    if(def->type != UPB_DEF_ ## upper) return NULL; \
    return (const struct _upb_ ## lower*)def; \
  } \
  INLINE struct _upb_ ## lower *upb_downcast_ ## lower(upb_def *def) { \
    assert(def->type == UPB_DEF_ ## upper); \
    return (struct _upb_ ## lower*)def; \
  } \
  INLINE const struct _upb_ ## lower *upb_downcast_ ## lower ## _const(const upb_def *def) { \
    assert(def->type == UPB_DEF_ ## upper); \
    return (const struct _upb_ ## lower*)def; \
  }
UPB_DEF_CASTS(msgdef, MSG);
UPB_DEF_CASTS(enumdef, ENUM);
UPB_DEF_CASTS(svcdef, SERVICE);
UPB_DEF_CASTS(unresolveddef, UNRESOLVED);
#undef UPB_DEF_CASTS

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DEF_H_ */
