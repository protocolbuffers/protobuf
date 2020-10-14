
#include "upb/msg.h"

#include "upb/table.int.h"

#include "upb/port_def.inc"

/** upb_msg *******************************************************************/

static const size_t overhead = sizeof(upb_msg_internal);

static const upb_msg_internal *upb_msg_getinternal_const(const upb_msg *msg) {
  ptrdiff_t size = sizeof(upb_msg_internal);
  return UPB_PTR_AT(msg, -size, upb_msg_internal);
}

upb_msg *_upb_msg_new(const upb_msglayout *l, upb_arena *a) {
  return _upb_msg_new_inl(l, a);
}

void _upb_msg_clear(upb_msg *msg, const upb_msglayout *l) {
  void *mem = UPB_PTR_AT(msg, -sizeof(upb_msg_internal), char);
  memset(mem, 0, upb_msg_sizeof(l));
}

bool _upb_msg_addunknown(upb_msg *msg, const char *data, size_t len,
                         upb_arena *arena) {

  upb_msg_internal *in = upb_msg_getinternal(msg);
  if (!in->unknown) {
    size_t size = 128;
    while (size < len) size *= 2;
    in->unknown = upb_arena_malloc(arena, size + overhead);
    if (!in->unknown) return false;
    in->unknown->size = size;
    in->unknown->len = 0;
  } else if (in->unknown->size - in->unknown->len < len) {
    size_t need = in->unknown->len + len;
    size_t size = in->unknown->size;
    while (size < need)  size *= 2;
    in->unknown = upb_arena_realloc(
        arena, in->unknown, in->unknown->size + overhead, size + overhead);
    if (!in->unknown) return false;
    in->unknown->size = size;
  }
  memcpy(UPB_PTR_AT(in->unknown + 1, in->unknown->len, char), data, len);
  in->unknown->len += len;
  return true;
}

void _upb_msg_discardunknown_shallow(upb_msg *msg) {
  upb_msg_internal *in = upb_msg_getinternal(msg);
  if (in->unknown) {
    in->unknown->len = 0;
  }
}

const char *upb_msg_getunknown(const upb_msg *msg, size_t *len) {
  const upb_msg_internal *in = upb_msg_getinternal_const(msg);
  if (in->unknown) {
    *len = in->unknown->len;
    return (char*)(in->unknown + 1);
  } else {
    *len = 0;
    return NULL;
  }
}

/** upb_array *****************************************************************/

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

  arr->data = _upb_tag_arrptr(ptr, elem_size_lg2);
  arr->size = new_size;
  return true;
}

static upb_array *getorcreate_array(upb_array **arr_ptr, int elem_size_lg2,
                                    upb_arena *arena) {
  upb_array *arr = *arr_ptr;
  if (!arr) {
    arr = _upb_array_new(arena, 4, elem_size_lg2);
    if (!arr) return NULL;
    *arr_ptr = arr;
  }
  return arr;
}

void *_upb_array_resize_fallback(upb_array **arr_ptr, size_t size,
                                 int elem_size_lg2, upb_arena *arena) {
  upb_array *arr = getorcreate_array(arr_ptr, elem_size_lg2, arena);
  return arr && _upb_array_resize(arr, size, arena) ? _upb_array_ptr(arr)
                                                    : NULL;
}

bool _upb_array_append_fallback(upb_array **arr_ptr, const void *value,
                                int elem_size_lg2, upb_arena *arena) {
  upb_array *arr = getorcreate_array(arr_ptr, elem_size_lg2, arena);
  size_t elem = arr->len;
  char *data;

  if (!arr || !_upb_array_resize(arr, elem + 1, arena)) return false;

  data = _upb_array_ptr(arr);
  memcpy(data + (elem << elem_size_lg2), value, 1 << elem_size_lg2);
  return true;
}

/** upb_map *******************************************************************/

upb_map *_upb_map_new(upb_arena *a, size_t key_size, size_t value_size) {
  upb_map *map = upb_arena_malloc(a, sizeof(upb_map));

  if (!map) {
    return NULL;
  }

  upb_strtable_init2(&map->table, UPB_CTYPE_INT32, 4, upb_arena_alloc(a));
  map->key_size = key_size;
  map->val_size = value_size;

  return map;
}
