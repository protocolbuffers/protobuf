// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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
//     * Neither the name of Google Inc. nor the names of its
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

#include <string>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

bool ExperimentalRustGeneratorEnabled(
    const std::vector<std::pair<std::string, std::string>>& options) {
  static constexpr std::pair<absl::string_view, absl::string_view> kMagicValue =
      {"experimental-codegen", "enabled"};

  return absl::c_any_of(
      options, [](std::pair<absl::string_view, absl::string_view> pair) {
        return pair == kMagicValue;
      });
}

// Marks which kernel the Rust codegen should generate code for.
enum class Kernel {
  kUpb,
  kCpp,
};

absl::optional<Kernel> ParseKernelConfiguration(
    const std::vector<std::pair<std::string, std::string>>& options) {
  for (const auto& pair : options) {
    if (pair.first == "kernel") {
      if (pair.second == "upb") {
        return Kernel::kUpb;
      }
      if (pair.second == "cpp") {
        return Kernel::kCpp;
      }
    }
  }
  return absl::nullopt;
}

std::string GetCrateName(const FileDescriptor* dependency) {
  absl::string_view path = dependency->name();
  auto basename = path.substr(path.rfind('/') + 1);
  return absl::StrReplaceAll(basename, {
                                           {".", "_"},
                                           {"-", "_"},
                                       });
}

std::string GetFileExtensionForKernel(Kernel kernel) {
  switch (kernel) {
    case Kernel::kUpb:
      return ".u.pb.rs";
    case Kernel::kCpp:
      return ".c.pb.rs";
  }
  ABSL_LOG(FATAL) << "Unknown kernel type: ";
  return "";
}

// The prefix used by the UPB compiler to generate unique function names.
// TODO(b/275708201): Determine a principled way to generate names of UPB
// accessors.
std::string GetUpbMessagePrefix(const Descriptor* msg_descriptor) {
  std::string upb_msg_prefix = msg_descriptor->full_name();
  absl::StrReplaceAll({{".", "_"}}, &upb_msg_prefix);
  return upb_msg_prefix;
}

void GenerateMessageFunctionsForUpb(const Descriptor* msg_descriptor,
                                    google::protobuf::io::Printer& p) {
  p.Emit({{"Msg", msg_descriptor->name()},
          {"pkg_Msg", GetUpbMessagePrefix(msg_descriptor)}},
         R"rs(
    impl $Msg$ {
      pub fn new() -> Self {
        let arena = unsafe { ::__pb::Arena::new() };
        let msg = unsafe { $pkg_Msg$_new(arena) };
        $Msg$ { msg, arena }
      }

      pub fn serialize(&self) -> ::__pb::SerializedData {
        let arena = unsafe { ::__pb::__runtime::upb_Arena_New() };
        let mut len = 0;
        let chars = unsafe { $pkg_Msg$_serialize(self.msg, arena, &mut len) };
        unsafe {::__pb::SerializedData::from_raw_parts(arena, chars, len)}
      }
    }

    extern "C" {
      fn $pkg_Msg$_new(arena: *mut ::__pb::Arena) -> ::__std::ptr::NonNull<u8>;
      fn $pkg_Msg$_serialize(
        msg: ::__std::ptr::NonNull<u8>,
        arena: *mut ::__pb::Arena,
        len: &mut usize) -> ::__std::ptr::NonNull<u8>;
    }
  )rs");
}

void GenerateForUpb(const FileDescriptor* file, google::protobuf::io::Printer& p) {
  for (int i = 0; i < file->message_type_count(); ++i) {
    auto msg_descriptor = file->message_type(i);

    p.Emit({{"Msg", msg_descriptor->name()},
            {"ImplMessageFunctions",
             [&] { GenerateMessageFunctionsForUpb(msg_descriptor, p); }}},
           R"rs(
      pub struct $Msg$ {
        msg: ::__std::ptr::NonNull<u8>,
        arena: *mut ::__pb::Arena,
      }

      $ImplMessageFunctions$;
    )rs");
  }
}

void GenerateForCpp(const FileDescriptor* file, google::protobuf::io::Printer& p) {
  for (int i = 0; i < file->message_type_count(); ++i) {
    // TODO(b/272728844): Implement real logic
    p.Emit({{"Msg", file->message_type(i)->name()}},
           R"rs(
      pub struct $Msg$ {
        msg: ::__std::ptr::NonNull<u8>,
      }

      impl $Msg$ {
        pub fn new() -> Self { Self { msg: ::__std::ptr::NonNull::dangling() }}
        pub fn serialize(&self) -> Vec<u8> { vec![] }
      }
    )rs");
  }
}

std::string GetKernelRustName(Kernel kernel) {
  switch (kernel) {
    case Kernel::kUpb:
      return "upb";
    case Kernel::kCpp:
      return "cpp";
  }
  ABSL_LOG(FATAL) << "Unknown kernel type: ";
  return "";
}

bool RustGenerator::Generate(const FileDescriptor* file,
                             const std::string& parameter,
                             GeneratorContext* generator_context,
                             std::string* error) const {
  std::vector<std::pair<std::string, std::string>> options;
  ParseGeneratorParameter(parameter, &options);

  if (!ExperimentalRustGeneratorEnabled(options)) {
    *error =
        "The Rust codegen is highly experimental. Future versions will break "
        "existing code. Use at your own risk. You can opt-in by passing "
        "'experimental-codegen=enabled' to '--rust_out'.";
    return false;
  }

  absl::optional<Kernel> kernel = ParseKernelConfiguration(options);
  if (!kernel.has_value()) {
    *error =
        "Mandatory option `kernel` missing, please specify `cpp` or "
        "`upb`.";
    return false;
  }

  auto basename = StripProto(file->name());
  auto outfile = absl::WrapUnique(generator_context->Open(
      absl::StrCat(basename, GetFileExtensionForKernel(*kernel))));

  google::protobuf::io::Printer p(outfile.get());
  p.Emit({{"kernel", GetKernelRustName(*kernel)}}, R"rs(
    extern crate protobuf_$kernel$ as __pb;
    extern crate std as __std;

  )rs");

  // TODO(b/270124215): Delete the following "placeholder impl" of `import
  // public`. Also make sure to figure out how to map FileDescriptor#name to
  // Rust crate names (currently Bazel labels).
  for (int i = 0; i < file->public_dependency_count(); ++i) {
    const FileDescriptor* dep = file->public_dependency(i);
    std::string crate_name = GetCrateName(dep);
    for (int j = 0; j < dep->message_type_count(); ++j) {
      // TODO(b/272728844): Implement real logic
      p.Emit(
          {{"crate", crate_name}, {"type_name", dep->message_type(j)->name()}},
          R"rs(
                pub use $crate$::$type_name$;
              )rs");
    }
  }

  switch (*kernel) {
    case Kernel::kUpb:
      GenerateForUpb(file, p);
      break;
    case Kernel::kCpp:
      GenerateForCpp(file, p);
      break;
  }
  return true;
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
