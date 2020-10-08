
#include <string.h>
#include <benchmark/benchmark.h>
#include "google/protobuf/descriptor.upb.h"
#include "google/protobuf/descriptor.upbdefs.h"
#include "google/protobuf/descriptor.pb.h"

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
    fprintf(stderr, "+++ finished parse\n");
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

static void BM_ParseDescriptorProto2NoArena(benchmark::State& state) {
  size_t bytes = 0;
  for (auto _ : state) {
    google::protobuf::FileDescriptorProto proto;
    bool ok = proto.ParseFromArray(descriptor.data, descriptor.size);

    if (!ok) {
      printf("Failed to parse.\n");
      exit(1);
    }
    bytes += descriptor.size;
  }
  state.SetBytesProcessed(state.iterations() * descriptor.size);
}
BENCHMARK(BM_ParseDescriptorProto2NoArena);

static void BM_ParseDescriptorProto2WithArena(benchmark::State& state) {
  size_t bytes = 0;
  for (auto _ : state) {
    google::protobuf::Arena arena;
    auto proto = google::protobuf::Arena::CreateMessage<
        google::protobuf::FileDescriptorProto>(&arena);
    bool ok = proto->ParseFromArray(descriptor.data, descriptor.size);

    if (!ok) {
      printf("Failed to parse.\n");
      exit(1);
    }
    bytes += descriptor.size;
  }
  state.SetBytesProcessed(state.iterations() * descriptor.size);
}
BENCHMARK(BM_ParseDescriptorProto2WithArena);

static void BM_SerializeDescriptorProto2(benchmark::State& state) {
  size_t bytes = 0;
  google::protobuf::FileDescriptorProto proto;
  proto.ParseFromArray(descriptor.data, descriptor.size);
  for (auto _ : state) {
    proto.SerializeToArray(buf, sizeof(buf));
    bytes += descriptor.size;
  }
  state.SetBytesProcessed(state.iterations() * descriptor.size);
}
BENCHMARK(BM_SerializeDescriptorProto2);

static void BM_SerializeDescriptor(benchmark::State& state) {
  int64_t total = 0;
  upb_arena* arena = upb_arena_new();
  google_protobuf_FileDescriptorProto* set =
      google_protobuf_FileDescriptorProto_parse(descriptor.data,
                                                descriptor.size, arena);
  if (!set) {
    printf("Failed to parse.\n");
    exit(1);
  }
  for (auto _ : state) {
    upb_arena* enc_arena = upb_arena_init(buf, sizeof(buf), NULL);
    size_t size;
    char *data = google_protobuf_FileDescriptorProto_serialize(set, enc_arena, &size);
    if (!data) {
      printf("Failed to serialize.\n");
      exit(1);
    }
    total += size;
  }
  state.SetBytesProcessed(total);
}
BENCHMARK(BM_SerializeDescriptor);
