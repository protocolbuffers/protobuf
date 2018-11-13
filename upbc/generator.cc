
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "absl/strings/ascii.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/descriptor.h"
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
  //SortDefs(&messages);
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

std::string ToPreproc(absl::string_view str) {
  return absl::AsciiStrToUpper(ToCIdent(str));
}

std::string EnumValueSymbol(const protobuf::EnumValueDescriptor* value) {
  return ToCIdent(value->full_name());
}

std::string GetSizeInit(const MessageLayout::Size& size) {
  return absl::Substitute("UPB_SIZE($0, $1)", size.size32, size.size64);
}

std::string CTypeInternal(const protobuf::FieldDescriptor* field,
                          bool is_const) {
  std::string maybe_const = is_const ? "const " : "";
  if (field->label() == protobuf::FieldDescriptor::LABEL_REPEATED) {
    return maybe_const + "upb_array*";
  }

  switch (field->cpp_type()) {
    case protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
      std::string maybe_struct =
          field->file() != field->message_type()->file() ? "struct " : "";
      return maybe_const + maybe_struct +
             ToCIdent(field->message_type()->full_name()) + "*";
    }
    case protobuf::FieldDescriptor::CPPTYPE_ENUM:
      return ToCIdent(field->enum_type()->full_name());
    case protobuf::FieldDescriptor::CPPTYPE_BOOL:
      return "bool";
    case protobuf::FieldDescriptor::CPPTYPE_FLOAT:
      return "float";
    case protobuf::FieldDescriptor::CPPTYPE_INT32:
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
      return "upb_stringview";
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
      return absl::Substitute("upb_stringview_make(\"$0\", strlen(\"$0\"))",
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
      "extern const upb_msglayout $0_msginit;\n"
      "UPB_INLINE $0 *$0_new(upb_arena *arena) {\n"
      "  return upb_msg_new(&$0_msginit, arena);\n"
      "}\n"
      "UPB_INLINE $0 *$0_parsenew(upb_stringview buf, upb_arena *arena) {\n"
      "  $0 *ret = $0_new(arena);\n"
      "  return (ret && upb_decode(buf, ret, &$0_msginit)) ? ret : NULL;\n"
      "}\n"
      "UPB_INLINE char *$0_serialize(const $0 *msg, upb_arena *arena, size_t "
      "*len) {\n"
      "  return upb_encode(msg, &$0_msginit, arena, len);\n"
      "}\n"
      "\n",
      msgname);

  for (int i = 0; i < message->oneof_decl_count(); i++) {
    const protobuf::OneofDescriptor* oneof = message->oneof_decl(i);
    std::string fullname = ToCIdent(oneof->full_name());
    output("typedef enum {\n");
    for (int i = 0; i < oneof->field_count(); i++) {
      const protobuf::FieldDescriptor* field = oneof->field(i);
      output("  $0_$1 = $2,\n", fullname, field->name(), field->number());
    }
    output(
        "  $0_NOT_SET = 0,\n"
        "} $0_oneofcases;\n",
        fullname);
    output(
        "UPB_INLINE $0_oneofcases $1_$2_case(const $1* msg) { "
        "return UPB_FIELD_AT(msg, int, $3); }\n"
        "\n",
        fullname, msgname, oneof->name(),
        GetSizeInit(layout.GetOneofCaseOffset(oneof)));
  }

  for (auto field : FieldNumberOrder(message)) {
    output("UPB_INLINE $0 $1_$2(const $1 *msg) {", CTypeConst(field), msgname,
           field->name());
    if (field->containing_oneof()) {
      output(" return UPB_READ_ONEOF(msg, $0, $1, $2, $3, $4); }\n",
             CTypeConst(field), GetSizeInit(layout.GetFieldOffset(field)),
             GetSizeInit(layout.GetOneofCaseOffset(field->containing_oneof())),
             field->number(), FieldDefault(field));
    } else {
      output(" return UPB_FIELD_AT(msg, $0, $1); }\n", CTypeConst(field),
             GetSizeInit(layout.GetFieldOffset(field)));
    }
  }

  output("\n");

  for (auto field : FieldNumberOrder(message)) {
    output("UPB_INLINE void $0_set_$1($0 *msg, $2 value) { ", msgname,
           field->name(), CType(field));
    if (field->containing_oneof()) {
      output("UPB_WRITE_ONEOF(msg, $0, $1, value, $2, $3); }\n", CType(field),
             GetSizeInit(layout.GetFieldOffset(field)),
             GetSizeInit(layout.GetOneofCaseOffset(field->containing_oneof())),
             field->number());
    } else {
      output("UPB_FIELD_AT(msg, $0, $1) = value; }\n", CType(field),
             GetSizeInit(layout.GetFieldOffset(field)));
    }
  }

  output("\n\n");
}

void WriteHeader(const protobuf::FileDescriptor* file, Output& output) {
  EmitFileWarning(file, output);
  output(
      "#ifndef $0_UPB_H_\n"
      "#define $0_UPB_H_\n\n"
      "#include \"upb/msg.h\"\n\n"
      "#include \"upb/decode.h\"\n"
      "#include \"upb/encode.h\"\n"
      "#include \"upb/port_def.inc\"\n"
      "UPB_BEGIN_EXTERN_C\n\n",
      ToPreproc(file->name()));

  // Forward-declare types defined in this file.
  for (auto message : SortedMessages(file)) {
    output("struct $0;\n", ToCIdent(message->full_name()));
  }
  for (auto message : SortedMessages(file)) {
    output("typedef struct $0 $0;\n", ToCIdent(message->full_name()));
  };

  // Forward-declare types not in this file, but used as submessages.
  std::set<std::string> forward_names;
  for (auto message : SortedMessages(file)) {
    for (int i = 0; i < message->field_count(); i++) {
      const protobuf::FieldDescriptor* field = message->field(i);
      if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE &&
          field->file() != message->file()) {
        forward_names.insert(ToCIdent(field->message_type()->full_name()));
      }
    }
  }
  for (const auto& name : forward_names) {
    output("struct $0;\n", name);
  }

  output(
      "\n"
      "/* Enums */\n\n");
  for (auto enumdesc : SortedEnums(file)) {
    output("typedef enum {\n");
    DumpEnumValues(enumdesc, output);
    output("} $0;\n\n", ToCIdent(enumdesc->full_name()));
  }

  for (auto message : SortedMessages(file)) {
    GenerateMessageInHeader(message, output);
  }

  output(
      "UPB_END_EXTERN_C\n"
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
    std::unordered_map<const protobuf::Descriptor*, int> submsg_indexes;
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
        output("  &$0_msginit,\n",
               ToCIdent(submsg->message_type()->full_name()));
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
          presence = absl::StrCat(layout.GetHasbitIndex(field) + 1);
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

    output("const upb_msglayout $0_msginit = {\n", msgname);
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

bool Generator::Generate(const protobuf::FileDescriptor* file,
                         const std::string& parameter,
                         protoc::GeneratorContext* context,
                         std::string* error) const {
  Output h_output(context->Open(HeaderFilename(file->name())));
  WriteHeader(file, h_output);

  Output c_output(context->Open(SourceFilename(file->name())));
  WriteSource(file, c_output);

  return true;
}

std::unique_ptr<google::protobuf::compiler::CodeGenerator> GetGenerator() {
  return std::unique_ptr<google::protobuf::compiler::CodeGenerator>(
      new Generator());
}

}  // namespace upbc
