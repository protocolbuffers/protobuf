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

#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/compiler/objectivec/names.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

// Get/Set the path to a file to load for objc class prefix lookups.
std::string PROTOC_EXPORT GetPackageToPrefixMappingsPath();
void PROTOC_EXPORT SetPackageToPrefixMappingsPath(
    const std::string& file_path);
// Get/Set if the proto package should be used to make the default prefix for
// symbols. This will then impact most of the type naming apis below. It is done
// as a global to not break any other generator reusing the methods since they
// are exported.
bool PROTOC_EXPORT UseProtoPackageAsDefaultPrefix();
void PROTOC_EXPORT SetUseProtoPackageAsDefaultPrefix(bool on_or_off);
// Get/Set the path to a file to load as exceptions when
// `UseProtoPackageAsDefaultPrefix()` is `true`. An empty string means there
// should be no exceptions.
std::string PROTOC_EXPORT GetProtoPackagePrefixExceptionList();
void PROTOC_EXPORT SetProtoPackagePrefixExceptionList(
    const std::string& file_path);
// Get/Set a prefix to add before the prefix generated from the package name.
// This is only used when UseProtoPackageAsDefaultPrefix() is True.
std::string PROTOC_EXPORT GetForcedPackagePrefix();
void PROTOC_EXPORT SetForcedPackagePrefix(const std::string& prefix);

// Escape C++ trigraphs by escaping question marks to "\?".
std::string PROTOC_EXPORT EscapeTrigraphs(absl::string_view to_escape);

// Remove white space from either end of a absl::string_view.
void PROTOC_EXPORT TrimWhitespace(absl::string_view* input);

// Returns true if the name requires a ns_returns_not_retained attribute applied
// to it.
bool PROTOC_EXPORT IsRetainedName(const std::string& name);

// Returns true if the name starts with "init" and will need to have special
// handling under ARC.
bool PROTOC_EXPORT IsInitName(const std::string& name);

// Returns true if the name requires a cf_returns_not_retained attribute applied
// to it.
bool PROTOC_EXPORT IsCreateName(const std::string& name);

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

// Generate decode data needed for ObjC's GPBDecodeTextFormatName() to transform
// the input into the expected output.
class PROTOC_EXPORT TextFormatDecodeData {
 public:
  TextFormatDecodeData();
  ~TextFormatDecodeData();

  TextFormatDecodeData(const TextFormatDecodeData&) = delete;
  TextFormatDecodeData& operator=(const TextFormatDecodeData&) = delete;

  void AddString(int32_t key, const std::string& input_for_decode,
                 const std::string& desired_output);
  size_t num_entries() const { return entries_.size(); }
  std::string Data() const;

  static std::string DecodeDataForString(const std::string& input_for_decode,
                                         const std::string& desired_output);

 private:
  typedef std::pair<int32_t, std::string> DataEntry;
  std::vector<DataEntry> entries_;
};

// Helper for parsing simple files.
class PROTOC_EXPORT LineConsumer {
 public:
  LineConsumer();
  virtual ~LineConsumer();
  virtual bool ConsumeLine(const absl::string_view& line, std::string* out_error) = 0;
};

bool PROTOC_EXPORT ParseSimpleFile(const std::string& path,
                                   LineConsumer* line_consumer,
                                   std::string* out_error);

bool PROTOC_EXPORT ParseSimpleStream(io::ZeroCopyInputStream& input_stream,
                                     const std::string& stream_name,
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
  void Print(io::Printer* printer) const;

  static void PrintRuntimeImports(io::Printer* printer,
                                  const std::vector<std::string>& header_to_import,
                                  const std::string& runtime_import_prefix,
                                  bool default_cpp_symbol = false);

 private:
  class ProtoFrameworkCollector : public LineConsumer {
   public:
    ProtoFrameworkCollector(std::map<std::string, std::string>* inout_proto_file_to_framework_name)
        : map_(inout_proto_file_to_framework_name) {}

    virtual bool ConsumeLine(const absl::string_view& line, std::string* out_error) override;

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

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_HELPERS_H__
