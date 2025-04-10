// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstddef>
#include <string>

#include "google/protobuf/descriptor.upb.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/io/printer.h"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.hpp"
#include "upb/util/def_to_proto.h"
#include "upb_generator/common/names.h"
#include "upb_generator/minitable/names.h"
#include "upb_generator/reflection/context.h"
#include "upb_generator/reflection/header.h"
#include "upb_generator/reflection/names.h"

namespace upb::generator::reflection {
namespace {

using Sub = google::protobuf::io::Printer::Sub;

static std::string DefSourceFilename(upb::FileDefPtr file) {
  return StripExtension(file.name()) + ".upbdefs.c";
}

void WriteIncludes(upb::FileDefPtr file, Context& ctx) {
  ctx.Emit(
      {
          {"def_header_filename", DefHeaderFilename(file)},
          {"mini_table_header_filename", MiniTableHeaderFilename(file.name())},
      },
      R"(
        #include "upb/reflection/def.h"
        #include "$def_header_filename$"
        #include "$mini_table_header_filename$"
      )");
}

void WriteDefPoolFwdDecls(upb::FileDefPtr file, Context& ctx) {
  if (file.dependency_count() == 0) return;

  for (int i = 0; i < file.dependency_count(); i++) {
    ctx.Emit(
        {{"dllexport_decl", ctx.options().dllexport_decl},
         {"def_init_symbol", ReflectionFileSymbol(file.dependency(i).name())}},
        R"cc(
          extern$ dllexport_decl$ _upb_DefPool_Init $def_init_symbol$;
        )cc");
  }

  ctx.Emit("\n");
}

void WriteStringArrayLine(absl::string_view data, Context& ctx) {
  std::string line = absl::StrJoin(data, ", ", [](std::string* out, char ch) {
    absl::StrAppendFormat(out, "'%v'",
                          absl::CEscape(absl::string_view(&ch, 1)));
  });
  ctx.Emit({{"line", line}},
           R"cc(
             $line$,
           )cc");
}

void WriteStringArray(absl::string_view data, Context& ctx) {
  const size_t kMaxLineLength = 12;
  for (size_t i = 0; i < data.size(); i += kMaxLineLength) {
    WriteStringArrayLine(data.substr(i, kMaxLineLength), ctx);
  }
}

void WriteDescriptor(upb::FileDefPtr file, Context& ctx) {
  upb::Arena arena;
  google_protobuf_FileDescriptorProto* file_proto =
      upb_FileDef_ToProto(file.ptr(), arena.ptr());
  size_t serialized_size;
  const char* serialized = google_protobuf_FileDescriptorProto_serialize(
      file_proto, arena.ptr(), &serialized_size);
  absl::string_view file_data(serialized, serialized_size);

  ctx.Emit({{"serialized_size", file_data.size()},
            {"contents", [&] { WriteStringArray(file_data, ctx); }}},
           R"cc(
             static const char descriptor[$serialized_size$] = {
                 $contents$,
             };
           )cc");
  ctx.Emit("\n");
}

void WriteDependencies(upb::FileDefPtr file, Context& ctx) {
  auto write_dep = [&](int i) {
    ctx.Emit({{"sym", ReflectionFileSymbol(file.dependency(i).name())}},
             R"cc(
               &$sym$,
             )cc");
  };

  auto write_deps = [&] {
    for (int i = 0; i < file.dependency_count(); i++) write_dep(i);
  };

  ctx.Emit({{"dep_count", file.dependency_count() + 1},
            {google::protobuf::io::Printer::Sub("deps", write_deps).WithSuffix(",")}},
           R"cc(
             static _upb_DefPool_Init *deps[$dep_count$] = {
                 $deps$,
                 NULL,
             };
           )cc");
  ctx.Emit("\n");
}

void WriteDefPoolInitStruct(upb::FileDefPtr file, Context& ctx) {
  ctx.Emit(
      {
          {"defpool_init_name", ReflectionFileSymbol(file.name())},
          {"file_name", file.name()},
          {"mini_table_file_var_name", MiniTableFileVarName(file.name())},
      },
      R"cc(
        _upb_DefPool_Init $defpool_init_name$ = {
            deps,
            &$mini_table_file_var_name$,
            "$file_name$",
            UPB_STRINGVIEW_INIT(descriptor, sizeof(descriptor)),
        };
      )cc");
}

void WriteDefPoolInit(upb::FileDefPtr file, Context& ctx) {
  WriteDefPoolFwdDecls(file, ctx);
  WriteDescriptor(file, ctx);
  WriteDependencies(file, ctx);
  WriteDefPoolInitStruct(file, ctx);
}

void WriteDefSource(upb::FileDefPtr file, Context& ctx) {
  ctx.Emit(
      {
          {Sub("file_warning", FileWarning(file.name())).WithSuffix(";")},
          {Sub("includes", [&] { WriteIncludes(file, ctx); }).WithSuffix(";")},
          {Sub("def_pool_init", [&] { WriteDefPoolInit(file, ctx); })
               .WithSuffix(";")},
      },
      R"cc(
        $file_warning$;
        $includes$;

        $def_pool_init$;
      )cc");
}

}  // namespace

void GenerateReflectionSource(upb::FileDefPtr file, const Options& options,
                              google::protobuf::compiler::GeneratorContext* context) {
  Context c_context(options, context->Open(DefSourceFilename(file)));
  WriteDefSource(file, c_context);
}

}  // namespace upb::generator::reflection
