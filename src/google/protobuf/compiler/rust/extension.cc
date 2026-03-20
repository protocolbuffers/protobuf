// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/extension.h"

#include <string>

#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/rust/accessors/default_value.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/descriptor.h"
#include "upb/reflection/def.hpp"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

namespace {

std::string DefaultArgument(Context& ctx, const FieldDescriptor& extension) {
  if (extension.is_repeated()) {
    return ", ()";
  } else if (extension.message_type() != nullptr) {
    return ", ()";
  } else if (extension.type() == FieldDescriptor::TYPE_STRING ||
             extension.type() == FieldDescriptor::TYPE_BYTES) {
    // Coercion from byte array reference to slice is not yet stable within
    // const functions.
    return absl::StrCat(", ", DefaultValue(ctx, extension),
                        " as &'static [u8]");
  } else {
    return absl::StrCat(", ", DefaultValue(ctx, extension));
  }
}

}  // namespace

// Generates code for a particular extension in `.pb.rs`.
void GenerateRs(Context& ctx, const FieldDescriptor& extension,
                const upb::DefPool& pool) {
  // The extension symbol defined by both backends is the same.
  ctx.Emit({{"extendee", RsTypePath(ctx, *extension.containing_type())},
            {"extension", absl::AsciiStrToUpper(extension.name())},
            {"type", extension.is_repeated()
                         ? absl::StrCat("::protobuf::Repeated<",
                                        RsTypePath(ctx, extension), ">")
                         : RsTypePath(ctx, extension)}},
           R"rs(
        pub const $extension$: ::protobuf::ExtensionId<$extendee$, $type$> =
    )rs");

  // Definition differs.

  if (ctx.is_cpp()) {
    if (extension.is_repeated()) {
      ctx.Emit({{"default", DefaultArgument(ctx, extension)},
                {"number", absl::StrCat(extension.number())},
                {"thunk_name", ThunkName(ctx, extension, "ExtensionPtr")},
                {"type_number", absl::StrCat(extension.type())},
                {"get_thunk", ThunkName(ctx, extension, "get")},
                {"mut_thunk", ThunkName(ctx, extension, "get_mut")}},
               R"rs(
        $pbi$::new_extension_id(
            $number$$default$, $pbr$::InnerExtensionId::new_repeated($type_number$, $thunk_name$, $get_thunk$, $mut_thunk$)
        );

    unsafe extern "C" {
      fn $thunk_name$() -> *const ::core::ffi::c_void;
      fn $get_thunk$(msg: *const ::core::ffi::c_void) -> *const ::core::ffi::c_void;
      fn $mut_thunk$(msg: *mut ::core::ffi::c_void) -> *mut ::core::ffi::c_void;
    }
    )rs");
    } else {
      ctx.Emit({{"default", DefaultArgument(ctx, extension)},
                {"number", absl::StrCat(extension.number())},
                {"thunk_name", ThunkName(ctx, extension, "ExtensionPtr")},
                {"type_number", absl::StrCat(extension.type())}},
               R"rs(
        $pbi$::new_extension_id(
            $number$$default$, $pbr$::InnerExtensionId::new($type_number$, $thunk_name$)
        );

    unsafe extern "C" {
      fn $thunk_name$() -> *const ::core::ffi::c_void;
    }
    )rs");
    }
  } else {
    std::string mini_descriptor =
        pool.FindExtensionByName(std::string(extension.full_name()).c_str())
            .MiniDescriptorEncode();
    ctx.Emit({{"default", DefaultArgument(ctx, extension)},
              {"extendee", RsTypePath(ctx, *extension.containing_type())},
              {"number", absl::StrCat(extension.number())},
              {"mini_descriptor", mini_descriptor}},
             R"rs(
    {
        #[linkme::distributed_slice($pbr$::EXTENSIONS)]
        static MT: ::std::sync::LazyLock<$pbr$::MiniTableExtensionInitPtr> = ::std::sync::LazyLock::new(|| unsafe {
            $pbr$::MiniTableExtensionInitPtr($pbr$::build_extension_mini_table(
                "$mini_descriptor$",
                <$extendee$ as $pbr$::AssociatedMiniTable>::mini_table(),
            ))
        });
        $pbi$::new_extension_id($number$$default$, $pbr$::InnerExtensionId::new(&MT))
    };
  )rs");
  }
}

// Generates code for a particular message in `.pb.thunk.cc`.
void GenerateThunksCc(Context& ctx, const FieldDescriptor& extension) {
  ctx.Emit({{"thunk_name", ThunkName(ctx, extension, "ExtensionPtr")},
            {"extension_name", cpp::QualifiedExtensionName(&extension)}},
           R"cc(
             extern "C" {
             void* $thunk_name$() { return &$extension_name$; }
             }
           )cc");
  if (extension.is_repeated()) {
    ctx.Emit(
        {{"get_thunk", ThunkName(ctx, extension, "get")},
         {"mut_thunk", ThunkName(ctx, extension, "get_mut")},
         {"extension_name", cpp::QualifiedExtensionName(&extension)},
         {"extendee", cpp::QualifiedClassName(extension.containing_type())}},
        R"cc(
          extern "C" {
          const void* $get_thunk$(const $extendee$* msg) {
            return &msg->GetRepeatedExtension($extension_name$);
          }
          void* $mut_thunk$($extendee$* msg) {
            return msg->MutableRepeatedExtension($extension_name$);
          }
          }
        )cc");
  }
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
