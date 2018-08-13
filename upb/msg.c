
#include "upb/msg.h"
#include "upb/structs.int.h"

static bool is_power_of_two(size_t val) {
  return (val & (val - 1)) == 0;
}

/* Align up to the given power of 2. */
static size_t align_up(size_t val, size_t align) {
  UPB_ASSERT(is_power_of_two(align));
  return (val + align - 1) & ~(align - 1);
}

static size_t div_round_up(size_t n, size_t d) {
  return (n + d - 1) / d;
}

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

#define upb_alignof(t) offsetof(struct { char c; t x; }, x)

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
      return sizeof(upb_stringview);
  }
  UPB_UNREACHABLE();
}

static uint8_t upb_msg_fieldsize(const upb_msglayout_fieldinit_v1 *field) {
  if (field->label == UPB_LABEL_REPEATED) {
    return sizeof(void*);
  } else {
    return upb_msgval_sizeof(upb_desctype_to_fieldtype[field->descriptortype]);
  }
}

static uint8_t upb_msg_fielddefsize(const upb_fielddef *f) {
  if (upb_fielddef_isseq(f)) {
    return sizeof(void*);
  } else {
    return upb_msgval_sizeof(upb_fielddef_type(f));
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

static upb_msgval upb_msgval_fromdefault(const upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
      case UPB_TYPE_FLOAT:
        return upb_msgval_float(upb_fielddef_defaultfloat(f));
      case UPB_TYPE_DOUBLE:
        return upb_msgval_double(upb_fielddef_defaultdouble(f));
      case UPB_TYPE_BOOL:
        return upb_msgval_bool(upb_fielddef_defaultbool(f));
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES: {
        size_t len;
        const char *ptr = upb_fielddef_defaultstr(f, &len);
        return upb_msgval_makestr(ptr, len);
      }
      case UPB_TYPE_MESSAGE:
        return upb_msgval_msg(NULL);
      case UPB_TYPE_ENUM:
      case UPB_TYPE_INT32:
        return upb_msgval_int32(upb_fielddef_defaultint32(f));
      case UPB_TYPE_UINT32:
        return upb_msgval_uint32(upb_fielddef_defaultuint32(f));
      case UPB_TYPE_INT64:
        return upb_msgval_int64(upb_fielddef_defaultint64(f));
      case UPB_TYPE_UINT64:
        return upb_msgval_uint64(upb_fielddef_defaultuint64(f));
      default:
        UPB_ASSERT(false);
        return upb_msgval_msg(NULL);
  }
}


/** upb_msglayout *************************************************************/

struct upb_msglayout {
  struct upb_msglayout_msginit_v1 data;
};

static void upb_msglayout_free(upb_msglayout *l) {
  upb_gfree(l->data.default_msg);
  upb_gfree(l);
}

static size_t upb_msglayout_place(upb_msglayout *l, size_t size) {
  size_t ret;

  l->data.size = align_up(l->data.size, size);
  ret = l->data.size;
  l->data.size += size;
  return ret;
}

static bool upb_msglayout_initdefault(upb_msglayout *l, const upb_msgdef *m) {
  upb_msg_field_iter it;

  if (upb_msgdef_syntax(m) == UPB_SYNTAX_PROTO2 && l->data.size) {
    /* Allocate default message and set default values in it. */
    l->data.default_msg = upb_gmalloc(l->data.size);
    if (!l->data.default_msg) {
      return false;
    }

    memset(l->data.default_msg, 0, l->data.size);

    for (upb_msg_field_begin(&it, m); !upb_msg_field_done(&it);
         upb_msg_field_next(&it)) {
      const upb_fielddef* f = upb_msg_iter_field(&it);

      if (upb_fielddef_containingoneof(f)) {
        continue;
      }

      /* TODO(haberman): handle strings. */
      if (!upb_fielddef_isstring(f) &&
          !upb_fielddef_issubmsg(f) &&
          !upb_fielddef_isseq(f)) {
        upb_msg_set(l->data.default_msg,
                    upb_fielddef_index(f),
                    upb_msgval_fromdefault(f),
                    l);
      }
    }
  }

  return true;
}

static bool upb_msglayout_init(const upb_msgdef *m,
                               upb_msglayout *l,
                               upb_msgfactory *factory) {
  upb_msg_field_iter it;
  upb_msg_oneof_iter oit;
  size_t hasbit;
  size_t submsg_count = 0;
  const upb_msglayout_msginit_v1 **submsgs;
  upb_msglayout_fieldinit_v1 *fields;
  upb_msglayout_oneofinit_v1 *oneofs;

  for (upb_msg_field_begin(&it, m);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* f = upb_msg_iter_field(&it);
    if (upb_fielddef_issubmsg(f)) {
      submsg_count++;
    }
  }

  memset(l, 0, sizeof(*l));

  fields = upb_gmalloc(upb_msgdef_numfields(m) * sizeof(*fields));
  submsgs = upb_gmalloc(submsg_count * sizeof(*submsgs));
  oneofs = upb_gmalloc(upb_msgdef_numoneofs(m) * sizeof(*oneofs));

  if ((!fields && upb_msgdef_numfields(m)) ||
      (!submsgs && submsg_count) ||
      (!oneofs && upb_msgdef_numoneofs(m))) {
    /* OOM. */
    upb_gfree(fields);
    upb_gfree(submsgs);
    upb_gfree(oneofs);
    return false;
  }

  l->data.field_count = upb_msgdef_numfields(m);
  l->data.oneof_count = upb_msgdef_numoneofs(m);
  l->data.fields = fields;
  l->data.submsgs = submsgs;
  l->data.oneofs = oneofs;
  l->data.is_proto2 = (upb_msgdef_syntax(m) == UPB_SYNTAX_PROTO2);

  /* Allocate data offsets in three stages:
   *
   * 1. hasbits.
   * 2. regular fields.
   * 3. oneof fields.
   *
   * OPT: There is a lot of room for optimization here to minimize the size.
   */

  /* Allocate hasbits and set basic field attributes. */
  submsg_count = 0;
  for (upb_msg_field_begin(&it, m), hasbit = 0;
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* f = upb_msg_iter_field(&it);
    upb_msglayout_fieldinit_v1 *field = &fields[upb_fielddef_index(f)];

    field->number = upb_fielddef_number(f);
    field->descriptortype = upb_fielddef_descriptortype(f);
    field->label = upb_fielddef_label(f);

    if (upb_fielddef_containingoneof(f)) {
      field->oneof_index = upb_oneofdef_index(upb_fielddef_containingoneof(f));
    } else {
      field->oneof_index = UPB_NOT_IN_ONEOF;
    }

    if (upb_fielddef_issubmsg(f)) {
      const upb_msglayout *sub_layout =
          upb_msgfactory_getlayout(factory, upb_fielddef_msgsubdef(f));
      field->submsg_index = submsg_count++;
      submsgs[field->submsg_index] = &sub_layout->data;
    } else {
      field->submsg_index = UPB_NO_SUBMSG;
    }

    if (upb_fielddef_haspresence(f) && !upb_fielddef_containingoneof(f)) {
      field->hasbit = hasbit++;
    } else {
      field->hasbit = UPB_NO_HASBIT;
    }
  }

  /* Account for space used by hasbits. */
  l->data.size = div_round_up(hasbit, 8);

  /* Allocate non-oneof fields. */
  for (upb_msg_field_begin(&it, m); !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* f = upb_msg_iter_field(&it);
    size_t field_size = upb_msg_fielddefsize(f);
    size_t index = upb_fielddef_index(f);

    if (upb_fielddef_containingoneof(f)) {
      /* Oneofs are handled separately below. */
      continue;
    }

    fields[index].offset = upb_msglayout_place(l, field_size);
  }

  /* Allocate oneof fields.  Each oneof field consists of a uint32 for the case
   * and space for the actual data. */
  for (upb_msg_oneof_begin(&oit, m); !upb_msg_oneof_done(&oit);
       upb_msg_oneof_next(&oit)) {
    const upb_oneofdef* o = upb_msg_iter_oneof(&oit);
    upb_oneof_iter fit;

    size_t case_size = sizeof(uint32_t);  /* Could potentially optimize this. */
    upb_msglayout_oneofinit_v1 *oneof = &oneofs[upb_oneofdef_index(o)];
    size_t field_size = 0;

    /* Calculate field size: the max of all field sizes. */
    for (upb_oneof_begin(&fit, o);
         !upb_oneof_done(&fit);
         upb_oneof_next(&fit)) {
      const upb_fielddef* f = upb_oneof_iter_field(&fit);
      field_size = UPB_MAX(field_size, upb_msg_fielddefsize(f));
    }

    /* Align and allocate case offset. */
    oneof->case_offset = upb_msglayout_place(l, case_size);
    oneof->data_offset = upb_msglayout_place(l, field_size);
  }

  /* Size of the entire structure should be a multiple of its greatest
   * alignment.  TODO: track overall alignment for real? */
  l->data.size = align_up(l->data.size, 8);

  return upb_msglayout_initdefault(l, m);
}


/** upb_msgfactory ************************************************************/

struct upb_msgfactory {
  const upb_symtab *symtab;  /* We own a ref. */
  upb_inttable layouts;
  upb_inttable mergehandlers;
};

upb_msgfactory *upb_msgfactory_new(const upb_symtab *symtab) {
  upb_msgfactory *ret = upb_gmalloc(sizeof(*ret));

  ret->symtab = symtab;
  upb_inttable_init(&ret->layouts, UPB_CTYPE_PTR);
  upb_inttable_init(&ret->mergehandlers, UPB_CTYPE_CONSTPTR);

  return ret;
}

void upb_msgfactory_free(upb_msgfactory *f) {
  upb_inttable_iter i;
  upb_inttable_begin(&i, &f->layouts);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_msglayout *l = upb_value_getptr(upb_inttable_iter_value(&i));
    upb_msglayout_free(l);
  }

  upb_inttable_begin(&i, &f->mergehandlers);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    const upb_handlers *h = upb_value_getconstptr(upb_inttable_iter_value(&i));
    upb_handlers_unref(h, f);
  }

  upb_inttable_uninit(&f->layouts);
  upb_inttable_uninit(&f->mergehandlers);
  upb_gfree(f);
}

const upb_symtab *upb_msgfactory_symtab(const upb_msgfactory *f) {
  return f->symtab;
}

const upb_msglayout *upb_msgfactory_getlayout(upb_msgfactory *f,
                                              const upb_msgdef *m) {
  upb_value v;
  UPB_ASSERT(upb_symtab_lookupmsg(f->symtab, upb_msgdef_fullname(m)) == m);
  UPB_ASSERT(!upb_msgdef_mapentry(m));

  if (upb_inttable_lookupptr(&f->layouts, m, &v)) {
    UPB_ASSERT(upb_value_getptr(v));
    return upb_value_getptr(v);
  } else {
    /* In case of circular dependency, layout has to be inserted first. */
    upb_msglayout *l = upb_gmalloc(sizeof(*l));
    upb_msgfactory *mutable_f = (void*)f;
    upb_inttable_insertptr(&mutable_f->layouts, m, upb_value_ptr(l));
    UPB_ASSERT(l);
    if (!upb_msglayout_init(m, l, f)) {
      upb_msglayout_free(l);
    }
    return l;
  }
}


/** upb_msg *******************************************************************/

/* If we always read/write as a consistent type to each address, this shouldn't
 * violate aliasing.
 */
#define DEREF(msg, ofs, type) *PTR_AT(msg, ofs, type)

/* Internal members of a upb_msg.  We can change this without breaking binary
 * compatibility.  We put these before the user's data.  The user's upb_msg*
 * points after the upb_msg_internal. */

/* Used when a message is not extendable. */
typedef struct {
  /* TODO(haberman): add unknown fields. */
  upb_arena *arena;
} upb_msg_internal;

/* Used when a message is extendable. */
typedef struct {
  upb_inttable *extdict;
  upb_msg_internal base;
} upb_msg_internal_withext;

static int upb_msg_internalsize(const upb_msglayout *l) {
    return sizeof(upb_msg_internal) - l->data.extendable * sizeof(void*);
}

static upb_msg_internal *upb_msg_getinternal(upb_msg *msg) {
  return VOIDPTR_AT(msg, -sizeof(upb_msg_internal));
}

static const upb_msg_internal *upb_msg_getinternal_const(const upb_msg *msg) {
  return VOIDPTR_AT(msg, -sizeof(upb_msg_internal));
}

static upb_msg_internal_withext *upb_msg_getinternalwithext(
    upb_msg *msg, const upb_msglayout *l) {
  UPB_ASSERT(l->data.extendable);
  return VOIDPTR_AT(msg, -sizeof(upb_msg_internal_withext));
}

static const upb_msglayout_fieldinit_v1 *upb_msg_checkfield(
    int field_index, const upb_msglayout *l) {
  UPB_ASSERT(field_index >= 0 && field_index < l->data.field_count);
  return &l->data.fields[field_index];
}

static bool upb_msg_inoneof(const upb_msglayout_fieldinit_v1 *field) {
  return field->oneof_index != UPB_NOT_IN_ONEOF;
}

static uint32_t *upb_msg_oneofcase(const upb_msg *msg, int field_index,
                                   const upb_msglayout *l) {
  const upb_msglayout_fieldinit_v1 *field = upb_msg_checkfield(field_index, l);
  UPB_ASSERT(upb_msg_inoneof(field));
  return PTR_AT(msg, l->data.oneofs[field->oneof_index].case_offset, uint32_t);
}

static size_t upb_msg_sizeof(const upb_msglayout *l) {
  return l->data.size + upb_msg_internalsize(l);
}

upb_msg *upb_msg_new(const upb_msglayout *l, upb_arena *a) {
  upb_alloc *alloc = upb_arena_alloc(a);
  void *mem = upb_malloc(alloc, upb_msg_sizeof(l));
  upb_msg *msg;

  if (!mem) {
    return NULL;
  }

  msg = VOIDPTR_AT(mem, upb_msg_internalsize(l));

  /* Initialize normal members. */
  if (l->data.default_msg) {
    memcpy(msg, l->data.default_msg, l->data.size);
  } else {
    memset(msg, 0, l->data.size);
  }

  /* Initialize internal members. */
  upb_msg_getinternal(msg)->arena = a;

  if (l->data.extendable) {
    upb_msg_getinternalwithext(msg, l)->extdict = NULL;
  }

  return msg;
}

upb_arena *upb_msg_arena(const upb_msg *msg) {
  return upb_msg_getinternal_const(msg)->arena;
}

bool upb_msg_has(const upb_msg *msg,
                 int field_index,
                 const upb_msglayout *l) {
  const upb_msglayout_fieldinit_v1 *field = upb_msg_checkfield(field_index, l);

  UPB_ASSERT(l->data.is_proto2);

  if (upb_msg_inoneof(field)) {
    /* Oneofs are set when the oneof number is set to this field. */
    return *upb_msg_oneofcase(msg, field_index, l) == field->number;
  } else {
    /* Other fields are set when their hasbit is set. */
    uint32_t hasbit = l->data.fields[field_index].hasbit;
    return DEREF(msg, hasbit / 8, char) | (1 << (hasbit % 8));
  }
}

upb_msgval upb_msg_get(const upb_msg *msg, int field_index,
                       const upb_msglayout *l) {
  const upb_msglayout_fieldinit_v1 *field = upb_msg_checkfield(field_index, l);
  int size = upb_msg_fieldsize(field);

  if (upb_msg_inoneof(field)) {
    if (*upb_msg_oneofcase(msg, field_index, l) == field->number) {
      size_t ofs = l->data.oneofs[field->oneof_index].data_offset;
      return upb_msgval_read(msg, ofs, size);
    } else {
      /* Return default. */
      return upb_msgval_read(l->data.default_msg, field->offset, size);
    }
  } else {
    return upb_msgval_read(msg, field->offset, size);
  }
}

void upb_msg_set(upb_msg *msg, int field_index, upb_msgval val,
                 const upb_msglayout *l) {
  const upb_msglayout_fieldinit_v1 *field = upb_msg_checkfield(field_index, l);
  int size = upb_msg_fieldsize(field);

  if (upb_msg_inoneof(field)) {
    size_t ofs = l->data.oneofs[field->oneof_index].data_offset;
    *upb_msg_oneofcase(msg, field_index, l) = field->number;
    upb_msgval_write(msg, ofs, val, size);
  } else {
    upb_msgval_write(msg, field->offset, val, size);
  }
}


/** upb_array *****************************************************************/

#define DEREF_ARR(arr, i, type) ((type*)arr->data)[i]

upb_array *upb_array_new(upb_fieldtype_t type, upb_arena *a) {
  upb_alloc *alloc = upb_arena_alloc(a);
  upb_array *ret = upb_malloc(alloc, sizeof(upb_array));

  if (!ret) {
    return NULL;
  }

  ret->type = type;
  ret->data = NULL;
  ret->len = 0;
  ret->size = 0;
  ret->element_size = upb_msgval_sizeof(type);
  ret->arena = a;

  return ret;
}

size_t upb_array_size(const upb_array *arr) {
  return arr->len;
}

upb_fieldtype_t upb_array_type(const upb_array *arr) {
  return arr->type;
}

upb_msgval upb_array_get(const upb_array *arr, size_t i) {
  UPB_ASSERT(i < arr->len);
  return upb_msgval_read(arr->data, i * arr->element_size, arr->element_size);
}

bool upb_array_set(upb_array *arr, size_t i, upb_msgval val) {
  UPB_ASSERT(i <= arr->len);

  if (i == arr->len) {
    /* Extending the array. */

    if (i == arr->size) {
      /* Need to reallocate. */
      size_t new_size = UPB_MAX(arr->size * 2, 8);
      size_t new_bytes = new_size * arr->element_size;
      size_t old_bytes = arr->size * arr->element_size;
      upb_alloc *alloc = upb_arena_alloc(arr->arena);
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

  upb_msgval_write(arr->data, i * arr->element_size, val, arr->element_size);
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
