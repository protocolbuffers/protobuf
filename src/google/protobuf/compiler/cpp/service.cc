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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/cpp/service.h"

#include <string>

#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
void ServiceGenerator::GenerateDeclarations(io::Printer* printer) {
  auto vars = printer->WithVars(&vars_);
  printer->Emit(
      {
          {"virts", [&] { GenerateMethodSignatures(kVirtual, printer); }},
          {"impls", [&] { GenerateMethodSignatures(kNonVirtual, printer); }},
      },
      R"cc(
        class $classname$_Stub;
        class $dllexport_decl $$classname$ : public ::$proto_ns$::Service {
         protected:
          $classname$() = default;

         public:
          using Stub = $classname$_Stub;

          $classname$(const $classname$&) = delete;
          $classname$& operator=(const $classname$&) = delete;
          virtual ~$classname$() = default;

          static const ::$proto_ns$::ServiceDescriptor* descriptor();

          $virts$;

          // implements Service ----------------------------------------------
          const ::$proto_ns$::ServiceDescriptor* GetDescriptor() override;

          void CallMethod(const ::$proto_ns$::MethodDescriptor* method,
                          ::$proto_ns$::RpcController* controller,
                          const ::$proto_ns$::Message* request,
                          ::$proto_ns$::Message* response,
                          ::google::protobuf::Closure* done) override;

          const ::$proto_ns$::Message& GetRequestPrototype(
              const ::$proto_ns$::MethodDescriptor* method) const override;

          const ::$proto_ns$::Message& GetResponsePrototype(
              const ::$proto_ns$::MethodDescriptor* method) const override;
        };

        class $dllexport_decl $$classname$_Stub final : public $classname$ {
         public:
          $classname$_Stub(::$proto_ns$::RpcChannel* channel);
          $classname$_Stub(::$proto_ns$::RpcChannel* channel,
                           ::$proto_ns$::Service::ChannelOwnership ownership);

          $classname$_Stub(const $classname$_Stub&) = delete;
          $classname$_Stub& operator=(const $classname$_Stub&) = delete;

          ~$classname$_Stub() override;

          inline ::$proto_ns$::RpcChannel* channel() { return channel_; }

          // implements $classname$ ------------------------------------------
          $impls$;

         private:
          ::$proto_ns$::RpcChannel* channel_;
          bool owns_channel_;
        };
      )cc");
}

void ServiceGenerator::GenerateMethodSignatures(VirtualOrNot virtual_or_not,
                                                io::Printer* printer) {
  for (int i = 0; i < descriptor_->method_count(); ++i) {
    const MethodDescriptor* method = descriptor_->method(i);

    printer->Emit(
        {
            {"name", method->name()},
            {"input", QualifiedClassName(method->input_type(), *options_)},
            {"output", QualifiedClassName(method->output_type(), *options_)},
            {"virtual", virtual_or_not == kVirtual ? "virtual" : ""},
            {"override", virtual_or_not != kVirtual ? "override" : ""},
        },
        // No cc, clang-format does not format this string well due to the
        // $ override$ substitution.
        R"(
          $virtual $void $name$(::$proto_ns$::RpcController* controller,
                                const $input$* request,
                                $output$* response,
                                ::google::protobuf::Closure* done)$ override$;
        )");
  }
}

// ===================================================================

void ServiceGenerator::GenerateImplementation(io::Printer* printer) {
  auto vars = printer->WithVars(&vars_);
  printer->Emit(
      {
          {"index", index_in_metadata_},
          {"no_impl_methods", [&] { GenerateNotImplementedMethods(printer); }},
          {"call_method", [&] { GenerateCallMethod(printer); }},
          {"get_request", [&] { GenerateGetPrototype(kRequest, printer); }},
          {"get_response", [&] { GenerateGetPrototype(kResponse, printer); }},
          {"stub_methods", [&] { GenerateStubMethods(printer); }},
      },
      R"cc(
        const ::$proto_ns$::ServiceDescriptor* $classname$::descriptor() {
          ::$proto_ns$::internal::AssignDescriptors(&$desc_table$);
          return $file_level_service_descriptors$[$index$];
        }

        const ::$proto_ns$::ServiceDescriptor* $classname$::GetDescriptor() {
          return descriptor();
        }

        $no_impl_methods$;

        $call_method$;

        $get_request$;

        $get_response$;

        $classname$_Stub::$classname$_Stub(::$proto_ns$::RpcChannel* channel)
            : channel_(channel), owns_channel_(false) {}

        $classname$_Stub::$classname$_Stub(
            ::$proto_ns$::RpcChannel* channel,
            ::$proto_ns$::Service::ChannelOwnership ownership)
            : channel_(channel),
              owns_channel_(ownership ==
                            ::$proto_ns$::Service::STUB_OWNS_CHANNEL) {}

        $classname$_Stub::~$classname$_Stub() {
          if (owns_channel_) delete channel_;
        }

        $stub_methods$;
      )cc");
}

void ServiceGenerator::GenerateNotImplementedMethods(io::Printer* printer) {
  for (int i = 0; i < descriptor_->method_count(); ++i) {
    const MethodDescriptor* method = descriptor_->method(i);

    printer->Emit(
        {
            {"name", method->name()},
            {"input", QualifiedClassName(method->input_type(), *options_)},
            {"output", QualifiedClassName(method->output_type(), *options_)},
        },
        R"cc(
          void $classname$::$name$(::$proto_ns$::RpcController* controller,
                                   const $input$*, $output$*, ::google::protobuf::Closure* done) {
            controller->SetFailed("Method $name$() not implemented.");
            done->Run();
          }
        )cc");
  }
}

void ServiceGenerator::GenerateCallMethod(io::Printer* printer) {
  printer->Emit(
      {
          {"index", absl::StrCat(index_in_metadata_)},
          {"cases", [&] { GenerateCallMethodCases(printer); }},
      },
      R"cc(
        void $classname$::CallMethod(
            const ::$proto_ns$::MethodDescriptor* method,
            ::$proto_ns$::RpcController* controller,
            const ::$proto_ns$::Message* request,
            ::$proto_ns$::Message* response, ::google::protobuf::Closure* done) {
          ABSL_DCHECK_EQ(method->service(), $file_level_service_descriptors$[$index$]);
          switch (method->index()) {
            $cases$;

            default:
              ABSL_LOG(FATAL) << "Bad method index; this should never happen.";
              break;
          }
        }
      )cc");
}

void ServiceGenerator::GenerateGetPrototype(RequestOrResponse which,
                                            io::Printer* printer) {
  printer->Emit(
      {
          {"which", which == kRequest ? "Request" : "Response"},
          {"which_type", which == kRequest ? "input" : "output"},
          {"cases",
           [&] {
             for (int i = 0; i < descriptor_->method_count(); ++i) {
               const MethodDescriptor* method = descriptor_->method(i);
               const Descriptor* type = which == kRequest
                                            ? method->input_type()
                                            : method->output_type();

               printer->Emit(
                   {
                       {"index", absl::StrCat(i)},
                       {"type", QualifiedClassName(type, *options_)},
                   },
                   R"cc(
                     case $index$:
                       return $type$::default_instance();
                   )cc");
             }
           }},
      },
      R"cc(
        const ::$proto_ns$::Message& $classname$::Get$which$Prototype(
            const ::$proto_ns$::MethodDescriptor* method) const {
          ABSL_DCHECK_EQ(method->service(), descriptor());
          switch (method->index()) {
            $cases$;

            default:
              ABSL_LOG(FATAL) << "Bad method index; this should never happen.";
              return *::$proto_ns$::MessageFactory::generated_factory()
                          ->GetPrototype(method->$which_type$_type());
          }
        }
      )cc");
}

void ServiceGenerator::GenerateCallMethodCases(io::Printer* printer) {
  for (int i = 0; i < descriptor_->method_count(); ++i) {
    const MethodDescriptor* method = descriptor_->method(i);
    printer->Emit(
        {
            {"name", method->name()},
            {"input", QualifiedClassName(method->input_type(), *options_)},
            {"output", QualifiedClassName(method->output_type(), *options_)},
            {"index", absl::StrCat(i)},
        },
        R"cc(
          case $index$:
            $name$(controller,
                   ::$proto_ns$::internal::DownCast<const $input$*>(request),
                   ::$proto_ns$::internal::DownCast<$output$*>(response), done);
            break;
        )cc");
  }
}

void ServiceGenerator::GenerateStubMethods(io::Printer* printer) {
  for (int i = 0; i < descriptor_->method_count(); ++i) {
    const MethodDescriptor* method = descriptor_->method(i);

    printer->Emit(
        {
            {"name", method->name()},
            {"input", QualifiedClassName(method->input_type(), *options_)},
            {"output", QualifiedClassName(method->output_type(), *options_)},
            {"index", absl::StrCat(i)},
        },
        R"cc(
          void $classname$_Stub::$name$(::$proto_ns$::RpcController* controller,
                                        const $input$* request,
                                        $output$* response, ::google::protobuf::Closure* done) {
            channel_->CallMethod(descriptor()->method($index$), controller,
                                 request, response, done);
          }
        )cc");
  }
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
