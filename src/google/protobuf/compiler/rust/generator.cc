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

#include "absl/log/absl_log.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/compiler/rust/upb_kernel.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {
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
    file.Emit({{"segment", segment}},
              R"rs(
           } // mod $segment$
           )rs");
  }
}

void EmitGetterExpr(Context<FieldDescriptor> field) {
  std::string thunk_name = GetAccessorThunkName(field, "get");
  switch (field.desc().type()) {
    case FieldDescriptor::TYPE_BYTES:
      field.Emit({{"getter_thunk_name", thunk_name}},
                 R"rs(
              let val = unsafe { $getter_thunk_name$(self.msg) };
              Some(unsafe { ::__std::slice::from_raw_parts(val.ptr, val.len) })
            )rs");
      return;
    default:
      field.Emit({{"getter_thunk_name", thunk_name}},
                 R"rs(
              Some(unsafe { $getter_thunk_name$(self.msg) })
            )rs");
  }
}

void GenerateAccessorFns(Context<Descriptor> msg) {
  for (int i = 0; i < msg.desc().field_count(); ++i) {
    auto field = msg.WithDesc(msg.desc().field(i));
    if (!IsSupportedFieldType(field)) {
      continue;
    }
    field.Emit(
        {
            {"field_name", field.desc().name()},
            {"FieldType", PrimitiveRsTypeName(field)},
            {"hazzer_thunk_name", GetAccessorThunkName(field, "has")},
            {"getter_thunk_name", GetAccessorThunkName(field, "get")},
            {"getter_expr", [&] { EmitGetterExpr(field); }},
            {"setter_thunk_name", GetAccessorThunkName(field, "set")},
            {"setter_args",
             [&] {
               switch (field.desc().type()) {
                 case FieldDescriptor::TYPE_BYTES:
                   field.Emit("val.as_ptr(), val.len()");
                   return;
                 default:
                   field.Emit("val");
               }
             }},
            {"clearer_thunk_name", GetAccessorThunkName(field, "clear")},
        },
        R"rs(
             pub fn $field_name$(&self) -> Option<$FieldType$> {
               if !unsafe { $hazzer_thunk_name$(self.msg) } {
                return None;
               }
               $getter_expr$
             }
             pub fn $field_name$_set(&mut self, val: Option<$FieldType$>) {
               match val {
                 Some(val) => unsafe { $setter_thunk_name$(self.msg, $setter_args$) },
                 None => unsafe { $clearer_thunk_name$(self.msg) },
               }
             }
           )rs");
  }
}

void GenerateAccessorThunkRsDeclarations(Context<Descriptor> msg) {
  for (int i = 0; i < msg.desc().field_count(); ++i) {
    auto field = msg.WithDesc(msg.desc().field(i));
    if (!IsSupportedFieldType(field)) {
      continue;
    }
    absl::string_view type_name = PrimitiveRsTypeName(field);
    field.Emit(
        {
            {"FieldType", type_name},
            {"GetterReturnType",
             [&] {
               switch (field.desc().type()) {
                 case FieldDescriptor::TYPE_BYTES:
                   field.Emit("::__pb::PtrAndLen");
                   return;
                 default:
                   field.Emit(type_name);
               }
             }},
            {"hazzer_thunk_name", GetAccessorThunkName(field, "has")},
            {"getter_thunk_name", GetAccessorThunkName(field, "get")},
            {"setter_thunk_name", GetAccessorThunkName(field, "set")},
            {"setter_params",
             [&] {
               switch (field.desc().type()) {
                 case FieldDescriptor::TYPE_BYTES:
                   field.Emit("val: *const u8, len: usize");
                   return;
                 default:
                   field.Emit({{"type_name", type_name}}, "val: $type_name$");
               }
             }},
            {"clearer_thunk_name", GetAccessorThunkName(field, "clear")},
        },
        R"rs(
            fn $hazzer_thunk_name$(raw_msg: ::__std::ptr::NonNull<u8>) -> bool;
            fn $getter_thunk_name$(raw_msg: ::__std::ptr::NonNull<u8>) -> $GetterReturnType$;;
            fn $setter_thunk_name$(raw_msg: ::__std::ptr::NonNull<u8>, $setter_params$);
            fn $clearer_thunk_name$(raw_msg: ::__std::ptr::NonNull<u8>);
           )rs");
  }
}

void GenerateAccessorThunksCcDefinitions(Context<Descriptor> msg) {
  for (int i = 0; i < msg.desc().field_count(); ++i) {
    auto field = msg.WithDesc(msg.desc().field(i));
    if (!IsSupportedFieldType(field)) {
      continue;
    }
    absl::string_view type_name =
        cpp::PrimitiveTypeName(field.desc().cpp_type());
    field.Emit(
        {{"field_name", field.desc().name()},
         {"FieldType", type_name},
         {"GetterReturnType",
          [&] {
            switch (field.desc().type()) {
              case FieldDescriptor::TYPE_BYTES:
                field.Emit("::google::protobuf::rust_internal::PtrAndLen");
                return;
              default:
                field.Emit(type_name);
            }
          }},
         {"namespace", cpp::Namespace(&field.desc())},
         {"hazzer_thunk_name", GetAccessorThunkName(field, "has")},
         {"getter_thunk_name", GetAccessorThunkName(field, "get")},
         {"getter_body",
          [&] {
            switch (field.desc().type()) {
              case FieldDescriptor::TYPE_BYTES:
                field.Emit({{"field_name", field.desc().name()}}, R"cc(
                  absl::string_view val = msg->$field_name$();
                  return google::protobuf::rust_internal::PtrAndLen(val.data(), val.size());
                )cc");
                return;
              default:
                field.Emit(R"cc(return msg->$field_name$();)cc");
            }
          }},
         {"setter_thunk_name", GetAccessorThunkName(field, "set")},
         {"setter_params",
          [&] {
            switch (field.desc().type()) {
              case FieldDescriptor::TYPE_BYTES:
                field.Emit("const char* ptr, size_t size");
                return;
              default:
                field.Emit({{"type_name", type_name}}, "$type_name$ val");
            }
          }},
         {"setter_args",
          [&] {
            switch (field.desc().type()) {
              case FieldDescriptor::TYPE_BYTES:
                field.Emit("absl::string_view(ptr, size)");
                return;
              default:
                field.Emit("val");
            }
          }},
         {"clearer_thunk_name", GetAccessorThunkName(field, "clear")}},
        R"cc(
          extern "C" {
          bool $hazzer_thunk_name$($namespace$::$Msg$* msg) {
            return msg->has_$field_name$();
          }
          $GetterReturnType$ $getter_thunk_name$($namespace$::$Msg$* msg) {
            $getter_body$
          }
          void $setter_thunk_name$($namespace$::$Msg$* msg, $setter_params$) {
            msg->set_$field_name$($setter_args$);
          }
          void $clearer_thunk_name$($namespace$::$Msg$* msg) {
            msg->clear_$field_name$();
          }
          }
        )cc");
  }
}

void GenerateForCpp(Context<FileDescriptor> file) {
  for (int i = 0; i < file.desc().message_type_count(); ++i) {
    auto msg = file.WithDesc(file.desc().message_type(i));
    msg.Emit(
        {
            {"Msg", msg.desc().name()},
            {"pkg_Msg", GetUnderscoreDelimitedFullName(msg)},
            {"accessor_fns", [&] { GenerateAccessorFns(msg); }},
            {"accessor_thunks",
             [&] { GenerateAccessorThunkRsDeclarations(msg); }},
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

void GenerateThunksForCpp(Context<FileDescriptor> file) {
  for (int i = 0; i < file.desc().message_type_count(); ++i) {
    auto msg = file.WithDesc(file.desc().message_type(i));
    file.Emit(
        {
            {"Msg", msg.desc().name()},
            {"pkg_Msg", GetUnderscoreDelimitedFullName(msg)},
            {"namespace", cpp::Namespace(&msg.desc())},
            {"accessor_thunks",
             [&] { GenerateAccessorThunksCcDefinitions(msg); }},
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
  ABSL_LOG(FATAL) << "Unknown kernel type: " << static_cast<int>(kernel);
  return "";
}

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

  file.Emit({{"kernel", GetKernelRustName(opts->kernel)}}, R"rs(
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
      // TODO(b/272728844): Implement real logic
      file.Emit({{"crate", crate_name},
                 {"pkg::Msg", GetCrateRelativeQualifiedPath(msg)}},
                R"rs(
                pub use $crate$::$pkg::Msg$;
              )rs");
    }
  }

  switch (opts->kernel) {
    case Kernel::kUpb:
      rust::UpbKernel().Generate(&file.desc(), printer);
      break;
    case Kernel::kCpp:
      GenerateForCpp(file);

      auto thunksfile =
          absl::WrapUnique(generator_context->Open(GetThunkCcFile(file)));
      google::protobuf::io::Printer thunks(thunksfile.get());
      auto thunk_file = file.WithPrinter(&thunks);

      thunk_file.Emit({{"proto_h", GetHeaderFile(file)}},
                      R"cc(
#include "$proto_h$"
#include "google/protobuf/rust/cpp_kernel/cpp_api.h"
                      )cc");
      GenerateThunksForCpp(thunk_file);
      break;
  }
  EmitClosingOfPackageModules(file);
  return true;
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
