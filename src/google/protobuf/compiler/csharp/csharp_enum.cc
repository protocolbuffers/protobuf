// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/csharp/csharp_enum.h"

#include <sstream>
#include <string>

#include "google/protobuf/compiler/code_generator.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/csharp/csharp_doc_comment.h"
#include "google/protobuf/compiler/csharp/csharp_helpers.h"
#include "google/protobuf/compiler/csharp/csharp_options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

EnumGenerator::EnumGenerator(const EnumDescriptor* descriptor, const Options* options) :
    SourceGeneratorBase(options),
    descriptor_(descriptor) {
}

EnumGenerator::~EnumGenerator() {
}

void EnumGenerator::Generate(io::Printer* printer) {
  WriteEnumDocComment(printer, options(), descriptor_);
  if (descriptor_->options().deprecated()) {
    printer->Print("[global::System.ObsoleteAttribute]\n");
  }
  printer->Print("$access_level$ enum $name$ {\n",
                 "access_level", class_access_level(),
                 "name", descriptor_->name());
  printer->Indent();
  absl::flat_hash_set<std::string> used_names;
  absl::flat_hash_set<int> used_number;
  for (int i = 0; i < descriptor_->value_count(); i++) {
    WriteEnumValueDocComment(printer, options(), descriptor_->value(i));
    if (descriptor_->value(i)->options().deprecated()) {
      printer->Print("[global::System.ObsoleteAttribute]\n");
    }
    const absl::string_view original_name = descriptor_->value(i)->name();
    std::string name =
        GetEnumValueName(descriptor_->name(), descriptor_->value(i)->name());
    // Make sure we don't get any duplicate names due to prefix removal.
    while (!used_names.insert(name).second) {
      // It's possible we'll end up giving this warning multiple times, but
      // that's better than not at all.
      ABSL_LOG(WARNING) << "Duplicate enum value " << name << " (originally "
                        << original_name << ") in " << descriptor_->name()
                        << "; adding underscore to distinguish";
      absl::StrAppend(&name, "_");
    }
    int number = descriptor_->value(i)->number();
    if (!used_number.insert(number).second) {
      printer->Print(
          "[pbr::OriginalName(\"$original_name$\", PreferredAlias = false)] "
          "$name$ = $number$,\n",
          "original_name", original_name, "name", name, "number",
          absl::StrCat(number));
    } else {
      printer->Print(
          "[pbr::OriginalName(\"$original_name$\")] $name$ = $number$,\n",
          "original_name", original_name, "name", name, "number",
          absl::StrCat(number));
    }
  }
  printer->Outdent();
  printer->Print("}\n");
  printer->Print("\n");
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
