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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <google/protobuf/compiler/java/java_service.h>
#include <google/protobuf/compiler/java/java_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

ServiceGenerator::ServiceGenerator(const ServiceDescriptor* descriptor)
  : descriptor_(descriptor) {}

ServiceGenerator::~ServiceGenerator() {}

void ServiceGenerator::Generate(io::Printer* printer) {
  bool is_own_file = descriptor_->file()->options().java_multiple_files();
  printer->Print(
    "public $static$ abstract class $classname$\n"
    "    implements com.google.protobuf.Service {\n",
    "static", is_own_file ? "" : "static",
    "classname", descriptor_->name());
  printer->Indent();

  // Generate abstract method declarations.
  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    map<string, string> vars;
    vars["name"] = UnderscoresToCamelCase(method);
    vars["input"] = ClassName(method->input_type());
    vars["output"] = ClassName(method->output_type());
    printer->Print(vars,
      "public abstract void $name$(\n"
      "    com.google.protobuf.RpcController controller,\n"
      "    $input$ request,\n"
      "    com.google.protobuf.RpcCallback<$output$> done);\n");
  }

  // Generate getDescriptor() and getDescriptorForType().
  printer->Print(
    "\n"
    "public static final\n"
    "    com.google.protobuf.Descriptors.ServiceDescriptor\n"
    "    getDescriptor() {\n"
    "  return $file$.getDescriptor().getServices().get($index$);\n"
    "}\n"
    "public final com.google.protobuf.Descriptors.ServiceDescriptor\n"
    "    getDescriptorForType() {\n"
    "  return getDescriptor();\n"
    "}\n",
    "file", ClassName(descriptor_->file()),
    "index", SimpleItoa(descriptor_->index()));

  // Generate more stuff.
  GenerateCallMethod(printer);
  GenerateGetPrototype(REQUEST, printer);
  GenerateGetPrototype(RESPONSE, printer);
  GenerateStub(printer);

  printer->Outdent();
  printer->Print("}\n\n");
}

void ServiceGenerator::GenerateCallMethod(io::Printer* printer) {
  printer->Print(
    "\n"
    "public final void callMethod(\n"
    "    com.google.protobuf.Descriptors.MethodDescriptor method,\n"
    "    com.google.protobuf.RpcController controller,\n"
    "    com.google.protobuf.Message request,\n"
    "    com.google.protobuf.RpcCallback<\n"
    "      com.google.protobuf.Message> done) {\n"
    "  if (method.getService() != getDescriptor()) {\n"
    "    throw new java.lang.IllegalArgumentException(\n"
    "      \"Service.callMethod() given method descriptor for wrong \" +\n"
    "      \"service type.\");\n"
    "  }\n"
    "  switch(method.getIndex()) {\n");
  printer->Indent();
  printer->Indent();

  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    map<string, string> vars;
    vars["index"] = SimpleItoa(i);
    vars["method"] = UnderscoresToCamelCase(method);
    vars["input"] = ClassName(method->input_type());
    vars["output"] = ClassName(method->output_type());
    printer->Print(vars,
      "case $index$:\n"
      "  this.$method$(controller, ($input$)request,\n"
      "    com.google.protobuf.RpcUtil.<$output$>specializeCallback(\n"
      "      done));\n"
      "  return;\n");
  }

  printer->Print(
    "default:\n"
    "  throw new java.lang.RuntimeException(\"Can't get here.\");\n");

  printer->Outdent();
  printer->Outdent();

  printer->Print(
    "  }\n"
    "}\n"
    "\n");
}

void ServiceGenerator::GenerateGetPrototype(RequestOrResponse which,
                                            io::Printer* printer) {
  printer->Print(
    "public final com.google.protobuf.Message\n"
    "    get$request_or_response$Prototype(\n"
    "    com.google.protobuf.Descriptors.MethodDescriptor method) {\n"
    "  if (method.getService() != getDescriptor()) {\n"
    "    throw new java.lang.IllegalArgumentException(\n"
    "      \"Service.get$request_or_response$Prototype() given method \" +\n"
    "      \"descriptor for wrong service type.\");\n"
    "  }\n"
    "  switch(method.getIndex()) {\n",
    "request_or_response", (which == REQUEST) ? "Request" : "Response");
  printer->Indent();
  printer->Indent();

  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    map<string, string> vars;
    vars["index"] = SimpleItoa(i);
    vars["type"] = ClassName(
      (which == REQUEST) ? method->input_type() : method->output_type());
    printer->Print(vars,
      "case $index$:\n"
      "  return $type$.getDefaultInstance();\n");
  }

  printer->Print(
    "default:\n"
    "  throw new java.lang.RuntimeException(\"Can't get here.\");\n");

  printer->Outdent();
  printer->Outdent();

  printer->Print(
    "  }\n"
    "}\n"
    "\n");
}

void ServiceGenerator::GenerateStub(io::Printer* printer) {
  printer->Print(
    "public static Stub newStub(\n"
    "    com.google.protobuf.RpcChannel channel) {\n"
    "  return new Stub(channel);\n"
    "}\n"
    "\n"
    "public static final class Stub extends $classname$ {\n",
    "classname", ClassName(descriptor_));
  printer->Indent();

  printer->Print(
    "private Stub(com.google.protobuf.RpcChannel channel) {\n"
    "  this.channel = channel;\n"
    "}\n"
    "\n"
    "private final com.google.protobuf.RpcChannel channel;\n"
    "\n"
    "public com.google.protobuf.RpcChannel getChannel() {\n"
    "  return channel;\n"
    "}\n");

  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    map<string, string> vars;
    vars["index"] = SimpleItoa(i);
    vars["method"] = UnderscoresToCamelCase(method);
    vars["input"] = ClassName(method->input_type());
    vars["output"] = ClassName(method->output_type());
    printer->Print(vars,
      "\n"
      "public void $method$(\n"
      "    com.google.protobuf.RpcController controller,\n"
      "    $input$ request,\n"
      "    com.google.protobuf.RpcCallback<$output$> done) {\n"
      "  channel.callMethod(\n"
      "    getDescriptor().getMethods().get($index$),\n"
      "    controller,\n"
      "    request,\n"
      "    $output$.getDefaultInstance(),\n"
      "    com.google.protobuf.RpcUtil.generalizeCallback(\n"
      "      done,\n"
      "      $output$.class,\n"
      "      $output$.getDefaultInstance()));\n"
      "}\n");
  }

  printer->Outdent();
  printer->Print("}\n");
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
