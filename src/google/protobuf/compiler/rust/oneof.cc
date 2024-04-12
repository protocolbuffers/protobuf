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
//   oneof some_oneof {
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
//   pub enum SomeOneofCase {
//     FieldA = 7,
//     FieldB = 9,
//     not_set = 0
//   }
// }
// impl SomeMsg {
//   pub fn some_oneof(&self) -> SomeOneof {...}
//   pub fn some_oneof_case(&self) -> SomeOneofCase {...}
// }
// impl SomeMsgMut {
//   pub fn some_oneof(&self) -> SomeOneof {...}
//   pub fn some_oneof_case(&self) -> SomeOneofCase {...}
// }
// impl SomeMsgView {
//   pub fn some_oneof(self) -> SomeOneof {...}
//   pub fn some_oneof_case(self) -> SomeOneofCase {...}
// }
//
// An additional "Case" enum which just reflects the corresponding slot numbers
// is emitted for usage with the FFI (exactly matching the Case struct that both
// cpp and upb generate).
//
// #[repr(C)] pub(super) enum SomeOneofCase {
//   FieldA = 7,
//   FieldB = 9,
//   not_set = 0
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
          {"view_enum_name", OneofViewEnumRsName(oneof)},
          {"view_fields",
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
      // TODO: not_set currently has phantom data just to avoid the
      // lifetime on the enum breaking compilation if there are zero supported
      // fields on it (e.g. if the oneof only has Messages inside).
      R"rs(
      #[non_exhaustive]
      #[derive(Debug, Clone, Copy)]
      #[allow(dead_code)]
      #[repr(isize)]
      pub enum $view_enum_name$<'msg> {
        $view_fields$

        #[allow(non_camel_case_types)]
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
                 ctx.Emit({{"name", OneofCaseRsName(field)},
                           {"number", std::to_string(field.number())}},
                          R"rs($name$ = $number$,
                )rs");
               }
             }}},
           R"rs(
      #[repr(C)]
      #[derive(Debug, Copy, Clone, PartialEq, Eq)]
      #[allow(dead_code)]
      pub enum $case_enum_name$ {
        $cases$

        #[allow(non_camel_case_types)]
        not_set = 0
      }

      )rs");
}

void GenerateOneofAccessors(Context& ctx, const OneofDescriptor& oneof,
                            AccessorCase accessor_case) {
  ctx.Emit(
      {
          {"oneof_name", RsSafeName(oneof.name())},
          {"view_lifetime", ViewLifetime(accessor_case)},
          {"self", ViewReceiver(accessor_case)},
          {"oneof_enum_module",
           absl::StrCat("crate::", RustModuleForContainingType(
                                       ctx, oneof.containing_type()))},
          {"view_enum_name", OneofViewEnumRsName(oneof)},
          {"case_enum_name", OneofCaseEnumRsName(oneof)},
          {"view_cases",
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
                       {"type", rs_type},
                   },
                   R"rs(
                $oneof_enum_module$$case_enum_name$::$case$ =>
                    $oneof_enum_module$$view_enum_name$::$case$(self.$rs_getter$()),
                )rs");
             }
           }},
          {"case_thunk", ThunkName(ctx, oneof, "case")},
      },
      R"rs(
        pub fn $oneof_name$($self$) -> $oneof_enum_module$$view_enum_name$<$view_lifetime$> {
          match $self$.$oneof_name$_case() {
            $view_cases$
            _ => $oneof_enum_module$$view_enum_name$::not_set(std::marker::PhantomData)
          }
        }

        pub fn $oneof_name$_case($self$) -> $oneof_enum_module$$case_enum_name$ {
          unsafe { $case_thunk$(self.raw_msg()) }
        }
      )rs");
}

void GenerateOneofExternC(Context& ctx, const OneofDescriptor& oneof) {
  ctx.Emit(
      {
          {"oneof_enum_module",
           absl::StrCat("crate::", RustModuleForContainingType(
                                       ctx, oneof.containing_type()))},
          {"case_enum_rs_name", OneofCaseEnumRsName(oneof)},
          {"case_thunk", ThunkName(ctx, oneof, "case")},
      },
      R"rs(
        fn $case_thunk$(raw_msg: $pbr$::RawMessage) -> $oneof_enum_module$$case_enum_rs_name$;
      )rs");
}

void GenerateOneofThunkCc(Context& ctx, const OneofDescriptor& oneof) {
  ctx.Emit(
      {
          {"oneof_name", oneof.name()},
          {"case_enum_name", OneofCaseEnumRsName(oneof)},
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
