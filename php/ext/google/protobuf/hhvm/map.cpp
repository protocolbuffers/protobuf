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

void MapField_free_c(
    MapField *intern TSRMLS_DC) {
}

void MapField_wrap(MapField *intern, upb_map *map, void *klass) {
  intern->map = map;
  intern->klass = klass;
}

void MapField___construct(MapField *intern,
                          upb_descriptortype_t key_type,
                          upb_descriptortype_t value_type,
                          void *klass) {
  upb_arena* arena = reinterpret_cast<upb_arena*>(ALLOC(upb_arena));
  upb_arena_init(arena);
  intern->map = upb_map_new(
      to_fieldtype(key_type), to_fieldtype(value_type), upb_arena_alloc(arena));
  intern->klass = klass;
}
