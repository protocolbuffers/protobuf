/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * A upb_msgdef provides a full description of a message type as defined in a
 * .proto file.  Using a upb_msgdef, it is possible to treat an arbitrary hunk
 * of memory (a void*) as a protobuf of the given type.  We will call this
 * void* a upb_msg in the context of this interface.
 *
 * Clients generally do not construct or destruct upb_msgdef objects directly.
 * They are managed by upb_contexts, and clients can obtain upb_msgdef pointers
 * directly from a upb_context.
 *
 * A upb_msg is READ-ONLY, and the upb_msgdef functions in this file provide
 * read-only access.  For a mutable message, or for a message that you can take
 * a reference to to prevents its destruction, see upb_mm_msg.h, which is a
 * layer on top of upb_msg that adds memory management semantics.
 *
 * upb_msgdef supports many features and operations for dealing with proto
 * messages:
 * - reflection over .proto types at runtime (list fields, get names, etc).
 * - an in-memory byte-level format for efficiently storing and accessing msgs.
 * - serializing from the in-memory format to a protobuf.
 * - parsing from a protobuf to an in-memory data structure (you either
 *   supply callbacks for allocating/repurposing memory or use a simplified
 *   version that parses into newly-allocated memory).
 *
 * The in-memory format is very much like a C struct that you can define at
 * run-time, but also supports reflection.  Like C structs it supports
 * offset-based access, as opposed to the much slower name-based lookup.  The
 * format stores both the values themselves and bits describing whether each
 * field is set or not.
 *
 * For a more in-depth description of the in-memory format, see:
 *   http://wiki.github.com/haberman/upb/inmemoryformat
 *
 * Because the C struct emitted by the upb compiler uses exactly the same
 * byte-level format as the reflection interface, you can access the same hunk
 * of memory either way.  The C struct provides maximum performance and static
 * type safety; upb_msg_def provides flexibility.
 *
 * The in-memory format has no interoperability guarantees whatsoever, except
 * that a single version of upb will interoperate with itself.  Don't even
 * think about persisting the in-memory format or sending it anywhere.  That's
 * what serialized protobufs are for!  The in-memory format is just that -- an
 * in-memory representation that allows for fast access.
 */

#ifndef UPB_MSG_H_
#define UPB_MSG_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "descriptor.h"
#include "upb.h"
#include "upb_parse.h"
#include "upb_table.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Message definition. ********************************************************/

struct upb_msg_fielddef;
struct upb_context;
/* Structure that describes a single .proto message type. */
struct upb_msgdef {
  struct upb_context *context;
  struct upb_msg *default_msg;   /* Message with all default values set. */
  struct google_protobuf_DescriptorProto *descriptor;
  struct upb_string fqname;      /* Fully qualified. */
  size_t size;
  uint32_t num_fields;
  uint32_t set_flags_bytes;
  uint32_t num_required_fields;  /* Required fields have the lowest set bytemasks. */
  struct upb_inttable fields_by_num;
  struct upb_strtable fields_by_name;
  struct upb_msg_fielddef *fields;
  struct google_protobuf_FieldDescriptorProto **field_descriptors;
};

/* Structure that describes a single field in a message.  This structure is very
 * consciously designed to fit into 12/16 bytes (32/64 bit, respectively),
 * because copies of this struct are in the hash table that is read in the
 * critical path of parsing.  Minimizing the size of this struct increases
 * cache-friendliness. */
struct upb_msg_fielddef {
  union upb_symbol_ref ref;
  uint32_t byte_offset;     /* Where to find the data. */
  uint16_t field_index;     /* Indexes upb_msgdef.fields and indicates set bit */
  upb_field_type_t type;    /* Copied from descriptor for cache-friendliness. */
  upb_label_t label;
};

INLINE bool upb_issubmsg(struct upb_msg_fielddef *f) {
  return upb_issubmsgtype(f->type);
}
INLINE bool upb_isstring(struct upb_msg_fielddef *f) {
  return upb_isstringtype(f->type);
}
INLINE bool upb_isarray(struct upb_msg_fielddef *f) {
  return f->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REPEATED;
}

INLINE bool upb_field_ismm(struct upb_msg_fielddef *f) {
  return upb_isarray(f) || upb_isstring(f) || upb_issubmsg(f);
}

INLINE bool upb_elem_ismm(struct upb_msg_fielddef *f) {
  return upb_isstring(f) || upb_issubmsg(f);
}

/* Defined iff upb_field_ismm(f). */
INLINE upb_mm_ptrtype upb_field_ptrtype(struct upb_msg_fielddef *f) {
  if(upb_isarray(f)) return UPB_MM_ARR_REF;
  else if(upb_isstring(f)) return UPB_MM_STR_REF;
  else if(upb_issubmsg(f)) return UPB_MM_MSG_REF;
  else return -1;
}

/* Defined iff upb_elem_ismm(f). */
INLINE upb_mm_ptrtype upb_elem_ptrtype(struct upb_msg_fielddef *f) {
  if(upb_isstring(f)) return UPB_MM_STR_REF;
  else if(upb_issubmsg(f)) return UPB_MM_MSG_REF;
  else return -1;
}

/* Can be used to retrieve a field descriptor given the upb_msg_fielddef. */
INLINE struct google_protobuf_FieldDescriptorProto *upb_msg_field_descriptor(
    struct upb_msg_fielddef *f, struct upb_msgdef *m) {
  return m->field_descriptors[f->field_index];
}

/* Message structure. *********************************************************/

/* Constructs a new msg corresponding to the given msgdef, and having one
 * counted reference. */
INLINE struct upb_msg *upb_msg_new(struct upb_msgdef *md) {
  size_t size = md->size + offsetof(struct upb_msg, data);
  struct upb_msg *msg = (struct upb_msg*)malloc(size);
  memset(msg, 0, size);
  upb_mmhead_init(&msg->mmhead);
  msg->def = md;
  return msg;
}

/* Field access. **************************************************************/

/* Note that these only provide access to fields that are directly in the msg
 * itself.  For dynamic fields (strings, arrays, and submessages) it will be
 * necessary to dereference the returned values. */

/* Returns a pointer to a specific field in a message. */
INLINE union upb_value_ptr upb_msg_getptr(struct upb_msg *msg,
                                          struct upb_msg_fielddef *f) {
  union upb_value_ptr p;
  p._void = &msg->data[f->byte_offset];
  return p;
}

/* "Set" flag reading and writing.  *******************************************/

/* All upb code and code using upb should guarantee that the set flags are
 * always valid.  It should always be the case that if a flag's field is set
 * for a dynamic field that the pointer is valid.
 *
 * Clients should never set fields on a plain upb_msg, only on a upb_mm_msg. */

/* Returns the byte offset where we store whether this field is set. */
INLINE size_t upb_isset_offset(uint32_t field_index) {
  return field_index / 8;
}

/* Returns the mask within the appropriate byte that selects the set bit. */
INLINE uint8_t upb_isset_mask(uint32_t field_index) {
  return 1 << (field_index % 8);
}

/* Returns true if the given field is set, false otherwise. */
INLINE void upb_msg_set(struct upb_msg *msg, struct upb_msg_fielddef *f)
{
  msg->data[upb_isset_offset(f->field_index)] |= upb_isset_mask(f->field_index);
}

/* Clears the set bit for this field in the given message. */
INLINE void upb_msg_unset(struct upb_msg *msg, struct upb_msg_fielddef *f)
{
  msg->data[upb_isset_offset(f->field_index)] &= ~upb_isset_mask(f->field_index);
}

/* Tests whether the given field is set. */
INLINE bool upb_msg_isset(struct upb_msg *msg, struct upb_msg_fielddef *f)
{
  return msg->data[upb_isset_offset(f->field_index)] & upb_isset_mask(f->field_index);
}

/* Returns true if *all* required fields are set, false otherwise. */
INLINE bool upb_msg_all_required_fields_set(struct upb_msg *msg)
{
  int num_fields = msg->def->num_required_fields;
  int i = 0;
  while(num_fields > 8) {
    if(msg->data[i++] != 0xFF) return false;
    num_fields -= 8;
  }
  if(msg->data[i] != (1 << num_fields) - 1) return false;
  return true;
}

/* Clears the set bit for all fields. */
INLINE void upb_msg_clear(struct upb_msg *msg)
{
  memset(msg->data, 0, msg->def->set_flags_bytes);
}

/* Number->field and name->field lookup.  *************************************/

/* The num->field and name->field maps in upb_msgdef allow fast lookup of fields
 * by number or name.  These lookups are in the critical path of parsing and
 * field lookup, so they must be as fast as possible.  To make these more
 * cache-friendly, we put the data in the table by value. */

struct upb_fieldsbynum_entry {
  struct upb_inttable_entry e;
  struct upb_msg_fielddef f;
};

struct upb_fieldsbyname_entry {
  struct upb_strtable_entry e;
  struct upb_msg_fielddef f;
};

/* Looks up a field by name or number.  While these are written to be as fast
 * as possible, it will still be faster to cache the results of this lookup if
 * possible.  These return NULL if no such field is found. */
INLINE struct upb_msg_fielddef *upb_msg_fieldbynum(struct upb_msgdef *m,
                                                   uint32_t number) {
  struct upb_fieldsbynum_entry *e =
      (struct upb_fieldsbynum_entry*)upb_inttable_fast_lookup(
          &m->fields_by_num, number, sizeof(struct upb_fieldsbynum_entry));
  return e ? &e->f : NULL;
}

INLINE struct upb_msg_fielddef *upb_msg_fieldbyname(struct upb_msgdef *m,
                                                    struct upb_string *name) {
  struct upb_fieldsbyname_entry *e =
      (struct upb_fieldsbyname_entry*)upb_strtable_lookup(
          &m->fields_by_name, name);
  return e ? &e->f : NULL;
}


/* Parsing ********************************************************************/

/* TODO: a stream parser. */
void upb_msg_parsestr(struct upb_msg *msg, void *buf, size_t len,
                      struct upb_status *status);

struct upb_msgparser *upb_msgparser_new(struct upb_msgdef *def);
void upb_msgparser_free(struct upb_msgparser *mp);

void upb_msgparser_reset(struct upb_msgparser *mp, struct upb_msg *m,
                         bool byref);

size_t upb_msgparser_parse(struct upb_msgparser *mp, void *buf, size_t len,
                           struct upb_status *status);

/* Serialization  *************************************************************/

/* For messages that contain any submessages, we must do a pre-pass on the
 * message tree to discover the size of all submessages.  This is necessary
 * because when serializing, the message length has to precede the message data
 * itself.
 *
 * We can calculate these sizes once and reuse them as long as the message is
 * known not to have changed. */
struct upb_msgsizes;

/* Initialize/free a upb_msgsizes for the given message. */
struct upb_msgsizes *upb_msgsizes_new(void);
void upb_msgsizes_free(struct upb_msgsizes *sizes);

/* Given a previously initialized sizes, recurse over the message and store its
 * sizes in 'sizes'. */
void upb_msgsizes_read(struct upb_msgsizes *sizes, struct upb_msg *msg);

/* Returns the total size of the serialized message given in sizes.  Must be
 * preceeded by a call to upb_msgsizes_read. */
size_t upb_msgsizes_totalsize(struct upb_msgsizes *sizes);

struct upb_msg_serialize_state;

/* Initializes the state of serialization.  The provided message must not
 * change between the upb_msgsizes_read() call that was used to construct
 * "sizes" and the parse being fully completed. */
void upb_msg_serialize_alloc(struct upb_msg_serialize_state *s);
void upb_msg_serialize_free(struct upb_msg_serialize_state *s);
void upb_msg_serialize_init(struct upb_msg_serialize_state *s,
                            struct upb_msg *msg, struct upb_msgsizes *sizes);

/* Serializes the next set of bytes into buf (which has size len).  Returns
 * UPB_STATUS_OK if serialization is complete, or UPB_STATUS_NEED_MORE_DATA
 * if there is more data from the message left to be serialized.
 *
 * The number of bytes written to buf is returned in *written.  This will be
 * equal to len unless we finished serializing. */
size_t upb_msg_serialize(struct upb_msg_serialize_state *s,
                         void *buf, size_t len, struct upb_status *status);

void upb_msg_serialize_all(struct upb_msg *msg, struct upb_msgsizes *sizes,
                           void *buf, struct upb_status *status);

/* Text dump  *****************************************************************/

bool upb_msg_eql(struct upb_msg *msg1, struct upb_msg *msg2, bool recursive);
void upb_msg_print(struct upb_msg *data, bool single_line, FILE *stream);

/* Internal functions. ********************************************************/

/* Initializes/frees a upb_msgdef.  Usually this will be called by upb_context,
 * and clients will not have to construct one directly.
 *
 * Caller retains ownership of d, but the msg will contain references to it, so
 * it must outlive the msg.  Note that init does not resolve
 * upb_msg_fielddef.ref the caller should do that post-initialization by
 * calling upb_msg_ref() below.
 *
 * fqname indicates the fully-qualified name of this message.  Ownership of
 * fqname passes to the msg, but the msg will contain references to it, so it
 * must outlive the msg.
 *
 * sort indicates whether or not it is safe to reorder the fields from the order
 * they appear in d.  This should be false if code has been compiled against a
 * header for this type that expects the given order. */
void upb_msgdef_init(struct upb_msgdef *m,
                     struct google_protobuf_DescriptorProto *d,
                     struct upb_string fqname, bool sort,
                     struct upb_context *c, struct upb_status *status);
void upb_msgdef_free(struct upb_msgdef *m);

/* Sort the given field descriptors in-place, according to what we think is an
 * optimal ordering of fields.  This can change from upb release to upb
 * release. */
void upb_msgdef_sortfds(struct google_protobuf_FieldDescriptorProto **fds,
                        size_t num);

/* Clients use this function on a previously initialized upb_msgdef to resolve
 * the "ref" field in the upb_msg_fielddef.  Since messages can refer to each
 * other in mutually-recursive ways, this step must be separated from
 * initialization. */
void upb_msgdef_setref(struct upb_msgdef *m, struct upb_msg_fielddef *f,
                       union upb_symbol_ref ref);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_MSG_H_ */
