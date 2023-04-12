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

#include "google/protobuf/compiler/rust/cpp_kernel.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

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
  return !field->is_repeated() && !field->options().has_ctype() &&
         (field->type() == FieldDescriptor::TYPE_BOOL ||
          field->type() == FieldDescriptor::TYPE_INT64 ||
          field->type() == FieldDescriptor::TYPE_BYTES);
}

absl::string_view PrimitiveRsTypeName(const FieldDescriptor* field) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_BOOL:
      return "bool";
    case FieldDescriptor::TYPE_INT64:
      return "i64";
    case FieldDescriptor::TYPE_BYTES:
      return "&[u8]";
    default:
      break;
  }
  ABSL_LOG(FATAL) << "Unsupported field type: " << field->type_name();
  return "";
}

void EmitGetterExpr(const FieldDescriptor* field, google::protobuf::io::Printer& p,
                    absl::string_view underscore_delimited_full_name) {
  std::string thunk_name =
      GetAccessorThunkName(field, "get", underscore_delimited_full_name);
  switch (field->type()) {
    case FieldDescriptor::TYPE_BYTES:
      p.Emit({{"getter_thunk_name", thunk_name}},
             R"rs(
              let val = unsafe { $getter_thunk_name$(self.msg) };
              Some(unsafe { ::__std::slice::from_raw_parts(val.ptr, val.len) })
            )rs");
      return;
    default:
      p.Emit({{"getter_thunk_name", thunk_name}},
             R"rs(
              Some(unsafe { $getter_thunk_name$(self.msg) })
            )rs");
  }
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
             GetAccessorThunkName(field, "get",
                                  underscore_delimited_full_name)},
            {"getter_expr",
             [&] { EmitGetterExpr(field, p, underscore_delimited_full_name); }},
            {"setter_thunk_name",
             GetAccessorThunkName(field, "set",
                                  underscore_delimited_full_name)},
            {"setter_args",
             [&] {
               switch (field->type()) {
                 case FieldDescriptor::TYPE_BYTES:
                   p.Emit("val.as_ptr(), val.len()");
                   return;
                 default:
                   p.Emit("val");
               }
             }},
            {"clearer_thunk_name",
             GetAccessorThunkName(field, "clear",
                                  underscore_delimited_full_name)},
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

void GenerateAccessorThunkRsDeclarations(
    const Descriptor* msg, google::protobuf::io::Printer& p,
    std::string underscore_delimited_full_name) {
  for (int i = 0; i < msg->field_count(); ++i) {
    const FieldDescriptor* field = msg->field(i);
    if (!IsSupportedFieldType(field)) {
      continue;
    }
    absl::string_view type_name = PrimitiveRsTypeName(field);
    p.Emit(
        {
            {"FieldType", type_name},
            {"GetterReturnType",
             [&] {
               switch (field->type()) {
                 case FieldDescriptor::TYPE_BYTES:
                   p.Emit("::__pb::PtrAndLen");
                   return;
                 default:
                   p.Emit(type_name);
               }
             }},
            {"hazzer_thunk_name",
             GetAccessorThunkName(field, "has",
                                  underscore_delimited_full_name)},
            {"getter_thunk_name",
             GetAccessorThunkName(field, "get",
                                  underscore_delimited_full_name)},
            {"setter_thunk_name",
             GetAccessorThunkName(field, "set",
                                  underscore_delimited_full_name)},
            {"setter_params",
             [&] {
               switch (field->type()) {
                 case FieldDescriptor::TYPE_BYTES:
                   p.Emit("val: *const u8, len: usize");
                   return;
                 default:
                   p.Emit({{"type_name", type_name}}, "val: $type_name$");
               }
             }},
            {"clearer_thunk_name",
             GetAccessorThunkName(field, "clear",
                                  underscore_delimited_full_name)},
        },
        R"rs(
            fn $hazzer_thunk_name$(raw_msg: ::__std::ptr::NonNull<u8>) -> bool;
            fn $getter_thunk_name$(raw_msg: ::__std::ptr::NonNull<u8>) -> $GetterReturnType$;;
            fn $setter_thunk_name$(raw_msg: ::__std::ptr::NonNull<u8>, $setter_params$);
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
    const char* type_name = cpp::PrimitiveTypeName(field->cpp_type());
    p.Emit(
        {{"field_name", field->name()},
         {"FieldType", type_name},
         {"GetterReturnType",
          [&] {
            switch (field->type()) {
              case FieldDescriptor::TYPE_BYTES:
                p.Emit("::google::protobuf::rust_internal::PtrAndLen");
                return;
              default:
                p.Emit(type_name);
            }
          }},
         {"namespace", cpp::Namespace(msg)},
         {"hazzer_thunk_name",
          GetAccessorThunkName(field, "has", underscore_delimited_full_name)},
         {"getter_thunk_name",
          GetAccessorThunkName(field, "get", underscore_delimited_full_name)},
         {"getter_body",
          [&] {
            switch (field->type()) {
              case FieldDescriptor::TYPE_BYTES:
                p.Emit({{"field_name", field->name()}}, R"cc(
                  absl::string_view val = msg->$field_name$();
                  return google::protobuf::rust_internal::PtrAndLen(val.data(), val.size());
                )cc");
                return;
              default:
                p.Emit(R"cc(return msg->$field_name$();)cc");
            }
          }},
         {"setter_thunk_name",
          GetAccessorThunkName(field, "set", underscore_delimited_full_name)},
         {"setter_params",
          [&] {
            switch (field->type()) {
              case FieldDescriptor::TYPE_BYTES:
                p.Emit("const char* ptr, size_t size");
                return;
              default:
                p.Emit({{"type_name", type_name}}, "$type_name$ val");
            }
          }},
         {"setter_args",
          [&] {
            switch (field->type()) {
              case FieldDescriptor::TYPE_BYTES:
                p.Emit("absl::string_view(ptr, size)");
                return;
              default:
                p.Emit("val");
            }
          }},
         {"clearer_thunk_name",
          GetAccessorThunkName(field, "clear",
                               underscore_delimited_full_name)}},
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

void CppKernel::Generate(const FileDescriptor* file,
                         google::protobuf::io::Printer& p) const {
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

void CppKernel::GenerateThunks(const FileDescriptor* file,
                               google::protobuf::io::Printer& p) const {
  auto basename = StripProto(file->name());
  p.Emit({{"basename", basename}},
         R"cc(
#include "$basename$.pb.h"
#include "google/protobuf/rust/cpp_kernel/cpp_api.h"
         )cc");

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

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
