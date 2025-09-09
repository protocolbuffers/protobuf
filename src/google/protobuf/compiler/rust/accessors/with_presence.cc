// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/accessors/with_presence.h"

#include <string>

#include "absl/log/absl_check.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/rust/accessors/accessor_case.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/compiler/rust/upb_helpers.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

void WithPresenceAccessorsInMsgImpl(Context& ctx, const FieldDescriptor& field,
                                    AccessorCase accessor_case) {
  ABSL_CHECK(field.has_presence());

  std::string field_name = FieldNameWithCollisionAvoidance(field);

  ctx.Emit(
      {{"field", RsSafeName(field_name)},
       {"raw_field_name", field_name},  // Never r# prefixed
       {"view_type", RsViewType(ctx, field, ViewLifetime(accessor_case))},
       {"view_self", ViewReceiver(accessor_case)},
       {"hazzer",
        [&] {
          if (ctx.is_cpp()) {
            ctx.Emit({{"hazzer_thunk", ThunkName(ctx, field, "has")}},
                     R"rs(
                  pub fn has_$raw_field_name$($view_self$) -> bool {
                    unsafe {
                      $hazzer_thunk$(self.raw_msg())
                    }
                  }
                  )rs");
          } else {
            ctx.Emit({{"upb_mt_field_index", UpbMiniTableFieldIndex(field)}},
                     R"rs(
                  pub fn has_$raw_field_name$($view_self$) -> bool {
                    unsafe {
                      let f = $pbr$::upb_MiniTable_GetFieldByIndex(
                          <Self as $pbr$::AssociatedMiniTable>::mini_table(),
                          $upb_mt_field_index$);
                      $pbr$::upb_Message_HasBaseField(self.raw_msg(), f)
                    }
                  }
                  )rs");
          }
        }},
       {"clearer",
        [&] {
          if (accessor_case == AccessorCase::VIEW) return;
          if (ctx.is_cpp()) {
            ctx.Emit({{"clearer_thunk", ThunkName(ctx, field, "clear")}},
                     R"rs(
                    pub fn clear_$raw_field_name$(&mut self) {
                      unsafe { $clearer_thunk$(self.raw_msg()) }
                    })rs");
          } else {
            ctx.Emit({{"upb_mt_field_index", UpbMiniTableFieldIndex(field)}},
                     R"rs(
                    pub fn clear_$raw_field_name$(&mut self) {
                      unsafe {
                        let mt = <Self as $pbr$::AssociatedMiniTable>::mini_table();
                        let f = $pbr$::upb_MiniTable_GetFieldByIndex(
                            mt, $upb_mt_field_index$);
                        $pbr$::upb_Message_ClearBaseField(self.raw_msg(), f);
                      }
                    })rs");
          }
        }},
       {"opt_getter",
        [&] {
          // Cord fields don't support the _opt getter.
          if (ctx.is_cpp() &&
              field.cpp_type() == FieldDescriptor::CPPTYPE_STRING &&
              field.cpp_string_type() ==
                  FieldDescriptor::CppStringType::kCord) {
            return;
          }
          ctx.Emit(
              R"rs(
              pub fn $raw_field_name$_opt($view_self$) -> $pb$::Optional<$view_type$> {
                    $pb$::Optional::new(self.$field$(), self.has_$raw_field_name$())
              }
              )rs");
        }}},
      R"rs(
    $hazzer$
    $clearer$
    $opt_getter$
    )rs");
}

void WithPresenceAccessorsInExternC(Context& ctx,
                                    const FieldDescriptor& field) {
  ABSL_CHECK(ctx.is_cpp());
  ABSL_CHECK(field.has_presence());

  ctx.Emit(
      {
          {"hazzer_thunk", ThunkName(ctx, field, "has")},
          {"clearer_thunk", ThunkName(ctx, field, "clear")},
      },
      R"rs(
    fn $hazzer_thunk$(raw_msg: $pbr$::RawMessage) -> bool;
    fn $clearer_thunk$(raw_msg: $pbr$::RawMessage);
    )rs");
}

void WithPresenceAccessorsInThunkCc(Context& ctx,
                                    const FieldDescriptor& field) {
  ABSL_CHECK(ctx.is_cpp());
  ABSL_CHECK(field.has_presence());

  ctx.Emit(
      {
          {"field", cpp::FieldName(&field)},
          {"QualifiedMsg", cpp::QualifiedClassName(field.containing_type())},
          {"hazzer_thunk", ThunkName(ctx, field, "has")},
          {"clearer_thunk", ThunkName(ctx, field, "clear")},
      },
      R"rs(
    bool $hazzer_thunk$($QualifiedMsg$* msg) {
      return msg->has_$field$();
    }
    void $clearer_thunk$($QualifiedMsg$* msg) { msg->clear_$field$(); }
    )rs");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
