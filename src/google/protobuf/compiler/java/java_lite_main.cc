#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/java/java_generator.h>
#include <google/protobuf/compiler/plugin.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

class JavaLiteGenerator : public CodeGenerator {
 public:
  JavaLiteGenerator() {}
  ~JavaLiteGenerator() {}
  bool Generate(const FileDescriptor* file,
                const string& parameter,
                GeneratorContext* context,
                string* error) const {
    // Only pass 'lite' as the generator parameter.
    return generator_.Generate(file, "lite", context, error);
  }
 private:
  JavaGenerator generator_;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(JavaLiteGenerator);
};

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

int main(int argc, char* argv[]) {
  google::protobuf::compiler::java::JavaLiteGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
