
#ifndef UPBC_GENERATOR_H_
#define UPBC_GENERATOR_H_

#include <memory>
#include <google/protobuf/compiler/code_generator.h>

namespace upbc {
std::unique_ptr<google::protobuf::compiler::CodeGenerator> GetGenerator();
}

#endif  // UPBC_GENERATOR_H_
