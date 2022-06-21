// Copyright (c) 2009-2021, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Google LLC nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <benchmark/benchmark.h>

#include <string.h>

#include "google/ads/googleads/v11/services/google_ads_service.upbdefs.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/dynamic_message.h"
#include "absl/container/flat_hash_set.h"
#include "benchmarks/descriptor.pb.h"
#include "benchmarks/descriptor.upb.h"
#include "benchmarks/descriptor.upbdefs.h"
#include "benchmarks/descriptor_sv.pb.h"
#include "upb/def.hpp"

upb_StringView descriptor = benchmarks_descriptor_proto_upbdefinit.descriptor;
namespace protobuf = ::google::protobuf;

/* A buffer big enough to parse descriptor.proto without going to heap. */
char buf[65535];

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
    upb_Arena* arena = upb_Arena_Init(buf, sizeof(buf), NULL);
    upb_Arena_Malloc(arena, 1);
    upb_Arena_Free(arena);
  }
}
BENCHMARK(BM_ArenaInitialBlockOneAlloc);

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
      init->descriptor.data, init->descriptor.size, NULL,
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
      google_ads_googleads_v11_services_SearchGoogleAdsRequest_getmsgdef(
          defpool.ptr());
      bytes_per_iter = _upb_DefPool_BytesLoaded(defpool.ptr());
    } else {
      bytes_per_iter = 0;
      LoadDefInit_BuildLayout(
          defpool.ptr(),
          &google_ads_googleads_v11_services_google_ads_service_proto_upbdefinit,
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
      google_ads_googleads_v11_services_google_ads_service_proto_upbdefinit;
  std::vector<upb_StringView> serialized_files;
  absl::flat_hash_set<const _upb_DefPool_Init*> seen_files;
  CollectFileDescriptors(
      &google_ads_googleads_v11_services_google_ads_service_proto_upbdefinit,
      serialized_files, seen_files);
  size_t bytes_per_iter = 0;
  for (auto _ : state) {
    bytes_per_iter = 0;
    protobuf::Arena arena;
    protobuf::DescriptorPool pool;
    for (auto file : serialized_files) {
      protobuf::StringPiece input(file.data, file.size);
      auto proto =
          protobuf::Arena::CreateMessage<protobuf::FileDescriptorProto>(&arena);
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
          "google.ads.googleads.v11.services.SearchGoogleAdsResponse");
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
      arena = upb_Arena_Init(buf, sizeof(buf), NULL);
    } else {
      arena = upb_Arena_New();
    }
    upb_benchmark_FileDescriptorProto* set =
        upb_benchmark_FileDescriptorProto_parse_ex(
            descriptor.data, descriptor.size, NULL,
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
  P* GetProto() { return &proto_; }

 private:
  P proto_;
};

template <class P>
struct Proto2Factory<UseArena, P> {
 public:
  P* GetProto() { return protobuf::Arena::CreateMessage<P>(&arena_); }

 private:
  protobuf::Arena arena_;
};

template <class P>
struct Proto2Factory<InitBlock, P> {
 public:
  Proto2Factory() : arena_(GetOptions()) {}
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

template <class P, ArenaMode AMode, CopyStrings kCopy>
void BM_Parse_Proto2(benchmark::State& state) {
  constexpr protobuf::MessageLite::ParseFlags kParseFlags =
      kCopy == Copy
          ? protobuf::MessageLite::ParseFlags::kMergePartial
          : protobuf::MessageLite::ParseFlags::kMergePartialWithAliasing;
  for (auto _ : state) {
    Proto2Factory<AMode, P> proto_factory;
    auto proto = proto_factory.GetProto();
    protobuf::StringPiece input(descriptor.data, descriptor.size);
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

static void BM_SerializeDescriptor_Upb(benchmark::State& state) {
  int64_t total = 0;
  upb_Arena* arena = upb_Arena_New();
  upb_benchmark_FileDescriptorProto* set =
      upb_benchmark_FileDescriptorProto_parse(descriptor.data, descriptor.size,
                                              arena);
  if (!set) {
    printf("Failed to parse.\n");
    exit(1);
  }
  for (auto _ : state) {
    upb_Arena* enc_arena = upb_Arena_Init(buf, sizeof(buf), NULL);
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
