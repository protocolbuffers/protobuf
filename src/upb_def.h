/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * Provides definitions of .proto constructs:
 * - upb_msgdef: describes a "message" construct.
 * - upb_fielddef: describes a message field.
 * - upb_enumdef: describes an enum.
 * (TODO: descriptions of extensions and services).
 *
 * Defs should be obtained from a upb_context object; the APIs for creating
 * them directly are internal-only.
 *
 * Defs are immutable and reference-counted.  Contexts reference any defs
 * that are the currently in their symbol table.  If an extension is loaded
 * that adds a field to an existing message, a new msgdef is constructed that
 * includes the new field and the old msgdef is unref'd.  The old msgdef will
 * still be ref'd by message (if any) that were constructed with that msgdef.
 *
 * This file contains routines for creating and manipulating the definitions
 * themselves.  To create and manipulate actual messages, see upb_msg.h.
 */

#ifndef UPB_DEF_H_
#define UPB_DEF_H_

#include "upb_atomic.h"
#include "upb_table.h"

#ifdef __cplusplus
extern "C" {
#endif

/* "Base class" for defs; defines common members and functions.  **************/

// All the different kind of defs we support.  These correspond 1:1 with
// declarations in a .proto file.
enum upb_def_type {
  UPB_DEF_MESSAGE,
  UPB_DEF_ENUM,
  UPB_DEF_SERVICE,
  UPB_DEF_EXTENSION,
  // Represented by a string, symbol hasn't been resolved yet.
  UPB_DEF_UNRESOLVED
};

// Common members.
struct upb_def {
  struct upb_string *fqname;  // Fully qualified.
  enum upb_def_type type;
  upb_atomic_refcount_t refcount;
};

void upb_def_init(struct upb_def *def, enum upb_def_type type,
                  struct upb_string *fqname);
void upb_def_uninit(struct upb_def *def);
INLINE void upb_def_ref(struct upb_def *def) { upb_atomic_ref(&def->refcount); }

/* Field definition. **********************************************************/

// A upb_fielddef describes a single field in a message.  It isn't a full def
// in the sense that it derives from upb_def.  It cannot stand on its own; it
// is either a field of a upb_msgdef or contained inside a upb_extensiondef.
struct upb_fielddef {
  upb_field_type_t type;
  upb_label_t label;
  upb_field_number_t number;
  struct upb_string *name;

  // These are set only when this fielddef is part of a msgdef.
  uint32_t byte_offset;     // Where in a upb_msg to find the data.
  uint16_t field_index;     // Indicates set bit.

  // For the case of an enum or a submessage, points to the def for that type.
  // We own a ref on this def.
  struct upb_def *def;
};

// A variety of tests about the type of a field.
INLINE bool upb_issubmsg(struct upb_fielddef *f) {
  return upb_issubmsgtype(f->type);
}
INLINE bool upb_isstring(struct upb_fielddef *f) {
  return upb_isstringtype(f->type);
}
INLINE bool upb_isarray(struct upb_fielddef *f) {
  return f->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REPEATED;
}
// Does the type of this field imply that it should contain an associated def?
INLINE bool upb_fielddef_hasdef(struct upb_fielddef *f) {
  return upb_issubmsg(f) || f->type == UPB_TYPENUM(ENUM);
}

INLINE bool upb_field_ismm(struct upb_fielddef *f) {
  return upb_isarray(f) || upb_isstring(f) || upb_issubmsg(f);
}

INLINE bool upb_elem_ismm(struct upb_fielddef *f) {
  return upb_isstring(f) || upb_issubmsg(f);
}

/* Defined iff upb_field_ismm(f). */
INLINE upb_mm_ptrtype upb_field_ptrtype(struct upb_fielddef *f) {
  if(upb_isarray(f)) return UPB_MM_ARR_REF;
  else if(upb_isstring(f)) return UPB_MM_STR_REF;
  else if(upb_issubmsg(f)) return UPB_MM_MSG_REF;
  else return -1;
}

/* Defined iff upb_elem_ismm(f). */
INLINE upb_mm_ptrtype upb_elem_ptrtype(struct upb_fielddef *f) {
  if(upb_isstring(f)) return UPB_MM_STR_REF;
  else if(upb_issubmsg(f)) return UPB_MM_MSG_REF;
  else return -1;
}

struct google_protobuf_FieldDescriptorProto;

// Interfaces for constructing/destroying fielddefs.  These are internal-only.

// Initializes a upb_fielddef from a FieldDescriptorProto.  The caller must
// have previously allocated the upb_fielddef.
void upb_fielddef_init(struct upb_fielddef *f,
                       struct google_protobuf_FieldDescriptorProto *fd);
struct upb_fielddef *upb_fielddef_dup(struct upb_fielddef *f);
void upb_fielddef_uninit(struct upb_fielddef *f);

// Sort the given fielddefs in-place, according to what we think is an optimal
// ordering of fields.  This can change from upb release to upb release.
void upb_fielddef_sort(struct upb_fielddef *defs, size_t num);
void upb_fielddef_sortfds(struct google_protobuf_FieldDescriptorProto **fds,
                          size_t num);

/* Message definition. ********************************************************/

struct google_protobuf_EnumDescriptorProto;
struct google_protobuf_DescriptorProto;

// Structure that describes a single .proto message type.
struct upb_msgdef {
  struct upb_def def;
  struct upb_msg *default_msg;   // Message with all default values set.
  size_t size;
  uint32_t num_fields;
  uint32_t set_flags_bytes;
  uint32_t num_required_fields;  // Required fields have the lowest set bytemasks.
  struct upb_fielddef *fields;   // We have exclusive ownership of these.

  // Tables for looking up fields by number and name.
  struct upb_inttable fields_by_num;
  struct upb_strtable fields_by_name;
};

// The num->field and name->field maps in upb_msgdef allow fast lookup of fields
// by number or name.  These lookups are in the critical path of parsing and
// field lookup, so they must be as fast as possible.
struct upb_fieldsbynum_entry {
  struct upb_inttable_entry e;
  struct upb_fielddef f;
};
struct upb_fieldsbyname_entry {
  struct upb_strtable_entry e;
  struct upb_fielddef f;
};

// Looks up a field by name or number.  While these are written to be as fast
// as possible, it will still be faster to cache the results of this lookup if
// possible.  These return NULL if no such field is found.
INLINE struct upb_fielddef *upb_msg_fieldbynum(struct upb_msgdef *m,
                                               uint32_t number) {
  struct upb_fieldsbynum_entry *e = (struct upb_fieldsbynum_entry*)
      upb_inttable_fast_lookup(
          &m->fields_by_num, number, sizeof(struct upb_fieldsbynum_entry));
  return e ? &e->f : NULL;
}

INLINE struct upb_fielddef *upb_msg_fieldbyname(struct upb_msgdef *m,
                                                struct upb_string *name) {
  struct upb_fieldsbyname_entry *e = (struct upb_fieldsbyname_entry*)
      upb_strtable_lookup(
          &m->fields_by_name, name);
  return e ? &e->f : NULL;
}

// Internal-only functions for constructing a msgdef.  Caller retains ownership
// of d and fqname.  Ownership of fields passes to the msgdef.
//
// Note that init does not resolve upb_fielddef.ref; the caller should do that
// post-initialization by calling upb_msgdef_resolve() below.
struct upb_msgdef *upb_msgdef_new(struct upb_fielddef *fields, int num_fields,
                                  struct upb_string *fqname);
void _upb_msgdef_free(struct upb_msgdef *m);
INLINE void upb_msgdef_ref(struct upb_msgdef *m) {
  upb_def_ref(&m->def);
}
INLINE void upb_msgdef_unref(struct upb_msgdef *m) {
  if(upb_atomic_unref(&m->def.refcount)) _upb_msgdef_free(m);
}

// Clients use this function on a previously initialized upb_msgdef to resolve
// the "ref" field in the upb_fielddef.  Since messages can refer to each
// other in mutually-recursive ways, this step must be separated from
// initialization.
void upb_msgdef_resolve(struct upb_msgdef *m, struct upb_fielddef *f,
                        struct upb_def *def);

// Downcasts.  They are checked only if asserts are enabled.
INLINE struct upb_msgdef *upb_downcast_msgdef(struct upb_def *def) {
  assert(def->type == UPB_DEF_MESSAGE);
  return (struct upb_msgdef*)def;
}

/* Enum defintion. ************************************************************/

struct upb_enumdef {
  struct upb_def def;
  struct upb_strtable nametoint;
  struct upb_inttable inttoname;
};

struct upb_enumdef_ntoi_entry {
  struct upb_strtable_entry e;
  uint32_t value;
};

struct upb_enumdef_iton_entry {
  struct upb_inttable_entry e;
  struct upb_string *string;
};

// Internal-only functions for creating/destroying an enumdef.  Caller retains
// ownership of ed.  The enumdef is initialized with one ref.
struct upb_enumdef *upb_enumdef_new(
    struct google_protobuf_EnumDescriptorProto *ed, struct upb_string *fqname);
void _upb_enumdef_free(struct upb_enumdef *e);
INLINE void upb_enumdef_ref(struct upb_enumdef *e) { upb_def_ref(&e->def); }
INLINE void upb_enumdef_unref(struct upb_enumdef *e) {
  if(upb_atomic_unref(&e->def.refcount)) _upb_enumdef_free(e);
}
INLINE struct upb_enumdef *upb_downcast_enumdef(struct upb_def *def) {
  assert(def->type == UPB_DEF_ENUM);
  return (struct upb_enumdef*)def;
}

/* Unresolved definition. *****************************************************/

// This is a placeholder definition that contains only the name of the type
// that should eventually be referenced.  Once symbols are resolved, this
// definition is replaced with a real definition.
struct upb_unresolveddef {
  struct upb_def def;
  struct upb_string *name;  // Not fully-qualified.
};

INLINE struct upb_unresolveddef *upb_unresolveddef_new(struct upb_string *name) {
  struct upb_unresolveddef *d = (struct upb_unresolveddef*)malloc(sizeof(*d));
  upb_def_init(&d->def, UPB_DEF_UNRESOLVED, name);
  d->name = name;
  upb_string_ref(name);
  return d;
}
INLINE void _upb_unresolveddef_free(struct upb_unresolveddef *def) {
  upb_def_uninit(&def->def);
  upb_string_unref(def->name);
}
INLINE struct upb_unresolveddef *upb_downcast_unresolveddef(struct upb_def *def) {
  assert(def->type == UPB_DEF_UNRESOLVED);
  return (struct upb_unresolveddef*)def;
}

INLINE void upb_def_unref(struct upb_def *def) {
  if(upb_atomic_unref(&def->refcount)) {
    switch(def->type) {
      case UPB_DEF_MESSAGE:
        _upb_msgdef_free((struct upb_msgdef*)def);
        break;
      case UPB_DEF_ENUM:
        _upb_enumdef_free((struct upb_enumdef*)def);
        break;
      case UPB_DEF_SERVICE:
        assert(false);  /* Unimplemented. */
        break;
      case UPB_DEF_EXTENSION:
        assert(false);  /* Unimplemented. */
        break;
      case UPB_DEF_UNRESOLVED:
        _upb_unresolveddef_free((struct upb_unresolveddef*)def);
        break;
      default:
        assert(false);
    }
  }
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DEF_H_ */
