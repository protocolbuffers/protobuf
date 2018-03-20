#include "protobuf_cpp.h"
#include "upb.h"

// -----------------------------------------------------------------------------
// Arena
// -----------------------------------------------------------------------------

void proto_arena_init(proto_arena *arena) {
  upb_arena_init((upb_arena*)arena);
  arena->wrapper = NULL;
}

void proto_arena_uninit(proto_arena *arena) {
  upb_arena_uninit((upb_arena*)arena);
}

void Arena_init_c_instance(
    Arena *intern TSRMLS_DC) {
  intern->arena = (proto_arena*)upb_gmalloc(sizeof(proto_arena));
  proto_arena_init(intern->arena);
}

void Arena_free_c(
    Arena *intern TSRMLS_DC) {
  proto_arena_uninit(intern->arena);
  upb_gfree(intern->arena);
}
