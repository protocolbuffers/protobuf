// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <benchmark/benchmark.h>

#include <math.h>
#include <stdint.h>
#include <string.h>

#include <string>
#include <vector>

#include "google/ads/googleads/v19/services/google_ads_service.upbdefs.h"
#include "google/protobuf/descriptor.pb.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/json/json.h"
#include "benchmarks/descriptor.pb.h"
#include "benchmarks/descriptor.upb.h"
#include "benchmarks/descriptor.upbdefs.h"
#include "benchmarks/descriptor_sv.pb.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/json/decode.h"
#include "upb/json/encode.h"
#include "upb/mem/arena.h"
#include "upb/reflection/def.hpp"
#include "upb/wire/decode.h"

upb_StringView descriptor =
    benchmarks_descriptor_proto_upbdefinit.descriptor;
namespace protobuf = ::google::protobuf;

// A buffer big enough to parse descriptor.proto without going to heap.
// We use 64-bit ints here to force alignment.
int64_t buf[8191];

void CollectFileDescriptors(
    const _upb_DefPool_Init* file,
    std::vector<upb_StringView>& serialized_files,
    absl::flat_hash_set<const _upb_DefPool_Init*>& seen) {
  if (!seen.insert(file).second) return;
  for (_upb_DefPool_Init** deps = file->deps; *deps; deps++) {
    CollectFileDescriptors(*deps, serialized_files, seen);
  }
  serialized_files.push_back(file->descriptor);
}

static void BM_ArenaOneAlloc(benchmark::State& state) {
  for (auto _ : state) {
    upb_Arena* arena = upb_Arena_New();
    upb_Arena_Malloc(arena, 1);
    upb_Arena_Free(arena);
  }
}
BENCHMARK(BM_ArenaOneAlloc);

static void BM_ArenaInitialBlockOneAlloc(benchmark::State& state) {
  for (auto _ : state) {
    upb_Arena* arena = upb_Arena_Init(buf, sizeof(buf), nullptr);
    upb_Arena_Malloc(arena, 1);
    upb_Arena_Free(arena);
  }
}
BENCHMARK(BM_ArenaInitialBlockOneAlloc);

static void BM_ArenaFuseUnbalanced(benchmark::State& state) {
  std::vector<upb_Arena*> arenas(state.range(0));
  size_t n = 0;
  for (auto _ : state) {
    for (auto& arena : arenas) {
      arena = upb_Arena_New();
    }
    for (auto& arena : arenas) {
      upb_Arena_Fuse(arenas[0], arena);
    }
    for (auto& arena : arenas) {
      upb_Arena_Free(arena);
    }
    n += arenas.size();
  }
  state.SetItemsProcessed(n);
}
BENCHMARK(BM_ArenaFuseUnbalanced)->Range(2, 128);

static void BM_ArenaFuseBalanced(benchmark::State& state) {
  std::vector<upb_Arena*> arenas(state.range(0));
  size_t n = 0;

  for (auto _ : state) {
    for (auto& arena : arenas) {
      arena = upb_Arena_New();
    }

    // Perform a series of fuses that keeps the halves balanced.
    const size_t max = ceil(log2(double(arenas.size())));
    for (size_t n = 0; n <= max; n++) {
      size_t step = 1 << n;
      for (size_t i = 0; i + step < arenas.size(); i += (step * 2)) {
        upb_Arena_Fuse(arenas[i], arenas[i + step]);
      }
    }

    for (auto& arena : arenas) {
      upb_Arena_Free(arena);
    }
    n += arenas.size();
  }
  state.SetItemsProcessed(n);
}
BENCHMARK(BM_ArenaFuseBalanced)->Range(2, 128);

enum LoadDescriptorMode {
  NoLayout,
  WithLayout,
};

// This function is mostly copied from upb/def.c, but it is modified to avoid
// passing in the pre-generated mini-tables, in order to force upb to compute
// them dynamically.  Generally you would never want to do this, but we want to
// simulate the cost we would pay if we were loading these types purely from
// descriptors, with no mini-tales available.
bool LoadDefInit_BuildLayout(upb_DefPool* s, const _upb_DefPool_Init* init,
                             size_t* bytes) {
  _upb_DefPool_Init** deps = init->deps;
  google_protobuf_FileDescriptorProto* file;
  upb_Arena* arena;
  upb_Status status;

  upb_Status_Clear(&status);

  if (upb_DefPool_FindFileByName(s, init->filename)) {
    return true;
  }

  arena = upb_Arena_New();

  for (; *deps; deps++) {
    if (!LoadDefInit_BuildLayout(s, *deps, bytes)) goto err;
  }

  file = google_protobuf_FileDescriptorProto_parse_ex(
      init->descriptor.data, init->descriptor.size, nullptr,
      kUpb_DecodeOption_AliasString, arena);
  *bytes += init->descriptor.size;

  if (!file) {
    upb_Status_SetErrorFormat(
        &status,
        "Failed to parse compiled-in descriptor for file '%s'. This should "
        "never happen.",
        init->filename);
    goto err;
  }

  // KEY DIFFERENCE: Here we pass in only the descriptor, and not the
  // pre-generated minitables.
  if (!upb_DefPool_AddFile(s, file, &status)) {
    goto err;
  }

  upb_Arena_Free(arena);
  return true;

err:
  fprintf(stderr,
          "Error loading compiled-in descriptor for file '%s' (this should "
          "never happen): %s\n",
          init->filename, upb_Status_ErrorMessage(&status));
  exit(1);
}

template <LoadDescriptorMode Mode>
static void BM_LoadAdsDescriptor_Upb(benchmark::State& state) {
  size_t bytes_per_iter = 0;
  for (auto _ : state) {
    upb::DefPool defpool;
    if (Mode == NoLayout) {
      google_ads_googleads_v19_services_SearchGoogleAdsRequest_getmsgdef(
          defpool.ptr());
      bytes_per_iter = _upb_DefPool_BytesLoaded(defpool.ptr());
    } else {
      bytes_per_iter = 0;
      LoadDefInit_BuildLayout(
          defpool.ptr(),
          &google_ads_googleads_v19_services_google_ads_service_proto_upbdefinit,
          &bytes_per_iter);
    }
  }
  state.SetBytesProcessed(state.iterations() * bytes_per_iter);
}
BENCHMARK_TEMPLATE(BM_LoadAdsDescriptor_Upb, NoLayout);
BENCHMARK_TEMPLATE(BM_LoadAdsDescriptor_Upb, WithLayout);

template <LoadDescriptorMode Mode>
static void BM_LoadAdsDescriptor_Proto2(benchmark::State& state) {
  extern _upb_DefPool_Init
      google_ads_googleads_v19_services_google_ads_service_proto_upbdefinit;
  std::vector<upb_StringView> serialized_files;
  absl::flat_hash_set<const _upb_DefPool_Init*> seen_files;
  CollectFileDescriptors(
      &google_ads_googleads_v19_services_google_ads_service_proto_upbdefinit,
      serialized_files, seen_files);
  size_t bytes_per_iter = 0;
  for (auto _ : state) {
    bytes_per_iter = 0;
    protobuf::Arena arena;
    protobuf::DescriptorPool pool;
    for (auto file : serialized_files) {
      absl::string_view input(file.data, file.size);
      auto proto =
          protobuf::Arena::Create<protobuf::FileDescriptorProto>(&arena);
      bool ok = proto->ParseFrom<protobuf::MessageLite::kMergePartial>(input) &&
                pool.BuildFile(*proto) != nullptr;
      if (!ok) {
        printf("Failed to add file.\n");
        exit(1);
      }
      bytes_per_iter += input.size();
    }

    if (Mode == WithLayout) {
      protobuf::DynamicMessageFactory factory;
      const protobuf::Descriptor* d = pool.FindMessageTypeByName(
          "google.ads.googleads.v19.services.SearchGoogleAdsResponse");
      if (!d) {
        printf("Failed to find descriptor.\n");
        exit(1);
      }
      factory.GetPrototype(d);
    }
  }
  state.SetBytesProcessed(state.iterations() * bytes_per_iter);
}
BENCHMARK_TEMPLATE(BM_LoadAdsDescriptor_Proto2, NoLayout);
BENCHMARK_TEMPLATE(BM_LoadAdsDescriptor_Proto2, WithLayout);

enum CopyStrings {
  Copy,
  Alias,
};

enum ArenaMode {
  NoArena,
  UseArena,
  InitBlock,
};

template <ArenaMode AMode, CopyStrings Copy>
static void BM_Parse_Upb_FileDesc(benchmark::State& state) {
  for (auto _ : state) {
    upb_Arena* arena;
    if (AMode == InitBlock) {
      arena = upb_Arena_Init(buf, sizeof(buf), nullptr);
    } else {
      arena = upb_Arena_New();
    }
    upb_benchmark_FileDescriptorProto* set =
        upb_benchmark_FileDescriptorProto_parse_ex(
            descriptor.data, descriptor.size, nullptr,
            Copy == Alias ? kUpb_DecodeOption_AliasString : 0, arena);
    if (!set) {
      printf("Failed to parse.\n");
      exit(1);
    }
    upb_Arena_Free(arena);
  }
  state.SetBytesProcessed(state.iterations() * descriptor.size);
}
BENCHMARK_TEMPLATE(BM_Parse_Upb_FileDesc, UseArena, Copy);
BENCHMARK_TEMPLATE(BM_Parse_Upb_FileDesc, UseArena, Alias);
BENCHMARK_TEMPLATE(BM_Parse_Upb_FileDesc, InitBlock, Copy);
BENCHMARK_TEMPLATE(BM_Parse_Upb_FileDesc, InitBlock, Alias);

template <ArenaMode AMode, class P>
struct Proto2Factory;

template <class P>
struct Proto2Factory<NoArena, P> {
 public:
  P* GetProto() { return &proto; }

 private:
  P proto;
};

template <class P>
struct Proto2Factory<UseArena, P> {
 public:
  P* GetProto() { return protobuf::Arena::Create<P>(&arena); }

 private:
  protobuf::Arena arena;
};

template <class P>
struct Proto2Factory<InitBlock, P> {
 public:
  Proto2Factory() : arena(GetOptions()) {}
  P* GetProto() { return protobuf::Arena::Create<P>(&arena); }

 private:
  protobuf::ArenaOptions GetOptions() {
    protobuf::ArenaOptions opts;
    opts.initial_block = (char*)buf;
    opts.initial_block_size = sizeof(buf);
    return opts;
  }

  protobuf::Arena arena;
};

using FileDesc = ::upb_benchmark::FileDescriptorProto;
using FileDescSV = ::upb_benchmark::sv::FileDescriptorProto;

template <class P, ArenaMode AMode, CopyStrings kCopy>
void BM_Parse_Proto2(benchmark::State& state) {
  constexpr protobuf::MessageLite::ParseFlags kParseFlags =
      kCopy == Copy
          ? protobuf::MessageLite::ParseFlags::kMergePartial
          : protobuf::MessageLite::ParseFlags::kMergePartialWithAliasing;
  for (auto _ : state) {
    Proto2Factory<AMode, P> proto_factory;
    auto proto = proto_factory.GetProto();
    absl::string_view input(descriptor.data, descriptor.size);
    bool ok = proto->template ParseFrom<kParseFlags>(input);
    if (!ok) {
      printf("Failed to parse.\n");
      exit(1);
    }
  }
  state.SetBytesProcessed(state.iterations() * descriptor.size);
}
BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDesc, NoArena, Copy);
BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDesc, UseArena, Copy);
BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDesc, InitBlock, Copy);
BENCHMARK_TEMPLATE(BM_Parse_Proto2, FileDescSV, InitBlock, Alias);

static void BM_SerializeDescriptor_Proto2(benchmark::State& state) {
  upb_benchmark::FileDescriptorProto proto;
  proto.ParseFromArray(descriptor.data, descriptor.size);
  for (auto _ : state) {
    proto.SerializePartialToArray(buf, sizeof(buf));
  }
  state.SetBytesProcessed(state.iterations() * descriptor.size);
}
BENCHMARK(BM_SerializeDescriptor_Proto2);

static upb_benchmark_FileDescriptorProto* UpbParseDescriptor(upb_Arena* arena) {
  upb_benchmark_FileDescriptorProto* set =
      upb_benchmark_FileDescriptorProto_parse(descriptor.data, descriptor.size,
                                              arena);
  if (!set) {
    printf("Failed to parse.\n");
    exit(1);
  }
  return set;
}

static void BM_SerializeDescriptor_Upb(benchmark::State& state) {
  int64_t total = 0;
  upb_Arena* arena = upb_Arena_New();
  upb_benchmark_FileDescriptorProto* set = UpbParseDescriptor(arena);
  for (auto _ : state) {
    upb_Arena* enc_arena = upb_Arena_Init(buf, sizeof(buf), nullptr);
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

static absl::string_view UpbJsonEncode(upb_benchmark_FileDescriptorProto* proto,
                                       const upb_MessageDef* md,
                                       upb_Arena* arena) {
  size_t size =
      upb_JsonEncode(UPB_UPCAST(proto), md, nullptr, 0, nullptr, 0, nullptr);
  char* buf = reinterpret_cast<char*>(upb_Arena_Malloc(arena, size + 1));
  upb_JsonEncode(UPB_UPCAST(proto), md, nullptr, 0, buf, size, nullptr);
  return absl::string_view(buf, size);
}

static void BM_JsonParse_Upb(benchmark::State& state) {
  upb_Arena* arena = upb_Arena_New();
  upb_benchmark_FileDescriptorProto* set =
      upb_benchmark_FileDescriptorProto_parse(descriptor.data, descriptor.size,
                                              arena);
  if (!set) {
    printf("Failed to parse.\n");
    exit(1);
  }

  upb::DefPool defpool;
  const upb_MessageDef* md =
      upb_benchmark_FileDescriptorProto_getmsgdef(defpool.ptr());
  auto json = UpbJsonEncode(set, md, arena);

  for (auto _ : state) {
    upb_Arena* arena = upb_Arena_New();
    upb_benchmark_FileDescriptorProto* proto =
        upb_benchmark_FileDescriptorProto_new(arena);
    upb_JsonDecode(json.data(), json.size(), UPB_UPCAST(proto), md,
                   defpool.ptr(), 0, arena, nullptr);
    upb_Arena_Free(arena);
  }
  state.SetBytesProcessed(state.iterations() * json.size());
}
BENCHMARK(BM_JsonParse_Upb);

static void BM_JsonParse_Proto2(benchmark::State& state) {
  protobuf::FileDescriptorProto proto;
  absl::string_view input(descriptor.data, descriptor.size);
  proto.ParseFromString(input);
  std::string json;
  ABSL_CHECK_OK(google::protobuf::json::MessageToJsonString(proto, &json));
  for (auto _ : state) {
    protobuf::FileDescriptorProto proto;
    ABSL_CHECK_OK(google::protobuf::json::JsonStringToMessage(json, &proto));
  }
  state.SetBytesProcessed(state.iterations() * json.size());
}
BENCHMARK(BM_JsonParse_Proto2);

static void BM_JsonSerialize_Upb(benchmark::State& state) {
  upb_Arena* arena = upb_Arena_New();
  upb_benchmark_FileDescriptorProto* set =
      upb_benchmark_FileDescriptorProto_parse(descriptor.data, descriptor.size,
                                              arena);
  ABSL_CHECK(set != nullptr);

  upb::DefPool defpool;
  const upb_MessageDef* md =
      upb_benchmark_FileDescriptorProto_getmsgdef(defpool.ptr());
  auto json = UpbJsonEncode(set, md, arena);
  std::string json_str;
  json_str.resize(json.size());

  for (auto _ : state) {
    // This isn't a fully fair comparison, as it assumes we already know the
    // correct size of the buffer.  In practice, we usually need to run the
    // encoder twice, once to discover the size of the buffer.
    upb_JsonEncode(UPB_UPCAST(set), md, nullptr, 0, json_str.data(),
                   json_str.size(), nullptr);
  }
  state.SetBytesProcessed(state.iterations() * json.size());
}
BENCHMARK(BM_JsonSerialize_Upb);

static void BM_JsonSerialize_Proto2(benchmark::State& state) {
  protobuf::FileDescriptorProto proto;
  absl::string_view input(descriptor.data, descriptor.size);
  proto.ParseFromString(input);
  std::string json;
  for (auto _ : state) {
    json.clear();
    ABSL_CHECK_OK(google::protobuf::json::MessageToJsonString(proto, &json));
  }
  state.SetBytesProcessed(state.iterations() * json.size());
}
BENCHMARK(BM_JsonSerialize_Proto2);
