
#ifndef UPB_LEGACY_MSG_REFLECTION_H_
#define UPB_LEGACY_MSG_REFLECTION_H_

#include "upb/def.h"
#include "upb/msg.h"
#include "upb/upb.h"

#include "upb/port_def.inc"

typedef union {
  bool bool_val;
  float float_val;
  double double_val;
  int32_t int32_val;
  int64_t int64_val;
  uint32_t uint32_val;
  uint64_t uint64_val;
  const upb_map* map_val;
  const upb_msg* msg_val;
  const upb_array* array_val;
  upb_strview str_val;
} upb_msgval;

typedef union {
  upb_map* map;
  upb_msg* msg;
  upb_array* array;
} upb_mutmsgval;

/** upb_msg *******************************************************************/

/* Creates a new message of the given type in the given arena. */
upb_msg *upb_msg_new(const upb_msgdef *m, upb_arena *a);

/* Returns the value associated with this field. */
upb_msgval upb_msg_get(const upb_msg *msg, const upb_fielddef *f);

/* Returns a mutable pointer to a map, array, or submessage value, constructing
 * a new object if it was not previously present.  May not be called for
 * primitive fields. */
upb_mutmsgval upb_msg_mutable(upb_msg *msg, const upb_fielddef *f, upb_arena *a);

/* May only be called for fields where upb_fielddef_haspresence(f) == true. */
bool upb_msg_has(const upb_msg *msg, const upb_fielddef *f);

/* Sets the given field to the given value.  For a msg/array/map/string, the
 * value must be in the same arena.  */
void upb_msg_set(upb_msg *msg, const upb_fielddef *f, upb_msgval val,
                 upb_arena *a);

/* Clears any field presence and sets the value back to its default. */
void upb_msg_clearfield(upb_msg *msg, const upb_fielddef *f);

/** upb_array *****************************************************************/

/* Returns the size of the array. */
size_t upb_array_size(const upb_array *arr);

/* Returns the given element, which must be within the array's current size. */
upb_msgval upb_array_get(const upb_array *arr, size_t i);

/* Sets the given element, which must be within the array's current size. */
void upb_array_set(upb_array *arr, size_t i, upb_msgval val);

/* Appends an element to the array.  Returns false on allocation failure. */
bool upb_array_append(upb_array *array, upb_msgval val, upb_arena *arena);

/* Changes the size of a vector.  New elements are initialized to empty/0.
 * Returns false on allocation failure. */
bool upb_array_resize(upb_array *array, size_t size, upb_arena *arena);

/** upb_map *******************************************************************/

/* Returns the number of entries in the map. */
size_t upb_map_size(const upb_map *map);

/* Stores a value for the given key into |*val| (or the zero value if the key is
 * not present).  Returns whether the key was present.  The |val| pointer may be
 * NULL, in which case the function tests whether the given key is present.  */
bool upb_map_get(const upb_map *map, upb_msgval key, upb_msgval *val);

/* Removes all entries in the map. */
void upb_map_clear(upb_map *map);

/* Sets the given key to the given value.  Returns true if this was a new key in
 * the map, or false if an existing key was replaced. */
bool upb_map_set(upb_map *map, upb_msgval key, upb_msgval val,
                 upb_arena *arena);

/* Deletes this key from the table.  Returns true if the key was present. */
/* TODO(haberman): can |arena| be removed once upb_table is arena-only and no
 * longer tries to free keys? */
bool upb_map_delete(upb_map *map, upb_msgval key, upb_arena *arena);

/** upb_mapiter ***************************************************************/

/* For iterating over a map.  Map iterators are invalidated by mutations to the
 * map, but an invalidated iterator will never return junk or crash the process
 * (this is an important property when exposing iterators to interpreted
 * languages like Ruby, PHP, etc).  An invalidated iterator may return entries
 * that were already returned though, and if you keep invalidating the iterator
 * during iteration, the program may enter an infinite loop. */
struct upb_mapiter;
typedef struct upb_mapiter upb_mapiter;

size_t upb_mapiter_sizeof(void);

/* Starts iteration.  If the map is mutable then we can modify entries while
 * iterating. */
void upb_mapiter_constbegin(upb_mapiter *i, const upb_map *map);
void upb_mapiter_begin(upb_mapiter *i, upb_map *map);

/* Sets the iterator to "done" state.  This will return "true" from
 * upb_mapiter_done() and will compare equal to other "done" iterators. */
void upb_mapiter_setdone(upb_mapiter *i);

/* Advances to the next entry.  The iterator must not be done. */
void upb_mapiter_next(upb_mapiter *i);

/* Returns the key and value for this entry of the map. */
upb_msgval upb_mapiter_key(const upb_mapiter *i);
upb_msgval upb_mapiter_value(const upb_mapiter *i);

/* Sets the value for this entry.  The iterator must not be done, and the
 * iterator must not have been initialized const. */
void upb_mapiter_setvalue(const upb_mapiter *i, upb_msgval value);

/* Returns true if the iterator is done. */
bool upb_mapiter_done(const upb_mapiter *i);

/* Compares two iterators for equality. */
bool upb_mapiter_isequal(const upb_mapiter *i1, const upb_mapiter *i2);

#include "upb/port_undef.inc"

#endif /* UPB_LEGACY_MSG_REFLECTION_H_ */
