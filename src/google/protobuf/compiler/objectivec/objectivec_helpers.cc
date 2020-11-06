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

#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <climits>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <unordered_set>
#include <vector>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/objectivec/objectivec_helpers.h>
#include <google/protobuf/compiler/objectivec/objectivec_nsobject_methods.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/io_win32.h>
#include <google/protobuf/port.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/strutil.h>

// NOTE: src/google/protobuf/compiler/plugin.cc makes use of cerr for some
// error cases, so it seems to be ok to use as a back door for errors.

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

// <io.h> is transitively included in this file. Import the functions explicitly
// in this port namespace to avoid ambiguous definition.
namespace posix {
#ifdef _WIN32
using ::google::protobuf::io::win32::open;
#else
using ::open;
#endif
}  // namespace port

Options::Options() {
  // Default is the value of the env for the package prefixes.
  const char* file_path = getenv("GPB_OBJC_EXPECTED_PACKAGE_PREFIXES");
  if (file_path) {
    expected_prefixes_path = file_path;
  }
  const char* suppressions = getenv("GPB_OBJC_EXPECTED_PACKAGE_PREFIXES_SUPPRESSIONS");
  if (suppressions) {
    expected_prefixes_suppressions =
        Split(suppressions, ";", true);
  }
}

namespace {

std::unordered_set<std::string> MakeWordsMap(const char* const words[],
                                             size_t num_words) {
  std::unordered_set<std::string> result;
  for (int i = 0; i < num_words; i++) {
    result.insert(words[i]);
  }
  return result;
}

const char* const kUpperSegmentsList[] = {"url", "http", "https"};

std::unordered_set<std::string> kUpperSegments =
    MakeWordsMap(kUpperSegmentsList, GOOGLE_ARRAYSIZE(kUpperSegmentsList));

bool ascii_isnewline(char c) {
  return c == '\n' || c == '\r';
}

// Internal helper for name handing.
// Do not expose this outside of helpers, stick to having functions for specific
// cases (ClassName(), FieldName()), so there is always consistent suffix rules.
std::string UnderscoresToCamelCase(const std::string& input,
                                   bool first_capitalized) {
  std::vector<std::string> values;
  std::string current;

  bool last_char_was_number = false;
  bool last_char_was_lower = false;
  bool last_char_was_upper = false;
  for (int i = 0; i < input.size(); i++) {
    char c = input[i];
    if (ascii_isdigit(c)) {
      if (!last_char_was_number) {
        values.push_back(current);
        current = "";
      }
      current += c;
      last_char_was_number = last_char_was_lower = last_char_was_upper = false;
      last_char_was_number = true;
    } else if (ascii_islower(c)) {
      // lowercase letter can follow a lowercase or uppercase letter
      if (!last_char_was_lower && !last_char_was_upper) {
        values.push_back(current);
        current = "";
      }
      current += c;  // already lower
      last_char_was_number = last_char_was_lower = last_char_was_upper = false;
      last_char_was_lower = true;
    } else if (ascii_isupper(c)) {
      if (!last_char_was_upper) {
        values.push_back(current);
        current = "";
      }
      current += ascii_tolower(c);
      last_char_was_number = last_char_was_lower = last_char_was_upper = false;
      last_char_was_upper = true;
    } else {
      last_char_was_number = last_char_was_lower = last_char_was_upper = false;
    }
  }
  values.push_back(current);

  std::string result;
  bool first_segment_forces_upper = false;
  for (std::vector<std::string>::iterator i = values.begin(); i != values.end();
       ++i) {
    std::string value = *i;
    bool all_upper = (kUpperSegments.count(value) > 0);
    if (all_upper && (result.length() == 0)) {
      first_segment_forces_upper = true;
    }
    for (int j = 0; j < value.length(); j++) {
      if (j == 0 || all_upper) {
        value[j] = ascii_toupper(value[j]);
      } else {
        // Nothing, already in lower.
      }
    }
    result += value;
  }
  if ((result.length() != 0) &&
      !first_capitalized &&
      !first_segment_forces_upper) {
    result[0] = ascii_tolower(result[0]);
  }
  return result;
}

const char* const kReservedWordList[] = {
  // Note NSObject Methods:
  // These are brought in from objectivec_nsobject_methods.h that is generated
  // using method_dump.sh. See kNSObjectMethods below.

  // Objective C "keywords" that aren't in C
  // From
  // http://stackoverflow.com/questions/1873630/reserved-keywords-in-objective-c
  // with some others added on.
  "id", "_cmd", "super", "in", "out", "inout", "bycopy", "byref", "oneway",
  "self", "instancetype", "nullable", "nonnull", "nil", "Nil",
  "YES", "NO", "weak",

  // C/C++ keywords (Incl C++ 0x11)
  // From http://en.cppreference.com/w/cpp/keywords
  "and", "and_eq", "alignas", "alignof", "asm", "auto", "bitand", "bitor",
  "bool", "break", "case", "catch", "char", "char16_t", "char32_t", "class",
  "compl", "const", "constexpr", "const_cast", "continue", "decltype",
  "default", "delete", "double", "dynamic_cast", "else", "enum", "explicit",
  "export", "extern ", "false", "float", "for", "friend", "goto", "if",
  "inline", "int", "long", "mutable", "namespace", "new", "noexcept", "not",
  "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected",
  "public", "register", "reinterpret_cast", "return", "short", "signed",
  "sizeof", "static", "static_assert", "static_cast", "struct", "switch",
  "template", "this", "thread_local", "throw", "true", "try", "typedef",
  "typeid", "typename", "union", "unsigned", "using", "virtual", "void",
  "volatile", "wchar_t", "while", "xor", "xor_eq",

  // C99 keywords
  // From
  // http://publib.boulder.ibm.com/infocenter/lnxpcomp/v8v101/index.jsp?topic=%2Fcom.ibm.xlcpp8l.doc%2Flanguage%2Fref%2Fkeyw.htm
  "restrict",

  // GCC/Clang extension
  "typeof",

  // Not a keyword, but will break you
  "NULL",

  // Objective-C Runtime typedefs
  // From <obc/runtime.h>
  "Category", "Ivar", "Method", "Protocol",

  // GPBMessage Methods
  // Only need to add instance methods that may conflict with
  // method declared in protos. The main cases are methods
  // that take no arguments, or setFoo:/hasFoo: type methods.
  "clear", "data", "delimitedData", "descriptor", "extensionRegistry",
  "extensionsCurrentlySet", "initialized", "isInitialized", "serializedSize",
  "sortedExtensionsInUse", "unknownFields",

  // MacTypes.h names
  "Fixed", "Fract", "Size", "LogicalAddress", "PhysicalAddress", "ByteCount",
  "ByteOffset", "Duration", "AbsoluteTime", "OptionBits", "ItemCount",
  "PBVersion", "ScriptCode", "LangCode", "RegionCode", "OSType",
  "ProcessSerialNumber", "Point", "Rect", "FixedPoint", "FixedRect", "Style",
  "StyleParameter", "StyleField", "TimeScale", "TimeBase", "TimeRecord",
};

// returns true is input starts with __ or _[A-Z] which are reserved identifiers
// in C/ C++. All calls should go through UnderscoresToCamelCase before getting here
// but this verifies and allows for future expansion if we decide to redefine what a
// reserved C identifier is (for example the GNU list
// https://www.gnu.org/software/libc/manual/html_node/Reserved-Names.html )
bool IsReservedCIdentifier(const std::string& input) {
  if (input.length() > 2) {
    if (input.at(0) == '_') {
      if (isupper(input.at(1)) || input.at(1) == '_') {
        return true;
      }
    }
  }
  return false;
}

std::string SanitizeNameForObjC(const std::string& prefix,
                           const std::string& input,
                           const std::string& extension,
                           std::string* out_suffix_added) {
  static const std::unordered_set<std::string> kReservedWords =
      MakeWordsMap(kReservedWordList, GOOGLE_ARRAYSIZE(kReservedWordList));
  static const std::unordered_set<std::string> kNSObjectMethods =
      MakeWordsMap(kNSObjectMethodsList, GOOGLE_ARRAYSIZE(kNSObjectMethodsList));
  std::string sanitized;
  // We add the prefix in the cases where the string is missing a prefix.
  // We define "missing a prefix" as where 'input':
  // a) Doesn't start with the prefix or
  // b) Isn't equivalent to the prefix or
  // c) Has the prefix, but the letter after the prefix is lowercase
  if (HasPrefixString(input, prefix)) {
    if (input.length() == prefix.length() || !ascii_isupper(input[prefix.length()])) {
      sanitized = prefix + input;
    } else {
      sanitized = input;
    }
  } else {
    sanitized = prefix + input;
  }
  if (IsReservedCIdentifier(sanitized) ||
      (kReservedWords.count(sanitized) > 0) ||
      (kNSObjectMethods.count(sanitized) > 0)) {
    if (out_suffix_added) *out_suffix_added = extension;
    return sanitized + extension;
  }
  if (out_suffix_added) out_suffix_added->clear();
  return sanitized;
}

std::string NameFromFieldDescriptor(const FieldDescriptor* field) {
  if (field->type() == FieldDescriptor::TYPE_GROUP) {
    return field->message_type()->name();
  } else {
    return field->name();
  }
}

void PathSplit(const std::string& path, std::string* directory,
               std::string* basename) {
  std::string::size_type last_slash = path.rfind('/');
  if (last_slash == std::string::npos) {
    if (directory) {
      *directory = "";
    }
    if (basename) {
      *basename = path;
    }
  } else {
    if (directory) {
      *directory = path.substr(0, last_slash);
    }
    if (basename) {
      *basename = path.substr(last_slash + 1);
    }
  }
}

bool IsSpecialName(const std::string& name, const std::string* special_names,
                   size_t count) {
  for (size_t i = 0; i < count; ++i) {
    size_t length = special_names[i].length();
    if (name.compare(0, length, special_names[i]) == 0) {
      if (name.length() > length) {
        // If name is longer than the retained_name[i] that it matches
        // the next character must be not lower case (newton vs newTon vs
        // new_ton).
        return !ascii_islower(name[length]);
      } else {
        return true;
      }
    }
  }
  return false;
}

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

}  // namespace

// Escape C++ trigraphs by escaping question marks to \?
std::string EscapeTrigraphs(const std::string& to_escape) {
  return StringReplace(to_escape, "?", "\\?", true);
}

void TrimWhitespace(StringPiece* input) {
  while (!input->empty() && ascii_isspace(*input->data())) {
    input->remove_prefix(1);
  }
  while (!input->empty() && ascii_isspace((*input)[input->length() - 1])) {
    input->remove_suffix(1);
  }
}

bool IsRetainedName(const std::string& name) {
  // List of prefixes from
  // http://developer.apple.com/library/mac/#documentation/Cocoa/Conceptual/MemoryMgmt/Articles/mmRules.html
  static const std::string retained_names[] = {"new", "alloc", "copy",
                                               "mutableCopy"};
  return IsSpecialName(name, retained_names,
                       sizeof(retained_names) / sizeof(retained_names[0]));
}

bool IsInitName(const std::string& name) {
  static const std::string init_names[] = {"init"};
  return IsSpecialName(name, init_names,
                       sizeof(init_names) / sizeof(init_names[0]));
}

std::string BaseFileName(const FileDescriptor* file) {
  std::string basename;
  PathSplit(file->name(), NULL, &basename);
  return basename;
}

std::string FileClassPrefix(const FileDescriptor* file) {
  // Default is empty string, no need to check has_objc_class_prefix.
  std::string result = file->options().objc_class_prefix();
  return result;
}

std::string FilePath(const FileDescriptor* file) {
  std::string output;
  std::string basename;
  std::string directory;
  PathSplit(file->name(), &directory, &basename);
  if (directory.length() > 0) {
    output = directory + "/";
  }
  basename = StripProto(basename);

  // CamelCase to be more ObjC friendly.
  basename = UnderscoresToCamelCase(basename, true);

  output += basename;
  return output;
}

std::string FilePathBasename(const FileDescriptor* file) {
  std::string output;
  std::string basename;
  std::string directory;
  PathSplit(file->name(), &directory, &basename);
  basename = StripProto(basename);

  // CamelCase to be more ObjC friendly.
  output = UnderscoresToCamelCase(basename, true);

  return output;
}

std::string FileClassName(const FileDescriptor* file) {
  const std::string prefix = FileClassPrefix(file);
  const std::string name =
      UnderscoresToCamelCase(StripProto(BaseFileName(file)), true) + "Root";
  // There aren't really any reserved words that end in "Root", but playing
  // it safe and checking.
  return SanitizeNameForObjC(prefix, name, "_RootClass", NULL);
}

std::string ClassNameWorker(const Descriptor* descriptor) {
  std::string name;
  if (descriptor->containing_type() != NULL) {
    name = ClassNameWorker(descriptor->containing_type());
    name += "_";
  }
  return name + descriptor->name();
}

std::string ClassNameWorker(const EnumDescriptor* descriptor) {
  std::string name;
  if (descriptor->containing_type() != NULL) {
    name = ClassNameWorker(descriptor->containing_type());
    name += "_";
  }
  return name + descriptor->name();
}

std::string ClassName(const Descriptor* descriptor) {
  return ClassName(descriptor, NULL);
}

std::string ClassName(const Descriptor* descriptor,
                      std::string* out_suffix_added) {
  // 1. Message names are used as is (style calls for CamelCase, trust it).
  // 2. Check for reserved word at the very end and then suffix things.
  const std::string prefix = FileClassPrefix(descriptor->file());
  const std::string name = ClassNameWorker(descriptor);
  return SanitizeNameForObjC(prefix, name, "_Class", out_suffix_added);
}

std::string EnumName(const EnumDescriptor* descriptor) {
  // 1. Enum names are used as is (style calls for CamelCase, trust it).
  // 2. Check for reserved word at the every end and then suffix things.
  //      message Fixed {
  //        message Size {...}
  //        enum Mumble {...}
  //      ...
  //      }
  //    yields Fixed_Class, Fixed_Size.
  const std::string prefix = FileClassPrefix(descriptor->file());
  const std::string name = ClassNameWorker(descriptor);
  return SanitizeNameForObjC(prefix, name, "_Enum", NULL);
}

std::string EnumValueName(const EnumValueDescriptor* descriptor) {
  // Because of the Switch enum compatibility, the name on the enum has to have
  // the suffix handing, so it slightly diverges from how nested classes work.
  //   enum Fixed {
  //     FOO = 1
  //   }
  // yields Fixed_Enum and Fixed_Enum_Foo (not Fixed_Foo).
  const std::string class_name = EnumName(descriptor->type());
  const std::string value_str =
      UnderscoresToCamelCase(descriptor->name(), true);
  const std::string name = class_name + "_" + value_str;
  // There aren't really any reserved words with an underscore and a leading
  // capital letter, but playing it safe and checking.
  return SanitizeNameForObjC("", name, "_Value", NULL);
}

std::string EnumValueShortName(const EnumValueDescriptor* descriptor) {
  // Enum value names (EnumValueName above) are the enum name turned into
  // a class name and then the value name is CamelCased and concatenated; the
  // whole thing then gets sanitized for reserved words.
  // The "short name" is intended to be the final leaf, the value name; but
  // you can't simply send that off to sanitize as that could result in it
  // getting modified when the full name didn't.  For example enum
  // "StorageModes" has a value "retain".  So the full name is
  // "StorageModes_Retain", but if we sanitize "retain" it would become
  // "RetainValue".
  // So the right way to get the short name is to take the full enum name
  // and then strip off the enum name (leaving the value name and anything
  // done by sanitize).
  const std::string class_name = EnumName(descriptor->type());
  const std::string long_name_prefix = class_name + "_";
  const std::string long_name = EnumValueName(descriptor);
  return StripPrefixString(long_name, long_name_prefix);
}

std::string UnCamelCaseEnumShortName(const std::string& name) {
  std::string result;
  for (int i = 0; i < name.size(); i++) {
    char c = name[i];
    if (i > 0 && ascii_isupper(c)) {
      result += '_';
    }
    result += ascii_toupper(c);
  }
  return result;
}

std::string ExtensionMethodName(const FieldDescriptor* descriptor) {
  const std::string name = NameFromFieldDescriptor(descriptor);
  const std::string result = UnderscoresToCamelCase(name, false);
  return SanitizeNameForObjC("", result, "_Extension", NULL);
}

std::string FieldName(const FieldDescriptor* field) {
  const std::string name = NameFromFieldDescriptor(field);
  std::string result = UnderscoresToCamelCase(name, false);
  if (field->is_repeated() && !field->is_map()) {
    // Add "Array" before do check for reserved worlds.
    result += "Array";
  } else {
    // If it wasn't repeated, but ends in "Array", force on the _p suffix.
    if (HasSuffixString(result, "Array")) {
      result += "_p";
    }
  }
  return SanitizeNameForObjC("", result, "_p", NULL);
}

std::string FieldNameCapitalized(const FieldDescriptor* field) {
  // Want the same suffix handling, so upcase the first letter of the other
  // name.
  std::string result = FieldName(field);
  if (result.length() > 0) {
    result[0] = ascii_toupper(result[0]);
  }
  return result;
}

std::string OneofEnumName(const OneofDescriptor* descriptor) {
  const Descriptor* fieldDescriptor = descriptor->containing_type();
  std::string name = ClassName(fieldDescriptor);
  name += "_" + UnderscoresToCamelCase(descriptor->name(), true) + "_OneOfCase";
  // No sanitize needed because the OS never has names that end in _OneOfCase.
  return name;
}

std::string OneofName(const OneofDescriptor* descriptor) {
  std::string name = UnderscoresToCamelCase(descriptor->name(), false);
  // No sanitize needed because it gets OneOfCase added and that shouldn't
  // ever conflict.
  return name;
}

std::string OneofNameCapitalized(const OneofDescriptor* descriptor) {
  // Use the common handling and then up-case the first letter.
  std::string result = OneofName(descriptor);
  if (result.length() > 0) {
    result[0] = ascii_toupper(result[0]);
  }
  return result;
}

std::string ObjCClass(const std::string& class_name) {
  return std::string("GPBObjCClass(") + class_name + ")";
}

std::string ObjCClassDeclaration(const std::string& class_name) {
  return std::string("GPBObjCClassDeclaration(") + class_name + ");";
}

std::string UnCamelCaseFieldName(const std::string& name, const FieldDescriptor* field) {
  std::string worker(name);
  if (HasSuffixString(worker, "_p")) {
    worker = StripSuffixString(worker, "_p");
  }
  if (field->is_repeated() && HasSuffixString(worker, "Array")) {
    worker = StripSuffixString(worker, "Array");
  }
  if (field->type() == FieldDescriptor::TYPE_GROUP) {
    if (worker.length() > 0) {
      if (ascii_islower(worker[0])) {
        worker[0] = ascii_toupper(worker[0]);
      }
    }
    return worker;
  } else {
    std::string result;
    for (int i = 0; i < worker.size(); i++) {
      char c = worker[i];
      if (ascii_isupper(c)) {
        if (i > 0) {
          result += '_';
        }
        result += ascii_tolower(c);
      } else {
        result += c;
      }
    }
    return result;
  }
}

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

bool IsPrimitiveType(const FieldDescriptor* field) {
  ObjectiveCType type = GetObjectiveCType(field);
  switch (type) {
    case OBJECTIVECTYPE_INT32:
    case OBJECTIVECTYPE_UINT32:
    case OBJECTIVECTYPE_INT64:
    case OBJECTIVECTYPE_UINT64:
    case OBJECTIVECTYPE_FLOAT:
    case OBJECTIVECTYPE_DOUBLE:
    case OBJECTIVECTYPE_BOOLEAN:
    case OBJECTIVECTYPE_ENUM:
      return true;
      break;
    default:
      return false;
  }
}

bool IsReferenceType(const FieldDescriptor* field) {
  return !IsPrimitiveType(field);
}

static std::string HandleExtremeFloatingPoint(std::string val,
                                              bool add_float_suffix) {
  if (val == "nan") {
    return "NAN";
  } else if (val == "inf") {
    return "INFINITY";
  } else if (val == "-inf") {
    return "-INFINITY";
  } else {
    // float strings with ., e or E need to have f appended
    if (add_float_suffix && (val.find(".") != std::string::npos ||
                             val.find("e") != std::string::npos ||
                             val.find("E") != std::string::npos)) {
      val += "f";
    }
    return val;
  }
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
      return StrCat(field->default_value_int32());
    case FieldDescriptor::CPPTYPE_UINT32:
      return StrCat(field->default_value_uint32()) + "U";
    case FieldDescriptor::CPPTYPE_INT64:
      // gcc and llvm reject the decimal form of kint32min and kint64min.
      if (field->default_value_int64() == LLONG_MIN) {
        return "-0x8000000000000000LL";
      }
      return StrCat(field->default_value_int64()) + "LL";
    case FieldDescriptor::CPPTYPE_UINT64:
      return StrCat(field->default_value_uint64()) + "ULL";
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
        uint32 length = ghtonl(default_string.length());
        std::string bytes((const char*)&length, sizeof(length));
        bytes.append(default_string);
        return "(NSData*)\"" + EscapeTrigraphs(CEscape(bytes)) + "\"";
      } else {
        return "@\"" + EscapeTrigraphs(CEscape(default_string)) + "\"";
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

bool HasNonZeroDefaultValue(const FieldDescriptor* field) {
  // Repeated fields don't have defaults.
  if (field->is_repeated()) {
    return false;
  }

  // As much as checking field->has_default_value() seems useful, it isn't
  // because of enums. proto2 syntax allows the first item in an enum (the
  // default) to be non zero. So checking field->has_default_value() would
  // result in missing this non zero default.  See MessageWithOneBasedEnum in
  // objectivec/Tests/unittest_objc.proto for a test Message to confirm this.

  // Some proto file set the default to the zero value, so make sure the value
  // isn't the zero case.
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return field->default_value_int32() != 0;
    case FieldDescriptor::CPPTYPE_UINT32:
      return field->default_value_uint32() != 0U;
    case FieldDescriptor::CPPTYPE_INT64:
      return field->default_value_int64() != 0LL;
    case FieldDescriptor::CPPTYPE_UINT64:
      return field->default_value_uint64() != 0ULL;
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return field->default_value_double() != 0.0;
    case FieldDescriptor::CPPTYPE_FLOAT:
      return field->default_value_float() != 0.0f;
    case FieldDescriptor::CPPTYPE_BOOL:
      return field->default_value_bool();
    case FieldDescriptor::CPPTYPE_STRING: {
      const std::string& default_string = field->default_value_string();
      return default_string.length() != 0;
    }
    case FieldDescriptor::CPPTYPE_ENUM:
      return field->default_value_enum()->number() != 0;
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return false;
  }

  // Some compilers report reaching end of function even though all cases of
  // the enum are handed in the switch.
  GOOGLE_LOG(FATAL) << "Can't get here.";
  return false;
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

std::string BuildCommentsString(const SourceLocation& location,
                           bool prefer_single_line) {
  const std::string& comments = location.leading_comments.empty()
                               ? location.trailing_comments
                               : location.leading_comments;
  std::vector<std::string> lines;
  lines = Split(comments, "\n", false);
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

  for (int i = 0; i < lines.size(); i++) {
    std::string line = StripPrefixString(lines[i], " ");
    // HeaderDoc and appledoc use '\' and '@' for markers; escape them.
    line = StringReplace(line, "\\", "\\\\", true);
    line = StringReplace(line, "@", "\\@", true);
    // Decouple / from * to not have inline comments inside comments.
    line = StringReplace(line, "/*", "/\\*", true);
    line = StringReplace(line, "*/", "*\\/", true);
    line = prefix + line;
    StripWhitespace(&line);
    // If not a one line, need to add the first space before *, as
    // StripWhitespace would have removed it.
    line = (add_leading_space ? " " : "") + line;
    final_comments += line + suffix;
  }
  final_comments += epilogue;
  return final_comments;
}

// Making these a generator option for folks that don't use CocoaPods, but do
// want to put the library in a framework is an interesting question. The
// problem is it means changing sources shipped with the library to actually
// use a different value; so it isn't as simple as a option.
const char* const ProtobufLibraryFrameworkName = "Protobuf";

std::string ProtobufFrameworkImportSymbol(const std::string& framework_name) {
  // GPB_USE_[framework_name]_FRAMEWORK_IMPORTS
  std::string result = std::string("GPB_USE_");
  result += ToUpper(framework_name);
  result += "_FRAMEWORK_IMPORTS";
  return result;
}

bool IsProtobufLibraryBundledProtoFile(const FileDescriptor* file) {
  // We don't check the name prefix or proto package because some files
  // (descriptor.proto), aren't shipped generated by the library, so this
  // seems to be the safest way to only catch the ones shipped.
  const std::string name = file->name();
  if (name == "google/protobuf/any.proto" ||
      name == "google/protobuf/api.proto" ||
      name == "google/protobuf/duration.proto" ||
      name == "google/protobuf/empty.proto" ||
      name == "google/protobuf/field_mask.proto" ||
      name == "google/protobuf/source_context.proto" ||
      name == "google/protobuf/struct.proto" ||
      name == "google/protobuf/timestamp.proto" ||
      name == "google/protobuf/type.proto" ||
      name == "google/protobuf/wrappers.proto") {
    return true;
  }
  return false;
}

bool ReadLine(StringPiece* input, StringPiece* line) {
  for (int len = 0; len < input->size(); ++len) {
    if (ascii_isnewline((*input)[len])) {
      *line = StringPiece(input->data(), len);
      ++len;  // advance over the newline
      *input = StringPiece(input->data() + len, input->size() - len);
      return true;
    }
  }
  return false;  // Ran out of input with no newline.
}

void RemoveComment(StringPiece* input) {
  int offset = input->find('#');
  if (offset != StringPiece::npos) {
    input->remove_suffix(input->length() - offset);
  }
}

namespace {

class ExpectedPrefixesCollector : public LineConsumer {
 public:
  ExpectedPrefixesCollector(std::map<std::string, std::string>* inout_package_to_prefix_map)
      : prefix_map_(inout_package_to_prefix_map) {}

  virtual bool ConsumeLine(const StringPiece& line, std::string* out_error);

 private:
  std::map<std::string, std::string>* prefix_map_;
};

bool ExpectedPrefixesCollector::ConsumeLine(
    const StringPiece& line, std::string* out_error) {
  int offset = line.find('=');
  if (offset == StringPiece::npos) {
    *out_error = std::string("Expected prefixes file line without equal sign: '") +
                 std::string(line) + "'.";
    return false;
  }
  StringPiece package = line.substr(0, offset);
  StringPiece prefix = line.substr(offset + 1);
  TrimWhitespace(&package);
  TrimWhitespace(&prefix);
  // Don't really worry about error checking the package/prefix for
  // being valid.  Assume the file is validated when it is created/edited.
  (*prefix_map_)[std::string(package)] = std::string(prefix);
  return true;
}

bool LoadExpectedPackagePrefixes(const Options& generation_options,
                                 std::map<std::string, std::string>* prefix_map,
                                 std::string* out_error) {
  if (generation_options.expected_prefixes_path.empty()) {
    return true;
  }

  ExpectedPrefixesCollector collector(prefix_map);
  return ParseSimpleFile(
      generation_options.expected_prefixes_path, &collector, out_error);
}

bool ValidateObjCClassPrefix(
    const FileDescriptor* file, const std::string& expected_prefixes_path,
    const std::map<std::string, std::string>& expected_package_prefixes,
    std::string* out_error) {
  const std::string prefix = file->options().objc_class_prefix();
  const std::string package = file->package();

  // NOTE: src/google/protobuf/compiler/plugin.cc makes use of cerr for some
  // error cases, so it seems to be ok to use as a back door for warnings.

  // Check: Error - See if there was an expected prefix for the package and
  // report if it doesn't match (wrong or missing).
  std::map<std::string, std::string>::const_iterator package_match =
      expected_package_prefixes.find(package);
  if (package_match != expected_package_prefixes.end()) {
    // There was an entry, and...
    if (package_match->second == prefix) {
      // ...it matches.  All good, out of here!
      return true;
    } else {
      // ...it didn't match!
      *out_error = "error: Expected 'option objc_class_prefix = \"" +
                   package_match->second + "\";' for package '" + package +
                   "' in '" + file->name() + "'";
      if (prefix.length()) {
        *out_error += "; but found '" + prefix + "' instead";
      }
      *out_error += ".";
      return false;
    }
  }

  // If there was no prefix option, we're done at this point.
  if (prefix.empty()) {
    // No prefix, nothing left to check.
    return true;
  }

  // Check: Warning - Make sure the prefix is is a reasonable value according
  // to Apple's rules (the checks above implicitly whitelist anything that
  // doesn't meet these rules).
  if (!ascii_isupper(prefix[0])) {
    std::cerr
         << "protoc:0: warning: Invalid 'option objc_class_prefix = \""
         << prefix << "\";' in '" << file->name() << "';"
         << " it should start with a capital letter." << std::endl;
    std::cerr.flush();
  }
  if (prefix.length() < 3) {
    // Apple reserves 2 character prefixes for themselves. They do use some
    // 3 character prefixes, but they haven't updated the rules/docs.
    std::cerr
         << "protoc:0: warning: Invalid 'option objc_class_prefix = \""
         << prefix << "\";' in '" << file->name() << "';"
         << " Apple recommends they should be at least 3 characters long."
         << std::endl;
    std::cerr.flush();
  }

  // Look for any other package that uses the same prefix.
  std::string other_package_for_prefix;
  for (std::map<std::string, std::string>::const_iterator i =
           expected_package_prefixes.begin();
       i != expected_package_prefixes.end(); ++i) {
    if (i->second == prefix) {
      other_package_for_prefix = i->first;
      break;
    }
  }

  // Check: Warning - If the file does not have a package, check whether
  // the prefix declared is being used by another package or not.
  if (package.empty()) {
    // The file does not have a package and ...
    if (other_package_for_prefix.empty()) {
      // ... no other package has declared that prefix.
      std::cerr
           << "protoc:0: warning: File '" << file->name() << "' has no "
           << "package. Consider adding a new package to the proto and adding '"
           << "new.package = " << prefix << "' to the expected prefixes file ("
           << expected_prefixes_path << ")." << std::endl;
      std::cerr.flush();
    } else {
      // ... another package has declared the same prefix.
      std::cerr
           << "protoc:0: warning: File '" << file->name() << "' has no package "
           << "and package '" << other_package_for_prefix << "' already uses '"
           << prefix << "' as its prefix. Consider either adding a new package "
           << "to the proto, or reusing one of the packages already using this "
           << "prefix in the expected prefixes file ("
           << expected_prefixes_path << ")." << std::endl;
      std::cerr.flush();
    }
    return true;
  }

  // Check: Error - Make sure the prefix wasn't expected for a different
  // package (overlap is allowed, but it has to be listed as an expected
  // overlap).
  if (!other_package_for_prefix.empty()) {
    *out_error =
        "error: Found 'option objc_class_prefix = \"" + prefix +
        "\";' in '" + file->name() +
        "'; that prefix is already used for 'package " +
        other_package_for_prefix + ";'. It can only be reused by listing " +
        "it in the expected file (" +
        expected_prefixes_path + ").";
    return false;  // Only report first usage of the prefix.
  }

  // Check: Warning - If the given package/prefix pair wasn't expected, issue a
  // warning issue a warning suggesting it gets added to the file.
  if (!expected_package_prefixes.empty()) {
    std::cerr
         << "protoc:0: warning: Found unexpected 'option objc_class_prefix = \""
         << prefix << "\";' in '" << file->name() << "';"
         << " consider adding it to the expected prefixes file ("
         << expected_prefixes_path << ")." << std::endl;
    std::cerr.flush();
  }

  return true;
}

}  // namespace

bool ValidateObjCClassPrefixes(const std::vector<const FileDescriptor*>& files,
                               const Options& generation_options,
                               std::string* out_error) {
  // Load the expected package prefixes, if available, to validate against.
  std::map<std::string, std::string> expected_package_prefixes;
  if (!LoadExpectedPackagePrefixes(generation_options,
                                   &expected_package_prefixes,
                                   out_error)) {
    return false;
  }

  for (int i = 0; i < files.size(); i++) {
    bool should_skip =
      (std::find(generation_options.expected_prefixes_suppressions.begin(),
                 generation_options.expected_prefixes_suppressions.end(),
                 files[i]->name())
          != generation_options.expected_prefixes_suppressions.end());
    if (should_skip) {
      continue;
    }

    bool is_valid =
        ValidateObjCClassPrefix(files[i],
                                generation_options.expected_prefixes_path,
                                expected_package_prefixes,
                                out_error);
    if (!is_valid) {
      return false;
    }
  }
  return true;
}

TextFormatDecodeData::TextFormatDecodeData() { }

TextFormatDecodeData::~TextFormatDecodeData() { }

void TextFormatDecodeData::AddString(int32 key,
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
  static constexpr uint8 kAddUnderscore = 0x80;

  static constexpr uint8 kOpAsIs = 0x00;
  static constexpr uint8 kOpFirstUpper = 0x40;
  static constexpr uint8 kOpFirstLower = 0x20;
  static constexpr uint8 kOpAllUpper = 0x60;

  static constexpr int kMaxSegmentLen = 0x1f;

  void AddChar(const char desired) {
    ++segment_len_;
    is_all_upper_ &= ascii_isupper(desired);
  }

  void Push() {
    uint8 op = (op_ | segment_len_);
    if (need_underscore_) op |= kAddUnderscore;
    if (op != 0) {
      decode_data_ += (char)op;
    }
    Reset();
  }

  bool AddFirst(const char desired, const char input) {
    if (desired == input) {
      op_ = kOpAsIs;
    } else if (desired == ascii_toupper(input)) {
      op_ = kOpFirstUpper;
    } else if (desired == ascii_tolower(input)) {
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
  uint8 op_;
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
    if ((op_ != kOpAllUpper) || ascii_isupper(desired)) {
      AddChar(desired);
      return true;
    }

    // Add the current segment, and start the next one.
    Push();
    return AddFirst(desired, input);
  }

  // If we need to uppercase, and everything so far has been uppercase,
  // promote op to AllUpper.
  if ((desired == ascii_toupper(input)) && is_all_upper_) {
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
         << " input: \"" << CEscape(input_for_decode) << "\", desired: \""
         << CEscape(desired_output) << "\"." << std::endl;
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

class Parser {
 public:
  Parser(LineConsumer* line_consumer)
      : line_consumer_(line_consumer), line_(0) {}

  // Parses a check of input, returning success/failure.
  bool ParseChunk(StringPiece chunk);

  // Should be called to finish parsing (after all input has been provided via
  // ParseChunk()).  Returns success/failure.
  bool Finish();

  int last_line() const { return line_; }
  std::string error_str() const { return error_str_; }

 private:
  bool ParseLoop();

  LineConsumer* line_consumer_;
  int line_;
  std::string error_str_;
  StringPiece p_;
  std::string leftover_;
};

bool Parser::ParseChunk(StringPiece chunk) {
  if (!leftover_.empty()) {
    leftover_ += std::string(chunk);
    p_ = StringPiece(leftover_);
  } else {
    p_ = chunk;
  }
  bool result = ParseLoop();
  if (p_.empty()) {
    leftover_.clear();
  } else {
    leftover_ = std::string(p_);
  }
  return result;
}

bool Parser::Finish() {
  if (leftover_.empty()) {
    return true;
  }
  // Force a newline onto the end to finish parsing.
  leftover_ += "\n";
  p_ = StringPiece(leftover_);
  if (!ParseLoop()) {
    return false;
  }
  return p_.empty();  // Everything used?
}

bool Parser::ParseLoop() {
  StringPiece line;
  while (ReadLine(&p_, &line)) {
    ++line_;
    RemoveComment(&line);
    TrimWhitespace(&line);
    if (line.empty()) {
      continue;  // Blank line.
    }
    if (!line_consumer_->ConsumeLine(line, &error_str_)) {
      return false;
    }
  }
  return true;
}

}  // namespace

LineConsumer::LineConsumer() {}

LineConsumer::~LineConsumer() {}

bool ParseSimpleFile(const std::string& path, LineConsumer* line_consumer,
                     std::string* out_error) {
  int fd;
  do {
    fd = posix::open(path.c_str(), O_RDONLY);
  } while (fd < 0 && errno == EINTR);
  if (fd < 0) {
    *out_error = std::string("error: Unable to open \"") + path + "\", " +
                 strerror(errno);
    return false;
  }
  io::FileInputStream file_stream(fd);
  file_stream.SetCloseOnDelete(true);

  Parser parser(line_consumer);
  const void* buf;
  int buf_len;
  while (file_stream.Next(&buf, &buf_len)) {
    if (buf_len == 0) {
      continue;
    }

    if (!parser.ParseChunk(StringPiece(static_cast<const char*>(buf), buf_len))) {
      *out_error =
          std::string("error: ") + path +
          " Line " + StrCat(parser.last_line()) + ", " + parser.error_str();
      return false;
    }
  }
  return parser.Finish();
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

bool ImportWriter::ProtoFrameworkCollector::ConsumeLine(
    const StringPiece& line, std::string* out_error) {
  int offset = line.find(':');
  if (offset == StringPiece::npos) {
    *out_error =
        std::string("Framework/proto file mapping line without colon sign: '") +
        std::string(line) + "'.";
    return false;
  }
  StringPiece framework_name = line.substr(0, offset);
  StringPiece proto_file_list = line.substr(offset + 1);
  TrimWhitespace(&framework_name);

  int start = 0;
  while (start < proto_file_list.length()) {
    offset = proto_file_list.find(',', start);
    if (offset == StringPiece::npos) {
      offset = proto_file_list.length();
    }

    StringPiece proto_file = proto_file_list.substr(start, offset - start);
    TrimWhitespace(&proto_file);
    if (!proto_file.empty()) {
      std::map<std::string, std::string>::iterator existing_entry =
          map_->find(string(proto_file));
      if (existing_entry != map_->end()) {
        std::cerr << "warning: duplicate proto file reference, replacing "
                     "framework entry for '"
                  << std::string(proto_file) << "' with '" << std::string(framework_name)
                  << "' (was '" << existing_entry->second << "')." << std::endl;
        std::cerr.flush();
      }

      if (proto_file.find(' ') != StringPiece::npos) {
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

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
