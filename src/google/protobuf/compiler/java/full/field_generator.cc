#include "google/protobuf/compiler/java/full/field_generator.h"

#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/name_resolver.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

ImmutableFieldGenerator::ImmutableFieldGenerator(
    const FieldDescriptor* descriptor, int bit_index, Context* context)
    : descriptor_(descriptor),
      bit_index_(bit_index),
      context_(context),
      name_resolver_(context->GetNameResolver()) {}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
