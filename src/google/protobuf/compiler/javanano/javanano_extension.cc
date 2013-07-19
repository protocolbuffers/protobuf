// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

// Author: bduff@google.com (Brian Duff)

#include <google/protobuf/compiler/javanano/javanano_extension.h>
#include <google/protobuf/compiler/javanano/javanano_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/wire_format.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace javanano {

using internal::WireFormat;

void SetVariables(const FieldDescriptor* descriptor, const Params params,
                  map<string, string>* variables) {
  (*variables)["name"] = 
    RenameJavaKeywords(UnderscoresToCamelCase(descriptor));
  (*variables)["number"] = SimpleItoa(descriptor->number());
  (*variables)["extends"] = ClassName(params, descriptor->containing_type());

  string type;
  JavaType java_type = GetJavaType(descriptor->type());
  switch (java_type) {
    case JAVATYPE_ENUM:
      type = "java.lang.Integer";
      break;
    case JAVATYPE_MESSAGE:
      type = ClassName(params, descriptor->message_type());
      break;
    default:
      type = BoxedPrimitiveTypeName(java_type);
      break;
  }
  (*variables)["type"] = type;
}

ExtensionGenerator::
ExtensionGenerator(const FieldDescriptor* descriptor, const Params& params)
  : params_(params), descriptor_(descriptor) {
  SetVariables(descriptor, params, &variables_);
}

ExtensionGenerator::~ExtensionGenerator() {}

void ExtensionGenerator::Generate(io::Printer* printer) const {
  if (descriptor_->is_repeated()) {
    printer->Print(variables_,
      "// Extends $extends$\n"
      "public static final com.google.protobuf.nano.Extension<java.util.List<$type$>> $name$ = \n"
      "    com.google.protobuf.nano.Extension.createRepeated($number$,\n"
      "        new com.google.protobuf.nano.Extension.TypeLiteral<java.util.List<$type$>>(){});\n");
  } else {
    printer->Print(variables_,
      "// Extends $extends$\n"
      "public static final com.google.protobuf.nano.Extension<$type$> $name$ =\n"
      "    com.google.protobuf.nano.Extension.create($number$,\n"
      "        new com.google.protobuf.nano.Extension.TypeLiteral<$type$>(){});\n");
  }
}

}  // namespace javanano
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

