// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "google/protobuf/compiler/rust/generator.h"

#include <memory>
#include <string>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/message.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {
namespace {
void EmitOpeningOfPackageModules(Context<FileDescriptor> file) {
  if (file.desc().package().empty()) return;
  for (absl::string_view segment : absl::StrSplit(file.desc().package(), '.')) {
    file.Emit({{"segment", segment}},
              R"rs(
           pub mod $segment$ {
           )rs");
  }
}

void EmitClosingOfPackageModules(Context<FileDescriptor> file) {
  if (file.desc().package().empty()) return;
  std::vector<absl::string_view> segments =
      absl::StrSplit(file.desc().package(), '.');
  absl::c_reverse(segments);

  for (absl::string_view segment : segments) {
    file.Emit({{"segment", segment}}, R"rs(
      } // mod $segment$
    )rs");
  }
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
  });

  file.Emit({{"kernel", KernelRsName(file.opts().kernel)}}, R"rs(
    extern crate protobuf_$kernel$ as __pb;
    extern crate std as __std;

  )rs");
  EmitOpeningOfPackageModules(file);

  // TODO(b/270124215): Delete the following "placeholder impl" of `import
  // public`. Also make sure to figure out how to map FileDescriptor#name to
  // Rust crate names (currently Bazel labels).
  for (int i = 0; i < file.desc().public_dependency_count(); ++i) {
    auto dep = file.WithDesc(file.desc().public_dependency(i));
    std::string crate_name = GetCrateName(dep);
    for (int j = 0; j < dep.desc().message_type_count(); ++j) {
      auto msg = file.WithDesc(dep.desc().message_type(j));
      file.Emit(
          {
              {"crate", crate_name},
              {"pkg::Msg", GetCrateRelativeQualifiedPath(msg)},
          },
          R"rs(
            pub use $crate$::$pkg::Msg$;
          )rs");
    }
  }

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

    MessageGenerator gen(msg);
    gen.GenerateRs(msg);
    msg.printer().PrintRaw("\n");

    if (file.is_cpp()) {
      auto thunks_msg = msg.WithPrinter(thunks_printer.get());

      thunks_msg.Emit({{"Msg", msg.desc().full_name()}}, R"cc(
        // $Msg$
      )cc");
      gen.GenerateThunksCc(thunks_msg);
      thunks_msg.printer().PrintRaw("\n");
    }
  }
  EmitClosingOfPackageModules(file);
  return true;
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
