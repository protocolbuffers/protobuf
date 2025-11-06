// Protocol Buffers - Google's data interrdchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb_generator/minitable/generator.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/memory/memory.h"
#include "absl/strings/cord.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/message.h"
#include "upb/reflection/def.hpp"
#include "upb/wire/decode_fast/select.h"
#include "upb_generator/common.h"
#include "upb_generator/common/names.h"
#include "upb_generator/file_layout.h"
#include "upb_generator/minitable/names.h"
#include "upb_generator/minitable/names_internal.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace generator {
namespace {

// Some local convenience aliases for MiniTable variable names.

std::string MessageVarName(upb::MessageDefPtr message) {
  return MiniTableMessageVarName(message.full_name());
}

std::string EnumVarName(upb::EnumDefPtr e) {
  return MiniTableEnumVarName(e.full_name());
}

std::string ExtensionVarName(upb::FieldDefPtr ext) {
  return MiniTableExtensionVarName(ext.full_name());
}

std::string FileVarName(upb::FileDefPtr file) {
  return MiniTableFileVarName(file.name());
}

std::string HeaderFilename(upb::FileDefPtr file, bool bootstrap) {
  return MiniTableHeaderFilename(file.name(), bootstrap);
}

std::string ArchDependentSize(int64_t size32, int64_t size64) {
  if (size32 == size64) return absl::StrCat(size32);
  return absl::Substitute("UPB_SIZE($0, $1)", size32, size64);
}

std::string FieldInitializer(const DefPoolPair& pools, upb::FieldDefPtr field) {
  return upb::generator::FieldInitializer(field, pools.GetField64(field),
                                          pools.GetField32(field));
}

// Writes a single field into a .upb.c source file.
void WriteMessageField(upb::FieldDefPtr field,
                       const upb_MiniTableField* field64,
                       const upb_MiniTableField* field32, Output& output) {
  output("  $0,\n", upb::generator::FieldInitializer(field, field64, field32));
}

std::string GetSub(upb::FieldDefPtr field, bool is_extension) {
  if (auto message_def = field.message_type()) {
    return absl::Substitute("{.UPB_PRIVATE(submsg) = &$0}",
                            MessageVarName(message_def));
  }

  if (auto enum_def = field.enum_subdef()) {
    if (enum_def.is_closed()) {
      return absl::Substitute("{.UPB_PRIVATE(subenum) = &$0}",
                              MiniTableEnumVarName(enum_def.full_name()));
    }
  }

  return std::string("{.UPB_PRIVATE(submsg) = NULL}");
}

bool IsCrossFile(upb::FieldDefPtr field) {
  return field.message_type() != field.containing_type();
}

// When using one_output_per_message, we use weak references to sub-MiniTables
// to allow them to be tree shaken if not used directly. This requires us to
// declare the sub-MiniTable as __attribute__((weakref(...))) in the file that
// uses it.
//
// We also have to add regular declarations for any sub-tables that are not
// tree shakable (enums and map entry messages). This is necessary because we
// cannot include the regular header for them, because the declarations there
// would conflict with the weak references.
void DeclareSubMiniTable(upb::FieldDefPtr field, const DefPoolPair& pools,
                         const MiniTableOptions& options, Output& output,
                         bool& emitted_static_tree_shaken,
                         absl::flat_hash_set<const upb_MiniTable*>& seen) {
  if (!options.one_output_per_message) return;

  if (field.IsEnum()) {
    output("extern const upb_MiniTableEnum $0;\n",
           EnumVarName(field.enum_subdef()));
    return;
  }

  ABSL_CHECK(field.IsSubMessage());

  if (field.IsMap() || !IsCrossFile(field)) {
    output("extern const upb_MiniTable $0;\n",
           MessageVarName(field.message_type()));
    return;
  }

  if (seen.insert(pools.GetMiniTable64(field.message_type())).second) {
    if (!emitted_static_tree_shaken) {
      // This is the MiniTable that we will weakly reference from this
      // sub-message field. If the real MiniTable for this message is not linked
      // in, we will use this one as an empty stub.
      //
      // We declare this weak so that it can be emitted in multiple
      // .upb_minitable.c files without causing duplicate symbol errors, but
      // luckily all of the instances will be de-duped into a single copy by the
      // linker, so we don't waste space.
      //
      // We also declare it hidden so that it doesn't show up in the dynamic
      // symbol table, if this .upb_minitable.c file gets linked into a shared
      // library. Each shared library will end up with its own copy of the
      // weakref stub, which wastes a tiny amount of space, but has the nice
      // property that it keeps this symbol totally hidden inside the library.
      output(R"cc(
        __attribute__((weak, visibility("hidden")))
        const upb_MiniTable upb_MiniTable_StaticallyTreeShaken = {
            .UPB_PRIVATE(subs) = NULL,
            .UPB_PRIVATE(fields) = NULL,
            .UPB_PRIVATE(size) = sizeof(struct upb_Message),
            .UPB_PRIVATE(field_count) = 0,
            .UPB_PRIVATE(ext) = kUpb_ExtMode_NonExtendable,
            .UPB_PRIVATE(dense_below) = 0,
            .UPB_PRIVATE(table_mask) = -1,
            .UPB_PRIVATE(required_count) = 0,
        };
      )cc");
      output("\n");
      emitted_static_tree_shaken = true;
    }
    output(
        R"cc(
          static __attribute__((weakref("upb_MiniTable_StaticallyTreeShaken")))
          const upb_MiniTable $0;
        )cc",
        MessageVarName(field.message_type()));
    output("\n");
  }
}

// Writes a single message into a .upb.c source file.
void WriteMessage(upb::MessageDefPtr message, const DefPoolPair& pools,
                  const MiniTableOptions& options, Output& output) {
  std::string fields_array_ref = "NULL";
  std::string submsgs_array_ref = "NULL";
  std::string subenums_array_ref = "NULL";
  const upb_MiniTable* mt_32 = pools.GetMiniTable32(message);
  const upb_MiniTable* mt_64 = pools.GetMiniTable64(message);
  std::map<int, std::string> subs;
  absl::flat_hash_set<const upb_MiniTable*> seen;

  // Construct map of sub messages by field number.
  bool emitted_static_tree_shaken = false;
  for (int i = 0; i < mt_64->UPB_PRIVATE(field_count); i++) {
    const upb_MiniTableField* f = &mt_64->UPB_PRIVATE(fields)[i];
    uint32_t index = f->UPB_PRIVATE(submsg_index);

    if (index == kUpb_NoSub) continue;

    const int f_number = upb_MiniTableField_Number(f);
    upb::FieldDefPtr field = message.FindFieldByNumber(f_number);
    auto pair = subs.emplace(index, GetSub(field, false));
    ABSL_CHECK(pair.second);

    DeclareSubMiniTable(field, pools, options, output,
                        emitted_static_tree_shaken, seen);
  }

  // Write upb_MiniTableSubInternal table for sub messages referenced from
  // fields.
  if (!subs.empty()) {
    std::string submsgs_array_name =
        MiniTableSubMessagesVarName(message.full_name());
    submsgs_array_ref = "&" + submsgs_array_name + "[0]";
    output("static const upb_MiniTableSubInternal $0[$1] = {\n",
           submsgs_array_name, subs.size());

    int i = 0;
    for (const auto& pair : subs) {
      ABSL_CHECK(pair.first == i++);
      output("  $0,\n", pair.second);
    }

    output("};\n\n");
  }

  // Write upb_MiniTableField table.
  if (mt_64->UPB_PRIVATE(field_count) > 0) {
    std::string fields_array_name = MiniTableFieldsVarName(message.full_name());
    fields_array_ref = "&" + fields_array_name + "[0]";
    output("static const upb_MiniTableField $0[$1] = {\n", fields_array_name,
           mt_64->UPB_PRIVATE(field_count));
    for (int i = 0; i < mt_64->UPB_PRIVATE(field_count); i++) {
      WriteMessageField(message.FindFieldByNumber(
                            mt_64->UPB_PRIVATE(fields)[i].UPB_PRIVATE(number)),
                        &mt_64->UPB_PRIVATE(fields)[i],
                        &mt_32->UPB_PRIVATE(fields)[i], output);
    }
    output("};\n\n");
  }

  upb_DecodeFast_TableEntry table_entries[32];
  int table_size = upb_DecodeFast_BuildTable(mt_64, table_entries);
  uint8_t table_mask = upb_DecodeFast_GetTableMask(table_size);

  std::string msgext = "kUpb_ExtMode_NonExtendable";

  if (message.extension_range_count()) {
    if (UPB_DESC(MessageOptions_message_set_wire_format)(message.options())) {
      msgext = "kUpb_ExtMode_IsMessageSet";
    } else {
      msgext = "kUpb_ExtMode_Extendable";
    }
  }

  output("const upb_MiniTable $0 = {\n", MessageVarName(message));
  output("  $0,\n", submsgs_array_ref);
  output("  $0,\n", fields_array_ref);
  output("  $0, $1, $2, $3, UPB_FASTTABLE_MASK($4), $5,\n",
         ArchDependentSize(mt_32->UPB_PRIVATE(size), mt_64->UPB_PRIVATE(size)),
         mt_64->UPB_PRIVATE(field_count), msgext,
         mt_64->UPB_PRIVATE(dense_below), table_mask,
         mt_64->UPB_PRIVATE(required_count));
  output("#ifdef UPB_TRACING_ENABLED\n");
  output("  \"$0\",\n", message.full_name());
  output("#endif\n");
  if (table_size > 0) {
    output("  UPB_FASTTABLE_INIT({\n");
    for (int i = 0; i < table_size; i++) {
      output("    {0x$1, &$0},\n",
             upb_DecodeFast_GetFunctionName(table_entries[i].function_idx),
             absl::StrCat(
                 absl::Hex(table_entries[i].function_data, absl::kZeroPad16)));
    }
    output("  })\n");
  }
  output("};\n\n");
}

void WriteEnum(upb::EnumDefPtr e, Output& output) {
  std::string values_init = "{\n";
  const upb_MiniTableEnum* mt = e.mini_table();
  uint32_t value_count =
      (mt->UPB_PRIVATE(mask_limit) / 32) + mt->UPB_PRIVATE(value_count);
  for (uint32_t i = 0; i < value_count; i++) {
    absl::StrAppend(&values_init, "                0x",
                    absl::Hex(mt->UPB_PRIVATE(data)[i]), ",\n");
  }
  values_init += "    }";

  output(
      R"cc(
        const upb_MiniTableEnum $0 = {
            $1,
            $2,
            $3,
        };
      )cc",
      EnumVarName(e), mt->UPB_PRIVATE(mask_limit), mt->UPB_PRIVATE(value_count),
      values_init);
  output("\n");
}

void WriteExtension(const DefPoolPair& pools, upb::FieldDefPtr ext,
                    Output& output) {
  output("UPB_LINKARR_APPEND(upb_AllExts)\n");
  output("const upb_MiniTableExtension $0 = {\n  ", ExtensionVarName(ext));
  output("$0,\n", FieldInitializer(pools, ext));
  output("  &$0,\n", MessageVarName(ext.containing_type()));
  output("  $0,\n", GetSub(ext, true));
  output("\n};\n");
}

}  // namespace

void WriteMiniTableHeader(const DefPoolPair& pools, upb::FileDefPtr file,
                          const MiniTableOptions& options, Output& output) {
  output(FileWarning(file.name()));
  output(
      "#ifndef $0_UPB_MINITABLE_H_\n"
      "#define $0_UPB_MINITABLE_H_\n\n"
      "#include \"upb/generated_code_support.h\"\n",
      IncludeGuard(file.name()));

  for (int i = 0; i < file.public_dependency_count(); i++) {
    if (i == 0) {
      output("/* Public Imports. */\n");
    }
    output("#include \"$0\"\n",
           HeaderFilename(file.public_dependency(i), options.bootstrap));
    if (i == file.public_dependency_count() - 1) {
      output("\n");
    }
  }

  output(
      "\n"
      "// Must be last.\n"
      "#include \"upb/port/def.inc\"\n"
      "\n"
      "#ifdef __cplusplus\n"
      "extern \"C\" {\n"
      "#endif\n"
      "\n");

  const std::vector<upb::MessageDefPtr> this_file_messages =
      SortedMessages(file);
  const std::vector<upb::FieldDefPtr> this_file_exts = SortedExtensions(file);

  for (auto message : this_file_messages) {
    output("extern const upb_MiniTable $0;\n", MessageVarName(message));
  }
  for (auto ext : this_file_exts) {
    output("extern const upb_MiniTableExtension $0;\n", ExtensionVarName(ext));
  }

  output("\n");

  std::vector<upb::EnumDefPtr> this_file_enums =
      SortedEnums(file, kClosedEnums);

  for (const auto enumdesc : this_file_enums) {
    output("extern const upb_MiniTableEnum $0;\n", EnumVarName(enumdesc));
  }

  output("extern const upb_MiniTableFile $0;\n\n", FileVarName(file));

  output(
      "#ifdef __cplusplus\n"
      "}  /* extern \"C\" */\n"
      "#endif\n"
      "\n"
      "#include \"upb/port/undef.inc\"\n"
      "\n"
      "#endif  /* $0_UPB_MINITABLE_H_ */\n",
      IncludeGuard(file.name()));
}

void WriteMiniTableSourceIncludes(upb::FileDefPtr file,
                                  const MiniTableOptions& options,
                                  Output& output, bool is_file) {
  output(FileWarning(file.name()));

  output(
      "#include <stddef.h>\n"
      "#include \"upb/generated_code_support.h\"\n");

  if (is_file) {
    output("#include \"$0\"\n", HeaderFilename(file, options.bootstrap));

    for (int i = 0; i < file.dependency_count(); i++) {
      if (options.strip_nonfunctional_codegen &&
          google::protobuf::compiler::IsKnownFeatureProto(file.dependency(i).name())) {
        // Strip feature imports for editions codegen tests.
        continue;
      }
      output("#include \"$0\"\n",
             HeaderFilename(file.dependency(i), options.bootstrap));
    }
  }

  output(
      "\n"
      "// Must be last.\n"
      "#include \"upb/port/def.inc\"\n"
      "\n");
}

void WriteMiniTableSource(const DefPoolPair& pools, upb::FileDefPtr file,
                          const MiniTableOptions& options, Output& output) {
  WriteMiniTableSourceIncludes(file, options, output, true);

  std::vector<upb::MessageDefPtr> messages = SortedMessages(file);
  std::vector<upb::FieldDefPtr> extensions = SortedExtensions(file);
  std::vector<upb::EnumDefPtr> enums = SortedEnums(file, kClosedEnums);

  if (options.one_output_per_message) {
    for (const auto e : enums) {
      output("extern const upb_MiniTableEnum $0;\n", EnumVarName(e));
    }
    for (const auto ext : extensions) {
      output("extern const upb_MiniTableExtension $0;\n",
             ExtensionVarName(ext));
    }
  } else {
    for (auto message : messages) {
      WriteMessage(message, pools, options, output);
    }
    for (const auto e : enums) {
      WriteEnum(e, output);
    }
    for (const auto ext : extensions) {
      WriteExtension(pools, ext, output);
    }
  }

  // Messages.
  if (!messages.empty()) {
    output("static const upb_MiniTable *$0[$1] = {\n", kMessagesInit,
           messages.size());
    for (auto message : messages) {
      output("  &$0,\n", MessageVarName(message));
    }
    output("};\n");
    output("\n");
  }

  // Enums.
  if (!enums.empty()) {
    output("static const upb_MiniTableEnum *$0[$1] = {\n", kEnumsInit,
           enums.size());
    for (const auto e : enums) {
      output("  &$0,\n", EnumVarName(e));
    }
    output("};\n");
    output("\n");
  }

  if (!extensions.empty()) {
    // Extensions.
    output(
        "\n"
        "static const upb_MiniTableExtension *$0[$1] = {\n",
        kExtensionsInit, extensions.size());

    for (auto ext : extensions) {
      output("  &$0,\n", ExtensionVarName(ext));
    }

    output(
        "};\n"
        "\n");
  }

  output("const upb_MiniTableFile $0 = {\n", FileVarName(file));
  output("  $0,\n", messages.empty() ? "NULL" : kMessagesInit);
  output("  $0,\n", enums.empty() ? "NULL" : kEnumsInit);
  output("  $0,\n", extensions.empty() ? "NULL" : kExtensionsInit);
  output("  $0,\n", messages.size());
  output("  $0,\n", enums.size());
  output("  $0,\n", extensions.size());
  output("};\n\n");

  output("#include \"upb/port/undef.inc\"\n");
  output("\n");
}

std::string MultipleSourceFilename(upb::FileDefPtr file,
                                   absl::string_view full_name, int* i) {
  *i += 1;
  return absl::StrCat(StripExtension(file.name()), ".upb_weak_minitables/", *i,
                      ".upb.c");
}

void WriteMiniTableMultipleSources(
    const DefPoolPair& pools, upb::FileDefPtr file,
    const MiniTableOptions& options,
    google::protobuf::compiler::GeneratorContext* context) {
  std::vector<upb::MessageDefPtr> messages = SortedMessages(file);
  std::vector<upb::FieldDefPtr> extensions = SortedExtensions(file);
  std::vector<upb::EnumDefPtr> enums = SortedEnums(file, kClosedEnums);
  int i = 0;

  for (auto message : messages) {
    Output output;
    WriteMiniTableSourceIncludes(file, options, output, false);
    WriteMessage(message, pools, options, output);
    auto stream = absl::WrapUnique(
        context->Open(MultipleSourceFilename(file, message.full_name(), &i)));
    ABSL_CHECK(stream->WriteCord(absl::Cord(output.output())));
  }
  for (const auto e : enums) {
    Output output;
    WriteMiniTableSourceIncludes(file, options, output, false);
    WriteEnum(e, output);
    auto stream = absl::WrapUnique(
        context->Open(MultipleSourceFilename(file, e.full_name(), &i)));
    ABSL_CHECK(stream->WriteCord(absl::Cord(output.output())));
  }
  for (const auto ext : extensions) {
    Output output;
    WriteMiniTableSourceIncludes(file, options, output, true);
    WriteExtension(pools, ext, output);
    auto stream = absl::WrapUnique(
        context->Open(MultipleSourceFilename(file, ext.full_name(), &i)));
    ABSL_CHECK(stream->WriteCord(absl::Cord(output.output())));
  }
}

}  // namespace generator
}  // namespace upb
