
#include "upb/reflection.h"

#include <string.h>
#include "upb/table.int.h"
#include "upb/msg.h"

#include "upb/port_def.inc"

bool upb_fieldtype_mapkeyok(upb_fieldtype_t type) {
  return type == UPB_TYPE_BOOL || type == UPB_TYPE_INT32 ||
         type == UPB_TYPE_UINT32 || type == UPB_TYPE_INT64 ||
         type == UPB_TYPE_UINT64 || type == UPB_TYPE_STRING;
}

#define PTR_AT(msg, ofs, type) (type*)((char*)msg + ofs)
#define VOIDPTR_AT(msg, ofs) PTR_AT(msg, ofs, void)
#define ENCODE_MAX_NESTING 64
#define CHECK_TRUE(x) if (!(x)) { return false; }


/** upb_msg *******************************************************************/

/* If we always read/write as a consistent type to each address, this shouldn't
 * violate aliasing.
 */
#define DEREF(msg, ofs, type) *PTR_AT(msg, ofs, type)

upb_msg *upb_msg_new(const upb_msgdef *m, upb_arena *a);

#if 0
static const upb_msglayout_field *upb_msg_checkfield(int field_index,
                                                     const upb_msglayout *l) {
  UPB_ASSERT(field_index >= 0 && field_index < l->field_count);
  return &l->fields[field_index];
}

static bool upb_msg_inoneof(const upb_msglayout_field *field) {
  return field->presence < 0;
}

static uint32_t *upb_msg_oneofcase(const upb_msg *msg, int field_index,
                                   const upb_msglayout *l) {
  const upb_msglayout_field *field = upb_msg_checkfield(field_index, l);
  UPB_ASSERT(upb_msg_inoneof(field));
  return PTR_AT(msg, ~field->presence, uint32_t);
}

bool upb_msg_has(const upb_msg *msg,
                 int field_index,
                 const upb_msglayout *l) {
  const upb_msglayout_field *field = upb_msg_checkfield(field_index, l);

  UPB_ASSERT(field->presence);

  if (upb_msg_inoneof(field)) {
    /* Oneofs are set when the oneof number is set to this field. */
    return *upb_msg_oneofcase(msg, field_index, l) == field->number;
  } else {
    /* Other fields are set when their hasbit is set. */
    uint32_t hasbit = field->presence;
    return DEREF(msg, hasbit / 8, char) | (1 << (hasbit % 8));
  }
}

upb_msgval upb_msg_get(const upb_msg *msg, int field_index,
                       const upb_msglayout *l) {
  const upb_msglayout_field *field = upb_msg_checkfield(field_index, l);
  int size = upb_msg_fieldsize(field);
  return upb_msgval_read(msg, field->offset, size);
}

void upb_msg_set(upb_msg *msg, int field_index, upb_msgval val,
                 const upb_msglayout *l) {
  const upb_msglayout_field *field = upb_msg_checkfield(field_index, l);
  int size = upb_msg_fieldsize(field);
  upb_msgval_write(msg, field->offset, val, size);
}

#undef DEREF
#endif

/** upb_array *****************************************************************/

size_t upb_array_size(const upb_array *arr) {
  return arr->len;
}

upb_msgval upb_array_get(const upb_array *arr, size_t i) {
  UPB_ASSERT(i < arr->len);
  upb_msgval ret;
  const char* data = _upb_array_constptr(arr);
  int lg2 = arr->data & 7;
  memcpy(&ret, data + (i << lg2), 1 << lg2);
  return ret;
}

void upb_array_set(upb_array *arr, size_t i, upb_msgval val) {
  UPB_ASSERT(i < arr->len);
  char* data = _upb_array_ptr(arr);
  int lg2 = arr->data & 7;
  memcpy(data + (i << lg2), &val, 1 << lg2);
}

bool upb_array_append(upb_array *arr, upb_msgval val, upb_arena *arena) {
  if (!_upb_array_realloc(arr, arr->len + 1, arena)) {
    return false;
  }
  arr->len++;
  upb_array_set(arr, arr->len - 1, val);
  return true;
}

/* Resizes the array to the given size, reallocating if necessary, and returns a
 * pointer to the new array elements. */
bool upb_array_resize(upb_array *arr, size_t size, upb_arena *arena) {
  return _upb_array_realloc(arr, size, arena);
}

/** upb_map *******************************************************************/

size_t upb_map_size(const upb_map *map) {
  return upb_strtable_count(&map->table);
}

static upb_strview upb_map_tokey(int size_lg2, upb_msgval *key) {
  if (size_lg2 == UPB_MAPTYPE_STRING) {
    return key->str_val;
  } else {
    return upb_strview_make((const char*)key, 1 << size_lg2);
  }
}

static upb_msgval upb_map_fromvalue(int size_lg2, upb_value val) {
  upb_msgval ret;
  if (size_lg2 == UPB_MAPTYPE_STRING) {
    upb_strview *strp = upb_value_getptr(val);
    ret.str_val = *strp;
  } else {
    memcpy(&ret, &val, 8);
  }
  return ret;
}

static upb_value upb_map_tovalue(int size_lg2, upb_msgval val, upb_arena *a) {
  upb_value ret;
  if (size_lg2 == UPB_MAPTYPE_STRING) {
    upb_strview *strp = upb_arena_malloc(a, sizeof(*strp));
    *strp = val.str_val;
    ret = upb_value_ptr(strp);
  } else {
    memcpy(&ret, &val, 8);
  }
  return ret;
}

bool upb_map_get(const upb_map *map, upb_msgval key, upb_msgval *val) {
  upb_strview strkey = upb_map_tokey(map->key_size_lg2, &key);
  upb_value tabval;
  bool ret;

  ret = upb_strtable_lookup2(&map->table, strkey.data, strkey.size, &tabval);
  if (ret) {
    *val = upb_map_fromvalue(map->val_size_lg2, tabval);
    memcpy(val, &tabval, sizeof(tabval));
  }

  return ret;
}

bool upb_map_set(upb_map *map, upb_msgval key, upb_msgval val,
                 upb_arena *arena) {
  upb_strview strkey = upb_map_tokey(map->key_size_lg2, &key);
  upb_value tabval = upb_map_tovalue(map->val_size_lg2, val, arena);
  upb_alloc *a = upb_arena_alloc(arena);

  /* TODO(haberman): add overwrite operation to minimize number of lookups. */
  if (upb_strtable_lookup2(&map->table, strkey.data, strkey.size, NULL)) {
    upb_strtable_remove3(&map->table, strkey.data, strkey.size, NULL, a);
  }

  return upb_strtable_insert3(&map->table, strkey.data, strkey.size, tabval, a);
}

bool upb_map_delete(upb_map *map, upb_msgval key, upb_arena *arena) {
  upb_strview strkey = upb_map_tokey(map->key_size_lg2, &key);
  upb_alloc *a = upb_arena_alloc(arena);
  return upb_strtable_remove3(&map->table, strkey.data, strkey.size, NULL, a);
}

/** upb_mapiter ***************************************************************/

struct upb_mapiter {
  upb_strtable_iter iter;
  char key_size_lg2;
  char val_size_lg2;
};

size_t upb_mapiter_sizeof(void) {
  return sizeof(upb_mapiter);
}

void upb_mapiter_begin(upb_mapiter *i, upb_map *map) {
  upb_strtable_begin(&i->iter, &map->table);
  i->key_size_lg2 = map->key_size_lg2;
  i->val_size_lg2 = map->val_size_lg2;
}

void upb_mapiter_free(upb_mapiter *i, upb_alloc *a) {
  upb_free(a, i);
}

void upb_mapiter_next(upb_mapiter *i) {
  upb_strtable_next(&i->iter);
}

bool upb_mapiter_done(const upb_mapiter *i) {
  return upb_strtable_done(&i->iter);
}

upb_msgval upb_mapiter_key(const upb_mapiter *i) {
  upb_strview key = upb_strtable_iter_key(&i->iter);
  upb_msgval ret;
  if (i->key_size_lg2 == UPB_MAPTYPE_STRING) {
    ret.str_val = key;
  } else {
    memcpy(&ret, key.data, 1 << i->key_size_lg2);
  }
  return ret;
}

upb_msgval upb_mapiter_value(const upb_mapiter *i) {
  return upb_map_fromvalue(i->val_size_lg2, upb_strtable_iter_value(&i->iter));
}

void upb_mapiter_setdone(upb_mapiter *i) {
  upb_strtable_iter_setdone(&i->iter);
}

bool upb_mapiter_isequal(const upb_mapiter *i1, const upb_mapiter *i2) {
  return upb_strtable_iter_isequal(&i1->iter, &i2->iter);
}
