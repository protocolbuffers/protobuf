/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * A upb_msg provides a full description of a message as defined in a .proto
 * file.  It supports many features and operations for dealing with proto
 * messages:
 * - reflection over .proto types at runtime (list fields, get names, etc).
 * - an in-memory byte-level format for efficiently storing and accessing msgs.
 * - serializing and deserializing from the in-memory format to a protobuf.
 * - optional memory management for handling strings, arrays, and submessages.
 *
 * Throughout this file, the following convention is used:
 * - "struct upb_msg *m" describes a message type (name, list of fields, etc).
 * - "void *data" is an actual message stored using the in-memory format.
 *
 * The in-memory format is very much like a C struct that you can define at
 * run-time, but also supports reflection.  Like C structs it supports
 * offset-based access, as opposed to the much slower name-based lookup.  The
 * format stores both the values themselves and bits describing whether each
 * field is set or not.  For example:
 *
 * parsed message Foo {
 *   optional bool a = 1;
 *   repeated uint32 b = 2;
 *   optional Bar c = 3;
 * }
 *
 * The in-memory layout for this message on a 32-bit machine will be something
 * like:
 *
 *  Foo
 * +------------------------+
 * | set_flags a:1, b:1, c:1|
 * +------------------------+
 * | bool a (1 byte)        |
 * +------------------------+
 * | padding (3 bytes)      |
 * +------------------------+         upb_array
 * | upb_array* b (4 bytes) | ---->  +----------------------------+
 * +------------------------+        | uint32* elements (4 bytes) | ---+
 * | Bar* c (4 bytes)       |        +----------------------------+    |
 * +------------------------+        | uint32 size (4 bytes)      |    |
 *                                   +----------------------------+    |
 *                                                                     |
 *    -----------------------------------------------------------------+
 *    |
 *    V
 *  uint32 array
 * +----+----+----+----+----+----+
 * | e1 | e2 | e3 | e4 | e5 | e6 |
 * +----+----+----+----+----+----+
 *
 * And the corresponding C structure (as emitted by the proto compiler) would be:
 *
 * struct Foo {
 *   union {
 *     uint8_t bytes[1];
 *     struct {
 *       bool a:1;
 *       bool b:1;
 *       bool c:1;
 *     } has;
 *   } set_flags;
 *   bool a;
 *   upb_uint32_array *b;
 *   Bar *c;
 * }
 *
 * Because the C struct emitted by the upb compiler uses exactly the same
 * byte-level format as the reflection interface, you can access the same hunk
 * of memory either way.  The C struct provides maximum performance and static
 * type safety; upb_msg provides flexibility.
 *
 * The in-memory format has no interoperability guarantees whatsoever, except
 * that a single version of upb will interoperate with itself.  Don't even
 * think about persisting the in-memory format or sending it anywhere.  That's
 * what serialized protobufs are for!  The in-memory format is just that -- an
 * in-memory representation that allows for fast access.
 *
 * The in-memory format is carefully designed to *not* mandate any particular
 * memory management scheme.  This should make it easier to integrate with
 * existing memory management schemes, or to perform advanced techniques like
 * reference counting, garbage collection, and string references.  Different
 * clients can read each others messages regardless of what memory management
 * scheme each is using.
 *
 * A memory management scheme is provided for convenience, and it is used by
 * default by the stock message parser.  Clients can substitute their own
 * memory management scheme into this parser without any loss of generality
 * or performance.
 */

#ifndef UPB_MSG_H_
#define UPB_MSG_H_

#include <stdbool.h>
#include <stdint.h>

#include "upb.h"
#include "upb_table.h"
#include "upb_parse.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Message definition. ********************************************************/

/* Structure that describes a single field in a message.  This structure is very
 * consciously designed to fit into 12/16 bytes (32/64 bit, respectively),
 * because copies of this struct are in the hash table that is read in the
 * critical path of parsing.  Minimizing the size of this struct increases
 * cache-friendliness. */
struct upb_msg_field {
  union upb_symbol_ref ref;
  uint32_t byte_offset;     /* Where to find the data. */
  uint16_t field_index;     /* Indexes upb_msg.fields. Also indicates set bit */
  upb_field_type_t type;    /* Copied from descriptor for cache-friendliness. */
  upb_label_t label;
};

/* Structure that describes a single .proto message type. */
struct upb_msg {
  struct google_protobuf_DescriptorProto *descriptor;
  struct upb_string fqname;      /* Fully qualified. */
  size_t size;
  uint32_t num_fields;
  uint32_t set_flags_bytes;
  uint32_t num_required_fields;  /* Required fields have the lowest set bytemasks. */
  struct upb_inttable fields_by_num;
  struct upb_strtable fields_by_name;
  struct upb_msg_field *fields;
  struct google_protobuf_FieldDescriptorProto **field_descriptors;
};

/* The num->field and name->field maps in upb_msg allow fast lookup of fields
 * by number or name.  These lookups are in the critical path of parsing and
 * field lookup, so they must be as fast as possible.  To make these more
 * cache-friendly, we put the data in the table by value. */

struct upb_fieldsbynum_entry {
  struct upb_inttable_entry e;
  struct upb_msg_field f;
};

struct upb_fieldsbyname_entry {
  struct upb_strtable_entry e;
  struct upb_msg_field f;
};

/* Can be used to retrieve a field descriptor given the upb_msg_field ref. */
INLINE struct google_protobuf_FieldDescriptorProto *upb_msg_field_descriptor(
    struct upb_msg_field *f, struct upb_msg *m) {
  return m->field_descriptors[f->field_index];
}

/* Initializes/frees a upb_msg.  Usually this will be called by upb_context, and
 * clients will not have to construct one directly.
 *
 * Caller retains ownership of d, but the msg will contain references to it, so
 * it must outlive the msg.  Note that init does not resolve upb_msg_field.ref
 * the caller should do that post-initialization by calling upb_msg_ref()
 * below.
 *
 * fqname indicates the fully-qualified name of this message.  Ownership of
 * fqname passes to the msg, but the msg will contain references to it, so it
 * must outlive the msg.
 *
 * sort indicates whether or not it is safe to reorder the fields from the order
 * they appear in d.  This should be false if code has been compiled against a
 * header for this type that expects the given order. */
bool upb_msg_init(struct upb_msg *m, struct google_protobuf_DescriptorProto *d,
                  struct upb_string fqname, bool sort);
void upb_msg_free(struct upb_msg *m);

/* Sort the given field descriptors in-place, according to what we think is an
 * optimal ordering of fields.  This can change from upb release to upb release.
 * This is meant for internal use. */
void upb_msg_sortfds(google_protobuf_FieldDescriptorProto **fds, size_t num);

/* Clients use this function on a previously initialized upb_msg to resolve the
 * "ref" field in the upb_msg_field.  Since messages can refer to each other in
 * mutually-recursive ways, this step must be separated from initialization. */
void upb_msg_ref(struct upb_msg *m, struct upb_msg_field *f, union upb_symbol_ref ref);

/* Looks up a field by name or number.  While these are written to be as fast
 * as possible, it will still be faster to cache the results of this lookup if
 * possible.  These return NULL if no such field is found. */
INLINE struct upb_msg_field *upb_msg_fieldbynum(struct upb_msg *m,
                                                uint32_t number) {
  struct upb_fieldsbynum_entry *e =
      (struct upb_fieldsbynum_entry*)upb_inttable_fast_lookup(
          &m->fields_by_num, number, sizeof(struct upb_fieldsbynum_entry));
  return e ? &e->f : NULL;
}
INLINE struct upb_msg_field *upb_msg_fieldbyname(struct upb_msg *m,
                                                 struct upb_string *name) {
  struct upb_fieldsbyname_entry *e =
      (struct upb_fieldsbyname_entry*)upb_strtable_lookup(
          &m->fields_by_name, name);
  return e ? &e->f : NULL;
}

INLINE bool upb_issubmsg(struct upb_msg_field *f) {
  return upb_issubmsgtype(f->type);
}
INLINE bool upb_isstring(struct upb_msg_field *f) {
  return upb_isstringtype(f->type);
}
INLINE bool upb_isarray(struct upb_msg_field *f) {
  return f->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REPEATED;
}

/* "Set" flag reading and writing.  *******************************************/

/* Please note that these functions do not perform any memory management or in
 * any way ensure that the fields are valid.  They *only* test/set/clear a bit
 * that indicates whether the field is set or not. */

/* Returns the byte offset where we store whether this field is set. */
INLINE size_t upb_isset_offset(uint32_t field_index) {
  return field_index / 8;
}

/* Returns the mask within the appropriate byte that selects the set bit. */
INLINE uint8_t upb_isset_mask(uint32_t field_index) {
  return 1 << (field_index % 8);
}

/* Returns true if the given field is set, false otherwise. */
INLINE void upb_msg_set(void *s, struct upb_msg_field *f)
{
  ((char*)s)[upb_isset_offset(f->field_index)] |= upb_isset_mask(f->field_index);
}

/* Clears the set bit for this field in the given message. */
INLINE void upb_msg_unset(void *s, struct upb_msg_field *f)
{
  ((char*)s)[upb_isset_offset(f->field_index)] &= ~upb_isset_mask(f->field_index);
}

/* Tests whether the given field is set. */
INLINE bool upb_msg_isset(void *s, struct upb_msg_field *f)
{
  return ((char*)s)[upb_isset_offset(f->field_index)] & upb_isset_mask(f->field_index);
}

/* Returns true if *all* required fields are set, false otherwise. */
INLINE bool upb_msg_all_required_fields_set(void *s, struct upb_msg *m)
{
  int num_fields = m->num_required_fields;
  int i = 0;
  while(num_fields > 8) {
    if(((uint8_t*)s)[i++] != 0xFF) return false;
    num_fields -= 8;
  }
  if(((uint8_t*)s)[i] != (1 << num_fields) - 1) return false;
  return true;
}

/* Clears the set bit for all fields. */
INLINE void upb_msg_clear(void *s, struct upb_msg *m)
{
  memset(s, 0, m->set_flags_bytes);
}

/* Scalar (non-array) data access. ********************************************/

/* Returns a pointer to a specific field in a message. */
INLINE union upb_value_ptr upb_msg_getptr(void *data, struct upb_msg_field *f) {
  union upb_value_ptr p;
  p._void = ((char*)data + f->byte_offset);
  return p;
}

/* Returns a a specific field in a message. */
INLINE union upb_value upb_msg_get(void *data, struct upb_msg_field *f) {
  return upb_deref(upb_msg_getptr(data, f), f->type);
}

/* Memory management  *********************************************************/

/* One important note about these memory management routines: they must be used
 * completely or not at all (for each message).  In other words, you can't
 * allocate your own message and then free it with upb_msgdata_free.  As
 * another example, you can't point a field to your own string and then call
 * upb_msg_reuse_str. */

/* Allocates and frees message data, respectively.  Newly allocated data is
 * initialized to empty.  Freeing a message always frees string data, but
 * the client can decide whether or not submessages should be deleted. */
void *upb_msgdata_new(struct upb_msg *m);
void upb_msgdata_free(void *data, struct upb_msg *m, bool free_submsgs);

/* Given a pointer to the appropriate field of the message or array, these
 * functions will lazily allocate memory for a string, array, or submessage.
 * If the previously allocated memory is big enough, it will reuse it without
 * re-allocating.  See upb_msg.c for example usage. */

/* Reuse a string of at least the given size. */
void upb_msg_reuse_str(struct upb_string **str, uint32_t size);
/* Like the previous, but assumes that the string will be by reference, so
 * doesn't allocate memory for the string itself. */
void upb_msg_reuse_strref(struct upb_string **str);

/* Reuse an array of at least the given size, with the given type. */
void upb_msg_reuse_array(struct upb_array **arr, uint32_t size,
                         upb_field_type_t t);

/* Reuse a submessage of the given type. */
void upb_msg_reuse_submsg(void **msg, struct upb_msg *m);

/* Parsing.  ******************************************************************/

/* This is all just a layer on top of the stream-oriented facility in
 * upb_parse.h. */

struct upb_msg_parse_frame {
  struct upb_msg *m;
  void *data;
};

#include "upb_text.h"
struct upb_msg_parse_state {
  struct upb_parse_state s;
  bool merge;
  bool byref;
  struct upb_msg *m;
  struct upb_msg_parse_frame stack[UPB_MAX_NESTING], *top;
  struct upb_text_printer p;
};

/* Initializes/frees a message parser.  The parser will write the data to the
 * message data "data", which the caller must have previously allocated (the
 * parser will allocate submsgs, strings, and arrays as needed, however).
 *
 * "Merge" controls whether the parser will append to data instead of
 * overwriting.  Merging concatenates arrays and merges submessages instead
 * of clearing both.
 *
 * "Byref" controls whether the new message data copies or references strings
 * it encounters.  If byref == true, then all strings supplied to upb_msg_parse
 * must remain unchanged and must outlive data. */
void upb_msg_parse_init(struct upb_msg_parse_state *s, void *data,
                        struct upb_msg *m, bool merge, bool byref);
void upb_msg_parse_reset(struct upb_msg_parse_state *s, void *data,
                         struct upb_msg *m, bool merge, bool byref);
void upb_msg_parse_free(struct upb_msg_parse_state *s);

/* Parses a protobuf fragment, writing the data to the message that was passed
 * to upb_msg_parse_init.  This function can be called multiple times as more
 * data becomes available. */
upb_status_t upb_msg_parse(struct upb_msg_parse_state *s,
                           void *data, size_t len, size_t *read);

/* Parses the protobuf in s (which is expected to be complete) and allocates
 * new message data to hold it.  This is an alternative to the streaming API
 * above.  "byref" works as in upb_msg_parse_init(). */
void *upb_alloc_and_parse(struct upb_msg *m, struct upb_string *s, bool byref);

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
void upb_msgsizes_init(struct upb_msgsizes *sizes);
void upb_msgsizes_free(struct upb_msgsizes *sizes);

/* Given a previously initialized sizes, recurse over the message and store its
 * sizes in 'sizes'. */
void upb_msgsizes_read(struct upb_msgsizes *sizes, void *data,
                       struct upb_msg *m);

/* Returns the total size of the serialized message given in sizes.  Must be
 * preceeded by a call to upb_msgsizes_read. */
size_t upb_msgsizes_totalsize(struct upb_msgsizes *sizes);

struct upb_msg_serialize_state;

/* Initializes the state of serialization.  The provided message must not
 * change between the upb_msgsizes_read() call that was used to construct
 * "sizes" and the parse being fully completed. */
void upb_msg_serialize_alloc(struct upb_msg_serialize_state *s);
void upb_msg_serialize_free(struct upb_msg_serialize_state *s);
void upb_msg_serialize_init(struct upb_msg_serialize_state *s, void *data,
                            struct upb_msg *m, struct upb_msgsizes *sizes);

/* Serializes the next set of bytes into buf (which has size len).  Returns
 * UPB_STATUS_OK if serialization is complete, or UPB_STATUS_NEED_MORE_DATA
 * if there is more data from the message left to be serialized.
 *
 * The number of bytes written to buf is returned in *read.  This will be
 * equal to len unless we finished serializing. */
upb_status_t upb_msg_serialize(struct upb_msg_serialize_state *s,
                               void *buf, size_t len, size_t *read);

/* Text dump  *****************************************************************/

bool upb_msg_eql(void *data1, void *data2, struct upb_msg *m, bool recursive);
void upb_msg_print(void *data, struct upb_msg *m, FILE *stream);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_MSG_H_ */
