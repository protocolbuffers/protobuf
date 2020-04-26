
#include <string.h>
#include <benchmark/benchmark.h>
#include "google/protobuf/descriptor.upb.h"
#include "google/protobuf/descriptor.upbdefs.h"

upb_strview descriptor = google_protobuf_descriptor_proto_upbdefinit.descriptor;

/* A buffer big enough to parse descriptor.proto without going to heap. */
char buf[65535];

static void BM_ArenaOneAlloc(benchmark::State& state) {
  for (auto _ : state) {
    upb_arena* arena = upb_arena_new();
    upb_arena_malloc(arena, 1);
    upb_arena_free(arena);
  }
}
BENCHMARK(BM_ArenaOneAlloc);

static void BM_ArenaInitialBlockOneAlloc(benchmark::State& state) {
  for (auto _ : state) {
    upb_arena* arena = upb_arena_init(buf, sizeof(buf), NULL);
    upb_arena_malloc(arena, 1);
    upb_arena_free(arena);
  }
}
BENCHMARK(BM_ArenaInitialBlockOneAlloc);

static void BM_ParseDescriptorNoHeap(benchmark::State& state) {
  size_t bytes = 0;
  for (auto _ : state) {
    upb_arena* arena = upb_arena_init(buf, sizeof(buf), NULL);
    google_protobuf_FileDescriptorProto* set =
        google_protobuf_FileDescriptorProto_parse(descriptor.data,
                                                descriptor.size, arena);
    if (!set) {
      printf("Failed to parse.\n");
      exit(1);
    }
    bytes += descriptor.size;
    upb_arena_free(arena);
  }
  state.SetBytesProcessed(state.iterations() * descriptor.size);
}
BENCHMARK(BM_ParseDescriptorNoHeap);

static void BM_ParseDescriptor(benchmark::State& state) {
  size_t bytes = 0;
  for (auto _ : state) {
    upb_arena* arena = upb_arena_new();
    google_protobuf_FileDescriptorProto* set =
        google_protobuf_FileDescriptorProto_parse(descriptor.data,
                                                descriptor.size, arena);
    if (!set) {
      printf("Failed to parse.\n");
      exit(1);
    }
    bytes += descriptor.size;
    upb_arena_free(arena);
  }
  state.SetBytesProcessed(state.iterations() * descriptor.size);
}
BENCHMARK(BM_ParseDescriptor);
