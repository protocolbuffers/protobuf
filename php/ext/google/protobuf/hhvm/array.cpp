#include "protobuf_cpp.h"
#include "upb.h"

// -----------------------------------------------------------------------------
// Define static methods
// -----------------------------------------------------------------------------

upb_fieldtype_t to_fieldtype(upb_descriptortype_t type) {
  switch (type) {
#define CASE(descriptor_type, type)           \
  case UPB_DESCRIPTOR_TYPE_##descriptor_type: \
    return UPB_TYPE_##type;

  CASE(FLOAT,    FLOAT);
  CASE(DOUBLE,   DOUBLE);
  CASE(BOOL,     BOOL);
  CASE(STRING,   STRING);
  CASE(BYTES,    BYTES);
  CASE(MESSAGE,  MESSAGE);
  CASE(GROUP,    MESSAGE);
  CASE(ENUM,     ENUM);
  CASE(INT32,    INT32);
  CASE(INT64,    INT64);
  CASE(UINT32,   UINT32);
  CASE(UINT64,   UINT64);
  CASE(SINT32,   INT32);
  CASE(SINT64,   INT64);
  CASE(FIXED32,  UINT32);
  CASE(FIXED64,  UINT64);
  CASE(SFIXED32, INT32);
  CASE(SFIXED64, INT64);

#undef CONVERT

  }

  return static_cast<upb_fieldtype_t>(0);
}

// -----------------------------------------------------------------------------
// RepeatedField
// -----------------------------------------------------------------------------

void RepeatedField_init_c_instance(
    RepeatedField *intern TSRMLS_DC) {
  intern->array = NULL;
  intern->klass = NULL;
}

void RepeatedField_deepclean(upb_array *array, const upb_msgdef *m) {
  if (array != NULL) {
    if (upb_array_type(array) == UPB_TYPE_MESSAGE) {
      for (int i = 0; i < upb_array_size(array); i++) {
        upb_msgval msgval = upb_array_get(array, i);
        upb_msg *msg = (upb_msg*)upb_msgval_getmsg(msgval);
        Message_deepclean(msg, m);
      }
    }
    PHP_OBJECT_FREE(upb_array_getalloc(array));
  }
}

void RepeatedField_free_c(
    RepeatedField *intern TSRMLS_DC) {
  RepeatedField_deepclean(intern->array, class2msgdef(intern->klass));
}

void RepeatedField_wrap(RepeatedField *intern, upb_array *arr,
                        void *klass) {
  intern->array = arr;
  intern->klass = klass;

  PHP_OBJECT_ADDREF(upb_array_getalloc(arr));
}

void RepeatedField___construct(RepeatedField *intern,
                               upb_descriptortype_t type,
                               upb_arena *arena_parent,
                               void *klass) {
  upb_arena* arena;
  if (arena_parent == NULL) {
    PHP_OBJECT_NEW(arena, Arena);
  } else {
    arena = arena_parent;
    PHP_OBJECT_ADDREF(arena_parent);
  }

  intern->array = upb_array_new(to_fieldtype(type), upb_arena_alloc(arena));
  intern->klass = klass;
}

// -----------------------------------------------------------------------------
// RepeatedFieldIter
// -----------------------------------------------------------------------------

void RepeatedFieldIter_init_c_instance(
    RepeatedFieldIter *intern TSRMLS_DC) {
  intern->repeated_field = NULL;
  intern->position = 0;
}

void RepeatedFieldIter_free_c(
    RepeatedFieldIter *intern TSRMLS_DC) {
}
