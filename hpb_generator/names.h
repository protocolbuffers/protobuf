// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_HPB_NAMES_H__
#define GOOGLE_PROTOBUF_COMPILER_HPB_NAMES_H__

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include "hpb_generator/context.h"

namespace google {
namespace protobuf {
namespace hpb_generator {

inline constexpr absl::string_view kNoPackageNamePrefix = "hpb_";

std::string ClassName(const google::protobuf::Descriptor* descriptor);
std::string QualifiedClassName(const google::protobuf::Descriptor* descriptor);
std::string QualifiedInternalClassName(const google::protobuf::Descriptor* descriptor);

std::string CppSourceFilename(const google::protobuf::FileDescriptor* file);
std::string UpbCFilename(const google::protobuf::FileDescriptor* file);
std::string CppHeaderFilename(const google::protobuf::FileDescriptor* file);

void WriteStartNamespace(const google::protobuf::FileDescriptor* file, Context& ctx);
void WriteEndNamespace(const google::protobuf::FileDescriptor* file, Context& ctx);

std::string CppConstType(const google::protobuf::FieldDescriptor* field);
std::string CppTypeParameterName(const google::protobuf::FieldDescriptor* field);

std::string MessageBaseType(const google::protobuf::FieldDescriptor* field,
                            bool is_const);
// Generate hpb::Ptr<const Model> to be used in accessors as public
// signatures.
std::string MessagePtrConstType(const google::protobuf::FieldDescriptor* field,
                                bool is_const);
std::string MessageCProxyType(const google::protobuf::FieldDescriptor* field,
                              bool is_const);
std::string MessageProxyType(const google::protobuf::FieldDescriptor* field,
                             bool is_const);

}  // namespace hpb_generator
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_HPB_NAMES_H__
