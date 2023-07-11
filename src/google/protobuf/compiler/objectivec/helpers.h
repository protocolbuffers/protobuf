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
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

// Escape C++ trigraphs by escaping question marks to "\?".
std::string EscapeTrigraphs(absl::string_view to_escape);

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

std::string GetCapitalizedType(const FieldDescriptor* field);

ObjectiveCType GetObjectiveCType(FieldDescriptor::Type field_type);

inline ObjectiveCType GetObjectiveCType(const FieldDescriptor* field) {
  return GetObjectiveCType(field->type());
}

inline bool IsPrimitiveType(const FieldDescriptor* field) {
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

inline bool IsReferenceType(const FieldDescriptor* field) {
  return !IsPrimitiveType(field);
}

std::string GPBGenericValueFieldName(const FieldDescriptor* field);
std::string DefaultValue(const FieldDescriptor* field);

std::string BuildFlagsString(FlagType type,
                             const std::vector<std::string>& strings);

// Returns a symbol that can be used in C code to refer to an Objective-C
// class without initializing the class.
std::string ObjCClass(absl::string_view class_name);

// Declares an Objective-C class without initializing the class so that it can
// be referred to by ObjCClass.
std::string ObjCClassDeclaration(absl::string_view class_name);

// Flag to control the behavior of `EmitCommentsString`.
enum CommentStringFlags : unsigned int {
  kNone = 0,
  kAddLeadingNewline = 1 << 1,  // Add a newline before the comment.
  kForceMultiline = 1 << 2,  // Force a multiline comment even if only 1 line.
};

// Emits HeaderDoc/appledoc style comments out of the comments in the .proto
// file.
void EmitCommentsString(io::Printer* printer, const SourceLocation& location,
                        CommentStringFlags flags = kNone);

// Emits HeaderDoc/appledoc style comments out of the comments in the .proto
// file.
template <class TDescriptor>
void EmitCommentsString(io::Printer* printer, const TDescriptor* descriptor,
                        CommentStringFlags flags = kNone) {
  SourceLocation location;
  if (descriptor->GetSourceLocation(&location)) {
    EmitCommentsString(printer, location, flags);
  }
}

template <class TDescriptor>
std::string GetOptionalDeprecatedAttribute(const TDescriptor* descriptor,
                                           const FileDescriptor* file = nullptr,
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
      message = absl::StrCat(sourceFile->name(), " is deprecated.");
    } else {
      message = absl::StrCat(descriptor->full_name(), " is deprecated (see ",
                             sourceFile->name(), ").");
    }

    return absl::StrCat(preSpace ? " " : "", "GPB_DEPRECATED_MSG(\"", message,
                        "\")", postNewline ? "\n" : "");
  } else {
    return "";
  }
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_HELPERS_H__
