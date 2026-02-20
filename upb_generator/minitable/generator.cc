// Protocol Buffers - Google's data interrdchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb_generator/minitable/generator.h"

#include <cstddef>
#include <cstdint>
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
#include "upb/mini_table/internal/sub.h"
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

bool IsCrossFile(upb::FieldDefPtr field) {
  return field.message_type() != field.containing_type();
}

std::string GetSub(upb::FieldDefPtr field, bool is_extension,
                   const MiniTableOptions& options) {
  if (auto message_def = field.message_type()) {
    return absl::Substitute(
        "{.UPB_PRIVATE(submsg) = &$0}",
        options.one_output_per_message && !is_extension && !field.IsMap() &&
                IsCrossFile(field)
            ? WeakMiniTableMessageVarName(message_def.full_name())
            : MessageVarName(message_def));
  }

  if (auto enum_def = field.enum_subdef()) {
    if (enum_def.is_closed()) {
      return absl::Substitute("{.UPB_PRIVATE(subenum) = &$0}",
                              MiniTableEnumVarName(enum_def.full_name()));
    }
  }

  return std::string("{.UPB_PRIVATE(submsg) = NULL}");
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
      output(R"(
        UPB_WEAK_SINGLETON_PLACEHOLDER_MINITABLE();
      )");
      emitted_static_tree_shaken = true;
    }
    std::string weak_var =
        WeakMiniTableMessageVarName(field.message_type().full_name());
    std::string stub_var = absl::StrCat(weak_var, "_stub");
    output("UPB_WEAK_PLACEHOLDER_MINITABLE($0);", stub_var);
    output("\n");
    output(
        R"(
          UPB_WEAK_ALIAS(const upb_MiniTable, $0, $1);
        )",
        stub_var,
        WeakMiniTableMessageVarName(field.message_type().full_name()));
    output("\n");
  }
}

// Writes a single message into a .upb.c source file.
void WriteMessage(upb::MessageDefPtr message, const DefPoolPair& pools,
                  const MiniTableOptions& options, Output& output) {
  const upb_MiniTable* mt_32 = pools.GetMiniTable32(message);
  const upb_MiniTable* mt_64 = pools.GetMiniTable64(message);
  const upb_MiniTable* mt_native = SIZE_MAX == UINT32_MAX ? mt_32 : mt_64;
  int field_count = upb_MiniTable_FieldCount(mt_64);
  size_t subs_base = UPB_ALIGN_UP(field_count * sizeof(upb_MiniTableField),
                                  UPB_ALIGN_OF(upb_MiniTableSubInternal));
  std::vector<std::string> subs;

  std::string fields_array_ref = "NULL";
  absl::flat_hash_set<const upb_MiniTable*> seen;

  // Construct map of sub messages by field number.
  bool emitted_static_tree_shaken = false;
  for (int i = 0; i < mt_64->UPB_PRIVATE(field_count); i++) {
    const upb_MiniTableField* f = &mt_native->UPB_PRIVATE(fields)[i];
    if (f->UPB_PRIVATE(submsg_ofs) == kUpb_NoSub) continue;
    const int f_number = upb_MiniTableField_Number(f);
    upb::FieldDefPtr field = message.FindFieldByNumber(f_number);
    size_t sub_index =
        ((i * sizeof(upb_MiniTableField)) +
         f->UPB_PRIVATE(submsg_ofs) * kUpb_SubmsgOffsetBytes - subs_base) /
        sizeof(upb_MiniTableSubInternal);
    if (subs.size() <= sub_index) {
      subs.resize(sub_index + 1);
    }
    ABSL_CHECK(subs[sub_index].empty());
    subs[sub_index] = GetSub(field, false, options);

    DeclareSubMiniTable(field, pools, options, output,
                        emitted_static_tree_shaken, seen);
  }

  std::string struct_name = MessageVarName(message) + "_Fields";

  if (field_count > 0) {
    // Create a custom struct for the fields/subs, with arrays that are
    // precisely the right size
    // .
    output("typedef struct {\n");
    output("  upb_MiniTableField fields[$0];\n", field_count);
    if (!subs.empty()) {
      output("  upb_MiniTableSubInternal subs[$0];\n", subs.size());
    }
    output("} $0;\n\n", struct_name);

    // Emit instance of the custom struct.
    std::string fields_array_name = MiniTableFieldsVarName(message.full_name());
    fields_array_ref = "&" + fields_array_name + ".fields[0]";
    output("static const $0 $1 = {{\n", struct_name, fields_array_name);
    for (int i = 0; i < mt_64->UPB_PRIVATE(field_count); i++) {
      WriteMessageField(message.FindFieldByNumber(
                            mt_64->UPB_PRIVATE(fields)[i].UPB_PRIVATE(number)),
                        &mt_64->UPB_PRIVATE(fields)[i],
                        &mt_32->UPB_PRIVATE(fields)[i], output);
    }
    if (!subs.empty()) {
      output(" },\n  {\n");
      for (const auto& sub : subs) {
        ABSL_CHECK(!sub.empty());
        output("  $0,\n", sub);
      }
    }
    output("}};\n\n");
  }

  upb_DecodeFast_TableEntry table_entries[32];
  int table_size = upb_DecodeFast_BuildTable(mt_64, table_entries);
  uint8_t table_mask = upb_DecodeFast_GetTableMask(table_size);

  std::string msgext = "kUpb_ExtMode_NonExtendable";

  if (message.extension_range_count()) {
    if (google_protobuf_MessageOptions_message_set_wire_format(message.options())) {
      msgext = "kUpb_ExtMode_IsMessageSet";
    } else {
      msgext = "kUpb_ExtMode_Extendable";
    }
  }

  output("const upb_MiniTable $0 = {\n", MessageVarName(message));
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
  if (options.one_output_per_message) {
    output(R"(
             UPB_STRONG_ALIAS(const upb_MiniTable, $0, $1);
           )",
           MessageVarName(message),
           WeakMiniTableMessageVarName(message.full_name()));
  }
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
                    const MiniTableOptions& options, Output& output) {
  output("UPB_LINKARR_APPEND(upb_AllExts)\n");
  output("const upb_MiniTableExtension $0 = {\n  ", ExtensionVarName(ext));
  output("$0,\n", FieldInitializer(pools, ext));
  output("  $0,\n", GetSub(ext, true, options));
  output("  &$0,\n", MessageVarName(ext.containing_type()));
  output("\n};\n");
}

void RegisterExtensions(Output& output, absl::string_view unique_name) {
  output("UPB_LINKARR_DECLARE(upb_AllExts, const upb_MiniTableExtension);\n");
  output("UPB_CONSTRUCTOR(upb_GeneratedRegistry_Constructor, $0) {\n",
         unique_name);
  // TODO Although we define this function as weak and only one
  // copy will ever exist in any binary, every instance will get registered as a
  // separate constructor call.  To avoid duplicate registrations, we use a
  // static variable to ensure that the function is only executed once.
  output("  static bool finished = false;\n");
  output("  if (finished) return;\n");
  output("  finished = true;\n");
  output("  static UPB_PRIVATE(upb_GeneratedExtensionListEntry) entry = {\n");
  output("    UPB_LINKARR_START(upb_AllExts),\n");
  output("    UPB_LINKARR_STOP(upb_AllExts),\n");
  output("    NULL\n");
  output("  };\n");
  output("  UPB_ASSERT(entry.next == NULL);\n");
  output("  entry.next = UPB_PRIVATE(upb_generated_extension_list);\n");
  output("  UPB_PRIVATE(upb_generated_extension_list) = &entry;\n");
  output("}\n");
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

  output(
      "extern const UPB_PRIVATE(upb_GeneratedExtensionListEntry)* "
      "UPB_PRIVATE(upb_generated_extension_list);\n");
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
      WriteExtension(pools, ext, options, output);
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

  if (!extensions.empty()) {
    RegisterExtensions(
        output,
        absl::StrCat(MiniTableExtensionVarName(file.name()), "_constructor"));
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
  if (!extensions.empty()) {
    // All extensions can be written to a single file because none of the
    // symbols are retain, and the only weak symbols exist for deduping.  It's
    // most efficient to write them all together, especially with
    // upb_RegisterExtensionList getting called once per weak definition.
    Output output;
    WriteMiniTableSourceIncludes(file, options, output, true);
    for (const auto ext : extensions) {
      WriteExtension(pools, ext, options, output);
    }
    RegisterExtensions(
        output, absl::StrCat(MiniTableExtensionVarName(file.name()), "_", i + 1,
                             "_constructor"));
    auto stream = absl::WrapUnique(
        context->Open(MultipleSourceFilename(file, "extensions", &i)));
    ABSL_CHECK(stream->WriteCord(absl::Cord(output.output())));
  }
}

}  // namespace generator
}  // namespace upb
