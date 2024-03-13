// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/oneof.h"

#include <string>

#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/rust/accessors/accessor_case.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/compiler/rust/rust_field_type.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

// The oneof getter accessor returns an enum which is a tagged union with the
// set field's value, or a sentinel not_set value if no field is set in the
// oneof.
//
// Example:
// For this oneof:
// message SomeMsg {
//   oneof some_oneof {
//     int32 field_a = 7;
//     SomeMsg field_b = 9;
//   }
// }
//
// This will emit as the exposed API:
// pub mod SomeMsg_ {
//   // The 'view' struct (no suffix on the name)
//   pub enum SomeOneof<'msg> {
//     FieldA(i32) = 7,
//     FieldB(View<'msg, SomeMsg>) = 9,
//     not_set(std::marker::PhantomData<&'msg ()>) = 0
//   }
// }
// impl SomeMsg {
//   pub fn some_oneof(&self) -> SomeOneof {...}
// }

namespace {
// A user-friendly rust type for a view of this field with lifetime 'msg.
std::string RsTypeNameView(Context& ctx, const FieldDescriptor& field) {
  if (field.options().has_ctype()) {
    return "";  // TODO: b/308792377 - ctype fields not supported yet.
  }
  switch (GetRustFieldType(field.type())) {
    case RustFieldType::INT32:
    case RustFieldType::INT64:
    case RustFieldType::UINT32:
    case RustFieldType::UINT64:
    case RustFieldType::FLOAT:
    case RustFieldType::DOUBLE:
    case RustFieldType::BOOL:
      return RsTypePath(ctx, field);
    case RustFieldType::BYTES:
      return "&'msg [u8]";
    case RustFieldType::STRING:
      return "&'msg ::__pb::ProtoStr";
    case RustFieldType::MESSAGE:
      return absl::StrCat("::__pb::View<'msg, ", RsTypePath(ctx, field), ">");
    case RustFieldType::ENUM:
      return absl::StrCat("::__pb::View<'msg, ", RsTypePath(ctx, field), ">");
  }

  ABSL_LOG(FATAL) << "Unexpected field type: " << field.type_name();
  return "";
}

}  // namespace

void GenerateOneofDefinition(Context& ctx, const OneofDescriptor& oneof) {
  ctx.Emit(
      {
          {"oneof_enum_name", OneofEnumRsName(oneof)},
          {"fields",
           [&] {
             for (int i = 0; i < oneof.field_count(); ++i) {
               auto& field = *oneof.field(i);
               std::string rs_type = RsTypeNameView(ctx, field);
               if (rs_type.empty()) {
                 continue;
               }
               ctx.Emit({{"name", OneofCaseRsName(field)},
                         {"type", rs_type},
                         {"number", std::to_string(field.number())}},
                        R"rs($name$($type$) = $number$,
                )rs");
             }
           }},
      },
      // TODO: Revisit if isize is the optimal repr for this enum.
      R"rs(
      #[non_exhaustive]
      #[derive(Debug, Clone, Copy)]
      #[allow(dead_code)]
      #[repr(isize)]
      pub enum $oneof_enum_name$<'msg> {
        $fields$

        #[allow(non_camel_case_types)]
        not_set(std::marker::PhantomData<&'msg ()>) = 0
      }
      )rs");
}

void GenerateOneofAccessors(Context& ctx, const OneofDescriptor& oneof,
                            AccessorCase accessor_case) {
  ctx.Emit({{"oneof_name", RsSafeName(oneof.name())},
            {"oneof_enum_name", OneofEnumRsName(oneof)},
            {"view_lifetime", ViewLifetime(accessor_case)},
            {"view_self", ViewReceiver(accessor_case)},
            {"cases",
             [&] {
               for (int i = 0; i < oneof.field_count(); ++i) {
                 auto& field = *oneof.field(i);
                 std::string rs_type = RsTypeNameView(ctx, field);
                 if (rs_type.empty()) {
                   continue;
                 }
                 ctx.Emit(
                     {
                         {"case", OneofCaseRsName(field)},
                         {"rs_getter", RsSafeName(field.name())},
                         {"int_value", std::to_string(field.number())},
                         {"type", rs_type},
                     },
                     R"rs(
                $int_value$ =>
                    $Msg$_::$oneof_enum_name$::$case$(self.$rs_getter$()),
                )rs");
               }
             }},
            {"case_thunk", ThunkName(ctx, oneof, "case")},
            {"getter",
             [&] {
               ctx.Emit({}, R"rs(
          pub fn $oneof_name$($view_self$) -> $Msg$_::$oneof_enum_name$<$view_lifetime$> {
            match unsafe { $case_thunk$(self.raw_msg()) } {
              $cases$
              0 => $Msg$_::$oneof_enum_name$::not_set(std::marker::PhantomData),
              _ => panic!("unexpected set oneof field, implies codegen skew!"),
            }
          }
          )rs");
             }}},
           R"rs(
        $getter$
      )rs");
}

void GenerateOneofExternC(Context& ctx, const OneofDescriptor& oneof) {
  ctx.Emit(
      {
          {"case_thunk", ThunkName(ctx, oneof, "case")},
      },
      R"rs(
        fn $case_thunk$(raw_msg: $pbi$::RawMessage) -> u32;
      )rs");
}

void GenerateOneofThunkCc(Context& ctx, const OneofDescriptor& oneof) {
  ctx.Emit(
      {
          {"oneof_name", oneof.name()},
          {"case_thunk", ThunkName(ctx, oneof, "case")},
          {"QualifiedMsg", cpp::QualifiedClassName(oneof.containing_type())},
      },
      R"cc(
        uint32_t $case_thunk$($QualifiedMsg$* msg) {
          return static_cast<uint32_t>(msg->$oneof_name$_case());
        }
      )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
