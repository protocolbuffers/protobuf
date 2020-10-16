
#include <benchmark/benchmark.h>
#include <string.h>

// For benchmarks of parsing speed.
#include "tests/descriptor.pb.h"
#include "tests/descriptor.upb.h"
#include "tests/descriptor.upbdefs.h"
#include "tests/descriptor_sv.pb.h"

// For for benchmarks of building descriptors.
#include "google/protobuf/descriptor.upb.h"
#include "google/protobuf/descriptor.pb.h"

#include "upb/def.hpp"

upb_strview descriptor = tests_descriptor_proto_upbdefinit.descriptor;
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

static void BM_LoadDescriptor_Upb(benchmark::State& state) {
  for (auto _ : state) {
    upb::SymbolTable symtab;
    upb::Arena arena;
    google_protobuf_FileDescriptorProto* file_proto =
        google_protobuf_FileDescriptorProto_parse(descriptor.data,
                                                  descriptor.size, arena.ptr());
    upb::FileDefPtr file_def = symtab.AddFile(file_proto, NULL);
    if (!file_def) {
      printf("Failed to add file.\n");
      exit(1);
    }
  }
  state.SetBytesProcessed(state.iterations() * descriptor.size);
}
BENCHMARK(BM_LoadDescriptor_Upb);

static void BM_LoadDescriptor_Proto2(benchmark::State& state) {
  for (auto _ : state) {
    protobuf::Arena arena;
    protobuf::StringPiece input(descriptor.data,descriptor.size);
    auto proto = protobuf::Arena::CreateMessage<protobuf::FileDescriptorProto>(
        &arena);
    protobuf::DescriptorPool pool;
    bool ok = proto->ParseFrom<protobuf::MessageLite::kMergePartial>(input) &&
              pool.BuildFile(*proto) != nullptr;
    if (!ok) {
      printf("Failed to add file.\n");
      exit(1);
    }
  }
  state.SetBytesProcessed(state.iterations() * descriptor.size);
}
BENCHMARK(BM_LoadDescriptor_Proto2);

static void BM_ParseDescriptor_Upb(benchmark::State& state) {
  size_t bytes = 0;
  for (auto _ : state) {
    upb_arena* arena = upb_arena_new();
    upb_benchmark_FileDescriptorProto* set =
        upb_benchmark_FileDescriptorProto_parse(descriptor.data,
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

static void BM_ParseDescriptor_Upb_LargeInitialBlock(benchmark::State& state) {
  size_t bytes = 0;
  for (auto _ : state) {
    upb_arena* arena = upb_arena_init(buf, sizeof(buf), NULL);
    upb_benchmark_FileDescriptorProto* set =
        upb_benchmark_FileDescriptorProto_parse(descriptor.data,
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

template <class P>
struct NoArena {
 public:
  P* GetProto() { return &proto_; }

 private:
  P proto_;
};

template <class P>
struct WithArena {
 public:
  P* GetProto() { return protobuf::Arena::CreateMessage<P>(&arena_); }

 private:
  protobuf::Arena arena_;
};

template <class P>
struct WithArenaInitialBlock {
 public:
  WithArenaInitialBlock() : arena_(GetOptions()) {}
  P* GetProto() { return protobuf::Arena::CreateMessage<P>(&arena_); }

 private:
  protobuf::ArenaOptions GetOptions() {
    protobuf::ArenaOptions opts;
    opts.initial_block = buf;
    opts.initial_block_size = sizeof(buf);
    return opts;
  }

  protobuf::Arena arena_;
};

using FileDescriptor = ::upb_benchmark::FileDescriptorProto;
using FileDescriptorSV = ::upb_benchmark::sv::FileDescriptorProto;

const protobuf::MessageLite::ParseFlags kMergePartial =
    protobuf::MessageLite::ParseFlags::kMergePartial;
const protobuf::MessageLite::ParseFlags kMergePartialWithAliasing =
    protobuf::MessageLite::ParseFlags::kMergePartialWithAliasing;

template <class P, template <class> class Factory,
          protobuf::MessageLite::ParseFlags kParseFlags = kMergePartial>
void BM_Parse_Proto2(benchmark::State& state) {
  size_t bytes = 0;
  for (auto _ : state) {
    Factory<P> proto_factory;
    auto proto = proto_factory.GetProto();
    protobuf::StringPiece input(descriptor.data,descriptor.size);
    bool ok = proto->template ParseFrom<kParseFlags>(input);
    if (!ok) {
      printf("Failed to parse.\n");
      exit(1);
    }
    bytes += descriptor.size;
  }
  state.SetBytesProcessed(state.iterations() * descriptor.size);
}
BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDescriptor, NoArena);
BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDescriptor, WithArena);
BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDescriptor, WithArenaInitialBlock);
//BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDescriptorSV, NoArena);
//BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDescriptorSV, WithArena);
BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDescriptorSV, WithArenaInitialBlock);
//BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDescriptorSV, NoArena, kMergePartialWithAliasing);
//BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDescriptorSV, WithArena, kMergePartialWithAliasing);
BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDescriptorSV, WithArenaInitialBlock, kMergePartialWithAliasing);

static void BM_SerializeDescriptor_Proto2(benchmark::State& state) {
  size_t bytes = 0;
  upb_benchmark::FileDescriptorProto proto;
  proto.ParseFromArray(descriptor.data, descriptor.size);
  for (auto _ : state) {
    proto.SerializePartialToArray(buf, sizeof(buf));
    bytes += descriptor.size;
  }
  state.SetBytesProcessed(state.iterations() * descriptor.size);
}
BENCHMARK(BM_SerializeDescriptor_Proto2);

static void BM_SerializeDescriptor_Upb(benchmark::State& state) {
  int64_t total = 0;
  upb_arena* arena = upb_arena_new();
  upb_benchmark_FileDescriptorProto* set =
      upb_benchmark_FileDescriptorProto_parse(descriptor.data, descriptor.size,
                                              arena);
  if (!set) {
    printf("Failed to parse.\n");
    exit(1);
  }
  for (auto _ : state) {
    upb_arena* enc_arena = upb_arena_init(buf, sizeof(buf), NULL);
    size_t size;
    char* data =
        upb_benchmark_FileDescriptorProto_serialize(set, enc_arena, &size);
    if (!data) {
      printf("Failed to serialize.\n");
      exit(1);
    }
    total += size;
  }
  state.SetBytesProcessed(total);
}
BENCHMARK(BM_SerializeDescriptor_Upb);
