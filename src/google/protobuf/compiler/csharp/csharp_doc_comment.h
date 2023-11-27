// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd


#ifndef GOOGLE_PROTOBUF_COMPILER_CSHARP_DOC_COMMENT_H__
#define GOOGLE_PROTOBUF_COMPILER_CSHARP_DOC_COMMENT_H__

#include "google/protobuf/io/printer.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {
    void WriteMessageDocComment(io::Printer* printer, const Descriptor* message);
    void WritePropertyDocComment(io::Printer* printer, const FieldDescriptor* field);
    void WriteEnumDocComment(io::Printer* printer, const EnumDescriptor* enumDescriptor);
    void WriteEnumValueDocComment(io::Printer* printer, const EnumValueDescriptor* value);
    void WriteMethodDocComment(io::Printer* printer, const MethodDescriptor* method);
}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_CSHARP_DOC_COMMENT_H__
