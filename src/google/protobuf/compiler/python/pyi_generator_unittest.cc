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

TEST_F(PyiGeneratorTest, UsesPrivateTypesForKeywordMessages) {
  CreateTempFile("keyword.proto", R"schema(
    syntax = "proto3";
    package keyword_test;

    message None {}
    message _pb_message_0 {}

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

    message Plain {
      string value = 1;
    }

    message OnlyKeywordFields {
      string class = 1;
      string from = 2;
    }

    message MixedFields {
      string class = 1;
      string first = 2;
      string from = 3;
      string second = 4;
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
      "keyword_pb2.pyi", "class _pb_message_0_(_message.Message):");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi", "class _pb_message_0(_message.Message):");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi", "class _pb_message_2_0(_message.Message):");
  ExpectFileContentContainsSubstring("keyword_pb2.pyi",
                                     "top_level: _pb_message_0_");
  ExpectFileContentContainsSubstring("keyword_pb2.pyi",
                                     "value: Outer._pb_message_2_0");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi",
      "values: _containers.RepeatedCompositeFieldContainer["
      "Outer._pb_message_2_0]");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi",
      "by_name: _containers.MessageMap[str, Outer._pb_message_2_0]");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi", "_Union[Outer._pb_message_2_0, _Mapping]");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi", "class _pb_message_3(_message.Message):");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi", "class _pb_message_3_0(_message.Message):");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi", "value: _pb_message_3._pb_message_3_0");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi",
      "def __init__(self, *, value: _Optional[str] = ...) -> None: ...");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi", "def __init__(self, **kwargs) -> None: ...");
  ExpectFileContentContainsSubstring(
      "keyword_pb2.pyi",
      "def __init__(self, *, first: _Optional[str] = ..., second: "
      "_Optional[str] = ..., **kwargs) -> None: ...");
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
