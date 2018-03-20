#include "protobuf_cpp.h"
#include "upb.h"

// -----------------------------------------------------------------------------
// MapField
// -----------------------------------------------------------------------------

void MapField_init_c_instance(
    MapField *intern TSRMLS_DC) {
  intern->map = NULL;
  intern->klass = NULL;
}

void MapField_deepclean(upb_map *map, const upb_msgdef *m) {
  if (map != NULL) {
    PHP_OBJECT_FREE(upb_map_getalloc(map));
  }
}

void MapField_free_c(
    MapField *intern TSRMLS_DC) {
  if (intern->map != NULL) {
    PHP_OBJECT_FREE(upb_map_getalloc(intern->map));
  }
}

void MapField_wrap(MapField *intern, upb_map *map, void *klass) {
  intern->map = map;
  intern->klass = klass;

  PHP_OBJECT_ADDREF(upb_map_getalloc(map));
}

void MapField___construct(MapField *intern,
                          upb_descriptortype_t key_type,
                          upb_descriptortype_t value_type,
                          void *klass) {
  upb_arena* arena;
  PHP_OBJECT_NEW(arena, Arena);

  intern->map = upb_map_new(
      to_fieldtype(key_type), to_fieldtype(value_type), upb_arena_alloc(arena));
  intern->klass = klass;
}

// -----------------------------------------------------------------------------
// MapFieldIter
// -----------------------------------------------------------------------------

void MapFieldIter_init_c_instance(
    MapFieldIter *intern TSRMLS_DC) {
  intern->map_field = NULL;
  intern->iter = NULL;
}

void MapFieldIter_free_c(
    MapFieldIter *intern TSRMLS_DC) {
}
