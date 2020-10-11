
#include <string.h>
#include <benchmark/benchmark.h>
#include "google/protobuf/descriptor.upb.h"
#include "google/protobuf/descriptor.upbdefs.h"
#include "google/protobuf/descriptor.pb.h"

upb_strview descriptor = google_protobuf_descriptor_proto_upbdefinit.descriptor;
namespace protobuf = ::google::protobuf;

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

static void BM_ParseDescriptor_Upb_LargeInitialBlock(benchmark::State& state) {
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
BENCHMARK(BM_ParseDescriptor_Upb_LargeInitialBlock);

static void BM_ParseDescriptor_Upb(benchmark::State& state) {
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
BENCHMARK(BM_ParseDescriptor_Upb);

static void BM_ParseDescriptor_Proto2_NoArena(benchmark::State& state) {
  size_t bytes = 0;
  for (auto _ : state) {
    protobuf::FileDescriptorProto proto;
    bool ok = proto.ParseFromArray(descriptor.data, descriptor.size);

    if (!ok) {
      printf("Failed to parse.\n");
      exit(1);
    }
    bytes += descriptor.size;
  }
  state.SetBytesProcessed(state.iterations() * descriptor.size);
}
BENCHMARK(BM_ParseDescriptor_Proto2_NoArena);

static void BM_ParseDescriptor_Proto2_WithArena(benchmark::State& state) {
  size_t bytes = 0;
  for (auto _ : state) {
    protobuf::Arena arena;
    auto proto = protobuf::Arena::CreateMessage<protobuf::FileDescriptorProto>(
        &arena);
    bool ok = proto->ParseFromArray(descriptor.data, descriptor.size);

    if (!ok) {
      printf("Failed to parse.\n");
      exit(1);
    }
    bytes += descriptor.size;
  }
  state.SetBytesProcessed(state.iterations() * descriptor.size);
}
BENCHMARK(BM_ParseDescriptor_Proto2_WithArena);

static void BM_SerializeDescriptor_Proto2(benchmark::State& state) {
  size_t bytes = 0;
  protobuf::FileDescriptorProto proto;
  proto.ParseFromArray(descriptor.data, descriptor.size);
  for (auto _ : state) {
    proto.SerializeToArray(buf, sizeof(buf));
    bytes += descriptor.size;
  }
  state.SetBytesProcessed(state.iterations() * descriptor.size);
}
BENCHMARK(BM_SerializeDescriptor_Proto2);

static void BM_SerializeDescriptor_Upb(benchmark::State& state) {
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
BENCHMARK(BM_SerializeDescriptor_Upb);
