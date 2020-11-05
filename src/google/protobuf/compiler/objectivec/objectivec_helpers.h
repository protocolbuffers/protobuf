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

// Helper functions for generating ObjectiveC code.

#ifndef GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_HELPERS_H__
#define GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_HELPERS_H__

#include <string>
#include <vector>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

// Generator options (see objectivec_generator.cc for a description of each):
struct Options {
  Options();
  std::string expected_prefixes_path;
  std::vector<std::string> expected_prefixes_suppressions;
  std::string generate_for_named_framework;
  std::string named_framework_to_proto_path_mappings_path;
  std::string runtime_import_prefix;
};

// Escape C++ trigraphs by escaping question marks to "\?".
std::string PROTOC_EXPORT EscapeTrigraphs(const std::string& to_escape);

// Strips ".proto" or ".protodevel" from the end of a filename.
std::string PROTOC_EXPORT StripProto(const std::string& filename);

// Remove white space from either end of a StringPiece.
void PROTOC_EXPORT TrimWhitespace(StringPiece* input);

// Returns true if the name requires a ns_returns_not_retained attribute applied
// to it.
bool PROTOC_EXPORT IsRetainedName(const std::string& name);

// Returns true if the name starts with "init" and will need to have special
// handling under ARC.
bool PROTOC_EXPORT IsInitName(const std::string& name);

// Gets the objc_class_prefix.
std::string PROTOC_EXPORT FileClassPrefix(const FileDescriptor* file);

// Gets the path of the file we're going to generate (sans the .pb.h
// extension).  The path will be dependent on the objectivec package
// declared in the proto package.
std::string PROTOC_EXPORT FilePath(const FileDescriptor* file);

// Just like FilePath(), but without the directory part.
std::string PROTOC_EXPORT FilePathBasename(const FileDescriptor* file);

// Gets the name of the root class we'll generate in the file.  This class
// is not meant for external consumption, but instead contains helpers that
// the rest of the classes need
std::string PROTOC_EXPORT FileClassName(const FileDescriptor* file);

// These return the fully-qualified class name corresponding to the given
// descriptor.
std::string PROTOC_EXPORT ClassName(const Descriptor* descriptor);
std::string PROTOC_EXPORT ClassName(const Descriptor* descriptor,
                               std::string* out_suffix_added);
std::string PROTOC_EXPORT EnumName(const EnumDescriptor* descriptor);

// Returns the fully-qualified name of the enum value corresponding to the
// the descriptor.
std::string PROTOC_EXPORT EnumValueName(const EnumValueDescriptor* descriptor);

// Returns the name of the enum value corresponding to the descriptor.
std::string PROTOC_EXPORT EnumValueShortName(const EnumValueDescriptor* descriptor);

// Reverse what an enum does.
std::string PROTOC_EXPORT UnCamelCaseEnumShortName(const std::string& name);

// Returns the name to use for the extension (used as the method off the file's
// Root class).
std::string PROTOC_EXPORT ExtensionMethodName(const FieldDescriptor* descriptor);

// Returns the transformed field name.
std::string PROTOC_EXPORT FieldName(const FieldDescriptor* field);
std::string PROTOC_EXPORT FieldNameCapitalized(const FieldDescriptor* field);

// Returns the transformed oneof name.
std::string PROTOC_EXPORT OneofEnumName(const OneofDescriptor* descriptor);
std::string PROTOC_EXPORT OneofName(const OneofDescriptor* descriptor);
std::string PROTOC_EXPORT OneofNameCapitalized(const OneofDescriptor* descriptor);

// Returns a symbol that can be used in C code to refer to an Objective C
// class without initializing the class.
std::string PROTOC_EXPORT ObjCClass(const std::string& class_name);

// Declares an Objective C class without initializing the class so that it can
// be refrerred to by ObjCClass.
std::string PROTOC_EXPORT ObjCClassDeclaration(const std::string& class_name);

inline bool HasPreservingUnknownEnumSemantics(const FileDescriptor* file) {
  return file->syntax() == FileDescriptor::SYNTAX_PROTO3;
}

inline bool IsMapEntryMessage(const Descriptor* descriptor) {
  return descriptor->options().map_entry();
}

// Reverse of the above.
std::string PROTOC_EXPORT UnCamelCaseFieldName(const std::string& name,
                                               const FieldDescriptor* field);

enum ObjectiveCType {
  OBJECTIVECTYPE_INT32,
  OBJECTIVECTYPE_UINT32,
  OBJECTIVECTYPE_INT64,
  OBJECTIVECTYPE_UINT64,
  OBJECTIVECTYPE_FLOAT,
  OBJECTIVECTYPE_DOUBLE,
  OBJECTIVECTYPE_BOOLEAN,
  OBJECTIVECTYPE_STRING,
  OBJECTIVECTYPE_DATA,
  OBJECTIVECTYPE_ENUM,
  OBJECTIVECTYPE_MESSAGE
};

enum FlagType {
  FLAGTYPE_DESCRIPTOR_INITIALIZATION,
  FLAGTYPE_EXTENSION,
  FLAGTYPE_FIELD
};

template <class TDescriptor>
std::string GetOptionalDeprecatedAttribute(const TDescriptor* descriptor,
                                           const FileDescriptor* file = NULL,
                                           bool preSpace = true,
                                           bool postNewline = false) {
  bool isDeprecated = descriptor->options().deprecated();
  // The file is only passed when checking Messages & Enums, so those types
  // get tagged. At the moment, it doesn't seem to make sense to tag every
  // field or enum value with when the file is deprecated.
  bool isFileLevelDeprecation = false;
  if (!isDeprecated && file) {
    isFileLevelDeprecation = file->options().deprecated();
    isDeprecated = isFileLevelDeprecation;
  }
  if (isDeprecated) {
    std::string message;
    const FileDescriptor* sourceFile = descriptor->file();
    if (isFileLevelDeprecation) {
      message = sourceFile->name() + " is deprecated.";
    } else {
      message = descriptor->full_name() + " is deprecated (see " +
                sourceFile->name() + ").";
    }

    std::string result = std::string("GPB_DEPRECATED_MSG(\"") + message + "\")";
    if (preSpace) {
      result.insert(0, " ");
    }
    if (postNewline) {
      result.append("\n");
    }
    return result;
  } else {
    return "";
  }
}

std::string PROTOC_EXPORT GetCapitalizedType(const FieldDescriptor* field);

ObjectiveCType PROTOC_EXPORT
GetObjectiveCType(FieldDescriptor::Type field_type);

inline ObjectiveCType GetObjectiveCType(const FieldDescriptor* field) {
  return GetObjectiveCType(field->type());
}

bool PROTOC_EXPORT IsPrimitiveType(const FieldDescriptor* field);
bool PROTOC_EXPORT IsReferenceType(const FieldDescriptor* field);

std::string PROTOC_EXPORT
GPBGenericValueFieldName(const FieldDescriptor* field);
std::string PROTOC_EXPORT DefaultValue(const FieldDescriptor* field);
bool PROTOC_EXPORT HasNonZeroDefaultValue(const FieldDescriptor* field);

std::string PROTOC_EXPORT
BuildFlagsString(const FlagType type, const std::vector<std::string>& strings);

// Builds HeaderDoc/appledoc style comments out of the comments in the .proto
// file.
std::string PROTOC_EXPORT BuildCommentsString(const SourceLocation& location,
                                              bool prefer_single_line);

// The name the commonly used by the library when built as a framework.
// This lines up to the name used in the CocoaPod.
extern PROTOC_EXPORT const char* const ProtobufLibraryFrameworkName;
// Returns the CPP symbol name to use as the gate for framework style imports
// for the given framework name to use.
std::string PROTOC_EXPORT
ProtobufFrameworkImportSymbol(const std::string& framework_name);

// Checks if the file is one of the proto's bundled with the library.
bool PROTOC_EXPORT
IsProtobufLibraryBundledProtoFile(const FileDescriptor* file);

// Checks the prefix for the given files and outputs any warnings as needed. If
// there are flat out errors, then out_error is filled in with the first error
// and the result is false.
bool PROTOC_EXPORT ValidateObjCClassPrefixes(
    const std::vector<const FileDescriptor*>& files,
    const Options& generation_options, std::string* out_error);

// Generate decode data needed for ObjC's GPBDecodeTextFormatName() to transform
// the input into the expected output.
class PROTOC_EXPORT TextFormatDecodeData {
 public:
  TextFormatDecodeData();
  ~TextFormatDecodeData();

  TextFormatDecodeData(const TextFormatDecodeData&) = delete;
  TextFormatDecodeData& operator=(const TextFormatDecodeData&) = delete;

  void AddString(int32 key, const std::string& input_for_decode,
                 const std::string& desired_output);
  size_t num_entries() const { return entries_.size(); }
  std::string Data() const;

  static std::string DecodeDataForString(const std::string& input_for_decode,
                                         const std::string& desired_output);

 private:
  typedef std::pair<int32, std::string> DataEntry;
  std::vector<DataEntry> entries_;
};

// Helper for parsing simple files.
class PROTOC_EXPORT LineConsumer {
 public:
  LineConsumer();
  virtual ~LineConsumer();
  virtual bool ConsumeLine(const StringPiece& line, std::string* out_error) = 0;
};

bool PROTOC_EXPORT ParseSimpleFile(const std::string& path,
                                   LineConsumer* line_consumer,
                                   std::string* out_error);

// Helper class for parsing framework import mappings and generating
// import statements.
class PROTOC_EXPORT ImportWriter {
 public:
  ImportWriter(const std::string& generate_for_named_framework,
               const std::string& named_framework_to_proto_path_mappings_path,
               const std::string& runtime_import_prefix,
               bool include_wkt_imports);
  ~ImportWriter();

  void AddFile(const FileDescriptor* file, const std::string& header_extension);
  void Print(io::Printer *printer) const;

  static void PrintRuntimeImports(io::Printer *printer,
                                  const std::vector<std::string>& header_to_import,
                                  const std::string& runtime_import_prefix,
                                  bool default_cpp_symbol = false);

 private:
  class ProtoFrameworkCollector : public LineConsumer {
   public:
    ProtoFrameworkCollector(std::map<std::string, std::string>* inout_proto_file_to_framework_name)
        : map_(inout_proto_file_to_framework_name) {}

    virtual bool ConsumeLine(const StringPiece& line, std::string* out_error);

   private:
    std::map<std::string, std::string>* map_;
  };

  void ParseFrameworkMappings();

  const std::string generate_for_named_framework_;
  const std::string named_framework_to_proto_path_mappings_path_;
  const std::string runtime_import_prefix_;
  const bool include_wkt_imports_;
  std::map<std::string, std::string> proto_file_to_framework_name_;
  bool need_to_parse_mapping_file_;

  std::vector<std::string> protobuf_imports_;
  std::vector<std::string> other_framework_imports_;
  std::vector<std::string> other_imports_;
};

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_HELPERS_H__
