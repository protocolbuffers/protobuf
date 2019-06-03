
#include <memory>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/zero_copy_stream.h"

#include "upbc/generator.h"
#include "upbc/message_layout.h"

namespace protoc = ::google::protobuf::compiler;
namespace protobuf = ::google::protobuf;

static std::string StripExtension(absl::string_view fname) {
  size_t lastdot = fname.find_last_of(".");
  if (lastdot == std::string::npos) {
    return std::string(fname);
  }
  return std::string(fname.substr(0, lastdot));
}

static std::string HeaderFilename(std::string proto_filename) {
  return StripExtension(proto_filename) + ".upb.h";
}

static std::string SourceFilename(std::string proto_filename) {
  return StripExtension(proto_filename) + ".upb.c";
}

static std::string DefHeaderFilename(std::string proto_filename) {
  return StripExtension(proto_filename) + ".upbdefs.h";
}

static std::string DefSourceFilename(std::string proto_filename) {
  return StripExtension(proto_filename) + ".upbdefs.c";
}

class Output {
 public:
  Output(protobuf::io::ZeroCopyOutputStream* stream) : stream_(stream) {}
  ~Output() { stream_->BackUp(size_); }

  template <class... Arg>
  void operator()(absl::string_view format, const Arg&... arg) {
    Write(absl::Substitute(format, arg...));
  }

 private:
  void Write(absl::string_view data) {
    while (!data.empty()) {
      RefreshOutput();
      size_t to_write = std::min(data.size(), size_);
      memcpy(ptr_, data.data(), to_write);
      data.remove_prefix(to_write);
      ptr_ += to_write;
      size_ -= to_write;
    }
  }

  void RefreshOutput() {
    while (size_ == 0) {
      void *ptr;
      int size;
      if (!stream_->Next(&ptr, &size)) {
        fprintf(stderr, "upbc: Failed to write to to output\n");
        abort();
      }
      ptr_ = static_cast<char*>(ptr);
      size_ = size;
    }
  }

  protobuf::io::ZeroCopyOutputStream* stream_;
  char *ptr_ = nullptr;
  size_t size_ = 0;
};

namespace upbc {

class Generator : public protoc::CodeGenerator {
  ~Generator() override {}
  bool Generate(const protobuf::FileDescriptor* file,
                const std::string& parameter, protoc::GeneratorContext* context,
                std::string* error) const override;

};

void AddMessages(const protobuf::Descriptor* message,
                 std::vector<const protobuf::Descriptor*>* messages) {
  messages->push_back(message);
  for (int i = 0; i < message->nested_type_count(); i++) {
    AddMessages(message->nested_type(i), messages);
  }
}

void AddEnums(const protobuf::Descriptor* message,
              std::vector<const protobuf::EnumDescriptor*>* enums) {
  for (int i = 0; i < message->enum_type_count(); i++) {
    enums->push_back(message->enum_type(i));
  }
  for (int i = 0; i < message->nested_type_count(); i++) {
    AddEnums(message->nested_type(i), enums);
  }
}

template <class T>
void SortDefs(std::vector<T>* defs) {
  std::sort(defs->begin(), defs->end(),
            [](T a, T b) { return a->full_name() < b->full_name(); });
}

std::vector<const protobuf::Descriptor*> SortedMessages(
    const protobuf::FileDescriptor* file) {
  std::vector<const protobuf::Descriptor*> messages;
  for (int i = 0; i < file->message_type_count(); i++) {
    AddMessages(file->message_type(i), &messages);
  }
  return messages;
}

std::vector<const protobuf::EnumDescriptor*> SortedEnums(
    const protobuf::FileDescriptor* file) {
  std::vector<const protobuf::EnumDescriptor*> enums;
  for (int i = 0; i < file->enum_type_count(); i++) {
    enums.push_back(file->enum_type(i));
  }
  for (int i = 0; i < file->message_type_count(); i++) {
    AddEnums(file->message_type(i), &enums);
  }
  SortDefs(&enums);
  return enums;
}

std::vector<const protobuf::FieldDescriptor*> FieldNumberOrder(
    const protobuf::Descriptor* message) {
  std::vector<const protobuf::FieldDescriptor*> messages;
  for (int i = 0; i < message->field_count(); i++) {
    messages.push_back(message->field(i));
  }
  std::sort(messages.begin(), messages.end(),
            [](const protobuf::FieldDescriptor* a,
               const protobuf::FieldDescriptor* b) {
              return a->number() < b->number();
            });
  return messages;
}

std::vector<const protobuf::FieldDescriptor*> SortedSubmessages(
    const protobuf::Descriptor* message) {
  std::vector<const protobuf::FieldDescriptor*> ret;
  for (int i = 0; i < message->field_count(); i++) {
    if (message->field(i)->cpp_type() ==
        protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      ret.push_back(message->field(i));
    }
  }
  std::sort(ret.begin(), ret.end(),
            [](const protobuf::FieldDescriptor* a,
               const protobuf::FieldDescriptor* b) {
              return a->message_type()->full_name() <
                     b->message_type()->full_name();
            });
  return ret;
}

std::string ToCIdent(absl::string_view str) {
  return absl::StrReplaceAll(str, {{".", "_"}, {"/", "_"}});
}

std::string DefInitSymbol(const protobuf::FileDescriptor *file) {
  return ToCIdent(file->name()) + "_upbdefinit";
}

std::string ToPreproc(absl::string_view str) {
  return absl::AsciiStrToUpper(ToCIdent(str));
}

std::string EnumValueSymbol(const protobuf::EnumValueDescriptor* value) {
  return ToCIdent(value->full_name());
}

std::string GetSizeInit(const MessageLayout::Size& size) {
  return absl::Substitute("UPB_SIZE($0, $1)", size.size32, size.size64);
}

std::string MessageName(const protobuf::Descriptor* descriptor) {
  return ToCIdent(descriptor->full_name());
}

std::string MessageInit(const protobuf::Descriptor* descriptor) {
  return MessageName(descriptor) + "_msginit";
}

std::string CTypeInternal(const protobuf::FieldDescriptor* field,
                          bool is_const) {
  std::string maybe_const = is_const ? "const " : "";
  switch (field->cpp_type()) {
    case protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
      std::string maybe_struct =
          field->file() != field->message_type()->file() ? "struct " : "";
      return maybe_const + maybe_struct + MessageName(field->message_type()) +
             "*";
    }
    case protobuf::FieldDescriptor::CPPTYPE_BOOL:
      return "bool";
    case protobuf::FieldDescriptor::CPPTYPE_FLOAT:
      return "float";
    case protobuf::FieldDescriptor::CPPTYPE_INT32:
    case protobuf::FieldDescriptor::CPPTYPE_ENUM:
      return "int32_t";
    case protobuf::FieldDescriptor::CPPTYPE_UINT32:
      return "uint32_t";
    case protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
      return "double";
    case protobuf::FieldDescriptor::CPPTYPE_INT64:
      return "int64_t";
    case protobuf::FieldDescriptor::CPPTYPE_UINT64:
      return "uint64_t";
    case protobuf::FieldDescriptor::CPPTYPE_STRING:
      return "upb_strview";
    default:
      fprintf(stderr, "Unexpected type");
      abort();
  }
}

std::string UpbType(const protobuf::FieldDescriptor* field) {
  switch (field->cpp_type()) {
    case protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
      return "UPB_TYPE_MESSAGE";
    case protobuf::FieldDescriptor::CPPTYPE_ENUM:
      return "UPB_TYPE_ENUM";
    case protobuf::FieldDescriptor::CPPTYPE_BOOL:
      return "UPB_TYPE_BOOL";
    case protobuf::FieldDescriptor::CPPTYPE_FLOAT:
      return "UPB_TYPE_FLOAT";
    case protobuf::FieldDescriptor::CPPTYPE_INT32:
      return "UPB_TYPE_INT32";
    case protobuf::FieldDescriptor::CPPTYPE_UINT32:
      return "UPB_TYPE_UINT32";
    case protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
      return "UPB_TYPE_DOUBLE";
    case protobuf::FieldDescriptor::CPPTYPE_INT64:
      return "UPB_TYPE_INT64";
    case protobuf::FieldDescriptor::CPPTYPE_UINT64:
      return "UPB_TYPE_UINT64";
    case protobuf::FieldDescriptor::CPPTYPE_STRING:
      return "UPB_TYPE_STRING";
    default:
      fprintf(stderr, "Unexpected type");
      abort();
  }
}

std::string FieldDefault(const protobuf::FieldDescriptor* field) {
  switch (field->cpp_type()) {
    case protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
      return "NULL";
    case protobuf::FieldDescriptor::CPPTYPE_STRING:
      return absl::Substitute("upb_strview_make(\"$0\", strlen(\"$0\"))",
                              absl::CEscape(field->default_value_string()));
    case protobuf::FieldDescriptor::CPPTYPE_INT32:
      return absl::StrCat(field->default_value_int32());
    case protobuf::FieldDescriptor::CPPTYPE_INT64:
      return absl::StrCat(field->default_value_int64());
    case protobuf::FieldDescriptor::CPPTYPE_UINT32:
      return absl::StrCat(field->default_value_uint32());
    case protobuf::FieldDescriptor::CPPTYPE_UINT64:
      return absl::StrCat(field->default_value_uint64());
    case protobuf::FieldDescriptor::CPPTYPE_FLOAT:
      return absl::StrCat(field->default_value_float());
    case protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
      return absl::StrCat(field->default_value_double());
    case protobuf::FieldDescriptor::CPPTYPE_BOOL:
      return field->default_value_bool() ? "true" : "false";
    case protobuf::FieldDescriptor::CPPTYPE_ENUM:
      return EnumValueSymbol(field->default_value_enum());
  }
  ABSL_ASSERT(false);
  return "XXX";
}

std::string CType(const protobuf::FieldDescriptor* field) {
  return CTypeInternal(field, false);
}

std::string CTypeConst(const protobuf::FieldDescriptor* field) {
  return CTypeInternal(field, true);
}

void DumpEnumValues(const protobuf::EnumDescriptor* desc, Output& output) {
  std::vector<const protobuf::EnumValueDescriptor*> values;
  for (int i = 0; i < desc->value_count(); i++) {
    values.push_back(desc->value(i));
  }
  std::sort(values.begin(), values.end(),
            [](const protobuf::EnumValueDescriptor* a,
               const protobuf::EnumValueDescriptor* b) {
              return a->number() < b->number();
            });

  for (size_t i = 0; i < values.size(); i++) {
    auto value = values[i];
    output("  $0 = $1", EnumValueSymbol(value), value->number());
    if (i != values.size() - 1) {
      output(",");
    }
    output("\n");
  }
}

void EmitFileWarning(const protobuf::FileDescriptor* file, Output& output) {
  output(
      "/* This file was generated by upbc (the upb compiler) from the input\n"
      " * file:\n"
      " *\n"
      " *     $0\n"
      " *\n"
      " * Do not edit -- your changes will be discarded when the file is\n"
      " * regenerated. */\n\n",
      file->name());
}

void GenerateMessageInHeader(const protobuf::Descriptor* message, Output& output) {
  MessageLayout layout(message);

  output("/* $0 */\n\n", message->full_name());
  std::string msgname = ToCIdent(message->full_name());
  output(
      "UPB_INLINE $0 *$0_new(upb_arena *arena) {\n"
      "  return ($0 *)upb_msg_new(&$1, arena);\n"
      "}\n"
      "UPB_INLINE $0 *$0_parse(const char *buf, size_t size,\n"
      "                        upb_arena *arena) {\n"
      "  $0 *ret = $0_new(arena);\n"
      "  return (ret && upb_decode(buf, size, ret, &$1, arena)) ? ret : NULL;\n"
      "}\n"
      "UPB_INLINE char *$0_serialize(const $0 *msg, upb_arena *arena, size_t "
      "*len) {\n"
      "  return upb_encode(msg, &$1, arena, len);\n"
      "}\n"
      "\n",
      MessageName(message), MessageInit(message));

  for (int i = 0; i < message->oneof_decl_count(); i++) {
    const protobuf::OneofDescriptor* oneof = message->oneof_decl(i);
    std::string fullname = ToCIdent(oneof->full_name());
    output("typedef enum {\n");
    for (int j = 0; j < oneof->field_count(); j++) {
      const protobuf::FieldDescriptor* field = oneof->field(j);
      output("  $0_$1 = $2,\n", fullname, field->name(), field->number());
    }
    output(
        "  $0_NOT_SET = 0\n"
        "} $0_oneofcases;\n",
        fullname);
    output(
        "UPB_INLINE $0_oneofcases $1_$2_case(const $1* msg) { "
        "return ($0_oneofcases)UPB_FIELD_AT(msg, int32_t, $3); }\n"
        "\n",
        fullname, msgname, oneof->name(),
        GetSizeInit(layout.GetOneofCaseOffset(oneof)));
  }

  for (auto field : FieldNumberOrder(message)) {

    if (layout.HasHasbit(field)) {
      output(
          "UPB_INLINE bool $0_has_$1(const $0 *msg) { "
          "return _upb_has_field(msg, $2); }\n",
          msgname, field->name(), layout.GetHasbitIndex(field));
    } else if (field->containing_oneof()) {
      output(
          "UPB_INLINE bool $0_has_$1(const $0 *msg) { "
          "return _upb_has_oneof_field(msg, $2, $3); }\n",
          msgname, field->name(),
          GetSizeInit(layout.GetOneofCaseOffset(field->containing_oneof())),
          field->number());
    }

    if (field->is_repeated()) {
      output(
          "UPB_INLINE $0 const* $1_$2(const $1 *msg, size_t *len) { "
          "return ($0 const*)_upb_array_accessor(msg, $3, len); }\n",
          CTypeConst(field), msgname, field->name(),
          GetSizeInit(layout.GetFieldOffset(field)));
    } else if (field->containing_oneof()) {
      output(
          "UPB_INLINE $0 $1_$2(const $1 *msg) { "
          "return UPB_READ_ONEOF(msg, $0, $3, $4, $5, $6); }\n",
          CTypeConst(field), msgname, field->name(),
          GetSizeInit(layout.GetFieldOffset(field)),
          GetSizeInit(layout.GetOneofCaseOffset(field->containing_oneof())),
          field->number(), FieldDefault(field));
    } else {
      output(
          "UPB_INLINE $0 $1_$2(const $1 *msg) { "
          "return UPB_FIELD_AT(msg, $0, $3); }\n",
          CTypeConst(field), msgname, field->name(),
          GetSizeInit(layout.GetFieldOffset(field)));
    }
  }

  output("\n");

  for (auto field : FieldNumberOrder(message)) {
    if (field->is_repeated()) {
      output(
          "UPB_INLINE $0* $1_mutable_$2($1 *msg, size_t *len) {\n"
          "  return ($0*)_upb_array_mutable_accessor(msg, $3, len);\n"
          "}\n",
          CType(field), msgname, field->name(),
          GetSizeInit(layout.GetFieldOffset(field)));
      output(
          "UPB_INLINE $0* $1_resize_$2($1 *msg, size_t len, "
          "upb_arena *arena) {\n"
          "  return ($0*)_upb_array_resize_accessor(msg, $3, len, $4, $5, "
          "arena);\n"
          "}\n",
          CType(field), msgname, field->name(),
          GetSizeInit(layout.GetFieldOffset(field)),
          GetSizeInit(MessageLayout::SizeOfUnwrapped(field).size),
          UpbType(field));
      if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        output(
            "UPB_INLINE struct $0* $1_add_$2($1 *msg, upb_arena *arena) {\n"
            "  struct $0* sub = (struct $0*)upb_msg_new(&$3, arena);\n"
            "  bool ok = _upb_array_append_accessor(\n"
            "      msg, $4, $5, $6, &sub, arena);\n"
            "  if (!ok) return NULL;\n"
            "  return sub;\n"
            "}\n",
            MessageName(field->message_type()), msgname, field->name(),
            MessageInit(field->message_type()),
            GetSizeInit(layout.GetFieldOffset(field)),
            GetSizeInit(MessageLayout::SizeOfUnwrapped(field).size),
            UpbType(field));
      } else {
        output(
            "UPB_INLINE bool $1_add_$2($1 *msg, $0 val, upb_arena *arena) {\n"
            "  return _upb_array_append_accessor(\n"
            "      msg, $3, $4, $5, &val, arena);\n"
            "}\n",
            CType(field), msgname, field->name(),
            GetSizeInit(layout.GetFieldOffset(field)),
            GetSizeInit(MessageLayout::SizeOfUnwrapped(field).size),
            UpbType(field));
      }
    } else {
      output("UPB_INLINE void $0_set_$1($0 *msg, $2 value) {\n", msgname,
             field->name(), CType(field));
      if (field->containing_oneof()) {
        output(
            "  UPB_WRITE_ONEOF(msg, $0, $1, value, $2, $3);\n"
            "}\n",
            CType(field), GetSizeInit(layout.GetFieldOffset(field)),
            GetSizeInit(layout.GetOneofCaseOffset(field->containing_oneof())),
            field->number());
      } else {
        if (MessageLayout::HasHasbit(field)) {
          output("  _upb_sethas(msg, $0);\n", layout.GetHasbitIndex(field));
        }
        output(
            "  UPB_FIELD_AT(msg, $0, $1) = value;\n"
            "}\n",
            CType(field), GetSizeInit(layout.GetFieldOffset(field)));
      }
      if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        output(
            "UPB_INLINE struct $0* $1_mutable_$2($1 *msg, upb_arena *arena) {\n"
            "  struct $0* sub = (struct $0*)$1_$2(msg);\n"
            "  if (sub == NULL) {\n"
            "    sub = (struct $0*)upb_msg_new(&$3, arena);\n"
            "    if (!sub) return NULL;\n"
            "    $1_set_$2(msg, sub);\n"
            "  }\n"
            "  return sub;\n"
            "}\n",
            MessageName(field->message_type()), msgname, field->name(),
            MessageInit(field->message_type()));
      }
    }
  }

  output("\n");
}

void WriteHeader(const protobuf::FileDescriptor* file, Output& output) {
  EmitFileWarning(file, output);
  output(
      "#ifndef $0_UPB_H_\n"
      "#define $0_UPB_H_\n\n"
      "#include \"upb/generated_util.h\"\n"
      "#include \"upb/msg.h\"\n"
      "#include \"upb/decode.h\"\n"
      "#include \"upb/encode.h\"\n\n",
      ToPreproc(file->name()));

  for (int i = 0; i < file->public_dependency_count(); i++) {
    const auto& name = file->public_dependency(i)->name();
    if (i == 0) {
      output("/* Public Imports. */\n");
    }
    output("#include \"$0\"\n", HeaderFilename(name));
    if (i == file->public_dependency_count() - 1) {
      output("\n");
    }
  }

  output(
      "#include \"upb/port_def.inc\"\n"
      "\n"
      "#ifdef __cplusplus\n"
      "extern \"C\" {\n"
      "#endif\n"
      "\n");

  std::vector<const protobuf::Descriptor*> this_file_messages =
      SortedMessages(file);

  // Forward-declare types defined in this file.
  for (auto message : this_file_messages) {
    output("struct $0;\n", ToCIdent(message->full_name()));
  }
  for (auto message : this_file_messages) {
    output("typedef struct $0 $0;\n", ToCIdent(message->full_name()));
  }
  for (auto message : this_file_messages) {
    output("extern const upb_msglayout $0;\n", MessageInit(message));
  }

  // Forward-declare types not in this file, but used as submessages.
  // Order by full name for consistent ordering.
  std::map<std::string, const protobuf::Descriptor*> forward_messages;

  for (auto message : SortedMessages(file)) {
    for (int i = 0; i < message->field_count(); i++) {
      const protobuf::FieldDescriptor* field = message->field(i);
      if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE &&
          field->file() != field->message_type()->file()) {
        forward_messages[field->message_type()->full_name()] =
            field->message_type();
      }
    }
  }
  for (const auto& pair : forward_messages) {
    output("struct $0;\n", MessageName(pair.second));
  }
  for (const auto& pair : forward_messages) {
    output("extern const upb_msglayout $0;\n", MessageInit(pair.second));
  }

  if (!this_file_messages.empty()) {
    output("\n");
  }

  std::vector<const protobuf::EnumDescriptor*> this_file_enums =
      SortedEnums(file);

  for (auto enumdesc : this_file_enums) {
    output("typedef enum {\n");
    DumpEnumValues(enumdesc, output);
    output("} $0;\n\n", ToCIdent(enumdesc->full_name()));
  }

  output("\n");

  for (auto message : this_file_messages) {
    GenerateMessageInHeader(message, output);
  }

  output(
      "#ifdef __cplusplus\n"
      "}  /* extern \"C\" */\n"
      "#endif\n"
      "\n"
      "#include \"upb/port_undef.inc\"\n"
      "\n"
      "#endif  /* $0_UPB_H_ */\n",
      ToPreproc(file->name()));
}

void WriteSource(const protobuf::FileDescriptor* file, Output& output) {
  EmitFileWarning(file, output);

  output(
      "#include <stddef.h>\n"
      "#include \"upb/msg.h\"\n"
      "#include \"$0\"\n",
      HeaderFilename(file->name()));

  for (int i = 0; i < file->dependency_count(); i++) {
    output("#include \"$0\"\n", HeaderFilename(file->dependency(i)->name()));
  }

  output(
      "\n"
      "#include \"upb/port_def.inc\"\n"
      "\n");


  for (auto message : SortedMessages(file)) {
    std::string msgname = ToCIdent(message->full_name());
    std::string fields_array_ref = "NULL";
    std::string submsgs_array_ref = "NULL";
    std::string oneofs_array_ref = "NULL";
    absl::flat_hash_map<const protobuf::Descriptor*, int> submsg_indexes;
    MessageLayout layout(message);
    std::vector<const protobuf::FieldDescriptor*> sorted_submsgs =
        SortedSubmessages(message);

    if (!sorted_submsgs.empty()) {
      // TODO(haberman): could save a little bit of space by only generating a
      // "submsgs" array for every strongly-connected component.
      std::string submsgs_array_name = msgname + "_submsgs";
      submsgs_array_ref = "&" + submsgs_array_name + "[0]";
      output("static const upb_msglayout *const $0[$1] = {\n",
             submsgs_array_name, sorted_submsgs.size());

      int i = 0;
      for (auto submsg : sorted_submsgs) {
        if (submsg_indexes.find(submsg->message_type()) !=
            submsg_indexes.end()) {
          continue;
        }
        output("  &$0,\n", MessageInit(submsg->message_type()));
        submsg_indexes[submsg->message_type()] = i++;
      }

      output("};\n\n");
    }

    std::vector<const protobuf::FieldDescriptor*> field_number_order =
        FieldNumberOrder(message);
    if (!field_number_order.empty()) {
      std::string fields_array_name = msgname + "__fields";
      fields_array_ref = "&" + fields_array_name + "[0]";
      output("static const upb_msglayout_field $0[$1] = {\n",
             fields_array_name, field_number_order.size());
      for (auto field : field_number_order) {
        int submsg_index = 0;
        std::string presence = "0";

        if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
          submsg_index = submsg_indexes[field->message_type()];
        }

        if (MessageLayout::HasHasbit(field)) {
          presence = absl::StrCat(layout.GetHasbitIndex(field));
        } else if (field->containing_oneof()) {
          MessageLayout::Size case_offset =
              layout.GetOneofCaseOffset(field->containing_oneof());

          // Our encoding that distinguishes oneofs from presence-having fields.
          case_offset.size32 = -case_offset.size32 - 1;
          case_offset.size64 = -case_offset.size64 - 1;
          presence = GetSizeInit(case_offset);
        }

        output("  {$0, $1, $2, $3, $4, $5},\n",
               field->number(),
               GetSizeInit(layout.GetFieldOffset(field)),
               presence,
               submsg_index,
               field->type(),
               field->label());
      }
      output("};\n\n");
    }

    output("const upb_msglayout $0 = {\n", MessageInit(message));
    output("  $0,\n", submsgs_array_ref);
    output("  $0,\n", fields_array_ref);
    output("  $0, $1, $2,\n", GetSizeInit(layout.message_size()),
           field_number_order.size(),
           "false"  // TODO: extendable
    );

    output("};\n\n");
  }

  output("#include \"upb/port_undef.inc\"\n");
  output("\n");
}

void GenerateMessageDefAccessor(const protobuf::Descriptor* d, Output& output) {
  output("UPB_INLINE const upb_msgdef *$0_getmsgdef(upb_symtab *s) {\n",
         ToCIdent(d->full_name()));
  output("  _upb_symtab_loaddefinit(s, &$0);\n", DefInitSymbol(d->file()));
  output("  return upb_symtab_lookupmsg(s, \"$0\");\n", d->full_name());
  output("}\n");
  output("\n");

  for (int i = 0; i < d->nested_type_count(); i++) {
    GenerateMessageDefAccessor(d->nested_type(i), output);
  }
}

void WriteDefHeader(const protobuf::FileDescriptor* file, Output& output) {
  EmitFileWarning(file, output);

  output(
      "#ifndef $0_UPBDEFS_H_\n"
      "#define $0_UPBDEFS_H_\n\n"
      "#include \"upb/def.h\"\n"
      "#include \"upb/port_def.inc\"\n"
      "#ifdef __cplusplus\n"
      "extern \"C\" {\n"
      "#endif\n\n",
      ToPreproc(file->name()));

  output("#include \"upb/def.h\"\n");
  output("\n");
  output("#include \"upb/port_def.inc\"\n");
  output("\n");

  output("extern upb_def_init $0;\n", DefInitSymbol(file));
  output("\n");

  for (int i = 0; i < file->message_type_count(); i++) {
    GenerateMessageDefAccessor(file->message_type(i), output);
  }

  output(
      "#ifdef __cplusplus\n"
      "}  /* extern \"C\" */\n"
      "#endif\n"
      "\n"
      "#include \"upb/port_undef.inc\"\n"
      "\n"
      "#endif  /* $0_UPBDEFS_H_ */\n",
      ToPreproc(file->name()));
}

// Escape C++ trigraphs by escaping question marks to \?
std::string EscapeTrigraphs(absl::string_view to_escape) {
  return absl::StrReplaceAll(to_escape, {{"?", "\\?"}});
}

void WriteDefSource(const protobuf::FileDescriptor* file, Output& output) {
  EmitFileWarning(file, output);

  output("#include \"upb/def.h\"\n");
  output("\n");

  for (int i = 0; i < file->dependency_count(); i++) {
    output("extern upb_def_init $0;\n", DefInitSymbol(file->dependency(i)));
  }

  protobuf::FileDescriptorProto file_proto;
  file->CopyTo(&file_proto);
  std::string file_data;
  file_proto.SerializeToString(&file_data);

  output("static const char descriptor[$0] =\n", file_data.size());

  {
    if (file_data.size() > 65535) {
      // Workaround for MSVC: "Error C1091: compiler limit: string exceeds
      // 65535 bytes in length". Declare a static array of chars rather than
      // use a string literal. Only write 25 bytes per line.
      static const size_t kBytesPerLine = 25;
      output("{ ");
      for (size_t i = 0; i < file_data.size();) {
        for (size_t j = 0; j < kBytesPerLine && i < file_data.size(); ++i, ++j) {
          output("'$0', ", absl::CEscape(file_data.substr(i, 1)));
        }
        output("\n");
      }
      output("'\\0' }");  // null-terminate
    } else {
      // Only write 40 bytes per line.
      static const size_t kBytesPerLine = 40;
      for (size_t i = 0; i < file_data.size(); i += kBytesPerLine) {
        output(
            "\"$0\"\n",
            EscapeTrigraphs(absl::CEscape(file_data.substr(i, kBytesPerLine))));
      }
    }
    output(";\n");
  }

  output("static upb_def_init *deps[$0] = {\n", file->dependency_count() + 1);
  for (int i = 0; i < file->dependency_count(); i++) {
    output("  &$0,\n", DefInitSymbol(file->dependency(i)));
  }
  output("  NULL\n");
  output("};\n");

  output("upb_def_init $0 = {\n", DefInitSymbol(file));
  output("  deps,\n");
  output("  \"$0\",\n", file->name());
  output("  UPB_STRVIEW_INIT(descriptor, $0)\n", file_data.size());
  output("};\n");
}

bool Generator::Generate(const protobuf::FileDescriptor* file,
                         const std::string& parameter,
                         protoc::GeneratorContext* context,
                         std::string* error) const {
  Output h_output(context->Open(HeaderFilename(file->name())));
  WriteHeader(file, h_output);

  Output c_output(context->Open(SourceFilename(file->name())));
  WriteSource(file, c_output);

  Output h_def_output(context->Open(DefHeaderFilename(file->name())));
  WriteDefHeader(file, h_def_output);

  Output c_def_output(context->Open(DefSourceFilename(file->name())));
  WriteDefSource(file, c_def_output);

  return true;
}

std::unique_ptr<google::protobuf::compiler::CodeGenerator> GetGenerator() {
  return std::unique_ptr<google::protobuf::compiler::CodeGenerator>(
      new Generator());
}

}  // namespace upbc
