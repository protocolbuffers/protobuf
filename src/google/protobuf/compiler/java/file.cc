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
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/enum.h"
#include "google/protobuf/compiler/java/enum_lite.h"
#include "google/protobuf/compiler/java/extension.h"
#include "google/protobuf/compiler/java/generator_factory.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/message.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/compiler/java/service.h"
#include "google/protobuf/compiler/java/shared_code_generator.h"
#include "google/protobuf/compiler/retention.h"
#include "google/protobuf/compiler/versions.h"
#include "google/protobuf/descriptor.pb.h"
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
    if (f1 == NULL) {
      return false;
    }
    if (f2 == NULL) {
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

// Finds all extensions in the given message and its sub-messages.  If the
// message contains unknown fields (which could be extensions), then those
// extensions are defined in alternate_pool.
// The message will be converted to a DynamicMessage backed by alternate_pool
// in order to handle this case.
void CollectExtensions(const FileDescriptorProto& file_proto,
                       const DescriptorPool& alternate_pool,
                       FieldDescriptorSet* extensions,
                       const std::string& file_data) {
  if (!CollectExtensions(file_proto, extensions)) {
    // There are unknown fields in the file_proto, which are probably
    // extensions. We need to parse the data into a dynamic message based on the
    // builder-pool to find out all extensions.
    const Descriptor* file_proto_desc = alternate_pool.FindMessageTypeByName(
        file_proto.GetDescriptor()->full_name());
    ABSL_CHECK(file_proto_desc)
        << "Find unknown fields in FileDescriptorProto when building "
        << file_proto.name()
        << ". It's likely that those fields are custom options, however, "
           "descriptor.proto is not in the transitive dependencies. "
           "This normally should not happen. Please report a bug.";
    DynamicMessageFactory factory;
    std::unique_ptr<Message> dynamic_file_proto(
        factory.GetPrototype(file_proto_desc)->New());
    ABSL_CHECK(dynamic_file_proto.get() != NULL);
    ABSL_CHECK(dynamic_file_proto->ParseFromString(file_data));

    // Collect the extensions again from the dynamic message. There should be no
    // more unknown fields this time, i.e. all the custom options should be
    // parsed as extensions now.
    extensions->clear();
    ABSL_CHECK(CollectExtensions(*dynamic_file_proto, extensions))
        << "Find unknown fields in FileDescriptorProto when building "
        << file_proto.name()
        << ". It's likely that those fields are custom options, however, "
           "those options cannot be recognized in the builder pool. "
           "This normally should not happen. Please report a bug.";
  }
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
}  // namespace

FileGenerator::FileGenerator(const FileDescriptor* file, const Options& options,
                             bool immutable_api)
    : file_(file),
      java_package_(FileJavaPackage(file, immutable_api, options)),
      message_generators_(file->message_type_count()),
      extension_generators_(file->extension_count()),
      context_(new Context(file, options)),
      name_resolver_(context_->GetNameResolver()),
      options_(options),
      immutable_api_(immutable_api) {
  classname_ = name_resolver_->GetFileClassName(file, immutable_api);
    generator_factory_.reset(new ImmutableGeneratorFactory(context_.get()));
  for (int i = 0; i < file_->message_type_count(); ++i) {
    message_generators_[i].reset(
        generator_factory_->NewMessageGenerator(file_->message_type(i)));
  }
  for (int i = 0; i < file_->extension_count(); ++i) {
    extension_generators_[i].reset(
        generator_factory_->NewExtensionGenerator(file_->extension(i)));
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
    error->assign(file_->name());
    error->append(
        ": Cannot generate Java output because the file's outer class name, "
        "\"");
    error->append(classname_);
    error->append(
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
  return true;
}

void FileGenerator::Generate(io::Printer* printer) {
  // We don't import anything because we refer to all classes by their
  // fully-qualified names in the generated source.
  printer->Print(
      "// Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
      "// source: $filename$\n"
      "\n",
      "filename", file_->name());
  if (options_.opensource_runtime) {
    printer->Print("// Protobuf Java Version: $protobuf_java_version$\n",
                   "protobuf_java_version", PROTOBUF_JAVA_VERSION_STRING);
  }
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

  if (!MultipleJavaFiles(file_, immutable_api_)) {
    for (int i = 0; i < file_->enum_type_count(); i++) {
      if (HasDescriptorMethods(file_, context_->EnforceLite())) {
        EnumGenerator(file_->enum_type(i), immutable_api_, context_.get())
            .Generate(printer);
      } else {
        EnumLiteGenerator(file_->enum_type(i), immutable_api_, context_.get())
            .Generate(printer);
      }
    }
    for (int i = 0; i < file_->message_type_count(); i++) {
      message_generators_[i]->GenerateInterface(printer);
      message_generators_[i]->Generate(printer);
    }
    if (HasGenericServices(file_, context_->EnforceLite())) {
      for (int i = 0; i < file_->service_count(); i++) {
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
    } else {
      GenerateDescriptorInitializationCodeForMutable(printer);
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
  FileDescriptorProto file_proto = StripSourceRetentionOptions(*file_);
  std::string file_data;
  file_proto.SerializeToString(&file_data);
  FieldDescriptorSet extensions;
  CollectExtensions(file_proto, *file_->pool(), &extensions, file_data);

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

  // Force descriptor initialization of all dependencies.
  for (int i = 0; i < file_->dependency_count(); i++) {
    if (ShouldIncludeDependency(file_->dependency(i), true)) {
      std::string dependency =
          name_resolver_->GetImmutableClassName(file_->dependency(i));
      printer->Print("$dependency$.getDescriptor();\n", "dependency",
                     dependency);
    }
  }

  printer->Outdent();
  printer->Print("}\n");
}

void FileGenerator::GenerateDescriptorInitializationCodeForMutable(
    io::Printer* printer) {
  printer->Print(
      "public static com.google.protobuf.Descriptors.FileDescriptor\n"
      "    getDescriptor() {\n"
      "  return descriptor;\n"
      "}\n"
      "private static final com.google.protobuf.Descriptors.FileDescriptor\n"
      "    descriptor;\n"
      "static {\n");
  printer->Indent();

  printer->Print(
      "descriptor = $immutable_package$.$descriptor_classname$.descriptor;\n",
      "immutable_package", FileJavaPackage(file_, true, options_),
      "descriptor_classname", name_resolver_->GetDescriptorClassName(file_));

  for (int i = 0; i < file_->message_type_count(); i++) {
    message_generators_[i]->GenerateStaticVariableInitializers(printer);
  }
  for (int i = 0; i < file_->extension_count(); i++) {
    extension_generators_[i]->GenerateNonNestedInitializationCode(printer);
  }

  // Check if custom options exist. If any, try to load immutable classes since
  // custom options are only represented with immutable messages.
  FileDescriptorProto file_proto = StripSourceRetentionOptions(*file_);
  std::string file_data;
  file_proto.SerializeToString(&file_data);
  FieldDescriptorSet extensions;
  CollectExtensions(file_proto, *file_->pool(), &extensions, file_data);

  if (!extensions.empty()) {
    // Try to load immutable messages' outer class. Its initialization code
    // will take care of interpreting custom options.
    printer->Print(
        "try {\n"
        // Note that we have to load the immutable class dynamically here as
        // we want the mutable code to be independent from the immutable code
        // at compile time. It is required to implement dual-compile for
        // mutable and immutable API in blaze.
        "  java.lang.Class<?> immutableClass = java.lang.Class.forName(\n"
        "      \"$immutable_classname$\");\n"
        "} catch (java.lang.ClassNotFoundException e) {\n",
        "immutable_classname", name_resolver_->GetImmutableClassName(file_));
    printer->Indent();

    // The immutable class can not be found. We try our best to collect all
    // custom option extensions to interpret the custom options.
    printer->Print(
        "com.google.protobuf.ExtensionRegistry registry =\n"
        "    com.google.protobuf.ExtensionRegistry.newInstance();\n"
        "com.google.protobuf.MessageLite defaultExtensionInstance = null;\n");

    for (const FieldDescriptor* field : extensions) {
      std::string scope;
      if (field->extension_scope() != NULL) {
        scope = absl::StrCat(
            name_resolver_->GetMutableClassName(field->extension_scope()),
            ".getDescriptor()");
      } else {
        scope =
            absl::StrCat(FileJavaPackage(field->file(), true, options_), ".",
                         name_resolver_->GetDescriptorClassName(field->file()),
                         ".descriptor");
      }
      if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        printer->Print(
            "defaultExtensionInstance = com.google.protobuf.Internal\n"
            "    .getDefaultInstance(\"$class$\");\n"
            "if (defaultExtensionInstance != null) {\n"
            "  registry.add(\n"
            "      $scope$.getExtensions().get($index$),\n"
            "      (com.google.protobuf.Message) defaultExtensionInstance);\n"
            "}\n",
            "scope", scope, "index", absl::StrCat(field->index()), "class",
            name_resolver_->GetImmutableClassName(field->message_type()));
      } else {
        printer->Print("registry.add($scope$.getExtensions().get($index$));\n",
                       "scope", scope, "index", absl::StrCat(field->index()));
      }
    }
    printer->Print(
        "com.google.protobuf.Descriptors.FileDescriptor\n"
        "    .internalUpdateFileDescriptor(descriptor, registry);\n");

    printer->Outdent();
    printer->Print("}\n");
  }

  // Force descriptor initialization of all dependencies.
  for (int i = 0; i < file_->dependency_count(); i++) {
    if (ShouldIncludeDependency(file_->dependency(i), false)) {
      std::string dependency =
          name_resolver_->GetMutableClassName(file_->dependency(i));
      printer->Print("$dependency$.getDescriptor();\n", "dependency",
                     dependency);
    }
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
    GeneratorClass* generator,
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
                      annotate_code ? &annotation_collector : NULL);

  printer.Print(
      "// Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
      "// source: $filename$\n"
      "\n",
      "filename", descriptor->file()->name());
  printer.Print("// Protobuf Java Version: $protobuf_java_version$\n",
                "protobuf_java_version", PROTOBUF_JAVA_VERSION_STRING);
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
  if (MultipleJavaFiles(file_, immutable_api_)) {
    for (int i = 0; i < file_->enum_type_count(); i++) {
      if (HasDescriptorMethods(file_, context_->EnforceLite())) {
        EnumGenerator generator(file_->enum_type(i), immutable_api_,
                                context_.get());
        GenerateSibling<EnumGenerator>(
            package_dir, java_package_, file_->enum_type(i), context, file_list,
            options_.annotate_code, annotation_list, "", &generator,
            &EnumGenerator::Generate);
      } else {
        EnumLiteGenerator generator(file_->enum_type(i), immutable_api_,
                                    context_.get());
        GenerateSibling<EnumLiteGenerator>(
            package_dir, java_package_, file_->enum_type(i), context, file_list,
            options_.annotate_code, annotation_list, "", &generator,
            &EnumLiteGenerator::Generate);
      }
    }
    for (int i = 0; i < file_->message_type_count(); i++) {
      if (immutable_api_) {
        GenerateSibling<MessageGenerator>(
            package_dir, java_package_, file_->message_type(i), context,
            file_list, options_.annotate_code, annotation_list, "OrBuilder",
            message_generators_[i].get(), &MessageGenerator::GenerateInterface);
      }
      GenerateSibling<MessageGenerator>(
          package_dir, java_package_, file_->message_type(i), context,
          file_list, options_.annotate_code, annotation_list, "",
          message_generators_[i].get(), &MessageGenerator::Generate);
    }
    if (HasGenericServices(file_, context_->EnforceLite())) {
      for (int i = 0; i < file_->service_count(); i++) {
        std::unique_ptr<ServiceGenerator> generator(
            generator_factory_->NewServiceGenerator(file_->service(i)));
        GenerateSibling<ServiceGenerator>(
            package_dir, java_package_, file_->service(i), context, file_list,
            options_.annotate_code, annotation_list, "", generator.get(),
            &ServiceGenerator::Generate);
      }
    }
  }
}

std::string FileGenerator::GetKotlinClassname() {
  return name_resolver_->GetFileClassName(file_, immutable_api_, true);
}

void FileGenerator::GenerateKotlin(io::Printer* printer) {
  printer->Print(
      "// Generated by the protocol buffer compiler. DO NOT EDIT!\n"
      "// source: $filename$\n"
      "\n",
      "filename", file_->name());
  printer->Print(
      "// Generated files should ignore deprecation warnings\n"
      "@file:Suppress(\"DEPRECATION\")\n");
  if (!java_package_.empty()) {
    printer->Print(
        "package $package$;\n"
        "\n",
        "package", EscapeKotlinKeywords(java_package_));
  }
}

void FileGenerator::GenerateKotlinSiblings(
    const std::string& package_dir, GeneratorContext* context,
    std::vector<std::string>* file_list,
    std::vector<std::string>* annotation_list) {
  for (int i = 0; i < file_->message_type_count(); i++) {
    const Descriptor* descriptor = file_->message_type(i);
    MessageGenerator* generator = message_generators_[i].get();
    auto open_file = [context](const std::string& filename) {
      return std::unique_ptr<io::ZeroCopyOutputStream>(context->Open(filename));
    };
    std::string filename =
        absl::StrCat(package_dir, descriptor->name(), "Kt.kt");
    file_list->push_back(filename);
    std::string info_full_path = absl::StrCat(filename, ".pb.meta");
    GeneratedCodeInfo annotations;
    io::AnnotationProtoCollector<GeneratedCodeInfo> annotation_collector(
        &annotations);
    auto output = open_file(filename);
    io::Printer printer(
        output.get(), '$',
        options_.annotate_code ? &annotation_collector : nullptr);

    printer.Print(
        "// Generated by the protocol buffer compiler. DO NOT EDIT!\n"
        "// source: $filename$\n"
        "\n",
        "filename", descriptor->file()->name());
    printer.Print(
        "// Generated files should ignore deprecation warnings\n"
        "@file:Suppress(\"DEPRECATION\")\n");
    if (!java_package_.empty()) {
      printer.Print(
          "package $package$;\n"
          "\n",
          "package", EscapeKotlinKeywords(java_package_));
    }

    generator->GenerateKotlinMembers(&printer);
    generator->GenerateTopLevelKotlinMembers(&printer);

    if (options_.annotate_code) {
      auto info_output = open_file(info_full_path);
      annotations.SerializeToZeroCopyStream(info_output.get());
      annotation_list->push_back(info_full_path);
    }
  }
}

bool FileGenerator::ShouldIncludeDependency(const FileDescriptor* descriptor,
                                            bool immutable_api) {
  return true;
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
