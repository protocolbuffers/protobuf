// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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
//     * Neither the name of Google Inc. nor the names of its
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

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/stubs/strutil.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"

// NOTE: src/google/protobuf/compiler/plugin.cc makes use of cerr for some
// error cases, so it seems to be ok to use as a back door for errors.

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

std::string EscapeTrigraphs(absl::string_view to_escape) {
  return absl::StrReplaceAll(to_escape, {{"?", "\\?"}});
}

namespace {

std::string GetZeroEnumNameForFlagType(const FlagType flag_type) {
  switch(flag_type) {
    case FLAGTYPE_DESCRIPTOR_INITIALIZATION:
      return "GPBDescriptorInitializationFlag_None";
    case FLAGTYPE_EXTENSION:
      return "GPBExtensionNone";
    case FLAGTYPE_FIELD:
      return "GPBFieldNone";
    default:
      GOOGLE_LOG(FATAL) << "Can't get here.";
      return "0";
  }
}

std::string GetEnumNameForFlagType(const FlagType flag_type) {
  switch(flag_type) {
    case FLAGTYPE_DESCRIPTOR_INITIALIZATION:
      return "GPBDescriptorInitializationFlags";
    case FLAGTYPE_EXTENSION:
      return "GPBExtensionOptions";
    case FLAGTYPE_FIELD:
      return "GPBFieldFlags";
    default:
      GOOGLE_LOG(FATAL) << "Can't get here.";
      return std::string();
  }
}

std::string HandleExtremeFloatingPoint(std::string val,
                                       bool add_float_suffix) {
  if (val == "nan") {
    return "NAN";
  } else if (val == "inf") {
    return "INFINITY";
  } else if (val == "-inf") {
    return "-INFINITY";
  } else {
    // float strings with ., e or E need to have f appended
    if (add_float_suffix && (val.find('.') != std::string::npos ||
                             val.find('e') != std::string::npos ||
                             val.find('E') != std::string::npos)) {
      val += "f";
    }
    return val;
  }
}

}  // namespace

std::string GetCapitalizedType(const FieldDescriptor* field) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_INT32:
      return "Int32";
    case FieldDescriptor::TYPE_UINT32:
      return "UInt32";
    case FieldDescriptor::TYPE_SINT32:
      return "SInt32";
    case FieldDescriptor::TYPE_FIXED32:
      return "Fixed32";
    case FieldDescriptor::TYPE_SFIXED32:
      return "SFixed32";
    case FieldDescriptor::TYPE_INT64:
      return "Int64";
    case FieldDescriptor::TYPE_UINT64:
      return "UInt64";
    case FieldDescriptor::TYPE_SINT64:
      return "SInt64";
    case FieldDescriptor::TYPE_FIXED64:
      return "Fixed64";
    case FieldDescriptor::TYPE_SFIXED64:
      return "SFixed64";
    case FieldDescriptor::TYPE_FLOAT:
      return "Float";
    case FieldDescriptor::TYPE_DOUBLE:
      return "Double";
    case FieldDescriptor::TYPE_BOOL:
      return "Bool";
    case FieldDescriptor::TYPE_STRING:
      return "String";
    case FieldDescriptor::TYPE_BYTES:
      return "Bytes";
    case FieldDescriptor::TYPE_ENUM:
      return "Enum";
    case FieldDescriptor::TYPE_GROUP:
      return "Group";
    case FieldDescriptor::TYPE_MESSAGE:
      return "Message";
  }

  // Some compilers report reaching end of function even though all cases of
  // the enum are handed in the switch.
  GOOGLE_LOG(FATAL) << "Can't get here.";
  return std::string();
}

ObjectiveCType GetObjectiveCType(FieldDescriptor::Type field_type) {
  switch (field_type) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SFIXED32:
      return OBJECTIVECTYPE_INT32;

    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_FIXED32:
      return OBJECTIVECTYPE_UINT32;

    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_SFIXED64:
      return OBJECTIVECTYPE_INT64;

    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_FIXED64:
      return OBJECTIVECTYPE_UINT64;

    case FieldDescriptor::TYPE_FLOAT:
      return OBJECTIVECTYPE_FLOAT;

    case FieldDescriptor::TYPE_DOUBLE:
      return OBJECTIVECTYPE_DOUBLE;

    case FieldDescriptor::TYPE_BOOL:
      return OBJECTIVECTYPE_BOOLEAN;

    case FieldDescriptor::TYPE_STRING:
      return OBJECTIVECTYPE_STRING;

    case FieldDescriptor::TYPE_BYTES:
      return OBJECTIVECTYPE_DATA;

    case FieldDescriptor::TYPE_ENUM:
      return OBJECTIVECTYPE_ENUM;

    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE:
      return OBJECTIVECTYPE_MESSAGE;
  }

  // Some compilers report reaching end of function even though all cases of
  // the enum are handed in the switch.
  GOOGLE_LOG(FATAL) << "Can't get here.";
  return OBJECTIVECTYPE_INT32;
}

std::string GPBGenericValueFieldName(const FieldDescriptor* field) {
  // Returns the field within the GPBGenericValue union to use for the given
  // field.
  if (field->is_repeated()) {
      return "valueMessage";
  }
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return "valueInt32";
    case FieldDescriptor::CPPTYPE_UINT32:
      return "valueUInt32";
    case FieldDescriptor::CPPTYPE_INT64:
      return "valueInt64";
    case FieldDescriptor::CPPTYPE_UINT64:
      return "valueUInt64";
    case FieldDescriptor::CPPTYPE_FLOAT:
      return "valueFloat";
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return "valueDouble";
    case FieldDescriptor::CPPTYPE_BOOL:
      return "valueBool";
    case FieldDescriptor::CPPTYPE_STRING:
      if (field->type() == FieldDescriptor::TYPE_BYTES) {
        return "valueData";
      } else {
        return "valueString";
      }
    case FieldDescriptor::CPPTYPE_ENUM:
      return "valueEnum";
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return "valueMessage";
  }

  // Some compilers report reaching end of function even though all cases of
  // the enum are handed in the switch.
  GOOGLE_LOG(FATAL) << "Can't get here.";
  return std::string();
}


std::string DefaultValue(const FieldDescriptor* field) {
  // Repeated fields don't have defaults.
  if (field->is_repeated()) {
    return "nil";
  }

  // Switch on cpp_type since we need to know which default_value_* method
  // of FieldDescriptor to call.
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      // gcc and llvm reject the decimal form of kint32min and kint64min.
      if (field->default_value_int32() == INT_MIN) {
        return "-0x80000000";
      }
      return absl::StrCat(field->default_value_int32());
    case FieldDescriptor::CPPTYPE_UINT32:
      return absl::StrCat(field->default_value_uint32()) + "U";
    case FieldDescriptor::CPPTYPE_INT64:
      // gcc and llvm reject the decimal form of kint32min and kint64min.
      if (field->default_value_int64() == LLONG_MIN) {
        return "-0x8000000000000000LL";
      }
      return absl::StrCat(field->default_value_int64()) + "LL";
    case FieldDescriptor::CPPTYPE_UINT64:
      return absl::StrCat(field->default_value_uint64()) + "ULL";
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return HandleExtremeFloatingPoint(
          SimpleDtoa(field->default_value_double()), false);
    case FieldDescriptor::CPPTYPE_FLOAT:
      return HandleExtremeFloatingPoint(
          SimpleFtoa(field->default_value_float()), true);
    case FieldDescriptor::CPPTYPE_BOOL:
      return field->default_value_bool() ? "YES" : "NO";
    case FieldDescriptor::CPPTYPE_STRING: {
      const bool has_default_value = field->has_default_value();
      const std::string& default_string = field->default_value_string();
      if (!has_default_value || default_string.length() == 0) {
        // If the field is defined as being the empty string,
        // then we will just assign to nil, as the empty string is the
        // default for both strings and data.
        return "nil";
      }
      if (field->type() == FieldDescriptor::TYPE_BYTES) {
        // We want constant fields in our data structures so we can
        // declare them as static. To achieve this we cheat and stuff
        // a escaped c string (prefixed with a length) into the data
        // field, and cast it to an (NSData*) so it will compile.
        // The runtime library knows how to handle it.

        // Must convert to a standard byte order for packing length into
        // a cstring.
        uint32_t length = ghtonl(default_string.length());
        std::string bytes((const char*)&length, sizeof(length));
        bytes.append(default_string);
        return "(NSData*)\"" + EscapeTrigraphs(absl::CEscape(bytes)) + "\"";
      } else {
        return "@\"" + EscapeTrigraphs(absl::CEscape(default_string)) + "\"";
      }
    }
    case FieldDescriptor::CPPTYPE_ENUM:
      return EnumValueName(field->default_value_enum());
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return "nil";
  }

  // Some compilers report reaching end of function even though all cases of
  // the enum are handed in the switch.
  GOOGLE_LOG(FATAL) << "Can't get here.";
  return std::string();
}

std::string BuildFlagsString(const FlagType flag_type,
                             const std::vector<std::string>& strings) {
  if (strings.empty()) {
    return GetZeroEnumNameForFlagType(flag_type);
  } else if (strings.size() == 1) {
    return strings[0];
  }
  std::string string("(" + GetEnumNameForFlagType(flag_type) + ")(");
  for (size_t i = 0; i != strings.size(); ++i) {
    if (i > 0) {
      string.append(" | ");
    }
    string.append(strings[i]);
  }
  string.append(")");
  return string;
}

std::string ObjCClass(const std::string& class_name) {
  return std::string("GPBObjCClass(") + class_name + ")";
}

std::string ObjCClassDeclaration(const std::string& class_name) {
  return std::string("GPBObjCClassDeclaration(") + class_name + ");";
}

std::string BuildCommentsString(const SourceLocation& location,
                           bool prefer_single_line) {
  const std::string& comments = location.leading_comments.empty()
                               ? location.trailing_comments
                               : location.leading_comments;
  std::vector<std::string> lines;
  lines = absl::StrSplit(comments, "\n", absl::AllowEmpty());
  while (!lines.empty() && lines.back().empty()) {
    lines.pop_back();
  }
  // If there are no comments, just return an empty string.
  if (lines.empty()) {
    return "";
  }

  std::string prefix;
  std::string suffix;
  std::string final_comments;
  std::string epilogue;

  bool add_leading_space = false;

  if (prefer_single_line && lines.size() == 1) {
    prefix = "/** ";
    suffix = " */\n";
  } else {
    prefix = "* ";
    suffix = "\n";
    final_comments += "/**\n";
    epilogue = " **/\n";
    add_leading_space = true;
  }

  for (size_t i = 0; i < lines.size(); i++) {
    std::string line = absl::StrReplaceAll(
        absl::StripPrefix(lines[i], " "),
        {// HeaderDoc and appledoc use '\' and '@' for markers; escape them.
         {"\\", "\\\\"},
         {"@", "\\@"},
         // Decouple / from * to not have inline comments inside comments.
         {"/*", "/\\*"},
         {"*/", "*\\/"}});
    line = prefix + line;
    absl::StripAsciiWhitespace(&line);
    // If not a one line, need to add the first space before *, as
    // absl::StripAsciiWhitespace would have removed it.
    line = (add_leading_space ? " " : "") + line;
    final_comments += line + suffix;
  }
  final_comments += epilogue;
  return final_comments;
}

TextFormatDecodeData::TextFormatDecodeData() { }

TextFormatDecodeData::~TextFormatDecodeData() { }

void TextFormatDecodeData::AddString(int32_t key,
                                     const std::string& input_for_decode,
                                     const std::string& desired_output) {
  for (std::vector<DataEntry>::const_iterator i = entries_.begin();
       i != entries_.end(); ++i) {
    if (i->first == key) {
      std::cerr << "error: duplicate key (" << key
           << ") making TextFormat data, input: \"" << input_for_decode
           << "\", desired: \"" << desired_output << "\"." << std::endl;
      std::cerr.flush();
      abort();
    }
  }

  const std::string& data = TextFormatDecodeData::DecodeDataForString(
      input_for_decode, desired_output);
  entries_.push_back(DataEntry(key, data));
}

std::string TextFormatDecodeData::Data() const {
  std::ostringstream data_stringstream;

  if (num_entries() > 0) {
    io::OstreamOutputStream data_outputstream(&data_stringstream);
    io::CodedOutputStream output_stream(&data_outputstream);

    output_stream.WriteVarint32(num_entries());
    for (std::vector<DataEntry>::const_iterator i = entries_.begin();
         i != entries_.end(); ++i) {
      output_stream.WriteVarint32(i->first);
      output_stream.WriteString(i->second);
    }
  }

  data_stringstream.flush();
  return data_stringstream.str();
}

namespace {

// Helper to build up the decode data for a string.
class DecodeDataBuilder {
 public:
  DecodeDataBuilder() { Reset(); }

  bool AddCharacter(const char desired, const char input);
  void AddUnderscore() {
    Push();
    need_underscore_ = true;
  }
  std::string Finish() {
    Push();
    return decode_data_;
  }

 private:
  static constexpr uint8_t kAddUnderscore = 0x80;

  static constexpr uint8_t kOpAsIs = 0x00;
  static constexpr uint8_t kOpFirstUpper = 0x40;
  static constexpr uint8_t kOpFirstLower = 0x20;
  static constexpr uint8_t kOpAllUpper = 0x60;

  static constexpr int kMaxSegmentLen = 0x1f;

  void AddChar(const char desired) {
    ++segment_len_;
    is_all_upper_ &= absl::ascii_isupper(desired);
  }

  void Push() {
    uint8_t op = (op_ | segment_len_);
    if (need_underscore_) op |= kAddUnderscore;
    if (op != 0) {
      decode_data_ += (char)op;
    }
    Reset();
  }

  bool AddFirst(const char desired, const char input) {
    if (desired == input) {
      op_ = kOpAsIs;
    } else if (desired == absl::ascii_toupper(input)) {
      op_ = kOpFirstUpper;
    } else if (desired == absl::ascii_tolower(input)) {
      op_ = kOpFirstLower;
    } else {
      // Can't be transformed to match.
      return false;
    }
    AddChar(desired);
    return true;
  }

  void Reset() {
    need_underscore_ = false;
    op_ = 0;
    segment_len_ = 0;
    is_all_upper_ = true;
  }

  bool need_underscore_;
  bool is_all_upper_;
  uint8_t op_;
  int segment_len_;

  std::string decode_data_;
};

bool DecodeDataBuilder::AddCharacter(const char desired, const char input) {
  // If we've hit the max size, push to start a new segment.
  if (segment_len_ == kMaxSegmentLen) {
    Push();
  }
  if (segment_len_ == 0) {
    return AddFirst(desired, input);
  }

  // Desired and input match...
  if (desired == input) {
    // If we aren't transforming it, or we're upper casing it and it is
    // supposed to be uppercase; just add it to the segment.
    if ((op_ != kOpAllUpper) || absl::ascii_isupper(desired)) {
      AddChar(desired);
      return true;
    }

    // Add the current segment, and start the next one.
    Push();
    return AddFirst(desired, input);
  }

  // If we need to uppercase, and everything so far has been uppercase,
  // promote op to AllUpper.
  if ((desired == absl::ascii_toupper(input)) && is_all_upper_) {
    op_ = kOpAllUpper;
    AddChar(desired);
    return true;
  }

  // Give up, push and start a new segment.
  Push();
  return AddFirst(desired, input);
}

// If decode data can't be generated, a directive for the raw string
// is used instead.
std::string DirectDecodeString(const std::string& str) {
  std::string result;
  result += (char)'\0';  // Marker for full string.
  result += str;
  result += (char)'\0';  // End of string.
  return result;
}

}  // namespace

// static
std::string TextFormatDecodeData::DecodeDataForString(
    const std::string& input_for_decode, const std::string& desired_output) {
  if (input_for_decode.empty() || desired_output.empty()) {
    std::cerr << "error: got empty string for making TextFormat data, input: \""
         << input_for_decode << "\", desired: \"" << desired_output << "\"."
         << std::endl;
    std::cerr.flush();
    abort();
  }
  if ((input_for_decode.find('\0') != std::string::npos) ||
      (desired_output.find('\0') != std::string::npos)) {
    std::cerr << "error: got a null char in a string for making TextFormat data,"
         << " input: \"" << absl::CEscape(input_for_decode) << "\", desired: \""
         << absl::CEscape(desired_output) << "\"." << std::endl;
    std::cerr.flush();
    abort();
  }

  DecodeDataBuilder builder;

  // Walk the output building it from the input.
  int x = 0;
  for (int y = 0; y < desired_output.size(); y++) {
    const char d = desired_output[y];
    if (d == '_') {
      builder.AddUnderscore();
      continue;
    }

    if (x >= input_for_decode.size()) {
      // Out of input, no way to encode it, just return a full decode.
      return DirectDecodeString(desired_output);
    }
    if (builder.AddCharacter(d, input_for_decode[x])) {
      ++x;  // Consumed one input
    } else {
      // Couldn't transform for the next character, just return a full decode.
      return DirectDecodeString(desired_output);
    }
  }

  if (x != input_for_decode.size()) {
    // Extra input (suffix from name sanitizing?), just return a full decode.
    return DirectDecodeString(desired_output);
  }

  // Add the end marker.
  return builder.Finish() + (char)'\0';
}

namespace {

class ProtoFrameworkCollector : public LineConsumer {
 public:
  ProtoFrameworkCollector(std::map<std::string, std::string>* inout_proto_file_to_framework_name)
      : map_(inout_proto_file_to_framework_name) {}

  virtual bool ConsumeLine(const absl::string_view& line, std::string* out_error) override;

 private:
  std::map<std::string, std::string>* map_;
};

bool ProtoFrameworkCollector::ConsumeLine(
    const absl::string_view& line, std::string* out_error) {
  int offset = line.find(':');
  if (offset == absl::string_view::npos) {
    *out_error =
        std::string("Framework/proto file mapping line without colon sign: '") +
        std::string(line) + "'.";
    return false;
  }
  absl::string_view framework_name = line.substr(0, offset);
  absl::string_view proto_file_list = line.substr(offset + 1);
  TrimWhitespace(&framework_name);

  int start = 0;
  while (start < proto_file_list.length()) {
    offset = proto_file_list.find(',', start);
    if (offset == absl::string_view::npos) {
      offset = proto_file_list.length();
    }

    absl::string_view proto_file = proto_file_list.substr(start, offset - start);
    TrimWhitespace(&proto_file);
    if (!proto_file.empty()) {
      std::map<std::string, std::string>::iterator existing_entry =
          map_->find(std::string(proto_file));
      if (existing_entry != map_->end()) {
        std::cerr << "warning: duplicate proto file reference, replacing "
                     "framework entry for '"
                  << std::string(proto_file) << "' with '" << std::string(framework_name)
                  << "' (was '" << existing_entry->second << "')." << std::endl;
        std::cerr.flush();
      }

      if (proto_file.find(' ') != absl::string_view::npos) {
        std::cerr << "note: framework mapping file had a proto file with a "
                     "space in, hopefully that isn't a missing comma: '"
                  << std::string(proto_file) << "'" << std::endl;
        std::cerr.flush();
      }

      (*map_)[std::string(proto_file)] = std::string(framework_name);
    }

    start = offset + 1;
  }

  return true;
}

}

ImportWriter::ImportWriter(
    const std::string& generate_for_named_framework,
    const std::string& named_framework_to_proto_path_mappings_path,
    const std::string& runtime_import_prefix, bool include_wkt_imports)
    : generate_for_named_framework_(generate_for_named_framework),
      named_framework_to_proto_path_mappings_path_(
          named_framework_to_proto_path_mappings_path),
      runtime_import_prefix_(runtime_import_prefix),
      include_wkt_imports_(include_wkt_imports),
      need_to_parse_mapping_file_(true) {}

ImportWriter::~ImportWriter() {}

void ImportWriter::AddFile(const FileDescriptor* file,
                           const std::string& header_extension) {
  if (IsProtobufLibraryBundledProtoFile(file)) {
    // The imports of the WKTs are only needed within the library itself,
    // in other cases, they get skipped because the generated code already
    // import GPBProtocolBuffers.h and hence proves them.
    if (include_wkt_imports_) {
      const std::string header_name =
          "GPB" + FilePathBasename(file) + header_extension;
      protobuf_imports_.push_back(header_name);
    }
    return;
  }

  // Lazy parse any mappings.
  if (need_to_parse_mapping_file_) {
    ParseFrameworkMappings();
  }

  std::map<std::string, std::string>::iterator proto_lookup =
      proto_file_to_framework_name_.find(file->name());
  if (proto_lookup != proto_file_to_framework_name_.end()) {
    other_framework_imports_.push_back(
        proto_lookup->second + "/" +
        FilePathBasename(file) + header_extension);
    return;
  }

  if (!generate_for_named_framework_.empty()) {
    other_framework_imports_.push_back(
        generate_for_named_framework_ + "/" +
        FilePathBasename(file) + header_extension);
    return;
  }

  other_imports_.push_back(FilePath(file) + header_extension);
}

void ImportWriter::Print(io::Printer* printer) const {
  bool add_blank_line = false;

  if (!protobuf_imports_.empty()) {
    PrintRuntimeImports(printer, protobuf_imports_, runtime_import_prefix_);
    add_blank_line = true;
  }

  if (!other_framework_imports_.empty()) {
    if (add_blank_line) {
      printer->Print("\n");
    }

    for (std::vector<std::string>::const_iterator iter =
             other_framework_imports_.begin();
         iter != other_framework_imports_.end(); ++iter) {
      printer->Print(
          "#import <$header$>\n",
          "header", *iter);
    }

    add_blank_line = true;
  }

  if (!other_imports_.empty()) {
    if (add_blank_line) {
      printer->Print("\n");
    }

    for (std::vector<std::string>::const_iterator iter = other_imports_.begin();
         iter != other_imports_.end(); ++iter) {
      printer->Print(
          "#import \"$header$\"\n",
          "header", *iter);
    }
  }
}

void ImportWriter::PrintRuntimeImports(
    io::Printer* printer, const std::vector<std::string>& header_to_import,
    const std::string& runtime_import_prefix, bool default_cpp_symbol) {
  // Given an override, use that.
  if (!runtime_import_prefix.empty()) {
    for (const auto& header : header_to_import) {
      printer->Print(
          " #import \"$import_prefix$/$header$\"\n",
          "import_prefix", runtime_import_prefix,
          "header", header);
    }
    return;
  }

  const std::string framework_name(ProtobufLibraryFrameworkName);
  const std::string cpp_symbol(ProtobufFrameworkImportSymbol(framework_name));

  if (default_cpp_symbol) {
    printer->Print(
        "// This CPP symbol can be defined to use imports that match up to the framework\n"
        "// imports needed when using CocoaPods.\n"
        "#if !defined($cpp_symbol$)\n"
        " #define $cpp_symbol$ 0\n"
        "#endif\n"
        "\n",
        "cpp_symbol", cpp_symbol);
  }

  printer->Print(
      "#if $cpp_symbol$\n",
      "cpp_symbol", cpp_symbol);
  for (const auto& header : header_to_import) {
    printer->Print(
        " #import <$framework_name$/$header$>\n",
        "framework_name", framework_name,
        "header", header);
  }
  printer->Print(
      "#else\n");
  for (const auto& header : header_to_import) {
    printer->Print(
        " #import \"$header$\"\n",
        "header", header);
  }
  printer->Print(
      "#endif\n");
}

void ImportWriter::ParseFrameworkMappings() {
  need_to_parse_mapping_file_ = false;
  if (named_framework_to_proto_path_mappings_path_.empty()) {
    return;  // Nothing to do.
  }

  ProtoFrameworkCollector collector(&proto_file_to_framework_name_);
  std::string parse_error;
  if (!ParseSimpleFile(named_framework_to_proto_path_mappings_path_,
                       &collector, &parse_error)) {
    std::cerr << "error parsing " << named_framework_to_proto_path_mappings_path_
         << " : " << parse_error << std::endl;
    std::cerr.flush();
  }
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
