// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <vector>
#include <algorithm>

#include <google/protobuf/compiler/parser.h>

#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/unittest.pb.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>

#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace compiler {

namespace {

class MockErrorCollector : public io::ErrorCollector {
 public:
  MockErrorCollector() {}
  ~MockErrorCollector() {}

  string text_;

  // implements ErrorCollector ---------------------------------------
  void AddError(int line, int column, const string& message) {
    strings::SubstituteAndAppend(&text_, "$0:$1: $2\n",
                                 line, column, message);
  }
};

class MockValidationErrorCollector : public DescriptorPool::ErrorCollector {
 public:
  MockValidationErrorCollector(const SourceLocationTable& source_locations,
                               io::ErrorCollector* wrapped_collector)
    : source_locations_(source_locations),
      wrapped_collector_(wrapped_collector) {}
  ~MockValidationErrorCollector() {}

  // implements ErrorCollector ---------------------------------------
  void AddError(const string& filename,
                const string& element_name,
                const Message* descriptor,
                ErrorLocation location,
                const string& message) {
    int line, column;
    source_locations_.Find(descriptor, location, &line, &column);
    wrapped_collector_->AddError(line, column, message);
  }

 private:
  const SourceLocationTable& source_locations_;
  io::ErrorCollector* wrapped_collector_;
};

class ParserTest : public testing::Test {
 protected:
  ParserTest()
    : require_syntax_identifier_(false) {}

  // Set up the parser to parse the given text.
  void SetupParser(const char* text) {
    raw_input_.reset(new io::ArrayInputStream(text, strlen(text)));
    input_.reset(new io::Tokenizer(raw_input_.get(), &error_collector_));
    parser_.reset(new Parser());
    parser_->RecordErrorsTo(&error_collector_);
    parser_->SetRequireSyntaxIdentifier(require_syntax_identifier_);
  }

  // Parse the input and expect that the resulting FileDescriptorProto matches
  // the given output.  The output is a FileDescriptorProto in protocol buffer
  // text format.
  void ExpectParsesTo(const char* input, const char* output) {
    SetupParser(input);
    FileDescriptorProto actual, expected;

    parser_->Parse(input_.get(), &actual);
    EXPECT_EQ(io::Tokenizer::TYPE_END, input_->current().type);
    ASSERT_EQ("", error_collector_.text_);

    // Parse the ASCII representation in order to canonicalize it.  We could
    // just compare directly to actual.DebugString(), but that would require
    // that the caller precisely match the formatting that DebugString()
    // produces.
    ASSERT_TRUE(TextFormat::ParseFromString(output, &expected));

    // Compare by comparing debug strings.
    // TODO(kenton):  Use differencer, once it is available.
    EXPECT_EQ(expected.DebugString(), actual.DebugString());
  }

  // Parse the text and expect that the given errors are reported.
  void ExpectHasErrors(const char* text, const char* expected_errors) {
    ExpectHasEarlyExitErrors(text, expected_errors);
    EXPECT_EQ(io::Tokenizer::TYPE_END, input_->current().type);
  }

  // Same as above but does not expect that the parser parses the complete
  // input.
  void ExpectHasEarlyExitErrors(const char* text, const char* expected_errors) {
    SetupParser(text);
    FileDescriptorProto file;
    parser_->Parse(input_.get(), &file);
    EXPECT_EQ(expected_errors, error_collector_.text_);
  }

  // Parse the text as a file and validate it (with a DescriptorPool), and
  // expect that the validation step reports the given errors.
  void ExpectHasValidationErrors(const char* text,
                                 const char* expected_errors) {
    SetupParser(text);
    SourceLocationTable source_locations;
    parser_->RecordSourceLocationsTo(&source_locations);

    FileDescriptorProto file;
    file.set_name("foo.proto");
    parser_->Parse(input_.get(), &file);
    EXPECT_EQ(io::Tokenizer::TYPE_END, input_->current().type);
    ASSERT_EQ("", error_collector_.text_);

    MockValidationErrorCollector validation_error_collector(
      source_locations, &error_collector_);
    EXPECT_TRUE(pool_.BuildFileCollectingErrors(
      file, &validation_error_collector) == NULL);
    EXPECT_EQ(expected_errors, error_collector_.text_);
  }

  MockErrorCollector error_collector_;
  DescriptorPool pool_;

  scoped_ptr<io::ZeroCopyInputStream> raw_input_;
  scoped_ptr<io::Tokenizer> input_;
  scoped_ptr<Parser> parser_;
  bool require_syntax_identifier_;
};

// ===================================================================

typedef ParserTest ParseMessageTest;

TEST_F(ParseMessageTest, SimpleMessage) {
  ExpectParsesTo(
    "message TestMessage {\n"
    "  required int32 foo = 1;\n"
    "}\n",

    "message_type {"
    "  name: \"TestMessage\""
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_INT32 number:1 }"
    "}");
}

TEST_F(ParseMessageTest, ImplicitSyntaxIdentifier) {
  require_syntax_identifier_ = false;
  ExpectParsesTo(
    "message TestMessage {\n"
    "  required int32 foo = 1;\n"
    "}\n",

    "message_type {"
    "  name: \"TestMessage\""
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_INT32 number:1 }"
    "}");
  EXPECT_EQ("proto2", parser_->GetSyntaxIndentifier());
}

TEST_F(ParseMessageTest, ExplicitSyntaxIdentifier) {
  ExpectParsesTo(
    "syntax = \"proto2\";\n"
    "message TestMessage {\n"
    "  required int32 foo = 1;\n"
    "}\n",

    "message_type {"
    "  name: \"TestMessage\""
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_INT32 number:1 }"
    "}");
  EXPECT_EQ("proto2", parser_->GetSyntaxIndentifier());
}

TEST_F(ParseMessageTest, ExplicitRequiredSyntaxIdentifier) {
  require_syntax_identifier_ = true;
  ExpectParsesTo(
    "syntax = \"proto2\";\n"
    "message TestMessage {\n"
    "  required int32 foo = 1;\n"
    "}\n",

    "message_type {"
    "  name: \"TestMessage\""
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_INT32 number:1 }"
    "}");
  EXPECT_EQ("proto2", parser_->GetSyntaxIndentifier());
}

TEST_F(ParseMessageTest, SimpleFields) {
  ExpectParsesTo(
    "message TestMessage {\n"
    "  required int32 foo = 15;\n"
    "  optional int32 bar = 34;\n"
    "  repeated int32 baz = 3;\n"
    "}\n",

    "message_type {"
    "  name: \"TestMessage\""
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_INT32 number:15 }"
    "  field { name:\"bar\" label:LABEL_OPTIONAL type:TYPE_INT32 number:34 }"
    "  field { name:\"baz\" label:LABEL_REPEATED type:TYPE_INT32 number:3  }"
    "}");
}

TEST_F(ParseMessageTest, PrimitiveFieldTypes) {
  ExpectParsesTo(
    "message TestMessage {\n"
    "  required int32    foo = 1;\n"
    "  required int64    foo = 1;\n"
    "  required uint32   foo = 1;\n"
    "  required uint64   foo = 1;\n"
    "  required sint32   foo = 1;\n"
    "  required sint64   foo = 1;\n"
    "  required fixed32  foo = 1;\n"
    "  required fixed64  foo = 1;\n"
    "  required sfixed32 foo = 1;\n"
    "  required sfixed64 foo = 1;\n"
    "  required float    foo = 1;\n"
    "  required double   foo = 1;\n"
    "  required string   foo = 1;\n"
    "  required bytes    foo = 1;\n"
    "  required bool     foo = 1;\n"
    "}\n",

    "message_type {"
    "  name: \"TestMessage\""
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_INT32    number:1 }"
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_INT64    number:1 }"
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_UINT32   number:1 }"
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_UINT64   number:1 }"
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_SINT32   number:1 }"
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_SINT64   number:1 }"
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_FIXED32  number:1 }"
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_FIXED64  number:1 }"
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_SFIXED32 number:1 }"
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_SFIXED64 number:1 }"
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_FLOAT    number:1 }"
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_DOUBLE   number:1 }"
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_STRING   number:1 }"
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_BYTES    number:1 }"
    "  field { name:\"foo\" label:LABEL_REQUIRED type:TYPE_BOOL     number:1 }"
    "}");
}

TEST_F(ParseMessageTest, FieldDefaults) {
  ExpectParsesTo(
    "message TestMessage {\n"
    "  required int32  foo = 1 [default=  1  ];\n"
    "  required int32  foo = 1 [default= -2  ];\n"
    "  required int64  foo = 1 [default=  3  ];\n"
    "  required int64  foo = 1 [default= -4  ];\n"
    "  required uint32 foo = 1 [default=  5  ];\n"
    "  required uint64 foo = 1 [default=  6  ];\n"
    "  required float  foo = 1 [default=  7.5];\n"
    "  required float  foo = 1 [default= -8.5];\n"
    "  required float  foo = 1 [default=  9  ];\n"
    "  required double foo = 1 [default= 10.5];\n"
    "  required double foo = 1 [default=-11.5];\n"
    "  required double foo = 1 [default= 12  ];\n"
    "  required string foo = 1 [default='13\\001'];\n"
    "  required string foo = 1 [default='a' \"b\" \n \"c\"];\n"
    "  required bytes  foo = 1 [default='14\\002'];\n"
    "  required bytes  foo = 1 [default='a' \"b\" \n 'c'];\n"
    "  required bool   foo = 1 [default=true ];\n"
    "  required Foo    foo = 1 [default=FOO  ];\n"

    "  required int32  foo = 1 [default= 0x7FFFFFFF];\n"
    "  required int32  foo = 1 [default=-0x80000000];\n"
    "  required uint32 foo = 1 [default= 0xFFFFFFFF];\n"
    "  required int64  foo = 1 [default= 0x7FFFFFFFFFFFFFFF];\n"
    "  required int64  foo = 1 [default=-0x8000000000000000];\n"
    "  required uint64 foo = 1 [default= 0xFFFFFFFFFFFFFFFF];\n"
    "  required double foo = 1 [default= 0xabcd];\n"
    "}\n",

#define ETC "name:\"foo\" label:LABEL_REQUIRED number:1"
    "message_type {"
    "  name: \"TestMessage\""
    "  field { type:TYPE_INT32   default_value:\"1\"         "ETC" }"
    "  field { type:TYPE_INT32   default_value:\"-2\"        "ETC" }"
    "  field { type:TYPE_INT64   default_value:\"3\"         "ETC" }"
    "  field { type:TYPE_INT64   default_value:\"-4\"        "ETC" }"
    "  field { type:TYPE_UINT32  default_value:\"5\"         "ETC" }"
    "  field { type:TYPE_UINT64  default_value:\"6\"         "ETC" }"
    "  field { type:TYPE_FLOAT   default_value:\"7.5\"       "ETC" }"
    "  field { type:TYPE_FLOAT   default_value:\"-8.5\"      "ETC" }"
    "  field { type:TYPE_FLOAT   default_value:\"9\"         "ETC" }"
    "  field { type:TYPE_DOUBLE  default_value:\"10.5\"      "ETC" }"
    "  field { type:TYPE_DOUBLE  default_value:\"-11.5\"     "ETC" }"
    "  field { type:TYPE_DOUBLE  default_value:\"12\"        "ETC" }"
    "  field { type:TYPE_STRING  default_value:\"13\\001\"   "ETC" }"
    "  field { type:TYPE_STRING  default_value:\"abc\"       "ETC" }"
    "  field { type:TYPE_BYTES   default_value:\"14\\\\002\" "ETC" }"
    "  field { type:TYPE_BYTES   default_value:\"abc\"       "ETC" }"
    "  field { type:TYPE_BOOL    default_value:\"true\"      "ETC" }"
    "  field { type_name:\"Foo\" default_value:\"FOO\"       "ETC" }"

    "  field { type:TYPE_INT32   default_value:\"2147483647\"           "ETC" }"
    "  field { type:TYPE_INT32   default_value:\"-2147483648\"          "ETC" }"
    "  field { type:TYPE_UINT32  default_value:\"4294967295\"           "ETC" }"
    "  field { type:TYPE_INT64   default_value:\"9223372036854775807\"  "ETC" }"
    "  field { type:TYPE_INT64   default_value:\"-9223372036854775808\" "ETC" }"
    "  field { type:TYPE_UINT64  default_value:\"18446744073709551615\" "ETC" }"
    "  field { type:TYPE_DOUBLE  default_value:\"43981\"                "ETC" }"
    "}");
#undef ETC
}

TEST_F(ParseMessageTest, FieldOptions) {
  ExpectParsesTo(
    "message TestMessage {\n"
    "  optional string foo = 1\n"
    "      [ctype=CORD, (foo)=7, foo.(.bar.baz).qux.quux.(corge)=-33, \n"
    "       (quux)=\"x\040y\", (baz.qux)=hey];\n"
    "}\n",

    "message_type {"
    "  name: \"TestMessage\""
    "  field { name: \"foo\" label: LABEL_OPTIONAL type: TYPE_STRING number: 1"
    "          options { uninterpreted_option: { name { name_part: \"ctype\" "
    "                                                   is_extension: false } "
    "                                            identifier_value: \"CORD\"  }"
    "                    uninterpreted_option: { name { name_part: \"foo\" "
    "                                                   is_extension: true } "
    "                                            positive_int_value: 7  }"
    "                    uninterpreted_option: { name { name_part: \"foo\" "
    "                                                   is_extension: false } "
    "                                            name { name_part: \".bar.baz\""
    "                                                   is_extension: true } "
    "                                            name { name_part: \"qux\" "
    "                                                   is_extension: false } "
    "                                            name { name_part: \"quux\" "
    "                                                   is_extension: false } "
    "                                            name { name_part: \"corge\" "
    "                                                   is_extension: true } "
    "                                            negative_int_value: -33 }"
    "                    uninterpreted_option: { name { name_part: \"quux\" "
    "                                                   is_extension: true } "
    "                                            string_value: \"x y\" }"
    "                    uninterpreted_option: { name { name_part: \"baz.qux\" "
    "                                                   is_extension: true } "
    "                                            identifier_value: \"hey\" }"
    "          }"
    "  }"
    "}");
}

TEST_F(ParseMessageTest, Group) {
  ExpectParsesTo(
    "message TestMessage {\n"
    "  optional group TestGroup = 1 {};\n"
    "}\n",

    "message_type {"
    "  name: \"TestMessage\""
    "  nested_type { name: \"TestGroup\" }"
    "  field { name:\"testgroup\" label:LABEL_OPTIONAL number:1"
    "          type:TYPE_GROUP type_name: \"TestGroup\" }"
    "}");
}

TEST_F(ParseMessageTest, NestedMessage) {
  ExpectParsesTo(
    "message TestMessage {\n"
    "  message Nested {}\n"
    "  optional Nested test_nested = 1;\n"
    "}\n",

    "message_type {"
    "  name: \"TestMessage\""
    "  nested_type { name: \"Nested\" }"
    "  field { name:\"test_nested\" label:LABEL_OPTIONAL number:1"
    "          type_name: \"Nested\" }"
    "}");
}

TEST_F(ParseMessageTest, NestedEnum) {
  ExpectParsesTo(
    "message TestMessage {\n"
    "  enum NestedEnum {}\n"
    "  optional NestedEnum test_enum = 1;\n"
    "}\n",

    "message_type {"
    "  name: \"TestMessage\""
    "  enum_type { name: \"NestedEnum\" }"
    "  field { name:\"test_enum\" label:LABEL_OPTIONAL number:1"
    "          type_name: \"NestedEnum\" }"
    "}");
}

TEST_F(ParseMessageTest, ExtensionRange) {
  ExpectParsesTo(
    "message TestMessage {\n"
    "  extensions 10 to 19;\n"
    "  extensions 30 to max;\n"
    "}\n",

    "message_type {"
    "  name: \"TestMessage\""
    "  extension_range { start:10 end:20        }"
    "  extension_range { start:30 end:536870912 }"
    "}");
}

TEST_F(ParseMessageTest, CompoundExtensionRange) {
  ExpectParsesTo(
    "message TestMessage {\n"
    "  extensions 2, 15, 9 to 11, 100 to max, 3;\n"
    "}\n",

    "message_type {"
    "  name: \"TestMessage\""
    "  extension_range { start:2   end:3         }"
    "  extension_range { start:15  end:16        }"
    "  extension_range { start:9   end:12        }"
    "  extension_range { start:100 end:536870912 }"
    "  extension_range { start:3   end:4         }"
    "}");
}

TEST_F(ParseMessageTest, Extensions) {
  ExpectParsesTo(
    "extend Extendee1 { optional int32 foo = 12; }\n"
    "extend Extendee2 { repeated TestMessage bar = 22; }\n",

    "extension { name:\"foo\" label:LABEL_OPTIONAL type:TYPE_INT32 number:12"
    "            extendee: \"Extendee1\" } "
    "extension { name:\"bar\" label:LABEL_REPEATED number:22"
    "            type_name:\"TestMessage\" extendee: \"Extendee2\" }");
}

TEST_F(ParseMessageTest, ExtensionsInMessageScope) {
  ExpectParsesTo(
    "message TestMessage {\n"
    "  extend Extendee1 { optional int32 foo = 12; }\n"
    "  extend Extendee2 { repeated TestMessage bar = 22; }\n"
    "}\n",

    "message_type {"
    "  name: \"TestMessage\""
    "  extension { name:\"foo\" label:LABEL_OPTIONAL type:TYPE_INT32 number:12"
    "              extendee: \"Extendee1\" }"
    "  extension { name:\"bar\" label:LABEL_REPEATED number:22"
    "              type_name:\"TestMessage\" extendee: \"Extendee2\" }"
    "}");
}

TEST_F(ParseMessageTest, MultipleExtensionsOneExtendee) {
  ExpectParsesTo(
    "extend Extendee1 {\n"
    "  optional int32 foo = 12;\n"
    "  repeated TestMessage bar = 22;\n"
    "}\n",

    "extension { name:\"foo\" label:LABEL_OPTIONAL type:TYPE_INT32 number:12"
    "            extendee: \"Extendee1\" } "
    "extension { name:\"bar\" label:LABEL_REPEATED number:22"
    "            type_name:\"TestMessage\" extendee: \"Extendee1\" }");
}

// ===================================================================

typedef ParserTest ParseEnumTest;

TEST_F(ParseEnumTest, SimpleEnum) {
  ExpectParsesTo(
    "enum TestEnum {\n"
    "  FOO = 0;\n"
    "}\n",

    "enum_type {"
    "  name: \"TestEnum\""
    "  value { name:\"FOO\" number:0 }"
    "}");
}

TEST_F(ParseEnumTest, Values) {
  ExpectParsesTo(
    "enum TestEnum {\n"
    "  FOO = 13;\n"
    "  BAR = -10;\n"
    "  BAZ = 500;\n"
    "}\n",

    "enum_type {"
    "  name: \"TestEnum\""
    "  value { name:\"FOO\" number:13 }"
    "  value { name:\"BAR\" number:-10 }"
    "  value { name:\"BAZ\" number:500 }"
    "}");
}

TEST_F(ParseEnumTest, ValueOptions) {
  ExpectParsesTo(
    "enum TestEnum {\n"
    "  FOO = 13;\n"
    "  BAR = -10 [ (something.text) = 'abc' ];\n"
    "  BAZ = 500 [ (something.text) = 'def', other = 1 ];\n"
    "}\n",

    "enum_type {"
    "  name: \"TestEnum\""
    "  value { name: \"FOO\" number: 13 }"
    "  value { name: \"BAR\" number: -10 "
    "    options { "
    "      uninterpreted_option { "
    "        name { name_part: \"something.text\" is_extension: true } "
    "        string_value: \"abc\" "
    "      } "
    "    } "
    "  } "
    "  value { name: \"BAZ\" number: 500 "
    "    options { "
    "      uninterpreted_option { "
    "        name { name_part: \"something.text\" is_extension: true } "
    "        string_value: \"def\" "
    "      } "
    "      uninterpreted_option { "
    "        name { name_part: \"other\" is_extension: false } "
    "        positive_int_value: 1 "
    "      } "
    "    } "
    "  } "
    "}");
}

// ===================================================================

typedef ParserTest ParseServiceTest;

TEST_F(ParseServiceTest, SimpleService) {
  ExpectParsesTo(
    "service TestService {\n"
    "  rpc Foo(In) returns (Out);\n"
    "}\n",

    "service {"
    "  name: \"TestService\""
    "  method { name:\"Foo\" input_type:\"In\" output_type:\"Out\" }"
    "}");
}

TEST_F(ParseServiceTest, Methods) {
  ExpectParsesTo(
    "service TestService {\n"
    "  rpc Foo(In1) returns (Out1);\n"
    "  rpc Bar(In2) returns (Out2);\n"
    "  rpc Baz(In3) returns (Out3);\n"
    "}\n",

    "service {"
    "  name: \"TestService\""
    "  method { name:\"Foo\" input_type:\"In1\" output_type:\"Out1\" }"
    "  method { name:\"Bar\" input_type:\"In2\" output_type:\"Out2\" }"
    "  method { name:\"Baz\" input_type:\"In3\" output_type:\"Out3\" }"
    "}");
}

// ===================================================================
// imports and packages

typedef ParserTest ParseMiscTest;

TEST_F(ParseMiscTest, ParseImport) {
  ExpectParsesTo(
    "import \"foo/bar/baz.proto\";\n",
    "dependency: \"foo/bar/baz.proto\"");
}

TEST_F(ParseMiscTest, ParseMultipleImports) {
  ExpectParsesTo(
    "import \"foo.proto\";\n"
    "import \"bar.proto\";\n"
    "import \"baz.proto\";\n",
    "dependency: \"foo.proto\""
    "dependency: \"bar.proto\""
    "dependency: \"baz.proto\"");
}

TEST_F(ParseMiscTest, ParsePackage) {
  ExpectParsesTo(
    "package foo.bar.baz;\n",
    "package: \"foo.bar.baz\"");
}

TEST_F(ParseMiscTest, ParsePackageWithSpaces) {
  ExpectParsesTo(
    "package foo   .   bar.  \n"
    "  baz;\n",
    "package: \"foo.bar.baz\"");
}

// ===================================================================
// options

TEST_F(ParseMiscTest, ParseFileOptions) {
  ExpectParsesTo(
    "option java_package = \"com.google.foo\";\n"
    "option optimize_for = CODE_SIZE;",

    "options {"
    "uninterpreted_option { name { name_part: \"java_package\" "
    "                              is_extension: false }"
    "                       string_value: \"com.google.foo\"} "
    "uninterpreted_option { name { name_part: \"optimize_for\" "
    "                              is_extension: false }"
    "                       identifier_value: \"CODE_SIZE\" } "
    "}");
}

// ===================================================================
// Error tests
//
// There are a very large number of possible errors that the parser could
// report, so it's infeasible to test every single one of them.  Instead,
// we test each unique call to AddError() in parser.h.  This does not mean
// we are testing every possible error that Parser can generate because
// each variant of the Consume() helper only counts as one unique call to
// AddError().

typedef ParserTest ParseErrorTest;

TEST_F(ParseErrorTest, MissingSyntaxIdentifier) {
  require_syntax_identifier_ = true;
  ExpectHasEarlyExitErrors(
    "message TestMessage {}",
    "0:0: File must begin with 'syntax = \"proto2\";'.\n");
  EXPECT_EQ("", parser_->GetSyntaxIndentifier());
}

TEST_F(ParseErrorTest, UnknownSyntaxIdentifier) {
  ExpectHasEarlyExitErrors(
    "syntax = \"no_such_syntax\";",
    "0:9: Unrecognized syntax identifier \"no_such_syntax\".  This parser "
      "only recognizes \"proto2\".\n");
  EXPECT_EQ("no_such_syntax", parser_->GetSyntaxIndentifier());
}

TEST_F(ParseErrorTest, SimpleSyntaxError) {
  ExpectHasErrors(
    "message TestMessage @#$ { blah }",
    "0:20: Expected \"{\".\n");
  EXPECT_EQ("proto2", parser_->GetSyntaxIndentifier());
}

TEST_F(ParseErrorTest, ExpectedTopLevel) {
  ExpectHasErrors(
    "blah;",
    "0:0: Expected top-level statement (e.g. \"message\").\n");
}

TEST_F(ParseErrorTest, UnmatchedCloseBrace) {
  // This used to cause an infinite loop.  Doh.
  ExpectHasErrors(
    "}",
    "0:0: Expected top-level statement (e.g. \"message\").\n"
    "0:0: Unmatched \"}\".\n");
}

// -------------------------------------------------------------------
// Message errors

TEST_F(ParseErrorTest, MessageMissingName) {
  ExpectHasErrors(
    "message {}",
    "0:8: Expected message name.\n");
}

TEST_F(ParseErrorTest, MessageMissingBody) {
  ExpectHasErrors(
    "message TestMessage;",
    "0:19: Expected \"{\".\n");
}

TEST_F(ParseErrorTest, EofInMessage) {
  ExpectHasErrors(
    "message TestMessage {",
    "0:21: Reached end of input in message definition (missing '}').\n");
}

TEST_F(ParseErrorTest, MissingFieldNumber) {
  ExpectHasErrors(
    "message TestMessage {\n"
    "  optional int32 foo;\n"
    "}\n",
    "1:20: Missing field number.\n");
}

TEST_F(ParseErrorTest, ExpectedFieldNumber) {
  ExpectHasErrors(
    "message TestMessage {\n"
    "  optional int32 foo = ;\n"
    "}\n",
    "1:23: Expected field number.\n");
}

TEST_F(ParseErrorTest, FieldNumberOutOfRange) {
  ExpectHasErrors(
    "message TestMessage {\n"
    "  optional int32 foo = 0x100000000;\n"
    "}\n",
    "1:23: Integer out of range.\n");
}

TEST_F(ParseErrorTest, MissingLabel) {
  ExpectHasErrors(
    "message TestMessage {\n"
    "  int32 foo = 1;\n"
    "}\n",
    "1:2: Expected \"required\", \"optional\", or \"repeated\".\n");
}

TEST_F(ParseErrorTest, ExpectedOptionName) {
  ExpectHasErrors(
    "message TestMessage {\n"
    "  optional uint32 foo = 1 [];\n"
    "}\n",
    "1:27: Expected identifier.\n");
}

TEST_F(ParseErrorTest, NonExtensionOptionNameBeginningWithDot) {
  ExpectHasErrors(
    "message TestMessage {\n"
    "  optional uint32 foo = 1 [.foo=1];\n"
    "}\n",
    "1:27: Expected identifier.\n");
}

TEST_F(ParseErrorTest, DefaultValueTypeMismatch) {
  ExpectHasErrors(
    "message TestMessage {\n"
    "  optional uint32 foo = 1 [default=true];\n"
    "}\n",
    "1:35: Expected integer.\n");
}

TEST_F(ParseErrorTest, DefaultValueNotBoolean) {
  ExpectHasErrors(
    "message TestMessage {\n"
    "  optional bool foo = 1 [default=blah];\n"
    "}\n",
    "1:33: Expected \"true\" or \"false\".\n");
}

TEST_F(ParseErrorTest, DefaultValueNotString) {
  ExpectHasErrors(
    "message TestMessage {\n"
    "  optional string foo = 1 [default=1];\n"
    "}\n",
    "1:35: Expected string.\n");
}

TEST_F(ParseErrorTest, DefaultValueUnsignedNegative) {
  ExpectHasErrors(
    "message TestMessage {\n"
    "  optional uint32 foo = 1 [default=-1];\n"
    "}\n",
    "1:36: Unsigned field can't have negative default value.\n");
}

TEST_F(ParseErrorTest, DefaultValueTooLarge) {
  ExpectHasErrors(
    "message TestMessage {\n"
    "  optional int32  foo = 1 [default= 0x80000000];\n"
    "  optional int32  foo = 1 [default=-0x80000001];\n"
    "  optional uint32 foo = 1 [default= 0x100000000];\n"
    "  optional int64  foo = 1 [default= 0x80000000000000000];\n"
    "  optional int64  foo = 1 [default=-0x80000000000000001];\n"
    "  optional uint64 foo = 1 [default= 0x100000000000000000];\n"
    "}\n",
    "1:36: Integer out of range.\n"
    "2:36: Integer out of range.\n"
    "3:36: Integer out of range.\n"
    "4:36: Integer out of range.\n"
    "5:36: Integer out of range.\n"
    "6:36: Integer out of range.\n");
}

TEST_F(ParseErrorTest, DefaultValueMissing) {
  ExpectHasErrors(
    "message TestMessage {\n"
    "  optional uint32 foo = 1 [default=];\n"
    "}\n",
    "1:35: Expected integer.\n");
}

TEST_F(ParseErrorTest, DefaultValueForGroup) {
  ExpectHasErrors(
    "message TestMessage {\n"
    "  optional group Foo = 1 [default=blah] {}\n"
    "}\n",
    "1:34: Messages can't have default values.\n");
}

TEST_F(ParseErrorTest, DuplicateDefaultValue) {
  ExpectHasErrors(
    "message TestMessage {\n"
    "  optional uint32 foo = 1 [default=1,default=2];\n"
    "}\n",
    "1:37: Already set option \"default\".\n");
}

TEST_F(ParseErrorTest, GroupNotCapitalized) {
  ExpectHasErrors(
    "message TestMessage {\n"
    "  optional group foo = 1 {}\n"
    "}\n",
    "1:17: Group names must start with a capital letter.\n");
}

TEST_F(ParseErrorTest, GroupMissingBody) {
  ExpectHasErrors(
    "message TestMessage {\n"
    "  optional group Foo = 1;\n"
    "}\n",
    "1:24: Missing group body.\n");
}

TEST_F(ParseErrorTest, ExtendingPrimitive) {
  ExpectHasErrors(
    "extend int32 { optional string foo = 4; }\n",
    "0:7: Expected message type.\n");
}

TEST_F(ParseErrorTest, ErrorInExtension) {
  ExpectHasErrors(
    "message Foo { extensions 100 to 199; }\n"
    "extend Foo { optional string foo; }\n",
    "1:32: Missing field number.\n");
}

TEST_F(ParseErrorTest, MultipleParseErrors) {
  // When a statement has a parse error, the parser should be able to continue
  // parsing at the next statement.
  ExpectHasErrors(
    "message TestMessage {\n"
    "  optional int32 foo;\n"
    "  !invalid statement ending in a block { blah blah { blah } blah }\n"
    "  optional int32 bar = 3 {}\n"
    "}\n",
    "1:20: Missing field number.\n"
    "2:2: Expected \"required\", \"optional\", or \"repeated\".\n"
    "2:2: Expected type name.\n"
    "3:25: Expected \";\".\n");
}

// -------------------------------------------------------------------
// Enum errors

TEST_F(ParseErrorTest, EofInEnum) {
  ExpectHasErrors(
    "enum TestEnum {",
    "0:15: Reached end of input in enum definition (missing '}').\n");
}

TEST_F(ParseErrorTest, EnumValueMissingNumber) {
  ExpectHasErrors(
    "enum TestEnum {\n"
    "  FOO;\n"
    "}\n",
    "1:5: Missing numeric value for enum constant.\n");
}

// -------------------------------------------------------------------
// Service errors

TEST_F(ParseErrorTest, EofInService) {
  ExpectHasErrors(
    "service TestService {",
    "0:21: Reached end of input in service definition (missing '}').\n");
}

TEST_F(ParseErrorTest, ServiceMethodPrimitiveParams) {
  ExpectHasErrors(
    "service TestService {\n"
    "  rpc Foo(int32) returns (string);\n"
    "}\n",
    "1:10: Expected message type.\n"
    "1:26: Expected message type.\n");
}

TEST_F(ParseErrorTest, EofInMethodOptions) {
  ExpectHasErrors(
    "service TestService {\n"
    "  rpc Foo(Bar) returns(Bar) {",
    "1:29: Reached end of input in method options (missing '}').\n"
    "1:29: Reached end of input in service definition (missing '}').\n");
}

TEST_F(ParseErrorTest, PrimitiveMethodInput) {
  ExpectHasErrors(
    "service TestService {\n"
    "  rpc Foo(int32) returns(Bar);\n"
    "}\n",
    "1:10: Expected message type.\n");
}

TEST_F(ParseErrorTest, MethodOptionTypeError) {
  // This used to cause an infinite loop.
  ExpectHasErrors(
    "message Baz {}\n"
    "service Foo {\n"
    "  rpc Bar(Baz) returns(Baz) { option invalid syntax; }\n"
    "}\n",
    "2:45: Expected \"=\".\n");
}

// -------------------------------------------------------------------
// Import and package errors

TEST_F(ParseErrorTest, ImportNotQuoted) {
  ExpectHasErrors(
    "import foo;\n",
    "0:7: Expected a string naming the file to import.\n");
}

TEST_F(ParseErrorTest, MultiplePackagesInFile) {
  ExpectHasErrors(
    "package foo;\n"
    "package bar;\n",
    "1:0: Multiple package definitions.\n");
}

// ===================================================================
// Test that errors detected by DescriptorPool correctly report line and
// column numbers.  We have one test for every call to RecordLocation() in
// parser.cc.

typedef ParserTest ParserValidationErrorTest;

TEST_F(ParserValidationErrorTest, PackageNameError) {
  // Create another file which defines symbol "foo".
  FileDescriptorProto other_file;
  other_file.set_name("bar.proto");
  other_file.add_message_type()->set_name("foo");
  EXPECT_TRUE(pool_.BuildFile(other_file) != NULL);

  // Now try to define it as a package.
  ExpectHasValidationErrors(
    "package foo.bar;",
    "0:8: \"foo\" is already defined (as something other than a package) "
      "in file \"bar.proto\".\n");
}

TEST_F(ParserValidationErrorTest, MessageNameError) {
  ExpectHasValidationErrors(
    "message Foo {}\n"
    "message Foo {}\n",
    "1:8: \"Foo\" is already defined.\n");
}

TEST_F(ParserValidationErrorTest, FieldNameError) {
  ExpectHasValidationErrors(
    "message Foo {\n"
    "  optional int32 bar = 1;\n"
    "  optional int32 bar = 2;\n"
    "}\n",
    "2:17: \"bar\" is already defined in \"Foo\".\n");
}

TEST_F(ParserValidationErrorTest, FieldTypeError) {
  ExpectHasValidationErrors(
    "message Foo {\n"
    "  optional Baz bar = 1;\n"
    "}\n",
    "1:11: \"Baz\" is not defined.\n");
}

TEST_F(ParserValidationErrorTest, FieldNumberError) {
  ExpectHasValidationErrors(
    "message Foo {\n"
    "  optional int32 bar = 0;\n"
    "}\n",
    "1:23: Field numbers must be positive integers.\n");
}

TEST_F(ParserValidationErrorTest, FieldExtendeeError) {
  ExpectHasValidationErrors(
    "extend Baz { optional int32 bar = 1; }\n",
    "0:7: \"Baz\" is not defined.\n");
}

TEST_F(ParserValidationErrorTest, FieldDefaultValueError) {
  ExpectHasValidationErrors(
    "enum Baz { QUX = 1; }\n"
    "message Foo {\n"
    "  optional Baz bar = 1 [default=NO_SUCH_VALUE];\n"
    "}\n",
    "2:32: Enum type \"Baz\" has no value named \"NO_SUCH_VALUE\".\n");
}

TEST_F(ParserValidationErrorTest, FileOptionNameError) {
  ExpectHasValidationErrors(
    "option foo = 5;",
    "0:7: Option \"foo\" unknown.\n");
}

TEST_F(ParserValidationErrorTest, FileOptionValueError) {
  ExpectHasValidationErrors(
    "option java_outer_classname = 5;",
    "0:30: Value must be quoted string for string option "
    "\"google.protobuf.FileOptions.java_outer_classname\".\n");
}

TEST_F(ParserValidationErrorTest, FieldOptionNameError) {
  ExpectHasValidationErrors(
    "message Foo {\n"
    "  optional bool bar = 1 [foo=1];\n"
    "}\n",
    "1:25: Option \"foo\" unknown.\n");
}

TEST_F(ParserValidationErrorTest, FieldOptionValueError) {
  ExpectHasValidationErrors(
    "message Foo {\n"
    "  optional int32 bar = 1 [ctype=1];\n"
    "}\n",
    "1:32: Value must be identifier for enum-valued option "
    "\"google.protobuf.FieldOptions.ctype\".\n");
}

TEST_F(ParserValidationErrorTest, ExtensionRangeNumberError) {
  ExpectHasValidationErrors(
    "message Foo {\n"
    "  extensions 0;\n"
    "}\n",
    "1:13: Extension numbers must be positive integers.\n");
}

TEST_F(ParserValidationErrorTest, EnumNameError) {
  ExpectHasValidationErrors(
    "enum Foo {A = 1;}\n"
    "enum Foo {B = 1;}\n",
    "1:5: \"Foo\" is already defined.\n");
}

TEST_F(ParserValidationErrorTest, EnumValueNameError) {
  ExpectHasValidationErrors(
    "enum Foo {\n"
    "  BAR = 1;\n"
    "  BAR = 1;\n"
    "}\n",
    "2:2: \"BAR\" is already defined.\n");
}

TEST_F(ParserValidationErrorTest, ServiceNameError) {
  ExpectHasValidationErrors(
    "service Foo {}\n"
    "service Foo {}\n",
    "1:8: \"Foo\" is already defined.\n");
}

TEST_F(ParserValidationErrorTest, MethodNameError) {
  ExpectHasValidationErrors(
    "message Baz {}\n"
    "service Foo {\n"
    "  rpc Bar(Baz) returns(Baz);\n"
    "  rpc Bar(Baz) returns(Baz);\n"
    "}\n",
    "3:6: \"Bar\" is already defined in \"Foo\".\n");
}

TEST_F(ParserValidationErrorTest, MethodInputTypeError) {
  ExpectHasValidationErrors(
    "message Baz {}\n"
    "service Foo {\n"
    "  rpc Bar(Qux) returns(Baz);\n"
    "}\n",
    "2:10: \"Qux\" is not defined.\n");
}

TEST_F(ParserValidationErrorTest, MethodOutputTypeError) {
  ExpectHasValidationErrors(
    "message Baz {}\n"
    "service Foo {\n"
    "  rpc Bar(Baz) returns(Qux);\n"
    "}\n",
    "2:23: \"Qux\" is not defined.\n");
}

// ===================================================================
// Test that the output from FileDescriptor::DebugString() (and all other
// descriptor types) is parseable, and results in the same Descriptor
// definitions again afoter parsing (not, however, that the order of messages
// cannot be guaranteed to be the same)

typedef ParserTest ParseDecriptorDebugTest;

class CompareDescriptorNames {
 public:
  bool operator()(const DescriptorProto* left, const DescriptorProto* right) {
    return left->name() < right->name();
  }
};

// Sorts nested DescriptorProtos of a DescriptoProto, by name.
void SortMessages(DescriptorProto *descriptor_proto) {
  int size = descriptor_proto->nested_type_size();
  // recursively sort; we can't guarantee the order of nested messages either
  for (int i = 0; i < size; ++i) {
    SortMessages(descriptor_proto->mutable_nested_type(i));
  }
  DescriptorProto **data =
    descriptor_proto->mutable_nested_type()->mutable_data();
  sort(data, data + size, CompareDescriptorNames());
}

// Sorts DescriptorProtos belonging to a FileDescriptorProto, by name.
void SortMessages(FileDescriptorProto *file_descriptor_proto) {
  int size = file_descriptor_proto->message_type_size();
  // recursively sort; we can't guarantee the order of nested messages either
  for (int i = 0; i < size; ++i) {
    SortMessages(file_descriptor_proto->mutable_message_type(i));
  }
  DescriptorProto **data =
    file_descriptor_proto->mutable_message_type()->mutable_data();
  sort(data, data + size, CompareDescriptorNames());
}

TEST_F(ParseDecriptorDebugTest, TestAllDescriptorTypes) {
  const FileDescriptor* original_file =
     protobuf_unittest::TestAllTypes::descriptor()->file();
  FileDescriptorProto expected;
  original_file->CopyTo(&expected);

  // Get the DebugString of the unittest.proto FileDecriptor, which includes
  // all other descriptor types
  string debug_string = original_file->DebugString();

  // Parse the debug string
  SetupParser(debug_string.c_str());
  FileDescriptorProto parsed;
  parser_->Parse(input_.get(), &parsed);
  EXPECT_EQ(io::Tokenizer::TYPE_END, input_->current().type);
  ASSERT_EQ("", error_collector_.text_);

  // We now have a FileDescriptorProto, but to compare with the expected we
  // need to link to a FileDecriptor, then output back to a proto. We'll
  // also need to give it the same name as the original.
  parsed.set_name("google/protobuf/unittest.proto");
  // We need the imported dependency before we can build our parsed proto
  const FileDescriptor* import =
       protobuf_unittest_import::ImportMessage::descriptor()->file();
  FileDescriptorProto import_proto;
  import->CopyTo(&import_proto);
  ASSERT_TRUE(pool_.BuildFile(import_proto) != NULL);
  const FileDescriptor* actual = pool_.BuildFile(parsed);
  parsed.Clear();
  actual->CopyTo(&parsed);
  ASSERT_TRUE(actual != NULL);

  // The messages might be in different orders, making them hard to compare.
  // So, sort the messages in the descriptor protos (including nested messages,
  // recursively).
  SortMessages(&expected);
  SortMessages(&parsed);

  // I really wanted to use StringDiff here for the debug output on fail,
  // but the strings are too long for it, and if I increase its max size,
  // we get a memory allocation failure :(
  EXPECT_EQ(expected.DebugString(), parsed.DebugString());
}

// ===================================================================

}  // anonymous namespace

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
