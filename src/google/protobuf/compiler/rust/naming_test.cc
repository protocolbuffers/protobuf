#include "google/protobuf/compiler/rust/naming.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

using google::protobuf::compiler::rust::Context;
using google::protobuf::compiler::rust::Kernel;
using google::protobuf::compiler::rust::Options;
using google::protobuf::compiler::rust::RustGeneratorContext;
using google::protobuf::compiler::rust::RustInternalModuleName;
using google::protobuf::io::Printer;
using google::protobuf::io::StringOutputStream;

namespace {
TEST(RustProtoNaming, RustInternalModuleName) {
  google::protobuf::FileDescriptorProto foo_file;
  foo_file.set_name("strong_bad/lol.proto");
  google::protobuf::DescriptorPool pool;
  const google::protobuf::FileDescriptor* fd = pool.BuildFile(foo_file);

  const Options opts = {Kernel::kUpb};
  std::vector<const google::protobuf::FileDescriptor*> files{fd};
  const RustGeneratorContext rust_generator_context(&files);
  std::string output;
  StringOutputStream stream{&output};
  Printer printer(&stream);
  Context c = Context(&opts, &rust_generator_context, &printer);

  EXPECT_EQ(RustInternalModuleName(c, *fd), "strong__bad_slol");
}

}  // namespace
