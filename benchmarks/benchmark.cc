
#include <benchmark/benchmark.h>
#include <string.h>

// For benchmarks of parsing speed.
#include "benchmarks/descriptor.pb.h"
#include "benchmarks/descriptor.upb.h"
#include "benchmarks/descriptor.upbdefs.h"
#include "benchmarks/descriptor_sv.pb.h"

// For for benchmarks of building descriptors.
#include "google/protobuf/descriptor.upb.h"
#include "google/protobuf/descriptor.pb.h"

#include "upb/def.hpp"

upb_strview descriptor = benchmarks_descriptor_proto_upbdefinit.descriptor;
namespace protobuf = ::google::protobuf;

/* A buffer big enough to parse descriptor.proto without going to heap. */
char buf[65535];

static void BM_LoadDescriptor(benchmark::State& state) {
  FILE *f = fopen("/tmp/ads-descriptor.pb", "rb");
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  fseek(f, 0, SEEK_SET);
  std::string str(size, '\0');
  fread(&str[0], 1, size, f);
  fclose(f);
  fprintf(stderr, "size: %zu\n", str.size());
  for (auto _ : state) {
    upb::SymbolTable symtab;
    upb::Arena arena;
    google_protobuf_FileDescriptorSet* set =
        google_protobuf_FileDescriptorSet_parse(str.data(), str.size(),
                                                arena.ptr());
    const google_protobuf_FileDescriptorProto*const * files =
        google_protobuf_FileDescriptorSet_file(set, &size);
    for (size_t i = 0; i < size; i++) {
      upb::FileDefPtr file_def = symtab.AddFile(files[i], NULL);
      if (!file_def) {
        printf("Failed to add file.\n");
        exit(1);
      }
    }
  }
}
BENCHMARK(BM_LoadDescriptor);

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

static void BM_Parse_Upb_FileDesc_WithArena(benchmark::State& state) {
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
BENCHMARK(BM_Parse_Upb_FileDesc_WithArena);

static void BM_Parse_Upb_FileDesc_WithInitialBlock(benchmark::State& state) {
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
BENCHMARK(BM_Parse_Upb_FileDesc_WithInitialBlock);

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
struct WithInitialBlock {
 public:
  WithInitialBlock() : arena_(GetOptions()) {}
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

using FileDesc = ::upb_benchmark::FileDescriptorProto;
using FileDescSV = ::upb_benchmark::sv::FileDescriptorProto;

const protobuf::MessageLite::ParseFlags kMergePartial =
    protobuf::MessageLite::ParseFlags::kMergePartial;
const protobuf::MessageLite::ParseFlags kAliasStrings =
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
BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDesc, NoArena);
BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDesc, WithArena);
BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDesc, WithInitialBlock);
//BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDescSV, NoArena);
//BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDescSV, WithArena);
BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDescSV, WithInitialBlock);
//BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDescSV, NoArena, kAliasStrings);
//BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDescSV, WithArena, kAliasStrings);
BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDescSV, WithInitialBlock, kAliasStrings);

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
