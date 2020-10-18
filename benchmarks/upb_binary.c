
#include "google/protobuf/empty.upb.h"

char buf[1];

int main() {
  upb_arena *arena = upb_arena_new();
  size_t size;
  google_protobuf_Empty *proto = google_protobuf_Empty_parse(buf, 1, arena);
  google_protobuf_Empty_serialize(proto, arena, &size);
  return 0;
}
