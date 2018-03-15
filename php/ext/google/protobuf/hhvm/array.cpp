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

void RepeatedField_free_c(
    RepeatedField *intern TSRMLS_DC) {
}

void RepeatedField_wrap(RepeatedField *intern, upb_array *arr, void *klass) {
  intern->array = arr;
  intern->klass = klass;
}

void RepeatedField___construct(RepeatedField *intern,
                               upb_descriptortype_t type,
                               void *klass) {
  upb_arena* arena = reinterpret_cast<upb_arena*>(ALLOC(upb_arena));
  upb_arena_init(arena);
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
