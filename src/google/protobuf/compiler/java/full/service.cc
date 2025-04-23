// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/full/service.h"

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/doc_comment.h"
#include "google/protobuf/compiler/java/generator_factory.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/compiler/java/names.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

ImmutableServiceGenerator::ImmutableServiceGenerator(
    const ServiceDescriptor* descriptor, Context* context)
    : ServiceGenerator(descriptor),
      context_(context),
      name_resolver_(context->GetNameResolver()) {}

ImmutableServiceGenerator::~ImmutableServiceGenerator() {}

void ImmutableServiceGenerator::Generate(io::Printer* printer) {
  bool is_own_file = IsOwnFile(descriptor_, /* immutable = */ true);
  WriteServiceDocComment(printer, descriptor_, context_->options());
  MaybePrintGeneratedAnnotation(context_, printer, descriptor_,
                                /* immutable = */ true);
  if (!context_->options().opensource_runtime) {
    printer->Print("@com.google.protobuf.Internal.ProtoNonnullApi\n");
  }

  printer->Print(
      "public $static$ abstract class $classname$\n"
      "    implements com.google.protobuf.Service {\n",
      "static", is_own_file ? "" : "static", "classname", descriptor_->name());
  printer->Indent();

  printer->Print("protected $classname$() {}\n\n", "classname",
                 descriptor_->name());

  GenerateInterface(printer);

  GenerateNewReflectiveServiceMethod(printer);
  GenerateNewReflectiveBlockingServiceMethod(printer);

  GenerateAbstractMethods(printer);

  // Generate getDescriptor() and getDescriptorForType().
  printer->Print(
      "public static final\n"
      "    com.google.protobuf.Descriptors.ServiceDescriptor\n"
      "    getDescriptor() {\n"
      "  return $file$.getDescriptor().getServices().get($index$);\n"
      "}\n",
      "file", name_resolver_->GetImmutableClassName(descriptor_->file()),
      "index", absl::StrCat(descriptor_->index()));
  GenerateGetDescriptorForType(printer);

  // Generate more stuff.
  GenerateCallMethod(printer);
  GenerateGetPrototype(REQUEST, printer);
  GenerateGetPrototype(RESPONSE, printer);
  GenerateStub(printer);
  GenerateBlockingStub(printer);

  // Add an insertion point.
  printer->Print(
      "\n"
      "// @@protoc_insertion_point(class_scope:$full_name$)\n",
      "full_name", descriptor_->full_name());

  printer->Outdent();
  printer->Print("}\n\n");
}

void ImmutableServiceGenerator::GenerateGetDescriptorForType(
    io::Printer* printer) {
  printer->Print(
      "public final com.google.protobuf.Descriptors.ServiceDescriptor\n"
      "    getDescriptorForType() {\n"
      "  return getDescriptor();\n"
      "}\n");
}

void ImmutableServiceGenerator::GenerateInterface(io::Printer* printer) {
  printer->Print("public interface Interface {\n");
  printer->Indent();
  GenerateAbstractMethods(printer);
  printer->Outdent();
  printer->Print("}\n\n");
}

void ImmutableServiceGenerator::GenerateNewReflectiveServiceMethod(
    io::Printer* printer) {
  printer->Print(
      "public static com.google.protobuf.Service newReflectiveService(\n"
      "    final Interface impl) {\n"
      "  return new $classname$() {\n",
      "classname", descriptor_->name());
  printer->Indent();
  printer->Indent();

  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    printer->Print("@java.lang.Override\n");
    GenerateMethodSignature(printer, method, IS_CONCRETE);
    printer->Print(
        " {\n"
        "  impl.$method$(controller, request, done);\n"
        "}\n\n",
        "method", UnderscoresToCamelCase(method));
  }

  printer->Outdent();
  printer->Print("};\n");
  printer->Outdent();
  printer->Print("}\n\n");
}

void ImmutableServiceGenerator::GenerateNewReflectiveBlockingServiceMethod(
    io::Printer* printer) {
  printer->Print(
      "public static com.google.protobuf.BlockingService\n"
      "    newReflectiveBlockingService(final BlockingInterface impl) {\n"
      "  return new com.google.protobuf.BlockingService() {\n");
  printer->Indent();
  printer->Indent();

  GenerateGetDescriptorForType(printer);

  GenerateCallBlockingMethod(printer);
  GenerateGetPrototype(REQUEST, printer);
  GenerateGetPrototype(RESPONSE, printer);

  printer->Outdent();
  printer->Print("};\n");
  printer->Outdent();
  printer->Print("}\n\n");
}

void ImmutableServiceGenerator::GenerateAbstractMethods(io::Printer* printer) {
  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    WriteMethodDocComment(printer, method, context_->options());
    GenerateMethodSignature(printer, method, IS_ABSTRACT);
    printer->Print(";\n\n");
  }
}

std::string ImmutableServiceGenerator::GetOutput(
    const MethodDescriptor* method) {
  return name_resolver_->GetImmutableClassName(method->output_type());
}

void ImmutableServiceGenerator::GenerateCallMethod(io::Printer* printer) {
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
    absl::flat_hash_map<absl::string_view, std::string> vars;
    vars["index"] = absl::StrCat(i);
    vars["method"] = UnderscoresToCamelCase(method);
    vars["input"] = name_resolver_->GetImmutableClassName(method->input_type());
    vars["output"] = GetOutput(method);
    printer->Print(
        vars,
        "case $index$:\n"
        "  this.$method$(controller, ($input$)request,\n"
        "    com.google.protobuf.RpcUtil.<$output$>specializeCallback(\n"
        "      done));\n"
        "  return;\n");
  }

  printer->Print(
      "default:\n"
      "  throw new java.lang.AssertionError(\"Can't get here.\");\n");

  printer->Outdent();
  printer->Outdent();

  printer->Print(
      "  }\n"
      "}\n"
      "\n");
}

void ImmutableServiceGenerator::GenerateCallBlockingMethod(
    io::Printer* printer) {
  printer->Print(
      "\n"
      "public final com.google.protobuf.Message callBlockingMethod(\n"
      "    com.google.protobuf.Descriptors.MethodDescriptor method,\n"
      "    com.google.protobuf.RpcController controller,\n"
      "    com.google.protobuf.Message request)\n"
      "    throws com.google.protobuf.ServiceException {\n"
      "  if (method.getService() != getDescriptor()) {\n"
      "    throw new java.lang.IllegalArgumentException(\n"
      "      \"Service.callBlockingMethod() given method descriptor for \" +\n"
      "      \"wrong service type.\");\n"
      "  }\n"
      "  switch(method.getIndex()) {\n");
  printer->Indent();
  printer->Indent();

  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    absl::flat_hash_map<absl::string_view, std::string> vars;
    vars["index"] = absl::StrCat(i);
    vars["method"] = UnderscoresToCamelCase(method);
    vars["input"] = name_resolver_->GetImmutableClassName(method->input_type());
    vars["output"] = GetOutput(method);
    printer->Print(vars,
                   "case $index$:\n"
                   "  return impl.$method$(controller, ($input$)request);\n");
  }

  printer->Print(
      "default:\n"
      "  throw new java.lang.AssertionError(\"Can't get here.\");\n");

  printer->Outdent();
  printer->Outdent();

  printer->Print(
      "  }\n"
      "}\n"
      "\n");
}

void ImmutableServiceGenerator::GenerateGetPrototype(RequestOrResponse which,
                                                     io::Printer* printer) {
  /*
   * TODO: The exception message says "Service.foo" when it may be
   * "BlockingService.foo."  Consider fixing.
   */
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
    absl::flat_hash_map<absl::string_view, std::string> vars;
    vars["index"] = absl::StrCat(i);
    vars["type"] =
        (which == REQUEST)
            ? name_resolver_->GetImmutableClassName(method->input_type())
            : GetOutput(method);
    printer->Print(vars,
                   "case $index$:\n"
                   "  return $type$.getDefaultInstance();\n");
  }

  printer->Print(
      "default:\n"
      "  throw new java.lang.AssertionError(\"Can't get here.\");\n");

  printer->Outdent();
  printer->Outdent();

  printer->Print(
      "  }\n"
      "}\n"
      "\n");
}

void ImmutableServiceGenerator::GenerateStub(io::Printer* printer) {
  printer->Print(
      "public static Stub newStub(\n"
      "    com.google.protobuf.RpcChannel channel) {\n"
      "  return new Stub(channel);\n"
      "}\n"
      "\n"
      "public static final class Stub extends $classname$ implements Interface "
      "{"
      "\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_));
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
    printer->Print("\n");
    GenerateMethodSignature(printer, method, IS_CONCRETE);
    printer->Print(" {\n");
    printer->Indent();

    absl::flat_hash_map<absl::string_view, std::string> vars;
    vars["index"] = absl::StrCat(i);
    vars["output"] = GetOutput(method);
    printer->Print(vars,
                   "channel.callMethod(\n"
                   "  getDescriptor().getMethods().get($index$),\n"
                   "  controller,\n"
                   "  request,\n"
                   "  $output$.getDefaultInstance(),\n"
                   "  com.google.protobuf.RpcUtil.generalizeCallback(\n"
                   "    done,\n"
                   "    $output$.class,\n"
                   "    $output$.getDefaultInstance()));\n");

    printer->Outdent();
    printer->Print("}\n");
  }

  printer->Outdent();
  printer->Print(
      "}\n"
      "\n");
}

void ImmutableServiceGenerator::GenerateBlockingStub(io::Printer* printer) {
  printer->Print(
      "public static BlockingInterface newBlockingStub(\n"
      "    com.google.protobuf.BlockingRpcChannel channel) {\n"
      "  return new BlockingStub(channel);\n"
      "}\n"
      "\n");

  printer->Print("public interface BlockingInterface {");
  printer->Indent();

  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    GenerateBlockingMethodSignature(printer, method);
    printer->Print(";\n");
  }

  printer->Outdent();
  printer->Print(
      "}\n"
      "\n");

  printer->Print(
      "private static final class BlockingStub implements BlockingInterface "
      "{\n");
  printer->Indent();

  printer->Print(
      "private BlockingStub(com.google.protobuf.BlockingRpcChannel channel) {\n"
      "  this.channel = channel;\n"
      "}\n"
      "\n"
      "private final com.google.protobuf.BlockingRpcChannel channel;\n");

  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    GenerateBlockingMethodSignature(printer, method);
    printer->Print(" {\n");
    printer->Indent();

    absl::flat_hash_map<absl::string_view, std::string> vars;
    vars["index"] = absl::StrCat(i);
    vars["output"] = GetOutput(method);
    printer->Print(vars,
                   "return ($output$) channel.callBlockingMethod(\n"
                   "  getDescriptor().getMethods().get($index$),\n"
                   "  controller,\n"
                   "  request,\n"
                   "  $output$.getDefaultInstance());\n");

    printer->Outdent();
    printer->Print(
        "}\n"
        "\n");
  }

  printer->Outdent();
  printer->Print("}\n");
}

void ImmutableServiceGenerator::GenerateMethodSignature(
    io::Printer* printer, const MethodDescriptor* method,
    IsAbstract is_abstract) {
  absl::flat_hash_map<absl::string_view, std::string> vars;
  vars["name"] = UnderscoresToCamelCase(method);
  vars["input"] = name_resolver_->GetImmutableClassName(method->input_type());
  vars["output"] = GetOutput(method);
  vars["abstract"] = (is_abstract == IS_ABSTRACT) ? "abstract" : "";
  printer->Print(vars,
                 "public $abstract$ void $name$(\n"
                 "    com.google.protobuf.RpcController controller,\n"
                 "    $input$ request,\n"
                 "    com.google.protobuf.RpcCallback<$output$> done)");
}

void ImmutableServiceGenerator::GenerateBlockingMethodSignature(
    io::Printer* printer, const MethodDescriptor* method) {
  absl::flat_hash_map<absl::string_view, std::string> vars;
  vars["method"] = UnderscoresToCamelCase(method);
  vars["input"] = name_resolver_->GetImmutableClassName(method->input_type());
  vars["output"] = GetOutput(method);
  printer->Print(vars,
                 "\n"
                 "public $output$ $method$(\n"
                 "    com.google.protobuf.RpcController controller,\n"
                 "    $input$ request)\n"
                 "    throws com.google.protobuf.ServiceException");
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
