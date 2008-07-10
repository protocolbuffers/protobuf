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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <google/protobuf/compiler/cpp/cpp_service.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

ServiceGenerator::ServiceGenerator(const ServiceDescriptor* descriptor,
                                   const string& dllexport_decl)
  : descriptor_(descriptor) {
  vars_["classname"] = descriptor_->name();
  vars_["full_name"] = descriptor_->full_name();
  if (dllexport_decl.empty()) {
    vars_["dllexport"] = "";
  } else {
    vars_["dllexport"] = dllexport_decl + " ";
  }
}

ServiceGenerator::~ServiceGenerator() {}

void ServiceGenerator::GenerateDeclarations(io::Printer* printer) {
  // Forward-declare the stub type.
  printer->Print(vars_,
    "class $classname$_Stub;\n"
    "\n");

  GenerateInterface(printer);
  GenerateStubDefinition(printer);
}

void ServiceGenerator::GenerateInterface(io::Printer* printer) {
  printer->Print(vars_,
    "class $dllexport$$classname$ : public ::google::protobuf::Service {\n"
    " protected:\n"
    "  // This class should be treated as an abstract interface.\n"
    "  inline $classname$() {};\n"
    " public:\n"
    "  virtual ~$classname$();\n");
  printer->Indent();

  printer->Print(vars_,
    "\n"
    "typedef $classname$_Stub Stub;\n"
    "\n"
    "static const ::google::protobuf::ServiceDescriptor* descriptor();\n"
    "\n");

  GenerateMethodSignatures(VIRTUAL, printer);

  printer->Print(
    "\n"
    "// implements Service ----------------------------------------------\n"
    "\n"
    "const ::google::protobuf::ServiceDescriptor* GetDescriptor();\n"
    "void CallMethod(const ::google::protobuf::MethodDescriptor* method,\n"
    "                ::google::protobuf::RpcController* controller,\n"
    "                const ::google::protobuf::Message* request,\n"
    "                ::google::protobuf::Message* response,\n"
    "                ::google::protobuf::Closure* done);\n"
    "const ::google::protobuf::Message& GetRequestPrototype(\n"
    "  const ::google::protobuf::MethodDescriptor* method) const;\n"
    "const ::google::protobuf::Message& GetResponsePrototype(\n"
    "  const ::google::protobuf::MethodDescriptor* method) const;\n");

  printer->Outdent();
  printer->Print(vars_,
    "\n"
    " private:\n"
    "  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS($classname$);\n"
    "};\n"
    "\n");
}

void ServiceGenerator::GenerateStubDefinition(io::Printer* printer) {
  printer->Print(vars_,
    "class $dllexport$$classname$_Stub : public $classname$ {\n"
    " public:\n");

  printer->Indent();

  printer->Print(vars_,
    "$classname$_Stub(::google::protobuf::RpcChannel* channel);\n"
    "$classname$_Stub(::google::protobuf::RpcChannel* channel,\n"
    "                 ::google::protobuf::Service::ChannelOwnership ownership);\n"
    "~$classname$_Stub();\n"
    "\n"
    "inline ::google::protobuf::RpcChannel* channel() { return channel_; }\n"
    "\n"
    "// implements $classname$ ------------------------------------------\n"
    "\n");

  GenerateMethodSignatures(NON_VIRTUAL, printer);

  printer->Outdent();
  printer->Print(vars_,
    " private:\n"
    "  ::google::protobuf::RpcChannel* channel_;\n"
    "  bool owns_channel_;\n"
    "  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS($classname$_Stub);\n"
    "};\n"
    "\n");
}

void ServiceGenerator::GenerateMethodSignatures(
    VirtualOrNon virtual_or_non, io::Printer* printer) {
  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    map<string, string> sub_vars;
    sub_vars["name"] = method->name();
    sub_vars["input_type"] = ClassName(method->input_type(), true);
    sub_vars["output_type"] = ClassName(method->output_type(), true);
    sub_vars["virtual"] = virtual_or_non == VIRTUAL ? "virtual " : "";

    printer->Print(sub_vars,
      "$virtual$void $name$(::google::protobuf::RpcController* controller,\n"
      "                     const $input_type$* request,\n"
      "                     $output_type$* response,\n"
      "                     ::google::protobuf::Closure* done);\n");
  }
}

// ===================================================================

void ServiceGenerator::GenerateDescriptorInitializer(
    io::Printer* printer, int index) {
  map<string, string> vars;
  vars["classname"] = descriptor_->name();
  vars["index"] = SimpleItoa(index);

  printer->Print(vars,
    "$classname$_descriptor_ = file->service($index$);\n");
}

// ===================================================================

void ServiceGenerator::GenerateImplementation(io::Printer* printer) {
  printer->Print(vars_,
    "$classname$::~$classname$() {}\n"
    "\n"
    "const ::google::protobuf::ServiceDescriptor* $classname$::descriptor() {\n"
    "  return $classname$_descriptor_;\n"
    "}\n"
    "\n"
    "const ::google::protobuf::ServiceDescriptor* $classname$::GetDescriptor() {\n"
    "  return $classname$_descriptor_;\n"
    "}\n"
    "\n");

  // Generate methods of the interface.
  GenerateNotImplementedMethods(printer);
  GenerateCallMethod(printer);
  GenerateGetPrototype(REQUEST, printer);
  GenerateGetPrototype(RESPONSE, printer);

  // Generate stub implementation.
  printer->Print(vars_,
    "$classname$_Stub::$classname$_Stub(::google::protobuf::RpcChannel* channel)\n"
    "  : channel_(channel), owns_channel_(false) {}\n"
    "$classname$_Stub::$classname$_Stub(\n"
    "    ::google::protobuf::RpcChannel* channel,\n"
    "    ::google::protobuf::Service::ChannelOwnership ownership)\n"
    "  : channel_(channel),\n"
    "    owns_channel_(ownership == ::google::protobuf::Service::STUB_OWNS_CHANNEL) {}\n"
    "$classname$_Stub::~$classname$_Stub() {\n"
    "  if (owns_channel_) delete channel_;\n"
    "}\n"
    "\n");

  GenerateStubMethods(printer);
}

void ServiceGenerator::GenerateNotImplementedMethods(io::Printer* printer) {
  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    map<string, string> sub_vars;
    sub_vars["classname"] = descriptor_->name();
    sub_vars["name"] = method->name();
    sub_vars["index"] = SimpleItoa(i);
    sub_vars["input_type"] = ClassName(method->input_type(), true);
    sub_vars["output_type"] = ClassName(method->output_type(), true);

    printer->Print(sub_vars,
      "void $classname$::$name$(::google::protobuf::RpcController* controller,\n"
      "                         const $input_type$* request,\n"
      "                         $output_type$* response,\n"
      "                         ::google::protobuf::Closure* done) {\n"
      "  controller->SetFailed(\"Method $name$() not implemented.\");\n"
      "  done->Run();\n"
      "}\n"
      "\n");
  }
}

void ServiceGenerator::GenerateCallMethod(io::Printer* printer) {
  printer->Print(vars_,
    "void $classname$::CallMethod(const ::google::protobuf::MethodDescriptor* method,\n"
    "                             ::google::protobuf::RpcController* controller,\n"
    "                             const ::google::protobuf::Message* request,\n"
    "                             ::google::protobuf::Message* response,\n"
    "                             ::google::protobuf::Closure* done) {\n"
    "  GOOGLE_DCHECK_EQ(method->service(), $classname$_descriptor_);\n"
    "  switch(method->index()) {\n");

  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    map<string, string> sub_vars;
    sub_vars["name"] = method->name();
    sub_vars["index"] = SimpleItoa(i);
    sub_vars["input_type"] = ClassName(method->input_type(), true);
    sub_vars["output_type"] = ClassName(method->output_type(), true);

    // Note:  ::google::protobuf::down_cast does not work here because it only works on pointers,
    //   not references.
    printer->Print(sub_vars,
      "    case $index$:\n"
      "      $name$(controller,\n"
      "             ::google::protobuf::down_cast<const $input_type$*>(request),\n"
      "             ::google::protobuf::down_cast< $output_type$*>(response),\n"
      "             done);\n"
      "      break;\n");
  }

  printer->Print(vars_,
    "    default:\n"
    "      GOOGLE_LOG(FATAL) << \"Bad method index; this should never happen.\";\n"
    "      break;\n"
    "  }\n"
    "}\n"
    "\n");
}

void ServiceGenerator::GenerateGetPrototype(RequestOrResponse which,
                                            io::Printer* printer) {
  if (which == REQUEST) {
    printer->Print(vars_,
      "const ::google::protobuf::Message& $classname$::GetRequestPrototype(\n");
  } else {
    printer->Print(vars_,
      "const ::google::protobuf::Message& $classname$::GetResponsePrototype(\n");
  }

  printer->Print(vars_,
    "    const ::google::protobuf::MethodDescriptor* method) const {\n"
    "  GOOGLE_DCHECK_EQ(method->service(), $classname$_descriptor_);\n"
    "  switch(method->index()) {\n");

  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    const Descriptor* type =
      (which == REQUEST) ? method->input_type() : method->output_type();

    map<string, string> sub_vars;
    sub_vars["index"] = SimpleItoa(i);
    sub_vars["type"] = ClassName(type, true);

    printer->Print(sub_vars,
      "    case $index$:\n"
      "      return $type$::default_instance();\n");
  }

  printer->Print(vars_,
    "    default:\n"
    "      GOOGLE_LOG(FATAL) << \"Bad method index; this should never happen.\";\n"
    "      return *reinterpret_cast< ::google::protobuf::Message*>(NULL);\n"
    "  }\n"
    "}\n"
    "\n");
}

void ServiceGenerator::GenerateStubMethods(io::Printer* printer) {
  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    map<string, string> sub_vars;
    sub_vars["classname"] = descriptor_->name();
    sub_vars["name"] = method->name();
    sub_vars["index"] = SimpleItoa(i);
    sub_vars["input_type"] = ClassName(method->input_type(), true);
    sub_vars["output_type"] = ClassName(method->output_type(), true);

    printer->Print(sub_vars,
      "void $classname$_Stub::$name$(::google::protobuf::RpcController* controller,\n"
      "                              const $input_type$* request,\n"
      "                              $output_type$* response,\n"
      "                              ::google::protobuf::Closure* done) {\n"
      "  channel_->CallMethod($classname$_descriptor_->method($index$),\n"
      "                       controller, request, response, done);\n"
      "}\n");
  }
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
