
#include "upb/msg.h"

#include "upb/table.int.h"

#include "upb/port_def.inc"

/** upb_msg *******************************************************************/

static const char _upb_fieldtype_to_sizelg2[12] = {
  0,
  0,  /* UPB_TYPE_BOOL */
  2,  /* UPB_TYPE_FLOAT */
  2,  /* UPB_TYPE_INT32 */
  2,  /* UPB_TYPE_UINT32 */
  2,  /* UPB_TYPE_ENUM */
  UPB_SIZE(2, 3),  /* UPB_TYPE_MESSAGE */
  3,  /* UPB_TYPE_DOUBLE */
  3,  /* UPB_TYPE_INT64 */
  3,  /* UPB_TYPE_UINT64 */
  UPB_SIZE(3, 4),  /* UPB_TYPE_STRING */
  UPB_SIZE(3, 4),  /* UPB_TYPE_BYTES */
};

static uintptr_t tag_arrptr(void* ptr, int elem_size_lg2) {
  UPB_ASSERT(elem_size_lg2 <= 4);
  return (uintptr_t)ptr | elem_size_lg2;
}

static int upb_msg_internalsize(const upb_msglayout *l) {
  return sizeof(upb_msg_internal) - l->extendable * sizeof(void *);
}

static size_t upb_msg_sizeof(const upb_msglayout *l) {
  return l->size + upb_msg_internalsize(l);
}

static const upb_msg_internal *upb_msg_getinternal_const(const upb_msg *msg) {
  ptrdiff_t size = sizeof(upb_msg_internal);
  return UPB_PTR_AT(msg, -size, upb_msg_internal);
}

static upb_msg_internal *upb_msg_getinternal(upb_msg *msg) {
  return (upb_msg_internal*)upb_msg_getinternal_const(msg);
}

void _upb_msg_clear(upb_msg *msg, const upb_msglayout *l) {
  ptrdiff_t internal = upb_msg_internalsize(l);
  void *mem = UPB_PTR_AT(msg, -internal, char);
  memset(mem, 0, l->size + internal);
}

upb_msg *_upb_msg_new(const upb_msglayout *l, upb_arena *a) {
  void *mem = upb_arena_malloc(a, upb_msg_sizeof(l));
  upb_msg *msg;

  if (!mem) {
    return NULL;
  }

  msg = UPB_PTR_AT(mem, upb_msg_internalsize(l), upb_msg);
  _upb_msg_clear(msg, l);
  return msg;
}

bool _upb_msg_addunknown(upb_msg *msg, const char *data, size_t len,
                         upb_arena *arena) {
  upb_msg_internal *in = upb_msg_getinternal(msg);
  if (len > in->unknown_size - in->unknown_len) {
    upb_alloc *alloc = upb_arena_alloc(arena);
    size_t need = in->unknown_size + len;
    size_t newsize = UPB_MAX(in->unknown_size * 2, need);
    void *mem = upb_realloc(alloc, in->unknown, in->unknown_size, newsize);
    if (!mem) return false;
    in->unknown = mem;
    in->unknown_size = newsize;
  }
  memcpy(in->unknown + in->unknown_len, data, len);
  in->unknown_len += len;
  return true;
}

void _upb_msg_discardunknown_shallow(upb_msg *msg) {
  upb_msg_internal *in = upb_msg_getinternal(msg);
  in->unknown_len = 0;
}

const char *upb_msg_getunknown(const upb_msg *msg, size_t *len) {
  const upb_msg_internal *in = upb_msg_getinternal_const(msg);
  *len = in->unknown_len;
  return in->unknown;
}

/** upb_array *****************************************************************/

upb_array *_upb_array_new(upb_arena *a, upb_fieldtype_t type) {
  upb_array *arr = upb_arena_malloc(a, sizeof(upb_array));

  if (!arr) {
    return NULL;
  }

  arr->data = tag_arrptr(NULL, _upb_fieldtype_to_sizelg2[type]);
  arr->len = 0;
  arr->size = 0;

  return arr;
}

bool _upb_array_realloc(upb_array *arr, size_t min_size, upb_arena *arena) {
  size_t new_size = UPB_MAX(arr->size, 4);
  int elem_size_lg2 = arr->data & 7;
  size_t old_bytes = arr->size << elem_size_lg2;
  size_t new_bytes;
  void* ptr = _upb_array_ptr(arr);

  /* Log2 ceiling of size. */
  while (new_size < min_size) new_size *= 2;

  new_bytes = new_size << elem_size_lg2;
  ptr = upb_arena_realloc(arena, ptr, old_bytes, new_bytes);

  if (!ptr) {
    return false;
  }

  arr->data = tag_arrptr(ptr, elem_size_lg2);
  arr->size = new_size;
  return true;
}

static upb_array *getorcreate_array(upb_array **arr_ptr, upb_fieldtype_t type,
                                    upb_arena *arena) {
  upb_array *arr = *arr_ptr;
  if (!arr) {
    arr = _upb_array_new(arena, type);
    if (!arr) return NULL;
    *arr_ptr = arr;
  }
  return arr;
}

void *_upb_array_resize_fallback(upb_array **arr_ptr, size_t size,
                                 upb_fieldtype_t type, upb_arena *arena) {
  upb_array *arr = getorcreate_array(arr_ptr, type, arena);
  return arr && _upb_array_resize(arr, size, arena) ? _upb_array_ptr(arr) : NULL;
}

bool _upb_array_append_fallback(upb_array **arr_ptr, const void *value,
                                upb_fieldtype_t type, upb_arena *arena) {
  upb_array *arr = getorcreate_array(arr_ptr, type, arena);
  size_t elem = arr->len;
  int lg2 = _upb_fieldtype_to_sizelg2[type];
  char *data;

  if (!arr || !_upb_array_resize(arr, elem + 1, arena)) return false;

  data = _upb_array_ptr(arr);
  memcpy(data + (elem << lg2), value, 1 << lg2);
  return true;
}

/** upb_map *******************************************************************/

upb_map *_upb_map_new(upb_arena *a, size_t key_size, size_t value_size) {
  upb_map *map = upb_arena_malloc(a, sizeof(upb_map));

  if (!map) {
    return NULL;
  }

  upb_strtable_init2(&map->table, UPB_CTYPE_INT32, upb_arena_alloc(a));
  map->key_size = key_size;
  map->val_size = value_size;

  return map;
}
