#include "google/protobuf/compiler/rust/naming.h"

#include <string>

#include <gtest/gtest.h>
#include "google/protobuf/descriptor.h"

using google::protobuf::compiler::rust::CamelToSnakeCase;
using google::protobuf::compiler::rust::RustInternalModuleName;
using google::protobuf::compiler::rust::ScreamingSnakeToUpperCamelCase;

namespace {
TEST(RustProtoNaming, RustInternalModuleName) {
  auto get_internal_module_name = [](const std::string& name) {
    google::protobuf::FileDescriptorProto file_proto;
    file_proto.set_name(name);
    google::protobuf::DescriptorPool pool;
    return RustInternalModuleName(*pool.BuildFile(file_proto));
  };

  EXPECT_EQ(get_internal_module_name("strong_bad/lol.proto"),
            "strong__bad_slol");
  EXPECT_EQ(get_internal_module_name("0.1.proto"), "0_2e_1");
  EXPECT_EQ(get_internal_module_name("_.proto"), "__");
  EXPECT_EQ(get_internal_module_name("abc   .proto"), "abc_20__20__20_");
  EXPECT_EQ(get_internal_module_name("hello (2).proto"), "hello_20__28_2_29_");
  EXPECT_EQ(get_internal_module_name("k8s.min.proto"), "k8s_2e_min");
  EXPECT_EQ(get_internal_module_name("c++.proto"), "c_2b__2b_");
  EXPECT_EQ(get_internal_module_name("hello,world.proto"), "hello_2c_world");
  EXPECT_EQ(get_internal_module_name("hello..world.proto"),
            "hello_2e__2e_world");
  EXPECT_EQ(get_internal_module_name("hello_你好.proto"),
            "hello___e4__bd__a0__e5__a5__bd_");
  EXPECT_EQ(get_internal_module_name("my-message.proto"), "my__message");
}

TEST(RustProtoNaming, CamelToSnakeCase) {
  // TODO: Review this behavior.
  EXPECT_EQ(CamelToSnakeCase("CamelCase"), "camel_case");
  EXPECT_EQ(CamelToSnakeCase("_CamelCase"), "_camel_case");
  EXPECT_EQ(CamelToSnakeCase("camelCase"), "camel_case");
  EXPECT_EQ(CamelToSnakeCase("Number2020"), "number2020");
  EXPECT_EQ(CamelToSnakeCase("Number_2020"), "number_2020");
  EXPECT_EQ(CamelToSnakeCase("camelCase_"), "camel_case_");
  EXPECT_EQ(CamelToSnakeCase("CamelCaseTrio"), "camel_case_trio");
  EXPECT_EQ(CamelToSnakeCase("UnderIn_Middle"), "under_in_middle");
  EXPECT_EQ(CamelToSnakeCase("Camel_Case"), "camel_case");
  EXPECT_EQ(CamelToSnakeCase("Camel__Case"), "camel__case");
  EXPECT_EQ(CamelToSnakeCase("CAMEL_CASE"), "c_a_m_e_l_c_a_s_e");
}

TEST(RustProtoNaming, ScreamingSnakeToUpperCamelCase) {
  // TODO: Review this behavior.
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("CAMEL_CASE"), "CamelCase");
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("NUMBER2020"), "Number2020");
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("NUMBER_2020"), "Number2020");
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("FOO_4040_BAR"), "Foo4040Bar");
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("FOO_4040bar"), "Foo4040Bar");
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("_CAMEL_CASE"), "CamelCase");

  // This function doesn't currently preserve prefix/suffix underscore,
  // while CamelToSnakeCase does.
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("CAMEL_CASE_"), "CamelCase");
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("camel_case"), "CamelCase");
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("CAMEL_CASE_TRIO"), "CamelCaseTrio");
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("UNDER_IN__MIDDLE"),
            "UnderInMiddle");
}

}  // namespace
