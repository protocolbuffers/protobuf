// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/oneof.h"

#include <string>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/rust/accessors/accessor_case.h"
#include "google/protobuf/compiler/rust/accessors/accessors.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/compiler/rust/rust_field_type.h"
#include "google/protobuf/compiler/rust/upb_helpers.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

// For each oneof we emit two Rust enums with corresponding accessors:
// -  An enum acting as a tagged union that has each case holds a View<> of
//    each of the cases. Named as the one_of name in CamelCase.
// -  A simple 'which oneof field is set' enum which directly maps to the
//    underlying enum used for the 'cases' accessor in C++ or upb. Named as the
//    one_of camelcase with "Case" appended.
//
// Example:
// For this oneof:
// message SomeMsg {
//   oneof some {
//     int32 field_a = 7;
//     SomeMsg field_b = 9;
//   }
// }
//
// This will emit as the exposed API:
// pub mod some_msg {
//   pub enum SomeOneof<'msg> {
//     FieldA(i32) = 7,
//     FieldB(View<'msg, SomeMsg>) = 9,
//     not_set(std::marker::PhantomData<&'msg ()>) = 0
//   }
//
//   #[repr(C)]
//   pub enum SomeCase {
//     FieldA = 7,
//     FieldB = 9,
//     not_set = 0
//   }
// }
// impl SomeMsg {
//   pub fn some_oneof(&self) -> SomeOneof {...}
//   pub fn some_oneof_case(&self) -> SomeCase {...}
// }
// impl SomeMsgMut {
//   pub fn some_oneof(&self) -> SomeOneof {...}
//   pub fn some_oneof_case(&self) -> SomeCase {...}
// }
// impl SomeMsgView {
//   pub fn some_oneof(self) -> SomeOneof {...}
//   pub fn some_oneof_case(self) -> SomeCase {...}
// }

namespace {

bool IsSupportedOneofFieldCase(Context& ctx, const FieldDescriptor& field) {
  if (!IsSupportedField(ctx, field)) {
    return false;
  }

  // In addition to any fields that are otherwise unsupported, if the
  // oneof contains a string or bytes field which is not string_view or string
  // representation (namely, Cord or StringPiece), we don't support it
  // currently.
  if (ctx.is_cpp() && field.cpp_type() == FieldDescriptor::CPPTYPE_STRING &&
      field.cpp_string_type() != FieldDescriptor::CppStringType::kString &&
      field.cpp_string_type() != FieldDescriptor::CppStringType::kView) {
    return false;
  }
  return true;
}

// A user-friendly rust type for a view of this field with lifetime 'msg.
std::string RsTypeNameView(Context& ctx, const FieldDescriptor& field) {
  ABSL_CHECK(IsSupportedOneofFieldCase(ctx, field));

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
      return "&'msg ::protobuf::ProtoStr";
    case RustFieldType::MESSAGE:
      return absl::StrCat("::protobuf::View<'msg, ", RsTypePath(ctx, field),
                          ">");
    case RustFieldType::ENUM:
      return absl::StrCat("::protobuf::View<'msg, ", RsTypePath(ctx, field),
                          ">");
  }

  ABSL_LOG(FATAL) << "Unexpected field type: " << field.type_name();
  return "";
}

}  // namespace

void GenerateOneofDefinition(Context& ctx, const OneofDescriptor& oneof) {
  ctx.Emit(
      {
          {"view_enum_name", OneofViewEnumRsName(oneof)},
          {"view_fields",
           [&] {
             for (int i = 0; i < oneof.field_count(); ++i) {
               auto& field = *oneof.field(i);
               if (!IsSupportedOneofFieldCase(ctx, field)) {
                 continue;
               }
               std::string rs_type = RsTypeNameView(ctx, field);
               ctx.Emit({{"name", OneofCaseRsName(field)},
                         {"type", rs_type},
                         {"number", std::to_string(field.number())}},
                        R"rs($name$($type$) = $number$,
                )rs");
             }
           }},
      },
      // Note: This enum deliberately has a 'msg lifetime associated with it
      // even if all fields were scalars; we could conditionally exclude the
      // lifetime under that case, but it would mean changing the .proto file
      // to add an additional string or message-typed field to the oneof would
      // be a more breaking change than it needs to be.
      R"rs(
      #[non_exhaustive]
      #[derive(Debug, Clone, Copy)]
      #[allow(dead_code)]
      #[repr(u32)]
      pub enum $view_enum_name$<'msg> {
        $view_fields$

        not_set(std::marker::PhantomData<&'msg ()>) = 0
      }
      )rs");

  // Note: This enum is used as the Thunk return type for getting which case is
  // used: it exactly matches the generate case enum that both cpp and upb use.
  ctx.Emit({{"case_enum_name", OneofCaseEnumRsName(oneof)},
            {"cases",
             [&] {
               for (int i = 0; i < oneof.field_count(); ++i) {
                 auto& field = *oneof.field(i);
                 if (!IsSupportedOneofFieldCase(ctx, field)) {
                   continue;
                 }
                 ctx.Emit({{"name", OneofCaseRsName(field)},
                           {"number", std::to_string(field.number())}},
                          R"rs($name$ = $number$,
                          )rs");
               }
             }},
            {"try_from_cases",
             [&] {
               for (int i = 0; i < oneof.field_count(); ++i) {
                 auto& field = *oneof.field(i);
                 if (!IsSupportedOneofFieldCase(ctx, field)) {
                   continue;
                 }
                 ctx.Emit({{"name", OneofCaseRsName(field)},
                           {"number", std::to_string(field.number())}},
                          R"rs($number$ => Some($case_enum_name$::$name$),
                          )rs");
               }
             }}},
           R"rs(
      #[repr(C)]
      #[derive(Debug, Copy, Clone, PartialEq, Eq)]
      #[non_exhaustive]
      #[allow(dead_code)]
      pub enum $case_enum_name$ {
        $cases$

        not_set = 0
      }

      impl $case_enum_name$ {
        //~ This try_from is not a TryFrom impl so that it isn't
        //~ committed to as part of our public api.
        #[allow(dead_code)]
        pub(crate) fn try_from(v: u32) -> $Option$<$case_enum_name$> {
          match v {
            0 => Some($case_enum_name$::not_set),
            $try_from_cases$
            _ => None
          }
        }
      }

      )rs");
}

void GenerateOneofAccessors(Context& ctx, const OneofDescriptor& oneof,
                            AccessorCase accessor_case) {
  ctx.Emit(
      {{"oneof_name", RsSafeName(oneof.name())},
       {"view_lifetime", ViewLifetime(accessor_case)},
       {"self", ViewReceiver(accessor_case)},
       {"oneof_enum_module", RustModule(ctx, oneof)},
       {"view_enum_name", OneofViewEnumRsName(oneof)},
       {"case_enum_name", OneofCaseEnumRsName(oneof)},
       {"view_cases",
        [&] {
          for (int i = 0; i < oneof.field_count(); ++i) {
            auto& field = *oneof.field(i);
            if (!IsSupportedOneofFieldCase(ctx, field)) {
              continue;
            }
            std::string rs_type = RsTypeNameView(ctx, field);
            std::string field_name = FieldNameWithCollisionAvoidance(field);
            ctx.Emit(
                {
                    {"case", OneofCaseRsName(field)},
                    {"rs_getter", RsSafeName(field_name)},
                    {"type", rs_type},
                },
                R"rs(
                $oneof_enum_module$$case_enum_name$::$case$ =>
                    $oneof_enum_module$$view_enum_name$::$case$(self.$rs_getter$()),
                )rs");
          }
        }},
       {"oneof_case_body",
        [&] {
          if (ctx.is_cpp()) {
            ctx.Emit({{"case_thunk", ThunkName(ctx, oneof, "case")}},
                     "unsafe { $case_thunk$(self.raw_msg()) }");
          } else {
            ctx.Emit(
                // The field index for an arbitrary field that in the oneof.
                {{"upb_mt_field_index",
                  UpbMiniTableFieldIndex(*oneof.field(0))}},
                R"rs(
                let field_num = unsafe {
                  let f = $pbr$::upb_MiniTable_GetFieldByIndex(
                      <Self as $pbr$::AssociatedMiniTable>::mini_table(),
                      $upb_mt_field_index$);
                  $pbr$::upb_Message_WhichOneofFieldNumber(
                        self.raw_msg(), f)
                };
                unsafe {
                  $oneof_enum_module$$case_enum_name$::try_from(field_num).unwrap_unchecked()
                }
              )rs");
          }
        }}},
      R"rs(
        pub fn $oneof_name$($self$) -> $oneof_enum_module$$view_enum_name$<$view_lifetime$> {
          match $self$.$oneof_name$_case() {
            $view_cases$
            _ => $oneof_enum_module$$view_enum_name$::not_set(std::marker::PhantomData)
          }
        }

        pub fn $oneof_name$_case($self$) -> $oneof_enum_module$$case_enum_name$ {
          $oneof_case_body$
        }
      )rs");
}

void GenerateOneofExternC(Context& ctx, const OneofDescriptor& oneof) {
  ABSL_CHECK(ctx.is_cpp());

  ctx.Emit(
      {
          {"oneof_enum_module", RustModule(ctx, oneof)},
          {"case_enum_rs_name", OneofCaseEnumRsName(oneof)},
          {"case_thunk", ThunkName(ctx, oneof, "case")},
      },
      R"rs(
        fn $case_thunk$(raw_msg: $pbr$::RawMessage) -> $oneof_enum_module$$case_enum_rs_name$;
      )rs");
}

void GenerateOneofThunkCc(Context& ctx, const OneofDescriptor& oneof) {
  ABSL_CHECK(ctx.is_cpp());

  ctx.Emit(
      {
          {"oneof_name", oneof.name()},
          {"case_enum_name", OneofCaseEnumCppName(oneof)},
          {"case_thunk", ThunkName(ctx, oneof, "case")},
          {"QualifiedMsg", cpp::QualifiedClassName(oneof.containing_type())},
      },
      R"cc(
        $QualifiedMsg$::$case_enum_name$ $case_thunk$($QualifiedMsg$* msg) {
          return msg->$oneof_name$_case();
        }
      )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
