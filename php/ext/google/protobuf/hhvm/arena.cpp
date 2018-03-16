#include "protobuf_cpp.h"
#include "upb.h"

// -----------------------------------------------------------------------------
// Arena
// -----------------------------------------------------------------------------

void Arena_init_c_instance(
    Arena *intern TSRMLS_DC) {
  intern->arena = reinterpret_cast<upb_arena*>(upb_gmalloc(sizeof(upb_arena)));
  upb_arena_init(intern->arena);
}

void Arena_free_c(
    Arena *intern TSRMLS_DC) {
  upb_arena_uninit(intern->arena);
  upb_gfree(intern->arena);
}
