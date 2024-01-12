// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/generator.h"

#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/memory/memory.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/enum.h"
#include "google/protobuf/compiler/rust/message.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/compiler/rust/relative_path.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {
namespace {

// Emits openings for a tree of submodules for a given `pkg`.
//
// For example for `package.uses.dots.submodule.separator` this function
// generates:
// ```
//   pub mod package {
//   pub mod uses {
//   pub mod dots {
//   pub mod submodule {
//   pub mod separator {
// ```
void EmitOpeningOfPackageModules(Context& ctx, absl::string_view pkg) {
  if (pkg.empty()) return;
  for (absl::string_view segment : absl::StrSplit(pkg, '.')) {
    ctx.Emit({{"segment", segment}},
             R"rs(
           #[allow(non_snake_case)]
           pub mod $segment$ {
           )rs");
  }
}

// Emits closing curly brace for a tree of submodules for a given `pkg`.
//
// For example for `package.uses.dots.submodule.separator` this function
// generates:
// ```
//   } // mod separator
//   } // mod submodule
//   } // mod dots
//   } // mod uses
//   } // mod package
// ```
void EmitClosingOfPackageModules(Context& ctx, absl::string_view pkg) {
  if (pkg.empty()) return;
  std::vector<absl::string_view> segments = absl::StrSplit(pkg, '.');
  absl::c_reverse(segments);

  for (absl::string_view segment : segments) {
    ctx.Emit({{"segment", segment}}, R"rs(
      } // mod $segment$
    )rs");
  }
}

// Emits `pub use <internal submodule name>::Type` for all messages and enums of
// a `non_primary_src` into the `primary_file`.
//
// `non_primary_src` has to be a non-primary src of the current `proto_library`.
void EmitPubUseOfOwnTypes(Context& ctx, const FileDescriptor& primary_file,
                          const FileDescriptor& non_primary_src) {
  auto mod = RustInternalModuleName(ctx, non_primary_src);
  for (int i = 0; i < non_primary_src.message_type_count(); ++i) {
    auto& msg = *non_primary_src.message_type(i);
    ctx.Emit({{"mod", mod}, {"Msg", msg.name()}},
             R"rs(
                        pub use crate::$mod$::$Msg$;
                        // TODO Address use for imported crates
                        pub use crate::$mod$::$Msg$View;
                        pub use crate::$mod$::$Msg$Mut;
                      )rs");
  }
  for (int i = 0; i < non_primary_src.enum_type_count(); ++i) {
    auto& enum_ = *non_primary_src.enum_type(i);
    ctx.Emit({{"mod", mod}, {"Enum", EnumRsName(enum_)}},
             R"rs(
                        pub use crate::$mod$::$Enum$;
                      )rs");
  }
}

// Emits `pub use <crate_name>::<public package>::Type` for all messages and
// enums of a `dep` into the `primary_file`. This should only be called for
// 'import public' deps.
//
// `dep` is a primary src of a dependency of the current `proto_library`.
// TODO: Add support for public import of non-primary srcs of deps.
void EmitPubUseForImportedTypes(Context& ctx,
                                const FileDescriptor& primary_file,
                                const FileDescriptor& dep) {
  std::string crate_name = GetCrateName(ctx, dep);
  for (int i = 0; i < dep.message_type_count(); ++i) {
    auto& msg = *dep.message_type(i);
    auto path = GetCrateRelativeQualifiedPath(ctx, msg);
    ctx.Emit({{"crate", crate_name}, {"pkg::Msg", path}},
             R"rs(
                        pub use $crate$::$pkg::Msg$;
                        pub use $crate$::$pkg::Msg$View;
                      )rs");
  }
  for (int i = 0; i < dep.enum_type_count(); ++i) {
    auto& enum_ = *dep.enum_type(i);
    auto path = GetCrateRelativeQualifiedPath(ctx, enum_);
    ctx.Emit({{"crate", crate_name}, {"pkg::Enum", path}},
             R"rs(
                        pub use $crate$::$pkg::Enum$;
                      )rs");
  }
}

// Emits all public imports of the current file
void EmitPublicImports(Context& ctx, const FileDescriptor& primary_file) {
  for (int i = 0; i < primary_file.public_dependency_count(); ++i) {
    auto& dep_file = *primary_file.public_dependency(i);
    // If the publicly imported file is a src of the current `proto_library`
    // we don't need to emit `pub use` here, we already do it for all srcs in
    // RustGenerator::Generate. In other words, all srcs are implicitly publicly
    // imported into the primary file for Protobuf Rust.
    // TODO: Handle the case where a non-primary src with the same
    // declared package as the primary src publicly imports a file that the
    // primary doesn't.
    if (IsInCurrentlyGeneratingCrate(ctx, dep_file)) {
      return;
    }
    EmitPubUseForImportedTypes(ctx, primary_file, dep_file);
  }
}

// Emits submodule declarations so `rustc` can find non primary sources from the
// primary file.
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

// Emits `pub use <...>::Msg` for all messages in non primary sources into their
// corresponding packages (each source file can declare a different package).
//
// Returns the non-primary sources that should be reexported from the package of
// the primary file.
std::vector<const FileDescriptor*> ReexportMessagesFromSubmodules(
    Context& ctx, const FileDescriptor& primary_file,
    absl::Span<const FileDescriptor* const> non_primary_srcs) {
  absl::btree_map<absl::string_view, std::vector<const FileDescriptor*>>
      packages;
  for (const FileDescriptor* file : non_primary_srcs) {
    packages[file->package()].push_back(file);
  }
  for (const auto& pair : packages) {
    // We will deal with messages for the package of the primary file later.
    auto fds = pair.second;
    absl::string_view package = fds[0]->package();
    if (package == primary_file.package()) continue;

    EmitOpeningOfPackageModules(ctx, package);
    for (const FileDescriptor* c : fds) {
      EmitPubUseOfOwnTypes(ctx, primary_file, *c);
    }
    EmitClosingOfPackageModules(ctx, package);
  }

  return packages[primary_file.package()];
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

  RustGeneratorContext rust_generator_context(&files_in_current_crate);

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

  std::vector<const FileDescriptor*> file_contexts;
  for (const FileDescriptor* f : files_in_current_crate) {
    file_contexts.push_back(f);
  }

  // Generating the primary file?
  if (file == &rust_generator_context.primary_file()) {
    auto non_primary_srcs = absl::MakeConstSpan(file_contexts).subspan(1);
    DeclareSubmodulesForNonPrimarySrcs(ctx, *file, non_primary_srcs);

    std::vector<const FileDescriptor*> non_primary_srcs_in_primary_package =
        ReexportMessagesFromSubmodules(ctx, *file, non_primary_srcs);

    EmitOpeningOfPackageModules(ctx, file->package());

    for (const FileDescriptor* non_primary_file :
         non_primary_srcs_in_primary_package) {
      EmitPubUseOfOwnTypes(ctx, *file, *non_primary_file);
    }
  }

  EmitPublicImports(ctx, *file);

  std::unique_ptr<io::ZeroCopyOutputStream> thunks_cc;
  std::unique_ptr<io::Printer> thunks_printer;
  if (ctx.is_cpp()) {
    thunks_cc.reset(generator_context->Open(GetThunkCcFile(ctx, *file)));
    thunks_printer = std::make_unique<io::Printer>(thunks_cc.get());

    thunks_printer->Emit({{"proto_h", GetHeaderFile(ctx, *file)}},
                         R"cc(
#include "$proto_h$"
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
    GenerateEnumDefinition(ctx, *file->enum_type(i));
    ctx.printer().PrintRaw("\n");
  }

  if (file == files_in_current_crate.front()) {
    EmitClosingOfPackageModules(ctx, file->package());
  }
  return true;
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
