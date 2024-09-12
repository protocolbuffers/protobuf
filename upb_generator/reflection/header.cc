// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Generates foo.upbdefs.h.

#include <string>

#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "upb/reflection/def.hpp"
#include "upb_generator/common/names.h"
#include "upb_generator/file_layout.h"
#include "upb_generator/plugin.h"
#include "upb_generator/reflection/context.h"
#include "upb_generator/reflection/names.h"

namespace upb {
namespace generator {
namespace reflection {
namespace {

void GenerateMessageDefAccessor(upb::MessageDefPtr d, Context& ctx) {
  ctx.Emit(
      {{"def_init_symbol", ReflectionFileSymbol(d.file().name())},
       {"full_name", d.full_name()},
       {"get_message_symbol", ReflectionGetMessageSymbol(d.full_name())}},
      R"cc(
        UPB_INLINE const upb_MessageDef *$get_message_symbol$(upb_DefPool *s) {
          _upb_DefPool_LoadDefInit(s, &$def_init_symbol$);
          return upb_DefPool_FindMessageByName(s, "$full_name$");
        }
      )cc");
}

void WriteDefHeader(upb::FileDefPtr file, Context& ctx) {
  ctx.Emit({{"def_init_symbol", ReflectionFileSymbol(file.name())},
            {"dllexport_decl", ctx.options().dllexport_decl},
            {"file_warning", FileWarning(file.name())},
            {"include_guard", IncludeGuard(file.name())}},
           R"(
             $file_warning$

             #ifndef $include_guard$_UPBDEFS_H_
             #define $include_guard$_UPBDEFS_H_

             #include "upb/reflection/def.h"
             #include "upb/reflection/internal/def_pool.h"

             #include "upb/port/def.inc"

             #ifdef __cplusplus
             extern "C" {
             #endif

             extern$ dllexport_decl$ _upb_DefPool_Init $def_init_symbol$;
           )");

  for (auto msg : SortedMessages(file)) {
    GenerateMessageDefAccessor(msg, ctx);
    ctx.Emit("\n");
  }

  ctx.Emit({{"include_guard", IncludeGuard(file.name())}},
           R"(
             #ifdef __cplusplus
             }  /* extern "C" */
             #endif

             #include "upb/port/undef.inc"

             #endif  /* $include_guard$_UPBDEFS_H_ */
      )");
}

}  // namespace

std::string DefHeaderFilename(upb::FileDefPtr file) {
  return StripExtension(file.name()) + ".upbdefs.h";
}

void GenerateReflectionHeader(upb::FileDefPtr file, const Options& options,
                              Plugin* plugin) {
  Context h_context(options, plugin->Open(DefHeaderFilename(file)));
  WriteDefHeader(file, h_context);
}

}  // namespace reflection
}  // namespace generator
}  // namespace upb
