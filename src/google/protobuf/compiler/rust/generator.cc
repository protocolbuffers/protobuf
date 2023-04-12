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

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/optional.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/rust/upb_kernel.h"
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

std::string GetUnderscoreDelimitedFullName(const Descriptor* msg) {
  std::string result = msg->full_name();
  absl::StrReplaceAll({{".", "_"}}, &result);
  return result;
}

std::string GetAccessorThunkName(
    const FieldDescriptor* field, absl::string_view op,
    absl::string_view underscore_delimited_full_name) {
  return absl::Substitute("__rust_proto_thunk__$0_$1_$2",
                          underscore_delimited_full_name, op, field->name());
}

bool IsSupportedFieldType(const FieldDescriptor* field) {
  return !field->is_repeated() &&
         (field->cpp_type() == FieldDescriptor::CPPTYPE_BOOL ||
          field->cpp_type() == FieldDescriptor::CPPTYPE_INT64);
}

std::string PrimitiveRsTypeName(const FieldDescriptor* field) {
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT64:
      return "i64";
    case FieldDescriptor::CPPTYPE_BOOL:
      return "bool";
    default:
      break;
  }
  ABSL_LOG(FATAL) << "Unsupported field type: " << field->type_name();
  return "";
}

void GenerateAccessorFns(const Descriptor* msg, google::protobuf::io::Printer& p,
                         absl::string_view underscore_delimited_full_name) {
  for (int i = 0; i < msg->field_count(); ++i) {
    const FieldDescriptor* field = msg->field(i);
    if (!IsSupportedFieldType(field)) {
      continue;
    }
    p.Emit(
        {
            {"field_name", field->name()},
            {"FieldType", PrimitiveRsTypeName(field)},
            {"hazzer_thunk_name",
             GetAccessorThunkName(field, "has",
                                  underscore_delimited_full_name)},
            {"getter_thunk_name",
             GetAccessorThunkName(field, "", underscore_delimited_full_name)},
            {"setter_thunk_name",
             GetAccessorThunkName(field, "set",
                                  underscore_delimited_full_name)},
            {"clearer_thunk_name",
             GetAccessorThunkName(field, "clear",
                                  underscore_delimited_full_name)},
        },
        R"rs(
             pub fn $field_name$(&self) -> Option<$FieldType$> {
               if !unsafe { $hazzer_thunk_name$(self.msg) } {
                return None;
               }
               Some(unsafe { $getter_thunk_name$(self.msg) })
             }
             pub fn $field_name$_set(&mut self, val: Option<$FieldType$>) {
               match val {
                 Some(val) => unsafe { $setter_thunk_name$(self.msg, val) },
                 None => unsafe { $clearer_thunk_name$(self.msg) },
               }
             }
           )rs");
  }
}

void GenerateAccessorThunkRsDeclarations(
    const Descriptor* msg, google::protobuf::io::Printer& p,
    std::string underscore_delimited_full_name) {
  for (int i = 0; i < msg->field_count(); ++i) {
    const FieldDescriptor* field = msg->field(i);
    if (!IsSupportedFieldType(field)) {
      continue;
    }
    p.Emit(
        {
            {"FieldType", PrimitiveRsTypeName(field)},
            {"hazzer_thunk_name",
             GetAccessorThunkName(field, "has",
                                  underscore_delimited_full_name)},
            {"getter_thunk_name",
             GetAccessorThunkName(field, "", underscore_delimited_full_name)},
            {"setter_thunk_name",
             GetAccessorThunkName(field, "set",
                                  underscore_delimited_full_name)},
            {"clearer_thunk_name",
             GetAccessorThunkName(field, "clear",
                                  underscore_delimited_full_name)},
        },
        R"rs(
            fn $hazzer_thunk_name$(raw_msg: ::__std::ptr::NonNull<u8>) -> bool;
            fn $getter_thunk_name$(raw_msg: ::__std::ptr::NonNull<u8>) -> $FieldType$;
            fn $setter_thunk_name$(raw_msg: ::__std::ptr::NonNull<u8>, val: $FieldType$);
            fn $clearer_thunk_name$(raw_msg: ::__std::ptr::NonNull<u8>);
           )rs");
  }
}

void GenerateAccessorThunksCcDefinitions(
    const Descriptor* msg, google::protobuf::io::Printer& p,
    absl::string_view underscore_delimited_full_name) {
  for (int i = 0; i < msg->field_count(); ++i) {
    const FieldDescriptor* field = msg->field(i);
    if (!IsSupportedFieldType(field)) {
      continue;
    }
    p.Emit(
        {{"field_name", field->name()},
         {"FieldType", cpp::PrimitiveTypeName(field->cpp_type())},
         {"namespace", cpp::Namespace(msg)},
         {"hazzer_thunk_name",
          GetAccessorThunkName(field, "has", underscore_delimited_full_name)},
         {"getter_thunk_name",
          GetAccessorThunkName(field, "", underscore_delimited_full_name)},
         {"setter_thunk_name",
          GetAccessorThunkName(field, "set", underscore_delimited_full_name)},
         {"clearer_thunk_name",
          GetAccessorThunkName(field, "clear",
                               underscore_delimited_full_name)}},
        R"cc(
          extern "C" {
          bool $hazzer_thunk_name$($namespace$::$Msg$* msg) {
            return msg->has_$field_name$();
          }
          $FieldType$ $getter_thunk_name$($namespace$::$Msg$* msg) {
            return msg->$field_name$();
          }
          void $setter_thunk_name$($namespace$::$Msg$* msg, $FieldType$ val) {
            msg->set_$field_name$(val);
          }
          void $clearer_thunk_name$($namespace$::$Msg$* msg) {
            msg->clear_$field_name$();
          }
          }
        )cc");
  }
}

void GenerateForCpp(const FileDescriptor* file, google::protobuf::io::Printer& p) {
  for (int i = 0; i < file->message_type_count(); ++i) {
    const Descriptor* msg = file->message_type(i);
    std::string underscore_delimited_full_name =
        GetUnderscoreDelimitedFullName(msg);
    p.Emit(
        {
            {"Msg", msg->name()},
            {"pkg_Msg", underscore_delimited_full_name},
            {"accessor_fns",
             [&] {
               GenerateAccessorFns(file->message_type(i), p,
                                   underscore_delimited_full_name);
             }},
            {"accessor_thunks",
             [&] {
               GenerateAccessorThunkRsDeclarations(
                   file->message_type(i), p, underscore_delimited_full_name);
             }},
        },
        R"rs(
          #[allow(non_camel_case_types)]
          pub struct $Msg$ {
            msg: ::__std::ptr::NonNull<u8>,
          }

          impl $Msg$ {
            pub fn new() -> Self {
              Self {
                msg: unsafe { __rust_proto_thunk__$pkg_Msg$__new() }
              }
            }
            pub fn serialize(&self) -> ::__pb::SerializedData {
              return unsafe { __rust_proto_thunk__$pkg_Msg$__serialize(self.msg) };
            }
            pub fn __unstable_cpp_repr_grant_permission_to_break(&mut self) -> ::__std::ptr::NonNull<u8> {
              self.msg
            }
            pub fn deserialize(&mut self, data: &[u8]) -> Result<(), ::__pb::ParseError> {
              let success = unsafe { __rust_proto_thunk__$pkg_Msg$__deserialize(
                self.msg,
                ::__pb::SerializedData::from_raw_parts(
                  ::__std::ptr::NonNull::new(data.as_ptr() as *mut _).unwrap(),
                  data.len()))
              };
              success.then_some(()).ok_or(::__pb::ParseError)
            }
            $accessor_fns$
          }

          extern "C" {
            fn __rust_proto_thunk__$pkg_Msg$__new() -> ::__std::ptr::NonNull<u8>;
            fn __rust_proto_thunk__$pkg_Msg$__serialize(raw_msg: ::__std::ptr::NonNull<u8>) -> ::__pb::SerializedData;
            fn __rust_proto_thunk__$pkg_Msg$__deserialize(raw_msg: ::__std::ptr::NonNull<u8>, data: ::__pb::SerializedData) -> bool;

            $accessor_thunks$
          }
        )rs");
  }
}

void GenerateThunksForCpp(const FileDescriptor* file, google::protobuf::io::Printer& p) {
  for (int i = 0; i < file->message_type_count(); ++i) {
    const Descriptor* msg = file->message_type(i);
    std::string underscore_delimited_full_name =
        GetUnderscoreDelimitedFullName(msg);
    p.Emit(
        {
            {"Msg", msg->name()},
            {"pkg_Msg", GetUnderscoreDelimitedFullName(msg)},
            {"namespace", cpp::Namespace(msg)},
            {"accessor_thunks",
             [&] {
               GenerateAccessorThunksCcDefinitions(
                   file->message_type(i), p, underscore_delimited_full_name);
             }},
        },
        R"cc(
          extern "C" {
          void* __rust_proto_thunk__$pkg_Msg$__new() { return new $namespace$::$Msg$(); }

          google::protobuf::rust_internal::SerializedData
          __rust_proto_thunk__$pkg_Msg$__serialize($namespace$::$Msg$* msg) {
            return google::protobuf::rust_internal::SerializeMsg(msg);
          }

          bool __rust_proto_thunk__$pkg_Msg$__deserialize(
              $namespace$::$Msg$* msg,
              google::protobuf::rust_internal::SerializedData data) {
            return msg->ParseFromArray(data.data, data.len);
          }

          $accessor_thunks$
          }
        )cc");
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
      rust::UpbKernel k;
      k.Generate(file, p);
      break;
    case Kernel::kCpp:
      GenerateForCpp(file, p);

      auto thunksfile = absl::WrapUnique(
          generator_context->Open(absl::StrCat(basename, ".pb.thunks.cc")));
      google::protobuf::io::Printer thunks(thunksfile.get());
      thunks.Emit({{"basename", basename}},
                  R"cc(
#include "$basename$.pb.h"
#include "google/protobuf/rust/cpp_kernel/cpp_api.h"
                  )cc");
      GenerateThunksForCpp(file, thunks);
      break;
  }
  return true;
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
