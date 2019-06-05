
#include "upb/legacy_msg_reflection.h"

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

/** upb_msgval ****************************************************************/

/* These functions will generate real memcpy() calls on ARM sadly, because
 * the compiler assumes they might not be aligned. */

static upb_msgval upb_msgval_read(const void *p, size_t ofs,
                                  uint8_t size) {
  upb_msgval val;
  p = (char*)p + ofs;
  memcpy(&val, p, size);
  return val;
}

static void upb_msgval_write(void *p, size_t ofs, upb_msgval val,
                             uint8_t size) {
  p = (char*)p + ofs;
  memcpy(p, &val, size);
}

static size_t upb_msgval_sizeof(upb_fieldtype_t type) {
  switch (type) {
    case UPB_TYPE_DOUBLE:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT64:
      return 8;
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_FLOAT:
      return 4;
    case UPB_TYPE_BOOL:
      return 1;
    case UPB_TYPE_MESSAGE:
      return sizeof(void*);
    case UPB_TYPE_BYTES:
    case UPB_TYPE_STRING:
      return sizeof(upb_strview);
  }
  UPB_UNREACHABLE();
}

static uint8_t upb_msg_fieldsize(const upb_msglayout_field *field) {
  if (field->label == UPB_LABEL_REPEATED) {
    return sizeof(void*);
  } else {
    return upb_msgval_sizeof(upb_desctype_to_fieldtype[field->descriptortype]);
  }
}

/* TODO(haberman): this is broken right now because upb_msgval can contain
 * a char* / size_t pair, which is too big for a upb_value.  To fix this
 * we'll probably need to dynamically allocate a upb_msgval and store a
 * pointer to that in the tables for extensions/maps. */
static upb_value upb_toval(upb_msgval val) {
  upb_value ret;
  UPB_UNUSED(val);
  memset(&ret, 0, sizeof(upb_value));  /* XXX */
  return ret;
}

static upb_msgval upb_msgval_fromval(upb_value val) {
  upb_msgval ret;
  UPB_UNUSED(val);
  memset(&ret, 0, sizeof(upb_msgval));  /* XXX */
  return ret;
}

static upb_ctype_t upb_fieldtotabtype(upb_fieldtype_t type) {
  switch (type) {
    case UPB_TYPE_FLOAT: return UPB_CTYPE_FLOAT;
    case UPB_TYPE_DOUBLE: return UPB_CTYPE_DOUBLE;
    case UPB_TYPE_BOOL: return UPB_CTYPE_BOOL;
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE:
    case UPB_TYPE_STRING: return UPB_CTYPE_CONSTPTR;
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32: return UPB_CTYPE_INT32;
    case UPB_TYPE_UINT32: return UPB_CTYPE_UINT32;
    case UPB_TYPE_INT64: return UPB_CTYPE_INT64;
    case UPB_TYPE_UINT64: return UPB_CTYPE_UINT64;
    default: UPB_ASSERT(false); return 0;
  }
}


/** upb_msg *******************************************************************/

/* If we always read/write as a consistent type to each address, this shouldn't
 * violate aliasing.
 */
#define DEREF(msg, ofs, type) *PTR_AT(msg, ofs, type)

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


/** upb_array *****************************************************************/

#define DEREF_ARR(arr, i, type) ((type*)arr->data)[i]

size_t upb_array_size(const upb_array *arr) {
  return arr->len;
}

upb_msgval upb_array_get(const upb_array *arr, upb_fieldtype_t type, size_t i) {
  size_t element_size = upb_msgval_sizeof(type);
  UPB_ASSERT(i < arr->len);
  return upb_msgval_read(arr->data, i * element_size, element_size);
}

bool upb_array_set(upb_array *arr, upb_fieldtype_t type, size_t i,
                   upb_msgval val, upb_arena *arena) {
  size_t element_size = upb_msgval_sizeof(type);
  UPB_ASSERT(i <= arr->len);

  if (i == arr->len) {
    /* Extending the array. */

    if (i == arr->size) {
      /* Need to reallocate. */
      size_t new_size = UPB_MAX(arr->size * 2, 8);
      size_t new_bytes = new_size * element_size;
      size_t old_bytes = arr->size * element_size;
      upb_alloc *alloc = upb_arena_alloc(arena);
      upb_msgval *new_data =
          upb_realloc(alloc, arr->data, old_bytes, new_bytes);

      if (!new_data) {
        return false;
      }

      arr->data = new_data;
      arr->size = new_size;
    }

    arr->len = i + 1;
  }

  upb_msgval_write(arr->data, i * element_size, val, element_size);
  return true;
}

/** upb_map *******************************************************************/

struct upb_map {
  upb_fieldtype_t key_type;
  upb_fieldtype_t val_type;
  /* We may want to optimize this to use inttable where possible, for greater
   * efficiency and lower memory footprint. */
  upb_strtable strtab;
  upb_arena *arena;
};

static void upb_map_tokey(upb_fieldtype_t type, upb_msgval *key,
                          const char **out_key, size_t *out_len) {
  switch (type) {
    case UPB_TYPE_STRING:
      /* Point to string data of the input key. */
      *out_key = key->str.data;
      *out_len = key->str.size;
      return;
    case UPB_TYPE_BOOL:
    case UPB_TYPE_INT32:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT64:
      /* Point to the key itself.  XXX: big-endian. */
      *out_key = (const char*)key;
      *out_len = upb_msgval_sizeof(type);
      return;
    case UPB_TYPE_BYTES:
    case UPB_TYPE_DOUBLE:
    case UPB_TYPE_ENUM:
    case UPB_TYPE_FLOAT:
    case UPB_TYPE_MESSAGE:
      break;  /* Cannot be a map key. */
  }
  UPB_UNREACHABLE();
}

static upb_msgval upb_map_fromkey(upb_fieldtype_t type, const char *key,
                                  size_t len) {
  switch (type) {
    case UPB_TYPE_STRING:
      return upb_msgval_makestr(key, len);
    case UPB_TYPE_BOOL:
    case UPB_TYPE_INT32:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT64:
      return upb_msgval_read(key, 0, upb_msgval_sizeof(type));
    case UPB_TYPE_BYTES:
    case UPB_TYPE_DOUBLE:
    case UPB_TYPE_ENUM:
    case UPB_TYPE_FLOAT:
    case UPB_TYPE_MESSAGE:
      break;  /* Cannot be a map key. */
  }
  UPB_UNREACHABLE();
}

upb_map *upb_map_new(upb_fieldtype_t ktype, upb_fieldtype_t vtype,
                     upb_arena *a) {
  upb_ctype_t vtabtype = upb_fieldtotabtype(vtype);
  upb_alloc *alloc = upb_arena_alloc(a);
  upb_map *map = upb_malloc(alloc, sizeof(upb_map));

  if (!map) {
    return NULL;
  }

  UPB_ASSERT(upb_fieldtype_mapkeyok(ktype));
  map->key_type = ktype;
  map->val_type = vtype;
  map->arena = a;

  if (!upb_strtable_init2(&map->strtab, vtabtype, alloc)) {
    return NULL;
  }

  return map;
}

size_t upb_map_size(const upb_map *map) {
  return upb_strtable_count(&map->strtab);
}

upb_fieldtype_t upb_map_keytype(const upb_map *map) {
  return map->key_type;
}

upb_fieldtype_t upb_map_valuetype(const upb_map *map) {
  return map->val_type;
}

bool upb_map_get(const upb_map *map, upb_msgval key, upb_msgval *val) {
  upb_value tabval;
  const char *key_str;
  size_t key_len;
  bool ret;

  upb_map_tokey(map->key_type, &key, &key_str, &key_len);
  ret = upb_strtable_lookup2(&map->strtab, key_str, key_len, &tabval);
  if (ret) {
    memcpy(val, &tabval, sizeof(tabval));
  }

  return ret;
}

bool upb_map_set(upb_map *map, upb_msgval key, upb_msgval val,
                 upb_msgval *removed) {
  const char *key_str;
  size_t key_len;
  upb_value tabval = upb_toval(val);
  upb_value removedtabval;
  upb_alloc *a = upb_arena_alloc(map->arena);

  upb_map_tokey(map->key_type, &key, &key_str, &key_len);

  /* TODO(haberman): add overwrite operation to minimize number of lookups. */
  if (upb_strtable_lookup2(&map->strtab, key_str, key_len, NULL)) {
    upb_strtable_remove3(&map->strtab, key_str, key_len, &removedtabval, a);
    memcpy(&removed, &removedtabval, sizeof(removed));
  }

  return upb_strtable_insert3(&map->strtab, key_str, key_len, tabval, a);
}

bool upb_map_del(upb_map *map, upb_msgval key) {
  const char *key_str;
  size_t key_len;
  upb_alloc *a = upb_arena_alloc(map->arena);

  upb_map_tokey(map->key_type, &key, &key_str, &key_len);
  return upb_strtable_remove3(&map->strtab, key_str, key_len, NULL, a);
}


/** upb_mapiter ***************************************************************/

struct upb_mapiter {
  upb_strtable_iter iter;
  upb_fieldtype_t key_type;
};

size_t upb_mapiter_sizeof() {
  return sizeof(upb_mapiter);
}

void upb_mapiter_begin(upb_mapiter *i, const upb_map *map) {
  upb_strtable_begin(&i->iter, &map->strtab);
  i->key_type = map->key_type;
}

upb_mapiter *upb_mapiter_new(const upb_map *t, upb_alloc *a) {
  upb_mapiter *ret = upb_malloc(a, upb_mapiter_sizeof());

  if (!ret) {
    return NULL;
  }

  upb_mapiter_begin(ret, t);
  return ret;
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
  return upb_map_fromkey(i->key_type, upb_strtable_iter_key(&i->iter),
                         upb_strtable_iter_keylength(&i->iter));
}

upb_msgval upb_mapiter_value(const upb_mapiter *i) {
  return upb_msgval_fromval(upb_strtable_iter_value(&i->iter));
}

void upb_mapiter_setdone(upb_mapiter *i) {
  upb_strtable_iter_setdone(&i->iter);
}

bool upb_mapiter_isequal(const upb_mapiter *i1, const upb_mapiter *i2) {
  return upb_strtable_iter_isequal(&i1->iter, &i2->iter);
}
