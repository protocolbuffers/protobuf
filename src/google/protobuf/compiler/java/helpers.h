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

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_HELPERS_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_HELPERS_H__

#include <cstdint>
#include <string>

#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/java/names.h"
#include "google/protobuf/compiler/java/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor_legacy.h"
#include "google/protobuf/io/printer.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

// Commonly-used separator comments.  Thick is a line of '=', thin is a line
// of '-'.
extern const char kThickSeparator[];
extern const char kThinSeparator[];

bool IsForbiddenKotlin(absl::string_view field_name);

// If annotation_file is non-empty, prints a javax.annotation.Generated
// annotation to the given Printer. annotation_file will be referenced in the
// annotation's comments field. delimiter should be the Printer's delimiter
// character. annotation_file will be included verbatim into a Java literal
// string, so it should not contain quotes or invalid Java escape sequences;
// however, these are unlikely to appear in practice, as the value of
// annotation_file should be generated from the filename of the source file
// being annotated (which in turn must be a Java identifier plus ".java").
void PrintGeneratedAnnotation(io::Printer* printer, char delimiter = '$',
                              absl::string_view annotation_file = "",
                              Options options = {});

// If a GeneratedMessageLite contains non-lite enums, then its verifier
// must be instantiated inline, rather than retrieved from the enum class.
void PrintEnumVerifierLogic(
    io::Printer* printer, const FieldDescriptor* descriptor,
    const absl::flat_hash_map<absl::string_view, std::string>& variables,
    absl::string_view var_name, absl::string_view terminating_string,
    bool enforce_lite);

// Converts a name to camel-case. If cap_first_letter is true, capitalize the
// first letter.
std::string ToCamelCase(absl::string_view input, bool lower_first);

// Similar to UnderscoresToCamelCase, but guarantees that the result is a
// complete Java identifier by adding a _ if needed.
std::string CamelCaseFieldName(const FieldDescriptor* field);

// Get an identifier that uniquely identifies this type within the file.
// This is used to declare static variables related to this type at the
// outermost file scope.
std::string UniqueFileScopeIdentifier(const Descriptor* descriptor);

// Gets the unqualified class name for the file.  For each .proto file, there
// will be one Java class containing all the immutable messages and another
// Java class containing all the mutable messages.
// TODO(xiaofeng): remove the default value after updating client code.
std::string FileClassName(const FileDescriptor* file, bool immutable = true);

// Returns the file's Java package name.
std::string FileJavaPackage(const FileDescriptor* file, bool immutable,
                            Options options = {});

// Returns output directory for the given package name.
std::string JavaPackageToDir(std::string package_name);

// Returns the name with Kotlin keywords enclosed in backticks
std::string EscapeKotlinKeywords(std::string name);

// Comma-separate list of option-specified interfaces implemented by the
// Message, to follow the "implements" declaration of the Message definition.
std::string ExtraMessageInterfaces(const Descriptor* descriptor);
// Comma-separate list of option-specified interfaces implemented by the
// MutableMessage, to follow the "implements" declaration of the MutableMessage
// definition.
std::string ExtraMutableMessageInterfaces(const Descriptor* descriptor);
// Comma-separate list of option-specified interfaces implemented by the
// Builder, to follow the "implements" declaration of the Builder definition.
std::string ExtraBuilderInterfaces(const Descriptor* descriptor);
// Comma-separate list of option-specified interfaces extended by the
// MessageOrBuilder, to follow the "extends" declaration of the
// MessageOrBuilder definition.
std::string ExtraMessageOrBuilderInterfaces(const Descriptor* descriptor);

// Get the unqualified Java class name for mutable messages. i.e. without
// package or outer classnames.
inline std::string ShortMutableJavaClassName(const Descriptor* descriptor) {
  return descriptor->name();
}

// Whether the given descriptor is for one of the core descriptor protos. We
// cannot currently use the new runtime with core protos since there is a
// bootstrapping problem with obtaining their descriptors.
inline bool IsDescriptorProto(const Descriptor* descriptor) {
  return descriptor->file()->name() == "net/proto2/proto/descriptor.proto" ||
         descriptor->file()->name() == "google/protobuf/descriptor.proto";
}

// Returns the stored type string used by the experimental runtime for oneof
// fields.
std::string GetOneofStoredType(const FieldDescriptor* field);

// We use either the proto1 enums if the enum is generated, otherwise fall back
// to use integers.
enum class Proto1EnumRepresentation {
  kEnum,
  kInteger,
};

// Returns which representation we should use.
inline Proto1EnumRepresentation GetProto1EnumRepresentation(
    const EnumDescriptor* descriptor) {
  if (descriptor->containing_type() != nullptr) {
    return Proto1EnumRepresentation::kEnum;
  }
  return Proto1EnumRepresentation::kInteger;
}

// Whether we should generate multiple java files for messages.
inline bool MultipleJavaFiles(const FileDescriptor* descriptor,
                              bool immutable) {
  (void)immutable;
  return descriptor->options().java_multiple_files();
}


// Returns true if `descriptor` will be written to its own .java file.
// `immutable` should be set to true if we're generating for the immutable API.
template <typename Descriptor>
bool IsOwnFile(const Descriptor* descriptor, bool immutable) {
  return descriptor->containing_type() == NULL &&
         MultipleJavaFiles(descriptor->file(), immutable);
}

template <>
inline bool IsOwnFile(const ServiceDescriptor* descriptor, bool immutable) {
  return MultipleJavaFiles(descriptor->file(), immutable);
}

// If `descriptor` describes an object with its own .java file,
// returns the name (relative to that .java file) of the file that stores
// annotation data for that descriptor. `suffix` is usually empty, but may
// (e.g.) be "OrBuilder" for some generated interfaces.
template <typename Descriptor>
std::string AnnotationFileName(const Descriptor* descriptor,
                               absl::string_view suffix) {
  return absl::StrCat(descriptor->name(), suffix, ".java.pb.meta");
}

// Get the unqualified name that should be used for a field's field
// number constant.
std::string FieldConstantName(const FieldDescriptor* field);

// Returns the type of the FieldDescriptor.
// This does nothing interesting for the open source release, but is used for
// hacks that improve compatibility with version 1 protocol buffers at Google.
FieldDescriptor::Type GetType(const FieldDescriptor* field);

enum JavaType {
  JAVATYPE_INT,
  JAVATYPE_LONG,
  JAVATYPE_FLOAT,
  JAVATYPE_DOUBLE,
  JAVATYPE_BOOLEAN,
  JAVATYPE_STRING,
  JAVATYPE_BYTES,
  JAVATYPE_ENUM,
  JAVATYPE_MESSAGE
};

JavaType GetJavaType(const FieldDescriptor* field);

absl::string_view PrimitiveTypeName(JavaType type);

// Get the fully-qualified class name for a boxed primitive type, e.g.
// "java.lang.Integer" for JAVATYPE_INT.  Returns NULL for enum and message
// types.
absl::string_view BoxedPrimitiveTypeName(JavaType type);

// Kotlin source does not distinguish between primitives and non-primitives,
// but does use Kotlin-specific qualified types for them.
absl::string_view KotlinTypeName(JavaType type);

// Get the name of the java enum constant representing this type. E.g.,
// "INT32" for FieldDescriptor::TYPE_INT32. The enum constant's full
// name is "com.google.protobuf.WireFormat.FieldType.INT32".
absl::string_view FieldTypeName(const FieldDescriptor::Type field_type);

class ClassNameResolver;
std::string DefaultValue(const FieldDescriptor* field, bool immutable,
                         ClassNameResolver* name_resolver,
                         Options options = {});
inline std::string ImmutableDefaultValue(const FieldDescriptor* field,
                                         ClassNameResolver* name_resolver,
                                         Options options = {}) {
  return DefaultValue(field, true, name_resolver, options);
}
bool IsDefaultValueJavaDefault(const FieldDescriptor* field);
bool IsByteStringWithCustomDefaultValue(const FieldDescriptor* field);

// Does this message class have descriptor and reflection methods?
inline bool HasDescriptorMethods(const Descriptor* /* descriptor */,
                                 bool enforce_lite) {
  return !enforce_lite;
}
inline bool HasDescriptorMethods(const EnumDescriptor* /* descriptor */,
                                 bool enforce_lite) {
  return !enforce_lite;
}
inline bool HasDescriptorMethods(const FileDescriptor* /* descriptor */,
                                 bool enforce_lite) {
  return !enforce_lite;
}

// Should we generate generic services for this file?
inline bool HasGenericServices(const FileDescriptor* file, bool enforce_lite) {
  return file->service_count() > 0 &&
         HasDescriptorMethods(file, enforce_lite) &&
         file->options().java_generic_services();
}

// Methods for shared bitfields.

// Gets the name of the shared bitfield for the given index.
std::string GetBitFieldName(int index);

// Gets the name of the shared bitfield for the given bit index.
// Effectively, GetBitFieldName(bitIndex / 32)
std::string GetBitFieldNameForBit(int bitIndex);

// Generates the java code for the expression that returns the boolean value
// of the bit of the shared bitfields for the given bit index.
// Example: "((bitField1_ & 0x04) == 0x04)"
std::string GenerateGetBit(int bitIndex);

// Generates the java code for the expression that sets the bit of the shared
// bitfields for the given bit index.
// Example: "bitField1_ = (bitField1_ | 0x04)"
std::string GenerateSetBit(int bitIndex);

// Generates the java code for the expression that clears the bit of the shared
// bitfields for the given bit index.
// Example: "bitField1_ = (bitField1_ & ~0x04)"
std::string GenerateClearBit(int bitIndex);

// Does the same as GenerateGetBit but operates on the bit field on a local
// variable. This is used by the builder to copy the value in the builder to
// the message.
// Example: "((from_bitField1_ & 0x04) == 0x04)"
std::string GenerateGetBitFromLocal(int bitIndex);

// Does the same as GenerateSetBit but operates on the bit field on a local
// variable. This is used by the builder to copy the value in the builder to
// the message.
// Example: "to_bitField1_ = (to_bitField1_ | 0x04)"
std::string GenerateSetBitToLocal(int bitIndex);

// Does the same as GenerateGetBit but operates on the bit field on a local
// variable. This is used by the parsing constructor to record if a repeated
// field is mutable.
// Example: "((mutable_bitField1_ & 0x04) == 0x04)"
std::string GenerateGetBitMutableLocal(int bitIndex);

// Does the same as GenerateSetBit but operates on the bit field on a local
// variable. This is used by the parsing constructor to record if a repeated
// field is mutable.
// Example: "mutable_bitField1_ = (mutable_bitField1_ | 0x04)"
std::string GenerateSetBitMutableLocal(int bitIndex);

// Returns whether the JavaType is a reference type.
bool IsReferenceType(JavaType type);

// Returns the capitalized name for calling relative functions in
// CodedInputStream
absl::string_view GetCapitalizedType(const FieldDescriptor* field,
                                     bool immutable, Options options);

// For encodings with fixed sizes, returns that size in bytes.  Otherwise
// returns -1.
int FixedSize(FieldDescriptor::Type type);

// Comparators used to sort fields in MessageGenerator
struct FieldOrderingByNumber {
  inline bool operator()(const FieldDescriptor* a,
                         const FieldDescriptor* b) const {
    return a->number() < b->number();
  }
};

struct ExtensionRangeOrdering {
  bool operator()(const Descriptor::ExtensionRange* a,
                  const Descriptor::ExtensionRange* b) const {
    return a->start < b->start;
  }
};

// Sort the fields of the given Descriptor by number into a new[]'d array
// and return it. The caller should delete the returned array.
const FieldDescriptor** SortFieldsByNumber(const Descriptor* descriptor);

// Does this message class have any packed fields?
inline bool HasPackedFields(const Descriptor* descriptor) {
  for (int i = 0; i < descriptor->field_count(); i++) {
    if (descriptor->field(i)->is_packed()) {
      return true;
    }
  }
  return false;
}

// Check a message type and its sub-message types recursively to see if any of
// them has a required field. Return true if a required field is found.
bool HasRequiredFields(const Descriptor* descriptor);

bool IsRealOneof(const FieldDescriptor* descriptor);

inline bool HasHasbit(const FieldDescriptor* descriptor) {
  // TODO(b/241441075) Replace this with internal::cpp::HasHasbit once Elysium
  // unblocks this change.
  return (FieldDescriptorLegacy(descriptor).has_optional_keyword() ||
          descriptor->is_required()) &&
         !descriptor->options().weak();
}

// Whether generate classes expose public PARSER instances.
inline bool ExposePublicParser(const FileDescriptor* descriptor) {
  // TODO(liujisi): Mark the PARSER private in 3.1.x releases.
  return FileDescriptorLegacy(descriptor).syntax() ==
         FileDescriptorLegacy::Syntax::SYNTAX_PROTO2;
}

// Whether unknown enum values are kept (i.e., not stored in UnknownFieldSet
// but in the message and can be queried using additional getters that return
// ints.
inline bool SupportUnknownEnumValue(const FieldDescriptor* field) {
  return !field->legacy_enum_field_treated_as_closed();
}

// Check whether a message has repeated fields.
bool HasRepeatedFields(const Descriptor* descriptor);

inline bool IsMapEntry(const Descriptor* descriptor) {
  return descriptor->options().map_entry();
}

inline bool IsMapField(const FieldDescriptor* descriptor) {
  return descriptor->is_map();
}

inline bool IsAnyMessage(const Descriptor* descriptor) {
  return descriptor->full_name() == "google.protobuf.Any";
}

inline bool IsWrappersProtoFile(const FileDescriptor* descriptor) {
  return descriptor->name() == "google/protobuf/wrappers.proto";
}

inline bool CheckUtf8(const FieldDescriptor* descriptor) {
  return descriptor->requires_utf8_validation() ||
         descriptor->file()->options().java_string_check_utf8();
}

inline std::string GeneratedCodeVersionSuffix() {
  return "V3";
}

void WriteUInt32ToUtf16CharSequence(uint32_t number,
                                    std::vector<uint16_t>* output);

inline void WriteIntToUtf16CharSequence(int value,
                                        std::vector<uint16_t>* output) {
  WriteUInt32ToUtf16CharSequence(static_cast<uint32_t>(value), output);
}

// Escape a UTF-16 character so it can be embedded in a Java string literal.
void EscapeUtf16ToString(uint16_t code, std::string* output);

// Only the lowest two bytes of the return value are used. The lowest byte
// is the integer value of a j/c/g/protobuf/FieldType enum. For the other
// byte:
//    bit 0: whether the field is required.
//    bit 1: whether the field requires UTF-8 validation.
//    bit 2: whether the field needs isInitialized check.
//    bit 3: whether the field is a map field with proto2 enum value.
//    bits 4-7: unused
int GetExperimentalJavaFieldType(const FieldDescriptor* field);

// To get the total number of entries need to be built for experimental runtime
// and the first field number that are not in the table part
std::pair<int, int> GetTableDrivenNumberOfEntriesAndLookUpStartFieldNumber(
    const FieldDescriptor** fields, int count);

const FieldDescriptor* MapKeyField(const FieldDescriptor* descriptor);

const FieldDescriptor* MapValueField(const FieldDescriptor* descriptor);

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_HELPERS_H__
