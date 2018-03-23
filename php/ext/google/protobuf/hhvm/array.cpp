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
  intern->arena = NULL;
  intern->wrappers = NULL;
}

void RepeatedField_deepclean(upb_array *array, const upb_msgdef *m) {
}

void RepeatedField_free_c(
    RepeatedField *intern TSRMLS_DC) {
  if (upb_array_type(intern->array) == UPB_TYPE_MESSAGE) {
    for (std::unordered_map<void*, PHP_OBJECT>::iterator i =
             intern->wrappers->begin();
         i != intern->wrappers->end(); ++i) {
      PHP_OBJECT_FREE(i->second);
    }
    delete intern->wrappers;
  }
  ARENA_DTOR(intern->arena);
}

void RepeatedField_wrap(RepeatedField *intern, upb_array *arr,
                        void *klass) {
  intern->array = arr;
  intern->klass = klass;
}

void RepeatedField___construct(RepeatedField *intern,
                               upb_descriptortype_t type,
                               ARENA arena_parent,
                               void *klass) {
  upb_arena* arena;
  if (arena_parent == NULL) {
    ARENA_INIT(intern->arena, arena);
  } else {
    intern->arena = arena_parent;
    ARENA_ADDREF(intern->arena);
    Arena* cpparena = UNBOX_ARENA(intern->arena);
    arena = cpparena->arena;
  }

  intern->array = upb_array_new(to_fieldtype(type), upb_arena_alloc(arena));
  intern->klass = klass;

  if (upb_array_type(intern->array) == UPB_TYPE_MESSAGE) {
    intern->wrappers = new std::unordered_map<void*, PHP_OBJECT>;
  }
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
