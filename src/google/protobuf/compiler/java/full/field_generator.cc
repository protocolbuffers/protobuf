#include "google/protobuf/compiler/java/full/field_generator.h"

#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/name_resolver.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

ImmutableFieldGenerator::ImmutableFieldGenerator(
    const FieldDescriptor* descriptor, int messageBitIndex, int builderBitIndex,
    Context* context)
    : descriptor_(descriptor),
      message_bit_index_(messageBitIndex),
      builder_bit_index_(builderBitIndex),
      context_(context),
      name_resolver_(context->GetNameResolver()) {}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
