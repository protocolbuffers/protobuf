// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/file.h"

#include <memory>
#include <string>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/generator_common.h"
#include "google/protobuf/compiler/java/generator_factory.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/full/generator_factory.h"
#include "google/protobuf/compiler/java/internal_helpers.h"
#include "google/protobuf/compiler/java/lite/generator_factory.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/compiler/java/options.h"
#include "google/protobuf/compiler/java/shared_code_generator.h"
#include "google/protobuf/compiler/retention.h"
#include "google/protobuf/compiler/versions.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor_visitor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"


// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

namespace {

struct FieldDescriptorCompare {
  bool operator()(const FieldDescriptor* f1, const FieldDescriptor* f2) const {
    if (f1 == nullptr) {
      return false;
    }
    if (f2 == nullptr) {
      return true;
    }
    return f1->full_name() < f2->full_name();
  }
};

using FieldDescriptorSet =
    absl::btree_set<const FieldDescriptor*, FieldDescriptorCompare>;

// Recursively searches the given message to collect extensions.
// Returns true if all the extensions can be recognized. The extensions will be
// appended in to the extensions parameter.
// Returns false when there are unknown fields, in which case the data in the
// extensions output parameter is not reliable and should be discarded.
bool CollectExtensions(const Message& message, FieldDescriptorSet* extensions) {
  const Reflection* reflection = message.GetReflection();

  // There are unknown fields that could be extensions, thus this call fails.
  if (reflection->GetUnknownFields(message).field_count() > 0) return false;

  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);

  for (int i = 0; i < fields.size(); i++) {
    if (fields[i]->is_extension()) {
      extensions->insert(fields[i]);
    }

    if (GetJavaType(fields[i]) == JAVATYPE_MESSAGE) {
      if (fields[i]->is_repeated()) {
        int size = reflection->FieldSize(message, fields[i]);
        for (int j = 0; j < size; j++) {
          const Message& sub_message =
              reflection->GetRepeatedMessage(message, fields[i], j);
          if (!CollectExtensions(sub_message, extensions)) return false;
        }
      } else {
        const Message& sub_message = reflection->GetMessage(message, fields[i]);
        if (!CollectExtensions(sub_message, extensions)) return false;
      }
    }
  }

  return true;
}

// Finds all extensions for custom options in the given file descriptor with the
// builder pool which resolves Java features instead of the generated pool.
void CollectExtensions(const FileDescriptor& file,
                       FieldDescriptorSet* extensions) {
  FileDescriptorProto file_proto = StripSourceRetentionOptions(file);
  std::string file_data;
  file_proto.SerializeToString(&file_data);
  const Descriptor* file_proto_desc = file.pool()->FindMessageTypeByName(
      file_proto.GetDescriptor()->full_name());

  // descriptor.proto is not found in the builder pool, meaning there are no
  // custom options.
  if (file_proto_desc == nullptr) return;

  DynamicMessageFactory factory;
  std::unique_ptr<Message> dynamic_file_proto(
      factory.GetPrototype(file_proto_desc)->New());
  ABSL_CHECK(dynamic_file_proto.get() != nullptr);
  ABSL_CHECK(dynamic_file_proto->ParseFromString(file_data));

  // Collect the extensions again from the dynamic message.
  extensions->clear();
  ABSL_CHECK(CollectExtensions(*dynamic_file_proto, extensions))
      << "Found unknown fields in FileDescriptorProto when building "
      << file_proto.name()
      << ". It's likely that those fields are custom options, however, "
         "those options cannot be recognized in the builder pool. "
         "This normally should not happen. Please report a bug.";
}

// Our static initialization methods can become very, very large.
// So large that if we aren't careful we end up blowing the JVM's
// 64K bytes of bytecode/method. Fortunately, since these static
// methods are executed only once near the beginning of a program,
// there's usually plenty of stack space available and we can
// extend our methods by simply chaining them to another method
// with a tail call. This inserts the sequence call-next-method,
// end this one, begin-next-method as needed.
void MaybeRestartJavaMethod(io::Printer* printer, int* bytecode_estimate,
                            int* method_num, const char* chain_statement,
                            const char* method_decl) {
  // The goal here is to stay under 64K bytes of jvm bytecode/method,
  // since otherwise we hit a hardcoded limit in the jvm and javac will
  // then fail with the error "code too large". This limit lets our
  // estimates be off by a factor of two and still we're okay.
  static const int bytesPerMethod = kMaxStaticSize;

  if ((*bytecode_estimate) > bytesPerMethod) {
    ++(*method_num);
    printer->Print(chain_statement, "method_num", absl::StrCat(*method_num));
    printer->Outdent();
    printer->Print("}\n");
    printer->Print(method_decl, "method_num", absl::StrCat(*method_num));
    printer->Indent();
    *bytecode_estimate = 0;
  }
}

std::unique_ptr<GeneratorFactory> CreateGeneratorFactory(
    const FileDescriptor* file, const Options& options, Context* context,
    bool immutable_api) {
  ABSL_CHECK(immutable_api)
      << "Open source release does not support the mutable API";
  if (HasDescriptorMethods(file, context->EnforceLite())) {
    return MakeImmutableGeneratorFactory(context);
  } else {
    return MakeImmutableLiteGeneratorFactory(context);
  }
}
}  // namespace

FileGenerator::FileGenerator(const FileDescriptor* file, const Options& options,
                             bool immutable_api)
    : file_(file),
      java_package_(FileJavaPackage(file, immutable_api, options)),
      message_generators_(file->message_type_count()),
      extension_generators_(file->extension_count()),
      context_(new Context(file, options)),
      generator_factory_(
          CreateGeneratorFactory(file, options, context_.get(), immutable_api)),
      name_resolver_(context_->GetNameResolver()),
      options_(options),
      immutable_api_(immutable_api) {
  classname_ = name_resolver_->GetFileClassName(file, immutable_api);
  for (int i = 0; i < file_->message_type_count(); ++i) {
    message_generators_[i] =
        generator_factory_->NewMessageGenerator(file_->message_type(i));
  }
  for (int i = 0; i < file_->extension_count(); ++i) {
    extension_generators_[i] =
        generator_factory_->NewExtensionGenerator(file_->extension(i));
  }
}

FileGenerator::~FileGenerator() {}

bool FileGenerator::Validate(std::string* error) {
  // Check that no class name matches the file's class name.  This is a common
  // problem that leads to Java compile errors that can be hard to understand.
  // It's especially bad when using the java_multiple_files, since we would
  // end up overwriting the outer class with one of the inner ones.
  if (name_resolver_->HasConflictingClassName(file_, classname_,
                                              NameEquality::EXACT_EQUAL)) {
    *error = absl::StrCat(
        file_->name(),
        ": Cannot generate Java output because the file's outer class name, "
        "\"",
        classname_,
        "\", matches the name of one of the types declared inside it.  "
        "Please either rename the type or use the java_outer_classname "
        "option to specify a different outer class name for the .proto file.");
    return false;
  }
  // Similar to the check above, but ignore the case this time. This is not a
  // problem on Linux, but will lead to Java compile errors on Windows / Mac
  // because filenames are case-insensitive on those platforms.
  if (name_resolver_->HasConflictingClassName(
          file_, classname_, NameEquality::EQUAL_IGNORE_CASE)) {
    ABSL_LOG(WARNING)
        << file_->name() << ": The file's outer class name, \"" << classname_
        << "\", matches the name of one of the types declared inside it when "
        << "case is ignored. This can cause compilation issues on Windows / "
        << "MacOS. Please either rename the type or use the "
        << "java_outer_classname option to specify a different outer class "
        << "name for the .proto file to be safe.";
  }

  // Check that no field is a closed enum with implicit presence. For normal
  // cases this will be rejected by protoc before the generator is invoked, but
  // for cases like legacy_closed_enum it may reach the generator.
  google::protobuf::internal::VisitDescriptors(*file_, [&](const FieldDescriptor& field) {
    if (field.enum_type() != nullptr && !SupportUnknownEnumValue(&field) &&
        !field.has_presence() && !field.is_repeated()) {
      absl::StrAppend(error, "Field ", field.full_name(),
                      " has a closed enum type with implicit presence.\n");
    }
  });

  // Print a warning if optimize_for = LITE_RUNTIME is used.
  if (file_->options().optimize_for() == FileOptions::LITE_RUNTIME &&
      !options_.enforce_lite) {
    ABSL_LOG(WARNING)
        << "The optimize_for = LITE_RUNTIME option is no longer supported by "
        << "protobuf Java code generator and is ignored--protoc will always "
        << "generate full runtime code for Java. To use Java Lite runtime, "
        << "users should use the Java Lite plugin instead. See:\n"
        << "  "
           "https://github.com/protocolbuffers/protobuf/blob/main/java/"
           "lite.md";
  }
  google::protobuf::internal::VisitDescriptors(*file_, [&](const EnumDescriptor& enm) {
    if (CheckLargeEnum(&enm) && enm.is_closed()) {
      absl::StrAppend(
          error, enm.full_name(),
          " is a closed enum and can not be used with the large_enum feature.  "
          "Please migrate to an open enum first, which is a better fit for "
          "extremely large enums.\n");
    }
    absl::Status status = ValidateNestInFileClassFeature(enm);
    if (!status.ok()) {
      absl::StrAppend(error, status.message());
    }
  });

  google::protobuf::internal::VisitDescriptors(*file_, [&](const Descriptor& message) {
    absl::Status status = ValidateNestInFileClassFeature(message);
    if (!status.ok()) {
      absl::StrAppend(error, status.message());
    }
  });

  return error->empty();
}

void FileGenerator::Generate(io::Printer* printer) {
  // We don't import anything because we refer to all classes by their
  // fully-qualified names in the generated source.
  printer->Print(
      "// Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
      "// NO CHECKED-IN PROTOBUF "
      // Intentional line breaker
      "GENCODE\n"
      "// source: $filename$\n",
      "filename", file_->name());
  if (options_.opensource_runtime) {
    printer->Print("// Protobuf Java Version: $protobuf_java_version$\n",
                   "protobuf_java_version", PROTOBUF_JAVA_VERSION_STRING);
  }
  printer->Print("\n");
  if (!java_package_.empty()) {
    printer->Print(
        "package $package$;\n"
        "\n",
        "package", java_package_);
  }
  PrintGeneratedAnnotation(
      printer, '$',
      options_.annotate_code ? absl::StrCat(classname_, ".java.pb.meta") : "",
      options_);

  if (!options_.opensource_runtime) {
    printer->Print("@com.google.protobuf.Internal.ProtoNonnullApi\n");
  }
  printer->Print(
      "$deprecation$public final class $classname$ {\n"
      "  private $ctor$() {}\n",
      "deprecation",
      file_->options().deprecated() ? "@java.lang.Deprecated " : "",
      "classname", classname_, "ctor", classname_);
  printer->Annotate("classname", file_->name());
  printer->Indent();

  if (!context_->EnforceLite()) {
    printer->Print("static {\n");
    printer->Indent();
    PrintGencodeVersionValidator(printer, options_.opensource_runtime,
                                 classname_);
    printer->Outdent();
    printer->Print("}\n");
  }

  // -----------------------------------------------------------------

  printer->Print(
      "public static void registerAllExtensions(\n"
      "    com.google.protobuf.ExtensionRegistryLite registry) {\n");

  printer->Indent();

  for (int i = 0; i < file_->extension_count(); i++) {
    extension_generators_[i]->GenerateRegistrationCode(printer);
  }

  for (int i = 0; i < file_->message_type_count(); i++) {
    message_generators_[i]->GenerateExtensionRegistrationCode(printer);
  }

  printer->Outdent();
  printer->Print("}\n");
  if (HasDescriptorMethods(file_, context_->EnforceLite())) {
    // Overload registerAllExtensions for the non-lite usage to
    // redundantly maintain the original signature (this is
    // redundant because ExtensionRegistryLite now invokes
    // ExtensionRegistry in the non-lite usage). Intent is
    // to remove this in the future.
    printer->Print(
        "\n"
        "public static void registerAllExtensions(\n"
        "    com.google.protobuf.ExtensionRegistry registry) {\n"
        "  registerAllExtensions(\n"
        "      (com.google.protobuf.ExtensionRegistryLite) registry);\n"
        "}\n");
  }

  // -----------------------------------------------------------------

  for (int i = 0; i < file_->enum_type_count(); i++) {
    if (NestedInFileClass(*file_->enum_type(i), immutable_api_)) {
      generator_factory_->NewEnumGenerator(file_->enum_type(i))
          ->Generate(printer);
    }
  }
  for (int i = 0; i < file_->message_type_count(); i++) {
    if (NestedInFileClass(*file_->message_type(i), immutable_api_)) {
      message_generators_[i]->GenerateInterface(printer);
      message_generators_[i]->Generate(printer);
    }
  }
  if (HasGenericServices(file_, context_->EnforceLite())) {
    for (int i = 0; i < file_->service_count(); i++) {
      if (NestedInFileClass(*file_->service(i), immutable_api_)) {
        std::unique_ptr<ServiceGenerator> generator(
            generator_factory_->NewServiceGenerator(file_->service(i)));
        generator->Generate(printer);
      }
    }
  }

  // Extensions must be generated in the outer class since they are values,
  // not classes.
  for (int i = 0; i < file_->extension_count(); i++) {
    extension_generators_[i]->Generate(printer);
  }

  // Static variables. We'd like them to be final if possible, but due to
  // the JVM's 64k size limit on static blocks, we have to initialize some
  // of them in methods; thus they cannot be final.
  int static_block_bytecode_estimate = 0;
  for (int i = 0; i < file_->message_type_count(); i++) {
    message_generators_[i]->GenerateStaticVariables(
        printer, &static_block_bytecode_estimate);
  }

  printer->Print("\n");

  if (HasDescriptorMethods(file_, context_->EnforceLite())) {
    if (immutable_api_) {
      GenerateDescriptorInitializationCodeForImmutable(printer);
    }
  } else {
    printer->Print("static {\n");
    printer->Indent();
    int bytecode_estimate = 0;
    int method_num = 0;

    for (int i = 0; i < file_->message_type_count(); i++) {
      bytecode_estimate +=
          message_generators_[i]->GenerateStaticVariableInitializers(printer);
      MaybeRestartJavaMethod(
          printer, &bytecode_estimate, &method_num,
          "_clinit_autosplit_$method_num$();\n",
          "private static void _clinit_autosplit_$method_num$() {\n");
    }

    printer->Outdent();
    printer->Print("}\n");
  }

  printer->Print(
      "\n"
      "// @@protoc_insertion_point(outer_class_scope)\n");

  printer->Outdent();
  printer->Print("}\n");
}

void FileGenerator::GenerateDescriptorInitializationCodeForImmutable(
    io::Printer* printer) {
  printer->Print(
      "public static com.google.protobuf.Descriptors.FileDescriptor\n"
      "    getDescriptor() {\n"
      "  return descriptor;\n"
      "}\n"
      "private static $final$ com.google.protobuf.Descriptors.FileDescriptor\n"
      "    descriptor;\n"
      "static {\n",
      // TODO: Mark this as final.
      "final", options_.opensource_runtime ? "" : "final");
  printer->Indent();

  if (options_.opensource_runtime) {
    SharedCodeGenerator shared_code_generator(file_, options_);
    shared_code_generator.GenerateDescriptors(printer);
  } else {
  }

  int bytecode_estimate = 0;
  int method_num = 0;

  for (int i = 0; i < file_->message_type_count(); i++) {
    bytecode_estimate +=
        message_generators_[i]->GenerateStaticVariableInitializers(printer);
    MaybeRestartJavaMethod(
        printer, &bytecode_estimate, &method_num,
        "_clinit_autosplit_dinit_$method_num$();\n",
        "private static void _clinit_autosplit_dinit_$method_num$() {\n");
  }
  for (int i = 0; i < file_->extension_count(); i++) {
    bytecode_estimate +=
        extension_generators_[i]->GenerateNonNestedInitializationCode(printer);
    MaybeRestartJavaMethod(
        printer, &bytecode_estimate, &method_num,
        "_clinit_autosplit_dinit_$method_num$();\n",
        "private static void _clinit_autosplit_dinit_$method_num$() {\n");
  }
  // Feature resolution for Java features uses extension registry
  // which must happen after internalInit() from
  // GenerateNonNestedInitializationCode
  printer->Print("descriptor.resolveAllFeaturesImmutable();\n");

  // Proto compiler builds a DescriptorPool, which holds all the descriptors to
  // generate, when processing the ".proto" files. We call this DescriptorPool
  // the parsed pool (a.k.a. file_->pool()).
  //
  // Note that when users try to extend the (.*)DescriptorProto in their
  // ".proto" files, it does not affect the pre-built FileDescriptorProto class
  // in proto compiler. When we put the descriptor data in the file_proto, those
  // extensions become unknown fields.
  //
  // Now we need to find out all the extension value to the (.*)DescriptorProto
  // in the file_proto message, and prepare an ExtensionRegistry to return.
  //
  // To find those extensions, we need to parse the data into a dynamic message
  // of the FileDescriptor based on the builder-pool, then we can use
  // reflections to find all extension fields
  FieldDescriptorSet extensions;
  CollectExtensions(*file_, &extensions);

  if (options_.strip_nonfunctional_codegen) {
    // Skip feature extensions, which are a visible (but non-functional)
    // deviation between editions and legacy syntax.
    absl::erase_if(extensions, [](const FieldDescriptor* field) {
      return field->containing_type()->full_name() == "google.protobuf.FeatureSet";
    });
  }

  // Force descriptor initialization of all dependencies.
  for (int i = 0; i < file_->dependency_count(); i++) {
    if (ShouldIncludeDependency(file_->dependency(i), true)) {
      std::string dependency =
          name_resolver_->GetImmutableClassName(file_->dependency(i));
      printer->Print("$dependency$.getDescriptor();\n", "dependency",
                     dependency);
    }
  }

  if (!extensions.empty()) {
    // Must construct an ExtensionRegistry containing all existing extensions
    // and use it to parse the descriptor data again to recognize extensions.
    printer->Print(
        "com.google.protobuf.ExtensionRegistry registry =\n"
        "    com.google.protobuf.ExtensionRegistry.newInstance();\n");
    FieldDescriptorSet::iterator it;
    for (const FieldDescriptor* field : extensions) {
      std::unique_ptr<ExtensionGenerator> generator(
          generator_factory_->NewExtensionGenerator(field));
      bytecode_estimate += generator->GenerateRegistrationCode(printer);
      MaybeRestartJavaMethod(
          printer, &bytecode_estimate, &method_num,
          "_clinit_autosplit_dinit_$method_num$(registry);\n",
          "private static void _clinit_autosplit_dinit_$method_num$(\n"
          "    com.google.protobuf.ExtensionRegistry registry) {\n");
    }
    printer->Print(
        "com.google.protobuf.Descriptors.FileDescriptor\n"
        "    .internalUpdateFileDescriptor(descriptor, registry);\n");
  }

  printer->Outdent();
  printer->Print("}\n");
}


template <typename GeneratorClass, typename DescriptorClass>
static void GenerateSibling(
    const std::string& package_dir, const std::string& java_package,
    const DescriptorClass* descriptor, GeneratorContext* context,
    std::vector<std::string>* file_list, bool annotate_code,
    std::vector<std::string>* annotation_list, const std::string& name_suffix,
    GeneratorClass* generator, bool opensource_runtime,
    void (GeneratorClass::*pfn)(io::Printer* printer)) {
  std::string filename =
      absl::StrCat(package_dir, descriptor->name(), name_suffix, ".java");
  file_list->push_back(filename);
  std::string info_full_path = absl::StrCat(filename, ".pb.meta");
  GeneratedCodeInfo annotations;
  io::AnnotationProtoCollector<GeneratedCodeInfo> annotation_collector(
      &annotations);

  std::unique_ptr<io::ZeroCopyOutputStream> output(context->Open(filename));
  io::Printer printer(output.get(), '$',
                      annotate_code ? &annotation_collector : nullptr);

  printer.Print(
      "// Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
      "// NO CHECKED-IN PROTOBUF "
      // Intentional line breaker
      "GENCODE\n"
      "// source: $filename$\n",
      "filename", descriptor->file()->name());
  if (opensource_runtime) {
    printer.Print("// Protobuf Java Version: $protobuf_java_version$\n",
                  "protobuf_java_version", PROTOBUF_JAVA_VERSION_STRING);
  }
  printer.Print("\n");
  if (!java_package.empty()) {
    printer.Print(
        "package $package$;\n"
        "\n",
        "package", java_package);
  }

  (generator->*pfn)(&printer);

  if (annotate_code) {
    std::unique_ptr<io::ZeroCopyOutputStream> info_output(
        context->Open(info_full_path));
    annotations.SerializeToZeroCopyStream(info_output.get());
    annotation_list->push_back(info_full_path);
  }
}

void FileGenerator::GenerateSiblings(
    const std::string& package_dir, GeneratorContext* context,
    std::vector<std::string>* file_list,
    std::vector<std::string>* annotation_list) {
  for (int i = 0; i < file_->enum_type_count(); i++) {
    if (NestedInFileClass(*file_->enum_type(i), immutable_api_)) continue;
    std::unique_ptr<EnumGenerator> generator(
        generator_factory_->NewEnumGenerator(file_->enum_type(i)));
    GenerateSibling<EnumGenerator>(
        package_dir, java_package_, file_->enum_type(i), context, file_list,
        options_.annotate_code, annotation_list, "", generator.get(),
        options_.opensource_runtime, &EnumGenerator::Generate);
  }
  for (int i = 0; i < file_->message_type_count(); i++) {
    if (NestedInFileClass(*file_->message_type(i), immutable_api_)) continue;
    if (immutable_api_) {
      GenerateSibling<MessageGenerator>(
          package_dir, java_package_, file_->message_type(i), context,
          file_list, options_.annotate_code, annotation_list, "OrBuilder",
          message_generators_[i].get(), options_.opensource_runtime,
          &MessageGenerator::GenerateInterface);
    }
    GenerateSibling<MessageGenerator>(
        package_dir, java_package_, file_->message_type(i), context, file_list,
        options_.annotate_code, annotation_list, "",
        message_generators_[i].get(), options_.opensource_runtime,
        &MessageGenerator::Generate);
  }
  if (HasGenericServices(file_, context_->EnforceLite())) {
    for (int i = 0; i < file_->service_count(); i++) {
      if (NestedInFileClass(*file_->service(i), immutable_api_)) continue;
      std::unique_ptr<ServiceGenerator> generator(
          generator_factory_->NewServiceGenerator(file_->service(i)));
      GenerateSibling<ServiceGenerator>(
          package_dir, java_package_, file_->service(i), context, file_list,
          options_.annotate_code, annotation_list, "", generator.get(),
          options_.opensource_runtime, &ServiceGenerator::Generate);
    }
  }
}

bool FileGenerator::ShouldIncludeDependency(const FileDescriptor* descriptor,
                                            bool immutable_api) {
  // Skip feature imports, which are a visible (but non-functional) deviation
  // between editions and legacy syntax.
  if (options_.strip_nonfunctional_codegen &&
      IsKnownFeatureProto(descriptor->name())) {
    return false;
  }

  return true;
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
