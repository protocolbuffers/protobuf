#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_GENERATOR_COMMON_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_GENERATOR_COMMON_H__

#include <cstddef>
#include <memory>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

static const int kMaxStaticSize = 1 << 15;  // aka 32k

class FieldGenerator {
 public:
  virtual ~FieldGenerator() = default;
  virtual void GenerateSerializationCode(io::Printer* printer) const = 0;
};

// Convenience class which constructs FieldGenerators for a Descriptor.
template <typename FieldGeneratorType>
class FieldGeneratorMap {
 public:
  explicit FieldGeneratorMap(const Descriptor* descriptor)
      : descriptor_(descriptor) {
    insert_order_.reserve(static_cast<size_t>(descriptor->field_count()));
    index_order_.resize(static_cast<size_t>(descriptor->field_count()));
  }

  ~FieldGeneratorMap() {
    for (const auto* g : insert_order_) {
      delete g;
    }
  }

  FieldGeneratorMap(FieldGeneratorMap&&) = default;
  FieldGeneratorMap& operator=(FieldGeneratorMap&&) = default;

  FieldGeneratorMap(const FieldGeneratorMap&) = delete;
  FieldGeneratorMap& operator=(const FieldGeneratorMap&) = delete;

  size_t size() const { return insert_order_.size(); }

  void Add(const FieldDescriptor* field,
           std::unique_ptr<FieldGeneratorType> field_generator) {
    ABSL_CHECK_EQ(field->containing_type(), descriptor_);
    insert_order_.push_back(field_generator.release());
    index_order_[static_cast<size_t>(field->index())] = insert_order_.back();
  }

  const FieldGeneratorType& get(const FieldDescriptor* field) const {
    ABSL_CHECK_EQ(field->containing_type(), descriptor_);
    return *index_order_[static_cast<size_t>(field->index())];
  }

  const FieldGeneratorType& getInInsertOrder(int index) const {
    return *insert_order_[static_cast<size_t>(index)];
  }

  std::vector<const FieldGenerator*> field_generators() const {
    std::vector<const FieldGenerator*> field_generators;
    field_generators.reserve(index_order_.size());
    for (const auto* g : index_order_) {
      field_generators.push_back(g);
    }
    return field_generators;
  }

 private:
  const Descriptor* descriptor_;
  std::vector<const FieldGeneratorType*> insert_order_;
  std::vector<const FieldGeneratorType*> index_order_;
};

inline void ReportUnexpectedPackedFieldsCall() {
  // Reaching here indicates a bug. Cases are:
  //   - This FieldGenerator should support packing,
  //     but this method should be overridden.
  //   - This FieldGenerator doesn't support packing, and this method
  //     should never have been called.
  ABSL_LOG(FATAL) << "GenerateBuilderParsingCodeFromPacked() "
                  << "called on field generator that does not support packing.";
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_GENERATOR_COMMON_H__
