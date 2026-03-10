// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/cpp/service.h"

#include <string>

#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/descriptor.h"
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
        class $dllexport_decl $$classname$ : public $pb$::Service {
         protected:
          $classname$() = default;

         public:
          using Stub = $classname$_Stub;

          $classname$(const $classname$&) = delete;
          $classname$& operator=(const $classname$&) = delete;
          virtual ~$classname$() = default;

          static const $pb$::ServiceDescriptor* $nonnull$ descriptor();

          $virts$;

          // implements Service ----------------------------------------------
          const $pb$::ServiceDescriptor* $nonnull$ GetDescriptor() override;

          void CallMethod(
              //~
              const $pb$::MethodDescriptor* $nonnull$ method,
              $pb$::RpcController* $nullable$ controller,
              const $pb$::Message* $nonnull$ request,
              $pb$::Message* $nonnull$ response,
              ::google::protobuf::Closure* $nullable$ done) override;

          const $pb$::Message& GetRequestPrototype(
              const $pb$::MethodDescriptor* $nonnull$ method) const override;

          const $pb$::Message& GetResponsePrototype(
              const $pb$::MethodDescriptor* $nonnull$ method) const override;
        };

        class $dllexport_decl $$classname$_Stub final : public $classname$ {
         public:
          //~ It seems like channel should be nonnull, but some tests use
          //~ nullptr. TODO: clean up and switch to nonnull.
          $classname$_Stub($pb$::RpcChannel* $nullable$ channel);
          $classname$_Stub($pb$::RpcChannel* $nullable$ channel,
                           $pb$::Service::ChannelOwnership ownership);

          $classname$_Stub(const $classname$_Stub&) = delete;
          $classname$_Stub& operator=(const $classname$_Stub&) = delete;

          ~$classname$_Stub() override;

          inline $pb$::RpcChannel* $nullable$ channel() { return channel_; }

          // implements $classname$ ------------------------------------------
          $impls$;

         private:
          $pb$::RpcChannel* $nullable$ channel_;
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
          $virtual $void $name$($pb$::RpcController* $nullable$ controller,
                                const $input$* $nonnull$ request,
                                $output$* $nonnull$ response,
                                ::google::protobuf::Closure* $nullable$ done)$ override$;
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
        const $pb$::ServiceDescriptor* $nonnull$ $classname$::descriptor() {
          $pbi$::AssignDescriptors(&$desc_table$);
          return $file_level_service_descriptors$[$index$];
        }

        const $pb$::ServiceDescriptor* $nonnull$ $classname$::GetDescriptor() {
          return descriptor();
        }

        $no_impl_methods$;

        $call_method$;

        $get_request$;

        $get_response$;

        $classname$_Stub::$classname$_Stub($pb$::RpcChannel* $nullable$ channel)
            : channel_(channel), owns_channel_(false) {}

        $classname$_Stub::$classname$_Stub(
            $pb$::RpcChannel* $nullable$ channel,
            $pb$::Service::ChannelOwnership ownership)
            : channel_(channel),
              owns_channel_(ownership == $pb$::Service::STUB_OWNS_CHANNEL) {}

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
          void $classname$::$name$($pb$::RpcController* $nullable$ controller,
                                   const $input$* $nonnull$,
                                   $output$* $nonnull$,
                                   ::google::protobuf::Closure* $nullable$ done) {
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
            const $pb$::MethodDescriptor* $nonnull$ method,
            $pb$::RpcController* $nullable$ controller,
            const $pb$::Message* $nonnull$ request,
            $pb$::Message* $nonnull$ response, ::google::protobuf::Closure* $nullable$ done) {
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
        const $pb$::Message& $classname$::Get$which$Prototype(
            const $pb$::MethodDescriptor* $nonnull$ method) const {
          ABSL_DCHECK_EQ(method->service(), descriptor());
          switch (method->index()) {
            $cases$;

            default:
              ABSL_LOG(FATAL) << "Bad method index; this should never happen.";
              return *$pb$::MessageFactory::generated_factory()->GetPrototype(
                  method->$which_type$_type());
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
            this->$name$(controller, $pb$::DownCastMessage<$input$>(request),
                         $pb$::DownCastMessage<$output$>(response), done);
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
          void $classname$_Stub::$name$(
              $pb$::RpcController* $nullable$ controller,
              const $input$* $nonnull$ request, $output$* $nonnull$ response,
              ::google::protobuf::Closure* $nullable$ done) {
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
