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
 * Defs are immutable and reference-counted.  
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

/* Message definition. ********************************************************/

struct upb_fielddef;
struct upb_context;
struct google_protobuf_EnumDescriptorProto;
struct google_protobuf_DescriptorProto;
struct google_protobuf_FieldDescriptorProto;
/* Structure that describes a single .proto message type. */
struct upb_msgdef {
  struct upb_context *context;
  struct upb_msg *default_msg;   /* Message with all default values set. */
  struct upb_string *fqname;     /* Fully qualified. */
  size_t size;
  uint32_t num_fields;
  uint32_t set_flags_bytes;
  uint32_t num_required_fields;  /* Required fields have the lowest set bytemasks. */
  struct upb_inttable fields_by_num;
  struct upb_strtable fields_by_name;
  struct upb_fielddef *fields;
};

/* Structure that describes a single field in a message. */
struct upb_fielddef {
  union upb_symbol_ref ref;
  uint32_t byte_offset;     /* Where to find the data. */
  uint16_t field_index;     /* Indicates set bit. */

  /* TODO: Performance test whether it's better to move the name and number
   * into an array in upb_msgdef, indexed by field_index. */
  upb_field_number_t number;
  struct upb_string *name;

  upb_field_type_t type;
  upb_label_t label;
};

INLINE bool upb_issubmsg(struct upb_fielddef *f) {
  return upb_issubmsgtype(f->type);
}
INLINE bool upb_isstring(struct upb_fielddef *f) {
  return upb_isstringtype(f->type);
}
INLINE bool upb_isarray(struct upb_fielddef *f) {
  return f->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REPEATED;
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

/* Number->field and name->field lookup.  *************************************/

/* The num->field and name->field maps in upb_msgdef allow fast lookup of fields
 * by number or name.  These lookups are in the critical path of parsing and
 * field lookup, so they must be as fast as possible.  To make these more
 * cache-friendly, we put the data in the table by value. */

struct upb_fieldsbynum_entry {
  struct upb_inttable_entry e;
  struct upb_fielddef f;
};

struct upb_fieldsbyname_entry {
  struct upb_strtable_entry e;
  struct upb_fielddef f;
};

/* Looks up a field by name or number.  While these are written to be as fast
 * as possible, it will still be faster to cache the results of this lookup if
 * possible.  These return NULL if no such field is found. */
INLINE struct upb_fielddef *upb_msg_fieldbynum(struct upb_msgdef *m,
                                                   uint32_t number) {
  struct upb_fieldsbynum_entry *e =
      (struct upb_fieldsbynum_entry*)upb_inttable_fast_lookup(
          &m->fields_by_num, number, sizeof(struct upb_fieldsbynum_entry));
  return e ? &e->f : NULL;
}

INLINE struct upb_fielddef *upb_msg_fieldbyname(struct upb_msgdef *m,
                                                    struct upb_string *name) {
  struct upb_fieldsbyname_entry *e =
      (struct upb_fieldsbyname_entry*)upb_strtable_lookup(
          &m->fields_by_name, name);
  return e ? &e->f : NULL;
}

/* Enums. *********************************************************************/

struct upb_enumdef {
  upb_atomic_refcount_t refcount;
  struct upb_context *context;
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

/* Initializes and frees an enum, respectively.  Caller retains ownership of
 * ed, but it must outlive e. */
void upb_enumdef_init(struct upb_enumdef *e,
                      struct google_protobuf_EnumDescriptorProto *ed,
                      struct upb_context *c);
void upb_enumdef_free(struct upb_enumdef *e);


/* Internal functions. ********************************************************/

/* Initializes/frees a upb_msgdef.  Usually this will be called by upb_context,
 * and clients will not have to construct one directly.
 *
 * Caller retains ownership of d.  Note that init does not resolve
 * upb_fielddef.ref the caller should do that post-initialization by
 * calling upb_msg_ref() below.
 *
 * fqname indicates the fully-qualified name of this message.
 *
 * sort indicates whether or not it is safe to reorder the fields from the order
 * they appear in d.  This should be false if code has been compiled against a
 * header for this type that expects the given order. */
void upb_msgdef_init(struct upb_msgdef *m,
                     struct google_protobuf_DescriptorProto *d,
                     struct upb_string *fqname, bool sort,
                     struct upb_context *c, struct upb_status *status);
void upb_msgdef_free(struct upb_msgdef *m);

/* Sort the given field descriptors in-place, according to what we think is an
 * optimal ordering of fields.  This can change from upb release to upb
 * release. */
void upb_msgdef_sortfds(struct google_protobuf_FieldDescriptorProto **fds,
                        size_t num);

/* Clients use this function on a previously initialized upb_msgdef to resolve
 * the "ref" field in the upb_fielddef.  Since messages can refer to each
 * other in mutually-recursive ways, this step must be separated from
 * initialization. */
void upb_msgdef_setref(struct upb_msgdef *m, struct upb_fielddef *f,
                       union upb_symbol_ref ref);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DEF_H_ */
