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

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_HELPERS_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_HELPERS_H__

#include <map>
#include <string>
#include <google/protobuf/compiler/cpp/cpp_options.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

// Commonly-used separator comments.  Thick is a line of '=', thin is a line
// of '-'.
extern const char kThickSeparator[];
extern const char kThinSeparator[];

// Name space of the proto file. This namespace is such that the string
// "<namespace>::some_name" is the correct fully qualified namespace.
// This means if the package is empty the namespace is "", and otherwise
// the namespace is "::foo::bar::...::baz" without trailing semi-colons.
string Namespace(const string& package);
inline string Namespace(const FileDescriptor* d) {
  return Namespace(d->package());
}
template <typename Desc>
string Namespace(const Desc* d) {
  return Namespace(d->file());
}

// Returns true if it's safe to reset "field" to zero.
bool CanInitializeByZeroing(const FieldDescriptor* field);

string ClassName(const Descriptor* descriptor);
string ClassName(const EnumDescriptor* enum_descriptor);
template <typename Desc>
string QualifiedClassName(const Desc* d) {
  return Namespace(d) + "::" + ClassName(d);
}

// DEPRECATED just use ClassName or QualifiedClassName, a boolean is very
// unreadable at the callsite.
// Returns the non-nested type name for the given type.  If "qualified" is
// true, prefix the type with the full namespace.  For example, if you had:
//   package foo.bar;
//   message Baz { message Qux {} }
// Then the qualified ClassName for Qux would be:
//   ::foo::bar::Baz_Qux
// While the non-qualified version would be:
//   Baz_Qux
inline string ClassName(const Descriptor* descriptor, bool qualified) {
  return qualified ? QualifiedClassName(descriptor) : ClassName(descriptor);
}

inline string ClassName(const EnumDescriptor* descriptor, bool qualified) {
  return qualified ? QualifiedClassName(descriptor) : ClassName(descriptor);
}

// Fully qualified name of the default_instance of this message.
string DefaultInstanceName(const Descriptor* descriptor);

// Returns the name of a no-op function that we can call to introduce a linker
// dependency on the given message type. This is used to implement implicit weak
// fields.
string ReferenceFunctionName(const Descriptor* descriptor);

// Name of the CRTP class template (for use with proto_h).
// This is a class name, like "ProtoName_InternalBase".
string DependentBaseClassTemplateName(const Descriptor* descriptor);

// Name of the base class: either the dependent base class (for use with
// proto_h) or google::protobuf::Message.
string SuperClassName(const Descriptor* descriptor, const Options& options);

// Returns a string that down-casts from the dependent base class to the
// derived class.
string DependentBaseDownCast();
string DependentBaseConstDownCast();

// Get the (unqualified) name that should be used for this field in C++ code.
// The name is coerced to lower-case to emulate proto1 behavior.  People
// should be using lowercase-with-underscores style for proto field names
// anyway, so normally this just returns field->name().
string FieldName(const FieldDescriptor* field);

// Get the sanitized name that should be used for the given enum in C++ code.
string EnumValueName(const EnumValueDescriptor* enum_value);

// Returns an estimate of the compiler's alignment for the field.  This
// can't guarantee to be correct because the generated code could be compiled on
// different systems with different alignment rules.  The estimates below assume
// 64-bit pointers.
int EstimateAlignmentSize(const FieldDescriptor* field);

// Get the unqualified name that should be used for a field's field
// number constant.
string FieldConstantName(const FieldDescriptor *field);

// Returns the scope where the field was defined (for extensions, this is
// different from the message type to which the field applies).
inline const Descriptor* FieldScope(const FieldDescriptor* field) {
  return field->is_extension() ?
    field->extension_scope() : field->containing_type();
}

// Returns true if the given 'field_descriptor' has a message type that is
// a dependency of the file where the field is defined (i.e., the field
// type is defined in a different file than the message holding the field).
//
// This only applies to Message-typed fields. Enum-typed fields may refer
// to an enum in a dependency; however, enums are specified and
// forward-declared with an enum-base, so the definition is not required to
// manipulate the field value.
bool IsFieldDependent(const FieldDescriptor* field_descriptor);

// Returns the name that should be used for forcing dependent lookup from a
// dependent base class.
string DependentTypeName(const FieldDescriptor* field);

// Returns the fully-qualified type name field->message_type().  Usually this
// is just ClassName(field->message_type(), true);
string FieldMessageTypeName(const FieldDescriptor* field);

// Strips ".proto" or ".protodevel" from the end of a filename.
LIBPROTOC_EXPORT string StripProto(const string& filename);

// Get the C++ type name for a primitive type (e.g. "double", "::google::protobuf::int32", etc.).
// Note:  non-built-in type names will be qualified, meaning they will start
// with a ::.  If you are using the type as a template parameter, you will
// need to insure there is a space between the < and the ::, because the
// ridiculous C++ standard defines "<:" to be a synonym for "[".
const char* PrimitiveTypeName(FieldDescriptor::CppType type);

// Get the declared type name in CamelCase format, as is used e.g. for the
// methods of WireFormat.  For example, TYPE_INT32 becomes "Int32".
const char* DeclaredTypeMethodName(FieldDescriptor::Type type);

// Return the code that evaluates to the number when compiled.
string Int32ToString(int number);

// Return the code that evaluates to the number when compiled.
string Int64ToString(int64 number);

// Get code that evaluates to the field's default value.
string DefaultValue(const FieldDescriptor* field);

// Convert a file name into a valid identifier.
string FilenameIdentifier(const string& filename);

// For each .proto file generates a unique namespace. In this namespace global
// definitions are put to prevent collisions.
string FileLevelNamespace(const string& filename);
inline string FileLevelNamespace(const FileDescriptor* file) {
  return FileLevelNamespace(file->name());
}
inline string FileLevelNamespace(const Descriptor* d) {
  return FileLevelNamespace(d->file());
}

// Return the qualified C++ name for a file level symbol.
string QualifiedFileLevelSymbol(const string& package, const string& name);

// Escape C++ trigraphs by escaping question marks to \?
string EscapeTrigraphs(const string& to_escape);

// Escaped function name to eliminate naming conflict.
string SafeFunctionName(const Descriptor* descriptor,
                        const FieldDescriptor* field,
                        const string& prefix);

// Returns true if unknown fields are always preserved after parsing.
inline bool AlwaysPreserveUnknownFields(const FileDescriptor* file) {
  return file->syntax() != FileDescriptor::SYNTAX_PROTO3;
}

// Returns true if unknown fields are preserved after parsing.
inline bool AlwaysPreserveUnknownFields(const Descriptor* message) {
  return AlwaysPreserveUnknownFields(message->file());
}

// Returns true if generated messages have public unknown fields accessors
inline bool PublicUnknownFieldsAccessors(const Descriptor* message) {
  return message->file()->syntax() != FileDescriptor::SYNTAX_PROTO3;
}

// Returns the optimize mode for <file>, respecting <options.enforce_lite>.
::google::protobuf::FileOptions_OptimizeMode GetOptimizeFor(
    const FileDescriptor* file, const Options& options);

// Determines whether unknown fields will be stored in an UnknownFieldSet or
// a string.
inline bool UseUnknownFieldSet(const FileDescriptor* file,
                               const Options& options) {
  return GetOptimizeFor(file, options) != FileOptions::LITE_RUNTIME;
}


// Does the file have any map fields, necessitating the file to include
// map_field_inl.h and map.h.
bool HasMapFields(const FileDescriptor* file);

// Does this file have any enum type definitions?
bool HasEnumDefinitions(const FileDescriptor* file);

// Does this file have generated parsing, serialization, and other
// standard methods for which reflection-based fallback implementations exist?
inline bool HasGeneratedMethods(const FileDescriptor* file,
                                const Options& options) {
  return GetOptimizeFor(file, options) != FileOptions::CODE_SIZE;
}

// Do message classes in this file have descriptor and reflection methods?
inline bool HasDescriptorMethods(const FileDescriptor* file,
                                 const Options& options) {
  return GetOptimizeFor(file, options) != FileOptions::LITE_RUNTIME;
}

// Should we generate generic services for this file?
inline bool HasGenericServices(const FileDescriptor* file,
                               const Options& options) {
  return file->service_count() > 0 &&
         GetOptimizeFor(file, options) != FileOptions::LITE_RUNTIME &&
         file->options().cc_generic_services();
}

// Should we generate a separate, super-optimized code path for serializing to
// flat arrays?  We don't do this in Lite mode because we'd rather reduce code
// size.
inline bool HasFastArraySerialization(const FileDescriptor* file,
                                      const Options& options) {
  return GetOptimizeFor(file, options) == FileOptions::SPEED;
}


inline bool IsMapEntryMessage(const Descriptor* descriptor) {
  return descriptor->options().map_entry();
}

// Returns true if the field's CPPTYPE is string or message.
bool IsStringOrMessage(const FieldDescriptor* field);

// For a string field, returns the effective ctype.  If the actual ctype is
// not supported, returns the default of STRING.
FieldOptions::CType EffectiveStringCType(const FieldDescriptor* field);

string UnderscoresToCamelCase(const string& input, bool cap_next_letter);

inline bool HasFieldPresence(const FileDescriptor* file) {
  return file->syntax() != FileDescriptor::SYNTAX_PROTO3;
}

// Returns true if 'enum' semantics are such that unknown values are preserved
// in the enum field itself, rather than going to the UnknownFieldSet.
inline bool HasPreservingUnknownEnumSemantics(const FileDescriptor* file) {
  return file->syntax() == FileDescriptor::SYNTAX_PROTO3;
}

inline bool SupportsArenas(const FileDescriptor* file) {
  return file->options().cc_enable_arenas();
}

inline bool SupportsArenas(const Descriptor* desc) {
  return SupportsArenas(desc->file());
}

inline bool SupportsArenas(const FieldDescriptor* field) {
  return SupportsArenas(field->file());
}

inline bool IsCrossFileMessage(const FieldDescriptor* field) {
  return field->type() == FieldDescriptor::TYPE_MESSAGE &&
         field->message_type()->file() != field->file();
}

bool IsAnyMessage(const FileDescriptor* descriptor);
bool IsAnyMessage(const Descriptor* descriptor);

bool IsWellKnownMessage(const FileDescriptor* descriptor);

void GenerateUtf8CheckCodeForString(const FieldDescriptor* field,
                                    const Options& options, bool for_parse,
                                    const std::map<string, string>& variables,
                                    const char* parameters,
                                    io::Printer* printer);

void GenerateUtf8CheckCodeForCord(const FieldDescriptor* field,
                                  const Options& options, bool for_parse,
                                  const std::map<string, string>& variables,
                                  const char* parameters, io::Printer* printer);

inline ::google::protobuf::FileOptions_OptimizeMode GetOptimizeFor(
    const FileDescriptor* file, const Options& options) {
  return options.enforce_lite
      ? FileOptions::LITE_RUNTIME
      : file->options().optimize_for();
}

// This orders the messages in a .pb.cc as it's outputted by file.cc
void FlattenMessagesInFile(const FileDescriptor* file,
                           std::vector<const Descriptor*>* result);
inline std::vector<const Descriptor*> FlattenMessagesInFile(
    const FileDescriptor* file) {
  std::vector<const Descriptor*> result;
  FlattenMessagesInFile(file, &result);
  return result;
}

bool HasWeakFields(const Descriptor* desc);
bool HasWeakFields(const FileDescriptor* desc);

// Indicates whether we should use implicit weak fields for this file.
bool UsingImplicitWeakFields(const FileDescriptor* file,
                             const Options& options);

// Indicates whether to treat this field as implicitly weak.
bool IsImplicitWeakField(const FieldDescriptor* field, const Options& options);

// Returns true if the "required" restriction check should be ignored for the
// given field.
inline static bool ShouldIgnoreRequiredFieldCheck(const FieldDescriptor* field,
                                                  const Options& options) {
  return false;
}

class LIBPROTOC_EXPORT NamespaceOpener {
 public:
  explicit NamespaceOpener(io::Printer* printer) : printer_(printer) {}
  NamespaceOpener(const string& name, io::Printer* printer)
      : printer_(printer) {
    ChangeTo(name);
  }
  ~NamespaceOpener() { ChangeTo(""); }

  void ChangeTo(const string& name) {
    std::vector<string> new_stack_ =
        Split(name, "::", true);
    int len = std::min(name_stack_.size(), new_stack_.size());
    int common_idx = 0;
    while (common_idx < len) {
      if (name_stack_[common_idx] != new_stack_[common_idx]) break;
      common_idx++;
    }
    for (int i = name_stack_.size() - 1; i >= common_idx; i--) {
      printer_->Print("}  // namespace $ns$\n", "ns", name_stack_[i]);
    }
    name_stack_.swap(new_stack_);
    for (int i = common_idx; i < name_stack_.size(); i++) {
      printer_->Print("namespace $ns$ {\n", "ns", name_stack_[i]);
    }
  }

 private:
  io::Printer* printer_;
  std::vector<string> name_stack_;
};

// Description of each strongly connected component. Note that the order
// of both the descriptors in this SCC and the order of children is
// deterministic.
struct SCC {
  std::vector<const Descriptor*> descriptors;
  std::vector<const SCC*> children;

  const Descriptor* GetRepresentative() const { return descriptors[0]; }
};

struct MessageAnalysis {
  bool is_recursive;
  bool contains_cord;
  bool contains_extension;
  bool contains_required;
};

// This class is used in FileGenerator, to ensure linear instead of
// quadratic performance, if we do this per message we would get O(V*(V+E)).
// Logically this is just only used in message.cc, but in the header for
// FileGenerator to help share it.
class LIBPROTOC_EXPORT SCCAnalyzer {
 public:
  explicit SCCAnalyzer(const Options& options) : options_(options), index_(0) {}
  ~SCCAnalyzer() {
    for (int i = 0; i < garbage_bin_.size(); i++) delete garbage_bin_[i];
  }

  const SCC* GetSCC(const Descriptor* descriptor) {
    if (cache_.count(descriptor)) return cache_[descriptor].scc;
    return DFS(descriptor).scc;
  }

  MessageAnalysis GetSCCAnalysis(const SCC* scc);

  bool HasRequiredFields(const Descriptor* descriptor) {
    MessageAnalysis result = GetSCCAnalysis(GetSCC(descriptor));
    return result.contains_required || result.contains_extension;
  }

 private:
  struct NodeData {
    const SCC* scc;  // if null it means its still on the stack
    int index;
    int lowlink;
  };

  Options options_;
  std::map<const Descriptor*, NodeData> cache_;
  std::map<const SCC*, MessageAnalysis> analysis_cache_;
  std::vector<const Descriptor*> stack_;
  int index_;
  std::vector<SCC*> garbage_bin_;

  SCC* CreateSCC() {
    garbage_bin_.push_back(new SCC());
    return garbage_bin_.back();
  }

  // Tarjan's Strongly Connected Components algo
  NodeData DFS(const Descriptor* descriptor);

  // Add the SCC's that are children of this SCC to its children.
  void AddChildren(SCC* scc);
};

void ListAllFields(const FileDescriptor* d,
                   std::vector<const FieldDescriptor*>* fields);
void ListAllTypesForServices(const FileDescriptor* fd,
                             std::vector<const Descriptor*>* types);

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_HELPERS_H__
