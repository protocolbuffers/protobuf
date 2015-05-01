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

#include <sstream>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>

#include <google/protobuf/compiler/csharp/csharp_extension.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/compiler/csharp/csharp_writer.h>
#include <google/protobuf/compiler/csharp/csharp_field_base.h>

using google::protobuf::internal::scoped_ptr;

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

ExtensionGenerator::ExtensionGenerator(const FieldDescriptor* descriptor)
    : FieldGeneratorBase(descriptor, 0) {
  if (descriptor_->extension_scope()) {
    scope_ = GetClassName(descriptor_->extension_scope());
  } else {
    scope_ = GetFullUmbrellaClassName(descriptor_->file());
  }
  extends_ = GetClassName(descriptor_->containing_type());
}

ExtensionGenerator::~ExtensionGenerator() {
}

void ExtensionGenerator::Generate(Writer* writer) {
  writer->WriteLine("public const int $0$ = $1$;",
                    GetFieldConstantName(descriptor_),
                    SimpleItoa(descriptor_->number()));

  if (use_lite_runtime()) {
    // TODO(jtattermusch): include the following check
    //if (Descriptor.MappedType == MappedType.Message && Descriptor.MessageType.Options.MessageSetWireFormat)
    //{
    //    throw new ArgumentException(
    //        "option message_set_wire_format = true; is not supported in Lite runtime extensions.");
    //}

    writer->Write("$0$ ", class_access_level());
    writer->WriteLine(
        "static pb::$3$<$0$, $1$> $2$;",
        extends_,
        type_name(),
        property_name(),
        descriptor_->is_repeated() ?
            "GeneratedRepeatExtensionLite" : "GeneratedExtensionLite");
  } else if (descriptor_->is_repeated()) {
    writer->WriteLine(
        "$0$ static pb::GeneratedExtensionBase<scg::IList<$1$>> $2$;",
        class_access_level(), type_name(), property_name());
  } else {
    writer->WriteLine("$0$ static pb::GeneratedExtensionBase<$1$> $2$;",
                      class_access_level(), type_name(), property_name());
  }
}

void ExtensionGenerator::GenerateStaticVariableInitializers(Writer* writer) {
  if (use_lite_runtime()) {
    writer->WriteLine("$0$.$1$ = ", scope_, property_name());
    writer->Indent();
    writer->WriteLine(
        "new pb::$0$<$1$, $2$>(",
        descriptor_->is_repeated() ?
            "GeneratedRepeatExtensionLite" : "GeneratedExtensionLite",
        extends_, type_name());
    writer->Indent();
    writer->WriteLine("\"$0$\",", descriptor_->full_name());
    writer->WriteLine("$0$.DefaultInstance,", extends_);
    if (!descriptor_->is_repeated()) {
      std::string default_val;
      if (descriptor_->has_default_value()) {
        default_val = default_value();
      } else {
        default_val = is_nullable_type() ? "null" : ("default(" + type_name() + ")");
      }
      writer->WriteLine("$0$,", default_val);
    }
    writer->WriteLine(
        "$0$,",
        (GetCSharpType(descriptor_->type()) == CSHARPTYPE_MESSAGE) ?
            type_name() + ".DefaultInstance" : "null");
    writer->WriteLine(
        "$0$,",
        (GetCSharpType(descriptor_->type()) == CSHARPTYPE_ENUM) ?
            "new EnumLiteMap<" + type_name() + ">()" : "null");
    writer->WriteLine("$0$.$1$FieldNumber,", scope_,
                      GetPropertyName(descriptor_));
    writer->Write("pbd::FieldType.$0$", capitalized_type_name());
    if (descriptor_->is_repeated()) {
      writer->WriteLine(",");
      writer->Write(descriptor_->is_packed() ? "true" : "false");
    }
    writer->Outdent();
    writer->WriteLine(");");
    writer->Outdent();
  }
  else if (descriptor_->is_repeated())
  {
     writer->WriteLine(
         "$0$.$1$ = pb::GeneratedRepeatExtension<$2$>.CreateInstance($0$.Descriptor.Extensions[$3$]);",
         scope_, property_name(), type_name(), SimpleItoa(descriptor_->index()));
  }
  else
  {
     writer->WriteLine(
         "$0$.$1$ = pb::GeneratedSingleExtension<$2$>.CreateInstance($0$.Descriptor.Extensions[$3$]);",
         scope_, property_name(), type_name(), SimpleItoa(descriptor_->index()));
  }
}

void ExtensionGenerator::GenerateExtensionRegistrationCode(Writer* writer) {
  writer->WriteLine("registry.Add($0$.$1$);", scope_, property_name());
}

void ExtensionGenerator::WriteHash(Writer* writer) {
}

void ExtensionGenerator::WriteEquals(Writer* writer) {
}

void ExtensionGenerator::WriteToString(Writer* writer) {
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
