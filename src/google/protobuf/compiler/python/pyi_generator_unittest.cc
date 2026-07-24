// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/python/pyi_generator.h"

#include <gtest/gtest.h>

#include <memory>

#include "google/protobuf/compiler/command_line_interface_tester.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace python {
namespace {

class PyiGeneratorTest : public CommandLineInterfaceTester {
 protected:
  PyiGeneratorTest() {
    RegisterGenerator("--pyi_out", "--pyi_opt",
                      std::make_unique<PyiGenerator>(), "Python pyi generator");
  }
};

TEST_F(PyiGeneratorTest, UsesPrivateTypeForKeywordMessage) {
  CreateTempFile("keyword.proto", R"schema(
    syntax = "proto3";
    package keyword_test;

    message None {}

    message Holder {
      None value = 1;
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --pyi_out=$tmpdir "
      "keyword.proto");
  ExpectNoErrors();

  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi", "class _pb_message_0(_message.Message):");
  ExpectFileContentContainsSubstring("keyword_pb2.pyi",
                                     "value: _pb_message_0");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi",
      "def __init__(self, value: "
      "_Optional[_Union[_pb_message_0, _Mapping]] = ...) -> None: ...");
  ExpectFileContentNotContainsSubstring("keyword_pb2.pyi", "class None(");
}

TEST_F(PyiGeneratorTest, ScopesPrivateTypesForKeywordMessages) {
  CreateTempFile("nested_scope.proto", R"schema(
    syntax = "proto3";
    package nested_scope_test;

    message None {}

    message Outer {
      // This nested binding must not perturb the module-level None.
      message _pb_message_0 {}
      message None {}
    }

    message Holder {
      None value = 1;
    }
  )schema");
  CreateTempFile("module_collision.proto", R"schema(
    syntax = "proto3";
    package module_collision_test;

    message None {}
    // This module-level binding must force the module-level None to move.
    message _pb_message_0 {}
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --pyi_out=$tmpdir "
      "nested_scope.proto module_collision.proto");
  ExpectNoErrors();

  ExpectFileContentContainsSubstring(
      "nested_scope_pb2.pyi", "class _pb_message_0(_message.Message):");
  ExpectFileContentContainsSubstring(
      "nested_scope_pb2.pyi", "class _pb_message_1_1(_message.Message):");
  ExpectFileContentContainsSubstring("nested_scope_pb2.pyi",
                                     "value: _pb_message_0");
  ExpectFileContentNotContainsSubstring(
      "nested_scope_pb2.pyi", "class _pb_message_0_(_message.Message):");

  ExpectFileContentContainsSubstring(
      "module_collision_pb2.pyi",
      "class _pb_message_0_(_message.Message):");
  ExpectFileContentContainsSubstring(
      "module_collision_pb2.pyi",
      "class _pb_message_0(_message.Message):");
}

TEST_F(PyiGeneratorTest, HandlesKeywordMessageReferences) {
  CreateTempFile("keyword.proto", R"schema(
    syntax = "proto3";
    package keyword_test;

    message None {}

    message Outer {
      message None {
        keyword_test.None top_level = 1;
      }

      None value = 1;
      repeated None values = 2;
      map<string, None> by_name = 3;
    }

    message False {
      message None {}
      None value = 1;
    }
  )schema");
  CreateTempFile("public_keyword.proto", R"schema(
    syntax = "proto3";
    package public_keyword_test;

    message None {}
  )schema");
  CreateTempFile("public_facade.proto", R"schema(
    syntax = "proto3";
    package public_keyword_test;

    import public "public_keyword.proto";

    message Holder {
      None value = 1;
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --pyi_out=$tmpdir "
      "keyword.proto public_keyword.proto public_facade.proto");
  ExpectNoErrors();

  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi", "class _pb_message_0(_message.Message):");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi", "class _pb_message_1_0(_message.Message):");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi", "top_level: _pb_message_0");
  ExpectFileContentContainsSubstring("keyword_pb2.pyi",
                                     "value: Outer._pb_message_1_0");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi",
      "values: _containers.RepeatedCompositeFieldContainer["
      "Outer._pb_message_1_0]");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi",
      "by_name: _containers.MessageMap[str, Outer._pb_message_1_0]");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi", "_Union[Outer._pb_message_1_0, _Mapping]");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi", "class _pb_message_2(_message.Message):");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi", "class _pb_message_2_0(_message.Message):");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi", "value: _pb_message_2._pb_message_2_0");
  ExpectFileContentNotContainsSubstring("keyword_pb2.pyi", "class None(");
  ExpectFileContentNotContainsSubstring("keyword_pb2.pyi", "class False(");

  ExpectFileContentContainsSubstring(
      "public_facade_pb2.pyi",
      "value: _public_keyword_pb2._pb_message_0");
  ExpectFileContentContainsSubstring(
      "public_facade_pb2.pyi",
      "_Union[_public_keyword_pb2._pb_message_0, _Mapping]");
  ExpectFileContentNotContainsSubstring(
      "public_facade_pb2.pyi", "from public_keyword_pb2 import None");
}

}  // namespace
}  // namespace python
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
