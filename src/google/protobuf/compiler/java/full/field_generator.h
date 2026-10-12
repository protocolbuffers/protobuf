#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_FIELD_GENERATOR_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_FIELD_GENERATOR_H__

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/java/generator_common.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {
class Context;            // context.h
class ClassNameResolver;  // name_resolver.h

class ImmutableFieldGenerator : public FieldGenerator {
 public:
  explicit ImmutableFieldGenerator(const FieldDescriptor* descriptor,
                                   int bit_index, Context* context);
  ImmutableFieldGenerator(const ImmutableFieldGenerator&) = delete;
  ImmutableFieldGenerator& operator=(const ImmutableFieldGenerator&) = delete;
  ~ImmutableFieldGenerator() override = default;

  bool HasHasbit() const;
  bool IsRealOneof() const;

  int GetBitIndex() const { return bit_index_; }
  constexpr int GetNumBits() const { return 1; }
  virtual void GenerateInterfaceMembers(io::Printer* printer) const = 0;
  virtual void GenerateMembers(io::Printer* printer) const = 0;
  virtual void GenerateBuilderMembers(io::Printer* printer) const = 0;
  virtual void GenerateInitializationCode(io::Printer* printer) const = 0;
  virtual void GenerateBuilderClearCode(io::Printer* printer) const = 0;
  virtual void GenerateMergingCode(io::Printer* printer) const = 0;
  virtual void GenerateBuildingCode(io::Printer* printer) const = 0;
  virtual void GenerateBuilderParsingCode(io::Printer* printer) const = 0;
  virtual void GenerateSerializedSizeCode(io::Printer* printer) const = 0;
  virtual void GenerateFieldBuilderInitializationCode(
      io::Printer* printer) const = 0;

  virtual void GenerateBuilderParsingCodeFromPacked(
      io::Printer* printer) const {
    ReportUnexpectedPackedFieldsCall();
  }

  virtual void GenerateEqualsCode(io::Printer* printer) const = 0;
  virtual void GenerateHashCode(io::Printer* printer) const = 0;

  virtual std::string GetBoxedType() const = 0;

 protected:
  const FieldDescriptor* descriptor_;
  int bit_index_;
  Context* context_;
  ClassNameResolver* name_resolver_;
  absl::flat_hash_map<absl::string_view, std::string> variables_;
};

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_FIELD_GENERATOR_H__
