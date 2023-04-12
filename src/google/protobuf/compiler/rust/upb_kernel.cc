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

#include "google/protobuf/compiler/rust/upb_kernel.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

// The prefix used by the UPB compiler to generate unique function names.
// TODO(b/275708201): Determine a principled way to generate names of UPB
// accessors.
std::string UpbMsgPrefix(const Descriptor* msg_descriptor) {
  std::string upb_msg_prefix = msg_descriptor->full_name();
  absl::StrReplaceAll({{".", "_"}}, &upb_msg_prefix);
  return upb_msg_prefix;
}

std::string UpbThunkName(const FieldDescriptor* field,
                         const std::string& msg_prefix,
                         const std::string& accessor) {
  return absl::StrCat(msg_prefix, accessor, field->name());
}

bool IsSupported(const FieldDescriptor* field) {
  // Per v0 design document.
  return field->is_optional() && !field->is_repeated();
}

absl::string_view RustTypeForField(const FieldDescriptor::Type field_type) {
  switch (field_type) {
    case FieldDescriptor::Type::TYPE_BOOL:
      return "bool";
    case FieldDescriptor::Type::TYPE_INT64:
      return "i64";
    case FieldDescriptor::Type::TYPE_BYTES:
      return "&[u8]";
    default: {
      ABSL_LOG(FATAL) << "Unsupported field type: " << field_type;
    }
  }
}

absl::string_view UpbTypeForField(const FieldDescriptor::Type field_type) {
  switch (field_type) {
    case FieldDescriptor::Type::TYPE_BOOL:
      return "bool";
    case FieldDescriptor::Type::TYPE_INT64:
      return "i64";
    case FieldDescriptor::Type::TYPE_BYTES:
      return "::__pb::StringView";
    default: {
      ABSL_LOG(FATAL) << "Unsupported field type: " << field_type;
    }
  }
}

void GenAccessorsForField(const std::string& upb_msg_prefix,
                          const std::string& msg_name,
                          const FieldDescriptor* field,
                          std::function<void()> upb_get_value,
                          std::function<void()> upb_set_value,
                          google::protobuf::io::Printer& p) {
  if (!IsSupported(field)) {
    return;
  }

  p.Emit({{"Msg", msg_name},
          {"field_name", field->name()},
          {"rust_type", RustTypeForField(field->type())},
          {"upb_type", UpbTypeForField(field->type())},
          {"has_thunk", UpbThunkName(field, upb_msg_prefix, "_has_")},
          {"getter_thunk", UpbThunkName(field, upb_msg_prefix, "_")},
          {"upb_get_value", upb_get_value},
          {"upb_set_value", upb_set_value},
          {"setter_thunk", UpbThunkName(field, upb_msg_prefix, "_set_")},
          {"clear_thunk", UpbThunkName(field, upb_msg_prefix, "_clear_")}},
         R"rs(
          impl $Msg$ {
            pub fn $field_name$(&self) -> Option<$rust_type$> {
              let field_present = unsafe { $has_thunk$(self.msg) };
              if !field_present {
                return None;
              }
              let value = $upb_get_value$ ;
              Some(value)
            }

            pub fn $field_name$_set(&mut self, value: Option<$rust_type$>) {
              match value {
                Some(value) => { $upb_set_value$ },
                None => unsafe { $clear_thunk$(self.msg); }
              }
            }
          }

          extern "C" {
            fn $getter_thunk$(msg: ::__std::ptr::NonNull<u8>) -> $upb_type$;
            fn $has_thunk$(msg: ::__std::ptr::NonNull<u8>) -> bool;
            fn $setter_thunk$(
              msg: ::__std::ptr::NonNull<u8>,
              value: $upb_type$
            );
            fn $clear_thunk$(msg: ::__std::ptr::NonNull<u8>);
          }
        )rs");
}

void GenBytesAccessors(const std::string& upb_msg_prefix,
                       const std::string& msg_name,
                       const FieldDescriptor* field, google::protobuf::io::Printer& p) {
  auto upb_get_value = [&] {
    p.Emit({{"getter_thunk", UpbThunkName(field, upb_msg_prefix, "_")}},
           R"rs(
             unsafe { 
                let upb_string_view = $getter_thunk$(self.msg);
                ::__std::slice::from_raw_parts(upb_string_view.data, upb_string_view.size)
              }
           )rs");
  };

  auto upb_set_value = [&] {
    p.Emit({{"setter_thunk", UpbThunkName(field, upb_msg_prefix, "_set_")}},
           R"rs(
            let upb_string_view = unsafe { ::__pb::StringView::new(value.as_ptr(), value.len()) };
            unsafe { $setter_thunk$(self.msg, upb_string_view); }
           )rs");
  };

  GenAccessorsForField(upb_msg_prefix, msg_name, field, upb_get_value,
                       upb_set_value, p);
}

void GenScalarAccessors(const std::string& upb_msg_prefix,
                        const std::string& msg_name,
                        const FieldDescriptor* field, google::protobuf::io::Printer& p) {
  auto upb_get_value = [&] {
    p.Emit({{"getter_thunk", UpbThunkName(field, upb_msg_prefix, "_")}},
           R"(
          unsafe { $getter_thunk$(self.msg) }
        )");
  };

  auto upb_set_value = [&] {
    p.Emit({{"setter_thunk", UpbThunkName(field, upb_msg_prefix, "_set_")}},
           R"(
            unsafe { $setter_thunk$(self.msg, value); }
           )");
  };

  GenAccessorsForField(upb_msg_prefix, msg_name, field, upb_get_value,
                       upb_set_value, p);
}

void GenAccessorsForMessage(const Descriptor* msg_descriptor,
                            google::protobuf::io::Printer& p) {
  auto upb_msg_prefix = UpbMsgPrefix(msg_descriptor);

  for (int i = 0; i < msg_descriptor->field_count(); ++i) {
    auto field = msg_descriptor->field(i);

    switch (field->type()) {
      case FieldDescriptor::Type::TYPE_INT64:
      case FieldDescriptor::Type::TYPE_BOOL:
        GenScalarAccessors(upb_msg_prefix, msg_descriptor->name(), field, p);
        break;
      case FieldDescriptor::Type::TYPE_BYTES:
        GenBytesAccessors(upb_msg_prefix, msg_descriptor->name(), field, p);
        break;
      default:
        // Not implemented type.
        break;
    }
  }
}

void GenMessageFunctions(const Descriptor* msg_descriptor,
                         google::protobuf::io::Printer& p) {
  p.Emit({{"Msg", msg_descriptor->name()},
          {"msg_prefix", UpbMsgPrefix(msg_descriptor)}},
         R"rs(
    impl $Msg$ {
      pub fn new() -> Self {
        let arena = unsafe { ::__pb::Arena::new() };
        let msg = unsafe { $msg_prefix$_new(arena) };
        $Msg$ { msg, arena }
      }

      pub fn serialize(&self) -> ::__pb::SerializedData {
        let arena = unsafe { ::__pb::__runtime::upb_Arena_New() };
        let mut len = 0;
        let chars = unsafe { $msg_prefix$_serialize(self.msg, arena, &mut len) };
        unsafe {::__pb::SerializedData::from_raw_parts(arena, chars, len)}
      }
    }

    extern "C" {
      fn $msg_prefix$_new(arena: *mut ::__pb::Arena) -> ::__std::ptr::NonNull<u8>;
      fn $msg_prefix$_serialize(
        msg: ::__std::ptr::NonNull<u8>,
        arena: *mut ::__pb::Arena,
        len: &mut usize) -> ::__std::ptr::NonNull<u8>;
    }
  )rs");
}

void UpbKernel::Generate(const FileDescriptor* file,
                         google::protobuf::io::Printer& p) const {
  for (int i = 0; i < file->message_type_count(); ++i) {
    auto msg_descriptor = file->message_type(i);

    p.Emit({{"Msg", msg_descriptor->name()},
            {
                "MsgFunctions",
                [&] { GenMessageFunctions(msg_descriptor, p); },
            },
            {
                "FieldAccessors",
                [&] { GenAccessorsForMessage(msg_descriptor, p); },
            }},
           R"rs(
      pub struct $Msg$ {
        msg: ::__std::ptr::NonNull<u8>,
        arena: *mut ::__pb::Arena,
      }

      $MsgFunctions$;
      $FieldAccessors$;
    )rs");
  }
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
