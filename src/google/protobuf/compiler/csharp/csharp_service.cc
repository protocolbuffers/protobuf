// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: jonskeet@google.com (Jon Skeet)
//  Following the Java generator by kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <google/protobuf/compiler/csharp/csharp_service.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

ServiceGenerator::ServiceGenerator(const ServiceDescriptor* descriptor)
  : descriptor_(descriptor) {}

ServiceGenerator::~ServiceGenerator() {}

void ServiceGenerator::Generate(io::Printer* printer) {
  bool is_own_file = descriptor_->file()->options().csharp_multiple_files();
  printer->Print(
    "public $static$ abstract class $classname$\r\n"
    "    implements pb::Service {\r\n",
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
      "public abstract void $name$(\r\n"
      "    pb::RpcController controller,\r\n"
      "    $input$ request,\r\n"
      "    pb::RpcCallback<$output$> done);\r\n");
  }

  // Generate getDescriptor() and getDescriptorForType().
  printer->Print(
    "\r\n"
    "public static final\r\n"
    "    pbd::ServiceDescriptor\r\n"
    "    getDescriptor() {\r\n"
    "  return $file$.getDescriptor().getServices().get($index$);\r\n"
    "}\r\n"
    "public final pbd::ServiceDescriptor\r\n"
    "    DescriptorForType {\r\n"
    "  return getDescriptor();\r\n"
    "}\r\n",
    "file", ClassName(descriptor_->file()),
    "index", SimpleItoa(descriptor_->index()));

  // Generate more stuff.
  GenerateCallMethod(printer);
  GenerateGetPrototype(REQUEST, printer);
  GenerateGetPrototype(RESPONSE, printer);
  GenerateStub(printer);

  printer->Outdent();
  printer->Print("}\r\n\r\n");
}

void ServiceGenerator::GenerateCallMethod(io::Printer* printer) {
  printer->Print(
    "\r\n"
    "public final void callMethod(\r\n"
    "    pbd::MethodDescriptor method,\r\n"
    "    pb::RpcController controller,\r\n"
    "    pb::IMessage request,\r\n"
    "    pb::RpcCallback<\r\n"
    "      pb::Message> done) {\r\n"
    "  if (method.getService() != getDescriptor()) {\r\n"
    "    throw new global::System.ArgumentException(\r\n"
    "      \"Service.CallMethod() given method descriptor for wrong \" +\r\n"
    "      \"service type.\");\r\n"
    "  }\r\n"
    "  switch(method.getIndex()) {\r\n");
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
      "case $index$:\r\n"
      "  this.$method$(controller, ($input$)request,\r\n"
      "    pb::RpcUtil.<$output$>specializeCallback(\r\n"
      "      done));\r\n"
      "  return;\r\n");
  }

  printer->Print(
    "default:\r\n"
    "  throw new global::System.InvalidOperationException(\"Can't get here.\");\r\n");

  printer->Outdent();
  printer->Outdent();

  printer->Print(
    "  }\r\n"
    "}\r\n"
    "\r\n");
}

void ServiceGenerator::GenerateGetPrototype(RequestOrResponse which,
                                            io::Printer* printer) {
  printer->Print(
    "public final pb::Message\r\n"
    "    Get$request_or_response$Prototype(\r\n"
    "    pbd::MethodDescriptor method) {\r\n"
    "  if (method.getService() != getDescriptor()) {\r\n"
    "    throw new global::System.ArgumentException(\r\n"
    "      \"Service.Get$request_or_response$Prototype() given method \" +\r\n"
    "      \"descriptor for wrong service type.\");\r\n"
    "  }\r\n"
    "  switch(method.Index) {\r\n",
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
      "case $index$:\r\n"
      "  return $type$.DefaultInstance;\r\n");
  }

  printer->Print(
    "default:\r\n"
    "  throw new global::System.ArgumentException(\"Can't get here.\");\r\n");

  printer->Outdent();
  printer->Outdent();

  printer->Print(
    "  }\r\n"
    "}\r\n"
    "\r\n");
}

void ServiceGenerator::GenerateStub(io::Printer* printer) {
  printer->Print(
    "public static Stub newStub(\r\n"
    "    pb::RpcChannel channel) {\r\n"
    "  return new Stub(channel);\r\n"
    "}\r\n"
    "\r\n"
    "public static final class Stub extends $classname$ {\r\n",
    "classname", ClassName(descriptor_));
  printer->Indent();

  printer->Print(
    "private Stub(pb::RpcChannel channel) {\r\n"
    "  this.channel = channel;\r\n"
    "}\r\n"
    "\r\n"
    "private final pb::RpcChannel channel;\r\n"
    "\r\n"
    "public pb::RpcChannel getChannel() {\r\n"
    "  return channel;\r\n"
    "}\r\n");

  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    map<string, string> vars;
    vars["index"] = SimpleItoa(i);
    vars["method"] = UnderscoresToCamelCase(method);
    vars["input"] = ClassName(method->input_type());
    vars["output"] = ClassName(method->output_type());
    printer->Print(vars,
      "\r\n"
      "public void $method$(\r\n"
      "    pb::RpcController controller,\r\n"
      "    $input$ request,\r\n"
      "    pb::RpcCallback<$output$> done) {\r\n"
      "  channel.callMethod(\r\n"
      "    Descriptor.Methods[$index$],\r\n"
      "    controller,\r\n"
      "    request,\r\n"
      "    $output$.DefaultInstance,\r\n"
      "    pb::RpcUtil.generalizeCallback(\r\n"
      "      done,\r\n"
      "      typeof ($output$),\r\n"
      "      $output$.DefaultInstance));\r\n"
      "}\r\n");
  }

  printer->Outdent();
  printer->Print("}\r\n");
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
