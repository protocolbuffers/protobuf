/*
** upb::Message is a representation for protobuf messages.
**
** However it differs from other common representations like
** google::protobuf::Message in one key way: it does not prescribe any
** ownership between messages and submessages, and it relies on the
** client to ensure that each submessage/array/map outlives its parent.
**
** All messages, arrays, and maps live in an Arena.  If the entire message
** tree is in the same arena, ensuring proper lifetimes is simple.  However
** the client can mix arenas as long as they ensure that there are no
** dangling pointers.
**
** A client can access a upb::Message without knowing anything about
** ownership semantics, but to create or mutate a message a user needs
** to implement the memory management themselves.
**
** TODO: UTF-8 checking?
**/

#ifndef UPB_MSG_H_
#define UPB_MSG_H_

#include "upb/def.h"
#include "upb/handlers.h"
#include "upb/sink.h"

#ifdef __cplusplus

namespace upb {
class Array;
class Map;
class MapIterator;
class MessageFactory;
class MessageLayout;
class Visitor;
class VisitorPlan;
}

#endif

UPB_DECLARE_TYPE(upb::MessageFactory, upb_msgfactory)
UPB_DECLARE_TYPE(upb::MessageLayout, upb_msglayout)
UPB_DECLARE_TYPE(upb::Array, upb_array)
UPB_DECLARE_TYPE(upb::Map, upb_map)
UPB_DECLARE_TYPE(upb::MapIterator, upb_mapiter)
UPB_DECLARE_TYPE(upb::Visitor, upb_visitor)
UPB_DECLARE_TYPE(upb::VisitorPlan, upb_visitorplan)

/* TODO(haberman): C++ accessors */

UPB_BEGIN_EXTERN_C

typedef void upb_msg;


/** upb_msglayout *************************************************************/

/* upb_msglayout represents the memory layout of a given upb_msgdef.  You get
 * instances of this from a upb_msgfactory, and the factory always owns the
 * msglayout. */


/** upb_msgfactory ************************************************************/

/* A upb_msgfactory contains a cache of upb_msglayout, upb_handlers, and
 * upb_visitorplan objects.  These are the objects necessary to represent,
 * populate, and and visit upb_msg objects.
 *
 * These caches are all populated by upb_msgdef, and lazily created on demand.
 */

/* Creates and destroys a msgfactory, respectively.  The messages for this
 * msgfactory must come from |symtab| (which should outlive the msgfactory). */
upb_msgfactory *upb_msgfactory_new(const upb_symtab *symtab);
void upb_msgfactory_free(upb_msgfactory *f);

const upb_symtab *upb_msgfactory_symtab(const upb_msgfactory *f);

/* The functions to get cached objects, lazily creating them on demand.  These
 * all require:
 *
 * - m is in upb_msgfactory_symtab(f)
 * - upb_msgdef_mapentry(m) == false (since map messages can't have layouts).
 *
 * The returned objects will live for as long as the msgfactory does.
 *
 * TODO(haberman): consider making this thread-safe and take a const
 * upb_msgfactory. */
const upb_msglayout *upb_msgfactory_getlayout(upb_msgfactory *f,
                                              const upb_msgdef *m);


/** upb_stringview ************************************************************/

typedef struct {
  const char *data;
  size_t size;
} upb_stringview;

UPB_INLINE upb_stringview upb_stringview_make(const char *data, size_t size) {
  upb_stringview ret;
  ret.data = data;
  ret.size = size;
  return ret;
}

#define UPB_STRINGVIEW_INIT(ptr, len) {ptr, len}


/** upb_msgval ****************************************************************/

/* A union representing all possible protobuf values.  Used for generic get/set
 * operations. */

typedef union {
  bool b;
  float flt;
  double dbl;
  int32_t i32;
  int64_t i64;
  uint32_t u32;
  uint64_t u64;
  const upb_map* map;
  const upb_msg* msg;
  const upb_array* arr;
  const void* ptr;
  upb_stringview str;
} upb_msgval;

#define ACCESSORS(name, membername, ctype) \
  UPB_INLINE ctype upb_msgval_get ## name(upb_msgval v) { \
    return v.membername; \
  } \
  UPB_INLINE void upb_msgval_set ## name(upb_msgval *v, ctype cval) { \
    v->membername = cval; \
  } \
  UPB_INLINE upb_msgval upb_msgval_ ## name(ctype v) { \
    upb_msgval ret; \
    ret.membername = v; \
    return ret; \
  }

ACCESSORS(bool,   b,   bool)
ACCESSORS(float,  flt, float)
ACCESSORS(double, dbl, double)
ACCESSORS(int32,  i32, int32_t)
ACCESSORS(int64,  i64, int64_t)
ACCESSORS(uint32, u32, uint32_t)
ACCESSORS(uint64, u64, uint64_t)
ACCESSORS(map,    map, const upb_map*)
ACCESSORS(msg,    msg, const upb_msg*)
ACCESSORS(ptr,    ptr, const void*)
ACCESSORS(arr,    arr, const upb_array*)
ACCESSORS(str,    str, upb_stringview)

#undef ACCESSORS

UPB_INLINE upb_msgval upb_msgval_makestr(const char *data, size_t size) {
  return upb_msgval_str(upb_stringview_make(data, size));
}


/** upb_msg *******************************************************************/

/* A upb_msg represents a protobuf message.  It always corresponds to a specific
 * upb_msglayout, which describes how it is laid out in memory.  */

/* Creates a new message of the given type/layout in this arena. */
upb_msg *upb_msg_new(const upb_msglayout *l, upb_arena *a);

/* Returns the arena for the given message. */
upb_arena *upb_msg_arena(const upb_msg *msg);

/* Read-only message API.  Can be safely called by anyone. */

/* Returns the value associated with this field:
 *   - for scalar fields (including strings), the value directly.
 *   - return upb_msg*, or upb_map* for msg/map.
 *     If the field is unset for these field types, returns NULL.
 *
 * TODO(haberman): should we let users store cached array/map/msg
 * pointers here for fields that are unset?  Could be useful for the
 * strongly-owned submessage model (ie. generated C API that doesn't use
 * arenas).
 */
upb_msgval upb_msg_get(const upb_msg *msg,
                       int field_index,
                       const upb_msglayout *l);

/* May only be called for fields where upb_fielddef_haspresence(f) == true. */
bool upb_msg_has(const upb_msg *msg,
                 int field_index,
                 const upb_msglayout *l);

/* Mutable message API.  May only be called by the owner of the message who
 * knows its ownership scheme and how to keep it consistent. */

/* Sets the given field to the given value.  Does not perform any memory
 * management: if you overwrite a pointer to a msg/array/map/string without
 * cleaning it up (or using an arena) it will leak.
 */
void upb_msg_set(upb_msg *msg,
                 int field_index,
                 upb_msgval val,
                 const upb_msglayout *l);

/* For a primitive field, set it back to its default. For repeated, string, and
 * submessage fields set it back to NULL.  This could involve releasing some
 * internal memory (for example, from an extension dictionary), but it is not
 * recursive in any way and will not recover any memory that may be used by
 * arrays/maps/strings/msgs that this field may have pointed to.
 */
bool upb_msg_clearfield(upb_msg *msg,
                        int field_index,
                        const upb_msglayout *l);

/* TODO(haberman): copyfrom()/mergefrom()? */


/** upb_array *****************************************************************/

/* A upb_array stores data for a repeated field.  The memory management
 * semantics are the same as upb_msg.  A upb_array allocates dynamic
 * memory internally for the array elements. */

upb_array *upb_array_new(upb_fieldtype_t type, upb_arena *a);
upb_fieldtype_t upb_array_type(const upb_array *arr);

/* Read-only interface.  Safe for anyone to call. */

size_t upb_array_size(const upb_array *arr);
upb_msgval upb_array_get(const upb_array *arr, size_t i);

/* Write interface.  May only be called by the message's owner who can enforce
 * its memory management invariants. */

bool upb_array_set(upb_array *arr, size_t i, upb_msgval val);


/** upb_map *******************************************************************/

/* A upb_map stores data for a map field.  The memory management semantics are
 * the same as upb_msg, with one notable exception.  upb_map will internally
 * store a copy of all string keys, but *not* any string values or submessages.
 * So you must ensure that any string or message values outlive the map, and you
 * must delete them manually when they are no longer required. */

upb_map *upb_map_new(upb_fieldtype_t ktype, upb_fieldtype_t vtype,
                     upb_arena *a);

/* Read-only interface.  Safe for anyone to call. */

size_t upb_map_size(const upb_map *map);
upb_fieldtype_t upb_map_keytype(const upb_map *map);
upb_fieldtype_t upb_map_valuetype(const upb_map *map);
bool upb_map_get(const upb_map *map, upb_msgval key, upb_msgval *val);

/* Write interface.  May only be called by the message's owner who can enforce
 * its memory management invariants. */

/* Sets or overwrites an entry in the map.  Return value indicates whether
 * the operation succeeded or failed with OOM, and also whether an existing
 * key was replaced or not. */
bool upb_map_set(upb_map *map,
                 upb_msgval key, upb_msgval val,
                 upb_msgval *valremoved);

/* Deletes an entry in the map.  Returns true if the key was present. */
bool upb_map_del(upb_map *map, upb_msgval key);


/** upb_mapiter ***************************************************************/

/* For iterating over a map.  Map iterators are invalidated by mutations to the
 * map, but an invalidated iterator will never return junk or crash the process.
 * An invalidated iterator may return entries that were already returned though,
 * and if you keep invalidating the iterator during iteration, the program may
 * enter an infinite loop. */

size_t upb_mapiter_sizeof();

void upb_mapiter_begin(upb_mapiter *i, const upb_map *t);
upb_mapiter *upb_mapiter_new(const upb_map *t, upb_alloc *a);
void upb_mapiter_free(upb_mapiter *i, upb_alloc *a);
void upb_mapiter_next(upb_mapiter *i);
bool upb_mapiter_done(const upb_mapiter *i);

upb_msgval upb_mapiter_key(const upb_mapiter *i);
upb_msgval upb_mapiter_value(const upb_mapiter *i);
void upb_mapiter_setdone(upb_mapiter *i);
bool upb_mapiter_isequal(const upb_mapiter *i1, const upb_mapiter *i2);


/** Interfaces for generated code *********************************************/

#define UPB_NOT_IN_ONEOF UINT16_MAX
#define UPB_NO_HASBIT UINT16_MAX
#define UPB_NO_SUBMSG UINT16_MAX

typedef struct {
  uint32_t number;
  uint32_t offset;  /* If in a oneof, offset of default in default_msg below. */
  uint16_t hasbit;        /* UPB_NO_HASBIT if no hasbit. */
  uint16_t oneof_index;   /* UPB_NOT_IN_ONEOF if not in a oneof. */
  uint16_t submsg_index;  /* UPB_NO_SUBMSG if no submsg. */
  uint8_t descriptortype;
  uint8_t label;
} upb_msglayout_fieldinit_v1;

typedef struct {
  uint32_t data_offset;
  uint32_t case_offset;
} upb_msglayout_oneofinit_v1;

typedef struct upb_msglayout_msginit_v1 {
  const struct upb_msglayout_msginit_v1 *const* submsgs;
  const upb_msglayout_fieldinit_v1 *fields;
  const upb_msglayout_oneofinit_v1 *oneofs;
  void *default_msg;
  /* Must be aligned to sizeof(void*).  Doesn't include internal members like
   * unknown fields, extension dict, pointer to msglayout, etc. */
  uint32_t size;
  uint16_t field_count;
  uint16_t oneof_count;
  bool extendable;
  bool is_proto2;
} upb_msglayout_msginit_v1;

#define UPB_ALIGN_UP_TO(val, align) ((val + (align - 1)) & -align)
#define UPB_ALIGNED_SIZEOF(type) UPB_ALIGN_UP_TO(sizeof(type), sizeof(void*))

/* Initialize/uninitialize a msglayout from a msginit.  If upb uses v1
 * internally, this will not allocate any memory.  Should only be used by
 * generated code. */
upb_msglayout *upb_msglayout_frominit_v1(
    const upb_msglayout_msginit_v1 *init, upb_alloc *a);
void upb_msglayout_uninit_v1(upb_msglayout *layout, upb_alloc *a);

UPB_END_EXTERN_C

#endif /* UPB_MSG_H_ */
