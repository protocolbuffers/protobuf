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
  printer->Print(
    "public abstract class $classname$ : pb::IService {\r\n",
    "classname", descriptor_->name());
  printer->Indent();

  // Generate abstract method declarations.
  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    map<string, string> vars;
    vars["name"] = UnderscoresToCapitalizedCamelCase(method);
    vars["input"] = ClassName(method->input_type());
    vars["output"] = ClassName(method->output_type());
    printer->Print(vars,
      "public abstract void $name$(\r\n"
      "    pb::IRpcController controller,\r\n"
      "    $input$ request,\r\n"
      "    global::System.Action<$output$> done);\r\n");
  }

  // Generate Descriptor and DescriptorForType.
  printer->Print(
    "\r\n"
    "public static pbd::ServiceDescriptor Descriptor {\r\n"
    "  get { return $file$.Descriptor.Services[$index$]; }\r\n"
    "}\r\n"
    "public pbd::ServiceDescriptor DescriptorForType {\r\n"
    "  get { return Descriptor; }\r\n"
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
    "public void CallMethod(\r\n"
    "    pbd::MethodDescriptor method,\r\n"
    "    pb::IRpcController controller,\r\n"
    "    pb::IMessage request,\r\n"
    "    global::System.Action<pb::IMessage> done) {\r\n"
    "  if (method.Service != Descriptor) {\r\n"
    "    throw new global::System.ArgumentException(\r\n"
    "      \"Service.CallMethod() given method descriptor for wrong \" +\r\n"
    "      \"service type.\");\r\n"
    "  }\r\n"
    "  switch(method.Index) {\r\n");
  printer->Indent();
  printer->Indent();

  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    map<string, string> vars;
    vars["index"] = SimpleItoa(i);
    vars["method"] = UnderscoresToCapitalizedCamelCase(method);
    vars["input"] = ClassName(method->input_type());
    vars["output"] = ClassName(method->output_type());
    printer->Print(vars,
      "case $index$:\r\n"
      "  this.$method$(controller, ($input$)request,\r\n"
      "    pb::RpcUtil.SpecializeCallback<$output$>(\r\n"
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
    "public pb::IMessage Get$request_or_response$Prototype(pbd::MethodDescriptor method) {\r\n"
    "  if (method.Service != Descriptor) {\r\n"
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
    "public static Stub CreateStub(\r\n"
    "    pb::IRpcChannel channel) {\r\n"
    "  return new Stub(channel);\r\n"
    "}\r\n"
    "\r\n"
    "public class Stub : $classname$ {\r\n",
    "classname", ClassName(descriptor_));
  printer->Indent();

  printer->Print(
    "internal Stub(pb::IRpcChannel channel) {\r\n"
    "  this.channel = channel;\r\n"
    "}\r\n"
    "\r\n"
    "private readonly pb::IRpcChannel channel;\r\n"
    "\r\n"
    "public pb::IRpcChannel Channel {\r\n"
    "  get { return channel; }\r\n"
    "}\r\n");

  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    map<string, string> vars;
    vars["index"] = SimpleItoa(i);
    vars["method"] = UnderscoresToCapitalizedCamelCase(method);
    vars["input"] = ClassName(method->input_type());
    vars["output"] = ClassName(method->output_type());
    printer->Print(vars,
      "\r\n"
      "public override void $method$(\r\n"
      "    pb::IRpcController controller,\r\n"
      "    $input$ request,\r\n"
      "    global::System.Action<$output$> done) {\r\n"
      "  channel.CallMethod(\r\n"
      "    Descriptor.Methods[$index$],\r\n"
      "    controller,\r\n"
      "    request,\r\n"
      "    $output$.DefaultInstance,\r\n"
      "    pb::RpcUtil.GeneralizeCallback(done, $output$.DefaultInstance));\r\n"
      "}\r\n");
  }

  printer->Outdent();
  printer->Print("}\r\n");
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
