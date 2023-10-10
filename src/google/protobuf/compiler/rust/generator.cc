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
void EmitOpeningOfPackageModules(absl::string_view pkg,
                                 Context<FileDescriptor> file) {
  if (pkg.empty()) return;
  for (absl::string_view segment : absl::StrSplit(pkg, '.')) {
    file.Emit({{"segment", segment}},
              R"rs(
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
void EmitClosingOfPackageModules(absl::string_view pkg,
                                 Context<FileDescriptor> file) {
  if (pkg.empty()) return;
  std::vector<absl::string_view> segments = absl::StrSplit(pkg, '.');
  absl::c_reverse(segments);

  for (absl::string_view segment : segments) {
    file.Emit({{"segment", segment}}, R"rs(
      } // mod $segment$
    )rs");
  }
}

// Emits `pub use <internal submodule name>::Msg` for all messages of a
// `non_primary_src` into the `primary_file`.
//
// `non_primary_src` has to be a non-primary src of the current `proto_library`.
void EmitPubUseOfOwnMessages(Context<FileDescriptor>& primary_file,
                             const Context<FileDescriptor>& non_primary_src) {
  for (int i = 0; i < non_primary_src.desc().message_type_count(); ++i) {
    auto msg = primary_file.WithDesc(non_primary_src.desc().message_type(i));
    auto mod = RustInternalModuleName(non_primary_src);
    auto name = msg.desc().name();
    primary_file.Emit({{"mod", mod}, {"Msg", name}},
                      R"rs(
                        pub use crate::$mod$::$Msg$;
                        // TODO Address use for imported crates
                        pub use crate::$mod$::$Msg$View;
                      )rs");
  }
}

// Emits `pub use <crate_name>::<public package>::Msg` for all messages of a
// `dep` into the `primary_file`. This should only be called for 'import public'
// deps.
//
// `dep` is a primary src of a dependency of the current `proto_library`.
// TODO: Add support for public import of non-primary srcs of deps.
void EmitPubUseForImportedMessages(Context<FileDescriptor>& primary_file,
                                   const Context<FileDescriptor>& dep) {
  std::string crate_name = GetCrateName(dep);
  for (int i = 0; i < dep.desc().message_type_count(); ++i) {
    auto msg = primary_file.WithDesc(dep.desc().message_type(i));
    auto path = GetCrateRelativeQualifiedPath(msg);
    primary_file.Emit({{"crate", crate_name}, {"pkg::Msg", path}},
                      R"rs(
                        pub use $crate$::$pkg::Msg$;
                        pub use $crate$::$pkg::Msg$View;
                      )rs");
  }
}

// Emits all public imports of the current file
void EmitPublicImports(
    Context<FileDescriptor>& primary_file,
    const std::vector<const FileDescriptor*>& files_in_current_crate) {
  absl::flat_hash_set<const FileDescriptor*> files(
      files_in_current_crate.begin(), files_in_current_crate.end());
  for (int i = 0; i < primary_file.desc().public_dependency_count(); ++i) {
    auto dep_file = primary_file.desc().public_dependency(i);
    // If the publicly imported file is a src of the current `proto_library`
    // we don't need to emit `pub use` here, we already do it for all srcs in
    // RustGenerator::Generate. In other words, all srcs are implicitly publicly
    // imported into the primary file for Protobuf Rust.
    // TODO: Handle the case where a non-primary src with the same
    // declared package as the primary src publicly imports a file that the
    // primary doesn't.
    if (files.contains(dep_file)) continue;
    auto dep = primary_file.WithDesc(dep_file);
    EmitPubUseForImportedMessages(primary_file, dep);
  }
}

// Emits submodule declarations so `rustc` can find non primary sources from the
// primary file.
void DeclareSubmodulesForNonPrimarySrcs(
    Context<FileDescriptor>& primary_file,
    absl::Span<const Context<FileDescriptor>> non_primary_srcs) {
  std::string primary_file_path = GetRsFile(primary_file);
  RelativePath primary_relpath(primary_file_path);
  for (const auto& non_primary_src : non_primary_srcs) {
    std::string non_primary_file_path = GetRsFile(non_primary_src);
    std::string relative_mod_path =
        primary_relpath.Relative(RelativePath(non_primary_file_path));
    primary_file.Emit({{"file_path", relative_mod_path},
                       {"foo", primary_file_path},
                       {"bar", non_primary_file_path},
                       {"mod_name", RustInternalModuleName(non_primary_src)}},
                      R"rs(
                        #[path="$file_path$"]
                        pub mod $mod_name$;
                      )rs");
  }
}

// Emits `pub use <...>::Msg` for all messages in non primary sources into their
// corresponding packages (each source file can declare a different package).
//
// Returns the non-primary sources that should be reexported from the package of
// the primary file.
std::vector<const Context<FileDescriptor>*> ReexportMessagesFromSubmodules(
    Context<FileDescriptor>& primary_file,
    absl::Span<const Context<FileDescriptor>> non_primary_srcs) {
  absl::btree_map<absl::string_view,
                  std::vector<const Context<FileDescriptor>*>>
      packages;
  for (const Context<FileDescriptor>& ctx : non_primary_srcs) {
    packages[ctx.desc().package()].push_back(&ctx);
  }
  for (const auto& pair : packages) {
    // We will deal with messages for the package of the primary file later.
    auto fds = pair.second;
    absl::string_view package = fds[0]->desc().package();
    if (package == primary_file.desc().package()) continue;

    EmitOpeningOfPackageModules(package, primary_file);
    for (const Context<FileDescriptor>* c : fds) {
      EmitPubUseOfOwnMessages(primary_file, *c);
    }
    EmitClosingOfPackageModules(package, primary_file);
  }

  return packages[primary_file.desc().package()];
}
}  // namespace

bool RustGenerator::Generate(const FileDescriptor* file_desc,
                             const std::string& parameter,
                             GeneratorContext* generator_context,
                             std::string* error) const {
  absl::StatusOr<Options> opts = Options::Parse(parameter);
  if (!opts.ok()) {
    *error = std::string(opts.status().message());
    return false;
  }

  Context<FileDescriptor> file(&*opts, file_desc, nullptr);

  auto outfile = absl::WrapUnique(generator_context->Open(GetRsFile(file)));
  io::Printer printer(outfile.get());
  file = file.WithPrinter(&printer);

  // Convenience shorthands for common symbols.
  auto v = file.printer().WithVars({
      {"std", "::__std"},
      {"pb", "::__pb"},
      {"pbi", "::__pb::__internal"},
      {"pbr", "::__pb::__runtime"},
      {"NonNull", "::__std::ptr::NonNull"},
      {"Phantom", "::__std::marker::PhantomData"},
  });

  file.Emit({{"kernel", KernelRsName(file.opts().kernel)}}, R"rs(
    extern crate protobuf_$kernel$ as __pb;
    extern crate std as __std;

  )rs");

  std::vector<const FileDescriptor*> files_in_current_crate;
  generator_context->ListParsedFiles(&files_in_current_crate);
  std::vector<Context<FileDescriptor>> file_contexts;
  for (const FileDescriptor* f : files_in_current_crate) {
    file_contexts.push_back(file.WithDesc(*f));
  }

  // Generating the primary file?
  if (file_desc == files_in_current_crate.front()) {
    auto non_primary_srcs = absl::MakeConstSpan(file_contexts).subspan(1);
    DeclareSubmodulesForNonPrimarySrcs(file, non_primary_srcs);

    std::vector<const Context<FileDescriptor>*>
        non_primary_srcs_in_primary_package =
            ReexportMessagesFromSubmodules(file, non_primary_srcs);

    EmitOpeningOfPackageModules(file.desc().package(), file);

    for (const Context<FileDescriptor>* non_primary_file :
         non_primary_srcs_in_primary_package) {
      EmitPubUseOfOwnMessages(file, *non_primary_file);
    }
  }

  EmitPublicImports(file, files_in_current_crate);

  std::unique_ptr<io::ZeroCopyOutputStream> thunks_cc;
  std::unique_ptr<io::Printer> thunks_printer;
  if (file.is_cpp()) {
    thunks_cc.reset(generator_context->Open(GetThunkCcFile(file)));
    thunks_printer = std::make_unique<io::Printer>(thunks_cc.get());

    thunks_printer->Emit({{"proto_h", GetHeaderFile(file)}},
                         R"cc(
#include "$proto_h$"
#include "google/protobuf/rust/cpp_kernel/cpp_api.h"
                         )cc");
  }

  for (int i = 0; i < file.desc().message_type_count(); ++i) {
    auto msg = file.WithDesc(file.desc().message_type(i));

    GenerateRs(msg);
    msg.printer().PrintRaw("\n");

    if (file.is_cpp()) {
      auto thunks_msg = msg.WithPrinter(thunks_printer.get());

      thunks_msg.Emit({{"Msg", msg.desc().full_name()}}, R"cc(
        // $Msg$
      )cc");
      GenerateThunksCc(thunks_msg);
      thunks_msg.printer().PrintRaw("\n");
    }
  }
  if (file_desc == files_in_current_crate.front()) {
    EmitClosingOfPackageModules(file.desc().package(), file);
  }
  return true;
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
