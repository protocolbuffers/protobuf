#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_MUTABLE_FIELD_GENERATOR_FACTORY_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_MUTABLE_FIELD_GENERATOR_FACTORY_H__

#include <memory>
#include <vector>

#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/generator_common.h"
#include "google/protobuf/compiler/java/lite/field_generator.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

FieldGeneratorMap<ImmutableFieldLiteGenerator> MakeImmutableFieldLiteGenerators(
    const Descriptor* descriptor, Context* context);

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_MUTABLE_FIELD_GENERATOR_FACTORY_H__
