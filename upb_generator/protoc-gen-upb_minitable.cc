// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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
//     * Neither the name of Google LLC nor the names of its
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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/reflection/def.hpp"
#include "upb/wire/types.h"
#include "upb_generator/common.h"
#include "upb_generator/file_layout.h"
#include "upb_generator/names.h"
#include "upb_generator/plugin.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace generator {
namespace {

// Returns fields in order of "hotness", eg. how frequently they appear in
// serialized payloads. Ideally this will use a profile. When we don't have
// that, we assume that fields with smaller numbers are used more frequently.
inline std::vector<upb::FieldDefPtr> FieldHotnessOrder(
    upb::MessageDefPtr message) {
  std::vector<upb::FieldDefPtr> fields;
  size_t field_count = message.field_count();
  fields.reserve(field_count);
  for (size_t i = 0; i < field_count; i++) {
    fields.push_back(message.field(i));
  }
  std::sort(fields.begin(), fields.end(),
            [](upb::FieldDefPtr a, upb::FieldDefPtr b) {
              return std::make_pair(!a.is_required(), a.number()) <
                     std::make_pair(!b.is_required(), b.number());
            });
  return fields;
}

std::string SourceFilename(upb::FileDefPtr file) {
  return StripExtension(file.name()) + ".upb_minitable.c";
}

std::string ExtensionIdentBase(upb::FieldDefPtr ext) {
  assert(ext.is_extension());
  std::string ext_scope;
  if (ext.extension_scope()) {
    return MessageName(ext.extension_scope());
  } else {
    return ToCIdent(ext.file().package());
  }
}

std::string ExtensionLayout(upb::FieldDefPtr ext) {
  return absl::StrCat(ExtensionIdentBase(ext), "_", ext.name(), "_ext");
}

const char* kEnumsInit = "enums_layout";
const char* kExtensionsInit = "extensions_layout";
const char* kMessagesInit = "messages_layout";

void WriteHeader(const DefPoolPair& pools, upb::FileDefPtr file,
                 Output& output) {
  EmitFileWarning(file.name(), output);
  output(
      "#ifndef $0_UPB_MINITABLE_H_\n"
      "#define $0_UPB_MINITABLE_H_\n\n"
      "#include \"upb/generated_code_support.h\"\n",
      ToPreproc(file.name()));

  for (int i = 0; i < file.public_dependency_count(); i++) {
    if (i == 0) {
      output("/* Public Imports. */\n");
    }
    output("#include \"$0\"\n",
           MiniTableHeaderFilename(file.public_dependency(i)));
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
    output("extern const upb_MiniTable $0;\n", MessageInitName(message));
  }
  for (auto ext : this_file_exts) {
    output("extern const upb_MiniTableExtension $0;\n", ExtensionLayout(ext));
  }

  output("\n");

  std::vector<upb::EnumDefPtr> this_file_enums = SortedEnums(file);

  if (file.syntax() == kUpb_Syntax_Proto2) {
    for (const auto enumdesc : this_file_enums) {
      output("extern const upb_MiniTableEnum $0;\n", EnumInit(enumdesc));
    }
  }

  output("extern const upb_MiniTableFile $0;\n\n", FileLayoutName(file));

  output(
      "#ifdef __cplusplus\n"
      "}  /* extern \"C\" */\n"
      "#endif\n"
      "\n"
      "#include \"upb/port/undef.inc\"\n"
      "\n"
      "#endif  /* $0_UPB_MINITABLE_H_ */\n",
      ToPreproc(file.name()));
}

typedef std::pair<std::string, uint64_t> TableEntry;

uint32_t GetWireTypeForField(upb::FieldDefPtr field) {
  if (field.packed()) return kUpb_WireType_Delimited;
  switch (field.type()) {
    case kUpb_FieldType_Double:
    case kUpb_FieldType_Fixed64:
    case kUpb_FieldType_SFixed64:
      return kUpb_WireType_64Bit;
    case kUpb_FieldType_Float:
    case kUpb_FieldType_Fixed32:
    case kUpb_FieldType_SFixed32:
      return kUpb_WireType_32Bit;
    case kUpb_FieldType_Int64:
    case kUpb_FieldType_UInt64:
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_Bool:
    case kUpb_FieldType_UInt32:
    case kUpb_FieldType_Enum:
    case kUpb_FieldType_SInt32:
    case kUpb_FieldType_SInt64:
      return kUpb_WireType_Varint;
    case kUpb_FieldType_Group:
      return kUpb_WireType_StartGroup;
    case kUpb_FieldType_Message:
    case kUpb_FieldType_String:
    case kUpb_FieldType_Bytes:
      return kUpb_WireType_Delimited;
  }
  UPB_UNREACHABLE();
}

uint32_t MakeTag(uint32_t field_number, uint32_t wire_type) {
  return field_number << 3 | wire_type;
}

size_t WriteVarint32ToArray(uint64_t val, char* buf) {
  size_t i = 0;
  do {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    buf[i++] = byte;
  } while (val);
  return i;
}

uint64_t GetEncodedTag(upb::FieldDefPtr field) {
  uint32_t wire_type = GetWireTypeForField(field);
  uint32_t unencoded_tag = MakeTag(field.number(), wire_type);
  char tag_bytes[10] = {0};
  WriteVarint32ToArray(unencoded_tag, tag_bytes);
  uint64_t encoded_tag = 0;
  memcpy(&encoded_tag, tag_bytes, sizeof(encoded_tag));
  // TODO: byte-swap for big endian.
  return encoded_tag;
}

int GetTableSlot(upb::FieldDefPtr field) {
  uint64_t tag = GetEncodedTag(field);
  if (tag > 0x7fff) {
    // Tag must fit within a two-byte varint.
    return -1;
  }
  return (tag & 0xf8) >> 3;
}

bool TryFillTableEntry(const DefPoolPair& pools, upb::FieldDefPtr field,
                       TableEntry& ent) {
  const upb_MiniTable* mt = pools.GetMiniTable64(field.containing_type());
  const upb_MiniTableField* mt_f =
      upb_MiniTable_FindFieldByNumber(mt, field.number());
  std::string type = "";
  std::string cardinality = "";
  switch (upb_MiniTableField_Type(mt_f)) {
    case kUpb_FieldType_Bool:
      type = "b1";
      break;
    case kUpb_FieldType_Enum:
      if (upb_MiniTableField_IsClosedEnum(mt_f)) {
        // We don't have the means to test proto2 enum fields for valid values.
        return false;
      }
      [[fallthrough]];
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_UInt32:
      type = "v4";
      break;
    case kUpb_FieldType_Int64:
    case kUpb_FieldType_UInt64:
      type = "v8";
      break;
    case kUpb_FieldType_Fixed32:
    case kUpb_FieldType_SFixed32:
    case kUpb_FieldType_Float:
      type = "f4";
      break;
    case kUpb_FieldType_Fixed64:
    case kUpb_FieldType_SFixed64:
    case kUpb_FieldType_Double:
      type = "f8";
      break;
    case kUpb_FieldType_SInt32:
      type = "z4";
      break;
    case kUpb_FieldType_SInt64:
      type = "z8";
      break;
    case kUpb_FieldType_String:
      type = "s";
      break;
    case kUpb_FieldType_Bytes:
      type = "b";
      break;
    case kUpb_FieldType_Message:
      type = "m";
      break;
    default:
      return false;  // Not supported yet.
  }

  switch (upb_FieldMode_Get(mt_f)) {
    case kUpb_FieldMode_Map:
      return false;  // Not supported yet (ever?).
    case kUpb_FieldMode_Array:
      if (mt_f->mode & kUpb_LabelFlags_IsPacked) {
        cardinality = "p";
      } else {
        cardinality = "r";
      }
      break;
    case kUpb_FieldMode_Scalar:
      if (mt_f->presence < 0) {
        cardinality = "o";
      } else {
        cardinality = "s";
      }
      break;
  }

  uint64_t expected_tag = GetEncodedTag(field);

  // Data is:
  //
  //                  48                32                16                 0
  // |--------|--------|--------|--------|--------|--------|--------|--------|
  // |   offset (16)   |case offset (16) |presence| submsg |  exp. tag (16)  |
  // |--------|--------|--------|--------|--------|--------|--------|--------|
  //
  // - |presence| is either hasbit index or field number for oneofs.

  uint64_t data = static_cast<uint64_t>(mt_f->offset) << 48 | expected_tag;

  if (field.IsSequence()) {
    // No hasbit/oneof-related fields.
  }
  if (field.real_containing_oneof()) {
    uint64_t case_offset = ~mt_f->presence;
    if (case_offset > 0xffff || field.number() > 0xff) return false;
    data |= field.number() << 24;
    data |= case_offset << 32;
  } else {
    uint64_t hasbit_index = 63;  // No hasbit (set a high, unused bit).
    if (mt_f->presence) {
      hasbit_index = mt_f->presence;
      if (hasbit_index > 31) return false;
    }
    data |= hasbit_index << 24;
  }

  if (field.ctype() == kUpb_CType_Message) {
    uint64_t idx = mt_f->UPB_PRIVATE(submsg_index);
    if (idx > 255) return false;
    data |= idx << 16;

    std::string size_ceil = "max";
    size_t size = SIZE_MAX;
    if (field.message_type().file() == field.file()) {
      // We can only be guaranteed the size of the sub-message if it is in the
      // same file as us.  We could relax this to increase the speed of
      // cross-file sub-message parsing if we are comfortable requiring that
      // users compile all messages at the same time.
      const upb_MiniTable* sub_mt = pools.GetMiniTable64(field.message_type());
      size = sub_mt->size + 8;
    }
    std::vector<size_t> breaks = {64, 128, 192, 256};
    for (auto brk : breaks) {
      if (size <= brk) {
        size_ceil = std::to_string(brk);
        break;
      }
    }
    ent.first = absl::Substitute("upb_p$0$1_$2bt_max$3b", cardinality, type,
                                 expected_tag > 0xff ? "2" : "1", size_ceil);

  } else {
    ent.first = absl::Substitute("upb_p$0$1_$2bt", cardinality, type,
                                 expected_tag > 0xff ? "2" : "1");
  }
  ent.second = data;
  return true;
}

std::vector<TableEntry> FastDecodeTable(upb::MessageDefPtr message,
                                        const DefPoolPair& pools) {
  std::vector<TableEntry> table;
  for (const auto field : FieldHotnessOrder(message)) {
    TableEntry ent;
    int slot = GetTableSlot(field);
    // std::cerr << "table slot: " << field->number() << ": " << slot << "\n";
    if (slot < 0) {
      // Tag can't fit in the table.
      continue;
    }
    if (!TryFillTableEntry(pools, field, ent)) {
      // Unsupported field type or offset, hasbit index, etc. doesn't fit.
      continue;
    }
    while ((size_t)slot >= table.size()) {
      size_t size = std::max(static_cast<size_t>(1), table.size() * 2);
      table.resize(size, TableEntry{"_upb_FastDecoder_DecodeGeneric", 0});
    }
    if (table[slot].first != "_upb_FastDecoder_DecodeGeneric") {
      // A hotter field already filled this slot.
      continue;
    }
    table[slot] = ent;
  }
  return table;
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

std::string GetSub(upb::FieldDefPtr field) {
  if (auto message_def = field.message_type()) {
    return absl::Substitute("{.submsg = &$0}", MessageInitName(message_def));
  }

  if (auto enum_def = field.enum_subdef()) {
    if (enum_def.is_closed()) {
      return absl::Substitute("{.subenum = &$0}", EnumInit(enum_def));
    }
  }

  return std::string("{.submsg = NULL}");
}

// Writes a single message into a .upb.c source file.
void WriteMessage(upb::MessageDefPtr message, const DefPoolPair& pools,
                  Output& output) {
  std::string msg_name = ToCIdent(message.full_name());
  std::string fields_array_ref = "NULL";
  std::string submsgs_array_ref = "NULL";
  std::string subenums_array_ref = "NULL";
  const upb_MiniTable* mt_32 = pools.GetMiniTable32(message);
  const upb_MiniTable* mt_64 = pools.GetMiniTable64(message);
  std::map<int, std::string> subs;

  for (int i = 0; i < mt_64->field_count; i++) {
    const upb_MiniTableField* f = &mt_64->fields[i];
    uint32_t index = f->UPB_PRIVATE(submsg_index);
    if (index != kUpb_NoSub) {
      auto pair =
          subs.emplace(index, GetSub(message.FindFieldByNumber(f->number)));
      ABSL_CHECK(pair.second);
    }
  }

  if (!subs.empty()) {
    std::string submsgs_array_name = msg_name + "_submsgs";
    submsgs_array_ref = "&" + submsgs_array_name + "[0]";
    output("static const upb_MiniTableSub $0[$1] = {\n", submsgs_array_name,
           subs.size());

    int i = 0;
    for (const auto& pair : subs) {
      ABSL_CHECK(pair.first == i++);
      output("  $0,\n", pair.second);
    }

    output("};\n\n");
  }

  if (mt_64->field_count > 0) {
    std::string fields_array_name = msg_name + "__fields";
    fields_array_ref = "&" + fields_array_name + "[0]";
    output("static const upb_MiniTableField $0[$1] = {\n", fields_array_name,
           mt_64->field_count);
    for (int i = 0; i < mt_64->field_count; i++) {
      WriteMessageField(message.FindFieldByNumber(mt_64->fields[i].number),
                        &mt_64->fields[i], &mt_32->fields[i], output);
    }
    output("};\n\n");
  }

  std::vector<TableEntry> table;
  uint8_t table_mask = -1;

  table = FastDecodeTable(message, pools);

  if (table.size() > 1) {
    assert((table.size() & (table.size() - 1)) == 0);
    table_mask = (table.size() - 1) << 3;
  }

  std::string msgext = "kUpb_ExtMode_NonExtendable";

  if (message.extension_range_count()) {
    if (UPB_DESC(MessageOptions_message_set_wire_format)(message.options())) {
      msgext = "kUpb_ExtMode_IsMessageSet";
    } else {
      msgext = "kUpb_ExtMode_Extendable";
    }
  }

  output("const upb_MiniTable $0 = {\n", MessageInitName(message));
  output("  $0,\n", submsgs_array_ref);
  output("  $0,\n", fields_array_ref);
  output("  $0, $1, $2, $3, UPB_FASTTABLE_MASK($4), $5,\n",
         ArchDependentSize(mt_32->size, mt_64->size), mt_64->field_count,
         msgext, mt_64->dense_below, table_mask, mt_64->required_count);
  if (!table.empty()) {
    output("  UPB_FASTTABLE_INIT({\n");
    for (const auto& ent : table) {
      output("    {0x$1, &$0},\n", ent.first,
             absl::StrCat(absl::Hex(ent.second, absl::kZeroPad16)));
    }
    output("  })\n");
  }
  output("};\n\n");
}

void WriteEnum(upb::EnumDefPtr e, Output& output) {
  std::string values_init = "{\n";
  const upb_MiniTableEnum* mt = e.mini_table();
  uint32_t value_count = (mt->mask_limit / 32) + mt->value_count;
  for (uint32_t i = 0; i < value_count; i++) {
    absl::StrAppend(&values_init, "                0x", absl::Hex(mt->data[i]),
                    ",\n");
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
      EnumInit(e), mt->mask_limit, mt->value_count, values_init);
  output("\n");
}

int WriteEnums(const DefPoolPair& pools, upb::FileDefPtr file, Output& output) {
  if (file.syntax() != kUpb_Syntax_Proto2) return 0;

  std::vector<upb::EnumDefPtr> this_file_enums = SortedEnums(file);

  for (const auto e : this_file_enums) {
    WriteEnum(e, output);
  }

  if (!this_file_enums.empty()) {
    output("static const upb_MiniTableEnum *$0[$1] = {\n", kEnumsInit,
           this_file_enums.size());
    for (const auto e : this_file_enums) {
      output("  &$0,\n", EnumInit(e));
    }
    output("};\n");
    output("\n");
  }

  return this_file_enums.size();
}

int WriteMessages(const DefPoolPair& pools, upb::FileDefPtr file,
                  Output& output) {
  std::vector<upb::MessageDefPtr> file_messages = SortedMessages(file);

  if (file_messages.empty()) return 0;

  for (auto message : file_messages) {
    WriteMessage(message, pools, output);
  }

  output("static const upb_MiniTable *$0[$1] = {\n", kMessagesInit,
         file_messages.size());
  for (auto message : file_messages) {
    output("  &$0,\n", MessageInitName(message));
  }
  output("};\n");
  output("\n");
  return file_messages.size();
}

void WriteExtension(upb::FieldDefPtr ext, const DefPoolPair& pools,
                    Output& output) {
  output("$0,\n", FieldInitializer(pools, ext));
  output("  &$0,\n", MessageInitName(ext.containing_type()));
  output("  $0,\n", GetSub(ext));
}

int WriteExtensions(const DefPoolPair& pools, upb::FileDefPtr file,
                    Output& output) {
  auto exts = SortedExtensions(file);

  if (exts.empty()) return 0;

  // Order by full name for consistent ordering.
  std::map<std::string, upb::MessageDefPtr> forward_messages;

  for (auto ext : exts) {
    forward_messages[ext.containing_type().full_name()] = ext.containing_type();
    if (ext.message_type()) {
      forward_messages[ext.message_type().full_name()] = ext.message_type();
    }
  }

  for (auto ext : exts) {
    output("const upb_MiniTableExtension $0 = {\n  ", ExtensionLayout(ext));
    WriteExtension(ext, pools, output);
    output("\n};\n");
  }

  output(
      "\n"
      "static const upb_MiniTableExtension *$0[$1] = {\n",
      kExtensionsInit, exts.size());

  for (auto ext : exts) {
    output("  &$0,\n", ExtensionLayout(ext));
  }

  output(
      "};\n"
      "\n");
  return exts.size();
}

void WriteMiniTableSource(const DefPoolPair& pools, upb::FileDefPtr file,
                          Output& output) {
  EmitFileWarning(file.name(), output);

  output(
      "#include <stddef.h>\n"
      "#include \"upb/generated_code_support.h\"\n"
      "#include \"$0\"\n",
      MiniTableHeaderFilename(file));

  for (int i = 0; i < file.dependency_count(); i++) {
    output("#include \"$0\"\n", MiniTableHeaderFilename(file.dependency(i)));
  }

  output(
      "\n"
      "// Must be last.\n"
      "#include \"upb/port/def.inc\"\n"
      "\n");

  int msg_count = WriteMessages(pools, file, output);
  int ext_count = WriteExtensions(pools, file, output);
  int enum_count = WriteEnums(pools, file, output);

  output("const upb_MiniTableFile $0 = {\n", FileLayoutName(file));
  output("  $0,\n", msg_count ? kMessagesInit : "NULL");
  output("  $0,\n", enum_count ? kEnumsInit : "NULL");
  output("  $0,\n", ext_count ? kExtensionsInit : "NULL");
  output("  $0,\n", msg_count);
  output("  $0,\n", enum_count);
  output("  $0,\n", ext_count);
  output("};\n\n");

  output("#include \"upb/port/undef.inc\"\n");
  output("\n");
}

void GenerateFile(const DefPoolPair& pools, upb::FileDefPtr file,
                  Plugin* plugin) {
  Output h_output;
  WriteHeader(pools, file, h_output);
  plugin->AddOutputFile(MiniTableHeaderFilename(file), h_output.output());

  Output c_output;
  WriteMiniTableSource(pools, file, c_output);
  plugin->AddOutputFile(SourceFilename(file), c_output.output());
}

bool ParseOptions(Plugin* plugin) {
  const auto param = ParseGeneratorParameter(plugin->parameter());
  if (!param.empty()) {
    plugin->SetError(absl::Substitute("Unknown parameter: $0", param[0].first));
    return false;
  }

  return true;
}

absl::string_view ToStringView(upb_StringView str) {
  return absl::string_view(str.data, str.size);
}

}  // namespace

}  // namespace generator
}  // namespace upb

int main(int argc, char** argv) {
  upb::generator::DefPoolPair pools;
  upb::generator::Plugin plugin;
  if (!upb::generator::ParseOptions(&plugin)) return 0;
  plugin.GenerateFilesRaw([&](const UPB_DESC(FileDescriptorProto) * file_proto,
                              bool generate) {
    upb::Status status;
    upb::FileDefPtr file = pools.AddFile(file_proto, &status);
    if (!file) {
      absl::string_view name = upb::generator::ToStringView(
          UPB_DESC(FileDescriptorProto_name)(file_proto));
      ABSL_LOG(FATAL) << "Couldn't add file " << name
                      << " to DefPool: " << status.error_message();
    }
    if (generate) upb::generator::GenerateFile(pools, file, &plugin);
  });
  return 0;
}
