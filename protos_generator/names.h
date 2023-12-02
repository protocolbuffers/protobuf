// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_PROTOS_GENERATOR_NAMES_H_
#define UPB_PROTOS_GENERATOR_NAMES_H_

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include "protos_generator/output.h"

namespace protos_generator {

namespace protobuf = ::google::protobuf;

inline constexpr absl::string_view kNoPackageNamePrefix = "protos_";

std::string ClassName(const protobuf::Descriptor* descriptor);
std::string QualifiedClassName(const protobuf::Descriptor* descriptor);
std::string QualifiedInternalClassName(const protobuf::Descriptor* descriptor);

std::string CppSourceFilename(const google::protobuf::FileDescriptor* file);
std::string ForwardingHeaderFilename(const google::protobuf::FileDescriptor* file);
std::string UpbCFilename(const google::protobuf::FileDescriptor* file);
std::string CppHeaderFilename(const google::protobuf::FileDescriptor* file);

void WriteStartNamespace(const protobuf::FileDescriptor* file, Output& output);
void WriteEndNamespace(const protobuf::FileDescriptor* file, Output& output);

std::string CppConstType(const protobuf::FieldDescriptor* field);
std::string CppTypeParameterName(const protobuf::FieldDescriptor* field);

std::string MessageBaseType(const protobuf::FieldDescriptor* field,
                            bool is_const);
// Generate protos::Ptr<const Model> to be used in accessors as public
// signatures.
std::string MessagePtrConstType(const protobuf::FieldDescriptor* field,
                                bool is_const);
std::string MessageCProxyType(const protobuf::FieldDescriptor* field,
                              bool is_const);
std::string MessageProxyType(const protobuf::FieldDescriptor* field,
                             bool is_const);

}  // namespace protos_generator

#endif  // UPB_PROTOS_GENERATOR_NAMES_H_
