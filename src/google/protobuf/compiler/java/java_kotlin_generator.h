#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_KOTLIN_GENERATOR_H_
#define GOOGLE_PROTOBUF_COMPILER_JAVA_KOTLIN_GENERATOR_H_

#include <string>
#include <google/protobuf/compiler/code_generator.h>

#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

// CodeGenerator implementation which generates Kotlin code.  If you create your
// own protocol compiler binary and you want it to support Kotlin output, you
// can do so by registering an instance of this CodeGenerator with the
// CommandLineInterface in your main() function.
class PROTOC_EXPORT KotlinGenerator : public CodeGenerator {
 public:
  KotlinGenerator();
  ~KotlinGenerator() override;

  // implements CodeGenerator ----------------------------------------
  bool Generate(const FileDescriptor* file, const std::string& parameter,
                GeneratorContext* context, std::string* error) const override;

  uint64_t GetSupportedFeatures() const override;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(KotlinGenerator);
};

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_KOTLIN_GENERATOR_H_
