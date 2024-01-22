// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/names.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/objectivec/line_consumer.h"
#include "google/protobuf/compiler/objectivec/nsobject_methods.h"
#include "google/protobuf/descriptor.h"

// NOTE: src/google/protobuf/compiler/plugin.cc makes use of cerr for some
// error cases, so it seems to be ok to use as a back door for errors.

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

namespace {

bool BoolFromEnvVar(const char* env_var, bool default_value) {
  const char* value = getenv(env_var);
  if (value) {
    return std::string("YES") == absl::AsciiStrToUpper(value);
  }
  return default_value;
}

class SimpleLineCollector : public LineConsumer {
 public:
  explicit SimpleLineCollector(absl::flat_hash_set<std::string>* inout_set)
      : set_(inout_set) {}

  bool ConsumeLine(absl::string_view line, std::string* out_error) override {
    set_->insert(std::string(line));
    return true;
  }

 private:
  absl::flat_hash_set<std::string>* set_;
};

class PackageToPrefixesCollector : public LineConsumer {
 public:
  PackageToPrefixesCollector(absl::string_view usage,
                             absl::flat_hash_map<std::string, std::string>*
                                 inout_package_to_prefix_map)
      : usage_(usage), prefix_map_(inout_package_to_prefix_map) {}

  bool ConsumeLine(absl::string_view line, std::string* out_error) override;

 private:
  const std::string usage_;
  absl::flat_hash_map<std::string, std::string>* prefix_map_;
};

class PrefixModeStorage {
 public:
  PrefixModeStorage();

  absl::string_view package_to_prefix_mappings_path() const {
    return package_to_prefix_mappings_path_;
  }
  void set_package_to_prefix_mappings_path(absl::string_view path) {
    package_to_prefix_mappings_path_ = std::string(path);
    package_to_prefix_map_.clear();
  }

  absl::string_view prefix_from_proto_package_mappings(
      const FileDescriptor* file);

  bool use_package_name() const { return use_package_name_; }
  void set_use_package_name(bool on_or_off) { use_package_name_ = on_or_off; }

  absl::string_view exception_path() const { return exception_path_; }
  void set_exception_path(absl::string_view path) {
    exception_path_ = std::string(path);
    exceptions_.clear();
  }

  bool is_package_exempted(absl::string_view package);

  // When using a proto package as the prefix, this should be added as the
  // prefix in front of it.
  absl::string_view forced_package_prefix() const { return forced_prefix_; }
  void set_forced_package_prefix(absl::string_view prefix) {
    forced_prefix_ = std::string(prefix);
  }

 private:
  bool use_package_name_;
  absl::flat_hash_map<std::string, std::string> package_to_prefix_map_;
  std::string package_to_prefix_mappings_path_;
  std::string exception_path_;
  std::string forced_prefix_;
  absl::flat_hash_set<std::string> exceptions_;
};

PrefixModeStorage::PrefixModeStorage() {
  // Even thought there are generation options, have an env back door since some
  // of these helpers could be used in other plugins.

  use_package_name_ = BoolFromEnvVar("GPB_OBJC_USE_PACKAGE_AS_PREFIX", false);

  const char* exception_path =
      getenv("GPB_OBJC_PACKAGE_PREFIX_EXCEPTIONS_PATH");
  if (exception_path) {
    exception_path_ = exception_path;
  }

  const char* prefix = getenv("GPB_OBJC_USE_PACKAGE_AS_PREFIX_PREFIX");
  if (prefix) {
    forced_prefix_ = prefix;
  }
}

constexpr absl::string_view kNoPackagePrefix = "no_package:";

absl::string_view PrefixModeStorage::prefix_from_proto_package_mappings(
    const FileDescriptor* file) {
  if (!file) {
    return "";
  }

  if (package_to_prefix_map_.empty() &&
      !package_to_prefix_mappings_path_.empty()) {
    std::string error_str;
    // Re use the same collector as we use for expected_prefixes_path since the
    // file format is the same.
    PackageToPrefixesCollector collector("Package to prefixes",
                                         &package_to_prefix_map_);
    if (!ParseSimpleFile(package_to_prefix_mappings_path_, &collector,
                         &error_str)) {
      if (error_str.empty()) {
        error_str = absl::StrCat("protoc:0: warning: Failed to parse ",
                                 "prefix to proto package mappings file: ",
                                 package_to_prefix_mappings_path_);
      }
      std::cerr << error_str << std::endl;
      std::cerr.flush();
      package_to_prefix_map_.clear();
    }
  }

  const std::string package = file->package();
  // For files without packages, the can be registered as "no_package:PATH",
  // allowing the expected prefixes file.
  const std::string lookup_key =
      package.empty() ? absl::StrCat(kNoPackagePrefix, file->name()) : package;

  auto prefix_lookup = package_to_prefix_map_.find(lookup_key);

  if (prefix_lookup != package_to_prefix_map_.end()) {
    return prefix_lookup->second;
  }

  return "";
}

bool PrefixModeStorage::is_package_exempted(absl::string_view package) {
  if (exceptions_.empty() && !exception_path_.empty()) {
    std::string error_str;
    SimpleLineCollector collector(&exceptions_);
    if (!ParseSimpleFile(exception_path_, &collector, &error_str)) {
      if (error_str.empty()) {
        error_str = std::string("protoc:0: warning: Failed to parse") +
                    std::string(" package prefix exceptions file: ") +
                    exception_path_;
      }
      std::cerr << error_str << std::endl;
      std::cerr.flush();
      exceptions_.clear();
    }

    // If the file was empty put something in it so it doesn't get reloaded over
    // and over.
    if (exceptions_.empty()) {
      exceptions_.insert("<not a real package>");
    }
  }

  return exceptions_.contains(package);
}

PrefixModeStorage& g_prefix_mode = *new PrefixModeStorage();

}  // namespace

absl::string_view GetPackageToPrefixMappingsPath() {
  return g_prefix_mode.package_to_prefix_mappings_path();
}

void SetPackageToPrefixMappingsPath(absl::string_view file_path) {
  g_prefix_mode.set_package_to_prefix_mappings_path(file_path);
}

bool UseProtoPackageAsDefaultPrefix() {
  return g_prefix_mode.use_package_name();
}

void SetUseProtoPackageAsDefaultPrefix(bool on_or_off) {
  g_prefix_mode.set_use_package_name(on_or_off);
}

absl::string_view GetProtoPackagePrefixExceptionList() {
  return g_prefix_mode.exception_path();
}

void SetProtoPackagePrefixExceptionList(absl::string_view file_path) {
  g_prefix_mode.set_exception_path(file_path);
}

absl::string_view GetForcedPackagePrefix() {
  return g_prefix_mode.forced_package_prefix();
}

void SetForcedPackagePrefix(absl::string_view prefix) {
  g_prefix_mode.set_forced_package_prefix(prefix);
}

namespace {

const char* const kUpperSegmentsList[] = {"url", "http", "https"};

const absl::flat_hash_set<absl::string_view>& UpperSegments() {
  static const auto* words = [] {
    auto* words = new absl::flat_hash_set<absl::string_view>();

    for (const auto word : kUpperSegmentsList) {
      words->emplace(word);
    }
    return words;
  }();
  return *words;
}

// Internal helper for name handing.
// Do not expose this outside of helpers, stick to having functions for specific
// cases (ClassName(), FieldName()), so there is always consistent suffix rules.
std::string UnderscoresToCamelCase(absl::string_view input,
                                   bool first_capitalized) {
  std::vector<std::string> values;
  std::string current;

  bool last_char_was_number = false;
  bool last_char_was_lower = false;
  bool last_char_was_upper = false;
  for (int i = 0; i < input.size(); i++) {
    char c = input[i];
    if (absl::ascii_isdigit(c)) {
      if (!last_char_was_number) {
        values.push_back(current);
        current = "";
      }
      current += c;
      last_char_was_number = last_char_was_lower = last_char_was_upper = false;
      last_char_was_number = true;
    } else if (absl::ascii_islower(c)) {
      // lowercase letter can follow a lowercase or uppercase letter
      if (!last_char_was_lower && !last_char_was_upper) {
        values.push_back(current);
        current = "";
      }
      current += c;  // already lower
      last_char_was_number = last_char_was_lower = last_char_was_upper = false;
      last_char_was_lower = true;
    } else if (absl::ascii_isupper(c)) {
      if (!last_char_was_upper) {
        values.push_back(current);
        current = "";
      }
      current += absl::ascii_tolower(c);
      last_char_was_number = last_char_was_lower = last_char_was_upper = false;
      last_char_was_upper = true;
    } else {
      last_char_was_number = last_char_was_lower = last_char_was_upper = false;
    }
  }
  values.push_back(current);

  std::string result;
  bool first_segment_forces_upper = false;
  for (auto& value : values) {
    bool all_upper = UpperSegments().contains(value);
    if (all_upper && (result.empty())) {
      first_segment_forces_upper = true;
    }
    if (all_upper) {
      absl::AsciiStrToUpper(&value);
    } else {
      value[0] = absl::ascii_toupper(value[0]);
    }
    result += value;
  }
  if ((!result.empty()) && !first_capitalized && !first_segment_forces_upper) {
    result[0] = absl::ascii_tolower(result[0]);
  }
  return result;
}

const char* const kReservedWordList[] = {
    // Note NSObject Methods:
    // These are brought in from nsobject_methods.h that is generated
    // using method_dump.sh. See kNSObjectMethods below.

    // Objective-C "keywords" that aren't in C
    // From
    // http://stackoverflow.com/questions/1873630/reserved-keywords-in-objective-c
    // with some others added on.
    "id",
    "_cmd",
    "super",
    "in",
    "out",
    "inout",
    "bycopy",
    "byref",
    "oneway",
    "self",
    "instancetype",
    "nullable",
    "nonnull",
    "nil",
    "Nil",
    "YES",
    "NO",
    "weak",

    // C/C++ keywords (Incl C++ 0x11)
    // From http://en.cppreference.com/w/cpp/keywords
    "and",
    "and_eq",
    "alignas",
    "alignof",
    "asm",
    "auto",
    "bitand",
    "bitor",
    "bool",
    "break",
    "case",
    "catch",
    "char",
    "char16_t",
    "char32_t",
    "class",
    "compl",
    "const",
    "constexpr",
    "const_cast",
    "continue",
    "decltype",
    "default",
    "delete",
    "double",
    "dynamic_cast",
    "else",
    "enum",
    "explicit",
    "export",
    "extern ",
    "false",
    "float",
    "for",
    "friend",
    "goto",
    "if",
    "inline",
    "int",
    "long",
    "mutable",
    "namespace",
    "new",
    "noexcept",
    "not",
    "not_eq",
    "nullptr",
    "operator",
    "or",
    "or_eq",
    "private",
    "protected",
    "public",
    "register",
    "reinterpret_cast",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "static_assert",
    "static_cast",
    "struct",
    "switch",
    "template",
    "this",
    "thread_local",
    "throw",
    "true",
    "try",
    "typedef",
    "typeid",
    "typename",
    "union",
    "unsigned",
    "using",
    "virtual",
    "void",
    "volatile",
    "wchar_t",
    "while",
    "xor",
    "xor_eq",

    // C99 keywords
    // From
    // http://publib.boulder.ibm.com/infocenter/lnxpcomp/v8v101/index.jsp?topic=%2Fcom.ibm.xlcpp8l.doc%2Flanguage%2Fref%2Fkeyw.htm
    "restrict",

    // GCC/Clang extension
    "typeof",

    // Not a keyword, but will break you
    "NULL",

    // C88+ specs call for these to be macros, so depending on what they are
    // defined to be it can lead to odd errors for some Xcode/SDK versions.
    "stdin",
    "stdout",
    "stderr",

    // Objective-C Runtime typedefs
    // From <obc/runtime.h>
    "Category",
    "Ivar",
    "Method",
    "Protocol",

    // GPBMessage Methods
    // Only need to add instance methods that may conflict with
    // method declared in protos. The main cases are methods
    // that take no arguments, or setFoo:/hasFoo: type methods.
    "clear",
    "data",
    "delimitedData",
    "descriptor",
    "extensionRegistry",
    "extensionsCurrentlySet",
    "initialized",
    "isInitialized",
    "serializedSize",
    "sortedExtensionsInUse",
    "unknownFields",

    // MacTypes.h names
    "Fixed",
    "Fract",
    "Size",
    "LogicalAddress",
    "PhysicalAddress",
    "ByteCount",
    "ByteOffset",
    "Duration",
    "AbsoluteTime",
    "OptionBits",
    "ItemCount",
    "PBVersion",
    "ScriptCode",
    "LangCode",
    "RegionCode",
    "OSType",
    "ProcessSerialNumber",
    "Point",
    "Rect",
    "FixedPoint",
    "FixedRect",
    "Style",
    "StyleParameter",
    "StyleField",
    "TimeScale",
    "TimeBase",
    "TimeRecord",
};

const absl::flat_hash_set<absl::string_view>& ReservedWords() {
  static const auto* words = [] {
    auto* words = new absl::flat_hash_set<absl::string_view>();

    for (const auto word : kReservedWordList) {
      words->emplace(word);
    }
    return words;
  }();
  return *words;
}

const absl::flat_hash_set<absl::string_view>& NSObjectMethods() {
  static const auto* words = [] {
    auto* words = new absl::flat_hash_set<absl::string_view>();

    for (const auto word : kNSObjectMethodsList) {
      words->emplace(word);
    }
    return words;
  }();
  return *words;
}

// returns true is input starts with __ or _[A-Z] which are reserved identifiers
// in C/ C++. All calls should go through UnderscoresToCamelCase before getting
// here but this verifies and allows for future expansion if we decide to
// redefine what a reserved C identifier is (for example the GNU list
// https://www.gnu.org/software/libc/manual/html_node/Reserved-Names.html )
bool IsReservedCIdentifier(absl::string_view input) {
  if (input.length() > 2) {
    if (input.at(0) == '_') {
      if (isupper(input.at(1)) || input.at(1) == '_') {
        return true;
      }
    }
  }
  return false;
}

std::string SanitizeNameForObjC(absl::string_view prefix,
                                absl::string_view input,
                                absl::string_view extension,
                                std::string* out_suffix_added) {
  std::string sanitized;
  // We add the prefix in the cases where the string is missing a prefix.
  // We define "missing a prefix" as where 'input':
  // a) Doesn't start with the prefix or
  // b) Isn't equivalent to the prefix or
  // c) Has the prefix, but the letter after the prefix is lowercase
  if (absl::StartsWith(input, prefix)) {
    if (input.length() == prefix.length() ||
        !absl::ascii_isupper(input[prefix.length()])) {
      sanitized = absl::StrCat(prefix, input);
    } else {
      sanitized = std::string(input);
    }
  } else {
    sanitized = absl::StrCat(prefix, input);
  }
  if (IsReservedCIdentifier(sanitized) || ReservedWords().contains(sanitized) ||
      NSObjectMethods().contains(sanitized)) {
    if (out_suffix_added) *out_suffix_added = std::string(extension);
    return absl::StrCat(sanitized, extension);
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

void PathSplit(absl::string_view path, std::string* directory,
               std::string* basename) {
  absl::string_view::size_type last_slash = path.rfind('/');
  if (last_slash == absl::string_view::npos) {
    if (directory) {
      *directory = "";
    }
    if (basename) {
      *basename = std::string(path);
    }
  } else {
    if (directory) {
      *directory = std::string(path.substr(0, last_slash));
    }
    if (basename) {
      *basename = std::string(path.substr(last_slash + 1));
    }
  }
}

bool IsSpecialNamePrefix(absl::string_view name,
                         const std::vector<std::string>& special_names) {
  for (const auto& special_name : special_names) {
    const size_t length = special_name.length();
    if (name.compare(0, length, special_name) == 0) {
      if (name.length() > length) {
        // If name is longer than the special_name that it matches the next
        // character must be not lower case (newton vs newTon vs new_ton).
        return !absl::ascii_islower(name[length]);
      } else {
        return true;
      }
    }
  }
  return false;
}

void MaybeUnQuote(absl::string_view* input) {
  if ((input->length() >= 2) &&
      ((*input->data() == '\'' || *input->data() == '"')) &&
      ((*input)[input->length() - 1] == *input->data())) {
    input->remove_prefix(1);
    input->remove_suffix(1);
  }
}

}  // namespace

bool IsRetainedName(absl::string_view name) {
  // List of prefixes from
  // http://developer.apple.com/library/mac/#documentation/Cocoa/Conceptual/MemoryMgmt/Articles/mmRules.html
  static const std::vector<std::string>* retained_names =
      new std::vector<std::string>({"new", "alloc", "copy", "mutableCopy"});
  return IsSpecialNamePrefix(name, *retained_names);
}

bool IsInitName(absl::string_view name) {
  static const std::vector<std::string>* init_names =
      new std::vector<std::string>({"init"});
  return IsSpecialNamePrefix(name, *init_names);
}

bool IsCreateName(absl::string_view name) {
  // List of segments from
  // https://developer.apple.com/library/archive/documentation/CoreFoundation/Conceptual/CFMemoryMgmt/Concepts/Ownership.html#//apple_ref/doc/uid/20001148-103029
  static const std::vector<std::string>* create_names =
      new std::vector<std::string>({"Create", "Copy"});

  for (const auto& create_name : *create_names) {
    const size_t length = create_name.length();
    size_t pos = name.find(create_name);
    if (pos != std::string::npos) {
      // The above docs don't actually call out anything about the characters
      // before the special words. So it's not clear if something like
      // "FOOCreate" would or would not match the "The Create Rule", but by not
      // checking, and claiming it does match, then callers will annotate with
      // `cf_returns_not_retained` which will ensure things work as desired.
      //
      // The footnote here is the docs do have a passing reference to "NoCopy",
      // but again, not looking for that and just returning `true` will cause
      // callers to annotate the api as not being a Create Rule function.

      // If name is longer than the create_names[i] that it matches the next
      // character must be not lower case (Copyright vs CopyFoo vs Copy_Foo).
      if (name.length() > pos + length) {
        return !absl::ascii_islower(name[pos + length]);
      } else {
        return true;
      }
    }
  }
  return false;
}

std::string BaseFileName(const FileDescriptor* file) {
  std::string basename;
  PathSplit(file->name(), nullptr, &basename);
  return basename;
}

std::string FileClassPrefix(const FileDescriptor* file) {
  // Always honor the file option.
  if (file->options().has_objc_class_prefix()) {
    return file->options().objc_class_prefix();
  }

  // If package prefix is specified in an prefix to proto mappings file then use
  // that.
  absl::string_view objc_class_prefix =
      g_prefix_mode.prefix_from_proto_package_mappings(file);
  if (!objc_class_prefix.empty()) {
    return std::string(objc_class_prefix);
  }

  // If package prefix isn't enabled, done.
  if (!g_prefix_mode.use_package_name()) {
    return "";
  }

  // If the package is in the exceptions list, done.
  if (g_prefix_mode.is_package_exempted(file->package())) {
    return "";
  }

  // Transform the package into a prefix: use the dot segments as part,
  // camelcase each one and then join them with underscores, and add an
  // underscore at the end.
  std::string result;
  const std::vector<std::string> segments =
      absl::StrSplit(file->package(), '.', absl::SkipEmpty());
  for (const auto& segment : segments) {
    const std::string part = UnderscoresToCamelCase(segment, true);
    if (part.empty()) {
      continue;
    }
    if (!result.empty()) {
      result.append("_");
    }
    result.append(part);
  }
  if (!result.empty()) {
    result.append("_");
  }
  return absl::StrCat(g_prefix_mode.forced_package_prefix(), result);
}

std::string FilePath(const FileDescriptor* file) {
  std::string output;
  std::string basename;
  std::string directory;
  PathSplit(file->name(), &directory, &basename);
  if (!directory.empty()) {
    output = absl::StrCat(directory, "/");
  }
  basename = StripProto(basename);

  // CamelCase to be more ObjC friendly.
  basename = UnderscoresToCamelCase(basename, true);

  return absl::StrCat(output, basename);
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
  const std::string name = absl::StrCat(
      UnderscoresToCamelCase(StripProto(BaseFileName(file)), true), "Root");
  // There aren't really any reserved words that end in "Root", but playing
  // it safe and checking.
  return SanitizeNameForObjC(prefix, name, "_RootClass", nullptr);
}

std::string ClassNameWorker(const Descriptor* descriptor) {
  std::string name;
  if (descriptor->containing_type() != nullptr) {
    return absl::StrCat(ClassNameWorker(descriptor->containing_type()), "_",
                        descriptor->name());
  }
  return absl::StrCat(name, descriptor->name());
}

std::string ClassNameWorker(const EnumDescriptor* descriptor) {
  std::string name;
  if (descriptor->containing_type() != nullptr) {
    return absl::StrCat(ClassNameWorker(descriptor->containing_type()), "_",
                        descriptor->name());
  }
  return absl::StrCat(name, descriptor->name());
}

std::string ClassName(const Descriptor* descriptor) {
  return ClassName(descriptor, nullptr);
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
  return SanitizeNameForObjC(prefix, name, "_Enum", nullptr);
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
  const std::string name = absl::StrCat(class_name, "_", value_str);
  // There aren't really any reserved words with an underscore and a leading
  // capital letter, but playing it safe and checking.
  return SanitizeNameForObjC("", name, "_Value", nullptr);
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
  const std::string long_name_prefix = absl::StrCat(class_name, "_");
  const std::string long_name = EnumValueName(descriptor);
  return std::string(absl::StripPrefix(long_name, long_name_prefix));
}

std::string UnCamelCaseEnumShortName(absl::string_view name) {
  std::string result;
  for (int i = 0; i < name.size(); i++) {
    char c = name[i];
    if (i > 0 && absl::ascii_isupper(c)) {
      result += '_';
    }
    result += absl::ascii_toupper(c);
  }
  return result;
}

std::string ExtensionMethodName(const FieldDescriptor* descriptor) {
  const std::string name = NameFromFieldDescriptor(descriptor);
  const std::string result = UnderscoresToCamelCase(name, false);
  return SanitizeNameForObjC("", result, "_Extension", nullptr);
}

std::string FieldName(const FieldDescriptor* field) {
  const std::string name = NameFromFieldDescriptor(field);
  std::string result = UnderscoresToCamelCase(name, false);
  if (field->is_repeated() && !field->is_map()) {
    // Add "Array" before do check for reserved worlds.
    absl::StrAppend(&result, "Array");
  } else {
    // If it wasn't repeated, but ends in "Array", force on the _p suffix.
    if (absl::EndsWith(result, "Array")) {
      absl::StrAppend(&result, "_p");
    }
  }
  return SanitizeNameForObjC("", result, "_p", nullptr);
}

std::string FieldNameCapitalized(const FieldDescriptor* field) {
  // Want the same suffix handling, so upcase the first letter of the other
  // name.
  std::string result = FieldName(field);
  if (!result.empty()) {
    result[0] = absl::ascii_toupper(result[0]);
  }
  return result;
}

namespace {

enum class FragmentNameMode : int { kCommon, kMapKey, kObjCGenerics };
std::string FragmentName(const FieldDescriptor* field,
                         FragmentNameMode mode = FragmentNameMode::kCommon) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SFIXED32:
      return "Int32";

    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_FIXED32:
      return "UInt32";

    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_SFIXED64:
      return "Int64";

    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_FIXED64:
      return "UInt64";

    case FieldDescriptor::TYPE_FLOAT:
      return "Float";

    case FieldDescriptor::TYPE_DOUBLE:
      return "Double";

    case FieldDescriptor::TYPE_BOOL:
      return "Bool";

    case FieldDescriptor::TYPE_STRING: {
      switch (mode) {
        case FragmentNameMode::kCommon:
          return "Object";
        case FragmentNameMode::kMapKey:
          return "String";
        case FragmentNameMode::kObjCGenerics:
          return "NSString*";
      }
    }

    case FieldDescriptor::TYPE_BYTES:
      return (mode == FragmentNameMode::kObjCGenerics ? "NSData*" : "Object");

    case FieldDescriptor::TYPE_ENUM:
      return "Enum";

    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE:
      return (mode == FragmentNameMode::kObjCGenerics
                  ? absl::StrCat(ClassName(field->message_type()), "*")
                  : "Object");
  }

  // Some compilers report reaching end of function even though all cases of
  // the enum are handed in the switch.
  ABSL_LOG(FATAL) << "Can't get here.";
}

std::string FieldObjCTypeInternal(const FieldDescriptor* field,
                                  bool* out_is_ptr, std::string* out_generics) {
  if (field->is_map()) {
    *out_is_ptr = true;
    const FieldDescriptor* key_field = field->message_type()->map_key();
    const FieldDescriptor* value_field = field->message_type()->map_value();

    bool value_is_object;
    switch (value_field->type()) {
      case FieldDescriptor::TYPE_STRING:
      case FieldDescriptor::TYPE_BYTES:
      case FieldDescriptor::TYPE_GROUP:
      case FieldDescriptor::TYPE_MESSAGE: {
        value_is_object = true;
        break;
      }
      default:
        value_is_object = false;
        break;
    }

    if (value_is_object && key_field->type() == FieldDescriptor::TYPE_STRING) {
      if (out_generics) {
        *out_generics = absl::StrCat(
            "<NSString*, ",
            FragmentName(value_field, FragmentNameMode::kObjCGenerics), ">");
      }
      return "NSMutableDictionary";
    }

    if (value_is_object && out_generics) {
      *out_generics = absl::StrCat(
          "<", FragmentName(value_field, FragmentNameMode::kObjCGenerics), ">");
    }
    return absl::StrCat("GPB",
                        FragmentName(key_field, FragmentNameMode::kMapKey),
                        FragmentName(value_field), "Dictionary");
  }

  if (field->is_repeated()) {
    *out_is_ptr = true;

    switch (field->type()) {
      case FieldDescriptor::TYPE_STRING:
      case FieldDescriptor::TYPE_BYTES:
      case FieldDescriptor::TYPE_GROUP:
      case FieldDescriptor::TYPE_MESSAGE: {
        if (out_generics) {
          *out_generics = absl::StrCat(
              "<", FragmentName(field, FragmentNameMode::kObjCGenerics), ">");
        }
        return "NSMutableArray";
      }
      default:
        return absl::StrCat("GPB", FragmentName(field), "Array");
    }
  }

  // Single field

  switch (field->type()) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SFIXED32: {
      *out_is_ptr = false;
      return "int32_t";
    }

    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_FIXED32: {
      *out_is_ptr = false;
      return "uint32_t";
    }

    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_SFIXED64: {
      *out_is_ptr = false;
      return "int64_t";
    }

    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_FIXED64: {
      *out_is_ptr = false;
      return "uint64_t";
    }

    case FieldDescriptor::TYPE_FLOAT: {
      *out_is_ptr = false;
      return "float";
    }

    case FieldDescriptor::TYPE_DOUBLE: {
      *out_is_ptr = false;
      return "double";
    }

    case FieldDescriptor::TYPE_BOOL: {
      *out_is_ptr = false;
      return "BOOL";
    }

    case FieldDescriptor::TYPE_STRING: {
      *out_is_ptr = true;
      return "NSString";
    }

    case FieldDescriptor::TYPE_BYTES: {
      *out_is_ptr = true;
      return "NSData";
    }

    case FieldDescriptor::TYPE_ENUM: {
      *out_is_ptr = false;
      return EnumName(field->enum_type());
    }

    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE: {
      *out_is_ptr = true;
      return ClassName(field->message_type());
    }
  }

  // Some compilers report reaching end of function even though all cases of
  // the enum are handed in the switch.
  ABSL_LOG(FATAL) << "Can't get here.";
}

}  // namespace

std::string FieldObjCType(const FieldDescriptor* field,
                          FieldObjCTypeOptions options) {
  std::string generics;
  bool is_ptr;
  std::string base_type = FieldObjCTypeInternal(
      field, &is_ptr,
      ((options & kFieldObjCTypeOptions_OmitLightweightGenerics) != 0)
          ? nullptr
          : &generics);

  if (!is_ptr) {
    if ((options & kFieldObjCTypeOptions_IncludeSpaceAfterBasicTypes) != 0) {
      return absl::StrCat(base_type, " ");
    }
    return base_type;
  }

  if ((options & kFieldObjCTypeOptions_IncludeSpaceBeforeStar) != 0) {
    return absl::StrCat(base_type, generics, " *");
  }
  return absl::StrCat(base_type, generics, "*");
}

std::string OneofEnumName(const OneofDescriptor* descriptor) {
  const Descriptor* fieldDescriptor = descriptor->containing_type();
  std::string name = absl::StrCat(
      ClassName(fieldDescriptor), "_",
      UnderscoresToCamelCase(descriptor->name(), true), "_OneOfCase");
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
  if (!result.empty()) {
    result[0] = absl::ascii_toupper(result[0]);
  }
  return result;
}

std::string UnCamelCaseFieldName(absl::string_view name,
                                 const FieldDescriptor* field) {
  absl::string_view worker(name);
  if (absl::EndsWith(worker, "_p")) {
    worker = absl::StripSuffix(worker, "_p");
  }
  if (field->is_repeated() && absl::EndsWith(worker, "Array")) {
    worker = absl::StripSuffix(worker, "Array");
  }
  if (field->type() == FieldDescriptor::TYPE_GROUP) {
    if (!worker.empty()) {
      if (absl::ascii_islower(worker[0])) {
        std::string copy(worker);
        copy[0] = absl::ascii_toupper(worker[0]);
        return copy;
      }
    }
    return std::string(worker);
  } else {
    std::string result;
    for (int i = 0; i < worker.size(); i++) {
      char c = worker[i];
      if (absl::ascii_isupper(c)) {
        if (i > 0) {
          result += '_';
        }
        result += absl::ascii_tolower(c);
      } else {
        result += c;
      }
    }
    return result;
  }
}

// Making these a generator option for folks that don't use CocoaPods, but do
// want to put the library in a framework is an interesting question. The
// problem is it means changing sources shipped with the library to actually
// use a different value; so it isn't as simple as a option.
const char* const ProtobufLibraryFrameworkName = "Protobuf";

std::string ProtobufFrameworkImportSymbol(absl::string_view framework_name) {
  // GPB_USE_[framework_name]_FRAMEWORK_IMPORTS
  return absl::StrCat("GPB_USE_", absl::AsciiStrToUpper(framework_name),
                      "_FRAMEWORK_IMPORTS");
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

namespace {

bool PackageToPrefixesCollector::ConsumeLine(absl::string_view line,
                                             std::string* out_error) {
  int offset = line.find('=');
  if (offset == absl::string_view::npos) {
    *out_error =
        absl::StrCat(usage_, " file line without equal sign: '", line, "'.");
    return false;
  }
  absl::string_view package =
      absl::StripAsciiWhitespace(line.substr(0, offset));
  absl::string_view prefix =
      absl::StripAsciiWhitespace(line.substr(offset + 1));
  MaybeUnQuote(&prefix);
  // Don't really worry about error checking the package/prefix for
  // being valid.  Assume the file is validated when it is created/edited.
  (*prefix_map_)[package] = std::string(prefix);
  return true;
}

bool LoadExpectedPackagePrefixes(
    absl::string_view expected_prefixes_path,
    absl::flat_hash_map<std::string, std::string>* prefix_map,
    std::string* out_error) {
  if (expected_prefixes_path.empty()) {
    return true;
  }

  PackageToPrefixesCollector collector("Expected prefixes", prefix_map);
  return ParseSimpleFile(expected_prefixes_path, &collector, out_error);
}

bool ValidateObjCClassPrefix(
    const FileDescriptor* file, absl::string_view expected_prefixes_path,
    const absl::flat_hash_map<std::string, std::string>&
        expected_package_prefixes,
    bool prefixes_must_be_registered, bool require_prefixes,
    std::string* out_error) {
  // Reminder: An explicit prefix option of "" is valid in case the default
  // prefixing is set to use the proto package and a file needs to be generated
  // without any prefix at all (for legacy reasons).

  bool has_prefix = file->options().has_objc_class_prefix();
  bool have_expected_prefix_file = !expected_prefixes_path.empty();

  const std::string prefix = file->options().objc_class_prefix();
  const std::string package = file->package();
  // For files without packages, the can be registered as "no_package:PATH",
  // allowing the expected prefixes file.
  const std::string lookup_key =
      package.empty() ? absl::StrCat(kNoPackagePrefix, file->name()) : package;

  // NOTE: src/google/protobuf/compiler/plugin.cc makes use of cerr for some
  // error cases, so it seems to be ok to use as a back door for warnings.

  // Check: Error - See if there was an expected prefix for the package and
  // report if it doesn't match (wrong or missing).
  auto package_match = expected_package_prefixes.find(lookup_key);
  if (package_match != expected_package_prefixes.end()) {
    // There was an entry, and...
    if (has_prefix && package_match->second == prefix) {
      // ...it matches.  All good, out of here!
      return true;
    } else {
      // ...it didn't match!
      *out_error =
          absl::StrCat("error: Expected 'option objc_class_prefix = \"",
                       package_match->second, "\";'");
      if (!package.empty()) {
        absl::StrAppend(out_error, " for package '", package, "'");
      }
      absl::StrAppend(out_error, " in '", file->name(), "'");
      if (has_prefix) {
        absl::StrAppend(out_error, "; but found '", prefix, "' instead");
      }
      absl::StrAppend(out_error, ".");
      return false;
    }
  }

  // If there was no prefix option, we're done at this point.
  if (!has_prefix) {
    if (require_prefixes) {
      *out_error = absl::StrCat("error: '", file->name(),
                                "' does not have a required 'option"
                                " objc_class_prefix'.");
      return false;
    }
    return true;
  }

  // When the prefix is non empty, check it against the expected entries.
  if (!prefix.empty() && have_expected_prefix_file) {
    // For a non empty prefix, look for any other package that uses the prefix.
    std::string other_package_for_prefix;
    for (auto i = expected_package_prefixes.begin();
         i != expected_package_prefixes.end(); ++i) {
      if (i->second == prefix) {
        other_package_for_prefix = i->first;
        // Stop on the first real package listing, if it was a no_package file
        // specific entry, keep looking to try and find a package one.
        if (!absl::StartsWith(other_package_for_prefix, kNoPackagePrefix)) {
          break;
        }
      }
    }

    // Check: Error - Make sure the prefix wasn't expected for a different
    // package (overlap is allowed, but it has to be listed as an expected
    // overlap).
    if (!other_package_for_prefix.empty()) {
      *out_error = absl::StrCat("error: Found 'option objc_class_prefix = \"",
                                prefix, "\";' in '", file->name(),
                                "'; that prefix is already used for ");
      if (absl::StartsWith(other_package_for_prefix, kNoPackagePrefix)) {
        absl::StrAppend(
            out_error, "file '",
            absl::StripPrefix(other_package_for_prefix, kNoPackagePrefix),
            "'.");
      } else {
        absl::StrAppend(out_error, "'package ", other_package_for_prefix,
                        ";'.");
      }
      absl::StrAppend(out_error, " It can only be reused by adding '",
                      lookup_key, " = ", prefix,
                      "' to the expected prefixes file (",
                      expected_prefixes_path, ").");
      return false;  // Only report first usage of the prefix.
    }
  }  // !prefix.empty() && have_expected_prefix_file

  // Check: Warning - Make sure the prefix is is a reasonable value according
  // to Apple's rules (the checks above implicitly whitelist anything that
  // doesn't meet these rules).
  if (!prefix.empty() && !absl::ascii_isupper(prefix[0])) {
    std::cerr << "protoc:0: warning: Invalid 'option objc_class_prefix = \""
              << prefix << "\";' in '" << file->name() << "';"
              << " it should start with a capital letter." << std::endl;
    std::cerr.flush();
  }
  if (!prefix.empty() && prefix.length() < 3) {
    // Apple reserves 2 character prefixes for themselves. They do use some
    // 3 character prefixes, but they haven't updated the rules/docs.
    std::cerr << "protoc:0: warning: Invalid 'option objc_class_prefix = \""
              << prefix << "\";' in '" << file->name() << "';"
              << " Apple recommends they should be at least 3 characters long."
              << std::endl;
    std::cerr.flush();
  }

  // Check: Error/Warning - If the given package/prefix pair wasn't expected,
  // issue a error/warning to added to the file.
  if (have_expected_prefix_file) {
    if (prefixes_must_be_registered) {
      *out_error = absl::StrCat(
          "error: '", file->name(), "' has 'option objc_class_prefix = \"",
          prefix, "\";', but it is not registered. Add '", lookup_key, " = ",
          (prefix.empty() ? "\"\"" : prefix),
          "' to the expected prefixes file (", expected_prefixes_path, ").");
      return false;
    }

    std::cerr
        << "protoc:0: warning: Found unexpected 'option objc_class_prefix = \""
        << prefix << "\";' in '" << file->name() << "'; consider adding '"
        << lookup_key << " = " << (prefix.empty() ? "\"\"" : prefix)
        << "' to the expected prefixes file (" << expected_prefixes_path << ")."
        << std::endl;
    std::cerr.flush();
  }

  return true;
}

}  // namespace

Options::Options() {
  // While there are generator options, also support env variables to help with
  // build systems where it isn't as easy to hook in for add the generation
  // options when invoking protoc.
  const char* file_path = getenv("GPB_OBJC_EXPECTED_PACKAGE_PREFIXES");
  if (file_path) {
    expected_prefixes_path = file_path;
  }
  const char* suppressions =
      getenv("GPB_OBJC_EXPECTED_PACKAGE_PREFIXES_SUPPRESSIONS");
  if (suppressions) {
    expected_prefixes_suppressions =
        absl::StrSplit(suppressions, ';', absl::SkipEmpty());
  }
  prefixes_must_be_registered =
      BoolFromEnvVar("GPB_OBJC_PREFIXES_MUST_BE_REGISTERED", false);
  require_prefixes = BoolFromEnvVar("GPB_OBJC_REQUIRE_PREFIXES", false);
}

bool ValidateObjCClassPrefixes(const std::vector<const FileDescriptor*>& files,
                               std::string* out_error) {
  // Options's ctor load from the environment.
  Options options;
  return ValidateObjCClassPrefixes(files, options, out_error);
}

bool ValidateObjCClassPrefixes(const std::vector<const FileDescriptor*>& files,
                               const Options& validation_options,
                               std::string* out_error) {
  // Allow a '-' as the path for the expected prefixes to completely disable
  // even the most basic of checks.
  if (validation_options.expected_prefixes_path == "-") {
    return true;
  }

  // Load the expected package prefixes, if available, to validate against.
  absl::flat_hash_map<std::string, std::string> expected_package_prefixes;
  if (!LoadExpectedPackagePrefixes(validation_options.expected_prefixes_path,
                                   &expected_package_prefixes, out_error)) {
    return false;
  }

  for (auto file : files) {
    bool should_skip =
        (std::find(validation_options.expected_prefixes_suppressions.begin(),
                   validation_options.expected_prefixes_suppressions.end(),
                   file->name()) !=
         validation_options.expected_prefixes_suppressions.end());
    if (should_skip) {
      continue;
    }

    bool is_valid =
        ValidateObjCClassPrefix(file, validation_options.expected_prefixes_path,
                                expected_package_prefixes,
                                validation_options.prefixes_must_be_registered,
                                validation_options.require_prefixes, out_error);
    if (!is_valid) {
      return false;
    }
  }
  return true;
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
