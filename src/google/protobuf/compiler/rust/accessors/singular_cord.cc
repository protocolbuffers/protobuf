// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/rust/accessors/accessor_case.h"
#include "google/protobuf/compiler/rust/accessors/default_value.h"
#include "google/protobuf/compiler/rust/accessors/generator.h"
#include "google/protobuf/compiler/rust/accessors/with_presence.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/compiler/rust/upb_helpers.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

namespace {

using Sub = ::google::protobuf::io::Printer::Sub;

std::string CordFieldNameWithCollisionAvoidance(const FieldDescriptor& field,
                                                absl::string_view field_name) {
  const Descriptor& msg = *field.containing_type();
  if (msg.FindFieldByName(absl::StrCat(field_name, "_cord")) != nullptr ||
      msg.FindFieldByName(absl::StrCat(field_name, "_cord_opt")) != nullptr ||
      msg.FindFieldByName(absl::StrCat("set_", field_name, "_cord")) !=
          nullptr) {
    return absl::StrCat(field_name, "_", field.number());
  }
  return std::string(field_name);
}

}  // namespace

void SingularCord::InMsgImpl(Context& ctx, const FieldDescriptor& field,
                             AccessorCase accessor_case) const {
  if (field.has_presence()) {
    WithPresenceAccessorsInMsgImpl(ctx, field, accessor_case);
  }

  std::string field_name = FieldNameWithCollisionAvoidance(field);
  std::string cord_field_name =
      CordFieldNameWithCollisionAvoidance(field, field_name);
  bool is_string_type = field.type() == FieldDescriptor::TYPE_STRING;

  if (ctx.is_upb()) {
    ctx.Emit(
        {
            {"default_value", DefaultValue(ctx, field)},
            {"upb_mt_field_index", UpbMiniTableFieldIndex(field)},
            {"proxied_type", RsTypePath(ctx, field)},
            {"raw_field_name", field_name},
            {"cord_field_name", cord_field_name},
            Sub("transform_view",
                [&] {
                  if (is_string_type) {
                    ctx.Emit(R"rs(
              $pb$::ProtoStr::from_utf8_unchecked(unsafe { str_view.as_ref() })
            )rs");
                  } else {
                    ctx.Emit("unsafe { str_view.as_ref() }");
                  }
                })
                .WithSuffix(""),
            {"view_lifetime", ViewLifetime(accessor_case)},
            {"view_self", ViewReceiver(accessor_case)},
            {"getter",
             [&] {
               ctx.Emit(
                   {Sub("getter_fn_name", RsSafeName(field_name))
                        .AnnotatedAs(&field)},
                   R"rs(
                  pub fn $getter_fn_name$($view_self$) -> $pb$::View<$view_lifetime$, $proxied_type$> {
                    let str_view = unsafe {
                      self.inner.ptr().get_string_at_index(
                        $upb_mt_field_index$, ($default_value$).into()
                      )
                    };
                    $transform_view$
                  })rs");
             }},
            {"setter",
             [&] {
               if (accessor_case == AccessorCase::VIEW) return;
               ctx.Emit(
                   {{"string_type", is_string_type ? "string" : "bytes"},
                    Sub("setter_fn_name", absl::StrCat("set_", field_name))
                        .AnnotatedAs(
                            {&field, io::AnnotationCollector::Semantic::kSet})},
                   R"rs(
              pub fn $setter_fn_name$(&mut self, val: impl $pb$::IntoProxied<$proxied_type$>) {
                unsafe {
                  $pbr$::message_set_$string_type$_field(
                    $pb$::AsMut::as_mut(self).inner,
                    $upb_mt_field_index$,
                    val);
                }
              }
            )rs");
             }},
            {"cord_getter",
             [&] {
               ctx.Emit(R"rs(
                pub fn $cord_field_name$_cord($view_self$) -> $pb$::ProtoCord<$view_lifetime$> {
                  let str_view = unsafe {
                    self.inner.ptr().get_string_at_index(
                      $upb_mt_field_index$, ($default_value$).into()
                    )
                  };
                  $pb$::ProtoCord::from_slice(unsafe { str_view.as_ref() })
                }
              )rs");
             }},
            {"cord_opt_getter",
             [&] {
               if (!field.has_presence()) return;
               ctx.Emit(R"rs(
                pub fn $cord_field_name$_cord_opt($view_self$) -> $std$::option::Option<$pb$::ProtoCord<$view_lifetime$>> {
                  self.has_$raw_field_name$().then(|| self.$cord_field_name$_cord())
                }
              )rs");
             }},
            {"cord_setter",
             [&] {
               if (accessor_case == AccessorCase::VIEW) return;
               ctx.Emit(R"rs(
                pub fn set_$cord_field_name$_cord(&mut self, val: impl $pb$::IntoProtoCord) {
                  unsafe {
                    $pbr$::message_set_cord_field(
                      $pb$::AsMut::as_mut(self).inner,
                      $upb_mt_field_index$,
                      val);
                  }
                }
              )rs");
             }},
        },
        R"rs(
          $getter$
          $setter$
          $cord_getter$
          $cord_opt_getter$
          $cord_setter$
        )rs");
    return;
  }

  ctx.Emit({{"field", RsSafeName(field_name)},
            {"raw_field_name", field_name},
            {"cord_field_name", cord_field_name},
            {"proxied_type", RsTypePath(ctx, field)},
            {"transform_borrowed",
             [&] {
               if (is_string_type) {
                 ctx.Emit(R"rs(
                $pb$::ProtoStringCow::Borrowed(
                  $pb$::ProtoStr::from_utf8_unchecked(unsafe { view.as_ref() })
                )
              )rs");
               } else {
                 ctx.Emit(R"rs(
                $pb$::ProtoBytesCow::Borrowed(
                  unsafe { view.as_ref() }
                )
               )rs");
               }
             }},
            {"transform_owned",
             [&] {
               if (is_string_type) {
                 ctx.Emit(R"rs(
                $pb$::ProtoStringCow::Owned(
                  $pb$::ProtoString::from_inner($pbi$::Private, inner)
                )
              )rs");
               } else {
                 ctx.Emit(R"rs(
                $pb$::ProtoBytesCow::Owned(
                  $pb$::ProtoBytes::from_inner($pbi$::Private, inner)
                )
              )rs");
               }
             }},
            {"view_lifetime", ViewLifetime(accessor_case)},
            {"view_type",
             [&] {
               if (is_string_type) {
                 ctx.Emit("$pb$::ProtoStringCow<$view_lifetime$>");
               } else {
                 ctx.Emit("$pb$::ProtoBytesCow<$view_lifetime$>");
               }
             }},
            {"view_self", ViewReceiver(accessor_case)},
            {"getter_impl",
             [&] {
               ctx.Emit(
                   {{"is_flat_thunk", ThunkName(ctx, field, "cord_is_flat")},
                    {"borrowed_getter_thunk",
                     ThunkName(ctx, field, "get_cord_borrowed")},
                    {"owned_getter_thunk",
                     ThunkName(ctx, field, "get_cord_owned")}},
                   R"rs(
                  let cord_is_flat = unsafe { $is_flat_thunk$(self.raw_msg()) };
                  if cord_is_flat {
                    let view = unsafe { $borrowed_getter_thunk$(self.raw_msg()) };
                    return $transform_borrowed$;
                  }

                  let owned = unsafe { $owned_getter_thunk$(self.raw_msg()) };
                  let inner = unsafe { $pbr$::InnerProtoString::from_raw(owned) };

                  $transform_owned$
                )rs");
             }},
            {"getter",
             [&] {
               ctx.Emit(R"rs(
                pub fn $field$($view_self$) -> $view_type$ {
                  $getter_impl$
                }
            )rs");
             }},
            {"setter_impl",
             [&] {
               ctx.Emit({{"setter_thunk", ThunkName(ctx, field, "set")}},
                        R"rs(
              let s = val.into_proxied($pbi$::Private);
              unsafe {
                $setter_thunk$(
                  self.inner.raw(),
                  s.into_inner($pbi$::Private).into_raw()
                );
              }
            )rs");
             }},
            {"setter",
             [&] {
               if (accessor_case == AccessorCase::VIEW) return;
               ctx.Emit({},
                        R"rs(
              pub fn set_$raw_field_name$(&mut self, val: impl $pb$::IntoProxied<$proxied_type$>) {
                $setter_impl$
              }
            )rs");
             }},
            {"cord_getter",
             [&] {
               ctx.Emit({{"raw_cord_getter_thunk",
                          ThunkName(ctx, field, "get_cord_raw")}},
                        R"rs(
              pub fn $cord_field_name$_cord($view_self$) -> $pb$::ProtoCord<$view_lifetime$> {
                let raw = unsafe { $raw_cord_getter_thunk$(self.raw_msg()) };
                $pb$::ProtoCord::from_inner(
                  $pbi$::Private,
                  unsafe { $pbr$::InnerProtoCord::from_raw(raw) },
                )
              }
            )rs");
             }},
            {"cord_opt_getter",
             [&] {
               if (!field.has_presence()) return;
               ctx.Emit(R"rs(
              pub fn $cord_field_name$_cord_opt($view_self$) -> $std$::option::Option<$pb$::ProtoCord<$view_lifetime$>> {
                self.has_$raw_field_name$().then(|| self.$cord_field_name$_cord())
              }
            )rs");
             }},
            {"cord_setter",
             [&] {
               if (accessor_case == AccessorCase::VIEW) return;
               ctx.Emit({{"raw_cord_setter_thunk",
                          ThunkName(ctx, field, "set_cord_raw")}},
                        R"rs(
              pub fn set_$cord_field_name$_cord(&mut self, val: impl $pb$::IntoProtoCord) {
                unsafe {
                  $pbr$::with_raw_cord_input(val, |raw_input| {
                    $raw_cord_setter_thunk$(self.inner.raw(), raw_input);
                  });
                }
              }
            )rs");
             }}},
           R"rs(
        $getter$
        $setter$
        $cord_getter$
        $cord_opt_getter$
        $cord_setter$
      )rs");
}

void SingularCord::InExternC(Context& ctx, const FieldDescriptor& field) const {
  ABSL_CHECK(ctx.is_cpp());

  if (field.has_presence()) {
    WithPresenceAccessorsInExternC(ctx, field);
  }

  ctx.Emit(
      {{"borrowed_getter_thunk", ThunkName(ctx, field, "get_cord_borrowed")},
       {"owned_getter_thunk", ThunkName(ctx, field, "get_cord_owned")},
       {"is_flat_thunk", ThunkName(ctx, field, "cord_is_flat")},
       {"setter_thunk", ThunkName(ctx, field, "set")},
       {"raw_cord_getter_thunk", ThunkName(ctx, field, "get_cord_raw")},
       {"raw_cord_setter_thunk", ThunkName(ctx, field, "set_cord_raw")},
       {"setter",
        [&] {
          ctx.Emit(R"rs(
              fn $setter_thunk$(raw_msg: $pbr$::RawMessage, val: $pbr$::CppStdString);
              fn $raw_cord_setter_thunk$(raw_msg: $pbr$::RawMessage, val: $pbr$::RawCordInput);
            )rs");
        }},
       {"getter_thunks",
        [&] {
          ctx.Emit(R"rs(
             fn $is_flat_thunk$(raw_msg: $pbr$::RawMessage) -> bool;
             fn $borrowed_getter_thunk$(raw_msg: $pbr$::RawMessage) -> $pbr$::PtrAndLen;
             fn $owned_getter_thunk$(raw_msg: $pbr$::RawMessage) -> $pbr$::CppStdString;
             fn $raw_cord_getter_thunk$(raw_msg: $pbr$::RawMessage) -> $pbr$::RawCord;
           )rs");
        }}},
      R"rs(
          $getter_thunks$
          $setter$
        )rs");
}

void SingularCord::InThunkCc(Context& ctx, const FieldDescriptor& field) const {
  ABSL_CHECK(ctx.is_cpp());

  if (field.has_presence()) {
    WithPresenceAccessorsInThunkCc(ctx, field);
  }

  ctx.Emit(
      {{"field", cpp::FieldName(&field)},
       {"QualifiedMsg", cpp::QualifiedClassName(field.containing_type())},
       {"setter_thunk", ThunkName(ctx, field, "set")},
       {"borrowed_getter_thunk", ThunkName(ctx, field, "get_cord_borrowed")},
       {"owned_getter_thunk", ThunkName(ctx, field, "get_cord_owned")},
       {"is_flat_thunk", ThunkName(ctx, field, "cord_is_flat")},
       {"raw_cord_getter_thunk", ThunkName(ctx, field, "get_cord_raw")},
       {"raw_cord_setter_thunk", ThunkName(ctx, field, "set_cord_raw")}},
      R"cc(
        bool $is_flat_thunk$($QualifiedMsg$* msg) {
          const absl::Cord& cord = msg->$field$();
          return cord.TryFlat().has_value();
        }
        ::google::protobuf::rust::PtrAndLen $borrowed_getter_thunk$($QualifiedMsg$* msg) {
          const absl::Cord& cord = msg->$field$();
          absl::string_view s = cord.TryFlat().value();
          return ::google::protobuf::rust::PtrAndLen{s.data(), s.size()};
        }
        std::string* $owned_getter_thunk$($QualifiedMsg$* msg) {
          const absl::Cord& cord = msg->$field$();
          std::string* owned = new std::string();
          absl::CopyCordToString(cord, owned);
          return owned;
        }
        void $setter_thunk$($QualifiedMsg$* msg, std::string* s) {
          msg->set_$field$(absl::Cord(std::move(*s)));
          delete s;
        }
        const absl::Cord* $raw_cord_getter_thunk$(const $QualifiedMsg$* msg) {
          return &msg->$field$();
        }
        void $raw_cord_setter_thunk$($QualifiedMsg$* msg,
                                     ::google::protobuf::rust::RawCordInput input) {
          msg->set_$field$(::google::protobuf::rust::TakeCordInput(input));
        }
      )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
