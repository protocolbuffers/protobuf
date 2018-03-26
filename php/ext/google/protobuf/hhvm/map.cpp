#include "protobuf_cpp.h"
#include "upb.h"

// -----------------------------------------------------------------------------
// MapField
// -----------------------------------------------------------------------------

void MapField_init_c_instance(
    MapField *intern TSRMLS_DC) {
  intern->map = NULL;
  intern->klass = NULL;
  intern->arena = NULL;
  intern->wrappers = NULL;
}

void MapField_deepclean(upb_map *map, const upb_msgdef *m) {
}

void MapField_free_c(
    MapField *intern TSRMLS_DC) {
  if (upb_map_valuetype(intern->map) == UPB_TYPE_MESSAGE) {
    for (std::unordered_map<void*, PHP_OBJECT>::iterator i =
             intern->wrappers->begin();
         i != intern->wrappers->end(); ++i) {
      PHP_OBJECT_FREE(i->second);
    }
    delete intern->wrappers;
  }
  ARENA_DTOR(intern->arena);
}

void MapField_wrap(MapField *intern, upb_map *map, void *klass, ARENA arena) {
  intern->map = map;
  intern->klass = klass;
  intern->arena = arena;
  ARENA_ADDREF(arena);
  if (upb_map_valuetype(intern->map) == UPB_TYPE_MESSAGE) {
    intern->wrappers = new std::unordered_map<void*, PHP_OBJECT>;
  }
}

void MapField___construct(MapField *intern,
                          upb_descriptortype_t key_type,
                          upb_descriptortype_t value_type,
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

  intern->map = upb_map_new(
      to_fieldtype(key_type), to_fieldtype(value_type), upb_arena_alloc(arena));
  intern->klass = klass;

  if (upb_map_valuetype(intern->map) == UPB_TYPE_MESSAGE) {
    intern->wrappers = new std::unordered_map<void*, PHP_OBJECT>;
  }
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
