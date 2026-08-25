// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/testing/file.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/command_line_interface_tester.h"
#include "google/protobuf/compiler/cpp/generator.h"
#include "google/protobuf/compiler/python/generator.h"
#include "google/protobuf/compiler/python/pyi_generator.h"
#include "google/protobuf/cpp_features.pb.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace python {
namespace {

class TestGenerator : public CodeGenerator {
 public:
  TestGenerator() = default;
  ~TestGenerator() override = default;

  bool Generate(const FileDescriptor* file, const std::string& parameter,
                GeneratorContext* context, std::string* error) const override {
    TryInsert("test_pb2.py", "imports", context);
    TryInsert("test_pb2.py", "module_scope", context);
    TryInsert("test_pb2.py", "class_scope:foo.Bar", context);
    TryInsert("test_pb2.py", "class_scope:foo.Bar.Baz", context);
    return true;
  }

  void TryInsert(const std::string& filename,
                 const std::string& insertion_point,
                 GeneratorContext* context) const {
    std::unique_ptr<io::ZeroCopyOutputStream> output(
        context->OpenForInsert(filename, insertion_point));
    io::Printer printer(output.get(), '$');
    printer.Print("// inserted $name$\n", "name", insertion_point);
  }
};

// opposed to importlib) in the usual case where the .proto file paths do not
// not contain any Python keywords.
TEST(PythonPluginTest, ImportTest) {
  // Create files test1.proto and test2.proto with the former importing the
  // latter.
  ABSL_CHECK_OK(
      File::SetContents(absl::StrCat(::testing::TempDir(), "/test1.proto"),
                        "syntax = \"proto3\";\n"
                        "package foo;\n"
                        "import \"test2.proto\";"
                        "message Message1 {\n"
                        "  Message2 message_2 = 1;\n"
                        "}\n",
                        true));
  ABSL_CHECK_OK(
      File::SetContents(absl::StrCat(::testing::TempDir(), "/test2.proto"),
                        "syntax = \"proto3\";\n"
                        "package foo;\n"
                        "message Message2 {}\n",
                        true));

  compiler::CommandLineInterface cli;
  cli.SetInputsAreProtoPathRelative(true);
  python::Generator python_generator;
  cli.RegisterGenerator("--python_out", &python_generator, "");
  std::string proto_path = absl::StrCat("-I", ::testing::TempDir());
  std::string python_out = absl::StrCat("--python_out=", ::testing::TempDir());
  const char* argv[] = {"protoc", proto_path.c_str(), "-I.", python_out.c_str(),
                        "test1.proto"};
  ASSERT_EQ(0, cli.Run(5, argv));

  // Loop over the lines of the generated code and verify that we find an
  // ordinary Python import but do not find the string "importlib".
  std::string output;
  ABSL_CHECK_OK(
      File::GetContents(absl::StrCat(::testing::TempDir(), "/test1_pb2.py"),
                        &output, true));
  std::vector<absl::string_view> lines = absl::StrSplit(output, '\n');
  std::string expected_import = "import test2_pb2";
  bool found_expected_import = false;
  for (absl::string_view line : lines) {
    if (absl::StrContains(line, expected_import)) {
      found_expected_import = true;
    }
    EXPECT_FALSE(absl::StrContains(line, "importlib"));
  }
  EXPECT_TRUE(found_expected_import);
}

TEST(PythonPluginTest, PyiFieldTypeDisambiguation) {
  ABSL_CHECK_OK(
      File::SetContents(absl::StrCat(::testing::TempDir(), "/collision.proto"),
                        R"pb(
                          syntax = "proto3"
                          ;
package foo;

enum kind {
  KIND_UNSPECIFIED = 0;
}
message item {}
message container {
  int32 kind = 1;
  int32 item = 2;
  kind other_kind = 3;
  repeated kind repeated_kind = 4;
  item other_item = 5;
  repeated item repeated_item = 6;
  map<string, kind> map_kind = 7;
  map<string, item> map_item = 8;
}

enum shadowed_enum {
  TOP_ENUM_UNSPECIFIED = 0;
}
message shadowed_msg {}
message shadow_scope {
  enum shadowed_enum {
    NESTED_ENUM_UNSPECIFIED = 0;
  }
  message shadowed_msg {}
  message inner {
    foo.shadowed_enum top_enum_ref = 1;
    foo.shadowed_msg top_msg_ref = 2;
    shadowed_enum nested_enum_ref = 3;
    shadowed_msg nested_msg_ref = 4;
  }
}

message outer {
  enum nested_enum {
    NESTED_UNSPECIFIED = 0;
  }
  message nested_leaf {}
  message middle {
    message inner {
      int32 outer = 1;
      nested_enum other_nested_enum = 2;
      nested_leaf other_nested_leaf = 3;
      map<string, nested_enum> map_nested_enum = 4;
      map<string, nested_leaf> map_nested_leaf = 5;
    }
  }
}

enum map_val_enum {
  MAP_VAL_UNSPECIFIED = 0;
}
message map_val_msg {}
message map_only_container {
  int32 map_val_enum = 1;
  int32 map_val_msg = 2;
  map<string, map_val_enum> enum_map = 3;
  map<string, map_val_msg> msg_map = 4;
}
)pb",
                        true));

  compiler::CommandLineInterface cli;
  cli.SetInputsAreProtoPathRelative(true);
  python::PyiGenerator pyi_generator;
  cli.RegisterGenerator("--pyi_out", &pyi_generator, "");
  std::string proto_path = absl::StrCat("-I", ::testing::TempDir());
  std::string pyi_out = absl::StrCat("--pyi_out=", ::testing::TempDir());
  const char* argv[] = {"protoc", proto_path.c_str(), "-I.", pyi_out.c_str(),
                        "collision.proto"};
  ASSERT_EQ(0, cli.Run(5, argv));

  std::string output;
  ABSL_CHECK_OK(File::GetContents(
      absl::StrCat(::testing::TempDir(), "/collision_pb2.pyi"), &output,
      true));

  // 1. Verify top-level message with fields shadowing enum and message types.
  EXPECT_TRUE(absl::StrContains(output, R"pyi(
class container(_message.Message):
    __slots__ = ("kind", "item", "other_kind", "repeated_kind", "other_item", "repeated_item", "map_kind", "map_item")
    class MapKindEntry(_message.Message):
        __slots__ = ("key", "value")
        KEY_FIELD_NUMBER: _ClassVar[int]
        VALUE_FIELD_NUMBER: _ClassVar[int]
        key: str
        value: _Type_kind
        def __init__(self, key: _Optional[str] = ..., value: _Optional[_Union[_Type_kind, str]] = ...) -> None: ...
    class MapItemEntry(_message.Message):
        __slots__ = ("key", "value")
        KEY_FIELD_NUMBER: _ClassVar[int]
        VALUE_FIELD_NUMBER: _ClassVar[int]
        key: str
        value: _Type_item
        def __init__(self, key: _Optional[str] = ..., value: _Optional[_Union[_Type_item, _Mapping]] = ...) -> None: ...
    KIND_FIELD_NUMBER: _ClassVar[int]
    ITEM_FIELD_NUMBER: _ClassVar[int]
    OTHER_KIND_FIELD_NUMBER: _ClassVar[int]
    REPEATED_KIND_FIELD_NUMBER: _ClassVar[int]
    OTHER_ITEM_FIELD_NUMBER: _ClassVar[int]
    REPEATED_ITEM_FIELD_NUMBER: _ClassVar[int]
    MAP_KIND_FIELD_NUMBER: _ClassVar[int]
    MAP_ITEM_FIELD_NUMBER: _ClassVar[int]
    kind: int
    item: int
    other_kind: _Type_kind
    repeated_kind: _containers.RepeatedScalarFieldContainer[_Type_kind]
    other_item: _Type_item
    repeated_item: _containers.RepeatedCompositeFieldContainer[_Type_item]
    map_kind: _containers.ScalarMap[str, _Type_kind]
    map_item: _containers.MessageMap[str, _Type_item]
    def __init__(self, kind: _Optional[int] = ..., item: _Optional[int] = ..., other_kind: _Optional[_Union[_Type_kind, str]] = ..., repeated_kind: _Optional[_Iterable[_Union[_Type_kind, str]]] = ..., other_item: _Optional[_Union[_Type_item, _Mapping]] = ..., repeated_item: _Optional[_Iterable[_Union[_Type_item, _Mapping]]] = ..., map_kind: _Optional[_Mapping[str, _Type_kind]] = ..., map_item: _Optional[_Mapping[str, _Type_item]] = ...) -> None: ...
)pyi"));

  // 2. Verify nested scope referencing outer types vs nested types.
  EXPECT_TRUE(absl::StrContains(output, R"pyi(
    class inner(_message.Message):
        __slots__ = ("top_enum_ref", "top_msg_ref", "nested_enum_ref", "nested_msg_ref")
        TOP_ENUM_REF_FIELD_NUMBER: _ClassVar[int]
        TOP_MSG_REF_FIELD_NUMBER: _ClassVar[int]
        NESTED_ENUM_REF_FIELD_NUMBER: _ClassVar[int]
        NESTED_MSG_REF_FIELD_NUMBER: _ClassVar[int]
        top_enum_ref: _Type_shadowed_enum
        top_msg_ref: _Type_shadowed_msg
        nested_enum_ref: shadow_scope.shadowed_enum
        nested_msg_ref: shadow_scope.shadowed_msg
        def __init__(self, top_enum_ref: _Optional[_Union[_Type_shadowed_enum, str]] = ..., top_msg_ref: _Optional[_Union[_Type_shadowed_msg, _Mapping]] = ..., nested_enum_ref: _Optional[_Union[shadow_scope.shadowed_enum, str]] = ..., nested_msg_ref: _Optional[_Union[shadow_scope.shadowed_msg, _Mapping]] = ...) -> None: ...
)pyi"));

  // 3. Verify deeply nested message with int32 field shadowing outer class
  // name.
  EXPECT_TRUE(absl::StrContains(output, R"pyi(
        class inner(_message.Message):
            __slots__ = ("outer", "other_nested_enum", "other_nested_leaf", "map_nested_enum", "map_nested_leaf")
            class MapNestedEnumEntry(_message.Message):
                __slots__ = ("key", "value")
                KEY_FIELD_NUMBER: _ClassVar[int]
                VALUE_FIELD_NUMBER: _ClassVar[int]
                key: str
                value: _Type_outer.nested_enum
                def __init__(self, key: _Optional[str] = ..., value: _Optional[_Union[_Type_outer.nested_enum, str]] = ...) -> None: ...
            class MapNestedLeafEntry(_message.Message):
                __slots__ = ("key", "value")
                KEY_FIELD_NUMBER: _ClassVar[int]
                VALUE_FIELD_NUMBER: _ClassVar[int]
                key: str
                value: _Type_outer.nested_leaf
                def __init__(self, key: _Optional[str] = ..., value: _Optional[_Union[_Type_outer.nested_leaf, _Mapping]] = ...) -> None: ...
            OUTER_FIELD_NUMBER: _ClassVar[int]
            OTHER_NESTED_ENUM_FIELD_NUMBER: _ClassVar[int]
            OTHER_NESTED_LEAF_FIELD_NUMBER: _ClassVar[int]
            MAP_NESTED_ENUM_FIELD_NUMBER: _ClassVar[int]
            MAP_NESTED_LEAF_FIELD_NUMBER: _ClassVar[int]
            outer: int
            other_nested_enum: _Type_outer.nested_enum
            other_nested_leaf: _Type_outer.nested_leaf
            map_nested_enum: _containers.ScalarMap[str, _Type_outer.nested_enum]
            map_nested_leaf: _containers.MessageMap[str, _Type_outer.nested_leaf]
            def __init__(self, outer: _Optional[int] = ..., other_nested_enum: _Optional[_Union[_Type_outer.nested_enum, str]] = ..., other_nested_leaf: _Optional[_Union[_Type_outer.nested_leaf, _Mapping]] = ..., map_nested_enum: _Optional[_Mapping[str, _Type_outer.nested_enum]] = ..., map_nested_leaf: _Optional[_Mapping[str, _Type_outer.nested_leaf]] = ...) -> None: ...
)pyi"));

  // 4. Verify message where shadowing only occurs in map values.
  EXPECT_TRUE(absl::StrContains(output, R"pyi(
class map_only_container(_message.Message):
    __slots__ = ("map_val_enum", "map_val_msg", "enum_map", "msg_map")
    class EnumMapEntry(_message.Message):
        __slots__ = ("key", "value")
        KEY_FIELD_NUMBER: _ClassVar[int]
        VALUE_FIELD_NUMBER: _ClassVar[int]
        key: str
        value: _Type_map_val_enum
        def __init__(self, key: _Optional[str] = ..., value: _Optional[_Union[_Type_map_val_enum, str]] = ...) -> None: ...
    class MsgMapEntry(_message.Message):
        __slots__ = ("key", "value")
        KEY_FIELD_NUMBER: _ClassVar[int]
        VALUE_FIELD_NUMBER: _ClassVar[int]
        key: str
        value: _Type_map_val_msg
        def __init__(self, key: _Optional[str] = ..., value: _Optional[_Union[_Type_map_val_msg, _Mapping]] = ...) -> None: ...
    MAP_VAL_ENUM_FIELD_NUMBER: _ClassVar[int]
    MAP_VAL_MSG_FIELD_NUMBER: _ClassVar[int]
    ENUM_MAP_FIELD_NUMBER: _ClassVar[int]
    MSG_MAP_FIELD_NUMBER: _ClassVar[int]
    map_val_enum: int
    map_val_msg: int
    enum_map: _containers.ScalarMap[str, _Type_map_val_enum]
    msg_map: _containers.MessageMap[str, _Type_map_val_msg]
    def __init__(self, map_val_enum: _Optional[int] = ..., map_val_msg: _Optional[int] = ..., enum_map: _Optional[_Mapping[str, _Type_map_val_enum]] = ..., msg_map: _Optional[_Mapping[str, _Type_map_val_msg]] = ...) -> None: ...
)pyi"));

  // 5. Verify on-demand aliases are emitted only for shadowed types with a
  // preceding blank line.
  EXPECT_TRUE(absl::StrContains(output, "\n_Type_kind = kind\n"));
  EXPECT_TRUE(absl::StrContains(output, "\n_Type_item = item\n"));
  EXPECT_TRUE(absl::StrContains(output, "\n_Type_outer = outer\n"));
  EXPECT_TRUE(
      absl::StrContains(output, "\n_Type_shadowed_enum = shadowed_enum\n"));
  EXPECT_TRUE(
      absl::StrContains(output, "\n_Type_shadowed_msg = shadowed_msg\n"));
  EXPECT_TRUE(
      absl::StrContains(output, "\n_Type_map_val_enum = map_val_enum\n"));
  EXPECT_TRUE(absl::StrContains(output, "\n_Type_map_val_msg = map_val_msg\n"));
  EXPECT_FALSE(absl::StrContains(output, "_Type_container = container\n"));
  EXPECT_FALSE(
      absl::StrContains(output, "_Type_shadow_scope = shadow_scope\n"));
  EXPECT_FALSE(absl::StrContains(
      output, "_Type_map_only_container = map_only_container\n"));
  EXPECT_FALSE(absl::StrContains(output, "TypeAlias"));
}

class PythonGeneratorTest : public CommandLineInterfaceTester,
                            public testing::WithParamInterface<bool> {
 protected:
  PythonGeneratorTest() {
    auto generator = std::make_unique<Generator>();
    generator->set_opensource_runtime(GetParam());
    RegisterGenerator("--python_out", "--python_opt", std::move(generator),
                      "Python test generator");

    // Generate built-in protos.
    CreateTempFile(
        google::protobuf::DescriptorProto::descriptor()->file()->name(),
        google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  }
};

TEST_P(PythonGeneratorTest, PythonWithCppFeatures) {
  // Test that the presence of C++ features does not break Python generation.
  RegisterGenerator("--cpp_out", "--cpp_opt",
                    std::make_unique<cpp::CppGenerator>(),
                    "C++ test generator");
  CreateTempFile("google/protobuf/cpp_features.proto",
                 pb::CppFeatures::descriptor()->file()->DebugString());
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";

    import "google/protobuf/cpp_features.proto";

    package foo;
    
    enum Bar {
      AAA = 0;
      BBB = 1;
    }

    message Foo {
      Bar bar_enum = 1 [features.(pb.cpp).legacy_closed_enum = true];
    })schema");

  RunProtoc(absl::Substitute(
      "protocol_compiler --proto_path=$$tmpdir --cpp_out=$$tmpdir "
      "--python_out=$$tmpdir foo.proto $0 "
      "google/protobuf/cpp_features.proto",
      google::protobuf::DescriptorProto::descriptor()->file()->name()));

  ExpectNoErrors();
}

INSTANTIATE_TEST_SUITE_P(PythonGeneratorTest, PythonGeneratorTest,
                         testing::Bool());

}  // namespace
}  // namespace python
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
