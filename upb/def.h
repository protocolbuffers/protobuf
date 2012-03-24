/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Defs are upb's internal representation of the constructs that can appear
 * in a .proto file:
 *
 * - upb_msgdef: describes a "message" construct.
 * - upb_fielddef: describes a message field.
 * - upb_enumdef: describes an enum.
 * (TODO: definitions of services).
 *
 * Defs go through two distinct phases of life:
 *
 * 1. MUTABLE: when first created, the properties of the def can be set freely
 *    (for example a message's name, its list of fields, the name/number of
 *    fields, etc).  During this phase the def is *not* thread-safe, and may
 *    not be used for any purpose except to set its properties (it can't be
 *    used to parse anything, create any messages in memory, etc).
 *
 * 2. FINALIZED: the upb_def_finalize() operation finalizes a set of defs,
 *    which makes them thread-safe and immutable.  Finalized defs may only be
 *    accessed through a CONST POINTER.  If you want to modify an existing
 *    immutable def, copy it with upb_*_dup() and modify and finalize the copy.
 *
 * The refcounting of defs works properly no matter what state the def is in.
 * Once the def is finalized it is guaranteed that any def reachable from a
 * live def is also live (so a ref on the base of a message tree keeps the
 * whole tree alive).
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

#include "upb/refcount.h"
#include "upb/table.h"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_def: base class for defs  **********************************************/

// All the different kind of defs we support.  These correspond 1:1 with
// declarations in a .proto file.
typedef enum {
  UPB_DEF_MSG,
  UPB_DEF_FIELD,
  UPB_DEF_ENUM,
  UPB_DEF_SERVICE,          // Not yet implemented.

  UPB_DEF_ANY = -1,         // Wildcard for upb_symtab_get*()
} upb_deftype_t;

typedef struct _upb_def {
  upb_refcount refcount;
  char *fullname;
  upb_deftype_t type;
  bool is_finalized;
} upb_def;

#define UPB_UPCAST(ptr) (&(ptr)->base)

// Call to ref/unref a def.  Can be used at any time, but is not thread-safe
// until the def is finalized.  While a def is finalized, everything reachable
// from that def is guaranteed to be alive.
void upb_def_ref(const upb_def *def, void *owner);
void upb_def_unref(const upb_def *def, void *owner);
void upb_def_donateref(const upb_def *def, void *from, void *to);
upb_def *upb_def_dup(const upb_def *def, void *owner);

// A def is mutable until it has been finalized.
bool upb_def_ismutable(const upb_def *def);
bool upb_def_isfinalized(const upb_def *def);

// "fullname" is the def's fully-qualified name (eg. foo.bar.Message).
INLINE const char *upb_def_fullname(const upb_def *d) { return d->fullname; }

// The def must be mutable.  Caller retains ownership of fullname.  Defs are
// not required to have a name; if a def has no name when it is finalized, it
// will remain an anonymous def.
bool upb_def_setfullname(upb_def *def, const char *fullname);

// Finalizes the given defs; this validates all constraints and marks the defs
// as finalized (read-only).  This will also cause fielddefs to take refs on
// their subdefs so that any reachable def will be kept alive (but this is
// done in a way that correctly handles circular references).
//
// On success, a new list is returned containing the finalized defs and
// ownership of the "defs" list passes to the function.  On failure NULL is
// returned and the caller retains ownership of "defs."
//
// Symbolic references to sub-types or enum defaults must have already been
// resolved.  "defs" must contain the transitive closure of any mutable defs
// reachable from the any def in the list.  In other words, there may not be a
// mutable def which is reachable from one of "defs" that does not appear
// elsewhere in "defs."  "defs" may not contain fielddefs, but any fielddefs
// reachable from the given msgdefs will be finalized.
//
// n is currently limited to 64k defs, if more are required break them into
// batches of 64k (or we could raise this limit, at the cost of a bigger
// upb_def structure or complexity in upb_finalize()).
bool upb_finalize(upb_def *const*defs, int n, upb_status *status);


/* upb_fielddef ***************************************************************/

// We choose these to match descriptor.proto.  Clients may use UPB_TYPE() and
// UPB_LABEL() instead of referencing these directly.
typedef enum {
  UPB_TYPE_NONE     = -1,  // Internal-only, may be removed.
  UPB_TYPE_ENDGROUP = 0,   // Internal-only, may be removed.
  UPB_TYPE_DOUBLE   = 1,
  UPB_TYPE_FLOAT    = 2,
  UPB_TYPE_INT64    = 3,
  UPB_TYPE_UINT64   = 4,
  UPB_TYPE_INT32    = 5,
  UPB_TYPE_FIXED64  = 6,
  UPB_TYPE_FIXED32  = 7,
  UPB_TYPE_BOOL     = 8,
  UPB_TYPE_STRING   = 9,
  UPB_TYPE_GROUP    = 10,
  UPB_TYPE_MESSAGE  = 11,
  UPB_TYPE_BYTES    = 12,
  UPB_TYPE_UINT32   = 13,
  UPB_TYPE_ENUM     = 14,
  UPB_TYPE_SFIXED32 = 15,
  UPB_TYPE_SFIXED64 = 16,
  UPB_TYPE_SINT32   = 17,
  UPB_TYPE_SINT64   = 18,
} upb_fieldtype_t;

#define UPB_NUM_TYPES 19

typedef enum {
  UPB_LABEL_OPTIONAL = 1,
  UPB_LABEL_REQUIRED = 2,
  UPB_LABEL_REPEATED = 3,
} upb_label_t;

// These macros are provided for legacy reasons.
#define UPB_TYPE(type) UPB_TYPE_ ## type
#define UPB_LABEL(type) UPB_LABEL_ ## type

// Info for a given field type.
typedef struct {
  uint8_t align;
  uint8_t size;
  uint8_t inmemory_type;    // For example, INT32, SINT32, and SFIXED32 -> INT32
} upb_typeinfo;

extern const upb_typeinfo upb_types[UPB_NUM_TYPES];

// A upb_fielddef describes a single field in a message.  It is most often
// found as a part of a upb_msgdef, but can also stand alone to represent
// an extension.
typedef struct _upb_fielddef {
  upb_def base;
  struct _upb_msgdef *msgdef;
  union {
    char *name;    // If subdef_is_symbolic.
    upb_def *def;  // If !subdef_is_symbolic.
  } sub;  // The msgdef or enumdef for this field, if upb_hassubdef(f).
  bool subdef_is_symbolic;
  bool default_is_string;
  bool subdef_is_owned;
  upb_fieldtype_t type;
  upb_label_t label;
  int16_t hasbit;
  uint16_t offset;
  int32_t number;
  upb_value defaultval;  // Only for non-repeated scalars and strings.
  upb_value fval;
  struct _upb_accessor_vtbl *accessor;
  const void *prototype;
} upb_fielddef;

// Returns NULL if memory allocation failed.
upb_fielddef *upb_fielddef_new(void *owner);

INLINE void upb_fielddef_ref(upb_fielddef *f, void *owner) {
  upb_def_ref(UPB_UPCAST(f), owner);
}
INLINE void upb_fielddef_unref(upb_fielddef *f, void *owner) {
  upb_def_unref(UPB_UPCAST(f), owner);
}

// Duplicates the given field, returning NULL if memory allocation failed.
// When a fielddef is duplicated, the subdef (if any) is made symbolic if it
// wasn't already.  If the subdef is set but has no name (which is possible
// since msgdefs are not required to have a name) the new fielddef's subdef
// will be unset.
upb_fielddef *upb_fielddef_dup(const upb_fielddef *f, void *owner);

INLINE bool upb_fielddef_ismutable(const upb_fielddef *f) {
  return upb_def_ismutable(UPB_UPCAST(f));
}
INLINE bool upb_fielddef_isfinalized(const upb_fielddef *f) {
  return !upb_fielddef_ismutable(f);
}

// Simple accessors. ///////////////////////////////////////////////////////////

INLINE upb_fieldtype_t upb_fielddef_type(const upb_fielddef *f) {
  return f->type;
}
INLINE upb_label_t upb_fielddef_label(const upb_fielddef *f) {
  return f->label;
}
INLINE int32_t upb_fielddef_number(const upb_fielddef *f) { return f->number; }
INLINE uint16_t upb_fielddef_offset(const upb_fielddef *f) { return f->offset; }
INLINE int16_t upb_fielddef_hasbit(const upb_fielddef *f) { return f->hasbit; }
INLINE const char *upb_fielddef_name(const upb_fielddef *f) {
  return upb_def_fullname(UPB_UPCAST(f));
}
INLINE upb_value upb_fielddef_fval(const upb_fielddef *f) { return f->fval; }
INLINE struct _upb_msgdef *upb_fielddef_msgdef(const upb_fielddef *f) {
  return f->msgdef;
}
INLINE struct _upb_accessor_vtbl *upb_fielddef_accessor(const upb_fielddef *f) {
  return f->accessor;
}

bool upb_fielddef_settype(upb_fielddef *f, upb_fieldtype_t type);
bool upb_fielddef_setlabel(upb_fielddef *f, upb_label_t label);
void upb_fielddef_sethasbit(upb_fielddef *f, int16_t hasbit);
void upb_fielddef_setoffset(upb_fielddef *f, uint16_t offset);
// TODO(haberman): need a way of keeping the fval alive even if some handlers
// outlast the fielddef.
void upb_fielddef_setfval(upb_fielddef *f, upb_value fval);
void upb_fielddef_setaccessor(upb_fielddef *f, struct _upb_accessor_vtbl *vtbl);

// "Number" and "fullname" must be set before the fielddef is added to a msgdef.
// For the moment we do not allow these to be set once the fielddef is added to
// a msgdef -- this could be relaxed in the future.
bool upb_fielddef_setnumber(upb_fielddef *f, int32_t number);
INLINE bool upb_fielddef_setname(upb_fielddef *f, const char *name) {
  return upb_def_setfullname(UPB_UPCAST(f), name);
}

// Field type tests. ///////////////////////////////////////////////////////////

INLINE bool upb_issubmsgtype(upb_fieldtype_t type) {
  return type == UPB_TYPE(GROUP) || type == UPB_TYPE(MESSAGE);
}
INLINE bool upb_isstringtype(upb_fieldtype_t type) {
  return type == UPB_TYPE(STRING) || type == UPB_TYPE(BYTES);
}
INLINE bool upb_isprimitivetype(upb_fieldtype_t type) {
  return !upb_issubmsgtype(type) && !upb_isstringtype(type);
}
INLINE bool upb_issubmsg(const upb_fielddef *f) {
  return upb_issubmsgtype(f->type);
}
INLINE bool upb_isstring(const upb_fielddef *f) {
  return upb_isstringtype(f->type);
}
INLINE bool upb_isseq(const upb_fielddef *f) {
  return f->label == UPB_LABEL(REPEATED);
}

// Default value. //////////////////////////////////////////////////////////////

// Returns the default value for this fielddef, which may either be something
// the client set explicitly or the "default default" (0 for numbers, empty for
// strings).  The field's type indicates the type of the returned value, except
// for enum fields that are still mutable.
//
// For enums the default can be set either numerically or symbolically -- the
// upb_fielddef_default_is_symbolic() function below will indicate which it is.
// For string defaults, the value will be a upb_byteregion which is invalidated
// by any other non-const call on this object.  Once the fielddef is finalized,
// symbolic enum defaults are resolved, so finalized enum fielddefs always have
// a default of type int32.
INLINE upb_value upb_fielddef_default(const upb_fielddef *f) {
  return f->defaultval;
}
// Sets default value for the field.  For numeric types, use
// upb_fielddef_setdefault(), and "value" must match the type of the field.
// For string/bytes types, use upb_fielddef_setdefaultstr().  Enum types may
// use either, since the default may be set either numerically or symbolically.
//
// NOTE: May only be called for fields whose type has already been set.
// Also, will be reset to default if the field's type is set again.
void upb_fielddef_setdefault(upb_fielddef *f, upb_value value);
bool upb_fielddef_setdefaultstr(upb_fielddef *f, const void *str, size_t len);
void upb_fielddef_setdefaultcstr(upb_fielddef *f, const char *str);

// The results of this function are only meaningful for mutable enum fields,
// which can have a default specified either as an integer or as a string.  If
// this returns true, the default returned from upb_fielddef_default() is a
// string, otherwise it is an integer.
INLINE bool upb_fielddef_default_is_symbolic(const upb_fielddef *f) {
  assert(f->type == UPB_TYPE(ENUM));
  return f->default_is_string;
}

// Subdef. /////////////////////////////////////////////////////////////////////

// Submessage and enum fields must reference a "subdef", which is the
// upb_msgdef or upb_enumdef that defines their type.  Note that when the
// fielddef is mutable it may not have a subdef *yet*, but this function still
// returns true to indicate that the field's type requires a subdef.
INLINE bool upb_hassubdef(const upb_fielddef *f) {
  return upb_issubmsg(f) || f->type == UPB_TYPE(ENUM);
}

// Before a fielddef is finalized, its subdef may be set either directly (with
// a upb_def*) or symbolically.  Symbolic refs must be resolved before the
// containing msgdef can be finalized (see upb_resolve() above).  The client is
// responsible for making sure that "subdef" lives until this fielddef is
// finalized or deleted.
//
// Both methods require that upb_hassubdef(f) (so the type must be set prior
// to calling these methods).  Returns false if this is not the case, or if
// the given subdef is not of the correct type.  The subtype is reset if the
// field's type is changed.
bool upb_fielddef_setsubdef(upb_fielddef *f, upb_def *subdef);
bool upb_fielddef_setsubtypename(upb_fielddef *f, const char *name);

// Returns the enum or submessage def or symbolic name for this field, if any.
// Requires that upb_hassubdef(f).  Returns NULL if the subdef has not been set
// or if you ask for a subtype name when the subtype is currently set
// symbolically (or vice-versa).  To access the subtype's name for a linked
// fielddef, use upb_def_fullname(upb_fielddef_subdef(f)).
//
// Caller does *not* own a ref on the returned def or string.
// upb_fielddef_subtypename() is non-const because finalized defs will never
// have a symbolic reference (they must be resolved before the msgdef can be
// finalized).
upb_def *upb_fielddef_subdef_mutable(upb_fielddef *f);
const upb_def *upb_fielddef_subdef(const upb_fielddef *f);
const char *upb_fielddef_subtypename(upb_fielddef *f);


/* upb_msgdef *****************************************************************/

// Structure that describes a single .proto message type.
typedef struct _upb_msgdef {
  upb_def base;

  // Tables for looking up fields by number and name.
  upb_inttable itof;  // int to field
  upb_strtable ntof;  // name to field

  // The following fields may be modified while mutable.
  uint16_t size;
  uint8_t hasbit_bytes;
  // The range of tag numbers used to store extensions.
  uint32_t extstart, extend;
  // Used for proto2 integration.
  const void *prototype;
} upb_msgdef;

// Returns NULL if memory allocation failed.
upb_msgdef *upb_msgdef_new(void *owner);

INLINE void upb_msgdef_unref(const upb_msgdef *md, void *owner) {
  upb_def_unref(UPB_UPCAST(md), owner);
}
INLINE void upb_msgdef_ref(const upb_msgdef *md, void *owner) {
  upb_def_ref(UPB_UPCAST(md), owner);
}

// Returns a new msgdef that is a copy of the given msgdef (and a copy of all
// the fields) but with any references to submessages broken and replaced with
// just the name of the submessage.  Returns NULL if memory allocation failed.
// This can be put back into another symtab and the names will be re-resolved
// in the new context.
upb_msgdef *upb_msgdef_dup(const upb_msgdef *m, void *owner);

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

// Adds a set of fields (upb_fielddef objects) to a msgdef.  Requires that the
// msgdef and all the fielddefs are mutable.  The fielddef's name and number
// must be set, and the message may not already contain any field with this
// name or number, and this fielddef may not be part of another message.  In
// error cases false is returned and the msgdef is unchanged.
//
// On success, the msgdef takes a ref on the fielddef so the caller needn't
// worry about continuing to keep it alive (however the reverse is not true;
// refs on the fielddef will *not* keep the msgdef alive).  If ref_donor is
// non-NULL, caller passes a ref on the fielddef from ref_donor to the msgdef,
// otherwise caller retains its reference(s) on the defs in f.
bool upb_msgdef_addfields(
    upb_msgdef *m, upb_fielddef *const *f, int n, void *ref_donor);
INLINE bool upb_msgdef_addfield(upb_msgdef *m, upb_fielddef *f,
                                void *ref_donor) {
  return upb_msgdef_addfields(m, &f, 1, ref_donor);
}

// Looks up a field by name or number.  While these are written to be as fast
// as possible, it will still be faster to cache the results of this lookup if
// possible.  These return NULL if no such field is found.
INLINE upb_fielddef *upb_msgdef_itof(const upb_msgdef *m, uint32_t i) {
  const upb_value *val = upb_inttable_lookup32(&m->itof, i);
  return val ? (upb_fielddef*)upb_value_getptr(*val) : NULL;
}

INLINE upb_fielddef *upb_msgdef_ntof(const upb_msgdef *m, const char *name) {
  const upb_value *val = upb_strtable_lookup(&m->ntof, name);
  return val ? (upb_fielddef*)upb_value_getptr(*val) : NULL;
}

INLINE int upb_msgdef_numfields(const upb_msgdef *m) {
  return upb_strtable_count(&m->ntof);
}

// Iteration over fields.  The order is undefined.
// TODO: the iteration should be in field order.
// Iterators are invalidated when a field is added or removed.
//   upb_msg_iter i;
//   for(upb_msg_begin(&i, m); !upb_msg_done(&i); upb_msg_next(&i)) {
//     upb_fielddef *f = upb_msg_iter_field(&i);
//     // ...
//   }
typedef upb_inttable_iter upb_msg_iter;

void upb_msg_begin(upb_msg_iter *iter, const upb_msgdef *m);
void upb_msg_next(upb_msg_iter *iter);
INLINE bool upb_msg_done(upb_msg_iter *iter) { return upb_inttable_done(iter); }

// Iterator accessor.
INLINE upb_fielddef *upb_msg_iter_field(upb_msg_iter *iter) {
  return (upb_fielddef*)upb_value_getptr(upb_inttable_iter_value(iter));
}


/* upb_enumdef ****************************************************************/

typedef struct _upb_enumdef {
  upb_def base;
  upb_strtable ntoi;
  upb_inttable iton;
  int32_t defaultval;
} upb_enumdef;

// Returns NULL if memory allocation failed.
upb_enumdef *upb_enumdef_new(void *owner);
INLINE void upb_enumdef_ref(const upb_enumdef *e, void *owner) {
  upb_def_ref(&e->base, owner);
}
INLINE void upb_enumdef_unref(const upb_enumdef *e, void *owner) {
  upb_def_unref(&e->base, owner);
}
upb_enumdef *upb_enumdef_dup(const upb_enumdef *e, void *owner);

INLINE int32_t upb_enumdef_default(const upb_enumdef *e) {
  return e->defaultval;
}

// May only be set if upb_def_ismutable(e).
void upb_enumdef_setdefault(upb_enumdef *e, int32_t val);

// Returns the number of values currently defined in the enum.  Note that
// multiple names can refer to the same number, so this may be greater than the
// total number of unique numbers.
INLINE int upb_enumdef_numvals(const upb_enumdef *e) {
  return upb_strtable_count(&e->ntoi);
}

// Adds a value to the enumdef.  Requires that no existing val has this name,
// but duplicate numbers are allowed.  May only be called if the enumdef is
// mutable.  Returns false if the existing name is used, or if "name" is not a
// valid label, or on memory allocation failure (we may want to distinguish
// these failure cases in the future).
bool upb_enumdef_addval(upb_enumdef *e, const char *name, int32_t num);

// Lookups from name to integer, returning true if found.
bool upb_enumdef_ntoi(const upb_enumdef *e, const char *name, int32_t *num);

// Finds the name corresponding to the given number, or NULL if none was found.
// If more than one name corresponds to this number, returns the first one that
// was added.
const char *upb_enumdef_iton(const upb_enumdef *e, int32_t num);

// Iteration over name/value pairs.  The order is undefined.
// Adding an enum val invalidates any iterators.
//   upb_enum_iter i;
//   for(upb_enum_begin(&i, e); !upb_enum_done(&i); upb_enum_next(&i)) {
//     // ...
//   }
typedef upb_strtable_iter upb_enum_iter;

void upb_enum_begin(upb_enum_iter *iter, const upb_enumdef *e);
void upb_enum_next(upb_enum_iter *iter);
bool upb_enum_done(upb_enum_iter *iter);

// Iterator accessors.
INLINE const char *upb_enum_iter_name(upb_enum_iter *iter) {
  return upb_strtable_iter_key(iter);
}
INLINE int32_t upb_enum_iter_number(upb_enum_iter *iter) {
  return upb_value_getint32(upb_strtable_iter_value(iter));
}


/* upb_symtab *****************************************************************/

// A symtab (symbol table) stores a name->def map of upb_defs.  Clients could
// always create such tables themselves, but upb_symtab has logic for resolving
// symbolic references, which is nontrivial.
typedef struct {
  uint32_t refcount;
  upb_strtable symtab;
} upb_symtab;

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
// If a def is found, the caller owns one ref on the returned def, owned by
// owner.  Otherwise returns NULL.
const upb_def *upb_symtab_resolve(const upb_symtab *s, const char *base,
                                  const char *sym, void *owner);

// Finds an entry in the symbol table with this exact name.  If a def is found,
// the caller owns one ref on the returned def, owned by owner.  Otherwise
// returns NULL.
const upb_def *upb_symtab_lookup(
    const upb_symtab *s, const char *sym, void *owner);
const upb_msgdef *upb_symtab_lookupmsg(
    const upb_symtab *s, const char *sym, void *owner);

// Gets an array of pointers to all currently active defs in this symtab.  The
// caller owns the returned array (which is of length *count) as well as a ref
// to each symbol inside (owned by owner).  If type is UPB_DEF_ANY then defs of
// all types are returned, otherwise only defs of the required type are
// returned.
const upb_def **upb_symtab_getdefs(
    const upb_symtab *s, int *n, upb_deftype_t type, void *owner);

// Adds the given defs to the symtab, resolving all symbols (including enum
// default values) and finalizing the defs.  Only one def per name may be in
// the list, but defs can replace existing defs in the symtab.  All defs must
// have a name -- anonymous defs are not allowed.  Anonymous defs can still be
// finalized by calling upb_def_finalize() directly.
//
// Any existing defs that can reach defs that are being replaced will
// themselves be replaced also, so that the resulting set of defs is fully
// consistent.
//
// This logic implemented in this method is a convenience; ultimately it calls
// some combination of upb_fielddef_setsubdef(), upb_def_dup(), and
// upb_finalize(), any of which the client could call themself.  However, since
// the logic for doing so is nontrivial, we provide it here.
//
// The entire operation either succeeds or fails.  If the operation fails, the
// symtab is unchanged, false is returned, and status indicates the error.  The
// caller passes a ref on all defs to the symtab (even if the operation fails).
bool upb_symtab_add(upb_symtab *s, upb_def *const*defs, int n, void *ref_donor,
                    upb_status *status);


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
UPB_DEF_CASTS(fielddef, FIELD);
UPB_DEF_CASTS(enumdef, ENUM);
UPB_DEF_CASTS(svcdef, SERVICE);
#undef UPB_DEF_CASTS

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DEF_H_ */
