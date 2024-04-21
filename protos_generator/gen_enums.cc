// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "protos_generator/gen_enums.h"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "protos_generator/gen_utils.h"
#include "protos_generator/names.h"

namespace protos_generator {

namespace protobuf = ::google::protobuf;

// Convert enum value to C++ literal.
//
// In C++, an value of -2147483648 gets interpreted as the negative of
// 2147483648, and since 2147483648 can't fit in an integer, this produces a
// compiler warning.  This works around that issue.
std::string EnumInt32ToString(int number) {
  if (number == std::numeric_limits<int32_t>::min()) {
    // This needs to be special-cased, see explanation here:
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52661
    return absl::StrCat(number + 1, " - 1");
  } else {
    return absl::StrCat(number);
  }
}

std::string EnumTypeName(const protobuf::EnumDescriptor* enum_descriptor) {
  auto containing_type = enum_descriptor->containing_type();
  if (containing_type == nullptr) {
    // enums types with no package name are prefixed with protos_ to prevent
    // conflicts with generated C headers.
    if (enum_descriptor->file()->package().empty()) {
      return absl::StrCat(kNoPackageNamePrefix,
                          ToCIdent(enum_descriptor->name()));
    }
    return ToCIdent(enum_descriptor->name());
  } else {
    // Since the enum is in global name space (no package), it will have the
    // same classified name as the C header include, to prevent collision
    // rename as above.
    if (containing_type->file()->package().empty()) {
      return ToCIdent(absl::StrCat(containing_type->name(), "_",
                                   kNoPackageNamePrefix,
                                   enum_descriptor->name()));
    } else {
      return ToCIdent(
          absl::StrCat(containing_type->name(), "_", enum_descriptor->name()));
    }
  }
}

std::string EnumValueSymbolInNameSpace(
    const protobuf::EnumDescriptor* desc,
    const protobuf::EnumValueDescriptor* value) {
  auto containing_type = desc->containing_type();
  if (containing_type != nullptr) {
    return ToCIdent(absl::StrCat(containing_type->name(), "_", desc->name(),
                                 "_", value->name()));
  } else {
    // protos enum values with no package name are prefixed with protos_ to
    // prevent conflicts with generated C headers.
    if (desc->file()->package().empty()) {
      return absl::StrCat(kNoPackageNamePrefix, ToCIdent(value->name()));
    }
    return ToCIdent(value->name());
  }
}

void WriteEnumValues(const protobuf::EnumDescriptor* desc, Output& output) {
  std::vector<const protobuf::EnumValueDescriptor*> values;
  auto value_count = desc->value_count();
  values.reserve(value_count);
  for (int i = 0; i < value_count; i++) {
    values.push_back(desc->value(i));
  }
  std::sort(values.begin(), values.end(),
            [](const protobuf::EnumValueDescriptor* a,
               const protobuf::EnumValueDescriptor* b) {
              return a->number() < b->number();
            });

  for (size_t i = 0; i < values.size(); i++) {
    auto value = values[i];
    output("  $0", EnumValueSymbolInNameSpace(desc, value));
    output(" = $0", EnumInt32ToString(value->number()));
    if (i != values.size() - 1) {
      output(",");
    }
    output("\n");
  }
}

void WriteEnumDeclarations(
    const std::vector<const protobuf::EnumDescriptor*>& enums, Output& output) {
  for (auto enumdesc : enums) {
    output("enum $0 : int {\n", EnumTypeName(enumdesc));
    WriteEnumValues(enumdesc, output);
    output("};\n\n");
  }
}

void WriteHeaderEnumForwardDecls(
    std::vector<const protobuf::EnumDescriptor*>& enums, Output& output) {
  for (const auto* enumdesc : enums) {
    output("enum $0 : int;\n", EnumTypeName(enumdesc));
  }
}

}  // namespace protos_generator
