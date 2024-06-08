// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/generator.h"

#include <memory>
#include <string>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/crate_mapping.h"
#include "google/protobuf/compiler/rust/enum.h"
#include "google/protobuf/compiler/rust/message.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/compiler/rust/relative_path.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {
namespace {

// Emits `pub use <internal submodule name>::Type` for all messages and enums of
// a `non_primary_src` into the `primary_file`.
//
// `non_primary_src` has to be a non-primary src of the current `proto_library`.
void EmitPubUseOfOwnTypes(Context& ctx, const FileDescriptor& primary_file,
                          const FileDescriptor& non_primary_src) {
  auto mod = RustInternalModuleName(ctx, non_primary_src);
  ctx.Emit({{"mod", mod}}, R"rs(
    #[allow(unused_imports)]
    pub use crate::$mod$::*;
  )rs");
}

// Emits `pub use <crate_name>::<modules for parent types>::Type` for all
// messages and enums of a `dep`. This should only be
// called for 'import public' deps.
void EmitPublicImportsForDepFile(Context& ctx, const FileDescriptor* dep) {
  std::string crate_name = GetCrateName(ctx, *dep);
  for (int i = 0; i < dep->message_type_count(); ++i) {
    auto* msg = dep->message_type(i);
    auto path = GetCrateRelativeQualifiedPath(ctx, *msg);
    ctx.Emit({{"crate", crate_name}, {"pkg::Msg", path}},
             R"rs(
                pub use $crate$::$pkg::Msg$;
                pub use $crate$::$pkg::Msg$View;
                pub use $crate$::$pkg::Msg$Mut;
              )rs");
  }
  for (int i = 0; i < dep->enum_type_count(); ++i) {
    auto* enum_ = dep->enum_type(i);
    auto path = GetCrateRelativeQualifiedPath(ctx, *enum_);
    ctx.Emit({{"crate", crate_name}, {"pkg::Enum", path}},
             R"rs(
                pub use $crate$::$pkg::Enum$;
              )rs");
  }
}

// Emits public imports of all files coming from dependencies (imports of local
// files are implicitly public).
//
// `import public` works transitively in C++ (although it doesn't respect
// layering_check in clang). For Rust we actually make it layering clean because
// Blaze compiles transitive proto deps as if they were direct.
//
// Note we don't reexport entire crates, only messages and enums from files that
// have been explicitly publicly imported. It may happen that a `proto_library`
// defines multiple files, but not all are publicly imported.
void EmitPublicImports(Context& ctx,
                       const std::vector<const FileDescriptor*>& srcs) {
  absl::flat_hash_set<const FileDescriptor*> files_in_current_target(
      srcs.begin(), srcs.end());
  std::vector<const FileDescriptor*> files_to_visit(srcs.begin(), srcs.end());
  absl::c_reverse(files_to_visit);
  while (!files_to_visit.empty()) {
    const FileDescriptor* file = files_to_visit.back();
    files_to_visit.pop_back();

    if (!files_in_current_target.contains(file)) {
      EmitPublicImportsForDepFile(ctx, file);
    }

    for (int i = 0; i < file->public_dependency_count(); ++i) {
      files_to_visit.push_back(file->dependency(i));
    }
  }
}

// Emits submodule declarations so `rustc` can find non primary sources from
// the primary file.
void DeclareSubmodulesForNonPrimarySrcs(
    Context& ctx, const FileDescriptor& primary_file,
    absl::Span<const FileDescriptor* const> non_primary_srcs) {
  std::string primary_file_path = GetRsFile(ctx, primary_file);
  RelativePath primary_relpath(primary_file_path);
  for (const FileDescriptor* non_primary_src : non_primary_srcs) {
    std::string non_primary_file_path = GetRsFile(ctx, *non_primary_src);
    std::string relative_mod_path =
        primary_relpath.Relative(RelativePath(non_primary_file_path));
    ctx.Emit({{"file_path", relative_mod_path},
              {"foo", primary_file_path},
              {"bar", non_primary_file_path},
              {"mod_name", RustInternalModuleName(ctx, *non_primary_src)}},
             R"rs(
                        #[path="$file_path$"]
                        #[allow(non_snake_case)]
                        pub mod $mod_name$;
                      )rs");
  }
}

// Emits `pub use <...>::Msg` for all messages in non primary sources.
void ReexportMessagesFromSubmodules(
    Context& ctx, const FileDescriptor& primary_file,
    absl::Span<const FileDescriptor* const> non_primary_srcs) {
  for (const FileDescriptor* file : non_primary_srcs) {
    EmitPubUseOfOwnTypes(ctx, primary_file, *file);
  }
}

}  // namespace

bool RustGenerator::Generate(const FileDescriptor* file,
                             const std::string& parameter,
                             GeneratorContext* generator_context,
                             std::string* error) const {
  absl::StatusOr<Options> opts = Options::Parse(parameter);
  if (!opts.ok()) {
    *error = std::string(opts.status().message());
    return false;
  }

  std::vector<const FileDescriptor*> files_in_current_crate;
  generator_context->ListParsedFiles(&files_in_current_crate);

  absl::StatusOr<absl::flat_hash_map<std::string, std::string>>
      import_path_to_crate_name = GetImportPathToCrateNameMap(&*opts);
  if (!import_path_to_crate_name.ok()) {
    *error = std::string(import_path_to_crate_name.status().message());
    return false;
  }

  RustGeneratorContext rust_generator_context(&files_in_current_crate,
                                              &*import_path_to_crate_name);

  Context ctx_without_printer(&*opts, &rust_generator_context, nullptr);

  auto outfile = absl::WrapUnique(
      generator_context->Open(GetRsFile(ctx_without_printer, *file)));
  io::Printer printer(outfile.get());
  Context ctx = ctx_without_printer.WithPrinter(&printer);

  // Convenience shorthands for common symbols.
  auto v = ctx.printer().WithVars({
      {"std", "::__std"},
      {"pb", "::__pb"},
      {"pbi", "::__pb::__internal"},
      {"pbr", "::__pb::__runtime"},
      {"NonNull", "::__std::ptr::NonNull"},
      {"Phantom", "::__std::marker::PhantomData"},
  });

  ctx.Emit({{"kernel", KernelRsName(ctx.opts().kernel)}}, R"rs(
    extern crate protobuf_$kernel$ as __pb;
    extern crate std as __std;

  )rs");

  std::vector<const FileDescriptor*> file_contexts(
      files_in_current_crate.begin(), files_in_current_crate.end());

  // Generating the primary file?
  if (file == &rust_generator_context.primary_file()) {
    auto non_primary_srcs = absl::MakeConstSpan(file_contexts).subspan(1);
    DeclareSubmodulesForNonPrimarySrcs(ctx, *file, non_primary_srcs);
    ReexportMessagesFromSubmodules(ctx, *file, non_primary_srcs);
    EmitPublicImports(ctx, file_contexts);
  }

  std::unique_ptr<io::ZeroCopyOutputStream> thunks_cc;
  std::unique_ptr<io::Printer> thunks_printer;
  if (ctx.is_cpp()) {
    thunks_cc.reset(generator_context->Open(GetThunkCcFile(ctx, *file)));
    thunks_printer = std::make_unique<io::Printer>(thunks_cc.get());

    thunks_printer->Emit(
        {{"proto_h", GetHeaderFile(ctx, *file)},
         {"proto_deps_h",
          [&] {
            for (int i = 0; i < file->dependency_count(); i++) {
              thunks_printer->Emit(
                  {{"proto_dep_h", GetHeaderFile(ctx, *file->dependency(i))}},
                  R"cc(
#include "$proto_dep_h$"
                  )cc");
            }
          }}},
        R"cc(
#include "$proto_h$"
          $proto_deps_h$
#include "google/protobuf/rust/cpp_kernel/cpp_api.h"
        )cc");
  }

  for (int i = 0; i < file->message_type_count(); ++i) {
    auto& msg = *file->message_type(i);

    GenerateRs(ctx, msg);
    ctx.printer().PrintRaw("\n");

    if (ctx.is_cpp()) {
      auto thunks_ctx = ctx.WithPrinter(thunks_printer.get());

      thunks_ctx.Emit({{"Msg", msg.full_name()}}, R"cc(
        // $Msg$
      )cc");
      GenerateThunksCc(thunks_ctx, msg);
      thunks_ctx.printer().PrintRaw("\n");
    }
  }

  for (int i = 0; i < file->enum_type_count(); ++i) {
    auto& enum_ = *file->enum_type(i);
    GenerateEnumDefinition(ctx, enum_);
    ctx.printer().PrintRaw("\n");

    if (ctx.is_cpp()) {
      auto thunks_ctx = ctx.WithPrinter(thunks_printer.get());

      thunks_ctx.Emit({{"enum", enum_.full_name()}}, R"cc(
        // $enum$
      )cc");
      GenerateEnumThunksCc(thunks_ctx, enum_);
      thunks_ctx.printer().PrintRaw("\n");
    }
  }

  return true;
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
